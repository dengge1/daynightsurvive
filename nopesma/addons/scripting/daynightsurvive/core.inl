// ============================================================
//  core.inl - 昼夜循环 + 调用发电机判定
// ============================================================

InitGameState()
{
    g_GameStarted = false
    g_Days = 1
    g_IsNight = false
    g_NextSwitch = 0.0
    g_CurrentLight = MORNING
    g_GeneratorCount = 0
    
    g_TeamMaxWoods = 0
    g_TeamMaxSteel = 0
    g_TeamMaxPower = 300
    g_TeamWoods = 0
    g_TeamSteel = 0
    g_TeamPower = 0
}

public StartGame()
{
    if(g_GameStarted) return
    g_GameStarted = true
    
    // 注册已在 plugin_init 中完成，此处不再重复
    
    g_Days = 1
    g_IsNight = false
    g_NextSwitch = get_gametime() + get_pcvar_float(cvar_day_len)
    g_CurrentLight = MORNING
    SetLight(g_CurrentLight)
    
    client_print(0, print_center, "第 1 天开始！")
    client_cmd(0, "spk %s", SOUND_DAY)
    
    for(new i = 1; i <= MAX_PLAYERS; i++)
        if(is_user_connected(i))
            cs_set_user_vip(i, false)
    
    SsScan()
    SpawnResources()
}

public StartFrame()
{
    if(!g_GameStarted) return
    
    ProcessGeneratorOverload()
    
    static Float:cur
    cur = get_gametime()
    
    new Float:remain = g_NextSwitch - cur
    new target_light
    
    if(!g_IsNight)
    {
        if(remain > TRANSITION_START)
            target_light = MORNING
        else
            target_light = floatround((1.0 - remain / TRANSITION_START) * 15.0)
        if(target_light < MORNING) target_light = MORNING
        if(target_light > NIGHT) target_light = NIGHT
    }
    else
    {
        if(remain > TRANSITION_START)
            target_light = NIGHT
        else
            target_light = floatround(remain / TRANSITION_START * 15.0)
        if(target_light < MORNING) target_light = MORNING
        if(target_light > NIGHT) target_light = NIGHT
    }
    
    if(g_CurrentLight != target_light)
    {
        g_CurrentLight = target_light
        SetLight(g_CurrentLight)
    }
    
    if(cur < g_NextSwitch) return
    
    if(!g_IsNight)
    {
        g_IsNight = true
        g_NextSwitch = cur + get_pcvar_float(cvar_night_len)
        client_print(0, print_center, "夜晚降临！小心僵尸！")
        client_cmd(0, "spk %s", SOUND_NIGHT)
        RemoveAllResources()
    }
    else
    {
        g_IsNight = false
        g_Days++
        g_NextSwitch = cur + get_pcvar_float(cvar_day_len)
        client_print(0, print_center, "第 %d 天开始！", g_Days)
        client_cmd(0, "spk %s", SOUND_DAY)
        SpawnResources()
        if(g_Days >= get_pcvar_num(cvar_win_days))
        {
            client_print(0, print_center, "恭喜！存活 %d 天，胜利！", g_Days)
            set_task(5.0, "ChangeMap")
        }
    }
}

public ChangeMap()
{
    new nextmap[64]
    get_cvar_string("amx_nextmap", nextmap, charsmax(nextmap))
    if(!nextmap[0])
        get_cvar_string("dns_default_nextmap", nextmap, charsmax(nextmap))
    if(nextmap[0])
        server_cmd("changelevel %s", nextmap)
    else
        server_cmd("changelevel de_dust2")
}

public GeneratorKilled(ent, attacker, gib)
{
    // 已由 generator.inl 处理
}