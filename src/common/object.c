#include "object.h"
#include <moonauxlib.h>
#include "signal.h"

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
    lua_pushcfunction(L, object_inherited);
    lua_setfield(L, -2, "__inherited");
    lua_pushcfunction(L, make_proptable);
    lua_setfield(L, -2, "Properties");
}

static inline int _get_class_signal(lua_State *L, const char *class, const char *name) {
    if (luaC_pushclass(L, class)) {
        lua_getfield(L, -1, "Signals");
        lua_getfield(L, -1, (name));
        lua_insert(L, -3);
        lua_pop(L, 2);
    }
    return lua_type(L, -1);
}

#define _signal_connect(L) (lua_insert((L), -2), luaC_pmcall((L), "connect", 1, 0, 0))
#define _signal_disconnect(L) (lua_insert((L), -2), luaC_pmcall((L), "disconnect", 1, 0, 0))
#define _signal_emit(L, nargs) (lua_insert((L), -(nargs)), lua_pcall((L), (nargs), 0, 0))

int luna_object_connect_signal(lua_State *L, int idx, const char *name) {
    int ret = LUA_ERRRUN;
    if (lua_getfield(L, idx, name) == LUA_TUSERDATA) {
        ret = _signal_connect(L);
        lua_pop(L, 1);
    }
    return ret;
}

int luna_object_disconnect_signal(lua_State *L, int idx, const char *name) {
    int ret = LUA_ERRRUN;
    if (lua_getfield(L, idx, name) == LUA_TUSERDATA) {
        ret = _signal_disconnect(L);
        lua_pop(L, 1);
    }
    return ret;
}

int luna_object_emit_signal(lua_State *L, int idx, const char *name, int nargs) {
    return lua_getfield(L, idx, name) == LUA_TUSERDATA ? _signal_emit(L, nargs) : LUA_ERRRUN;
}

int luna_class_connect_signal(lua_State *L, const char *class, const char *name) {
    int ret = LUA_ERRRUN;
    if (_get_class_signal(L, class, name) == LUA_TUSERDATA) {
        ret = _signal_connect(L);
        lua_pop(L, 1);
    }
    return ret;
}

int luna_class_disconnect_signal(lua_State *L, const char *class, const char *name) {
    int ret = LUA_ERRRUN;
    if (_get_class_signal(L, class, name) == LUA_TUSERDATA) {
        ret = _signal_disconnect(L);
        lua_pop(L, 1);
    }
    return ret;
}

int luna_class_emit_signal(lua_State *L, const char *class, const char *name, int nargs) {
    return _get_class_signal(L, class, name) == LUA_TUSERDATA ? _signal_emit(L, nargs) : LUA_ERRRUN;
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
