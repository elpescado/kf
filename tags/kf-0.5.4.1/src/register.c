/* file register.c */
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
#include <stdlib.h>
#include <string.h>
#include "kf.h"
#include "gui.h"
#include "register.h"
#include "x_data.h"
#include "register.h"

static LmHandlerResult register_hendel      (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);
static LmHandlerResult register_hendel2     (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);

static void kf_register_get_form (const gchar *userdir, GtkWidget *window);
extern LmConnection *kf_jabber_connection;

void        on_register_ok_clicked           (GtkButton *button,
                                            gpointer user_data);
void        on_register_ok2_clicked        (GtkButton *button,
                                            gpointer user_data);
void on_register_cancel_clicked            (GtkButton *button,
                                            gpointer user_data);


void kf_register_window (void) {
	GladeXML *reg;
	GtkWidget *window;

	reg = glade_xml_new (kf_find_file ("register.glade"), NULL, NULL);
	glade_xml_signal_autoconnect (reg);

	window = glade_xml_get_widget (reg, "register_window");
	g_object_set_data_full (G_OBJECT (window), "glade", reg, g_object_unref);
}

void kf_register_window_x (const gchar *jid) {
	GladeXML *reg;
	GtkWidget *entry, *button;

	reg = glade_xml_new (kf_find_file ("register.glade"), NULL, NULL);
	glade_xml_signal_autoconnect (reg);

	entry = glade_xml_get_widget (reg, "register_jid");
	gtk_entry_set_text (GTK_ENTRY (entry), jid);

	button = glade_xml_get_widget (reg, "register_ok");
	gtk_button_clicked (GTK_BUTTON (button));
}


void        on_register_ok_clicked           (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *searchbox;
	GtkWidget *entry;
	const gchar *jud;

	searchbox = glade_get_widget_tree (GTK_WIDGET (button));
	entry = glade_xml_get_widget (searchbox, "register_jid");
	jud = gtk_entry_get_text (GTK_ENTRY (entry));

	kf_register_get_form (jud, gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

static void kf_register_get_form (const gchar *userdir, GtkWidget *window) {
	LmMessageHandler *hendel;
	LmMessage *msg;
	LmMessageNode *node;

	hendel = lm_message_handler_new (register_hendel, (gpointer *) window, NULL);
	msg = lm_message_new_with_sub_type (userdir, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:register");

	lm_connection_send_with_reply (kf_jabber_connection, msg, hendel, NULL);
	g_object_set_data_full (G_OBJECT (window), "jud", g_strdup (userdir), g_free);
	lm_message_unref (msg);
}

static LmHandlerResult register_hendel      (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *node;
	GtkWidget *window;
	GtkWidget *table = NULL;
	GtkWidget *notebook;
	GladeXML *searchbox;

	node = lm_message_node_find_child (message->node, "error");
	if (node) {
		/* Nast±pi³ b³±d podczas rejestracji... */
		GtkWidget *msg;
		gint errcode;
		const gchar *errstr;

		errcode = atoi (lm_message_node_get_attribute (node, "code"));
		errstr = lm_message_node_get_value (node);

		msg = gtk_message_dialog_new (GTK_WINDOW (user_data),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("An error occured while trying to register:\n#%d: %s"),
				errcode, errstr);
		gtk_dialog_run (GTK_DIALOG (msg));
		gtk_widget_destroy (msg);
		/* Zamknijmy te¿ okno nadrzêdne... */
		gtk_widget_destroy (GTK_WIDGET (user_data));
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	} 
	
	node = lm_message_node_find_child (message->node, "x");
	if (!node && 0) {
		/* Nast±pi³ b³±d podczas rejestracji... */
		GtkWidget *msg;

		msg = gtk_message_dialog_new (GTK_WINDOW (user_data),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("Agent didn't supply registration form..."));
		gtk_dialog_run (GTK_DIALOG (msg));
		gtk_widget_destroy (msg);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	} 

	searchbox = glade_get_widget_tree (GTK_WIDGET (user_data));
	if (lm_message_node_find_child (message->node, "registered")) {
		GtkWidget *note;

		note = glade_xml_get_widget (searchbox, "registered_note");
		gtk_widget_show (note);
	}
	
	if (message->node->children && message->node->children->children) {
		for (node = message->node->children->children; node; node = node->next) {
			if (strcmp (node->name, "x") == 0) {
				const gchar *xmlns;

				xmlns = lm_message_node_get_attribute (node, "xmlns");
				if (xmlns && strcmp (xmlns, "jabber:x:data") == 0) {
					table = kf_x_data_form (node->children);
				}
			}
		}

		if (table == NULL ) {
			GtkWidget *win;
			table = kf_basic_form_create (message->node->children->children);

			win = gtk_widget_get_toplevel (GTK_WIDGET (user_data));
			g_object_set_data (G_OBJECT (win), "BasicForm", win);
		}
	}

	if (table == NULL) {
		/* Nast±pi³ b³±d podczas rejestracji... */
		GtkWidget *msg;

		msg = gtk_message_dialog_new (GTK_WINDOW (user_data),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("Agent didn't supply registration form..."));
		gtk_dialog_run (GTK_DIALOG (msg));
		gtk_widget_destroy (msg);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

	
//	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	window = glade_xml_get_widget (searchbox, "form_frame");
	notebook = glade_xml_get_widget (searchbox, "notebook1");
	gtk_container_add (GTK_CONTAINER (window), table);
	gtk_window_resize (GTK_WINDOW (user_data), 370, 400);
	gtk_widget_show (table);
	gtk_widget_show (window);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 1);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

void        on_register_ok2_clicked        (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *searchbox;
	GtkWidget *frame, *table, *window;
	GSList *list;
	LmMessage *msg;
	LmMessageNode *node;
	const gchar *jud;
	LmMessageHandler *hendel;
	
	window = gtk_widget_get_toplevel (GTK_WIDGET (button));
	jud = g_object_get_data (G_OBJECT (window), "jud");

	searchbox = glade_get_widget_tree (GTK_WIDGET (button));
	frame = glade_xml_get_widget (searchbox, "form_frame");
	table = GTK_BIN (frame)->child;
	list = g_object_get_data (G_OBJECT (table), "Fields");
	if (!list) { g_warning ("Ni ma pol"); return; }
	

	msg = lm_message_new_with_sub_type (jud ,LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:register");
	
	if (g_object_get_data (G_OBJECT (window), "BasicForm") == NULL) {
		kf_x_data_create_node (node, list);
	} else {
		foo_debug ("Using basic data...");
		kf_basic_create_node (node, list);
	}

	hendel = lm_message_handler_new (register_hendel2, (gpointer *) window, NULL);
	lm_connection_send_with_reply (kf_jabber_connection, msg, hendel, NULL);
	lm_message_unref (msg);
}


static LmHandlerResult register_hendel2     (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *node;
	GladeXML *glade;

	glade = glade_get_widget_tree (GTK_WIDGET (user_data));
	node = lm_message_node_find_child (message->node, "error");
	if (node) {
		/* Nast±pi³ b³±d podczas rejestracji... */
		GtkWidget *msg;
		gint errcode;
		const gchar *errstr;

		errcode = atoi (lm_message_node_get_attribute (node, "code"));
		errstr = lm_message_node_get_value (node);

		msg = gtk_message_dialog_new (GTK_WINDOW (user_data),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("An error occured while trying to register:\n#%d: %s"),
				errcode, errstr);
		gtk_dialog_run (GTK_DIALOG (msg));
		gtk_widget_destroy (msg);
	} else {
		GtkWidget *msg;

		msg = gtk_message_dialog_new (GTK_WINDOW (user_data),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				_("Registration successful"));


		gtk_dialog_run (GTK_DIALOG (msg));
		gtk_widget_destroy (msg);
		/* Zamknijmy te¿ okno nadrzêdne... */
		gtk_widget_destroy (GTK_WIDGET (user_data));
	}


	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

void on_register_cancel_clicked            (GtkButton *button,
                                            gpointer user_data)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

