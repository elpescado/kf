/* file kf.c */
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


#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdarg.h>
#include <stdlib.h>
#include "kf.h"
#include "preferences.h"
#include "jabber.h"
#include "gtktip.h"

#ifdef ENABLE_BINRELOC
#	include "prefix.h"
#endif

static void kf_config_dir (void);

const gchar *kf_find_file (const gchar *name) {
	static gchar text[1024];

	foo_debug ("Searching for '%s'\n", name);
	g_snprintf (text, 1024, "../data/%s", name);
	if (g_file_test (text, G_FILE_TEST_EXISTS)) {
		return text;
	}

#ifdef ENABLE_BINRELOC
	g_snprintf (text, 1024, "%s/kf/%s", DATADIR, name);
#else
	g_snprintf (text, 1024, "%s/kf/%s", PACKAGE_DATA_DIR, name);
#endif

#ifdef DEBUG
	if (! g_file_test (text, G_FILE_TEST_EXISTS)) {
		foo_debug ("File not found: '%s'\n", text);
	}
#endif
	
	return text;
}

const gchar *kf_config_file (const gchar *name) {
	static gchar path[1024];

	kf_config_dir ();
	g_snprintf (path, 1024, "%s/.kf/%s", g_get_home_dir (), name);
	return path;
}

gint kf_signal_connect (GladeXML *glade, const gchar *name, const gchar *signal, GCallback callback,
		gpointer data) {
	GtkWidget *widget;

	widget = glade_xml_get_widget (glade, name);
	if (widget) {
		return g_signal_connect (G_OBJECT (widget), signal, callback, data);
	} else {
		return -1;
	}
}

static void kf_config_dir (void) {
	static gchar path[1024];

	g_snprintf (path, 1024, "%s/.kf", g_get_home_dir ());
	mkdir (path, 00753);
}

void   kf_close_button                     (GtkWidget *button,
                                            gpointer user_data)
{
	gtk_widget_hide (gtk_widget_get_toplevel (button));
}

void kf_destroy_toplevel (GtkWidget *button, gpointer data) {
	gtk_widget_destroy (gtk_widget_get_toplevel (button));
}

gint kf_glade_get_widgets (GladeXML *glade, ...) {
	va_list ap;
	gint i = 0;

	va_start (ap, glade);
	while (TRUE) {
		gchar *name;
		GtkWidget **widget;

		name = va_arg (ap, char *);
		if (name) {
			widget = (GtkWidget **) va_arg (ap, GtkWidget *);
			*widget = glade_xml_get_widget (glade, name);
 			if (*widget == NULL) {
 				g_warning ("Could not find widget '%s'\n", name);
 			}

			i++;
		} else {
			break;
		}
	}
	return i;
}

void kf_history (const gchar *jid) {
	/* Since there is no history window, we'll launch a terminal with less
	 * showing log file:-) */
	gchar *jidx;
	gchar *file;

	jidx = g_strdup (jid);
	kf_jabber_jid_crop (jidx);
	
	file = g_strdup_printf ("%s/%s.txt", kf_config_file ("log"), jidx);
	if (!g_file_test (kf_config_file ("log"), G_FILE_TEST_EXISTS)) {
		g_printerr ("There's no file...\n");
	} else {
		gchar *cmd;
		const gchar *viewer;

		if ((viewer = kf_preferences_get_string ("historyViewer"))) {
			cmd = g_strdup_printf ("%s \"%s\" &", viewer, file);
		} else {
			cmd = g_strdup_printf ("%s -e less \"%s\" &", 
				kf_preferences_get_string ("terminal"), file);
		}
		foo_debug ("Trying to execute \"%s\"\n", cmd);
		system (cmd);
		g_free (cmd);
	}

	g_free (jidx);
	g_free (file);
}
/*
void kf_confirm (const gchar *text, GCallback cb, gpointer data) {
	GtkWidget *win;
	GtkWidget *ok;
	GtkWidget *cancel;
		
	win = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
			text);
	
	ok = gtk_dialog_add_button (GTK_DIALOG (win), GTK_STOCK_OK, GTK_RESPONSE_OK);
	cancel = gtk_dialog_add_button (GTK_DIALOG (win), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	
	g_signal_connect_after (G_OBJECT (win), "response", G_CALLBACK (kf_destroy_toplevel), NULL);
	g_signal_connect_swapped (G_OBJECT (ok), "clicked", G_CALLBACK (g_print), "DUPA");
	g_signal_connect (G_OBJECT (ok), "clicked", G_CALLBACK (cb), data);

	gtk_widget_show (win);
}
*/

GtkWidget *kf_tip_new (GdkPixbuf *pix, const gchar *text) {
	static GdkPixbuf *bg = NULL;
	GtkWidget *tip;

	if (bg == NULL) {
		bg = gdk_pixbuf_new_from_file (kf_find_file ("kf.png"), NULL);
	}
	tip = gtk_tip_new (pix, text);

	gtk_tip_set_bg_pixbuf (GTK_TIP (tip), bg);

	return tip;
}
