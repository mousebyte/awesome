/*
 * tag.h - tag class
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_OBJECTS_TAG_H
#define AWESOME_OBJECTS_TAG_H

#include "client.h"

int  tags_get_current_or_first_selected_index(void);
void tag_client(lua_State *, client_t *);
void untag_client(client_t *, tag_t *);
bool is_client_tagged(client_t *, tag_t *);
void tag_unref_simplified(tag_t **);

ARRAY_FUNCS(tag_t *, tag, tag_unref_simplified)

/** Tag type */
struct tag {
    /** Tag name */
    char          *name;
    /** true if activated */
    bool           activated;
    /** true if selected */
    bool           selected;
    /** clients in this tag */
    client_array_t clients;
};

void luaC_register_tag(lua_State *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
