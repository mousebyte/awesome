/*
 * key.h - Key class
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2023 Abigail Teague <ateague063@gmail.com>
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

#ifndef AWESOME_OBJECTS_KEY_H
#define AWESOME_OBJECTS_KEY_H

#include <lua.h>
#include <xcb/xproto.h>
#include <xkbcommon/xkbcommon.h>
#include "common/array.h"
#include "globalconf.h"

struct keyb_t {
    /** Key modifier */
    uint16_t      modifiers;
    /** Keysym */
    xcb_keysym_t  keysym;
    /** Keycode */
    xcb_keycode_t keycode;
};

ARRAY_FUNCS(keyb_t *, key, DO_NOTHING)

void luaC_register_key(lua_State *);

void luaA_key_array_set(lua_State *, int, int, key_array_t *);
int  luaA_key_array_get(lua_State *, int, key_array_t *);

int      luaA_pushmodifiers(lua_State *, uint16_t);
uint16_t luaA_tomodifiers(lua_State *L, int ud);

char *key_get_keysym_name(xkb_keysym_t keysym);

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
