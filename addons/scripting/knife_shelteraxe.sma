/*========================================
  插件：DNS 斧头
  版本：1.0-fixed
  说明：使用 knifeapi 注册并给予斧头
        通过 Ham_AddPlayerItem 拦截默认手枪
        禁用 knifeapi 的 R 键切换菜单（延迟设置）
  依赖：knifeapi.amxx（必须先加载）
========================================*/

#include <amxmodx>
#include <knifeapi>
#include <hamsandwich>
#include <fakemeta>
#include <cstrike>

new g_ShelterAxe

#define V_MODEL "models/DayNightSurvive/weapons/v_zsh_shelteraxe.mdl"
#define P_MODEL "models/DayNightSurvive/weapons/p_zsh_shelteraxe.mdl"

#define SOUND_DRAW "DayNightSurvive/weapons/shelteraxe_draw.wav"
#define SOUND_HIT "DayNightSurvive/weapons/shelteraxe_slash_hit.wav"
#define SOUND_STAB "DayNightSurvive/weapons/shelteraxe_stab_hit.wav"
#define SOUND_WALL "DayNightSurvive/weapons/shelteraxe_hitwall.wav"
new const SoundWiff[][] = {
    "DayNightSurvive/weapons/shelteraxe_slash1.wav",
    "DayNightSurvive/weapons/shelteraxe_slash2.wav",
    "DayNightSurvive/weapons/shelteraxe_slash3.wav"
}

// 武器私有数据偏移
const XO_WEAPONS = 4
const m_iId = 43

public plugin_precache()
{
    precache_model(V_MODEL)
    precache_model(P_MODEL)

    precache_sound(SOUND_DRAW)
    precache_sound(SOUND_HIT)
    precache_sound(SOUND_STAB)
    precache_sound(SOUND_WALL)
    for(new i = 0; i < sizeof SoundWiff; i++)
        precache_sound(SoundWiff[i])
}

public plugin_init()
{
    register_plugin("DNS 斧头", "1.0-fixed", "Reconstructor")

    // 修复：延迟 0.1 秒设置 cvar，确保 knifeapi 已加载并注册该 cvar
    set_task(0.1, "DisableSwitchMenu")

    // 注册斧头
    g_ShelterAxe = Knife_Register(
        "斧头",
        V_MODEL,
        P_MODEL,
        "",   // 无世界模型，使用默认
        SOUND_DRAW,
        SOUND_HIT,
        SOUND_STAB,
        SoundWiff[random_num(0, 2)],
        SOUND_WALL,
        1.0,
        1.0
    )

    Knife_SetProperty(g_ShelterAxe, KN_CLL_PrimaryNextAttack, 0.7)
    Knife_SetProperty(g_ShelterAxe, KN_CLL_SecondaryNextAttack, 1.1)
    Knife_SetProperty(g_ShelterAxe, KN_CLL_IgnoreFriendlyFire, true)

    RegisterHam(Ham_AddPlayerItem, "player", "FwdAddPlayerItem")
    RegisterHam(Ham_Spawn, "player", "PlayerSpawn", true)
}

public DisableSwitchMenu()
{
    set_cvar_num("knifeapi_switchmenu", 0)
}

// 拦截物品添加：阻止默认手枪被加入背包
public FwdAddPlayerItem(id, entity)
{
    if(pev_valid(entity) != 2)
        return HAM_IGNORED

    new weaponID = get_pdata_int(entity, m_iId, XO_WEAPONS)

    if(weaponID == CSW_USP || weaponID == CSW_GLOCK18)
        return HAM_SUPERCEDE

    return HAM_IGNORED
}

public PlayerSpawn(Player)
{
    if(!is_user_alive(Player)) return

    if(!Knife_PlayerHas(Player, g_ShelterAxe))
        Knife_PlayerGive(Player, g_ShelterAxe, true)
}