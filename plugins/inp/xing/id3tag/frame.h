/*
 * libid3tag - ID3 tag manipulation library
 * Copyright (C) 2000-2001 Robert Leslie
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 */

# ifndef LIBID3TAG_FRAME_H
# define LIBID3TAG_FRAME_H

# include "id3tag.h"

int id3_frame_validid(char const *);

struct id3_frame *id3_frame_new(char const *);
void id3_frame_delete(struct id3_frame *);

void id3_frame_addref(struct id3_frame *);
void id3_frame_delref(struct id3_frame *);

struct id3_frame *id3_frame_parse(id3_byte_t const **, id3_length_t,
				  unsigned int);
id3_length_t id3_frame_render(struct id3_frame const *, id3_byte_t **, int);

# endif
