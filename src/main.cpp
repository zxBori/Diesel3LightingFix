#include "d3d11_fix.h"
#include <superblt_flat.h>

void Plugin_Init()
{
	InstallD3D11LightingFix();
}

void Plugin_Update()
{
}

void Plugin_Setup_Lua(lua_State* L)
{
}

int Plugin_PushLua(lua_State* L)
{
	return 0;
}