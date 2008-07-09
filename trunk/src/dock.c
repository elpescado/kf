/* file dock.c */
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
#include <X11/Xatom.h>
#include <time.h>
#include "kf.h"
#include "jabber.h"
#include "preferences.h"
#include "gui.h"
#include "eggtrayicon.h"
#include "dock.h"
#include "events.h"
#include "xevent.h"
#include "callbacks.h"
//#include "xevent.h"

/* Dock */
static GtkWidget *dock;
static GladeXML *dock_glade;

EggTrayIcon *egg;
static gint timeout;
static int dock_icon_no = 0;

/* XXX */
static gboolean    dock_button_press       (GtkWidget *widget,
                                            GdkEventButton *event,
					    gpointer user_data);
static void        dock_menu               (GtkMenuItem *menuitem,
                                            gpointer user_data);
static gboolean dock_blink_func (GtkWidget *image);
void kf_jabber_disconnect (void);
static gboolean dock_deleted               (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data);
/* Code */

void kf_dock_init (void) {
	GtkWidget *image;
	GtkWidget *eventbox;
	GtkTooltips *tooltips;

	egg = egg_tray_icon_new ("kf");

	if (egg->manager_window == None) {
		/* Make dock work in older environments... taken from TleenX2 */
		glong data[1];
		GdkAtom kwm_dockwindow_atom = gdk_atom_intern("KWM_DOCKWINDOW", FALSE);
		GdkAtom kde_net_system_tray_window_for_atom =
			gdk_atom_intern("_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", FALSE);
 
		foo_debug ("Dock is using compatibility mode\n");
		
		dock = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(dock), "kf_docklet");
		gtk_window_set_wmclass(GTK_WINDOW(dock),
					"GM_Statusdocklet", "kf-dock");
		gtk_window_set_decorated(GTK_WINDOW(dock), 0);
#ifdef GDK_WINDOW_TYPE_HINT_DOCK
		gtk_window_set_type_hint (GTK_WINDOW(dock), GDK_WINDOW_TYPE_HINT_DOCK);
#endif

		gtk_widget_set_usize(GTK_WIDGET(dock), 22, 22);

		gtk_widget_realize(GTK_WIDGET(dock));

		  /* kde 1 & gnome 1.2 */
		data[0] = TRUE;
		gdk_property_change( dock->window,
                       kwm_dockwindow_atom,
                       kwm_dockwindow_atom,
                       32,
                       GDK_PROP_MODE_REPLACE,
                       (guchar *)&data,
                       1 );

		/* kde 2 */
		data[0] = 0;
		gdk_property_change( dock->window,
                       kde_net_system_tray_window_for_atom,
                       (GdkAtom)XA_WINDOW,
                       32,
                       GDK_PROP_MODE_REPLACE,
                       (guchar *)&data,
                       1 );



	} else {
		dock = GTK_WIDGET (egg);
	}

	eventbox = gtk_event_box_new ();
	gtk_widget_set_size_request (eventbox, 22, 22);
	g_signal_connect (G_OBJECT (eventbox), "button-press-event",
			G_CALLBACK (dock_button_press), NULL);

	image = gtk_image_new_from_file (kf_find_file ("unavailable.png"));

	gtk_container_add (GTK_CONTAINER (dock), eventbox);
	gtk_container_add (GTK_CONTAINER (eventbox), image);


	g_object_set_data (G_OBJECT (dock), "image", image);

	gtk_widget_show (eventbox);
	gtk_widget_show (image);

	dock_glade = glade_xml_new (kf_find_file ("dock.glade"), NULL, NULL);
	kf_signal_connect (dock_glade, "dock_available", "activate", G_CALLBACK (dock_menu), NULL);
	kf_signal_connect (dock_glade, "dock_chatty", "activate", G_CALLBACK (dock_menu), "chat");
	kf_signal_connect (dock_glade, "dock_away", "activate", G_CALLBACK (dock_menu), "away");
	kf_signal_connect (dock_glade, "dock_xa", "activate", G_CALLBACK (dock_menu), "xa");
	kf_signal_connect (dock_glade, "dock_dnd", "activate", G_CALLBACK (dock_menu), "dnd");
	kf_signal_connect (dock_glade, "dock_invisible", "activate", G_CALLBACK (dock_menu), "invisible");
	kf_signal_connect (dock_glade, "dock_disconnect", "activate", G_CALLBACK (kf_jabber_disconnect), NULL);
	kf_signal_connect (dock_glade, "dock_quit", "activate", G_CALLBACK (gtk_main_quit), NULL);

	kf_signal_connect (dock_glade, "dock_new_message",	"activate", G_CALLBACK (on_menu_new_msg_activate), NULL);
	kf_signal_connect (dock_glade, "dock_message_archive",	"activate", G_CALLBACK (on_menu_archive_activate), NULL);
	kf_signal_connect (dock_glade, "dock_join_chatroom",	"activate", G_CALLBACK (on_menu_groupchat_activate), NULL);
	kf_signal_connect (dock_glade, "dock_browse",	"activate", G_CALLBACK (on_menu_browse_activate), NULL);
	kf_signal_connect (dock_glade, "dock_search",	"activate", G_CALLBACK (on_menu_search_activate), NULL);
	kf_signal_connect (dock_glade, "dock_register",	"activate", G_CALLBACK (on_menu_register_activate), NULL);
	kf_signal_connect (dock_glade, "dock_prefs",	"activate", G_CALLBACK (on_menu_prefs_activate), NULL);

	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tooltips, eventbox, "kf", NULL);
	g_object_set_data (G_OBJECT (dock), "tooltips", tooltips);
	//foo_debug ("egg: %d\n", GTK_WIDGET_VISIBLE (GTK_WIDGET (egg)));
	
	timeout = gtk_timeout_add (800, (GtkFunction) dock_blink_func, image);

	if (kf_preferences_get_int ("dockEnable"))
		gtk_widget_show (dock);
	g_signal_connect (G_OBJECT (dock), "delete_event", G_CALLBACK (dock_deleted), NULL);
}

static GdkPixbuf *dock_pix = NULL;
void kf_dock_update_icon (gint type, const gchar *status) {
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	GtkTooltips *tooltips;
	gchar *text;
	gint index;

	image = g_object_get_data (G_OBJECT (dock), "image");

	dock_icon_no = type;

	index = kf_x_event_last (NULL);
//	if (index != -1 && time(NULL) % 2 == 1) {
//		pixbuf = kf_gui_pixmap_get_status (index + 8);
//	} else {
		pixbuf = kf_gui_pixmap_get_status (type);
		dock_pix = pixbuf;
//	}

	gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);

	text = g_strdup_printf ("kf: %s (%s)", kf_jabber_presence_type_to_string (type), status);
	tooltips = g_object_get_data (G_OBJECT (dock), "tooltips");
	gtk_tooltips_set_tip (tooltips, gtk_bin_get_child (GTK_BIN (dock)), text , NULL);
	g_free (text);
}

static gboolean    dock_button_press       (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		/* Right mouse button */
		GtkWidget *menu;

		menu = glade_xml_get_widget (dock_glade, "dock_menu");
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL,
				NULL, event->button, event->time);
	} else if (event->button == 1) {
		/* Left mouse button */

		if (kf_x_event_last (NULL) != -1) {
			kf_x_event_release_all ();
		} else {
			extern GladeXML *iface;
			GtkWidget *window;
			
			window = glade_xml_get_widget (iface, "main_window");
			if (GTK_WIDGET_VISIBLE (window)) {
				/* TODO: check whether roster window is below other windows.
				   	If so, raise it
					Otherwise (it is active window), simply hide it.

					The main problem is that when you click on dock icon,
					it removes focus from roster window, so it is always
					'unactive' so code written below always raises roster
					window.
				*/
//				gboolean is_active = TRUE;
//				
//				if (gtk_major_version >= 2 && gtk_minor_version >= 2)
//					g_object_get (G_OBJECT (window), "is-active", &is_active, NULL);
//
//				if (is_active)
					kf_app_hide ();
//				else
//					gdk_window_raise (window->window);
			} else {
				kf_app_unhide ();
			}
		}
	}
	return TRUE;
}

/* Callback for dock menu for changing presence */
static void        dock_menu               (GtkMenuItem *menuitem,
                                            gpointer user_data) {
	kf_gui_change_status (user_data);
}

/* Callback that blinks tray icon when there are events queued */
static gboolean dock_blink_func (GtkWidget *image) {
	KfEventClass event;
	static gboolean x = FALSE;

	if ((event = kf_x_event_last (NULL)) != -1) {
		static gboolean odd = TRUE;
		GdkPixbuf *pix;

		if (odd) {
			pix = kf_gui_pixmap_get_status (event + 8);
		} else {
			pix = dock_pix;
		}

		if (pix)
			gtk_image_set_from_pixbuf (GTK_IMAGE (image), pix);

		x = TRUE;
		odd = ! odd;
	} else {
		/* Return status pix */
		
		if (dock_pix)
			gtk_image_set_from_pixbuf (GTK_IMAGE (image), dock_pix);
		x = FALSE;
	}
	return TRUE;
}

static gboolean dock_deleted               (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data) {
			extern GladeXML *iface;
			GtkWidget *window;
			
			window = glade_xml_get_widget (iface, "main_window");
//			if (GTK_WIDGET_VISIBLE (window)) {
//				gtk_widget_hide (window);
//			} else {
				gtk_widget_show (window);
//			}

	return TRUE;
}

void kf_dock_show (void) {
	gtk_widget_show (dock);
}

void kf_dock_hide (void) {
	gtk_widget_hide (dock);
}

void kf_dock_icons_changed (void) {
	GtkWidget *image;
	GdkPixbuf *pixbuf;

	image = g_object_get_data (G_OBJECT (dock), "image");
	pixbuf = kf_gui_pixmap_get_status (dock_icon_no);
	dock_pix = pixbuf;
	gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
}

GladeXML *kf_dock_glade (void) {
	return dock_glade;
}
