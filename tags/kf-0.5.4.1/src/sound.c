/* file sound.c */
/*
 * kf linux jabber client
 * ----------------------
 *
 * Copyright (C) 2003-2004 Przemys³aw Sitek <psitek@rams.pl> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include <stdlib.h>
#include <glib.h>
#include "kf.h"
#include "sound.h"
#include "preferences.h"

void kf_sound_play (const gchar *f) {
	if (kf_preferences_get_int ("soundEnabled")) {
		gchar *cmd;
		const gchar *file;

	//	file = kf_find_file (f);
		if (f) {
			if (*f != '/' || *f == '.')
				file = kf_find_file (f);
			else 
				file = f;

//			foo_debug ("file = '%s'\n", file);
			cmd = g_strdup_printf ("%s \"%s\" &",
					kf_preferences_get_string ("soundPlayer"), file);
			system (cmd);
			g_free (cmd);
		}
	}
}


