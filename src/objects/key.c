/*
 * key.c - Key bindings configuration management
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2008 Pierre Habouzit <madcoder@debian.org>
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

/** awesome key API
 *
 * Furthermore to the classes described here, one can also use signals as
 * described in @{signals}.
 *
 * Some signal names are starting with a dot. These dots are artefacts from
 * the documentation generation, you get the real signal name by
 * removing the starting dot.
 *
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @coreclassmod key
 */

#include "objects/key.h"
#include "common/lualib.h"
#include "common/xutil.h"
#include "luaa.h"
#include "xkb.h"

/* XStringToKeysym() */
#include <X11/Xlib.h>
#include <glib.h>
#include <xkbcommon/xkbcommon.h>

/** Key object.
 *
 * @tfield string key The key to trigger an event.
 * @tfield string keysym Same as key, but return the name of the key symbol. It
 *   can be identical to key, but for characters like '.' it will return
 *   'period'.
 * @tfield table modifiers The modifier key that should be pressed while the
 *   key is pressed. An array with all the modifiers. Valid modifiers are: Any,
 *   Mod1, Mod2, Mod3, Mod4, Mod5, Shift, Lock and Control.
 * @table key
 */

/**
 * @signal press
 */

/**
 * @signal property::key
 */

/**
 * @signal property::modifiers
 */

/**
 * @signal release
 */

/** Get the number of instances.
 *
 * @return The number of key objects alive.
 * @staticfct instances
 */

/** Set a __index metamethod for all key instances.
 * @tparam function cb The meta-method
 * @staticfct set_index_miss_handler
 */

/** Set a __newindex metamethod for all key instances.
 * @tparam function cb The meta-method
 * @staticfct set_newindex_miss_handler
 */

/** Create a new key object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static void lunaL_key_alloc(lua_State *L) {
    keyb_t *p = lua_newuserdatauv(L, sizeof(keyb_t), 1);
    p_clear(p, 1);
}

/** Set a key array with a Lua table.
 * \param L The Lua VM state.
 * \param oidx The index of the object to store items into.
 * \param idx The index of the Lua table.
 * \param keys The array key to fill.
 */
void luaA_key_array_set(lua_State *L, int oidx, int idx, key_array_t *keys) {
    luaA_checktable(L, idx);

    foreach (key, *keys)
        luna_object_unref_item(L, oidx, *key);

    key_array_wipe(keys);
    key_array_init(keys);

    lua_pushnil(L);
    while (lua_next(L, idx))
        if (luaC_isinstance(L, -1, "Key")) key_array_append(keys, luna_object_ref_item(L, oidx));
        else lua_pop(L, 1);
}

/** Push an array of key as an Lua table onto the stack.
 * \param L The Lua VM state.
 * \param oidx The index of the object to get items from.
 * \param keys The key array to push.
 * \return The number of elements pushed on stack.
 */
int luaA_key_array_get(lua_State *L, int oidx, key_array_t *keys) {
    lua_createtable(L, keys->len, 0);
    for (int i = 0; i < keys->len; i++) {
        luna_object_push_item(L, oidx, keys->tab[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/** Push a modifier set to a Lua table.
 * \param L The Lua VM state.
 * \param modifiers The modifier.
 * \return The number of elements pushed on stack.
 */
int luaA_pushmodifiers(lua_State *L, uint16_t modifiers) {
    lua_newtable(L);
    {
        int i = 1;
        for (uint32_t maski = XCB_MOD_MASK_SHIFT; maski <= XCB_BUTTON_MASK_ANY; maski <<= 1)
            if (maski & modifiers) {
                const char *mod;
                size_t      slen;
                xutil_key_mask_tostr(maski, &mod, &slen);
                lua_pushlstring(L, mod, slen);
                lua_rawseti(L, -2, i++);
            }
    }
    return 1;
}

/** Take a modifier table from the stack and return modifiers mask.
 * \param L The Lua VM state.
 * \param ud The index of the table.
 * \return The mask value.
 */
uint16_t luaA_tomodifiers(lua_State *L, int ud) {
    luaA_checktable(L, ud);
    ssize_t  len = luaA_rawlen(L, ud);
    uint16_t mod = XCB_NONE;
    for (int i = 1; i <= len; i++) {
        lua_rawgeti(L, ud, i);
        const char *key = luaL_checkstring(L, -1);
        mod |= xutil_key_mask_fromstr(key);
        lua_pop(L, 1);
    }
    return mod;
}

static int luaA_key_set_modifiers(lua_State *L, keyb_t *k) {
    k->modifiers = luaA_tomodifiers(L, -1);
    luna_object_emit_signal(L, -3, ":property.modifiers", 0);
    return 0;
}

/* It's caller's responsibility to release the returned string. */
char *key_get_keysym_name(xkb_keysym_t keysym) {
    const ssize_t bufsize = 64;
    char         *buf     = p_new(char, bufsize);
    ssize_t       len;

    if ((len = xkb_keysym_get_name(keysym, buf, bufsize)) == -1) {
        p_delete(&buf);
        return NULL;
    }
    if (len + 1 > bufsize) {
        p_realloc(&buf, len + 1);
        if (xkb_keysym_get_name(keysym, buf, len + 1) != len) {
            p_delete(&buf);
            return NULL;
        }
    }
    return buf;
}

lunaL_getter(key, key) {
    keyb_t *k = luaC_checkuclass(L, 1, "Key");

    if (k->keycode) {
        char buf[12];
        int  slen = snprintf(buf, sizeof(buf), "#%u", k->keycode);
        lua_pushlstring(L, buf, slen);
    } else {
        char *name = key_get_keysym_name(k->keysym);
        if (!name) return 0;
        lua_pushstring(L, name);
        p_delete(&name);
    }
    return 1;
}

lunaL_setter(key, key) {
    size_t      len;
    const char *str = luaL_checklstring(L, 2, &len);
    if (len <= 0 || !str) return 0;

    keyb_t *key = luaC_checkuclass(L, 1, "Key");

    if (len == 1) {
        key->keycode = 0;
        key->keysym  = str[0];
    } else if (str[0] == '#') {
        key->keycode = atoi(str + 1);
        key->keysym  = 0;
    } else {
        key->keycode = 0;

        if ((key->keysym = XStringToKeysym(str)) == NoSymbol) {
            glong    length;
            gunichar unicode;

            if (!g_utf8_validate(str, -1, NULL)) {
                luaA_warn(L, "failed to convert \"%s\" into keysym (invalid UTF-8 string)", str);
                return 0;
            }

            length = g_utf8_strlen(str, -1); /* This function counts combining characters. */
            if (length <= 0) {
                luaA_warn(L, "failed to convert \"%s\" into keysym (empty UTF-8 string)", str);
                return 0;
            } else if (length > 1) {
                gchar *composed = g_utf8_normalize(str, -1, G_NORMALIZE_DEFAULT_COMPOSE);
                if (g_utf8_strlen(composed, -1) != 1) {
                    p_delete(&composed);
                    luaA_warn(
                        L,
                        "failed to convert \"%s\" into keysym (failed to compose a single "
                        "character)",
                        str);
                    return 0;
                }
                unicode = g_utf8_get_char(composed);
                p_delete(&composed);
            } else unicode = g_utf8_get_char(str);

            if (unicode == (gunichar)-1 || unicode == (gunichar)-2) {
                luaA_warn(
                    L, "failed to convert \"%s\" into keysym (neither keysym nor single unicode)",
                    str);
                return 0;
            }

            /* Unicode-to-Keysym Conversion
             *
             * http://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html#keysym_encoding
             */
            if (unicode <= 0x0ff) key->keysym = unicode;
            else if (unicode >= 0x100 && unicode <= 0x10ffff) key->keysym = unicode | (1 << 24);
            else {
                luaA_warn(
                    L, "failed to convert \"%s\" into keysym (unicode out of range): \"%u\"", str,
                    unicode);
                return 0;
            }
        }
    }

    luna_object_emit_signal(L, 1, ":property.key", 0);
    return 0;
}

lunaL_getter(key, keysym) {
    keyb_t *k    = luaC_checkuclass(L, 1, "Key");
    char   *name = key_get_keysym_name(k->keysym);
    if (!name) return 0;
    lua_pushstring(L, name);
    free(name);
    return 1;
}

lunaL_getter(key, modifiers) {
    keyb_t *k = luaC_checkuclass(L, 1, "Key");
    luaA_pushmodifiers(L, k->modifiers);
    return 1;
}

lunaL_setter(key, modifiers) {
    keyb_t *k    = luaC_checkuclass(L, 1, "Key");
    k->modifiers = luaA_tomodifiers(L, 2);
    luna_object_emit_signal(L, 1, ":property.modifiers", 0);
    return 0;
}

luaC_Class key_class = {
    .name      = "Key",
    .parent    = "Object",
    .user_ctor = 1,
    .alloc     = lunaL_key_alloc,
    .gc        = NULL,
    .methods   = NULL};

void luaC_register_key(lua_State *L) {
    lua_pushlightuserdata(L, &key_class);
    luaC_register(L, -1);
    lunaL_prop(key, key);
    lunaL_readonly_prop(key, keysym);
    lunaL_prop(key, modifiers);
    lua_pop(L, 1);
}

/* @DOC_cobject_COMMON@ */

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
