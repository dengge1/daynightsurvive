// ============================================================
//  warehouse.inl - 仓库完整逻辑（容量扩展 + 存/取交互 + E键图标）
//  功能：建造后木材/钢铁上限各 +30，E键菜单存入/取出
//  消耗：每次操作消耗 10 电力
//  材料：12 木材 + 6 钢铁（与原版一致）
//  图标：原版风格，4个E键图标在仓库四周
// ============================================================

new g_PlayerStoringType[33]

// ============================================================
//  预缓存（模型 + E键图标）
// ============================================================
PrecacheWarehouse()
{
    engfunc(EngFunc_PrecacheModel, "models/DayNightSurvive/bg_warehouse.mdl")
    g_ButtonESprite = engfunc(EngFunc_PrecacheModel, "sprites/DayNightSurvive/e_button01.spr")
}

// ============================================================
//  注册建筑
// ============================================================
RegisterWarehouseBuilding()
{
    new Float:mins[3] = {-64.0, -64.0, -0.0}
    new Float:maxs[3] = {64.0, 64.0, 120.0}
    g_WareHouseId = RegisterBuilding("仓库", 1250.0, mins, maxs, "models/DayNightSurvive/bg_warehouse.mdl", 12, 6,0)
    RegisterSpecialBuilding(g_WareHouseId)
}

// ============================================================
//  ★ 创建 E 键图标（原版风格：4个图标在四周）
// ============================================================
CreateWarehouseIcon(iEntity)
{
    if(!pev_valid(iEntity)) return
    
    new Float:origin[3]
    
    // 四个方向：前、后、左、右（距离80，高度60）
    get_aim_origin_vector(iEntity, 80.0, 0.0, 60.0, origin)
    CreateButtonE(iEntity, origin)
    
    get_aim_origin_vector(iEntity, -80.0, 0.0, 60.0, origin)
    CreateButtonE(iEntity, origin)
    
    get_aim_origin_vector(iEntity, 0.0, 80.0, 60.0, origin)
    CreateButtonE(iEntity, origin)
    
    get_aim_origin_vector(iEntity, 0.0, -80.0, 60.0, origin)
    CreateButtonE(iEntity, origin)
}

// 辅助函数：创建单个E键图标
CreateButtonE(iEntity, Float:origin[3])
{
    new iSprite = engfunc(EngFunc_CreateNamedEntity, engfunc(EngFunc_AllocString, "env_sprite"))
    set_pev(iSprite, pev_classname, "dns_button_e")
    set_pev(iSprite, pev_modelindex, g_ButtonESprite)
    set_pev(iSprite, pev_scale, 0.12)
    set_pev(iSprite, pev_frame, 0.0)
    set_pev(iSprite, pev_framerate, 1.0)
    set_pev(iSprite, pev_rendermode, kRenderTransAdd)
    set_pev(iSprite, pev_renderamt, 200.0)
    set_pev(iSprite, pev_movetype, MOVETYPE_NONE)
    set_pev(iSprite, pev_solid, SOLID_NOT)
    engfunc(EngFunc_SetOrigin, iSprite, origin)
    set_pev(iSprite, pev_euser1, iEntity)   // 关联到仓库，方便移除
}

// ============================================================
//  ★ 移除 E 键图标（遍历移除所有关联图标）
// ============================================================
RemoveWarehouseIcon(iEntity)
{
    if(!pev_valid(iEntity)) return
    
    new iSprite = -1
    while((iSprite = engfunc(EngFunc_FindEntityByString, iSprite, "classname", "dns_button_e")))
    {
        if(pev(iSprite, pev_euser1) == iEntity)
            engfunc(EngFunc_RemoveEntity, iSprite)
    }
}

// ============================================================
//  ★ CmdStart 钩子：E 键检测仓库交互
// ============================================================
public DNS_CmdStart_Pre(iPlayer, uc_handle, seed)
{
    if(!is_user_alive(iPlayer)) return
    
    new iButton = get_uc(uc_handle, UC_Buttons)
    new iOldButton = pev(iPlayer, pev_oldbuttons)
    
    if(iButton & IN_USE && !(iOldButton & IN_USE))
    {
        new iEntity = CheckWarehouseDistance(iPlayer)
        if(!IsBuildingCompleted(iEntity)) return
        
        if(g_TeamPower < 10)
        {
            client_print(iPlayer, print_center, "电力不足！需要 10 电力")
            return
        }
        ShowWarehouseMenu(iPlayer)
    }
}

// ============================================================
//  显示仓库主菜单
// ============================================================
ShowWarehouseMenu(iPlayer)
{
    new menu = menu_create("\r仓库操作 (每次消耗 10 电力)", "WarehouseMenuHandler")
    menu_additem(menu, "存入木材")
    menu_additem(menu, "取出木材")
    menu_additem(menu, "存入钢铁")
    menu_additem(menu, "取出钢铁")
    menu_setprop(menu, MPROP_EXITNAME, "退出")
    menu_display(iPlayer, menu)
}

// ============================================================
//  仓库菜单回调
// ============================================================
public WarehouseMenuHandler(iPlayer, menu, key)
{
    if(key == MENU_EXIT || !is_user_alive(iPlayer))
    {
        menu_destroy(menu)
        return PLUGIN_HANDLED
    }
    
    if(g_TeamPower < 10)
    {
        client_print(iPlayer, print_center, "电力不足！需要 10 电力")
        menu_destroy(menu)
        return PLUGIN_HANDLED
    }
    
    new iEntity = CheckWarehouseDistance(iPlayer)
    if(!IsBuildingCompleted(iEntity))
    {
        client_print(iPlayer, print_center, "你已离开仓库范围")
        menu_destroy(menu)
        return PLUGIN_HANDLED
    }
    
    switch(key)
    {
        case 0:  // 存入木材
        {
            g_PlayerStoringType[iPlayer] = WOODS + 1
            client_cmd(iPlayer, "messagemode InsertWarehouseAmount")
            client_print(iPlayer, print_center, "请输入要存入的木材数量 (当前背包: %d)", g_Woods[iPlayer])
        }
        case 1:  // 取出木材
        {
            g_PlayerStoringType[iPlayer] = WOODS + 4
            client_cmd(iPlayer, "messagemode InsertWarehouseAmount")
            client_print(iPlayer, print_center, "请输入要取出的木材数量 (公共仓库: %d)", g_TeamWoods)
        }
        case 2:  // 存入钢铁
        {
            g_PlayerStoringType[iPlayer] = STEEL + 1
            client_cmd(iPlayer, "messagemode InsertWarehouseAmount")
            client_print(iPlayer, print_center, "请输入要存入的钢铁数量 (当前背包: %d)", g_Steel[iPlayer])
        }
        case 3:  // 取出钢铁
        {
            g_PlayerStoringType[iPlayer] = STEEL + 4
            client_cmd(iPlayer, "messagemode InsertWarehouseAmount")
            client_print(iPlayer, print_center, "请输入要取出的钢铁数量 (公共仓库: %d)", g_TeamSteel)
        }
    }
    
    menu_destroy(menu)
    return PLUGIN_HANDLED
}

// ============================================================
//  处理玩家输入的数量
// ============================================================
public InsertWarehouseAmount(iPlayer)
{
    if(!is_user_alive(iPlayer)) return PLUGIN_HANDLED
    
    new argstext[32]
    read_args(argstext, charsmax(argstext))
    remove_quotes(argstext)
    
    if(!is_str_num(argstext) || equal(argstext, ""))
        return PLUGIN_HANDLED
    
    new Amount = str_to_num(argstext)
    if(Amount <= 0)
    {
        client_print(iPlayer, print_center, "数量必须大于 0")
        return PLUGIN_HANDLED
    }
    
    if(g_TeamPower < 10)
    {
        client_print(iPlayer, print_center, "电力不足！")
        return PLUGIN_HANDLED
    }
    
    new iEntity = CheckWarehouseDistance(iPlayer)
    if(!IsBuildingCompleted(iEntity))
    {
        client_print(iPlayer, print_center, "你已离开仓库范围")
        return PLUGIN_HANDLED
    }
    
    new Type = g_PlayerStoringType[iPlayer]
    g_PlayerStoringType[iPlayer] = 0
    
    if(Type == WOODS + 1 || Type == STEEL + 1)
    {
        // ★ 存入
        new resType = (Type == WOODS + 1) ? WOODS : STEEL
        new curRes = (resType == WOODS) ? g_Woods[iPlayer] : g_Steel[iPlayer]
        new teamRes = (resType == WOODS) ? g_TeamWoods : g_TeamSteel
        new teamMax = (resType == WOODS) ? g_TeamMaxWoods : g_TeamMaxSteel
        
        if(curRes <= 0)
        {
            client_print(iPlayer, print_center, "背包中没有 %s", (resType == WOODS) ? "木材" : "钢铁")
            return PLUGIN_HANDLED
        }
        
        if(teamRes >= teamMax)
        {
            client_print(iPlayer, print_center, "公共仓库已满！")
            return PLUGIN_HANDLED
        }
        
        new actual = Amount
        if(actual > curRes) actual = curRes
        if(teamRes + actual > teamMax) actual = teamMax - teamRes
        
        if(resType == WOODS)
        {
            g_Woods[iPlayer] -= actual
            g_TeamWoods += actual
        }
        else
        {
            g_Steel[iPlayer] -= actual
            g_TeamSteel += actual
        }
        
        g_TeamPower -= 10
        client_print(iPlayer, print_center, "成功存入 %d 个%s", actual, (resType == WOODS) ? "木材" : "钢铁")
    }
    else if(Type == WOODS + 4 || Type == STEEL + 4)
    {
        // ★ 取出
        new resType = (Type == WOODS + 4) ? WOODS : STEEL
        new maxRes = (resType == WOODS) ? g_MaxWoods[iPlayer] : g_MaxSteel[iPlayer]
        new curRes = (resType == WOODS) ? g_Woods[iPlayer] : g_Steel[iPlayer]
        new teamRes = (resType == WOODS) ? g_TeamWoods : g_TeamSteel
        
        if(teamRes <= 0)
        {
            client_print(iPlayer, print_center, "公共仓库没有 %s", (resType == WOODS) ? "木材" : "钢铁")
            return PLUGIN_HANDLED
        }
        
        if(curRes >= maxRes)
        {
            client_print(iPlayer, print_center, "背包已满！")
            return PLUGIN_HANDLED
        }
        
        new actual = Amount
        if(actual > teamRes) actual = teamRes
        if(curRes + actual > maxRes) actual = maxRes - curRes
        
        if(resType == WOODS)
        {
            g_Woods[iPlayer] += actual
            g_TeamWoods -= actual
        }
        else
        {
            g_Steel[iPlayer] += actual
            g_TeamSteel -= actual
        }
        
        g_TeamPower -= 10
        client_print(iPlayer, print_center, "成功取出 %d 个%s", actual, (resType == WOODS) ? "木材" : "钢铁")
    }
    
    return PLUGIN_HANDLED
}

// ============================================================
//  检测玩家是否面向一个已完成的仓库
// ============================================================
CheckWarehouseDistance(iPlayer, Float:fDistance = 150.0)
{
    new Float:start[3], Float:view_ofs[3], Float:end[3]
    pev(iPlayer, pev_origin, start)
    pev(iPlayer, pev_view_ofs, view_ofs)
    xs_vec_add(start, view_ofs, start)
    
    pev(iPlayer, pev_v_angle, end)
    engfunc(EngFunc_MakeVectors, end)
    global_get(glb_v_forward, end)
    xs_vec_mul_scalar(end, fDistance, end)
    xs_vec_add(start, end, end)
    
    engfunc(EngFunc_TraceLine, start, end, DONT_IGNORE_MONSTERS, iPlayer, 0)
    new iEntity = get_tr2(0, TR_pHit)
    
    if(!pev_valid(iEntity)) return 0
    new classname[32]
    pev(iEntity, pev_classname, classname, charsmax(classname))
    if(!equal(classname, "dns_building")) return 0
    if(pev(iEntity, pev_iuser1) != g_WareHouseId) return 0
    if(!IsBuildingCompleted(iEntity)) return 0
    
    return iEntity
}