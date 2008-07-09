/* file settings_statuses.c */
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
#include "preferences.h"
#include "settings_statuses.h"

	extern GList *kf_preferences_statuses;

static void
on_text_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void *get_selection (GtkWidget *widget);
static void on_statuses_remove_clicked     (GtkButton *button,
                                            gpointer user_data);
static void on_statuses_edit_clicked       (GtkButton *button,
                                            gpointer user_data);
static void on_statuses_new_clicked        (GtkButton *button,
                                            gpointer user_data);
static void on_statuses_edit_ok_clicked    (GtkButton *button,
                                            gpointer *data);
static void on_statuses_edit_cancel_clicked    (GtkButton *button,
                                            gpointer *data);

void kf_statuses_editor (GladeXML *glade) {
	GList *tmp;

	GtkWidget *tv;
	GtkListStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	tv = glade_xml_get_widget (glade, "statuses_treeview");
	if ((model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv)))) {
		store = GTK_LIST_STORE (model);
		gtk_list_store_clear (store);
	} else {
		store = gtk_list_store_new (1, G_TYPE_POINTER);

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                   renderer,
                                                   NULL);
		gtk_tree_view_column_set_cell_data_func (column,
				renderer,
				on_text_data,
				NULL, NULL);				
		gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

		gtk_tree_view_set_model (GTK_TREE_VIEW (tv), GTK_TREE_MODEL (store));

		kf_signal_connect (glade, "statuses_remove", "clicked",
			       G_CALLBACK (on_statuses_remove_clicked), NULL);
		kf_signal_connect (glade, "statuses_edit", "clicked",
			       G_CALLBACK (on_statuses_edit_clicked), NULL);
		kf_signal_connect (glade, "statuses_add", "clicked",
			       G_CALLBACK (on_statuses_new_clicked), NULL);
	}

	for (tmp = kf_preferences_statuses; tmp; tmp = tmp->next) {
//		KfPrefAccount *acc = tmp->data;
		GtkTreeIter iter;

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
                    0, tmp->data,
                    -1);

	}
}

/* This function displays a text in a TreeView */
static void
on_text_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
	gchar **texts;
	
	gtk_tree_model_get (tree_model, iter, 0, &texts, -1);

	if (texts) {
		g_object_set (G_OBJECT (renderer), "text", texts[0], NULL);
	}

}

static void *get_selection (GtkWidget *widget) {
	GladeXML *glade = glade_get_widget_tree (widget);
	GtkWidget *tv;
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	
	tv = glade_xml_get_widget (glade, "statuses_treeview");
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
		void *acc;
		gtk_tree_model_get (model, &iter, 0, &acc, -1);

		return acc;
	} else {
		return NULL;
	}

}

static void on_statuses_remove_clicked     (GtkButton *button,
                                            gpointer user_data) {
	GtkWidget *win;
	gint response;

	win = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			_("Do you really want to remove this status?"));
	response = gtk_dialog_run (GTK_DIALOG (win));
	gtk_widget_destroy (win);

	if (response == GTK_RESPONSE_YES) {
		void *acc;

		acc = get_selection (GTK_WIDGET (button));
		if (acc) {
			kf_preferences_statuses = g_list_remove (kf_preferences_statuses, acc);
			kf_statuses_editor (glade_get_widget_tree (GTK_WIDGET (button))); 
		}
	}
}

static void on_statuses_edit_clicked       (GtkButton *button,
                                            gpointer user_data) {
	gpointer *status;

	if ((status = get_selection (GTK_WIDGET (button)))) {
		GladeXML *glade;
		GtkWidget *name;
		GtkWidget *text;
		GtkTextBuffer *buffer;
		
		glade = glade_xml_new (kf_find_file ("status.glade"), NULL, NULL);

		name = glade_xml_get_widget (glade, "name");
		gtk_entry_set_text (GTK_ENTRY (name), status[0]);

		text = glade_xml_get_widget (glade, "text");
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
		gtk_text_buffer_set_text (buffer, status[1], -1);

		kf_signal_connect (glade, "okbutton1", "clicked",
			       G_CALLBACK (on_statuses_edit_ok_clicked), status);
		kf_signal_connect (glade, "cancelbutton1", "clicked",
			       G_CALLBACK (on_statuses_edit_cancel_clicked), status);

		g_object_set_data_full (G_OBJECT (text), "glade", glade, g_object_unref);
	}
}

static void on_statuses_new_clicked        (GtkButton *button,
                                            gpointer user_data) {
	gchar **status;

	status = g_new (gchar *, 2);
	status[0] = g_strdup (_("Unnamed"));
	status[1] = g_strdup (_("Enter text here"));
	
	kf_preferences_statuses = g_list_append (kf_preferences_statuses, status);
	kf_statuses_editor (glade_get_widget_tree (GTK_WIDGET (button))); 
}

static void on_statuses_edit_ok_clicked    (GtkButton *button,
                                            gpointer *data) {
	GladeXML *glade;
	GtkWidget *name;
	GtkWidget *text;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	const gchar *sname;
	gchar *stext;

	glade = glade_get_widget_tree (GTK_WIDGET (button));

	name = glade_xml_get_widget (glade, "name");
	sname = gtk_entry_get_text (GTK_ENTRY (name));

	text = glade_xml_get_widget (glade, "text");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	stext = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

	g_free (data[0]);
	g_free (data[1]);

	data[0] = g_strdup (sname);
	data[1] = stext;

	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

static void on_statuses_edit_cancel_clicked    (GtkButton *button,
                                            gpointer *data) {
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}
