/* file popup_menu.c */
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
#include "jabber.h"
#include "new_message.h"
#include "chat.h"
#include "vcard.h"
#include "popup_menu.h"
#include "register.h"
#include "search.h"
#include "archive_viewer.h"
#include "contact_xchange.h"

void kf_gui_add_contact (const gchar *fulljid);
//static void on_popup_delete_activate2      (GtkButton *button,
//                                            gpointer user_data);

void        on_popup_add_contact_activate  (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_gui_add_contact (kf_gui_get_selected_jid ());
}


void        on_popup_message_activate      (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_new_message (kf_gui_get_selected_jid ());
}

void        on_popup_chat_activate         (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_chat_window (kf_gui_get_selected_jid ());
}

void on_popup_delete_activate              (GtkMenuItem *item,
                                            gpointer user_data)
{
//	kf_confirm (_("Dou you want to remove this contact?"), on_popup_delete_activate2, kf_gui_get_selected_jid ());
//	kf_jabber_remove_contact (kf_gui_get_selected_jid ());
	GtkWidget *win;
	GtkWidget *label, *checkbox;
	extern GladeXML *iface;
	guint response;
	gchar *jid = g_strdup (kf_gui_get_selected_jid ());

	win = gtk_dialog_new_with_buttons (_("Question"),
			GTK_WINDOW (glade_xml_get_widget (iface, "main_window")),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_YES,		GTK_RESPONSE_ACCEPT,
                        GTK_STOCK_NO,		GTK_RESPONSE_REJECT,
			NULL);
	label = gtk_label_new (_("Dou you want to remove this contact?"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (win)->vbox), label, TRUE, TRUE, 4);

	checkbox = gtk_check_button_new_with_mnemonic (_("_Deny subscription to this contact"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), TRUE);
	gtk_widget_show (checkbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (win)->vbox), checkbox, FALSE, FALSE, 4);

	gtk_widget_show (win);

	response = gtk_dialog_run (GTK_DIALOG (win));
	if (response == GTK_RESPONSE_ACCEPT) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox))) {
			kf_jabber_send_presence_to_jid ("unsubscribed", jid);
		}
		kf_jabber_remove_contact (jid);
	}
	g_free (jid);
	gtk_widget_destroy (win);			
}
/*
static void on_popup_delete_activate2      (GtkButton *button,
                                            gpointer user_data)
{
	foo_debug ("on_popup_delete_activate2\n");
	kf_jabber_remove_contact (kf_gui_get_selected_jid ());
}
*/
void        on_popup_info_activate         (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_vcard_get (kf_gui_get_selected_jid ());
}


void        on_popup_send_contacts_activate(GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_contacts_send (kf_gui_get_selected_jid ());
}
/* Presence */
void        on_popup_presence_activate    (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
//	g_print ("------------------------------> %s\n", user_data);
//	if (user_data)
		kf_jabber_send_presence_to_jid (user_data, kf_gui_get_selected_jid ());
}

void        on_popup_log_in_activate       (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_jabber_send_presence_to_jid (NULL, kf_gui_get_selected_jid ());
}
void        on_popup_log_out_activate      (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_jabber_send_presence_to_jid ("unavailable", kf_gui_get_selected_jid ());
}

/* Activates history window */
void        on_popup_history_activate      (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	/* Since there is no history window, we'll launch a terminal with less
	 * showing log file:-) */
/*	gchar *jidx;
	gchar *file;

	jidx = g_strdup (kf_gui_get_selected_jid ());
	kf_jabber_jid_crop (jidx);

	file = g_strdup_printf ("%s/%s.txt", kf_config_file ("log"), jidx);
	if (!g_file_test (kf_config_file ("log"), G_FILE_TEST_EXISTS)) {
		g_printerr ("There's no file...\n");
	} else {
		gchar *cmd;

		cmd = g_strdup_printf ("rxvt -e less \"%s\" &", file);
		system (cmd);
		g_free (cmd);
	}

	g_free (jidx);
	g_free (file);*/
	gchar *jidx;
	jidx = g_strdup (kf_gui_get_selected_jid ());
	kf_jabber_jid_crop (jidx);
	kf_archive_viewer_jid (jidx);
	g_free (jidx);
//	kf_history (kf_gui_get_selected_jid ());
}


void        on_popup_register_activate  (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_register_window_x (kf_gui_get_selected_jid ());
}

void        on_popup_search_activate  (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_search_window_x (kf_gui_get_selected_jid ());
}

