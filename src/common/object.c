/*
 * object.c - base Lua object class
 *
 * Copyright © 2023 Abigail Teague <ateague063@gmail.com>
 *
 * Based on:
 * luaobject.c - useful functions for handling Lua objects
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
#include "object.h"
#include <moonauxlib.h>
#include "common/lualib.h"
#include "signals.h"

// NOTE: may expand with custom newindex later
static int make_proptable(lua_State *L) {
    lua_newtable(L);
    return 1;
}

static int object_init(lua_State *L) {
    luaC_construct(L, 0, "SignalStore");
    lua_setfield(L, 1, "Signals");
    return 0;
}

static int object_index(lua_State *L) {
    if (lua_type(L, 2) == LUA_TSTRING) {
        const char *s = lua_tostring(L, 2);
        if (*s == ':') {  // signals start with ':'
            lua_getfield(L, 1, "Signals");
            lua_getfield(L, -1, s);
            return 1;
        }
    }

    luaL_getmetafield(L, 1, "__class");
    if (lua_getfield(L, -1, "Properties") == LUA_TTABLE) {
        lua_pushvalue(L, 2);                      // push key
        if (lua_gettable(L, -2) == LUA_TTABLE &&  // check properties for key
            lua_getfield(L, -1, "get") == LUA_TFUNCTION) {
            lua_pushvalue(L, 1);  // push self
            lua_call(L, 1, 1);    // call getter
            return 1;
        }
    }

    luaC_deferindex(L);
    return 1;
}

static int object_newindex(lua_State *L) {
    luaL_getmetafield(L, 1, "__class");
    if (lua_getfield(L, -1, "Properties") == LUA_TTABLE) {
        lua_pushvalue(L, 2);                      // push key
        if (lua_gettable(L, -2) == LUA_TTABLE &&  // check properties for key
            lua_getfield(L, -1, "set") == LUA_TFUNCTION) {
            lua_pushvalue(L, 1);  // push self
            lua_pushvalue(L, 3);  // push value
            lua_call(L, 2, 0);    // call setter
            return 0;
        }
    }
    luaC_defernewindex(L);
    return 0;
}

static int object_inherited(lua_State *L) {
    luaC_construct(L, 0, "SignalStore");
    lua_setfield(L, 2, "Signals");
    lua_getfield(L, 2, "Properties");  // Properties field of inherited class
    luaC_getparent(L, 2);
    lua_getfield(L, -1, "Properties");  // Properties field of parent class
    lua_remove(L, -2);
    if (lua_type(L, -1) == LUA_TTABLE && lua_type(L, -2) == LUA_TTABLE) {
        lua_getmetatable(L, -2);
        lua_insert(L, -2);
        // set inherited class Properties meta __index to its parent
        lua_setfield(L, -2, "__index");
    }
    luaC_injectindex(L, 2, object_index);
    luaC_injectnewindex(L, 2, object_newindex);
    return 0;
}

static luaL_Reg object_methods[] = {
    {"new", object_init},
    {NULL,  NULL       }
};

void luaC_register_object(lua_State *L) {
    luaC_newclass(L, "Object", NULL, object_methods);
    luaC_construct(L, 0, "SignalStore");
    lua_setfield(L, -2, "Signals");
    luaC_setinheritcb(L, -2, object_inherited);
    lua_pushcfunction(L, make_proptable);
    lua_setfield(L, -2, "Properties");
}

int lunaL_object_constructor(lua_State *L) {
    if (lua_istable(L, 2)) {
        lua_pushnil(L);
        while (lua_next(L, 2)) {
            lua_pushvalue(L, -2);  // push copy of key
            lua_insert(L, -2);     // insert before value
            lua_settable(L, 1);    // obj[key] = value
        }
    }

    return 0;
}

void luna_object_connect_signal(lua_State *L, int idx, const char *name) {
    if (lua_getfield(L, idx, "Signals") == LUA_TUSERDATA) {
        lua_insert(L, -2);
        luna_signal_store_connect(L, -2, name);
    }
    lua_pop(L, 1);
}

void luna_object_disconnect_signal(lua_State *L, int idx, const char *name) {
    if (lua_getfield(L, idx, "Signals") == LUA_TUSERDATA) {
        lua_insert(L, -2);
        luna_signal_store_disconnect(L, -2, name);
    }
    lua_pop(L, 1);
}

void luna_object_emit_signal(lua_State *L, int idx, const char *name, int nargs) {
    if (lua_getfield(L, idx, "Signals") == LUA_TUSERDATA) {
        lua_insert(L, -nargs);
        luna_signal_store_emit(L, -nargs - 1, name, nargs);
    }
    lua_pop(L, 1);
}

void luna_class_connect_signal(lua_State *L, const char *class, const char *name) {
    if (luaC_pushclass(L, class)) {
        lua_insert(L, -2);
        luna_object_connect_signal(L, -2, name);
    }
    lua_pop(L, 1);
}

void luna_class_disconnect_signal(lua_State *L, const char *class, const char *name) {
    if (luaC_pushclass(L, class)) {
        lua_insert(L, -2);
        luna_object_disconnect_signal(L, -2, name);
    }
    lua_pop(L, 1);
}

void luna_class_emit_signal(lua_State *L, const char *class, const char *name, int nargs) {
    if (luaC_pushclass(L, class)) {
        lua_insert(L, -nargs - 1);
        luna_object_emit_signal(L, -nargs - 1, name, nargs);
    }
    lua_pop(L, 1);
}

void luna_class_add_property(
    lua_State    *L,
    int           idx,
    const char   *name,
    lua_CFunction get,
    lua_CFunction set) {
    if (lua_getfield(L, idx, "Properties") != LUA_TTABLE)
        luaL_error(L, "Invalid or missing property table");
    lua_newtable(L);
    lua_pushcfunction(L, get);
    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, set);
    lua_setfield(L, -2, "set");
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
}

void luna_class_setprops(lua_State *L, int idx, const luna_Prop props[], int nprops) {
    if (!props->name || !luaC_isclass(L, idx)) return;
    lua_pushstring(L, "Properties");
    lua_createtable(L, 0, nprops);  // properties table
    do {
        lua_pushstring(L, props->name);
        if (props->set) {
            lua_createtable(L, 0, 2);  // prop table (getter and setter)
            lua_pushstring(L, "set");
            lua_pushcfunction(L, props->set);
            lua_rawset(L, -3);            // prop["set"] = set
        } else lua_createtable(L, 0, 1);  // prop table (just getter)
        lua_pushstring(L, "get");
        lua_pushcfunction(L, props->get);
        lua_rawset(L, -3);  // prop["get"] = get
        lua_rawset(L, -3);  // properties[name] = prop
    } while ((++props)->name && props->get);
    lua_rawset(L, idx);  // class["Properties"] = properties
}
