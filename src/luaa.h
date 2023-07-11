/*
 * luaa.h - Lua configuration management header
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef AWESOME_LUA_H
#define AWESOME_LUA_H

#include <lauxlib.h>
#include <lua.h>

#include <basedir.h>

#include "common/lualib.h"
#include "draw.h"

#if !(501 <= LUA_VERSION_NUM && LUA_VERSION_NUM < 505)
#error \
    "Awesome only supports Lua versions 5.1-5.4 and LuaJIT2, please refer to https://awesomewm.org/apidoc/documentation/10-building-and-testing.md.html#Building"
#endif

static inline void free_string(char **c) {
    p_delete(c);
}

DO_ARRAY(char *, string, free_string)

static inline void luaA_getuservalue(lua_State *L, int idx) {
#if LUA_VERSION_NUM >= 502
    lua_getuservalue(L, idx);
#else
    lua_getfenv(L, idx);
#endif
}

static inline void luaA_setuservalue(lua_State *L, int idx) {
#if LUA_VERSION_NUM >= 502
    lua_setuservalue(L, idx);
#else
    lua_setfenv(L, idx);
#endif
}

static inline size_t luaA_rawlen(lua_State *L, int idx) {
#if LUA_VERSION_NUM >= 502
    return lua_rawlen(L, idx);
#else
    return lua_objlen(L, idx);
#endif
}

static inline void luaA_registerlib(lua_State *L, const char *libname, const luaL_Reg *l) {
    assert(libname);
#if LUA_VERSION_NUM >= 502
    lua_newtable(L);
    luaL_setfuncs(L, l, 0);
    lua_pushvalue(L, -1);
    lua_setglobal(L, libname);
#else
    luaL_register(L, libname, l);
#endif
}

static inline void luaA_setfuncs(lua_State *L, const luaL_Reg *l) {
#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(L, l, 0);
#else
    luaL_register(L, NULL, l);
#endif
}

/** Push a area type to a table on stack.
 * \param L The Lua VM state.
 * \param geometry The area geometry to push.
 * \return The number of elements pushed on stack.
 */
static inline int luaA_pusharea(lua_State *L, area_t geometry) {
    lua_createtable(L, 0, 4);
    lua_pushinteger(L, geometry.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, geometry.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, geometry.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, geometry.height);
    lua_setfield(L, -2, "height");
    return 1;
}

typedef bool luaA_config_callback(const char *);

void        luaA_init(xdgHandle *, string_array_t *);
const char *luaA_find_config(xdgHandle *, const char *, luaA_config_callback *);
bool        luaA_parserc(xdgHandle *, const char *);

/** Global signals */
// TODO: put global signals in the registry or something
// extern signal_array_t global_signals;

void luaA_emit_startup(void);

void luaA_systray_invalidate(void);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
