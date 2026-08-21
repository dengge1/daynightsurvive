// ============================================================
//  generator.inl - 发电机特殊逻辑
//  功能：注册发电机、过载损坏判定、产电
//  依赖：building_core.inl 的 RegisterBuilding() 和 RegisterSpecialBuilding()
// ============================================================

// ★ 变量 g_GeneratorBuildingId 已在主文件中声明，此处不再重复声明
new const GEN_POWER_RATE = 1
new const Float:GEN_CHECK_INTERVAL = 5.0

// ============================================================
//  预缓存
// ============================================================
PrecacheGenerator()
{
    engfunc(EngFunc_PrecacheModel, "models/DayNightSurvive/zsh_generator_1.mdl")
}

// ============================================================
//  注册建筑（由 core.inl 的 StartGame 调用）
// ============================================================
RegisterGeneratorBuilding()
{
    new Float:mins[3] = {-22.0, -15.0, 0.0}
    new Float:maxs[3] = {22.0, 15.0, 37.0}
    g_GeneratorBuildingId = RegisterBuilding("小型发电机", 1000.0, mins, maxs, "models/DayNightSurvive/zsh_generator_1.mdl", 5, 5,0)
    RegisterSpecialBuilding(g_GeneratorBuildingId)
}

// ============================================================
//  过载判定（由 core.inl 的 StartFrame 每帧调用）
// ============================================================
ProcessGeneratorOverload()
{
    static Float:cur
    cur = get_gametime()
    
    for(new i = 0; i < g_GeneratorCount; i++)
    {
        if(!pev_valid(g_GeneratorEntity[i])) continue
        
        // ★★★ 只有完成状态的发电机才能产电和累积 ★★★
        if(IsBuildingCompleted(g_GeneratorEntity[i]))
        {
            if(cur >= g_GenNextTick[i])
            {
                g_TeamPower += GEN_POWER_RATE
                if(g_TeamPower > g_TeamMaxPower) g_TeamPower = g_TeamMaxPower
                g_GenNextTick[i] = cur + 1.0
                
                g_GenTotalPower[i] += GEN_POWER_RATE
                g_GenCyclePower[i] += GEN_POWER_RATE
            }
        }
        else
        {
            // 未完成状态时不产电，但依然需要推进判定周期，防止累积停滞
            // 这里不重置 g_GenNextTick，让它在完成时继续从当前时间开始计时
            // 但为了不因为未完成而丢失周期，我们重置计时器为当前时间，避免堆积
            // 原版中，未完成的发电机不会产电，但周期判定依然进行（因为有 think）
            // 我们的判定是每5秒检查一次，所以即使不产电，也要保持 g_GenLastDamage 更新
            // 但我们需要保证损坏判定正常进行，所以 g_GenLastDamage 更新在下面，不依赖完成状态
        }
        
        // 损坏判定（每5秒检查一次），无论完成与否都要检查累计值
        if(cur - g_GenLastDamage[i] >= GEN_CHECK_INTERVAL)
        {
            g_GenLastDamage[i] = cur
            
            // 只有完成状态才进行过载判定（原版逻辑：只有发电才会累积，未完成不累积，所以条件自然不满足）
            // 但为了安全，我们也检查完成状态，防止未完成状态下误触
            if(IsBuildingCompleted(g_GeneratorEntity[i]) && g_GenTotalPower[i] >= g_GenThreshold[i] && g_GenCyclePower[i] >= g_GenThreshold[i])
            {
                g_GenTotalPower[i] = 0
                g_GenCyclePower[i] = 0
                
                // ★ 退回施工状态（与源码一致）
                set_pev(g_GeneratorEntity[i], pev_fuser1, BUILD_COMPLETION * 0.7)
                set_pev(g_GeneratorEntity[i], pev_fuser2, BUILD_COMPLETION)
                // 注意：不修改耐久，不改变渲染模式（原版逻辑）
                
                client_print(0, print_center, "一发电机发电过多已损坏")
            }
            else if(IsBuildingCompleted(g_GeneratorEntity[i]) && g_GenCyclePower[i] >= g_GenThreshold[i])
            {
                // 周期满但未达到总累计，重置周期（原版逻辑）
                g_GenCyclePower[i] = 0
            }
        }
    }
}