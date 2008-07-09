/* file privacy.c */
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
#include "privacy.h"
#include "privacy_list.h"

typedef struct {
	GtkWidget *win;
	GtkWidget *treeview;
	GtkListStore *store;
	GtkWidget *add;
	GtkWidget *modify;
	GtkWidget *del;
	GtkWidget *set_active;
	GtkWidget *set_default;
	gboolean waiting;
	gchar *selected;
	gchar *active_list;
	gchar *default_list;
} KfPrivacyEditor;

LmConnection *kf_jabber_get_connection (void);
static KfPrivacyEditor *kf_privacy_editor_new (void);
static void kf_privacy_editor_set_waiting (KfPrivacyEditor *self, gboolean value);
static void kf_privacy_editor_fetch_lists (KfPrivacyEditor *self);
static LmHandlerResult kf_privacy_editor_fetch_cb  (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);
static void
on_text_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void kf_privacy_editor_set_list (KfPrivacyEditor *self,
		const gchar *klass, const gchar *name);
static LmHandlerResult kf_privacy_editor_set_cb  (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);

/* Event handlers */
static void selection_changed (GtkTreeSelection *selection, gpointer data);
static void on_set_active_clicked (GtkButton *button, gpointer data);
static void on_set_default_clicked (GtkButton *button, gpointer data);
static void on_close_clicked (GtkButton *button, gpointer data);
static void on_add_clicked (GtkButton *button, gpointer data);
static void on_modify_clicked (GtkButton *button, gpointer data);
static void on_del_clicked (GtkButton *button, gpointer data);
	
void kf_privacy_editor (void) {
	kf_privacy_editor_new ();
}

static KfPrivacyEditor *kf_privacy_editor_new (void) {
	KfPrivacyEditor *self;
	GladeXML *glade = glade_xml_new (kf_find_file ("privacy.glade"), NULL, NULL);
	GtkWidget *close;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	
	self = g_new0 (KfPrivacyEditor, 1);
	self->waiting = FALSE;
	self->selected = NULL;
	kf_glade_get_widgets (glade,
			"window", &self->win,
			"treeview", &self->treeview,
			"set_active", &self->set_active,
			"set_default", &self->set_default,
			"close", &close,
			"add", &self->add,
			"modify", &self->modify,
			"del", &self->del,
			NULL);
	g_object_unref (G_OBJECT (glade));

	self->store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (self->treeview), GTK_TREE_MODEL (self->store));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("List",
                                                   renderer,
                                                   NULL);
	gtk_tree_view_column_set_cell_data_func (column,
				renderer,
				on_text_data,
				NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->treeview), column);
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->treeview));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
        	G_CALLBACK (selection_changed),
        	self);

	g_signal_connect (G_OBJECT (self->set_active), "clicked",
			G_CALLBACK (on_set_active_clicked), self);
	g_signal_connect (G_OBJECT (self->set_default), "clicked",
			G_CALLBACK (on_set_default_clicked), self);
	g_signal_connect (G_OBJECT (close), "clicked",
			G_CALLBACK (on_close_clicked), self);
	g_signal_connect (G_OBJECT (self->add), "clicked",
			G_CALLBACK (on_add_clicked), self);
	g_signal_connect (G_OBJECT (self->modify), "clicked",
			G_CALLBACK (on_modify_clicked), self);
	g_signal_connect (G_OBJECT (self->del), "clicked",
			G_CALLBACK (on_del_clicked), self);
	
	g_object_set_data_full (G_OBJECT (self->win), "self", self, g_free);
	kf_privacy_editor_fetch_lists (self);
	return self;
}


static void kf_privacy_editor_set_waiting (KfPrivacyEditor *self, gboolean value) {
	self->waiting = value;
	gtk_widget_set_sensitive (self->win, ! value);
}


static void kf_privacy_editor_fetch_lists (KfPrivacyEditor *self) {
	LmMessage *msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ,
			LM_MESSAGE_SUB_TYPE_GET);
	LmMessageNode *node = lm_message_node_add_child (msg->node, "query", NULL);
	LmMessageHandler *h = lm_message_handler_new (kf_privacy_editor_fetch_cb,self, NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:privacy");

	lm_connection_send_with_reply (kf_jabber_get_connection (), msg, h, NULL);
	lm_message_unref (msg);
	kf_privacy_editor_set_waiting (self, TRUE);
}


static LmHandlerResult kf_privacy_editor_fetch_cb  (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data) {
	LmMessageNode *n;
	const gchar *default_list = NULL;
	const gchar *active_list = NULL;
	KfPrivacyEditor *self = user_data;
	GtkTreeIter iter;

	gtk_list_store_clear (self->store);
	for (n = message->node->children; n; n = n->next) {
		const gchar *xmlns = lm_message_node_get_attribute (n, "xmlns");
		if (xmlns && strcmp (n->name, "query") == 0 && strcmp (xmlns, "jabber:iq:privacy") == 0) {
			LmMessageNode *node;

			for (node = n->children; node; node = node->next) {
				if (strcmp (node->name, "list") == 0) {
					const gchar *name = lm_message_node_get_attribute (node, "name");
					GtkTreeIter iter;
					gtk_list_store_append (self->store, &iter);
					gtk_list_store_set (self->store, &iter,
						0, name,
						-1);
					foo_debug ("List: %s\n", name);
				} else if (strcmp (node->name, "active") == 0) {
					active_list = lm_message_node_get_attribute (node, "name");
				} else if (strcmp (node->name, "default") == 0) {
					default_list = lm_message_node_get_attribute (node, "name");
				} 
			}
		}
	}

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->store), &iter)) {
		do {
			gchar *name;
	        	gtk_tree_model_get (GTK_TREE_MODEL (self->store), &iter, 0, &name, -1);
			
			if (active_list && strcmp (name, active_list) == 0)
		        	gtk_list_store_set (self->store, &iter, 1, TRUE, -1);
			if (default_list && strcmp (name, default_list) == 0)
		        	gtk_list_store_set (self->store, &iter, 2, TRUE, -1);
			g_free (name);
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->store), &iter));
	}

	g_free (self->active_list); self->active_list = g_strdup (active_list);
	g_free (self->default_list); self->default_list = g_strdup (default_list);
	
	kf_privacy_editor_set_waiting (self, FALSE);
	
	lm_message_handler_unref (handler);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}


static void
on_text_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
	gchar *name;
	gboolean active;
	gboolean def;
	gtk_tree_model_get (tree_model, iter, 0, &name, 1, &active, 2, &def, -1);

	if (def) {
		gchar *str = g_strdup_printf ("%s (%s)", name, _("default"));
		g_object_set (G_OBJECT (renderer), "text", str, NULL);
		g_free (str);
	} else
		g_object_set (G_OBJECT (renderer), "text", name, NULL);
	g_object_set (G_OBJECT (renderer), "font", active?"bold":"normal", NULL);
	g_free (name);
}


static void kf_privacy_editor_set_list (KfPrivacyEditor *self,
		const gchar *klass, const gchar *name) {
	LmMessage *msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ,
			LM_MESSAGE_SUB_TYPE_SET);
	LmMessageNode *node = lm_message_node_add_child (msg->node, "query", NULL);
	LmMessageNode *item = lm_message_node_add_child (node, klass, NULL);
	LmMessageHandler *h = lm_message_handler_new (kf_privacy_editor_set_cb,self, NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:privacy");
	if (name)
		lm_message_node_set_attribute (item, "name", name);

	lm_connection_send_with_reply (kf_jabber_get_connection (), msg, h, NULL);
	lm_message_unref (msg);
	kf_privacy_editor_set_waiting (self, TRUE);

}


static LmHandlerResult kf_privacy_editor_set_cb  (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data) {
	KfPrivacyEditor *self = user_data;
	kf_privacy_editor_set_waiting (self, FALSE);
	if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_RESULT) {
		/* All OK */	
		kf_privacy_editor_fetch_lists (self);
	} else {
		/* Error */
		kf_gui_alert ("Error");
	}

	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}



/*           */
/* Callbacks */
/*           */


static void
selection_changed (GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *name = NULL;
	KfPrivacyEditor *self = data;

	foo_debug ("Selection changed... ");
	g_free (self->selected);
        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
        	gtk_tree_model_get (model, &iter, 0, &name, -1);
		foo_debug ("Got '%s'\n", name);
	} else {
		foo_debug ("Got nothing:-(");
	}
	self->selected = name;

	gtk_widget_set_sensitive (self->set_default, TRUE);
	gtk_widget_set_sensitive (self->del, name != NULL);
	gtk_widget_set_sensitive (self->modify, name != NULL);
	gtk_widget_set_sensitive (self->set_active, name != NULL);
}


static void on_set_active_clicked (GtkButton *button, gpointer data)
{
	KfPrivacyEditor *self = data;
	if (self->selected) {
		if (self->active_list && strcmp (self->selected, self->active_list) == 0)
			kf_privacy_editor_set_list (self, "active", NULL);
		else
			kf_privacy_editor_set_list (self, "active", self->selected);
	}
}


static void on_set_default_clicked (GtkButton *button, gpointer data)
{
	KfPrivacyEditor *self = data;
	if (self->selected) {
		if (self->default_list && strcmp (self->selected, self->default_list) == 0)
			kf_privacy_editor_set_list (self, "default", NULL);
		else
			kf_privacy_editor_set_list (self, "default", self->selected);
	}
}


static void on_close_clicked (GtkButton *button, gpointer data)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


static void on_add_clicked (GtkButton *button, gpointer data)
{
	kf_privacy_edit_list (NULL);
}


static void on_modify_clicked (GtkButton *button, gpointer data)
{
	KfPrivacyEditor *self = data;

	kf_privacy_edit_list (self->selected);
}


static void on_del_clicked (GtkButton *button, gpointer data)
{
	KfPrivacyEditor *self = data;
	if (self->selected) {
		kf_privacy_editor_set_list (self, "list", self->selected);
	}
}


