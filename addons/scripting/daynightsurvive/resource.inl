// ============================================================
//  resource.inl - 资源生成与采集
// ============================================================

PrecacheResources()
{
    g_ModelWood = precache_model("models/DayNightSurvive/item_wood.mdl")
    g_ModelSteel = precache_model("models/DayNightSurvive/item_hbeam.mdl")
    g_ModelFood = precache_model("models/DayNightSurvive/item_meat.mdl")
    g_ModelGibsWood = precache_model("models/woodgibs.mdl")
    g_ModelGibsSteel = precache_model("models/metalplategibs.mdl")
    g_ModelGibsFood = precache_model("models/fleshgibs.mdl")
    
    precache_sound(SOUND_GET)
}

SpawnResources()
{
    RemoveAllResources()
    new amount
    if(g_Days == 1 && !g_IsNight)
        amount = random_num(60, 100)
    else
    {
        amount = random_num(20, 50) - g_Days / 2
        if(amount < 5) amount = 5
    }
    new Float:origin[3]
    for(new i = 0; i < amount; i++)
    {
        if(!SsGetOrigin(origin))
        {
            new players[32], pnum
            get_players(players, pnum, "a")
            if(pnum > 0)
            {
                new id = players[random(pnum)]
                pev(id, pev_origin, origin)
                origin[0] += random_float(-500.0, 500.0)
                origin[1] += random_float(-500.0, 500.0)
            }
            else
            {
                origin[0] = 0.0
                origin[1] = 0.0
                origin[2] = 0.0
            }
        }
        new Float:end[3]
        end[0] = origin[0]
        end[1] = origin[1]
        end[2] = -8192.0
        engfunc(EngFunc_TraceLine, origin, end, IGNORE_MONSTERS, 0, 0)
        get_tr2(0, TR_vecEndPos, origin)
        origin[2] += 5.0
        CreateResource(origin, random_num(0, 2))
    }
}

RemoveAllResources()
{
    new ent = -1
    while((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", "dns_resource")))
        engfunc(EngFunc_RemoveEntity, ent)
}

LieFlat(ent)
{
    engfunc(EngFunc_DropToFloor, ent)
    if(!(pev(ent, pev_flags) & FL_ONGROUND))
        return
    new Float:origin[3], Float:traceto[3]
    new Float:angles[3]
    new trace, Float:fraction
    new Float:planeNormal[3]
    pev(ent, pev_origin, origin)
    pev(ent, pev_angles, angles)
    traceto[0] = origin[0]
    traceto[1] = origin[1]
    traceto[2] = origin[2] - 10.0
    engfunc(EngFunc_TraceLine, origin, traceto, IGNORE_MONSTERS, ent, trace)
    get_tr2(trace, TR_flFraction, fraction)
    if(fraction == 1.0) return
    get_tr2(trace, TR_vecPlaneNormal, planeNormal)
    if(planeNormal[2] >= 0.99) return
    angles[0] = floatatan(planeNormal[0] / planeNormal[2], radian) * -57.2958
    angles[1] = floatatan(planeNormal[1] / planeNormal[2], radian) * 57.2958
    angles[2] = 0.0
    set_pev(ent, pev_angles, angles)
}

CreateResource(Float:origin[3], type)
{
    new ent_check = -1
    while((ent_check = engfunc(EngFunc_FindEntityInSphere, ent_check, origin, 30.0)) > 0)
    {
        new classname[32]
        pev(ent_check, pev_classname, classname, sizeof classname - 1)
        if(equal(classname, "dns_resource"))
            return
    }
    new Float:end[3]
    end[0] = origin[0]
    end[1] = origin[1]
    end[2] = -8192.0
    engfunc(EngFunc_TraceLine, origin, end, IGNORE_MONSTERS, 0, 0)
    get_tr2(0, TR_vecEndPos, origin)
    origin[2] += 5.0
    new ent = engfunc(EngFunc_CreateNamedEntity, engfunc(EngFunc_AllocString, "info_target"))
    if(!ent) return
    set_pev(ent, pev_classname, "dns_resource")
    set_pev(ent, pev_solid, SOLID_BBOX)
    set_pev(ent, pev_movetype, MOVETYPE_TOSS)
    set_pev(ent, pev_takedamage, DAMAGE_YES)
    set_pev(ent, pev_iuser1, type)
    set_pev(ent, pev_iuser2, 1)
    set_pev(ent, pev_iuser3, random_num(1, 10))
    set_pev(ent, pev_iuser4, 0)
    switch(type)
    {
        case 0: set_pev(ent, pev_modelindex, g_ModelWood)
        case 1: set_pev(ent, pev_modelindex, g_ModelSteel)
        case 2: set_pev(ent, pev_modelindex, g_ModelFood)
    }
    set_pev(ent, pev_sequence, 0)
    set_pev(ent, pev_animtime, get_gametime())
    set_pev(ent, pev_frame, 0.0)
    set_pev(ent, pev_framerate, 1.0)
    set_pev(ent, pev_nextthink, -1.0)
    new Float:mins[3] = {-20.0, -25.0, 0.0}
    new Float:maxs[3] = {20.0, 25.0, 25.0}
    engfunc(EngFunc_SetSize, ent, mins, maxs)
    engfunc(EngFunc_SetOrigin, ent, origin)
    LieFlat(ent)
}

public TraceAttack_Resource(ent, id, Float:damage, Float:dir[3], ptr, bits)
{
    if(!pev_valid(ent)) return HAM_IGNORED
    new classname[32]
    pev(ent, pev_classname, classname, sizeof classname - 1)
    if(!equal(classname, "dns_resource")) return HAM_IGNORED
    if(get_user_weapon(id) != CSW_KNIFE) return HAM_SUPERCEDE
    new type = pev(ent, pev_iuser1)
    new amount = pev(ent, pev_iuser2)
    new target = pev(ent, pev_iuser3)
    new count = pev(ent, pev_iuser4) + 1
    new max, cur
    switch(type)
    {
        case 0: { max = g_MaxWoods[id]; cur = g_Woods[id]; }
        case 1: { max = g_MaxSteel[id]; cur = g_Steel[id]; }
        case 2: { max = g_MaxFood[id]; cur = g_Food[id]; }
    }
    new Float:flEnd[3]
    get_tr2(ptr, TR_vecEndPos, flEnd)
    message_begin(MSG_BROADCAST, SVC_TEMPENTITY)
    write_byte(TE_SPARKS)
    engfunc(EngFunc_WriteCoord, flEnd[0])
    engfunc(EngFunc_WriteCoord, flEnd[1])
    engfunc(EngFunc_WriteCoord, flEnd[2])
    message_end()
    if(cur >= max)
    {
        set_hudmessage(0, 255, 0, -1.0, 0.70, 0, 1.0, 1.0, 0.0, 0.0, 3)
        show_hudmessage(id, "[背包已满!]")
        return HAM_SUPERCEDE
    }
    emit_sound(id, CHAN_STATIC, SOUND_GET, 1.0, ATTN_NORM, 0, PITCH_NORM)
    new actual = (amount < max - cur) ? amount : max - cur
    new res_name[8]
    switch(type)
    {
        case 0: {
            g_Woods[id] += actual
            // ★ 不加入公共资源
            copy(res_name, 7, "木材")
        }
        case 1: {
            g_Steel[id] += actual
            // ★ 不加入公共资源
            copy(res_name, 7, "钢铁")
        }
        case 2: {
            g_Food[id] += actual
            copy(res_name, 7, "食物")
        }
    }
    set_hudmessage(0, 255, 0, -1.0, 0.70, 0, 1.0, 1.0, 0.0, 0.0, 3)
    show_hudmessage(id, "[采集到 %d 个%s]", actual, res_name)
    set_pev(ent, pev_iuser4, count)
    if(count >= target)
        engfunc(EngFunc_RemoveEntity, ent)
    return HAM_SUPERCEDE
}

public Killed_Resource(ent, attacker, gib)
{
    if(!pev_valid(ent)) return
    new classname[32]
    pev(ent, pev_classname, classname, sizeof classname - 1)
    if(!equal(classname, "dns_resource")) return
    new type = pev(ent, pev_iuser1)
    new Float:origin[3]
    pev(ent, pev_origin, origin)
    origin[2] += 15.0
    new gibModel
    switch(type)
    {
        case 0: gibModel = g_ModelGibsWood
        case 1: gibModel = g_ModelGibsSteel
        case 2: gibModel = g_ModelGibsFood
    }
    engfunc(EngFunc_MessageBegin, MSG_PVS, SVC_TEMPENTITY, origin, 0)
    write_byte(TE_BREAKMODEL)
    engfunc(EngFunc_WriteCoord, origin[0])
    engfunc(EngFunc_WriteCoord, origin[1])
    engfunc(EngFunc_WriteCoord, origin[2])
    engfunc(EngFunc_WriteCoord, 20.0)
    engfunc(EngFunc_WriteCoord, 20.0)
    engfunc(EngFunc_WriteCoord, 20.0)
    engfunc(EngFunc_WriteCoord, random_float(-100.0, 100.0))
    engfunc(EngFunc_WriteCoord, random_float(-100.0, 100.0))
    engfunc(EngFunc_WriteCoord, random_float(50.0, 150.0))
    write_byte(10)
    write_short(gibModel)
    write_byte(5)
    write_byte(20)
    write_byte(0x02)
    message_end()
}