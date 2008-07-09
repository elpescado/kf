/* file contact_xchange.c */
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
#include <string.h>
#include "kf.h"
#include "contact_xchange.h"



typedef struct {
	GtkWidget *window;
	GtkWidget *to;
	GtkWidget *contacts;
	GtkListStore *store;
	GtkWidget *add;
	GtkWidget *del;
	GtkWidget *send;
	GtkWidget *cancel;
} KfContactsSendDialog;

typedef struct {
	GtkWidget *window;
	GtkWidget *from;
	GtkWidget *contacts;
	GtkListStore *store;
	GtkWidget *add;
	GtkWidget *add_selected;
	GtkWidget *close;
} KfContactsRecvDialog;

KfContactsSendDialog *kf_contacts_send_dialog_new (const gchar *to);
void kf_contacts_send_dialog_free (KfContactsSendDialog *self);
static void kf_contacts_send_dialog_add (KfContactsSendDialog *self, const gchar *jid,
					const gchar *name, const gchar *group);
static void kf_contacts_send_dialog_send (KfContactsSendDialog *self, const gchar *to);

static void on_add_clicked (GtkButton *button, gpointer data);
static void on_del_clicked (GtkButton *button, gpointer data);
static void on_send_clicked (GtkButton *button, gpointer data);
static void on_cancel_clicked (GtkButton *button, gpointer data);
LmHandlerResult kf_contacts_recv_msg_handle (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);



/* Receive dialog */
KfContactsRecvDialog *kf_contacts_recv_dialog_new (const gchar *from);
void kf_contacts_recv_dialog_free (KfContactsRecvDialog *self);
static void kf_contacts_recv_dialog_add (KfContactsRecvDialog *self, const gchar *jid,
					const gchar *name, const gchar *group);

static void on_recv_add_clicked (GtkButton *button, gpointer data);
static void on_add_selected_clicked (GtkButton *button, gpointer data);
static void on_close_clicked (GtkButton *button, gpointer data);
static void
fixed_toggled (GtkCellRendererToggle *cell,
	       gchar                 *path_str,
	       gpointer               data);



void kf_contacts_send (const gchar *to) {
	KfContactsSendDialog *win = kf_contacts_send_dialog_new (to);
	gtk_widget_show (win->window);
}

KfContactsSendDialog *kf_contacts_send_dialog_new (const gchar *to) {
	KfContactsSendDialog *self = g_new0 (KfContactsSendDialog, 1);
	GladeXML *glade = glade_xml_new (kf_find_file ("contacts.glade"), "send_dlg", NULL);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	kf_glade_get_widgets (glade,
			"send_dlg", &self->window,
			"to", &self->to,
			"send_contacts", &self->contacts,
			"add", &self->add,
			"remove", &self->del,
			"send", &self->send,
			"cancel", &self->cancel,
			NULL);
	g_object_unref (G_OBJECT (glade));
	
	gtk_entry_set_text (GTK_ENTRY (self->to), ISEMPTY (to));

	self->store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (self->contacts), GTK_TREE_MODEL (self->store));
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("JabberID", renderer,
			"text", 0,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->contacts), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
			"text", 1,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->contacts), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Group", renderer,
			"text", 2,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->contacts), column);

	g_signal_connect (G_OBJECT (self->add), "clicked",
			G_CALLBACK (on_add_clicked), self);
	g_signal_connect (G_OBJECT (self->del), "clicked",
			G_CALLBACK (on_del_clicked), self);
	g_signal_connect (G_OBJECT (self->send), "clicked",
			G_CALLBACK (on_send_clicked), self);
	g_signal_connect (G_OBJECT (self->cancel), "clicked",
			G_CALLBACK (on_cancel_clicked), self);
	g_object_set_data_full (G_OBJECT (self->window), "self", self, 
			(GDestroyNotify) kf_contacts_send_dialog_free);
	return self;
}


void kf_contacts_send_dialog_free (KfContactsSendDialog *self) {
	g_free (self);
}


static void kf_contacts_send_dialog_add (KfContactsSendDialog *self, const gchar *jid,
					const gchar *name, const gchar *group)
{
	GtkTreeIter iter;
	gtk_list_store_append (self->store, &iter);
	gtk_list_store_set (self->store, &iter, 0, jid, 1, name, 2, group, -1);
}


static void kf_contacts_send_dialog_send (KfContactsSendDialog *self, const gchar *to) {
	LmMessage *msg = lm_message_new (to, LM_MESSAGE_TYPE_MESSAGE);
	GString *str = g_string_sized_new (4096);
	GtkTreeIter iter;
	LmMessageNode *x;
	gint i = 0;
	
	lm_message_node_add_child (msg->node, "subject", _("Contacts"));
	x = lm_message_node_add_child (msg->node, "x", NULL);
	lm_message_node_set_attribute (x, "xmlns", "jabber:x:roster");

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->store), &iter)) {
		do {
			gchar *jid;	/* JabberID */
			gchar *name;	/* */
			gchar *group;	/* */
			LmMessageNode *item;
			
			gtk_tree_model_get (GTK_TREE_MODEL (self->store), &iter, 
					0, &jid, 1, &name, 2, &group, -1);
			item = lm_message_node_add_child (x, "item", NULL);
			lm_message_node_set_attribute (item, "jid", jid);
			lm_message_node_set_attribute (item, "name", name);
			lm_message_node_add_child (item, "group", group);

			g_string_append_printf (str, "%s <%s>\n", name, jid);

			g_free (jid);
			g_free (name);
			g_free (group);
			i++;
			
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->store), &iter));
	}
	lm_message_node_add_child (msg->node, "body", str->str);
	if (i > 0)
		lm_connection_send (kf_jabber_get_connection (), msg, NULL);
	lm_message_unref (msg);
	g_string_free (str, TRUE);
}

/*
 * Callbacks for send dialog
 */

/*
 * Callback for clicking on add button
 */
static void on_add_clicked (GtkButton *button, gpointer data)
{
	KfContactsSendDialog *self = data;
	kf_contacts_send_dialog_add (self, "", "", "");
}


/*
 * Callback for clicking on del button
 */
static void on_del_clicked (GtkButton *button, gpointer data)
{
	KfContactsSendDialog *self = data;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->contacts));

	if (selection) {
		GtkTreeIter iter;
		GtkTreeModel *model;
		
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		}
	}
}


/*
 * Callback for clicking on send button
 */
static void on_send_clicked (GtkButton *button, gpointer data)
{
	KfContactsSendDialog *self = data;
	const gchar *to = gtk_entry_get_text (GTK_ENTRY (self->to));
	/* TODO: check if JID is correct? */
	kf_contacts_send_dialog_send (self, to);
}


/*
 * Callback for clicking on cancel button
 */
static void on_cancel_clicked (GtkButton *button, gpointer data)
{
	KfContactsSendDialog *self = data;
	gtk_widget_destroy (self->window);
}


/* Receiving contacts... */


KfContactsRecvDialog *kf_contacts_recv_dialog_new (const gchar *from) {
	KfContactsRecvDialog *self = g_new0 (KfContactsRecvDialog, 1);
	GladeXML *glade = glade_xml_new (kf_find_file ("contacts.glade"), "receive_dlg", NULL);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	kf_glade_get_widgets (glade,
			"receive_dlg", &self->window,
			"from", &self->from,
			"recv_contacts", &self->contacts,
			"add_one", &self->add,
			"add_selected", &self->add_selected,
			"close", &self->close,
			NULL);
	g_object_unref (G_OBJECT (glade));

	gtk_entry_set_text (GTK_ENTRY (self->from), ISEMPTY (from));

	self->store = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (self->contacts), GTK_TREE_MODEL (self->store));
	
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled",
		    G_CALLBACK (fixed_toggled), self->store);
	column = gtk_tree_view_column_new_with_attributes ("Add", renderer,
			"active", 3,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->contacts), column);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("JabberID", renderer,
			"text", 0,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->contacts), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
			"text", 1,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->contacts), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Group", renderer,
			"text", 2,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->contacts), column);

	g_signal_connect (G_OBJECT (self->add), "clicked",
			G_CALLBACK (on_recv_add_clicked), self);
	g_signal_connect (G_OBJECT (self->add_selected), "clicked",
			G_CALLBACK (on_add_selected_clicked), self);
	g_signal_connect (G_OBJECT (self->close), "clicked",
			G_CALLBACK (on_close_clicked), self);
	g_object_set_data_full (G_OBJECT (self->window), "self", self,
			(GDestroyNotify) kf_contacts_recv_dialog_free);
	return self;
}


void kf_contacts_recv_dialog_free (KfContactsRecvDialog *self) {
	g_free (self);
}


static void
fixed_toggled (GtkCellRendererToggle *cell,
	       gchar                 *path_str,
	       gpointer               data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreeIter  iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, 3, &fixed, -1);

  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 3, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);
}

/*
 * Add contact to list in Receive dialogs
 */
static void kf_contacts_recv_dialog_add (KfContactsRecvDialog *self, const gchar *jid,
					const gchar *name, const gchar *group)
{
	GtkTreeIter iter;
	gtk_list_store_append (self->store, &iter);
	gtk_list_store_set (self->store, &iter, 0, jid, 1, name, 2, group, 3, TRUE, -1);
}

/*
 * Callback for clicking on recv_add button
 */
static void on_recv_add_clicked (GtkButton *button, gpointer data)
{
	KfContactsRecvDialog *self = data;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->contacts));
	void kf_gui_add_contact_full (const gchar *fulljid,
		const gchar *name, const gchar *group);

	if (selection) {
		GtkTreeIter iter;
		GtkTreeModel *model;
		
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			gchar *jid;
			gchar *name;
			gchar *group;

			gtk_tree_model_get (GTK_TREE_MODEL (self->store), &iter, 
					0, &jid, 1, &name, 2, &group, -1);
			
			kf_gui_add_contact_full (jid, name, group);
				
			g_free (jid);
			g_free (name);
			g_free (group);
		}
	}

}


/*
 * Callback for clicking on add_selected button
 */
static void on_add_selected_clicked (GtkButton *button, gpointer data)
{
	KfContactsRecvDialog *self = data;
	LmMessage *msg = lm_message_new_with_sub_type (NULL,
			LM_MESSAGE_TYPE_MESSAGE, LM_MESSAGE_SUB_TYPE_SET);
	LmMessageNode *query = lm_message_node_add_child (msg->node, "query", NULL);
	GtkTreeIter iter;
	gint i = 0;
	
	lm_message_node_set_attribute (query, "xmlns", "jabber:iq:roster");

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->store), &iter)) {
		do {
			gchar *jid;	/* JabberID */
			gchar *name;	/* */
			gchar *group;	/* */
			gboolean add;
			LmMessageNode *item;
			
			gtk_tree_model_get (GTK_TREE_MODEL (self->store), &iter, 
					0, &jid, 1, &name, 2, &group, 3, &add, -1);

			if (add) {
				item = lm_message_node_add_child (query, "item", NULL);
				lm_message_node_set_attribute (item, "jid", jid);
				lm_message_node_set_attribute (item, "name", name);
				lm_message_node_add_child (item, "group", group);
				i++;
			}

			g_free (jid);
			g_free (name);
			g_free (group);
			
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->store), &iter));
	}
	if (i > 0)
		lm_connection_send (kf_jabber_get_connection (), msg, NULL);
	lm_message_unref (msg);
}


/*
 * Callback for clicking on close button
 */
static void on_close_clicked (GtkButton *button, gpointer data)
{
	KfContactsRecvDialog *self = data;
	gtk_widget_destroy (self->window);
}


LmHandlerResult kf_contacts_recv_msg_handle (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *node = user_data;
	LmMessageNode *item;
	KfContactsRecvDialog *win;
	const gchar *from;
	from = lm_message_node_get_attribute (message->node, "from");
	       
	win = kf_contacts_recv_dialog_new (from);

	for (item = node->children; item; item = item->next) {
		foo_debug ("Processing <%s>\n", item->name);
		if (strcmp (item->name, "item") == 0) {
			/* That's what we are looking for... */
			const gchar *jid = NULL;
			const gchar *name = NULL;
			const gchar *group = NULL;
			LmMessageNode *group_node = NULL;

			jid = lm_message_node_get_attribute (item, "jid");
			name = lm_message_node_get_attribute (item, "name");
			
			group_node = lm_message_node_get_child (item, "group");
			if (group_node)
				group = lm_message_node_get_value (group_node);

			kf_contacts_recv_dialog_add (win, jid, name, group);
		}
	}
	gtk_widget_show (win->window);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}
