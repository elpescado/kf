/* file change_password.c */
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
#include <loudmouth/loudmouth.h>
#include <stdio.h>
#include <string.h>
#include "kf.h"
#include "jabber.h"
#include "gui.h"
#include "change_password.h"

static void change_password_ok             (GtkButton *button,
                                            gpointer user_data);
static void change_password_real (const gchar *server, const gchar *user, const gchar *new);
static LmHandlerResult cp_hendel            (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);

static void change_password_ok             (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *glade;
	GtkWidget *old, *new, *retype;
	const gchar *told, *tnew, *tretype;
	extern KfJabberConnection kf_jabber_connection_settings;

	glade = glade_get_widget_tree (GTK_WIDGET (button));

	old = glade_xml_get_widget (glade, "passwd_old");
	new = glade_xml_get_widget (glade, "passwd_new");
	retype = glade_xml_get_widget (glade, "passwd_retype");

	told = gtk_entry_get_text (GTK_ENTRY (old));
	tnew = gtk_entry_get_text (GTK_ENTRY (new));
	tretype = gtk_entry_get_text (GTK_ENTRY (retype));
	
	if (strcmp (told, kf_jabber_connection_settings.password) == 0) {
		if (strcmp (tnew, tretype) == 0) {
			change_password_real (kf_jabber_connection_settings.server,
					kf_jabber_connection_settings.username, tnew);
			gtk_entry_set_text (GTK_ENTRY (old), "");
			gtk_entry_set_text (GTK_ENTRY (new), "");
			gtk_entry_set_text (GTK_ENTRY (retype), "");
	
			gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
		} else {
			kf_gui_alert (_("Both passwords must be the same!"));
		}
	} else {
		kf_gui_alert (_("You have entered bad password!"));
	}
}


static void change_password_real (const gchar *server, const gchar *user, const gchar *new) {
	LmMessageHandler *hendel;
	LmMessage *msg;
	LmMessageNode *node;
	extern LmConnection *kf_jabber_connection;

		hendel = lm_message_handler_new (cp_hendel, NULL, NULL);
	msg = lm_message_new_with_sub_type (server, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:register");

	/*subnode =*/ lm_message_node_add_child (node, "username", user);

	/*subnode =*/ lm_message_node_add_child (node, "password", new);

	lm_connection_send_with_reply (kf_jabber_connection, msg, hendel, NULL);
	lm_message_unref (msg);
}

static LmHandlerResult cp_hendel            (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	g_print (lm_message_node_to_string (message->node));
	if (lm_message_node_find_child (message->node, "error")) {
		kf_gui_alert (_("An error occured while trying to change password."));
	} else {
		kf_gui_alert (_("Your password has been succesfully changed."));
	}
	lm_message_handler_unref (handler);	
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}


void kf_change_password (void) {
	extern GladeXML *iface;
	static gboolean first = TRUE;
	GtkWidget *window;

	window = glade_xml_get_widget (iface, "change_password");
	g_return_if_fail (window);

	if (first) {
		kf_signal_connect (iface, "cp_ok", "clicked", G_CALLBACK (change_password_ok), NULL);
		kf_signal_connect (iface, "cp_cancel", "clicked", G_CALLBACK (kf_close_button), NULL);
	}

	gtk_widget_show (window);
	gtk_window_present (GTK_WINDOW (window));
}
