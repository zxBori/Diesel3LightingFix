//
// Created by Campbell on 5/09/2026.
//

#include "superblt_flat.h"

typedef void* (*lua_access_func)(const char*);

void SBLT_LUA_INIT_FUNC_TABLE(void* (*loadFn)(const char*));

void (*pd2_log_fn)(const char* message, int level, const char* file, int line);
bool (*is_active_state_fn)(lua_State* L);

SBLT_API_EXPORT void SuperBLT_Plugin_Setup(lua_access_func get_exposed_function)
{
	// Load all our Lua functions
	SBLT_LUA_INIT_FUNC_TABLE(get_exposed_function);

	pd2_log_fn = (decltype(pd2_log_fn))get_exposed_function("pd2_log");
	is_active_state_fn = (decltype(is_active_state_fn))get_exposed_function("is_active_state");

	Plugin_Init();
}

// Forward on the appropriate calls
SBLT_API_EXPORT void SuperBLT_Plugin_Init_State(lua_State* L)
{
	Plugin_Setup_Lua(L);
}
SBLT_API_EXPORT void SuperBLT_Plugin_Update()
{
	Plugin_Update();
}
SBLT_API_EXPORT void SuperBLT_Plugin_PushLua(lua_State* L)
{
	Plugin_PushLua(L);
}

// The revision of the SuperBLT API used. This isn't used for now, but will be in the
// future to ensure backwards-compatibility.
SBLT_API_EXPORT const uint64_t SBLT_API_REVISION = 1;

void pd2_log(const char* message, int level, const char* file, int line)
{
	return pd2_log_fn(message, level, file, line);
}
bool is_active_state(lua_State* L)
{
	return is_active_state_fn(L);
}
