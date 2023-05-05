#ifndef LUNA_COMMON_OBJECT_H
#define LUNA_COMMON_OBJECT_H

#define LUNA_OBJECT_REGISTRY_KEY "lunaria.object.registry"

#include <luaclasslib.h>
#include "common/backtrace.h"

void luaC_register_object(lua_State *);

static inline const void *_luna_object_incref(lua_State *L, int idx) {
    const void *ptr = NULL;

    if ((ptr = lua_topointer(L, idx))) {
        lua_pushvalue(L, idx);                 // push object
        lua_rawsetp(L, -2, ptr);               // ref it in the table
        lua_getmetatable(L, -1);               // get the metatable
        lua_rawgetp(L, -1, ptr);               // get the refcount
        int count = lua_tointeger(L, -1) + 1;  // inc
        lua_pop(L, 1);                         // pop refcount
        lua_pushinteger(L, count);             // push new refcount
        lua_rawsetp(L, -2, ptr);               // set refcount
        lua_pop(L, 1);                         // pop metatable
    }
    lua_remove(L, idx);  // remove object
    return ptr;
}

static inline void _luna_object_decref(lua_State *L, const void *ptr) {
    if (!ptr) return;

    lua_getmetatable(L, -1);               // get metatable
    lua_rawgetp(L, -1, ptr);               // get refcount
    int count = lua_tointeger(L, -1) - 1;  // dec
    lua_pop(L, 1);                         // pop refcount

    // did something goof?
    if (count < 0) {
        buffer_t buf;
        backtrace_get(&buf);
        warn("BUG: Reference not found: %p\n%s", ptr, buf.s);

        /* Pop metatable */
        lua_pop(L, 1);
        return;
    }

    if (count) lua_pushinteger(L, count);  // push new refcount
    else lua_pushnil(L);                   // or nil if it's 0
    lua_rawsetp(L, -2, ptr);               // set new refcount
    lua_pop(L, 1);                         // pop metatable

    // remove ref from table if necessary
    if (!count) {
        lua_pushnil(L);
        lua_rawsetp(L, -2, ptr);
    }
}

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

int luna_object_connect_signal(lua_State *L, int idx, const char *name);

int luna_object_disconnect_signal(lua_State *L, int idx, const char *name);

int luna_object_emit_signal(lua_State *L, int idx, const char *name, int nargs);

int luna_class_connect_signal(lua_State *L, const char *class, const char *name);

int luna_class_disconnect_signal(lua_State *, const char *class, const char *);

int luna_class_emit_signal(lua_State *L, const char *class, const char *name, int nargs);

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
