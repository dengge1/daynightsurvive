// ============================================================
//  utils.inl - 辅助函数
// ============================================================

stock get_aim_origin_vector(iPlayer, Float:forw, Float:right, Float:up, Float:vStart[])
{
    new Float:vOrigin[3], Float:vAngle[3], Float:vForward[3], Float:vRight[3], Float:vUp[3]
    
    pev(iPlayer, pev_origin, vOrigin)
    pev(iPlayer, pev_view_ofs, vUp)
    xs_vec_add(vOrigin, vUp, vOrigin)
    pev(iPlayer, pev_v_angle, vAngle)
    
    angle_vector(vAngle, ANGLEVECTOR_FORWARD, vForward)
    angle_vector(vAngle, ANGLEVECTOR_RIGHT, vRight)
    angle_vector(vAngle, ANGLEVECTOR_UP, vUp)
    
    vStart[0] = vOrigin[0] + vForward[0] * forw + vRight[0] * right + vUp[0] * up
    vStart[1] = vOrigin[1] + vForward[1] * forw + vRight[1] * right + vUp[1] * up
    vStart[2] = vOrigin[2] + vForward[2] * forw + vRight[2] * right + vUp[2] * up
}

SetLight(level)
{
    static const styles[][] = { 
        "q","p","o","n","m","l","k","j","i","h","g","f","e","d","c","b" 
    }
    message_begin(MSG_BROADCAST, SVC_LIGHTSTYLE)
    write_byte(0)
    write_string(styles[level])
    message_end()
}

public ForceTimeLimit()
{
    if(get_cvar_num("mp_timelimit") != 0)
        set_cvar_num("mp_timelimit", 0)
}

// ========== 防卡系统 ==========
public CmdStuckMenu(id)
{
    if(!g_GameStarted || !is_user_alive(id))
        return PLUGIN_CONTINUE

    new menu = menu_create("\r[DNS] 防卡菜单", "StuckMenuHandler")
    menu_additem(menu, "解决卡住 (传送至安全位置)", "1")
    menu_setprop(menu, MPROP_EXITNAME, "退出")
    menu_display(id, menu)
    return PLUGIN_HANDLED
}

public StuckMenuHandler(id, menu, item)
{
    if(item == MENU_EXIT)
    {
        menu_destroy(menu)
        return PLUGIN_HANDLED
    }

    new data[6], access, callback
    menu_item_getinfo(menu, item, access, data, charsmax(data), _, _, callback)

    switch(str_to_num(data))
    {
        case 1:
        {
            if(!IsPlayerStuck(id))
            {
                client_print(id, print_center, "你当前没有卡住")
                menu_destroy(menu)
                return PLUGIN_HANDLED
            }
            DoSafeTeleport(id)
        }
    }

    menu_destroy(menu)
    return PLUGIN_HANDLED
}

bool:IsPlayerStuck(id)
{
    new Float:origin[3]
    pev(id, pev_origin, origin)
    engfunc(EngFunc_TraceHull, origin, origin, DONT_IGNORE_MONSTERS, HULL_HUMAN, id, 0)
    if(get_tr2(0, TR_StartSolid) || get_tr2(0, TR_AllSolid) || !get_tr2(0, TR_InOpen))
        return true
    return false
}

DoSafeTeleport(id)
{
    if(!g_GameStarted || !is_user_alive(id))
        return

    new Float:origin[3]
    new Float:test_origin[3]
    new bool:found = false
    new Float:player_origin[3]
    pev(id, pev_origin, player_origin)

    for(new attempt = 0; attempt < 30; attempt++)
    {
        if(!SsGetOrigin(origin))
            break

        new Float:end[3]
        end[0] = origin[0]
        end[1] = origin[1]
        end[2] = -8192.0
        engfunc(EngFunc_TraceLine, origin, end, IGNORE_MONSTERS, 0, 0)
        get_tr2(0, TR_vecEndPos, origin)
        origin[2] += 10.0

        xs_vec_copy(origin, test_origin)
        engfunc(EngFunc_TraceHull, test_origin, test_origin, DONT_IGNORE_MONSTERS, HULL_HUMAN, id, 0)

        if(!get_tr2(0, TR_StartSolid) && !get_tr2(0, TR_AllSolid) && get_tr2(0, TR_InOpen))
        {
            if(get_distance_f(origin, player_origin) > 200.0)
            {
                found = true
                break
            }
        }
    }

    if(!found)
    {
        for(new attempt = 0; attempt < 50; attempt++)
        {
            xs_vec_copy(player_origin, origin)
            origin[0] += random_float(-400.0, 400.0)
            origin[1] += random_float(-400.0, 400.0)
            origin[2] += 50.0

            new Float:end[3]
            end[0] = origin[0]
            end[1] = origin[1]
            end[2] = -8192.0
            engfunc(EngFunc_TraceLine, origin, end, IGNORE_MONSTERS, 0, 0)
            get_tr2(0, TR_vecEndPos, origin)
            origin[2] += 10.0

            engfunc(EngFunc_TraceHull, origin, origin, DONT_IGNORE_MONSTERS, HULL_HUMAN, id, 0)

            if(!get_tr2(0, TR_StartSolid) && !get_tr2(0, TR_AllSolid) && get_tr2(0, TR_InOpen))
            {
                if(get_distance_f(origin, player_origin) > 150.0)
                {
                    found = true
                    break
                }
            }
        }
    }

    if(found)
    {
        set_pev(id, pev_origin, origin)
        new Float:velocity[3]
        velocity[0] = 0.0
        velocity[1] = 0.0
        velocity[2] = 0.0
        set_pev(id, pev_velocity, velocity)
        client_print(id, print_center, "已传送到安全位置")
    }
    else
    {
        client_print(id, print_center, "无法找到安全位置，请尝试手动移动")
    }
}