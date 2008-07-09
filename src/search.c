/* file search.c */
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
#include "gui.h"
#include "search.h"
#include "x_data.h"
#include "callbacks.h"
#include "vcard.h"

typedef struct {
	gchar *id;
	GtkWidget *window;
	GtkListStore *model;
	GHashTable *hash;
	GSList *garbage;
	gint n_cols;
	gint jid_column;
} KfSearchSession;

static LmHandlerResult sercz_hendel         (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);
static LmHandlerResult sercz_hendel2        (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);
static void search_text_data                (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *cell,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void kf_search_get_form (const gchar *userdir, GtkWidget *window);
static void search_get_results (LmMessage *msg, GtkWidget *window);
static void search_result_process (LmMessage *message, KfSearchSession *context);

//static void on_destroy_garbage (GSList *garbage);

static GtkListStore *results_treeview (GtkTreeView *tv);
static GHashTable *create_array_and_columns (LmMessageNode *node, GtkTreeView *tv, KfSearchSession *context);
static gchar **insert_result (LmMessageNode *node, KfSearchSession *context); 

static gchar *get_selected_jid (GtkWidget *searchbox);
static KfSearchSession *kf_search_session_new (const gchar *id);
static void kf_search_session_register (KfSearchSession *sess);
static void kf_search_session_unregister (KfSearchSession *sess);
static void kf_search_session_garbage_collector (GSList *garbage);

void        on_search_cancel_clicked       (GtkButton *button,
                                            gpointer user_data);
void        on_search_back_clicked         (GtkButton *button,
                                            gpointer user_data);
void        on_search_ok_clicked           (GtkButton *button,
                                            gpointer user_data);
void        on_search_ok2_clicked          (GtkButton *button,
                                            gpointer user_data);
void        on_search_add_user_clicked     (GtkButton *button,
                                            gpointer user_data);
void        on_search_info_clicked         (GtkButton *button,
                                            gpointer user_data);


extern LmConnection *kf_jabber_connection;
/* Aktywne sesje */
static GHashTable *search_sessions = NULL;

/****************************/

/* Shows a Search window */
void kf_search_window (void) {
	GladeXML *searchbox;
	GtkWidget *window;

	searchbox = glade_xml_new (kf_find_file ("search.glade"), NULL, NULL);
	glade_xml_signal_autoconnect (searchbox);

	window = glade_xml_get_widget (searchbox, "search_window");
	g_object_set_data_full (G_OBJECT (window), "glade", searchbox, g_object_unref);
}

/* Shows a Search window and retrieves a search form from jid */
void kf_search_window_x (const gchar *jid) {
	GladeXML *searchbox;
	GtkWidget *entry, *button;

	searchbox = glade_xml_new (kf_find_file ("search.glade"), NULL, NULL);
	glade_xml_signal_autoconnect (searchbox);

	entry = glade_xml_get_widget (searchbox, "jud_name");
	gtk_entry_set_text (GTK_ENTRY (entry), jid);

	button = glade_xml_get_widget (searchbox, "search_ok");
	gtk_button_clicked (GTK_BUTTON (button));
}

/* Callback for "Cancel" button */
void        on_search_cancel_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

/* Callback for "Back" button */
void        on_search_back_clicked         (GtkButton *button,
                                            gpointer user_data)
{
	GtkWidget *notebook;
	GladeXML *searchbox;

	searchbox = glade_get_widget_tree (GTK_WIDGET (button));
	notebook = glade_xml_get_widget (searchbox, "notebook1");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 2);
}

/* Callback for "OK" button in JUD selector */
void        on_search_ok_clicked           (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *searchbox;
	GtkWidget *entry;
	const gchar *jud;
	GtkWidget *notebook;

	searchbox = glade_get_widget_tree (GTK_WIDGET (button));
	entry = glade_xml_get_widget (searchbox, "jud_name");
	jud = gtk_entry_get_text (GTK_ENTRY (entry));
	notebook = glade_xml_get_widget (searchbox, "notebook1");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 1);

	kf_search_get_form (jud, gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

/* Internal function to get a form */
static void kf_search_get_form (const gchar *userdir, GtkWidget *window) {
	LmMessageHandler *hendel;
	LmMessage *msg;
	LmMessageNode *node;

	hendel = lm_message_handler_new (sercz_hendel, (gpointer *) window, NULL);
	msg = lm_message_new_with_sub_type (userdir, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:search");

	lm_connection_send_with_reply (kf_jabber_connection, msg, hendel, NULL);
	g_object_set_data_full (G_OBJECT (window), "jud", g_strdup (userdir), g_free);
	lm_message_unref (msg);
}

/* Callback for incoming form */
static LmHandlerResult sercz_hendel         (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *node;
	GtkWidget *window;
	GtkWidget *table;
	GtkWidget *notebook;
	GladeXML *searchbox;
	
	
	node = lm_message_node_find_child (message->node, "x");
	if (!node) {
		/* Nast±pi³ b³±d podczas rejestracji... */
		GtkWidget *msg;

		msg = gtk_message_dialog_new (GTK_WINDOW (user_data),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("Agent didn't supply registration form..."));
		gtk_dialog_run (GTK_DIALOG (msg));
		gtk_widget_destroy (msg);
		/* Zamknijmy te¿ okno nadrzêdne... */
		gtk_widget_destroy (GTK_WIDGET (user_data));
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	} 
	table = kf_x_data_form (node->children);
	
	gtk_window_resize (GTK_WINDOW (user_data), 420, 400);
	searchbox = glade_get_widget_tree (GTK_WIDGET (user_data));
	window = glade_xml_get_widget (searchbox, "form_frame");
	notebook = glade_xml_get_widget (searchbox, "notebook1");
	gtk_container_add (GTK_CONTAINER (window), table);
	gtk_widget_show (window);
	gtk_widget_show (GTK_WIDGET (table));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 2);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

/* Callback for "OK" button that submits a form */
void        on_search_ok2_clicked          (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *searchbox;
	GtkWidget *frame, *table, *window;
	GSList *list;
	LmMessage *msg;
	LmMessageNode *node;
	const gchar *jud;
	LmMessageHandler *hendel;
	GtkWidget *notebook;
	
	window = gtk_widget_get_toplevel (GTK_WIDGET (button));
	jud = g_object_get_data (G_OBJECT (window), "jud");

	searchbox = glade_get_widget_tree (GTK_WIDGET (button));
	frame = glade_xml_get_widget (searchbox, "form_frame");
	table = GTK_BIN (frame)->child;
	list = g_object_get_data (G_OBJECT (table), "Fields");
	if (!list) g_error ("Ni ma pol");
	

	msg = lm_message_new_with_sub_type (jud ,LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:search");
	
	kf_x_data_create_node (node, list);

	hendel = lm_message_handler_new (sercz_hendel2, (gpointer *) window, NULL);

	search_get_results (msg, window);
	lm_message_unref (msg);
	notebook = glade_xml_get_widget (searchbox, "notebook1");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 3);

}

/* Sends search query and register listener for results */
static void search_get_results (LmMessage *msg, GtkWidget *window) {
	KfSearchSession *context;
	gchar *id;		/* ID zapytania */
	static gint seq = 1;	/* Kolejny numer w ID */

	id = g_strdup_printf ("search_%d", seq++);
	lm_message_node_set_attribute (msg->node, "id", id);

	/* Create new context */
	context = kf_search_session_new (id);
	context->window = window;

	/* Register new session */
	kf_search_session_register (context);

	/* Send your query */
	lm_connection_send (kf_jabber_connection, msg, NULL);

	/* Save a pointer to seacrh session structure and register
	 * a callback to free it when no longer used */
	g_object_set_data_full (G_OBJECT (window), "session", context,
			(GDestroyNotify) kf_search_session_unregister);

	g_free (id); /* A mo¿e lepiej nie... kto wie... TODO */
}

/* Callback for all incoming results */
LmHandlerResult search_result_callback (LmMessageHandler *handler,
					LmConnection *connection,
					LmMessage *message,
					gpointer user_data) {
	const gchar *id;
	KfSearchSession *context;

	foo_debug ("Search result:\n%s\n", lm_message_node_to_string (message->node));

	id = lm_message_node_get_attribute (message->node, "id");
	if ((context = g_hash_table_lookup (search_sessions, id))) {
		search_result_process (message, context);
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

/* Callback for incoming search results */
static void search_result_process (LmMessage *message, KfSearchSession *context) {
	LmMessageNode *node;
	GtkWidget *treeview;
	GtkWidget *notebook;
	GladeXML *searchbox;
	
	node = lm_message_node_find_child (message->node, "x");
	searchbox = glade_get_widget_tree (context->window);
	treeview = glade_xml_get_widget (searchbox, "results_treeview");

	node = node->children;

	while (node) {
		if (!strcmp (node->name, "reported")) {
			context->hash = create_array_and_columns (node->children,
					GTK_TREE_VIEW (treeview), context);
			context->model = results_treeview (GTK_TREE_VIEW (treeview));
//			if (context->hash == NULL) g_print ("!) hash is nuull!!!\n");
		} else if (!strcmp (node->name, "item")) {
			gchar **vars;

			vars = insert_result (node->children, context);

			/* As the values are strdup'ed, let's store them in order
			 * to free them later */
			context->garbage = g_slist_prepend (context->garbage, vars);
		}
		node = node->next;
	}

	notebook = glade_xml_get_widget (searchbox, "notebook1");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 4);
}


/* This function is obsolete */
static LmHandlerResult sercz_hendel2        (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *node;
	GtkWidget *treeview;
	GtkWidget *notebook;
	GladeXML *searchbox;
	GHashTable *hash = NULL;
	guint n_cols = 0;
	gint jid_column = 0;
	GtkListStore *model;
	GSList *garbage = NULL;	/* Garbage to be collected */
	

	foo_debug ("Obsolete\n");
	if (strcmp (g_object_get_data (G_OBJECT (user_data), "id"), lm_message_node_get_attribute (message->node, "id"))) {
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}
	
	node = lm_message_node_find_child (message->node, "x");
	
	searchbox = glade_get_widget_tree (GTK_WIDGET (user_data));
	treeview = glade_xml_get_widget (searchbox, "results_treeview");

	node = node->children;

	if ((hash = g_object_get_data (G_OBJECT (user_data), "hash")) == NULL) {
		model = GTK_LIST_STORE (results_treeview (GTK_TREE_VIEW (treeview)));
	} else {
		model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)));
	}
	n_cols = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (user_data), "n_cols"));
	
	while (node) {
		if (!strcmp (node->name, "reported")) {
		} else if (!strcmp (node->name, "item")) {
			gchar **vars;
			
			/* As the values are strdup'ed, let's store them in order
			 * to free them later */
			garbage = g_slist_prepend (garbage, vars);
		}
		node = node->next;
	}
	
	notebook = glade_xml_get_widget (searchbox, "notebook1");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 4);

	g_object_set_data (G_OBJECT (user_data), "garbage", garbage);
	g_object_set_data (G_OBJECT (user_data), "jid_column", GINT_TO_POINTER (jid_column));
	g_object_set_data (G_OBJECT (user_data), "n_cols", GINT_TO_POINTER (n_cols));
	g_object_set_data_full (G_OBJECT (user_data), "hash", hash,
			(GDestroyNotify) g_hash_table_destroy);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

/* setup for GtkTreeView */
static GtkListStore *results_treeview (GtkTreeView *tv) {
	GtkListStore *store;

	if ((store = GTK_LIST_STORE (gtk_tree_view_get_model (tv)))) {
		GtkTreeViewColumn *column;
		
		gtk_list_store_clear (store);

		/* We should remove columns */
		while ((column = gtk_tree_view_get_column (tv, 0))) {
			gtk_tree_view_remove_column (tv, column);
		}
	} else {
		store = gtk_list_store_new (1, G_TYPE_POINTER);
	
		gtk_tree_view_set_model (tv, GTK_TREE_MODEL (store));
	}

	return store;
}

/* Creates columns */
static GHashTable *create_array_and_columns (LmMessageNode *node, GtkTreeView *tv, KfSearchSession *context) {
	GHashTable *hash;
	gint index = 0;
	
	hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	while (node) {
		const gchar *var, *title, *type;
		GtkCellRenderer *renderer;
		if (strcmp (node->name, "field")) {
			node = node->next;
			continue;
		}

		var = lm_message_node_get_attribute (node, "var");
		title = lm_message_node_get_attribute (node, "label");
		if ((type = lm_message_node_get_attribute (node, "type")) && !strcmp (type, "jid-single")) {
			context->jid_column = index;
		}

		g_hash_table_insert (hash, g_strdup (var), GINT_TO_POINTER (index));

		renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_insert_column_with_data_func (
				tv,
				index,
				title,
				renderer,
				search_text_data,
				GINT_TO_POINTER (index),
				NULL);
			
		index++;
		node = node->next;
	}
	context->n_cols = index;
	
	return hash;
}

static gchar **insert_result (LmMessageNode *node, KfSearchSession *context) {
	gchar **texts;
	GtkTreeIter iter;

	texts = g_new0 (gchar *, (context->n_cols)+1);

	while (node) {
		const gchar *name;
		const gchar *value;
		LmMessageNode *child;
		guint pos;
		
		if (strcmp (node->name, "field")) {
			node = node->next;
			continue;
		}
		name = lm_message_node_get_attribute (node, "var");
		child = lm_message_node_get_child (node, "value");
		value = lm_message_node_get_value (child);

		pos = GPOINTER_TO_INT (g_hash_table_lookup (context->hash, name));
		texts[pos] = g_strdup (value);

		node = node->next;
	}

	gtk_list_store_append (context->model, &iter);
	gtk_list_store_set (context->model, &iter, 0, texts, -1);
	return texts;
}

static void search_text_data                (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
	gchar **vals;

	gtk_tree_model_get (tree_model, iter, 0, &vals, -1);
	g_object_set (G_OBJECT (renderer), "text", vals[GPOINTER_TO_INT (data)], NULL);
}

/* Frees the memory allocated for search results */
/*
static void on_destroy_garbage (GSList *garbage) {
	GSList *list = garbage;

	while (list) {
		gchar **texts, **t;

		t = texts = list->data;
		while (*t != NULL) {
			g_free (*t);
			t++;
		}
		g_free (texts);
		list = list->next;
	}
	g_slist_free (garbage);
}
*/
void        on_search_add_user_clicked     (GtkButton *button,
                                            gpointer user_data)
{
	const gchar *jid;
	
	jid = get_selected_jid (gtk_widget_get_toplevel (GTK_WIDGET(button)));

	if (jid);
		kf_gui_add_contact (jid);
}

void        on_search_info_clicked         (GtkButton *button,
                                            gpointer user_data)
{
	const gchar *jid;
	
	jid = get_selected_jid (gtk_widget_get_toplevel (GTK_WIDGET(button)));

	if (jid);
		kf_vcard_get (jid);
}

static gchar *get_selected_jid (GtkWidget *searchbox) {
	gint column;
	GladeXML *glade;
	GtkWidget *view;
	GtkTreeSelection *select;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar **data = NULL;

	column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (searchbox), "jid_column"));
	glade = glade_get_widget_tree (searchbox);
	view = glade_xml_get_widget (glade, "results_treeview");
	
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	if (select == NULL)
		return NULL;
	
	
        if (gtk_tree_selection_get_selected (select, &model, &iter))
        {
                gtk_tree_model_get (model, &iter, 0, &data, -1);
        }
	if (data)
		return data[column];
	else
		return NULL;
}

/* Creates new search session */
static KfSearchSession *kf_search_session_new (const gchar *id) {
	KfSearchSession *sess;

	sess = g_new (KfSearchSession, 1);
	sess->id = g_strdup (id);

	sess->hash = NULL;
	sess->garbage = NULL;

	return sess;
}

/* Registers a search session */
static void kf_search_session_register (KfSearchSession *sess) {
	if (search_sessions == NULL) {
		search_sessions = g_hash_table_new (g_str_hash, g_str_equal);
	}
	g_hash_table_insert (search_sessions, sess->id, sess);
}

/* Unregisters and frees searcg session */
static void kf_search_session_unregister (KfSearchSession *sess) {
	g_hash_table_remove (search_sessions, sess->id);
	g_free (sess->id);
	if (sess->hash)
		g_hash_table_destroy (sess->hash);
	kf_search_session_garbage_collector (sess->garbage);
	g_free (sess);
}

static void kf_search_session_garbage_collector (GSList *garbage) {
	GSList *list = garbage;

	while (list) {
		gchar **texts, **t;

		t = texts = list->data;
		while (*t != NULL) {
			g_free (*t);
			t++;
		}
		g_free (texts);
		list = list->next;
	}
	g_slist_free (garbage);
}

