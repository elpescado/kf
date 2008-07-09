/* file statusbar.c */
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
#include "kf.h"
#include "gui.h"
#include "statusbar.h"

static GtkWidget *statusbar = NULL;

void kf_statusbar_set_text (const gchar *text) {
	if (statusbar == NULL) {
		extern GladeXML *iface;

		statusbar = glade_xml_get_widget (iface, "statusbar");
	}

	if (text) {
		gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
		gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, text);
		gtk_widget_show (statusbar);
	} else {
		gtk_widget_hide (statusbar);
	}
}
