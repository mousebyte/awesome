#include "common/signals.h"
#include <string.h>

static inline int _cptr_cmp(const void *a, const void *b) {
    const void **x = (const void **)a, **y = (const void **)b;
    return *x > *y ? 1 : (*x < *y ? -1 : 0);
}

DO_BARRAY(const void *, cptr, DO_NOTHING, _cptr_cmp)

typedef struct {
    unsigned long id;
    size_t        nslots;
    cptr_array_t  strong;
    cptr_array_t  weak;
} signal_t;

static inline int _signal_cmp(const void *a, const void *b) {
    const signal_t *x = a, *y = b;
    return x->id > y->id ? 1 : (x->id < y->id ? -1 : 0);
}

static inline void _signal_wipe(signal_t *sig) {
    cptr_array_wipe(&sig->strong);
    cptr_array_wipe(&sig->weak);
}

DO_BARRAY(signal_t, signal, _signal_wipe, _signal_cmp)

static inline signal_t *signal_array_getbyid(signal_array_t *arr, unsigned long id) {
    signal_t sig = {.id = id};
    return signal_array_lookup(arr, &sig);
}

static int signal_interface_init(lua_State *L) {
    lua_setfield(L, 1, "_id");     // self._id = arg 2
    lua_setfield(L, 1, "_store");  // self._store = arg 1
    return 0;
}

static int signal_interface_connect(lua_State *L) {
    lua_getfield(L, 1, "_store");
    lua_getfield(L, 1, "_id");
    signal_array_t *arr      = luaC_checkuclass(L, -2, "SignalStore");
    unsigned long   id       = luaL_checknumber(L, -1);
    const void     *ref      = lua_topointer(L, 2);
    signal_t       *sigfound = signal_array_getbyid(arr, id);
    lua_rotate(L, 2, -1);  // rotate func to top
    if (sigfound) {
        luaC_uvrawsetp(L, -3, 2, ref);
        cptr_array_insert(&sigfound->strong, ref);
        sigfound->nslots++;
    } else {
        signal_t sig = {.id = id, .nslots = 1};
        luaC_uvrawsetp(L, -3, 2, ref);
        cptr_array_insert(&sig.strong, ref);
        signal_array_insert(arr, sig);
    }
    return 0;
}

static int signal_interface_connect_weak(lua_State *L) {
    lua_getfield(L, 1, "_store");
    lua_getfield(L, 1, "_id");
    signal_array_t *arr      = luaC_checkuclass(L, -2, "SignalStore");
    unsigned long   id       = luaL_checknumber(L, -1);
    const void     *ref      = lua_topointer(L, 2);
    signal_t       *sigfound = signal_array_getbyid(arr, id);
    lua_rotate(L, 2, -1);  // rotate func to top
    if (sigfound) {
        luaC_uvrawsetp(L, -3, 3, ref);
        cptr_array_insert(&sigfound->weak, ref);
        sigfound->nslots++;
    } else {
        signal_t sig = {.id = id, .nslots = 1};
        luaC_uvrawsetp(L, -3, 3, ref);
        cptr_array_insert(&sig.weak, ref);
        signal_array_insert(arr, sig);
    }
    return 0;
}

static int signal_interface_disconnect(lua_State *L) {
    lua_getfield(L, 1, "_store");
    lua_getfield(L, 1, "_id");
    signal_array_t *arr      = luaC_checkuclass(L, -2, "SignalStore");
    unsigned long   id       = luaL_checknumber(L, -1);
    const void     *ref      = lua_topointer(L, 2);
    signal_t       *sigfound = signal_array_getbyid(arr, id);
    lua_rotate(L, 2, -1);  // rotate func to top
    if (sigfound) {
        const void **elem;
        if ((elem = cptr_array_lookup(&sigfound->strong, &ref))) {
            lua_pushnil(L);
            luaC_uvrawsetp(L, -4, 2, *elem);
            cptr_array_remove(&sigfound->strong, elem);
            sigfound->nslots--;
        } else if ((elem = cptr_array_lookup(&sigfound->weak, &ref))) {
            lua_pushnil(L);
            luaC_uvrawsetp(L, -4, 3, *elem);
            cptr_array_remove(&sigfound->weak, elem);
            sigfound->nslots--;
        }
        if (sigfound->nslots == 0) {
            cptr_array_wipe(&sigfound->strong);
            cptr_array_wipe(&sigfound->weak);
            signal_array_remove(arr, sigfound);
        }
    }
    return 0;
}

static int signal_interface_call(lua_State *L) {
    lua_getfield(L, 1, "_store");
    lua_getfield(L, 1, "_id");
    signal_array_t *arr      = luaC_checkuclass(L, -2, "SignalStore");
    unsigned long   id       = luaL_checknumber(L, -1);
    signal_t       *sigfound = signal_array_getbyid(arr, id);
    lua_rotate(L, 2, 2);  // rotate args to top
    if (sigfound) {
        int nargs = lua_gettop(L) - 3;  // -3 for self, store, and id
        lua_getiuservalue(L, 2, 2);     // get strong table from store
        foreach (slot, sigfound->strong) {
            lua_rawgetp(L, -1, *slot);       // get func from strong table
            for (int i = 0; i < nargs; i++)  // push copies of args
                lua_pushvalue(L, i + 3);
            lua_call(L, nargs, 0);
        }
        lua_pop(L, 1);               // pop strong table
        lua_getiuservalue(L, 2, 3);  // get weak table from store
        foreach (slot, sigfound->weak) {
            lua_rawgetp(L, -1, *slot);       // get func from weak table
            for (int i = 0; i < nargs; i++)  // push copies of args
                lua_pushvalue(L, i + 3);
            lua_call(L, nargs, 0);
        }
    }
    return 0;
}

static luaL_Reg signal_interface_methods[] = {
    {"new",          signal_interface_init        },
    {"connect",      signal_interface_connect     },
    {"connect_weak", signal_interface_connect_weak},
    {"disconnect",   signal_interface_disconnect  },
    {"__call",       signal_interface_call        },
    {NULL,           NULL                         }
};

static luaC_Class signal_interface_class = {
    .name      = "SignalInterface",
    .parent    = NULL,
    .user_ctor = 0,
    .alloc     = NULL,
    .gc        = NULL,
    .methods   = signal_interface_methods};

static void signal_store_alloc(lua_State *L) {
    signal_array_t *arr = lua_newuserdatauv(L, sizeof(signal_array_t), 3);
    lua_newtable(L);  // strong slot table
    lua_setiuservalue(L, -2, 2);
    lua_newtable(L);  // weak slot table
    lua_newtable(L);  // weak slot metatable
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
    lua_setiuservalue(L, -2, 3);
    signal_array_init(arr);
}

static void signal_store_gc(lua_State *L, void *p) {
    signal_array_t *arr = (signal_array_t *)p;
    signal_array_wipe(arr);
}

static int signal_store_index(lua_State *L) {
    if (luaC_deferindex(L) == LUA_TNIL && lua_isstring(L, 2)) {
        lua_pop(L, 1);
        luaC_construct(L, 2, "SignalInterface");
    }
    return 1;
}

luaC_Class signal_store_class = {
    .name      = "SignalStore",
    .parent    = NULL,
    .user_ctor = 0,
    .alloc     = signal_store_alloc,
    .gc        = signal_store_gc,
    .methods   = NULL};

void luaC_register_signal_store(lua_State *L) {
    lua_pushlightuserdata(L, &signal_interface_class);
    luaC_register(L, -1);
    lua_pushlightuserdata(L, &signal_store_class);
    luaC_register(L, -1);
    luaC_injectindex(L, -1, signal_store_index);
}
