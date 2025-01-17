/*
 * window.c - window object
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

/** Handling of X properties.
 *
 * This can not be used as a standalone class, but is instead referenced
 * explicitely in the classes, where it can be used. In the respective
 * classes,it then can be used via `classname:get_xproperty(...)` etc.
 * @classmod xproperties
 */

/**
 * @signal property::border_color
 */

/**
 * @signal property::border_width
 */

/**
 * @signal property::buttons
 */

/**
 * @signal property::opacity
 */

/**
 * @signal property::struts
 */

/**
 * @signal property::type
 */

#include "objects/window.h"
#include <luaclasslib.h>
#include "common/atoms.h"
#include "common/object.h"
#include "common/xutil.h"
#include "ewmh.h"
#include "luaa.h"
#include "objects/screen.h"
#include "property.h"
#include "xwindow.h"

static xcb_window_t window_get(window_t *window) {
    if (window->frame_window != XCB_NONE) return window->frame_window;
    return window->window;
}

static void lunaL_window_gc(lua_State *L, void *window) {
    button_array_wipe(&((window_t *)window)->buttons);
}

/** Get or set mouse buttons bindings on a window.
 * \param L The Lua VM state.
 * \return The number of elements pushed on the stack.
 */
static int luaA_window_buttons(lua_State *L) {
    window_t *window = luaC_checkuclass(L, 1, "Window");

    if (lua_gettop(L) == 2) {
        luaA_button_array_set(L, 1, 2, &window->buttons);
        luna_object_emit_signal(L, 1, ":property.buttons", 0);
        xwindow_buttons_grab(window->window, &window->buttons);
    }

    return luaA_button_array_get(L, 1, &window->buttons);
}

/** Return window struts (reserved space at the edge of the screen).
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int luaA_window_struts(lua_State *L) {
    window_t *window = luaC_checkuclass(L, 1, "Window");

    if (lua_gettop(L) == 2) {
        luaA_tostrut(L, 2, &window->strut);
        ewmh_update_strut(window->window, &window->strut);
        luna_object_emit_signal(L, 1, ":property.struts", 0);
        /* We don't know the correct screen, update them all */
        foreach (s, globalconf.screens)
            screen_update_workarea(*s);
    }

    return luaA_pushstrut(L, window->strut);
}

/** Set a window opacity.
 * \param L The Lua VM state.
 * \param idx The index of the window on the stack.
 * \param opacity The opacity value.
 */
void window_set_opacity(lua_State *L, int idx, double opacity) {
    window_t *window = luaC_checkuclass(L, idx, "Window");

    if (window->opacity != opacity) {
        window->opacity = opacity;
        xwindow_set_opacity(window_get(window), opacity);
        luna_object_emit_signal(L, idx, ":property.opacity", 0);
    }
}

void window_border_refresh(window_t *window) {
    if (!window->border_need_update) return;
    window->border_need_update = false;
    xwindow_set_border_color(window_get(window), &window->border_color);
    if (window->window)
        xcb_configure_window(
            globalconf.connection, window_get(window), XCB_CONFIG_WINDOW_BORDER_WIDTH,
            (uint32_t[]) {window->border_width});
}

static xproperty_t *luaA_find_xproperty(lua_State *L, int idx) {
    const char *name = luaL_checkstring(L, idx);
    foreach (prop, globalconf.xproperties)
        if (A_STREQ(prop->name, name)) return prop;
    luaL_argerror(L, idx, "Unknown xproperty");
    return NULL;
}

int window_set_xproperty(lua_State *L, xcb_window_t window, int prop_idx, int value_idx) {
    xproperty_t *prop = luaA_find_xproperty(L, prop_idx);
    xcb_atom_t   type;
    size_t       len;
    uint32_t     number;
    const void  *data;

    if (lua_isnil(L, value_idx)) {
        xcb_delete_property(globalconf.connection, window, prop->atom);
    } else {
        uint8_t format;
        if (prop->type == PROP_STRING) {
            data   = luaL_checklstring(L, value_idx, &len);
            type   = UTF8_STRING;
            format = 8;
        } else if (prop->type == PROP_NUMBER || prop->type == PROP_BOOLEAN) {
            if (prop->type == PROP_NUMBER)
                number = luaA_checkinteger_range(L, value_idx, 0, UINT32_MAX);
            else number = luaA_checkboolean(L, value_idx);
            data   = &number;
            len    = 1;
            type   = XCB_ATOM_CARDINAL;
            format = 32;
        } else fatal("Got an xproperty with invalid type");

        xcb_change_property(
            globalconf.connection, XCB_PROP_MODE_REPLACE, window, prop->atom, type, format, len,
            data);
    }
    return 0;
}

int window_get_xproperty(lua_State *L, xcb_window_t window, int prop_idx) {
    xproperty_t              *prop = luaA_find_xproperty(L, prop_idx);
    xcb_atom_t                type;
    void                     *data;
    xcb_get_property_reply_t *reply;
    uint32_t                  length;

    type   = prop->type == PROP_STRING ? UTF8_STRING : XCB_ATOM_CARDINAL;
    length = prop->type == PROP_STRING ? UINT32_MAX : 1;
    reply  = xcb_get_property_reply(
        globalconf.connection,
        xcb_get_property_unchecked(
            globalconf.connection, false, window, prop->atom, type, 0, length),
        NULL);
    if (!reply) return 0;

    data = xcb_get_property_value(reply);

    if (prop->type == PROP_STRING) lua_pushlstring(L, data, reply->value_len);
    else {
        if (reply->value_len <= 0) {
            p_delete(&reply);
            return 0;
        }
        if (prop->type == PROP_NUMBER) lua_pushinteger(L, *(uint32_t *)data);
        else lua_pushboolean(L, *(uint32_t *)data);
    }

    p_delete(&reply);
    return 1;
}

/** Change a xproperty.
 *
 * @param name The name of the X11 property
 * @param value The new value for the property
 * @function set_xproperty
 */
static int luaA_window_set_xproperty(lua_State *L) {
    window_t *w = luaC_checkuclass(L, 1, "Window");
    return window_set_xproperty(L, w->window, 2, 3);
}

/** Get the value of a xproperty.
 *
 * @param name The name of the X11 property
 * @function get_xproperty
 */
static int luaA_window_get_xproperty(lua_State *L) {
    window_t *w = luaC_checkuclass(L, 1, "Window");
    return window_get_xproperty(L, w->window, 2);
}

/* Translate a window_type_t into the corresponding EWMH atom.
 * @param type The type to return.
 * @return The EWMH atom for this type.
 */
uint32_t window_translate_type(window_type_t type) {
    switch (type) {
        case WINDOW_TYPE_NORMAL:
            return _NET_WM_WINDOW_TYPE_NORMAL;
        case WINDOW_TYPE_DESKTOP:
            return _NET_WM_WINDOW_TYPE_DESKTOP;
        case WINDOW_TYPE_DOCK:
            return _NET_WM_WINDOW_TYPE_DOCK;
        case WINDOW_TYPE_SPLASH:
            return _NET_WM_WINDOW_TYPE_SPLASH;
        case WINDOW_TYPE_DIALOG:
            return _NET_WM_WINDOW_TYPE_DIALOG;
        case WINDOW_TYPE_MENU:
            return _NET_WM_WINDOW_TYPE_MENU;
        case WINDOW_TYPE_TOOLBAR:
            return _NET_WM_WINDOW_TYPE_TOOLBAR;
        case WINDOW_TYPE_UTILITY:
            return _NET_WM_WINDOW_TYPE_UTILITY;
        case WINDOW_TYPE_DROPDOWN_MENU:
            return _NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
        case WINDOW_TYPE_POPUP_MENU:
            return _NET_WM_WINDOW_TYPE_POPUP_MENU;
        case WINDOW_TYPE_TOOLTIP:
            return _NET_WM_WINDOW_TYPE_TOOLTIP;
        case WINDOW_TYPE_NOTIFICATION:
            return _NET_WM_WINDOW_TYPE_NOTIFICATION;
        case WINDOW_TYPE_COMBO:
            return _NET_WM_WINDOW_TYPE_COMBO;
        case WINDOW_TYPE_DND:
            return _NET_WM_WINDOW_TYPE_DND;
    }
    return _NET_WM_WINDOW_TYPE_NORMAL;
}

lunaL_getter(window, window) {
    window_t *w = luaC_checkuclass(L, 1, "Window");
    lua_pushinteger(L, w->window);
    return 1;
}

lunaL_getter(window, _opacity) {
    window_t *window = luaC_checkuclass(L, 1, "Window");
    if (window->opacity >= 0) lua_pushnumber(L, window->opacity);
    else lua_pushnumber(L, 1);
    return 1;
}

lunaL_setter(window, _opacity) {
    if (lua_isnil(L, 2)) window_set_opacity(L, 1, -1);
    else {
        double d = luaL_checknumber(L, 2);
        if (d >= 0 && d <= 1) window_set_opacity(L, 1, d);
    }
    return 0;
}

lunaL_getter(window, _border_color) {
    window_t *w = luaC_checkuclass(L, 1, "Window");
    luaA_pushcolor(L, w->border_color);
    return 1;
}

lunaL_setter(window, _border_color) {
    window_t   *window = luaC_checkuclass(L, 1, "Window");
    size_t      len;
    const char *color_name = luaL_checklstring(L, 2, &len);

    if (color_name && color_init_reply(color_init_unchecked(
                          &window->border_color, color_name, len, globalconf.visual))) {
        window->border_need_update = true;
        luna_object_emit_signal(L, -3, ":property.border_color", 0);
    }

    return 0;
}

lunaL_getter(window, _border_width) {
    window_t *w = luaC_checkuclass(L, 1, "Window");
    lua_pushinteger(L, w->border_width);
    return 1;
}

lunaL_setter(window, _border_width) {
    window_set_border_width(L, 1, round(luaA_checknumber_range(L, 2, 0, MAX_X11_SIZE)));
    return 0;
}

lunaL_getter(window, type) {
    window_t *w = luaC_checkuclass(L, 1, "Window");
    switch (w->type) {
        case WINDOW_TYPE_DESKTOP:
            lua_pushliteral(L, "desktop");
            break;
        case WINDOW_TYPE_DOCK:
            lua_pushliteral(L, "dock");
            break;
        case WINDOW_TYPE_SPLASH:
            lua_pushliteral(L, "splash");
            break;
        case WINDOW_TYPE_DIALOG:
            lua_pushliteral(L, "dialog");
            break;
        case WINDOW_TYPE_MENU:
            lua_pushliteral(L, "menu");
            break;
        case WINDOW_TYPE_TOOLBAR:
            lua_pushliteral(L, "toolbar");
            break;
        case WINDOW_TYPE_UTILITY:
            lua_pushliteral(L, "utility");
            break;
        case WINDOW_TYPE_DROPDOWN_MENU:
            lua_pushliteral(L, "dropdown_menu");
            break;
        case WINDOW_TYPE_POPUP_MENU:
            lua_pushliteral(L, "popup_menu");
            break;
        case WINDOW_TYPE_TOOLTIP:
            lua_pushliteral(L, "tooltip");
            break;
        case WINDOW_TYPE_NOTIFICATION:
            lua_pushliteral(L, "notification");
            break;
        case WINDOW_TYPE_COMBO:
            lua_pushliteral(L, "combo");
            break;
        case WINDOW_TYPE_DND:
            lua_pushliteral(L, "dnd");
            break;
        case WINDOW_TYPE_NORMAL:
            lua_pushliteral(L, "normal");
            break;
        default:
            return 0;
    }
    return 1;
}

lunaL_setter(window, type) {
    window_t     *w = luaC_checkuclass(L, 1, "Window");
    window_type_t type;
    const char   *buf = luaL_checkstring(L, -1);

    if (A_STREQ(buf, "desktop")) type = WINDOW_TYPE_DESKTOP;
    else if (A_STREQ(buf, "dock")) type = WINDOW_TYPE_DOCK;
    else if (A_STREQ(buf, "splash")) type = WINDOW_TYPE_SPLASH;
    else if (A_STREQ(buf, "dialog")) type = WINDOW_TYPE_DIALOG;
    else if (A_STREQ(buf, "menu")) type = WINDOW_TYPE_MENU;
    else if (A_STREQ(buf, "toolbar")) type = WINDOW_TYPE_TOOLBAR;
    else if (A_STREQ(buf, "utility")) type = WINDOW_TYPE_UTILITY;
    else if (A_STREQ(buf, "dropdown_menu")) type = WINDOW_TYPE_DROPDOWN_MENU;
    else if (A_STREQ(buf, "popup_menu")) type = WINDOW_TYPE_POPUP_MENU;
    else if (A_STREQ(buf, "tooltip")) type = WINDOW_TYPE_TOOLTIP;
    else if (A_STREQ(buf, "notification")) type = WINDOW_TYPE_NOTIFICATION;
    else if (A_STREQ(buf, "combo")) type = WINDOW_TYPE_COMBO;
    else if (A_STREQ(buf, "dnd")) type = WINDOW_TYPE_DND;
    else if (A_STREQ(buf, "normal")) type = WINDOW_TYPE_NORMAL;
    else {
        luaA_warn(L, "Unknown window type '%s'", buf);
        return 0;
    }

    if (w->type != type) {
        w->type = type;
        if (w->window != XCB_WINDOW_NONE)
            ewmh_update_window_type(w->window, window_translate_type(w->type));
        luna_object_emit_signal(L, 1, ":property.type", 0);
    }

    return 0;
}

static luaL_Reg window_methods[] = {
    {"struts",        luaA_window_struts       },
    {"_buttons",      luaA_window_buttons      },
    {"set_xproperty", luaA_window_set_xproperty},
    {"get_xproperty", luaA_window_get_xproperty},
    {NULL,            NULL                     }
};

luaC_Class window_class = {
    .name      = "Window",
    .parent    = "Object",
    .user_ctor = 0,
    .alloc     = NULL,
    .gc        = lunaL_window_gc,
    .methods   = window_methods};

void luaC_register_window(lua_State *L) {
    static const luna_Prop props[] = {
        lunaL_readonly_prop(window, window),
        lunaL_prop(window, _opacity),
        lunaL_prop(window, _border_color),
        lunaL_prop(window, _border_width),
        lunaL_prop(window, type),
        {NULL, NULL, NULL}
    };

    lua_pushlightuserdata(L, &window_class);
    luna_register_withprops(L, -1, props);

    lua_pop(L, 1);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
