// ============================================================
//  cvars.inl - CVAR 定义与注册
// ============================================================

RegisterCvars()
{
    cvar_day_len = register_cvar("dns_day_length", "220.0")
    cvar_night_len = register_cvar("dns_night_length", "160.0")
    cvar_startmoney = register_cvar("dns_startmoney", "0")
    cvar_win_days = register_cvar("dns_win_days", "30")
    register_cvar("dns_default_nextmap", "de_dust2")
}