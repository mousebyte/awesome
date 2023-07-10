/*
 * object.h - base Lua object class
 *
 * Copyright © 2023 Abigail Teague <ateague063@gmail.com>
 *
 * Based on:
 * luaobject.h - useful functions for handling Lua objects
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

#ifndef LUNA_COMMON_OBJECT_H
#define LUNA_COMMON_OBJECT_H

#define LUNA_OBJECT_REGISTRY_KEY "lunaria.object.registry"

#include <luaclasslib.h>
#include "refcount.h"

void luaC_register_object(lua_State *);

static inline void *luna_object_ref(lua_State *L, int idx) {
    lua_pushliteral(L, LUNA_OBJECT_REGISTRY_KEY);
    lua_rawget(L, LUA_REGISTRYINDEX);
    void *p = (void *)_luna_object_incref(L, idx > 0 ? idx : idx - 1);
    lua_pop(L, 1);
    return p;
}

static inline void luna_object_unref(lua_State *L, const void *ptr) {
    lua_pushliteral(L, LUNA_OBJECT_REGISTRY_KEY);
    lua_rawget(L, LUA_REGISTRYINDEX);
    _luna_object_decref(L, ptr);
    lua_pop(L, 1);
}

static inline void luna_object_push(lua_State *L, const void *ptr) {
    lua_pushliteral(L, LUNA_OBJECT_REGISTRY_KEY);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_rawgetp(L, -1, ptr);
    lua_remove(L, -2);
}

static inline void *luna_object_ref_item(lua_State *L, int idx) {
    lua_getiuservalue(L, idx, 1);
    void *p = (void *)_luna_object_incref(L, -2);
    lua_pop(L, 1);
    return p;
}

static inline void luna_object_unref_item(lua_State *L, int idx, const void *ptr) {
    lua_getiuservalue(L, idx, 1);
    _luna_object_decref(L, ptr);
    lua_pop(L, 1);
}

static inline void luna_object_push_item(lua_State *L, int idx, const void *ptr) {
    luaC_uvrawgetp(L, idx, 1, ptr);
}

int lunaL_object_constructor(lua_State *L);

void luna_object_connect_signal(lua_State *L, int idx, const char *name);

void luna_object_disconnect_signal(lua_State *L, int idx, const char *name);

void luna_object_emit_signal(lua_State *L, int idx, const char *name, int nargs);

void luna_class_connect_signal(lua_State *L, const char *class, const char *name);

void luna_class_disconnect_signal(lua_State *, const char *class, const char *);

void luna_class_emit_signal(lua_State *L, const char *class, const char *name, int nargs);

void luna_class_add_property(
    lua_State    *L,
    int           idx,
    const char   *name,
    lua_CFunction get,
    lua_CFunction set);

#define lunaL_getter(cls, name) static int lunaL_##cls##_get_##name(lua_State *L)
#define lunaL_setter(cls, name) static int lunaL_##cls##_set_##name(lua_State *L)
#define lunaL_prop(cls, name) \
    luna_class_add_property(L, -1, #name, lunaL_##cls##_get_##name, lunaL_##cls##_set_##name)
#define lunaL_readonly_prop(cls, name) \
    luna_class_add_property(L, -1, #name, lunaL_##cls##_get_##name, NULL);

#endif
