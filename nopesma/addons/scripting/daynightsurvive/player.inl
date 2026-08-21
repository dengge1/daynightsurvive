// ============================================================
//  player.inl - 玩家事件、Spawn/Killed、HUD
// ============================================================

public client_putinserver(id)
{
    g_MaxWoods[id] = 20
    g_MaxSteel[id] = 18
    g_MaxFood[id] = 100
    g_Woods[id] = 0
    g_Steel[id] = 0
    g_Food[id] = 30
    g_Money[id] = get_pcvar_num(cvar_startmoney)
    
    cs_set_user_vip(id, false)
}

public PlayerSpawn(id)
{
    if(!is_user_alive(id)) return
    cs_set_user_vip(id, false)
    set_user_health(id, 100)
}

public PlayerKilled(id)
{
    g_Food[id] /= 2
}

public MsgMoney(msgid, dest, id)
{
    set_pdata_int(id, 115, 0, 5)
    set_msg_arg_int(1, ARG_LONG, g_Money[id])
}

public PlayerPostThink(id)
{
    if(!is_user_alive(id) || !g_GameStarted) return
    
    if(g_IsBuilding[id] && pev_valid(g_Blueprint[id]))
    {
        new Float:origin[3], Float:vOrigin[3]
        pev(id, pev_origin, vOrigin)
        get_aim_origin_vector(id, 150.0, 0.0, 0.0, origin)
        origin[2] = vOrigin[2] - (pev(id, pev_flags) & FL_DUCKING ? 15.0 : 26.0)
        engfunc(EngFunc_SetOrigin, g_Blueprint[id], origin)
        engfunc(EngFunc_DropToFloor, g_Blueprint[id])
    }

    if(get_gametime() < g_NextHud[id]) return
    g_NextHud[id] = get_gametime() + 1.0
    
    new Float:health = float(get_user_health(id))
    new time_remaining = floatround(g_NextSwitch - get_gametime())
    if(time_remaining < 0) time_remaining = 0
    
    set_hudmessage(255, 255, 0, -1.0, 0.05, 0, 0.0, 1.1, 0.0, 0.0, 1)
    if(g_IsNight)
    {
        show_hudmessage(id,
            "[公共资源] 木材:%d/%d | 钢铁:%d/%d | 电力:%d/%d^n第 %d 天 - 夜晚剩余时间:%02d:%02d",
            g_TeamWoods, g_TeamMaxWoods,
            g_TeamSteel, g_TeamMaxSteel,
            g_TeamPower, g_TeamMaxPower,
            g_Days,
            time_remaining / 60, time_remaining % 60)
    }
    else
    {
        show_hudmessage(id,
            "[公共资源] 木材:%d/%d | 钢铁:%d/%d | 电力:%d/%d^n第 %d 天 - 白天剩余时间:%02d:%02d",
            g_TeamWoods, g_TeamMaxWoods,
            g_TeamSteel, g_TeamMaxSteel,
            g_TeamPower, g_TeamMaxPower,
            g_Days,
            time_remaining / 60, time_remaining % 60)
    }
    
    set_hudmessage(255, 255, 0, -1.0, 0.75, 0, 0.0, 1.1, 0.0, 0.0, 0)
    show_hudmessage(id,
        "生命: %.0f | 木材: %d/%d | 钢铁: %d/%d | 食物: %d",
        health,
        g_Woods[id], g_MaxWoods[id],
        g_Steel[id], g_MaxSteel[id],
        g_Food[id])
}