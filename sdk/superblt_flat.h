#pragma once

#include <Windows.h>
#include <stdint.h>

// Declare lua_State early, as we want to load the Lua files
// later on for ease of reading
typedef struct lua_State lua_State;

/**
 * Initialise the plugin. This is called when the DLL is first loaded and will only be called
 * once.
 *
 * This may be called before PAYDAY's main method has even been called, so don't expect that
 * anything has already been set up.
 *
 * Note that you should use this instead of DLLMain (Windows) or __attribute(constructor) for
 * any custom initialisation logic. This is guaranteed to be called after SuperBLT is set up, and
 * stuff like the logging functions work.
 */
void Plugin_Init();

/**
 * Do state-specific setup. Plugins should no longer register global variables to store their
 * state - rather, they should use `Plugin_PushLua` and return the table from that.
 *
 * The idea here is that, if two different DLLs end up getting included, they shouldn't
 * overwrite each other.
 *
 * This also gives more flexibility to Lua, and allows the global variables to be refractored
 * without modifying the native libraries.
 *
 * Old documentation, kept for archival reasons:
 *
 * Adds this plugin's global variables to a given Lua state. Since PAYDAY 2 loads
 * the Lua state multiple times (such as when changing levels), it's absolutely critical
 * that you keep this in mind. Additionally, more than one of these states can be active
 * at once, so you shouldn't use global variables to store the current state, as mysterious
 * crashes will be a likely result.
 */
void Plugin_Setup_Lua(lua_State* L);

/**
 * Update this plugin. Called once per frame.
 *
 * The main use for this is callbacks - if you need to perform anything asynchronously, ensure
 * that whatever you do you don't call any Lua functions from another thread. Instead, queue up
 * the results and act on them here.
 */
void Plugin_Update();

/**
 * Pushes a Lua value onto the stack, and returns the number of values it wishes to
 * return (see Lua's `lua_pushcfunction` to see how this works).
 *
 * This is the result returned (after `true` to indicate success) from `blt.load_native`.
 * For example:
 *
 * local success, val_1, val_2, val_3 = blt.load_native(...)
 *
 * Then the values `val_1`, `val_2`, and `val_3` would correspond to the 1st, 2nd and
 * 3rd values returned from `Plugin_PushLua`.
 *
 * Please note that this function may be called multiple times, if `blt.load_native` is called
 * multiple times (with the same filename it doesn't load the same DLL again, and it is perfectly
 * valid).
 */
int Plugin_PushLua(lua_State* L);

//////////////////////////
// Logging system

// The different logging functions available
enum class LogType
{
	LOGGING_FUNC = 0,
	LOGGING_LOG,
	LOGGING_LUA,
	LOGGING_WARN,
	LOGGING_ERROR
};

// clang-format off
#define PD2HOOK_LOG_FUNC(msg) PD2HOOK_LOG_LEVEL(msg, LogType::LOGGING_FUNC, __FILE__, 0, FOREGROUND_BLUE, FOREGROUND_GREEN, FOREGROUND_INTENSITY)
#define PD2HOOK_LOG_LOG(msg) PD2HOOK_LOG_LEVEL(msg, LogType::LOGGING_LOG, __FILE__, __LINE__, FOREGROUND_BLUE, FOREGROUND_GREEN, FOREGROUND_INTENSITY)
#define PD2HOOK_LOG_LUA(msg) PD2HOOK_LOG_LEVEL(msg, LogType::LOGGING_LUA, NULL, -1, FOREGROUND_RED, FOREGROUND_BLUE, FOREGROUND_GREEN, FOREGROUND_INTENSITY)
#define PD2HOOK_LOG_WARN(msg) PD2HOOK_LOG_LEVEL(msg, LogType::LOGGING_WARN, __FILE__, __LINE__, FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_INTENSITY)
#define PD2HOOK_LOG_ERROR(msg) PD2HOOK_LOG_LEVEL(msg, LogType::LOGGING_ERROR, __FILE__, __LINE__, FOREGROUND_RED, FOREGROUND_INTENSITY)
#define PD2HOOK_LOG_EXCEPTION(e) PD2HOOK_LOG_WARN(e)
#define PD2HOOK_DEBUG_CHECKPOINT PD2HOOK_LOG_LOG("Checkpoint")
// clang-format on

#define PD2HOOK_LOG_LEVEL(msg, level, file, line, ...) pd2_log(msg, (int)level, file, line)

void pd2_log(const char* message, int level, const char* file, int line);
bool is_active_state(lua_State* L);

//////////////////////////
// Platform specific stuff

#if defined(WIN32)
#define SBLT_API_EXPORT extern "C" __declspec(dllexport)
#elif defined(__GNUC__)
#define SBLT_API_EXPORT extern "C" __attribute__((visibility("default")))
#endif

/////////////////////////
// Lua headers

#include "lauxlib.h"
#include "lua.h"
