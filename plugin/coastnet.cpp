/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "icesat2/BathyFields.h"
#include "OsApi.h"
#include "EventLib.h"
#include "LuaEngine.h"
#include "CoastnetClassifier.h"

using BathyFields::extent_t;
using BathyFields::photon_t;

/******************************************************************************
 * DEFINES
 ******************************************************************************/

#define LUA_COASTNET_LIBNAME    "coastnet"

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * coastnet_version
 *----------------------------------------------------------------------------*/
int coastnet_version (lua_State* L)
{
    lua_pushstring(L, BINID);
    lua_pushstring(L, BUILDINFO);
    return 2;
}

/*----------------------------------------------------------------------------
 * coastnet_open
 *----------------------------------------------------------------------------*/
int coastnet_open (lua_State *L)
{
    static const struct luaL_Reg coastnet_functions[] = {
        {"version",             coastnet_version},
        {"classifier",          CoastnetClassifier::luaCreate},
        {NULL,                  NULL}
    };

    /* Set Library */
    luaL_newlib(L, coastnet_functions);

    return 1;
}

/******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************/

extern "C" {
void initcoastnet (void)
{
    /* Initialize Modules */
    CoastnetClassifier::init();

    /* Extend Lua */
    LuaEngine::extend(LUA_COASTNET_LIBNAME, coastnet_open);

    /* Indicate Presence of Package */
    LuaEngine::indicate(LUA_COASTNET_LIBNAME, BINID);

    /* Display Status */
    print2term("%s plugin initialized (%s)\n", LUA_COASTNET_LIBNAME, BINID);
}

void deinitcoastnet (void)
{
}
}
