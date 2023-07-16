/*
 * button.c - button class
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
 * Copyright ©      2023 Abigail Teague <ateague063@gmail.com>
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

/** awesome button API
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
 * @coreclassmod button
 */

#include "button.h"
#include "common/lualib.h"
#include "common/object.h"
#include "objects/key.h"

/** Button object.
 *
 * @tfield int button The mouse button number, or 0 for any button.
 * @tfield table modifiers The modifier key table that should be pressed while the
 *   button is pressed.
 * @table button
 */

/** Get the number of instances.
 * @treturn int The number of button objects alive.
 * @staticfct instances
 */

/** Set a __index metamethod for all button instances.
 * @tparam function cb The meta-method
 * @staticfct set_index_miss_handler
 */

/** Set a __newindex metamethod for all button instances.
 * @tparam function cb The meta-method
 * @staticfct set_newindex_miss_handler
 */

/** When bound mouse button + modifiers are pressed.
 * @param ... One or more arguments are possible
 * @signal press
 */

/** When property changes.
 * @signal property::button
 */

/** When property changes.
 * @signal property::modifiers
 */

/** When bound mouse button + modifiers are pressed.
 * @param ... One or more arguments are possible
 * @signal release
 */

/** Create a new mouse button bindings.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static void lunaL_button_alloc(lua_State *L) {
    button_t *p = lua_newuserdatauv(L, sizeof(button_t), 1);
    p_clear(p, 1);
}

/** Set a button array with a Lua table.
 * \param L The Lua VM state.
 * \param oidx The index of the object to store items into.
 * \param idx The index of the Lua table.
 * \param buttons The array button to fill.
 */
void luaA_button_array_set(lua_State *L, int oidx, int idx, button_array_t *buttons) {
    luaA_checktable(L, idx);

    foreach (button, *buttons)
        luna_object_unref_item(L, oidx, *button);

    button_array_wipe(buttons);
    button_array_init(buttons);

    lua_pushnil(L);
    while (lua_next(L, idx))
        if (luaC_isinstance(L, -1, "Button"))
            button_array_append(buttons, luna_object_ref_item(L, oidx));
        else lua_pop(L, 1);
}

/** Push an array of button as an Lua table onto the stack.
 * \param L The Lua VM state.
 * \param oidx The index of the object to get items from.
 * \param buttons The button array to push.
 * \return The number of elements pushed on stack.
 */
int luaA_button_array_get(lua_State *L, int oidx, button_array_t *buttons) {
    lua_createtable(L, buttons->len, 0);
    for (int i = 0; i < buttons->len; i++) {
        luna_object_push_item(L, oidx, buttons->tab[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

lunaL_getter(button, modifiers) {
    button_t *b = luaC_checkuclass(L, 1, "Button");
    luaA_pushmodifiers(L, b->modifiers);
    return 1;
}

lunaL_setter(button, modifiers) {
    button_t *b  = luaC_checkuclass(L, 1, "Button");
    b->modifiers = luaA_tomodifiers(L, 2);
    luna_object_emit_signal(L, 1, ":property.modifiers", 0);
    return 0;
}

lunaL_getter(button, button) {
    button_t *b = luaC_checkuclass(L, 1, "Button");
    lua_pushinteger(L, b->button);
    return 1;
}

lunaL_setter(button, button) {
    button_t *b = luaC_checkuclass(L, 1, "Button");
    b->button   = luaL_checkinteger(L, 2);
    luna_object_emit_signal(L, 1, ":property.button", 0);
    return 0;
}

static luaL_Reg button_methods[] = {
    {"new", lunaL_object_constructor},
    {NULL,  NULL                    }
};

static luaC_Class button_class = {
    .name      = "Button",
    .parent    = "Object",
    .user_ctor = 1,
    .alloc     = lunaL_button_alloc,
    .gc        = NULL,
    .methods   = button_methods};

void luaC_register_button(lua_State *L) {
    static const luna_Prop props[] = {
        lunaL_prop(button, button),
        lunaL_prop(button, modifiers),
        {NULL, NULL, NULL}
    };

    lua_pushlightuserdata(L, &button_class);
    luna_register_withprops(L, -1, props);

    lua_pop(L, 1);
}

/* @DOC_cobject_COMMON@ */

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
