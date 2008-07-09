/* file subscribe.c */
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
//#include <loudmouth/loudmouth.h>
#include "jabber.h"
#include "gui.h"
#include "preferences.h"
#include "events.h"
#include "xevent.h"
#include "subscribe.h"
#include "callbacks.h"

void        on_subscribe_yes_clicked       (GtkButton *button,
                                            gpointer user_data);
void        on_subscribe_no_clicked        (GtkButton *button,
                                            gpointer user_data);
gboolean    on_subscribe_win_delete_event  (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data);
static void delete_window (GtkWidget *win);

void kf_subscription_process (const gchar *jid) {
	GladeXML *glade;
	GtkWidget *win;
	GtkLabel *label;
	gchar *label_text;

	glade = glade_xml_new (kf_find_file ("subscribe.glade"), NULL, NULL);
	if (!glade) {
		g_error ("Nie mo¿na za³adowaæ interfejsu!\n");
	}
	glade_xml_signal_autoconnect (glade);	
	
	win = glade_xml_get_widget (glade, "subscribe_win");
	g_object_set_data (G_OBJECT (win), "glade", glade);
	g_object_set_data_full (G_OBJECT (win), "jid", g_strdup (jid), g_free);

	label = GTK_LABEL (glade_xml_get_widget (glade, "label1"));
	label_text = g_strdup_printf (gtk_label_get_text (label), jid);
	gtk_label_set_markup (label, label_text);
	g_free (label_text);

	if (kf_preferences_get_int ("autoPopup")) {
		kf_gui_show (win);
	} else {
		gchar *xjid;

		xjid = kf_jabber_jid_crop (g_strdup (jid));
		kf_x_event_push (xjid, KF_EVENT_CLASS_SUBSCRIPTION, win);
		g_free (xjid);
	}

}

void        on_subscribe_yes_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *glade;
	GtkWidget *win;
	GtkWidget *checkbox;
	gchar *jid;

	glade = glade_get_widget_tree (GTK_WIDGET (button));
	win = gtk_widget_get_toplevel (GTK_WIDGET (button));
	jid = g_object_get_data (G_OBJECT (win), "jid");
	if (jid) {
		checkbox = glade_xml_get_widget (glade, "add_to_contacts");
		
		kf_jabber_send_presence_to_jid ("subscribed", jid);

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox))) {
//			GtkWidget *window, *jid_entry;
//			extern GladeXML *iface;
//
//			window = glade_xml_get_widget (iface, "add_contact_window");
//			jid_entry = glade_xml_get_widget (iface, "add_jid");
//			gtk_entry_set_text (GTK_ENTRY (jid_entry), jid);
//			kf_gui_show (window);
			kf_gui_add_contact (jid);
		}
	}

	delete_window (win);
	gtk_widget_destroy (win);
}


void        on_subscribe_no_clicked        (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *glade;
	GtkWidget *win;
	gchar *jid;

	glade = glade_get_widget_tree (GTK_WIDGET (button));
	win = gtk_widget_get_toplevel (GTK_WIDGET (button));
	jid = g_object_get_data (G_OBJECT (win), "jid");
	if (jid) {
		kf_jabber_send_presence_to_jid ("unsubscribed", jid);
	}

	delete_window (win);
	gtk_widget_destroy (win);
}


gboolean    on_subscribe_win_delete_event  (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data)
{
	delete_window (widget);
	return FALSE;
}


static void delete_window (GtkWidget *win) {
	GladeXML *glade;

	glade = g_object_get_data (G_OBJECT (win), "glade");
	if (glade)
		g_object_unref (G_OBJECT (glade));
}
