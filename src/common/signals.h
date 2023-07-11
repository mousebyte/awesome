/*
 * signals.h - signal and related classes
 *
 * Copyright © 2023 Abigail Teague <ateague063@gmail.com>
 *
 * Based on:
 * common/signal.h - Signal handling functions
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

#ifndef LUNA_COMMON_SIGNAL_H
#define LUNA_COMMON_SIGNAL_H

#include <luaclasslib.h>
#include <stdlib.h>
#include "array.h"
#define LUNA_GLOBAL_SIGNALS "lunaria.signals.global"

void luna_signal_store_connect(lua_State *, int, const char *);
void luna_signal_store_disconnect(lua_State *, int, const char *);
void luna_signal_store_emit(lua_State *, int, const char *, int);

static inline void luna_connect_global_signal(lua_State *L, const char *name) {
    lua_pushstring(L, LUNA_GLOBAL_SIGNALS);
    lua_rawget(L, LUA_REGISTRYINDEX);  // get global SignalStore
    lua_insert(L, -2);                 // insert before func
    luna_signal_store_connect(L, -2, name);
    lua_pop(L, 1);  // pop SignalStore
}

static inline void luna_disconnect_global_signal(lua_State *L, const char *name) {
    lua_pushstring(L, LUNA_GLOBAL_SIGNALS);
    lua_rawget(L, LUA_REGISTRYINDEX);  // get global SignalStore
    lua_insert(L, -2);                 // insert before func
    luna_signal_store_disconnect(L, -2, name);
    lua_pop(L, 1);  // pop SignalStore
}

static inline void luna_emit_global_signal(lua_State *L, const char *name, int nargs) {
    lua_pushstring(L, LUNA_GLOBAL_SIGNALS);
    lua_rawget(L, LUA_REGISTRYINDEX);  // get global SignalStore
    lua_insert(L, -nargs - 1);         // insert before args
    luna_signal_store_emit(L, -nargs - 1, name, nargs);
    lua_pop(L, 1);  // pop SignalStore
}

void luaC_register_signal_store(lua_State *);

#endif
