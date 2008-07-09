/* file muc_join.c */
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
#include "muc.h"
#include "muc_join.h"

typedef struct {
	GtkWidget *window;
	
	GtkWidget *table;	/* Table with entries */

	/* Entries */
	GtkWidget *server;
	GtkWidget *room;
	GtkWidget *nickname;
	GtkWidget *password;
	GtkWidget *use_password;

	/* Bookmarks manager */
	GtkWidget *bookmarks;
	GtkWidget *add;
	GtkWidget *del;

	/* Buttons */
	GtkWidget *join;
	GtkWidget *cancel;

	gboolean multiple;	/* Whether multiple bookmarks have been selected */
} KfMUCJoinDialog;

/* Say, a constructor */
static KfMUCJoinDialog *kf_muc_join_dialog_new (void);

/* Button callbacks */
static void join_clicked (GtkButton *button, KfMUCJoinDialog *self);
static void cancel_clicked (GtkButton *button, KfMUCJoinDialog *self);

/* Bookmark buttons callbacks */
static void add_clicked (GtkButton *button, KfMUCJoinDialog *self);
static void del_clicked (GtkButton *button, KfMUCJoinDialog *self);

/* Bookmarks stuff */
static void init_bookmarks (KfMUCJoinDialog *self);
static void update_bookmarks (KfMUCJoinDialog *self);
static void selection_changed_cb (GtkTreeSelection *selection, KfMUCJoinDialog *self);
	
/* Misc callbacks */
static void use_password_toggled (GtkToggleButton *toggle, KfMUCJoinDialog *self);

/* Code */


/* Creates a new join dialog */
static KfMUCJoinDialog *kf_muc_join_dialog_new (void)
{
	GladeXML *glade;
	KfMUCJoinDialog *self;
	/* Stored old MUC */
	const gchar *server;
	const gchar *room;
	const gchar *nickname;
	

	self = g_new (KfMUCJoinDialog, 1);

	glade = glade_xml_new (kf_find_file ("muc_join.glade"), NULL, NULL);
	kf_glade_get_widgets (glade,
			"window", &self->window,
			"server", &self->server,
			"room", &self->room,
			"nickname", &self->nickname,
			"password", &self->password,
			"use_password", &self->use_password,
			"bookmarks", &self->bookmarks,
			"add", &self->add,
			"del", &self->del,
			"join", &self->join,
			"cancel", &self->cancel,
			"table1", &self->table,
			NULL);

	/* Get stored last conference */
	server = kf_preferences_get_string ("mucLastServer");
	room = kf_preferences_get_string ("mucLastRoom");
	nickname = kf_preferences_get_string ("mucLastNick");

	if (server)	gtk_entry_set_text (GTK_ENTRY (self->server), server);
	if (room)	gtk_entry_set_text (GTK_ENTRY (self->room), room);
	if (nickname)	gtk_entry_set_text (GTK_ENTRY (self->nickname), nickname);

	/* Connect signals */
	g_signal_connect (G_OBJECT (self->join), "clicked", G_CALLBACK (join_clicked), self);
	g_signal_connect (G_OBJECT (self->cancel), "clicked", G_CALLBACK (cancel_clicked), self);
	g_signal_connect (G_OBJECT (self->add), "clicked", G_CALLBACK (add_clicked), self);
	g_signal_connect (G_OBJECT (self->del), "clicked", G_CALLBACK (del_clicked), self);
	g_signal_connect (G_OBJECT (self->use_password), "toggled", G_CALLBACK (use_password_toggled), self);
	
	/* Misc stuff */
	init_bookmarks (self);
	update_bookmarks (self);
	g_object_unref (G_OBJECT (glade));
	g_object_set_data_full (G_OBJECT (self->window), "Self", self, g_free);
	return self;
}

/* Button callbacks */
static void join_clicked (GtkButton *button, KfMUCJoinDialog *self)
{
	if (self->multiple) {
		GList *list;
		GtkTreeSelection *selection;
		GList *tmp;
		GtkTreeModel *model;

		foo_debug ("Many bookmarks!\n");

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->bookmarks));
		list = gtk_tree_selection_get_selected_rows (selection, &model);

		for (tmp = list; tmp; tmp = tmp->next) {
			GtkTreePath *path = list->data;
			KfPrefMUCBookmark *bookmark;
			GtkTreeIter iter;
			gchar *jid;
			
			foo_debug ("Got path '%s'\n", gtk_tree_path_to_string (path));
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, 0, &bookmark, -1);

			jid = g_strdup_printf ("%s@%s", bookmark->room, bookmark->server);
			
			foo_debug ("Connecting to '%s'\n", jid);

			if (bookmark->pass)
				kf_muc_join_room_with_password (jid, bookmark->nick, bookmark->pass);
			else
				kf_muc_join_room (jid, bookmark->nick);
			g_free (jid);
		}

		g_list_foreach (list, (GDestroyNotify) gtk_tree_path_free, NULL);
		g_list_free (list);
	} else {
		const gchar *server, *room, *nick, *pass;
		gchar *jid;

		server = gtk_entry_get_text (GTK_ENTRY (self->server));
		room = gtk_entry_get_text (GTK_ENTRY (self->room));
		nick = gtk_entry_get_text (GTK_ENTRY (self->nickname));
		pass = gtk_entry_get_text (GTK_ENTRY (self->password));

		kf_preferences_set ("mucLastServer", server);
		kf_preferences_set ("mucLastRoom", room);
		kf_preferences_set ("mucLastNick", nick);

		jid = g_strdup_printf ("%s@%s", room, server);
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->use_password))) {
			kf_muc_join_room_with_password (jid, nick, pass);
		} else {
			kf_muc_join_room (jid, nick);
		}
		g_free (jid);
	
	}
	gtk_widget_destroy (self->window);
}


static void cancel_clicked (GtkButton *button, KfMUCJoinDialog *self)
{
	gtk_widget_destroy (self->window);
}



/* Bookmark buttons callbacks */
static void add_clicked (GtkButton *button, KfMUCJoinDialog *self)
{
	KfPrefMUCBookmark *bookmark = kf_pref_muc_bookmark_new ();
	const gchar *server, *room, *nick, *pass;

	server = gtk_entry_get_text (GTK_ENTRY (self->server));
	if (server && *server)
		bookmark->server = g_strdup (server);
	room = gtk_entry_get_text (GTK_ENTRY (self->room));
	if (room && *room)
		bookmark->room = g_strdup (room);
	nick = gtk_entry_get_text (GTK_ENTRY (self->nickname));
	if (nick && *nick)
		bookmark->nick = g_strdup (nick);
	pass = gtk_entry_get_text (GTK_ENTRY (self->password));
	if (pass && *pass)
		bookmark->pass = g_strdup (pass);
	
	kf_preferences_muc_bookmark_add (bookmark);
	update_bookmarks (self);
}


static void del_clicked (GtkButton *button, KfMUCJoinDialog *self)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->bookmarks));
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->bookmarks));
	GtkTreeIter iter;
	KfPrefMUCBookmark *bookmark = NULL;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, 0, &bookmark, -1);

		kf_preferences_muc_bookmark_del (bookmark);
	}
	update_bookmarks (self);
}


/* Misc callbacks */
static void use_password_toggled (GtkToggleButton *toggle, KfMUCJoinDialog *self)
{
	gtk_widget_set_sensitive (self->password, gtk_toggle_button_get_active (toggle));
}


/* Bookmarks stuff */
static void init_bookmarks (KfMUCJoinDialog *self) {
	GtkListStore *store = gtk_list_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	gtk_tree_view_set_model (GTK_TREE_VIEW (self->bookmarks), GTK_TREE_MODEL (store));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->bookmarks));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect (G_OBJECT (selection), "changed",
                  G_CALLBACK (selection_changed_cb),
                  self);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Bookmarks"),
			renderer,
			"text", 1,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->bookmarks), column);
}

static void update_bookmarks (KfMUCJoinDialog *self) {
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->bookmarks));
	GtkListStore *store = GTK_LIST_STORE (model);
	GList *tmp = kf_preferences_muc_bookmarks_get ();

	gtk_list_store_clear (store);
	
	for (;tmp;tmp=tmp->next) {
		KfPrefMUCBookmark *bookmark = tmp->data;
		gchar *title;
		GtkTreeIter iter;

		title = g_strdup_printf ("%s@%s",bookmark->room,bookmark->server);
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, bookmark, 1, title, -1);
		
		g_free (title);
	}
}

static void
selection_changed_cb (GtkTreeSelection *selection, KfMUCJoinDialog *self)
{
        GtkTreeIter iter;
        GtkTreeModel *model;
	gint n_selected;
	
	n_selected = gtk_tree_selection_count_selected_rows (selection);
	self->multiple = (n_selected > 1);
	gtk_widget_set_sensitive (self->table, ! self->multiple);
	
	if (self->multiple)	foo_debug ("More than one selected bookmark!\n");
		
	if (n_selected == 1) {
//		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
		GtkTreePath *path = list->data;
		gtk_tree_model_get_iter (model, &iter, path);
		KfPrefMUCBookmark *bookmark;
		gtk_tree_model_get (model, &iter, 0, &bookmark, -1);

		if (bookmark->server)
			gtk_entry_set_text (GTK_ENTRY (self->server), bookmark->server);
		if (bookmark->room)
			gtk_entry_set_text (GTK_ENTRY (self->room), bookmark->room);
		if (bookmark->nick)
			gtk_entry_set_text (GTK_ENTRY (self->nickname), bookmark->nick);

		if (bookmark->pass)
			gtk_entry_set_text (GTK_ENTRY (self->password), bookmark->pass);
		else
			gtk_entry_set_text (GTK_ENTRY (self->password), "");
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->use_password), bookmark->pass != NULL);

		g_list_foreach (list, (GDestroyNotify) gtk_tree_path_free, NULL);
		g_list_free (list);
        }
}


/* Public functions */
void kf_muc_join_dialog (void) {
	KfMUCJoinDialog *dialog;

	dialog = kf_muc_join_dialog_new ();
}

void kf_muc_join_dialog_jid (const gchar *jid) {
	gchar *muc_room;
	gchar *muc_server;
	KfMUCJoinDialog *dialog;

	if (jid == NULL) {
		dialog = kf_muc_join_dialog_new ();
		return;
	}

	muc_room = g_strdup (jid);
	for (muc_server = muc_room; *muc_server != '@' && *muc_server != '\0'; muc_server++)
		;
	if (muc_server == '\0') {
		g_free (muc_room);
		return;
	}
	*muc_server = '\0';
	muc_server++;
		
	foo_debug ("MUC: '%s'@'%s'", muc_room, muc_server);
		
	dialog = kf_muc_join_dialog_new ();
	gtk_entry_set_text (GTK_ENTRY (dialog->server), muc_server);
	gtk_entry_set_text (GTK_ENTRY (dialog->room), muc_room);

	g_free (muc_room);
	
}

