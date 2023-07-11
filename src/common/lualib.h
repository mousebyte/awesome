/*
 * lualib.h - useful functions and type for Lua
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_COMMON_LUALIB
#define AWESOME_COMMON_LUALIB

#include <lauxlib.h>
#include <lua.h>

#include "common/util.h"

/** Lua function to call on dofunction() error */
extern lua_CFunction lualib_dofunction_on_error;

#define luaA_deprecate(L, repl)                                                                    \
    do {                                                                                           \
        luaA_warn(                                                                                 \
            L, "%s: This function is deprecated and will be removed, see %s", __FUNCTION__, repl); \
        lua_pushlstring(L, __FUNCTION__, sizeof(__FUNCTION__));                                    \
        luna_emit_global_signal(L, ":debug.deprecation", 1);                                       \
    } while (0)

/** Print a warning about some Lua code.
 * This is less mean than luaL_error() which setjmp via lua_error() and kills
 * everything. This only warn, it's up to you to then do what's should be done.
 * \param L The Lua VM state.
 * \param fmt The warning message.
 */
static inline void __attribute__((format(printf, 2, 3)))
luaA_warn(lua_State *L, const char *fmt, ...) {
    va_list ap;
    luaL_where(L, 1);
    fprintf(stderr, "%s%sW: ", a_current_time_str(), lua_tostring(L, -1));
    lua_pop(L, 1);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");

#if LUA_VERSION_NUM >= 502
    luaL_traceback(L, L, NULL, 2);
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
#endif
}

static inline int luaA_typerror(lua_State *L, int narg, const char *tname) {
    const char *msg = lua_pushfstring(L, "%s expected, got %s", tname, luaL_typename(L, narg));
#if LUA_VERSION_NUM >= 502
    luaL_traceback(L, L, NULL, 2);
    lua_concat(L, 2);
#endif
    return luaL_argerror(L, narg, msg);
}

static inline int luaA_rangerror(lua_State *L, int narg, double min, double max) {
    const char *msg = lua_pushfstring(
        L, "value in [%f, %f] expected, got %f", min, max, (double)lua_tonumber(L, narg));
#if LUA_VERSION_NUM >= 502
    luaL_traceback(L, L, NULL, 2);
    lua_concat(L, 2);
#endif
    return luaL_argerror(L, narg, msg);
}

static inline bool luaA_checkboolean(lua_State *L, int n) {
    if (!lua_isboolean(L, n)) luaA_typerror(L, n, "boolean");
    return lua_toboolean(L, n);
}

static inline lua_Number
luaA_getopt_number(lua_State *L, int idx, const char *name, lua_Number def) {
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1) || lua_isnumber(L, -1)) def = luaL_optnumber(L, -1, def);
    lua_pop(L, 1);
    return def;
}

static inline lua_Number
luaA_checknumber_range(lua_State *L, int n, lua_Number min, lua_Number max) {
    lua_Number result = lua_tonumber(L, n);
    if (result < min || result > max) luaA_rangerror(L, n, min, max);
    return result;
}

static inline lua_Number
luaA_optnumber_range(lua_State *L, int narg, lua_Number def, lua_Number min, lua_Number max) {
    if (lua_isnoneornil(L, narg)) return def;
    return luaA_checknumber_range(L, narg, min, max);
}

static inline lua_Number luaA_getopt_number_range(
    lua_State  *L,
    int         idx,
    const char *name,
    lua_Number  def,
    lua_Number  min,
    lua_Number  max) {
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1) || lua_isnumber(L, -1)) def = luaA_optnumber_range(L, -1, def, min, max);
    lua_pop(L, 1);
    return def;
}

static inline int luaA_checkinteger(lua_State *L, int n) {
    lua_Number d = lua_tonumber(L, n);
    if (d != (int)d) luaA_typerror(L, n, "integer");
    return d;
}

static inline lua_Integer luaA_optinteger(lua_State *L, int narg, lua_Integer def) {
    return luaL_opt(L, luaA_checkinteger, narg, def);
}

static inline int luaA_getopt_integer(lua_State *L, int idx, const char *name, lua_Integer def) {
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1) || lua_isnumber(L, -1)) def = luaA_optinteger(L, -1, def);
    lua_pop(L, 1);
    return def;
}

static inline int luaA_checkinteger_range(lua_State *L, int n, lua_Number min, lua_Number max) {
    int result = luaA_checkinteger(L, n);
    if (result < min || result > max) luaA_rangerror(L, n, min, max);
    return result;
}

static inline lua_Integer
luaA_optinteger_range(lua_State *L, int narg, lua_Integer def, lua_Number min, lua_Number max) {
    if (lua_isnoneornil(L, narg)) return def;
    return luaA_checkinteger_range(L, narg, min, max);
}

static inline int luaA_getopt_integer_range(
    lua_State  *L,
    int         idx,
    const char *name,
    lua_Integer def,
    lua_Number  min,
    lua_Number  max) {
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1) || lua_isnumber(L, -1)) def = luaA_optinteger_range(L, -1, def, min, max);
    lua_pop(L, 1);
    return def;
}

void luaA_checkfunction(lua_State *, int);
void luaA_checktable(lua_State *, int);

/** Dump the Lua stack. Useful for debugging.
 * \param L The Lua VM state.
 */
void luaA_dumpstack(lua_State *);

/** Register an Lua object.
 * \param L The Lua stack.
 * \param idx Index of the object in the stack.
 * \param ref A int address: it will be filled with the int
 * registered. If the address points to an already registered object, it will
 * be unregistered.
 * \return Always 0.
 */
static inline int luaA_register(lua_State *L, int idx, int *ref) {
    lua_pushvalue(L, idx);
    if (*ref != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, *ref);
    *ref = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Unregister a Lua object.
 * \param L The Lua stack.
 * \param ref A reference to an Lua object.
 */
static inline void luaA_unregister(lua_State *L, int *ref) {
    luaL_unref(L, LUA_REGISTRYINDEX, *ref);
    *ref = LUA_REFNIL;
}

/** Register a function.
 * \param L The Lua stack.
 * \param idx Index of the function in the stack.
 * \param fct A int address: it will be filled with the int
 * registered. If the address points to an already registered function, it will
 * be unregistered.
 * \return luaA_register value.
 */
static inline int luaA_registerfct(lua_State *L, int idx, int *fct) {
    luaA_checkfunction(L, idx);
    return luaA_register(L, idx, fct);
}
/** Convert s stack index to positive.
 * \param L The Lua VM state.
 * \param ud The index.
 * \return A positive index.
 */
static inline int luaA_absindex(lua_State *L, int ud) {
    return (ud > 0 || ud <= LUA_REGISTRYINDEX) ? ud : lua_gettop(L) + ud + 1;
}

static inline int luaA_dofunction_error(lua_State *L) {
    if (lualib_dofunction_on_error) return lualib_dofunction_on_error(L);
    return 0;
}

/** Execute an Lua function on top of the stack.
 * \param L The Lua stack.
 * \param nargs The number of arguments for the Lua function.
 * \param nret The number of returned value from the Lua function.
 * \return True on no error, false otherwise.
 */
static inline bool luaA_dofunction(lua_State *L, int nargs, int nret) {
    /* Move function before arguments */
    lua_insert(L, -nargs - 1);
    /* Push error handling function */
    lua_pushcfunction(L, luaA_dofunction_error);
    /* Move error handling function before args and function */
    lua_insert(L, -nargs - 2);
    int error_func_pos = lua_gettop(L) - nargs - 1;
    if (lua_pcall(L, nargs, nret, -nargs - 2)) {
        warn("%s", lua_tostring(L, -1));
        /* Remove error function and error string */
        lua_pop(L, 2);
        return false;
    }
    /* Remove error function */
    lua_remove(L, error_func_pos);
    return true;
}

/** Call a registered function. Its arguments are the complete stack contents.
 * \param L The Lua VM state.
 * \param handler The function to call.
 * \return The number of elements pushed on stack.
 */
static inline int luaA_call_handler(lua_State *L, int handler) {
    /* This is based on luaA_dofunction, but allows multiple return values */
    assert(handler != LUA_REFNIL);

    int nargs = lua_gettop(L);

    /* Push error handling function and move it before args */
    lua_pushcfunction(L, luaA_dofunction_error);
    lua_insert(L, -nargs - 1);
    int error_func_pos = 1;

    /* push function and move it before args */
    lua_rawgeti(L, LUA_REGISTRYINDEX, handler);
    lua_insert(L, -nargs - 1);

    if (lua_pcall(L, nargs, LUA_MULTRET, error_func_pos)) {
        warn("%s", lua_tostring(L, -1));
        /* Remove error function and error string */
        lua_pop(L, 2);
        return 0;
    }
    /* Remove error function */
    lua_remove(L, error_func_pos);
    return lua_gettop(L);
}

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
