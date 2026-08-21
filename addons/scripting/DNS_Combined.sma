/*========================================
  插件：DNS 昼夜求生（模块化重构版）
  版本：2026-refactor-modular
  说明：主框架仅负责核心状态、玩家基本属性、模块加载
========================================*/

#include <amxmodx>
#include <fakemeta>
#include <hamsandwich>
#include <xs>
#include <superspawns>
#include <fun>
#include <cstrike>

// ============================================================
//  常量与宏定义
// ============================================================

#define MAX_PLAYERS 32
#define MAX_GENERATORS 32
#define MAX_BUILDINGS 64
#define MORNING 0
#define NIGHT 15
#define TRANSITION_START 40.0

enum { WOODS=0, STEEL=1, FOOD=2, POWER=3 }

// ============================================================
//  全局变量（所有 .inl 共享）
// ============================================================

// CVAR
new cvar_day_len, cvar_night_len, cvar_startmoney, cvar_win_days

// 游戏状态
new bool:g_GameStarted
new g_Days
new bool:g_IsNight
new Float:g_NextSwitch
new g_CurrentLight

// 玩家资源
new g_Woods[MAX_PLAYERS+1]
new g_Steel[MAX_PLAYERS+1]
new g_Food[MAX_PLAYERS+1]
new g_MaxWoods[MAX_PLAYERS+1]
new g_MaxSteel[MAX_PLAYERS+1]
new g_MaxFood[MAX_PLAYERS+1]
new g_Money[MAX_PLAYERS+1]
new Float:g_NextHud[MAX_PLAYERS+1]

// 团队资源
new g_TeamWoods, g_TeamSteel, g_TeamPower
new g_TeamMaxWoods, g_TeamMaxSteel, g_TeamMaxPower

// 资源模型（由 resource.inl 管理）
new g_ModelWood, g_ModelSteel, g_ModelFood
new g_ModelGibsWood, g_ModelGibsSteel, g_ModelGibsFood

// 发电机系统（由 generator.inl 管理）
new g_GeneratorCount
new g_GeneratorEntity[MAX_GENERATORS]
new Float:g_GenNextTick[MAX_GENERATORS]
new g_GenOwner[MAX_GENERATORS]
new g_GenTotalPower[MAX_GENERATORS]
new g_GenCyclePower[MAX_GENERATORS]
new g_GenThreshold[MAX_GENERATORS]
new Float:g_GenLastDamage[MAX_GENERATORS]

// 建筑系统（由 building_core.inl 管理）
new bool:g_IsBuilding[MAX_PLAYERS+1]
new g_Blueprint[MAX_PLAYERS+1]
new g_BuildingRotate[MAX_PLAYERS+1]
new g_BuildingId[MAX_PLAYERS+1]

// 建筑注册表（由 building_core.inl 管理）
new gBuildingPowerCost[MAX_BUILDINGS] 
new gBuildingCount
new gBuildingName[MAX_BUILDINGS][64]
new Float:gBuildingHealth[MAX_BUILDINGS]
new Float:gBuildingMins[MAX_BUILDINGS][3]
new Float:gBuildingMaxs[MAX_BUILDINGS][3]
new gBuildingWoodCost[MAX_BUILDINGS]
new gBuildingSteelCost[MAX_BUILDINGS]
new gBuildingModelIndex[MAX_BUILDINGS]
new const Float:BUILD_SPEED = 8.0
new const Float:BUILD_COMPLETION = 60.0
new const Float:BUILD_MAX_HEALTH = 1000.0

// Custom Forwards（用于模块间通信）
enum _:TOTAL_FORWARDS
{
    FW_BUILDING_PUT_POST = 0,
    FW_BUILDING_COMPLETE,
    FW_BUILDING_KILLED,
    FW_CMDSTART
}
new g_Forwards[TOTAL_FORWARDS]
new g_ForwardResult

// 特殊建筑ID（由 generator.inl / warehouse.inl 赋值）
new g_GeneratorBuildingId
new g_WareHouseId
new const WAREHOUSE_CAPACITY = 30

// ★ E 键图标 Sprite 索引
new g_ButtonESprite

// ============================================================
//  音效宏定义
// ============================================================

#define SOUND_GET     "DayNightSurvive/point5.wav"
#define SOUND_DAY     "DayNightSurvive/skill_bonus.wav"
#define SOUND_NIGHT   "DayNightSurvive/zombies_amb.wav"

// ============================================================
//  模块引用（顺序：基础→核心→特殊建筑）
// ============================================================

#include "daynightsurvive/cvars.inl"
#include "daynightsurvive/utils.inl"
#include "daynightsurvive/resource.inl"
#include "daynightsurvive/core.inl"
#include "daynightsurvive/player.inl"
#include "daynightsurvive/building_core.inl"
#include "daynightsurvive/generator.inl"
#include "daynightsurvive/warehouse.inl"

// ============================================================
//  插件入口
// ============================================================

public plugin_init()
{
    register_plugin("DNS 昼夜求生", "2026-refactor-modular", "Reconstructor")
    
    RegisterCvars()
    
    set_cvar_num("mp_timelimit", 0)
    set_task(2.0, "ForceTimeLimit", _, _, _, "b")
    
    register_message(get_user_msgid("Money"), "MsgMoney")
    register_forward(FM_StartFrame, "StartFrame")
    register_forward(FM_CmdStart, "CmdStart_Pre")
    register_forward(FM_PlayerPostThink, "PlayerPostThink")
    register_forward(FM_TraceLine, "TraceLine_Post", 1)
    
    RegisterHam(Ham_Spawn, "player", "PlayerSpawn", true)
    RegisterHam(Ham_Killed, "player", "PlayerKilled")
    
    RegisterHam(Ham_TraceAttack, "info_target", "TraceAttack_Resource")
    RegisterHam(Ham_TraceAttack, "info_target", "Ham_TraceAttack_Building")
    RegisterHam(Ham_Killed, "info_target", "Killed_Resource")
    RegisterHam(Ham_Killed, "info_target", "GeneratorKilled")
    
    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_knife", "Ham_Knife_PrimaryAttack_Post", 1)
    RegisterHam(Ham_Weapon_SecondaryAttack, "weapon_knife", "Ham_Knife_SecondaryAttack_Post", 1)
    RegisterHam(Ham_Item_Holster, "weapon_knife", "Ham_Knife_Holster_Post", 1)
    
    register_clcmd("buymenu", "CmdBuildMenu")
    register_clcmd("build", "CmdBuildMenu")
    register_clcmd("chooseteam", "CmdStuckMenu")
    
    // 注册 Forwards
    g_Forwards[FW_BUILDING_PUT_POST] = CreateMultiForward("DNS_BuildingPut_Post", ET_IGNORE, FP_CELL, FP_CELL, FP_CELL)
    g_Forwards[FW_BUILDING_COMPLETE] = CreateMultiForward("DNS_BuildingComplete_Post", ET_IGNORE, FP_CELL, FP_CELL, FP_CELL)
    g_Forwards[FW_BUILDING_KILLED] = CreateMultiForward("DNS_BuildingKilled_Post", ET_IGNORE, FP_CELL, FP_CELL, FP_CELL)
    g_Forwards[FW_CMDSTART] = CreateMultiForward("DNS_CmdStart_Pre", ET_IGNORE, FP_CELL, FP_CELL, FP_CELL)
    
    // 注册所有特殊建筑
    RegisterGeneratorBuilding()
    RegisterWarehouseBuilding()
    
    // 注册仓库的消息模式（用于输入数量）
    register_clcmd("InsertWarehouseAmount", "InsertWarehouseAmount")
    
    InitGameState()
    SetLight(MORNING)
}

public plugin_cfg()
{
    set_task(2.0, "DelayedRestart")
    SsInit(100.0)
}

public plugin_precache()
{
    PrecacheResources()
    PrecacheBuildingCore()
    PrecacheGenerator()
    PrecacheWarehouse()
}

public DelayedRestart()
{
    server_cmd("sv_restartround 5")
    set_task(6.0, "StartGame")
}