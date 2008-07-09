/* file popup_group.c */
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
#include "kf.h"
#include "gui.h"
#include "jabber.h"

static void group_menu_cb (GtkMenuItem *menuitem, gpointer data);
static void new_group_menu_cb (GtkMenuItem *menuitem, gpointer data);
static void new_group_response_cb          (GtkDialog *dialog,
                                            gint arg1,
                                            gpointer user_data);
static void set_group (KfJabberRosterItem *item, const gchar *text);
void kf_popup_groups_menu_attach (GladeXML *glade, KfJabberRosterItem *roster_item);

void kf_popup_groups_menu_attach (GladeXML *glade, KfJabberRosterItem *roster_item) {
	GtkWidget *parent;
	GtkWidget *menu;
	GtkWidget *new_group, *separator;
	GList *groups;

	parent = glade_xml_get_widget (glade, "popup_group");
	if ((menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (parent)))) {
		gtk_widget_destroy (menu);
	}
//	} else {
		groups = kf_gui_get_groups ();

		menu = gtk_menu_new ();

		new_group = gtk_menu_item_new_with_label (_("New group..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), new_group);
		g_signal_connect (G_OBJECT (new_group), "activate",
				G_CALLBACK (new_group_menu_cb), roster_item);
		gtk_widget_show (new_group);

		separator = gtk_separator_menu_item_new ();
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator);
		gtk_widget_show (separator);


		while (groups) {
			gchar *grp;
			GtkWidget *item;

			grp = groups->data;
			if (grp == NULL || *grp == '\0') {
				groups = groups->next;
				continue;
			}

			item = gtk_menu_item_new_with_label (grp);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			g_signal_connect (G_OBJECT (item), "activate",
					G_CALLBACK (group_menu_cb), roster_item);
			gtk_widget_show (item);
			groups = groups->next;
		}

		gtk_widget_show (menu);

		gtk_menu_item_set_submenu (GTK_MENU_ITEM (parent), menu);
//	}
}

static void group_menu_cb (GtkMenuItem *menuitem, gpointer data) {
	KfJabberRosterItem *item = data;
	GtkWidget *label;
	const gchar *text;

	label = gtk_bin_get_child (GTK_BIN (menuitem));
	text = gtk_label_get_text (GTK_LABEL (label));
//	g_print ("Dupa: %s\n", text);
	if (text) {
/*		LmConnection *con;
		LmMessage *msg;
		LmMessageNode *node;

		con = kf_jabber_get_connection ();
		msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);

		node = lm_message_node_add_child (msg->node, "query", NULL);
		lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");

		node = lm_message_node_add_child (node, "item", NULL);
		if (item->name)
			lm_message_node_set_attribute (node, "name", item->name);
		lm_message_node_set_attribute (node, "jid", item->jid);

		node = lm_message_node_add_child (node, "group", text);

		kf_connection_send (NULL, msg, NULL);

		g_print ("MSG:\n%s\n", lm_message_node_to_string (msg->node));

		lm_message_unref (msg);*/
		set_group (item, text);
	}
}
	

static void new_group_menu_cb (GtkMenuItem *menuitem, gpointer data) {
	GtkWidget *win;
	GtkWidget *label, *entry;

	win = gtk_dialog_new_with_buttons (_("Enter name..."), NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  NULL);

	label = gtk_label_new (_("New group name:"));
	gtk_widget_show (label);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (win)->vbox), label);

	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), _("Unnamed"));
	gtk_widget_show (entry);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (win)->vbox), entry);
	g_object_set_data (G_OBJECT (win), "entry", (gpointer) entry);

	g_signal_connect (G_OBJECT (win), "response", G_CALLBACK (new_group_response_cb), data);
	gtk_widget_show (win);
}

static void new_group_response_cb          (GtkDialog *dialog,
                                            gint response,
                                            gpointer data) {
//	g_print ("Response -> %d\n", arg1);

	if (response == GTK_RESPONSE_ACCEPT) {
		GtkWidget *entry;
		const gchar *text;

		entry = (GtkWidget *) g_object_get_data (G_OBJECT (dialog), "entry");
		text = gtk_entry_get_text (GTK_ENTRY (entry));
		
		set_group ((KfJabberRosterItem *) data, text);
	} else {
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
}



static void set_group (KfJabberRosterItem *item, const gchar *text) {
		LmConnection *con;
		LmMessage *msg;
		LmMessageNode *node;

		con = kf_jabber_get_connection ();
		msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);

		node = lm_message_node_add_child (msg->node, "query", NULL);
		lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");

		node = lm_message_node_add_child (node, "item", NULL);
		if (item->name)
			lm_message_node_set_attribute (node, "name", item->name);
		lm_message_node_set_attribute (node, "jid", item->jid);

		node = lm_message_node_add_child (node, "group", text);

		kf_connection_send (NULL, msg, NULL);

		g_print ("MSG:\n%s\n", lm_message_node_to_string (msg->node));

		lm_message_unref (msg);
}
