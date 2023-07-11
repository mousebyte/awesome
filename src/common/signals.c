/*
 * signals.c - signal and related classes
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

#include "signals.h"
#include <string.h>
#include "lualib.h"
#include "refcount.h"

static inline int _cptr_cmp(const void *a, const void *b) {
    const void **x = (const void **)a, **y = (const void **)b;
    return *x > *y ? 1 : (*x < *y ? -1 : 0);
}

DO_BARRAY(const void *, cptr, DO_NOTHING, _cptr_cmp)

typedef struct {
    unsigned long id;
    cptr_array_t  slots;
} signal_t;

static inline int _signal_cmp(const void *a, const void *b) {
    const signal_t *x = a, *y = b;
    return x->id > y->id ? 1 : (x->id < y->id ? -1 : 0);
}

static inline void _signal_wipe(signal_t *sig) {
    cptr_array_wipe(&sig->slots);
}

DO_BARRAY(signal_t, signal, _signal_wipe, _signal_cmp)

static inline signal_t *signal_array_getbyid(signal_array_t *arr, unsigned long id) {
    signal_t sig = {.id = id};
    return signal_array_lookup(arr, &sig);
}

void luna_signal_store_connect(lua_State *L, int idx, const char *name) {
    luaA_checkfunction(L, -1);
    signal_array_t *arr      = luaC_checkuclass(L, idx, "SignalStore");
    unsigned long   id       = a_strhash((unsigned const char *)name);
    signal_t       *sigfound = signal_array_getbyid(arr, id);
    lua_getiuservalue(L, idx, 2);                  // get slot table
    const void *ref = _luna_object_incref(L, -2);  // ref func

    if (sigfound) {
        cptr_array_insert(&sigfound->slots, ref);
    } else {
        signal_t sig = {.id = id};
        cptr_array_init(&sig.slots);
        cptr_array_insert(&sig.slots, ref);
        signal_array_insert(arr, sig);
    }

    lua_pop(L, 2);  // pop slot table and func
}

void luna_signal_store_disconnect(lua_State *L, int idx, const char *name) {
    signal_array_t *arr      = luaC_checkuclass(L, idx, "SignalStore");
    unsigned long   id       = a_strhash((unsigned const char *)name);
    signal_t       *sigfound = signal_array_getbyid(arr, id);
    const void     *ref = lua_islightuserdata(L, -1) ? lua_touserdata(L, -1) : lua_topointer(L, -1);

    if (sigfound) {
        const void **elem;
        if ((elem = cptr_array_lookup(&sigfound->slots, &ref))) {
            cptr_array_remove(&sigfound->slots, elem);
        }
        if (sigfound->slots.len == 0) {
            cptr_array_wipe(&sigfound->slots);
            signal_array_remove(arr, sigfound);
        }
        lua_getiuservalue(L, idx, 2);  // get slot table
        _luna_object_decref(L, ref);   // unref func
        lua_pop(L, 1);                 // pop slot table
    }

    lua_pop(L, 1);  // pop func
}

void luna_signal_store_emit(lua_State *L, int idx, const char *name, int nargs) {
    signal_array_t *arr      = luaC_checkuclass(L, idx, "SignalStore");
    unsigned long   id       = a_strhash((unsigned const char *)name);
    signal_t       *sigfound = signal_array_getbyid(arr, id);
    if (sigfound) {
        int start = lua_gettop(L) - nargs;
        lua_getiuservalue(L, idx, 2);  // get slot table from store
        foreach (slot, sigfound->slots) {
            lua_rawgetp(L, -1, *slot);  // get func from slot table
            for (int i = start; i < start + nargs; i++)
                lua_pushvalue(L, i);    // push copies of args
            lua_pcall(L, nargs, 0, 0);  // call the func
        }
        lua_pop(L, 1);  // pop slot table
    }
    lua_pop(L, nargs);  // pop args
}

static int signal_interface_init(lua_State *L) {
    lua_setfield(L, 1, "_name");   // self._id = arg 2
    lua_setfield(L, 1, "_store");  // self._store = arg 1
    return 0;
}

static int signal_interface_connect(lua_State *L) {
    lua_getfield(L, 1, "_store");
    lua_getfield(L, 1, "_name");
    lua_pushvalue(L, 2);  // push func
    luna_signal_store_connect(L, -3, lua_tostring(L, -2));
    // construct connection object from store, id, and func pointer
    lua_pushlightuserdata(L, (void *)lua_topointer(L, 2));
    luaC_construct(L, 3, "Connection");
    return 1;
}

static int signal_interface_disconnect(lua_State *L) {
    if (lua_getfield(L, 1, "_store") != LUA_TUSERDATA) return 0;
    lua_getfield(L, 1, "_name");
    lua_pushvalue(L, 2);  // push func
    luna_signal_store_disconnect(L, -3, lua_tostring(L, -2));
    return 0;
}

static int signal_interface_call(lua_State *L) {
    lua_getfield(L, 1, "_store");
    lua_getfield(L, 1, "_name");
    luna_signal_store_emit(L, -2, lua_tostring(L, -1), lua_gettop(L) - 3);
    return 0;
}

static luaL_Reg signal_interface_methods[] = {
    {"new",        signal_interface_init      },
    {"connect",    signal_interface_connect   },
    {"disconnect", signal_interface_disconnect},
    {"__call",     signal_interface_call      },
    {NULL,         NULL                       }
};

static luaC_Class signal_interface_class = {
    .name      = "SignalInterface",
    .parent    = NULL,
    .user_ctor = 0,
    .alloc     = NULL,
    .gc        = NULL,
    .methods   = signal_interface_methods};

static void signal_store_alloc(lua_State *L) {
    signal_array_t *arr = lua_newuserdatauv(L, sizeof(signal_array_t), 2);
    lua_newtable(L);  // slot table
    lua_newtable(L);  // slot metatable (for refcount)
    lua_setmetatable(L, -2);
    lua_setiuservalue(L, -2, 2);
    signal_array_init(arr);
}

static void signal_store_gc(lua_State *L, void *p) {
    signal_array_t *arr = (signal_array_t *)p;
    foreach (sig, *arr)
        cptr_array_wipe(&sig->slots);
    signal_array_wipe(arr);
}

static int signal_store_index(lua_State *L) {
    if (luaC_deferindex(L) == LUA_TNIL && lua_isstring(L, 2)) {
        lua_pop(L, 1);
        luaC_construct(L, 2, "SignalInterface");
    }
    return 1;
}

static luaC_Class signal_store_class = {
    .name      = "SignalStore",
    .parent    = NULL,
    .user_ctor = 0,
    .alloc     = signal_store_alloc,
    .gc        = signal_store_gc,
    .methods   = NULL};

static int connection_init(lua_State *L) {
    lua_setfield(L, 1, "_value");  // self._value = arg 3
    lua_setfield(L, 1, "_name");   // self._id = arg 2
    lua_setfield(L, 1, "_store");  // self._store = arg 1
    return 0;
}

static int connection_connected(lua_State *L) {
    int ret = 0;
    if (lua_getfield(L, 1, "_store") == LUA_TUSERDATA) {
        lua_getfield(L, 1, "_name");
        signal_array_t *arr = lua_touserdata(L, -2);
        signal_t       *sigfound =
            signal_array_getbyid(arr, a_strhash((const unsigned char *)lua_tostring(L, -1)));
        if (sigfound) {
            lua_getfield(L, 1, "_value");
            const void *ref = lua_touserdata(L, -1);
            ret             = (cptr_array_lookup(&sigfound->slots, &ref) != NULL);
        }
    }
    lua_pushboolean(L, ret);
    return 1;
}

static int connection_scoped(lua_State *L) {
    lua_getfield(L, 1, "_store");
    lua_getfield(L, 1, "_name");
    lua_getfield(L, 1, "_value");
    luaC_construct(L, 3, "ScopedConnection");
    return 1;
}

static int scoped_connection_gc(lua_State *L) {
    lua_getfield(L, 1, "_value");
    luaC_pmcall(L, "disconnect", 1, 0, 0);
    return 0;
}

static luaL_Reg connection_methods[] = {
    {"new",        connection_init            },
    {"connected",  connection_connected       },
    {"disconnect", signal_interface_disconnect},
    {"scoped",     connection_scoped          },
    {NULL,         NULL                       }
};

static luaL_Reg scoped_connection_methods[] = {
    {"new",        connection_init            },
    {"connected",  connection_connected       },
    {"disconnect", signal_interface_disconnect},
    {NULL,         NULL                       }
};

void luaC_register_signal_store(lua_State *L) {
    luaC_newclass(L, "Connection", NULL, connection_methods);
    luaC_getbase(L, -1);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_pop(L, 2);
    luaC_newclass(L, "ScopedConnection", NULL, scoped_connection_methods);
    luaC_getbase(L, -1);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_pushcfunction(L, scoped_connection_gc);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 2);
    lua_pushlightuserdata(L, &signal_interface_class);
    luaC_register(L, -1);
    lua_pushlightuserdata(L, &signal_store_class);
    luaC_register(L, -1);
    luaC_injectindex(L, -1, signal_store_index);
    lua_pop(L, 2);

    luaC_construct(L, 0, "SignalStore");
    lua_setfield(L, LUA_REGISTRYINDEX, LUNA_GLOBAL_SIGNALS);
}
