/* file settings_blacklist.c */
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
#include <stdlib.h>
#include "kf.h"
#include "settings_blacklist.h"

static void
on_text_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void add_cb (GtkButton *button, gpointer data);
static void del_cb (GtkButton *button, gpointer data);
static void edited_done                    (GtkCellRendererText *cellrenderertext,
                                            gchar *arg1,
                                            gchar *arg2,
                                            gpointer user_data);

extern GSList *kf_preferences_blocked_jids;

/* This function inits/shows blacklist editor */
void kf_blacklist_manager (GladeXML *glade) {
	GtkWidget *tv;
	GtkListStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	GSList *tmp;

	tv = glade_xml_get_widget (glade, "blacklist_treeview");
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

		kf_signal_connect (glade, "blacklist_add", "clicked",
			       G_CALLBACK (add_cb), store);
		kf_signal_connect (glade, "blacklist_del", "clicked",
			       G_CALLBACK (del_cb), tv);
		g_signal_connect (G_OBJECT (renderer), "edited",
			       G_CALLBACK (edited_done), store);
	}

	for (tmp = kf_preferences_blocked_jids; tmp; tmp = tmp->next) {
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
	gchar *text;
	
	gtk_tree_model_get (tree_model, iter, 0, &text, -1);

	if (text) {
		g_object_set (G_OBJECT (renderer), "text", text, "editable", TRUE, NULL);
	}
}

/* Click on "Add" button */
static void add_cb (GtkButton *button, gpointer data) {
	GtkTreeIter iter;
	GtkTreeModel *model = (GtkTreeModel *)data;
	gchar *text;

	text = g_strdup (_("Enter JID here"));
	//text = g_strdup_printf ("New JID #%d", ++x);
	
	kf_preferences_blocked_jids = g_slist_append (kf_preferences_blocked_jids, text);

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, text, -1);
}

/* Click on "Del" button */
static void del_cb (GtkButton *button, gpointer data) {
  GtkTreeIter iter;
  GtkTreeView *treeview = (GtkTreeView *)data;
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gint i;
      GtkTreePath *path;
      gpointer data;

      path = gtk_tree_model_get_path (model, &iter);
      i = gtk_tree_path_get_indices (path)[0];
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

   //   g_array_remove_index (articles, i);
   	data = g_slist_nth_data (kf_preferences_blocked_jids, i);
	g_slist_remove (kf_preferences_blocked_jids, data);
//	g_free (data);

      gtk_tree_path_free (path);
    }
}

/* Done editing... */
static void edited_done                    (GtkCellRendererText *cellrenderertext,
                                            gchar *arg1,
                                            gchar *arg2,
                                            gpointer data) 
{
	GtkTreeModel *model = (GtkTreeModel *) data;
	gint i = atoi (arg1);
	GSList *tmp;
	GtkTreePath *path = gtk_tree_path_new_from_string (arg1);
	GtkTreeIter iter;
	
	g_print ("Arg1: %s, arg2: %s\n", arg1, arg2);
	tmp = g_slist_nth (kf_preferences_blocked_jids, i);
	
	g_free (tmp->data);
	tmp->data = g_strdup (arg2);

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, tmp->data, -1);

	gtk_tree_path_free (path);
}
