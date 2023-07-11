/*
 * drawable.h - drawable class
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2010-2012 Uli Schlachter <psychon@znc.in>
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

#ifndef AWESOME_OBJECTS_DRAWABLE_H
#define AWESOME_OBJECTS_DRAWABLE_H

#include "draw.h"

typedef void drawable_refresh_callback(void *);

/** drawable type */
typedef struct drawable_t {
    /** The pixmap we are drawing to. */
    xcb_pixmap_t               pixmap;
    /** Surface for drawing. */
    cairo_surface_t           *surface;
    /** The geometry of the drawable (in root window coordinates). */
    area_t                     geometry;
    /** Surface contents are undefined if this is false. */
    bool                       refreshed;
    /** Callback for refreshing. */
    drawable_refresh_callback *refresh_callback;
    /** Data for refresh callback. */
    void                      *refresh_data;
} drawable_t;

drawable_t *make_drawable(lua_State *L, drawable_refresh_callback *callback, void *data);
void        drawable_set_geometry(lua_State *, int, area_t);
void        luaC_register_drawable(lua_State *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
