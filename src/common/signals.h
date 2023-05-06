/*
 * signals.h - signal and related classes
 *
 * Copyright © 2023 Abigail Teague <ateague063@gmail.com>
 *
 * Based on:
 * common/signal.h - Signal handling functions
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
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

#ifndef LUNA_COMMON_SIGNAL_H
#define LUNA_COMMON_SIGNAL_H

#include <luaclasslib.h>
#include <stdlib.h>
#include "array.h"

void luaC_register_signal_store(lua_State *);

#endif
