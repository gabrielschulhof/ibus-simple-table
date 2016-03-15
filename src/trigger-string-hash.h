/*
 * Simple IBus Table
 * Copyright (C) 2016  Gabriel Schulhof <gabrielschulhof@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _TRIGGER_STRING_HASH_H_
#define _TRIGGER_STRING_HASH_H_

#include <glib.h>

G_BEGIN_DECLS

GHashTable *trigger_string_hash_from_file(const char *fname);

void trigger_string_hash_dump(GHashTable *trigger_string_hash);

void trigger_string_hash_destroy(GHashTable *trigger_string_hash);

G_END_DECLS

#endif /* !_TRIGGER_STRING_HASH_H_ */
