/* file filter.c */
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


#include <glib.h>
#include <string.h>
#include "kf.h"
#include "filter.h"
#include "jabber.h"

gboolean kf_filter_is_blocked (const gchar *jid) {
	extern GSList *kf_preferences_blocked_jids;
	GSList *tmp;
	gchar *jidx;

	jidx = g_strdup (jid);
	kf_jabber_jid_crop (jidx);

	tmp = kf_preferences_blocked_jids;
	while (tmp) {
		if (strcmp (jidx, tmp->data) == 0) {
			/* Jid is on our blacklist!!! */
			return TRUE;
		}
		
		tmp = tmp->next;
	}
	g_free (jidx);

	return FALSE;
}

void kf_filter_block_user (const gchar *jid) {
	extern GSList *kf_preferences_blocked_jids;
//	GSList *tmp;
	gchar *jidx;

	jidx = g_strdup (jid);
	kf_jabber_jid_crop (jidx);

	kf_preferences_blocked_jids = g_slist_append (kf_preferences_blocked_jids, jidx);
}
