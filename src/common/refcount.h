#include <lua.h>
#include "backtrace.h"

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
