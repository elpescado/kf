/* file gui_pixmap.c */
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


#include <gtk/gtk.h>
#include <glade/glade.h>
#include "preferences.h"
#include "gui.h"
#include "jabber.h"
#include "jisp.h"

KfGuiPixmap kf_gui_pixmaps_status[] = {
	{"online.png", NULL},		/* Status icons  */
	{"chat.png", NULL},
	{"away.png", NULL},
	{"xa.png", NULL},
	{"dnd.png", NULL},
	{"invisible.png", NULL},
	{"unavailable.png", NULL},
	{"unknown.png", NULL},
	{"pix_message.png", NULL},	/* Event icons */
	{"pix_chat.png", NULL},
	{"online.png", NULL},
	{"pix_system.png", NULL}, 	/* To be changed */
	{NULL, NULL}
};

static KfJisp *selected_jisp = NULL;

gboolean kf_gui_pixmap_init (void) {
	KfGuiPixmap *pixmap;
	const gchar *icondef;

	pixmap = kf_gui_pixmaps_status;
	if ((icondef = kf_preferences_get_string ("rosterJisp"))) {
		selected_jisp = kf_jisp_new_from_file (icondef);
	}

	while (pixmap && pixmap->filename) {
//		filename = g_strdup_printf ("../ui/%s", pixmap->filename);
//		filename = kf_gui_find_file (pixmap->filename);
		pixmap->pixbuf = gdk_pixbuf_new_from_file (kf_find_file (pixmap->filename), NULL);
//		g_free (filename);
		pixmap++;	
	}
	return TRUE;
}

GdkPixbuf *kf_gui_pixmap_get_status (guint status) {
	if (selected_jisp && status < JISP_N_ICONS && selected_jisp->icons[status])
		return selected_jisp->icons[status];
	else
		return (kf_gui_pixmaps_status[status]).pixbuf;
};


void kf_gui_pixmap_set_jisp (KfJisp *jisp) {
	if (selected_jisp)
		kf_jisp_unref (selected_jisp);
	selected_jisp = jisp?kf_jisp_ref (jisp):NULL;
}
//GdkPixbuf *kf_gui_pixmap_get_status (guint status) {
//	return (kf_gui_pixmaps_status[status]).pixbuf;
//};
