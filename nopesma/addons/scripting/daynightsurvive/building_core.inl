// ============================================================
//  building_core.inl - 建筑核心（通用逻辑 + 音效 + 特殊建筑调度）
//  功能：菜单、蓝图、旋转、放置、碰撞检测、进度条、攻击交互
//  说明：所有建筑共用的基础逻辑，特殊建筑逻辑统一在此调度
// ============================================================

#define ATTACK_BUILD   1
#define ATTACK_DEMOLISH 2

// 音效宏（由本模块预缓存）
#define SOUND_BUILD_SELECT "DayNightSurvive/object_select.wav"
#define SOUND_BUILD_PUT "DayNightSurvive/build2.wav"
#define SOUND_BUILD_COMPLETE "DayNightSurvive/build_complete.wav"
#define SOUND_BUILD_NO "DayNightSurvive/build_impossible.wav"
#define SOUND_BUILD_CANCEL "DayNightSurvive/build_impossible.wav"
#define SOUND_BUILDING_CRASH "DayNightSurvive/crash.wav"

// ============================================================
//  特殊建筑注册表（用于统一管理特殊建筑ID）
// ============================================================
#define MAX_SPECIAL_BUILDINGS 16

new g_SpecialBuildingId[MAX_SPECIAL_BUILDINGS]
new g_SpecialBuildingCount

// 注册一个特殊建筑ID（由 generator.inl / warehouse.inl 调用）
RegisterSpecialBuilding(BuildingId)
{
    if (g_SpecialBuildingCount < MAX_SPECIAL_BUILDINGS)
    {
        g_SpecialBuildingId[g_SpecialBuildingCount] = BuildingId
        g_SpecialBuildingCount++
    }
}

// 检查某个ID是否为特殊建筑
IsSpecialBuilding(BuildingId)
{
    for (new i = 0; i < g_SpecialBuildingCount; i++)
    {
        if (g_SpecialBuildingId[i] == BuildingId)
            return 1
    }
    return 0
}

// ============================================================
//  预缓存
// ============================================================
PrecacheBuildingCore()
{
    precache_sound(SOUND_BUILD_SELECT)
    precache_sound(SOUND_BUILD_PUT)
    precache_sound(SOUND_BUILD_COMPLETE)
    precache_sound(SOUND_BUILD_NO)
    precache_sound(SOUND_BUILD_CANCEL)
    precache_sound(SOUND_BUILDING_CRASH)
}

// ============================================================
//  建筑注册（每个建筑有自己的木材/钢铁/电力消耗）
// ============================================================
RegisterBuilding(name[], Float:health, Float:mins[3], Float:maxs[3], model[], cost_wood, cost_steel, cost_power)
{
    if(gBuildingCount >= MAX_BUILDINGS)
        return -1
    copy(gBuildingName[gBuildingCount], charsmax(gBuildingName[]), name)
    gBuildingHealth[gBuildingCount] = health
    xs_vec_copy(mins, gBuildingMins[gBuildingCount])
    xs_vec_copy(maxs, gBuildingMaxs[gBuildingCount])
    gBuildingWoodCost[gBuildingCount] = cost_wood
    gBuildingSteelCost[gBuildingCount] = cost_steel
    gBuildingPowerCost[gBuildingCount] = cost_power
    gBuildingModelIndex[gBuildingCount] = engfunc(EngFunc_PrecacheModel, model)
    gBuildingCount++
    return gBuildingCount - 1
}

// 判断建筑是否处于完成状态（辅助函数）
stock IsBuildingCompleted(iEntity)
{
    if(!pev_valid(iEntity)) return 0
    new classname[32]
    pev(iEntity, pev_classname, classname, charsmax(classname))
    if(!equal(classname, "dns_building")) return 0
    new Float:completion
    pev(iEntity, pev_fuser1, completion)
    return (completion <= 0.0) ? 1 : 0
}

// ============================================================
//  建筑菜单（B键）
// ============================================================
public CmdBuildMenu(id)
{
    if(!g_GameStarted)
    {
        client_print(id, print_center, "游戏尚未开始！")
        return PLUGIN_HANDLED
    }
    if(!is_user_alive(id))
    {
        client_print(id, print_center, "只有活着的玩家才能建造！")
        return PLUGIN_HANDLED
    }
    if(g_IsBuilding[id] && pev_valid(g_Blueprint[id]))
    {
        client_print(id, print_center, "你已经在建造中！")
        return PLUGIN_HANDLED
    }
    
    new menu = menu_create("\y建造菜单", "BuildMenuHandler")
    for(new i = 0; i < gBuildingCount; i++)
    {
        new item[64]
        format(item, charsmax(item), "%s (木材:%d 钢铁:%d 电力:%d)", gBuildingName[i], gBuildingWoodCost[i], gBuildingSteelCost[i], gBuildingPowerCost[i])
        menu_additem(menu, item)
    }
    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL)
    menu_display(id, menu)
    return PLUGIN_HANDLED
}

public BuildMenuHandler(id, menu, item)
{
    if(item == MENU_EXIT)
    {
        menu_destroy(menu)
        return PLUGIN_HANDLED
    }

    // 检查对应建筑的材料是否足够
    if(g_Woods[id] < gBuildingWoodCost[item] || g_Steel[id] < gBuildingSteelCost[item] || g_TeamPower < gBuildingPowerCost[item])
    {
        client_print(id, print_center, "材料不足！需要 %d 木材 + %d 钢铁 + %d 电力", gBuildingWoodCost[item], gBuildingSteelCost[item], gBuildingPowerCost[item])
        menu_destroy(menu)
        return PLUGIN_HANDLED
    }
    
    g_BuildingId[id] = item
    StartBlueprint(id, item)
    menu_destroy(menu)
    return PLUGIN_HANDLED
}

// ============================================================
//  蓝图生成
// ============================================================
StartBlueprint(id, BuildingId)
{
    if(g_IsBuilding[id] && pev_valid(g_Blueprint[id]))
    {
        client_print(id, print_center, "已在建造中")
        return
    }
    
    new Float:origin[3], Float:vOrigin[3]
    pev(id, pev_origin, vOrigin)
    get_aim_origin_vector(id, 150.0, 0.0, 0.0, origin)
    origin[2] = vOrigin[2] - (pev(id, pev_flags) & FL_DUCKING ? 15.0 : 26.0)
    
    new ent = engfunc(EngFunc_CreateNamedEntity, engfunc(EngFunc_AllocString, "info_target"))
    if(!ent)
    {
        client_print(id, print_center, "创建蓝图失败！")
        return
    }
    
    set_pev(ent, pev_classname, "dns_blueprint")
    set_pev(ent, pev_solid, SOLID_BBOX)
    set_pev(ent, pev_movetype, MOVETYPE_NONE)
    set_pev(ent, pev_modelindex, gBuildingModelIndex[BuildingId])
    set_pev(ent, pev_rendermode, kRenderTransColor)
    set_pev(ent, pev_renderamt, 100.0)
    new Float:white[3] = {255.0, 255.0, 255.0}
    set_pev(ent, pev_rendercolor, white)
    set_pev(ent, pev_takedamage, DAMAGE_NO)
    set_pev(ent, pev_owner, id)
    set_pev(ent, pev_iuser1, BuildingId)
    
    engfunc(EngFunc_SetSize, ent, gBuildingMins[BuildingId], gBuildingMaxs[BuildingId])
    engfunc(EngFunc_SetOrigin, ent, origin)
    engfunc(EngFunc_DropToFloor, ent)
    
    g_Blueprint[id] = ent
    g_IsBuilding[id] = true
    g_BuildingRotate[id] = 0
    
    set_pev(ent, pev_vuser1, gBuildingMins[BuildingId])
    set_pev(ent, pev_vuser2, gBuildingMaxs[BuildingId])
    
    set_hudmessage(255, 255, 0, -1.0, 0.70, 0, 0.5, 999.0, 0.0, 0.0, 4)
    show_hudmessage(id, "按 [左键] 放置 | 按 [R] 旋转 | 按 [右键] 取消")
    
    engfunc(EngFunc_EmitSound, id, CHAN_STATIC, SOUND_BUILD_SELECT, 1.0, ATTN_NORM, 0, PITCH_NORM)
}

// ============================================================
//  旋转蓝图
// ============================================================
RotateBlueprint(id)
{
    new ent = g_Blueprint[id]
    if(!pev_valid(ent)) return
    
    new Float:angles[3]
    pev(ent, pev_angles, angles)
    angles[1] += 90.0
    set_pev(ent, pev_angles, angles)
    
    g_BuildingRotate[id] = (g_BuildingRotate[id] + 1) % 4
    
    new BuildingId = pev(ent, pev_iuser1)
    new Float:mins[3], Float:maxs[3]
    if(g_BuildingRotate[id] == 1 || g_BuildingRotate[id] == 3)
    {
        mins[0] = gBuildingMins[BuildingId][1]
        mins[1] = gBuildingMins[BuildingId][0]
        mins[2] = gBuildingMins[BuildingId][2]
        maxs[0] = gBuildingMaxs[BuildingId][1]
        maxs[1] = gBuildingMaxs[BuildingId][0]
        maxs[2] = gBuildingMaxs[BuildingId][2]
    }
    else
    {
        xs_vec_copy(gBuildingMins[BuildingId], mins)
        xs_vec_copy(gBuildingMaxs[BuildingId], maxs)
    }
    engfunc(EngFunc_SetSize, ent, mins, maxs)
    engfunc(EngFunc_DropToFloor, ent)
}

// ============================================================
//  取消建造
// ============================================================
CancelBuilding(id)
{
    if(!g_IsBuilding[id] || !pev_valid(g_Blueprint[id]))
        return
    
    if(pev_valid(g_Blueprint[id]))
    {
        engfunc(EngFunc_RemoveEntity, g_Blueprint[id])
        g_Blueprint[id] = 0
    }
    
    client_print(id, print_center, "已取消建造")
    engfunc(EngFunc_EmitSound, id, CHAN_STATIC, SOUND_BUILD_CANCEL, 1.0, ATTN_NORM, 0, PITCH_NORM)
    
    set_hudmessage(0, 0, 0, -1.0, 0.70, 0, 0.0, 0.1, 0.0, 0.0, 4)
    show_hudmessage(id, "")
    
    g_IsBuilding[id] = false
    g_BuildingRotate[id] = 0
}

// ============================================================
//  CmdStart（仅处理旋转和取消 + 触发仓库E键）
// ============================================================
public CmdStart_Pre(id, uc_handle, seed)
{
    if(!g_GameStarted || !is_user_alive(id))
        return
    
    if(g_IsBuilding[id] && pev_valid(g_Blueprint[id]))
    {
        new iButton = get_uc(uc_handle, UC_Buttons)
        new iOldButton = pev(id, pev_oldbuttons)
        
        if(iButton & IN_RELOAD && !(iOldButton & IN_RELOAD))
        {
            RotateBlueprint(id)
        }
        
        if(iButton & IN_ATTACK2 && !(iOldButton & IN_ATTACK2))
        {
            CancelBuilding(id)
        }
    }
    
    ExecuteForward(g_Forwards[FW_CMDSTART], g_ForwardResult, id, uc_handle, seed);
}

// ============================================================
//  ★ Ham 挂钩：建筑被攻击（阻止默认伤害）
// ============================================================
public Ham_TraceAttack_Building(ent, attacker, Float:damage, Float:dir[3], ptr, bits)
{
    return HAM_SUPERCEDE;
}

// ============================================================
//  ★ Ham 挂钩：左键攻击（处理蓝图放置 + 已放置建筑）
// ============================================================
public Ham_Knife_PrimaryAttack_Post(iWpEntity)
{
    new iPlayer = pev(iWpEntity, pev_owner)
    if(!is_user_alive(iPlayer)) return
    
    if(g_IsBuilding[iPlayer] && pev_valid(g_Blueprint[iPlayer]))
    {
        TryPlaceBuilding(iPlayer)
        return
    }
    
    new ent, body;
    get_user_aiming(iPlayer, ent, body, 65);
    if(!pev_valid(ent)) return
    
    new classname[32]
    pev(ent, pev_classname, classname, charsmax(classname))
    
    if(equal(classname, "dns_building"))
    {
        HandleBuildingAttack(iPlayer, ent, ATTACK_BUILD)
    }
}

// ============================================================
//  ★ Ham 挂钩：右键攻击（处理蓝图取消 + 已放置建筑）
// ============================================================
public Ham_Knife_SecondaryAttack_Post(iWpEntity)
{
    new iPlayer = pev(iWpEntity, pev_owner)
    if(!is_user_alive(iPlayer)) return
    
    if(g_IsBuilding[iPlayer] && pev_valid(g_Blueprint[iPlayer]))
    {
        CancelBuilding(iPlayer)
        set_pdata_float(iWpEntity, 47, 2.0, 4)
        return
    }
    
    new ent, body;
    get_user_aiming(iPlayer, ent, body, 65);
    if(!pev_valid(ent)) return
    
    new classname[32]
    pev(ent, pev_classname, classname, charsmax(classname))
    
    if(equal(classname, "dns_building"))
    {
        HandleBuildingAttack(iPlayer, ent, ATTACK_DEMOLISH)
    }
}

// ============================================================
//  ★ Ham 挂钩：收刀自动取消蓝图
// ============================================================
public Ham_Knife_Holster_Post(iWpEntity)
{
    new iPlayer = pev(iWpEntity, pev_owner)
    if(!is_user_alive(iPlayer)) return
    
    if(g_IsBuilding[iPlayer] && pev_valid(g_Blueprint[iPlayer]))
    {
        CancelBuilding(iPlayer)
    }
}

// ============================================================
//  处理建筑攻击（左键建造/修复，右键拆除）
// ============================================================
HandleBuildingAttack(iPlayer, ent, attack_type)
{
    new Float:completion;
    pev(ent, pev_fuser1, completion);
    new Float:max_completion;
    pev(ent, pev_fuser2, max_completion);
    new BuildingId = pev(ent, pev_iuser1);
    
    // 火花特效
    new iEnd[3]
    get_user_origin(iPlayer, iEnd, 3)
    new Float:flEnd[3]
    IVecFVec(iEnd, flEnd)
    message_begin(MSG_BROADCAST, SVC_TEMPENTITY)
    write_byte(TE_SPARKS)
    engfunc(EngFunc_WriteCoord, flEnd[0])
    engfunc(EngFunc_WriteCoord, flEnd[1])
    engfunc(EngFunc_WriteCoord, flEnd[2])
    message_end()
    
    if (attack_type == ATTACK_BUILD)
    {
        if (completion <= 0.0)
        {
            new Float:health, Float:max_health;
            pev(ent, pev_health, health);
            pev(ent, pev_max_health, max_health);
            if (health < max_health)
            {
                if (g_Food[iPlayer] <= 0)
                {
                    client_print(iPlayer, print_center, "体力不足！");
                    return;
                }
                g_Food[iPlayer]--;
                health += 10.0;
                if (health > max_health) health = max_health;
                set_pev(ent, pev_health, health);
            }
            else
            {
                return;
            }
        }
        else
        {
            if (g_Food[iPlayer] <= 0)
            {
                client_print(iPlayer, print_center, "体力不足！");
                return;
            }
            g_Food[iPlayer]--;
            completion -= BUILD_SPEED;
            if (completion < 0.0) completion = 0.0;
            set_pev(ent, pev_fuser1, completion);
            
            new Float:health, Float:max_health;
            pev(ent, pev_health, health);
            pev(ent, pev_max_health, max_health);
            health += BUILD_SPEED * (BUILD_MAX_HEALTH / BUILD_COMPLETION);
            if (health > max_health) health = max_health;
            set_pev(ent, pev_health, health);
            
            if (completion <= 0.0)
            {
                set_pev(ent, pev_fuser1, -1.0);
                set_pev(ent, pev_fuser2, -1.0);
                set_pev(ent, pev_health, BUILD_MAX_HEALTH);
                set_pev(ent, pev_max_health, BUILD_MAX_HEALTH);
                set_pev(ent, pev_rendermode, kRenderNormal);
                set_pev(ent, pev_renderamt, 255.0);
                engfunc(EngFunc_EmitSound, ent, CHAN_STATIC, SOUND_BUILD_COMPLETE, 1.0, ATTN_NORM, 0, PITCH_NORM);
                
                ExecuteForward(g_Forwards[FW_BUILDING_COMPLETE], g_ForwardResult, iPlayer, ent, BuildingId);
            }
        }
    }
    else // ATTACK_DEMOLISH
    {
        if (g_Food[iPlayer] <= 0)
        {
            client_print(iPlayer, print_center, "体力不足！");
            return;
        }
        g_Food[iPlayer]--;
        
        if (completion > 0.0)
        {
            completion += BUILD_SPEED;
            if (completion > max_completion) completion = max_completion;
            set_pev(ent, pev_fuser1, completion);
            
            new Float:health, Float:max_health;
            pev(ent, pev_health, health);
            pev(ent, pev_max_health, max_health);
            health -= BUILD_SPEED * (BUILD_MAX_HEALTH / BUILD_COMPLETION);
            if (health < 0.0) health = 0.0;
            set_pev(ent, pev_health, health);
            
            if (completion >= max_completion)
            {
                client_print(iPlayer, print_center, "建筑被拆除！");
                engfunc(EngFunc_EmitSound, ent, CHAN_STATIC, SOUND_BUILDING_CRASH, 1.0, ATTN_NORM, 0, PITCH_NORM);
                ExecuteForward(g_Forwards[FW_BUILDING_KILLED], g_ForwardResult, ent, iPlayer, BuildingId);
                DestroyBuilding(ent);
            }
        }
        else
        {
            new Float:health, Float:max_health;
            pev(ent, pev_health, health);
            pev(ent, pev_max_health, max_health);
            health -= max_health / 3.0;
            if (health <= 0.0)
            {
                client_print(iPlayer, print_center, "建筑被摧毁！");
                engfunc(EngFunc_EmitSound, ent, CHAN_STATIC, SOUND_BUILDING_CRASH, 1.0, ATTN_NORM, 0, PITCH_NORM);
                ExecuteForward(g_Forwards[FW_BUILDING_KILLED], g_ForwardResult, ent, iPlayer, BuildingId);
                DestroyBuilding(ent);
            }
            else
            {
                set_pev(ent, pev_health, health);
                client_print(iPlayer, print_center, "建筑耐久: %.0f/%.0f", health, max_health);
            }
        }
    }
}

// ============================================================
//  尝试放置建筑（扣除建筑自身的材料消耗 + 电力）
//  包含贴地检测和禁止建在其他实体上（参考原版）
// ============================================================
TryPlaceBuilding(id)
{
    if(!g_IsBuilding[id] || !pev_valid(g_Blueprint[id]))
        return
    
    new blueprint = g_Blueprint[id]
    new BuildingId = pev(blueprint, pev_iuser1)
    new Float:origin[3]
    pev(blueprint, pev_origin, origin)
    
    // ★ 贴地检测（参考原版 fm_distance_to_floor）
    if(fm_distance_to_floor(blueprint) >= 1.0)
    {
        client_print(id, print_center, "建筑必须放置在地面上！")
        engfunc(EngFunc_EmitSound, id, CHAN_STATIC, SOUND_BUILD_NO, 1.0, ATTN_NORM, 0, PITCH_NORM)
        return
    }
    
    // ★ 禁止建在其他实体上（参考原版 fm_floor_entity）
    if(fm_floor_entity(blueprint))
    {
        client_print(id, print_center, "不能在其他实体上建造！")
        engfunc(EngFunc_EmitSound, id, CHAN_STATIC, SOUND_BUILD_NO, 1.0, ATTN_NORM, 0, PITCH_NORM)
        return
    }
    
    new Float:fMin[3], Float:fMax[3]
    if(g_BuildingRotate[id] == 1 || g_BuildingRotate[id] == 3)
    {
        fMin[0] = gBuildingMins[BuildingId][1]
        fMin[1] = gBuildingMins[BuildingId][0]
        fMin[2] = gBuildingMins[BuildingId][2]
        fMax[0] = gBuildingMaxs[BuildingId][1]
        fMax[1] = gBuildingMaxs[BuildingId][0]
        fMax[2] = gBuildingMaxs[BuildingId][2]
    }
    else
    {
        xs_vec_copy(gBuildingMins[BuildingId], fMin)
        xs_vec_copy(gBuildingMaxs[BuildingId], fMax)
    }
    
    if(!CheckStuck(origin, fMin, fMax))
    {
        client_print(id, print_center, "空间不足，无法放置！")
        engfunc(EngFunc_EmitSound, id, CHAN_STATIC, SOUND_BUILD_NO, 1.0, ATTN_NORM, 0, PITCH_NORM)
        return
    }
    
    if(OverlapInSphere(blueprint, origin, fMin, fMax))
    {
        client_print(id, print_center, "与其他建筑重叠！")
        engfunc(EngFunc_EmitSound, id, CHAN_STATIC, SOUND_BUILD_NO, 1.0, ATTN_NORM, 0, PITCH_NORM)
        return
    }
    
    if(engfunc(EngFunc_PointContents, origin) != CONTENTS_EMPTY)
    {
        client_print(id, print_center, "空间不足，无法放置！")
        engfunc(EngFunc_EmitSound, id, CHAN_STATIC, SOUND_BUILD_NO, 1.0, ATTN_NORM, 0, PITCH_NORM)
        return
    }
    
    // 检查建筑自身的材料消耗
    if(g_Woods[id] < gBuildingWoodCost[BuildingId] || g_Steel[id] < gBuildingSteelCost[BuildingId])
    {
        client_print(id, print_center, "材料不足！需要 %d 木材 + %d 钢铁", gBuildingWoodCost[BuildingId], gBuildingSteelCost[BuildingId])
        return
    }
    
    // 检查电力
    if(g_TeamPower < gBuildingPowerCost[BuildingId])
    {
        client_print(id, print_center, "电力不足！需要 %d 电力", gBuildingPowerCost[BuildingId])
        return
    }
    
    g_Woods[id] -= gBuildingWoodCost[BuildingId]
    g_Steel[id] -= gBuildingSteelCost[BuildingId]
    g_TeamPower -= gBuildingPowerCost[BuildingId]
    
    set_pev(blueprint, pev_classname, "dns_building")
    set_pev(blueprint, pev_targetname, "dns_building_final")
    set_pev(blueprint, pev_solid, SOLID_BBOX)
    set_pev(blueprint, pev_movetype, MOVETYPE_NONE)
    set_pev(blueprint, pev_takedamage, DAMAGE_YES)
    set_pev(blueprint, pev_rendermode, kRenderTransColor)
    set_pev(blueprint, pev_renderamt, 100.0)
    new Float:buildColor[3] = {255.0, 255.0, 255.0}
    set_pev(blueprint, pev_rendercolor, buildColor)
    set_pev(blueprint, pev_effects, 0)
    set_pev(blueprint, pev_fuser1, BUILD_COMPLETION)
    set_pev(blueprint, pev_fuser2, BUILD_COMPLETION)
    set_pev(blueprint, pev_health, 100.0)
    set_pev(blueprint, pev_max_health, BUILD_MAX_HEALTH)
    set_pev(blueprint, pev_iuser1, BuildingId)
    set_pev(blueprint, pev_owner, 0)
    
    engfunc(EngFunc_SetSize, blueprint, fMin, fMax)
    engfunc(EngFunc_SetOrigin, blueprint, origin)
    engfunc(EngFunc_DropToFloor, blueprint)
    
    ExecuteForward(g_Forwards[FW_BUILDING_PUT_POST], g_ForwardResult, id, blueprint, BuildingId);
    
    g_IsBuilding[id] = false
    g_Blueprint[id] = 0
    
    set_hudmessage(0, 0, 0, -1.0, 0.70, 0, 0.0, 0.1, 0.0, 0.0, 4)
    show_hudmessage(id, "")
    
    engfunc(EngFunc_EmitSound, id, CHAN_STATIC, SOUND_BUILD_PUT, 1.0, ATTN_NORM, 0, PITCH_NORM)
    client_print(id, print_center, "建筑已放置，用刀左键建造，右键拆除")
}

// ============================================================
//  CheckStuck（八点检测）
// ============================================================
CheckStuck(Float:Origin[3], Float:fMin[3], Float:fMax[3])
{
    new Float:Origin_F[3], Float:Origin_R[3], Float:Origin_L[3], Float:Origin_B[3]
    xs_vec_copy(Origin, Origin_L)
    xs_vec_copy(Origin, Origin_F)
    xs_vec_copy(Origin, Origin_B)
    xs_vec_copy(Origin, Origin_R)
    
    Origin_F[0] += fMax[0]
    Origin_B[0] += fMin[0]
    Origin_L[1] += fMax[1]
    Origin_R[1] += fMin[1]
    
    if(engfunc(EngFunc_PointContents, Origin_F) != CONTENTS_EMPTY ||
       engfunc(EngFunc_PointContents, Origin_R) != CONTENTS_EMPTY ||
       engfunc(EngFunc_PointContents, Origin_L) != CONTENTS_EMPTY ||
       engfunc(EngFunc_PointContents, Origin_B) != CONTENTS_EMPTY)
        return 0
    
    return 1
}

// ============================================================
//  OverlapInSphere（建筑间重叠检测）
// ============================================================
OverlapInSphere(iEntity, Float:Origin[3], Float:fMin[3], Float:fMax[3])
{
    new Float:entOrigin[3]
    new i = -1
    while((i = engfunc(EngFunc_FindEntityByString, i, "classname", "dns_building")))
    {
        if(i == iEntity) continue
        if(!pev_valid(i)) continue
        
        pev(i, pev_origin, entOrigin)
        if(get_distance_f(Origin, entOrigin) > 150.0) continue
        
        new Float:entMins[3], Float:entMaxs[3]
        pev(i, pev_mins, entMins)
        pev(i, pev_maxs, entMaxs)
        
        if(Origin[0] + fMin[0] < entOrigin[0] + entMaxs[0] &&
           Origin[0] + fMax[0] > entOrigin[0] + entMins[0] &&
           Origin[1] + fMin[1] < entOrigin[1] + entMaxs[1] &&
           Origin[1] + fMax[1] > entOrigin[1] + entMins[1] &&
           Origin[2] + fMin[2] < entOrigin[2] + entMaxs[2] &&
           Origin[2] + fMax[2] > entOrigin[2] + entMins[2])
            return 1
    }
    return 0
}

// ============================================================
//  辅助函数：计算实体底部到地面的距离（参考原版）
// ============================================================
stock Float:fm_distance_to_floor(index, ignoremonsters = 1)
{
    new Float:start[3], Float:dest[3], Float:end[3]
    pev(index, pev_origin, start)
    dest[0] = start[0]
    dest[1] = start[1]
    dest[2] = -8191.0
    engfunc(EngFunc_TraceLine, start, dest, ignoremonsters, index, 0)
    get_tr2(0, TR_vecEndPos, end)
    pev(index, pev_absmin, start)
    new Float:ret = start[2] - end[2]
    return ret > 0 ? ret : 0.0
}

// ============================================================
//  辅助函数：检测建筑下方是否站在其他实体上（参考原版）
// ============================================================
stock bool:fm_floor_entity(index)
{
    new Float:start[3], Float:dest[3]
    pev(index, pev_origin, start)
    dest[0] = start[0]
    dest[1] = start[1]
    dest[2] = -8191.0
    engfunc(EngFunc_TraceLine, start, dest, DONT_IGNORE_MONSTERS, index, 0)
    new iEntity = get_tr2(0, TR_pHit)
    if(IsBuildingEntity(iEntity))
        return true
    return false
}

// ============================================================
//  辅助函数：判断实体是否为建筑
// ============================================================
stock bool:IsBuildingEntity(iEntity)
{
    if(!pev_valid(iEntity)) return false
    new classname[32]
    pev(iEntity, pev_classname, classname, charsmax(classname))
    if(equal(classname, "dns_building")) return true
    return false
}

// ============================================================
//  摧毁建筑（基础清理）
// ============================================================
DestroyBuilding(ent)
{
    if(pev_valid(ent))
    {
        engfunc(EngFunc_EmitSound, ent, CHAN_STATIC, SOUND_BUILDING_CRASH, 1.0, ATTN_NORM, 0, PITCH_NORM);
        
        for(new i = 1; i <= MAX_PLAYERS; i++)
        {
            if(g_Blueprint[i] == ent)
            {
                g_Blueprint[i] = 0;
                g_IsBuilding[i] = false;
            }
        }
        engfunc(EngFunc_RemoveEntity, ent);
    }
}

// ============================================================
//  ★★★ 统一实现三个 Forward（特殊建筑逻辑调度） ★★★
// ============================================================

// 放置后回调
public DNS_BuildingPut_Post(iPlayer, iEntity, BuildingId)
{
    if (!IsSpecialBuilding(BuildingId))
        return
    
    if (BuildingId == g_GeneratorBuildingId)
    {
        if (g_GeneratorCount < MAX_GENERATORS)
        {
            g_GeneratorEntity[g_GeneratorCount] = iEntity;
            g_GenNextTick[g_GeneratorCount] = get_gametime() + 1.0;
            g_GenOwner[g_GeneratorCount] = iPlayer;
            g_GenThreshold[g_GeneratorCount] = 30;
            g_GenLastDamage[g_GeneratorCount] = 0.0;
            g_GenTotalPower[g_GeneratorCount] = 0;
            g_GenCyclePower[g_GeneratorCount] = 0;
            g_GeneratorCount++;
            set_pev(iEntity, pev_iuser2, 1);
        }
    }
}

// 完成回调
public DNS_BuildingComplete_Post(iPlayer, iEntity, BuildingId)
{
    if (!IsSpecialBuilding(BuildingId))
        return
    
    if (BuildingId == g_WareHouseId)
    {
        g_TeamMaxWoods += WAREHOUSE_CAPACITY
        g_TeamMaxSteel += WAREHOUSE_CAPACITY
        CreateWarehouseIcon(iEntity)
    }
}

// 摧毁回调
public DNS_BuildingKilled_Post(iEntity, iPlayer, BuildingId)
{
    if (!IsSpecialBuilding(BuildingId))
        return
    
    if (BuildingId == g_GeneratorBuildingId)
    {
        if (pev(iEntity, pev_iuser2) == 1)
        {
            for (new i = 0; i < g_GeneratorCount; i++)
            {
                if (g_GeneratorEntity[i] == iEntity)
                {
                    g_GeneratorEntity[i] = g_GeneratorEntity[g_GeneratorCount - 1];
                    g_GenNextTick[i] = g_GenNextTick[g_GeneratorCount - 1];
                    g_GenOwner[i] = g_GenOwner[g_GeneratorCount - 1];
                    g_GenThreshold[i] = g_GenThreshold[g_GeneratorCount - 1];
                    g_GenLastDamage[i] = g_GenLastDamage[g_GeneratorCount - 1];
                    g_GenTotalPower[i] = g_GenTotalPower[g_GeneratorCount - 1];
                    g_GenCyclePower[i] = g_GenCyclePower[g_GeneratorCount - 1];
                    g_GeneratorCount--;
                    break;
                }
            }
        }
    }
    
    if (BuildingId == g_WareHouseId)
    {
        RemoveWarehouseIcon(iEntity)
        g_TeamMaxWoods -= WAREHOUSE_CAPACITY
        g_TeamMaxSteel -= WAREHOUSE_CAPACITY
        if(g_TeamMaxWoods < 0) g_TeamMaxWoods = 0
        if(g_TeamMaxSteel < 0) g_TeamMaxSteel = 0
    }
}

// ============================================================
//  TraceLine 后处理：显示建筑信息（含进度条）
// ============================================================
public TraceLine_Post(Float:v1[3], Float:v2[3], noMonsters, pentToSkip, trace)
{
    if(!is_user_alive(pentToSkip))
        return
    
    new id = pentToSkip
    set_hudmessage(0, 0, 0, -1.0, 0.65, 0, 0.0, 0.0, 0.0, 0.0, 2)
    show_hudmessage(id, "")
    
    new ent = get_tr2(trace, TR_pHit)
    if(!pev_valid(ent))
        return
    
    new classname[32]
    pev(ent, pev_classname, classname, charsmax(classname))
    
    if(equal(classname, "dns_blueprint"))
        return
    
    if(!equal(classname, "dns_building"))
        return
    
    new Float:completion;
    pev(ent, pev_fuser1, completion);
    new Float:max_completion;
    pev(ent, pev_fuser2, max_completion);
    new Float:health;
    pev(ent, pev_health, health);
    new Float:max_health;
    pev(ent, pev_max_health, max_health);
    new BuildingId = pev(ent, pev_iuser1);
    
    new text[128]
    if(completion > 0.0)
    {
        new percent = floatround(((max_completion - completion) / max_completion) * 100.0)
        if(percent < 0) percent = 0
        if(percent > 100) percent = 100

        new filled = floatround(percent / 5.0)
        if(filled < 0) filled = 0
        if(filled > 20) filled = 20

        new bar[32]
        bar[0] = 0
        for(new i = 0; i < 20; i++)
        {
            add(bar, charsmax(bar), i < filled ? "#" : "-")
        }

        format(text, charsmax(text), "%s^n[%s] %d%%^n耐久: %d",
               gBuildingName[BuildingId], bar, percent, floatround(health))
    }
    else
    {
        new percent = floatround((health / max_health) * 100.0)
        if(percent < 0) percent = 0
        format(text, charsmax(text), "%s | 耐久: %d%%", 
               gBuildingName[BuildingId], percent)
    }
    set_hudmessage(255, 255, 0, -1.0, 0.65, 0, 0.5, 0.5, 0.0, 0.0, 2)
    show_hudmessage(id, text)
}