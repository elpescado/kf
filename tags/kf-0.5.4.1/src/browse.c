/* file browse.c */
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
#include "browse.h"
#include "connection.h"
#include "register.h"
#include "search.h"
#include "vcard.h"
#include "muc_join.h"


struct _KfBrowseResult {
	gchar *name;
	gchar *jid;
	gchar *category;
	gchar *type;
	GSList *ns;

	/* We don't needs those fields... */
/*	struct _KfBrowseResult *parent;
	struct _KfBrowseResult *children;
	struct _KfBrowseResult *next;
	struct _KfBrowseResult *prev;*/
};
typedef struct _KfBrowseResult KfBrowseResult;

typedef struct {
	GtkTreeStore *store;
	GSList *garbage;
} KfBrowseContext;

typedef struct {
	gchar *type;
	gchar *file;
	GdkPixbuf *pixbuf;
} KfBrowseIcon;

static KfBrowseIcon browse_pixmaps[] = {
	{"jabber", "online.png", NULL},
	{"x-gadugadu", "browse_gg.png", NULL},
	{"icq", "browse_icq.png", NULL},
	{"ICQ", "browse_icq.png", NULL},
	{"aim", "browse_aim.png", NULL},
	{"msn", "browse_msn.png", NULL},
	{"public","browse_room.png", NULL},
	{"private","browse_room.png", NULL},
	{"jud","browse_jud.png", NULL},
	{"sms","browse_sms.png", NULL},
	{"tlen","browse_tlen.png", NULL},
	{"x-tlen","browse_tlen.png", NULL},
	{"dictionary", "browse_book.png", NULL},
	{"rss", "browse_news.png", NULL},
	{"yahoo","browse_yahoo.png", NULL},
	{"notice","browse_mail.png", NULL},
	{"x-weather","browse_weather.png", NULL},
	{NULL, NULL, NULL}};


static
gboolean    clicked_cb                     (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data);
static
void on_pixbuf_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static
void on_text_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data);

/*
static void kf_browse_jid (const gchar *jid, GtkWidget *window);
static LmHandlerResult browse_hendel        (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);

void        on_browse_register_clicked     (GtkButton *button,
                                            gpointer user_data);
void        on_browse_search_clicked       (GtkButton *button,
                                            gpointer user_data);
void        on_browse_join_clicked         (GtkButton *button,
                                            gpointer user_data);

static void kf_browse_gc (GSList *list);
void kf_browse_result_free (KfBrowseResult *result); 
*/
void on_browse_button_clicked (GtkButton *button, gpointer user_data);
void on_browse_browse_clicked (GtkWidget *button, gpointer user_data);
void on_browse_register_clicked (GtkButton *button, gpointer user_data);
void on_browse_search_clicked (GtkButton *button, gpointer user_data);
void on_browse_join_clicked (GtkButton *button, gpointer user_data);
void on_browse_info_clicked (GtkButton *button, gpointer user_data);
void on_browse_ok_clicked (GtkButton *button, gpointer user_data);
static void kf_browse_jid (const gchar *jid, GtkWidget *window);
static LmHandlerResult browse_hendel (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data);
static void kf_browse_gc (GSList *list);
static void kf_browse_display_results (LmMessageNode *node, GtkTreeIter *parent, KfBrowseContext *c);
KfBrowseResult *kf_browse_result_new (const gchar *name, const gchar *jid);
void kf_browse_result_free (KfBrowseResult *result);
gboolean on_browse_treeview_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data);


extern LmConnection *kf_jabber_connection;

void kf_browse_window (void) {
	GladeXML *browser;
	GtkTreeStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkTreeView *tv;
	GtkWidget *widget;
	GtkWidget *entry;


	browser = glade_xml_new (kf_find_file ("browse.glade"), NULL, NULL);
	glade_xml_signal_autoconnect (browser);

	tv = GTK_TREE_VIEW (glade_xml_get_widget (browser, "browse_treeview"));
/*	GtkTreeStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
*/
	store = gtk_tree_store_new (1,
			G_TYPE_POINTER);

	/* Ustawienie GtkTreeView */	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, on_pixbuf_data, NULL, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, on_text_data, NULL, NULL);
	gtk_tree_view_column_set_title (column, "Service");
	gtk_tree_view_append_column (tv, column);

/*	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, "JID");
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_cell_data_func (column, renderer, on_text_data, GINT_TO_POINTER (1), NULL);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_append_column (tv, column);
*/
	
	gtk_tree_view_set_model (tv, GTK_TREE_MODEL (store));

	/* Setup the selection handler */
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
//	g_signal_connect (G_OBJECT (select), "changed",
  //                G_CALLBACK (tree_selection_changed_cb),
    //              NULL);
	
	g_signal_connect (G_OBJECT (tv), "button_press_event",
			G_CALLBACK (clicked_cb), NULL);

	entry = glade_xml_get_widget (browser, "jid_entry");
	gtk_entry_set_text (GTK_ENTRY (entry), kf_connection_get_server ());
	
//	CONNECT ("browse_register", on_browse_register_clicked);
//	CONNECT ("browse_search", on_browse_search_clicked);
//	CONNECT ("browse_join", on_browse_join_clicked);
	widget = glade_xml_get_widget (browser, "browse_register");
				g_signal_connect (G_OBJECT (widget), "clicked",
						G_CALLBACK (on_browse_register_clicked), tv);

	widget = glade_xml_get_widget (browser, "browse_search");
				g_signal_connect (G_OBJECT (widget), "clicked",
						G_CALLBACK (on_browse_search_clicked), tv);

	widget = glade_xml_get_widget (browser, "browse_join");
				g_signal_connect (G_OBJECT (widget), "clicked",
						G_CALLBACK (on_browse_join_clicked), tv);

	widget= glade_xml_get_widget (browser, "browse_window");

	kf_signal_connect (browser, "popup_browse", "activate",
			G_CALLBACK (on_browse_browse_clicked), tv);
	kf_signal_connect (browser, "popup_register", "activate",
			G_CALLBACK (on_browse_register_clicked), tv);
	kf_signal_connect (browser, "popup_search", "activate",
			G_CALLBACK (on_browse_search_clicked), tv);
	kf_signal_connect (browser, "popup_info", "activate",
			G_CALLBACK (on_browse_info_clicked), tv);
	kf_signal_connect (browser, "popup_join", "activate",
			G_CALLBACK (on_browse_join_clicked), tv);
	
	g_object_set_data_full (G_OBJECT (widget), "glade", browser, g_object_unref);

}

/*
 * Handle right clicks
 */
static
gboolean    clicked_cb                     (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data)
{
	GtkTreeView *view = (GtkTreeView *) widget;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (event->window == gtk_tree_view_get_bin_window (view) &&
		gtk_tree_view_get_path_at_pos (view, event->x, event->y,  &path, NULL,
			NULL, NULL)) {
		/* W miejscu klikniêcia jest co¶ */
		model = gtk_tree_view_get_model (view);

		if (!gtk_tree_model_get_iter (model, &iter, path)) {
			gtk_tree_path_free (path);
			return FALSE;
		}

		if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
			/* Klikniêcie 3 klawiszem na kontakcie */
			GladeXML *glade;
			GtkWidget *menu;

			glade = glade_get_widget_tree (widget);

			menu = glade_xml_get_widget (glade, "popup_menu");

			gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, event->time);
		} else if (event->type == GDK_2BUTTON_PRESS) {
			on_browse_browse_clicked (GTK_WIDGET (view), (gpointer) view);
		}
		gtk_tree_path_free (path);
	}
	return FALSE;
}

static
void on_pixbuf_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	KfBrowseResult *result;
	gboolean set = FALSE;
	
	gtk_tree_model_get (model, iter, 0, &result, -1);

	if (result) {
		KfBrowseIcon *icon;

		for (icon = browse_pixmaps; icon->type; icon++) {
			if (result->type && !strcmp (icon->type, result->type)) {
				if (!(icon->pixbuf)) {
					icon->pixbuf = gdk_pixbuf_new_from_file (
							kf_find_file (icon->file), NULL);
				}

				g_object_set (G_OBJECT (renderer), "pixbuf", icon->pixbuf, NULL);
				set = TRUE;
			}
		}
	}

	if (!set)
		g_object_set (G_OBJECT (renderer), "pixbuf", NULL, NULL);
}

static
void on_text_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	KfBrowseResult *result;
	
	gtk_tree_model_get (model, iter, 0, &result, -1);

	if (result) {
		if (data)
			g_object_set (G_OBJECT (renderer), "text", result->jid, NULL);
		else
			g_object_set (G_OBJECT (renderer), "text", result->name, NULL);
	}
}

void        on_browse_button_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *browser;
	GtkWidget *jid_entry;
	const gchar *jid;

	browser = glade_get_widget_tree (GTK_WIDGET (button));
	jid_entry = glade_xml_get_widget (browser, "jid_entry");
	jid = gtk_entry_get_text (GTK_ENTRY (jid_entry));

	kf_browse_jid (jid, gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

void        on_browse_browse_clicked     (GtkWidget *button,
                                            gpointer user_data)
{
	GtkTreeSelection *selection;
        GtkTreeIter iter;
        GtkTreeModel *model;
	KfBrowseResult *result;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (user_data));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));
        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
		GladeXML *glade;
		GtkWidget *widget;
		GtkWidget *jid_entry;

		glade = glade_get_widget_tree (button);
		widget = glade_xml_get_widget (glade, "browse_window");
		
                gtk_tree_model_get (model, &iter, 0, &result, -1);

		jid_entry = glade_xml_get_widget (glade, "jid_entry");
		gtk_entry_set_text (GTK_ENTRY (jid_entry), result->jid);
		
		kf_browse_jid (result->jid, widget);
        }
}


void        on_browse_register_clicked     (GtkButton *button,
                                            gpointer user_data)
{
	GtkTreeSelection *selection;
        GtkTreeIter iter;
        GtkTreeModel *model;
	KfBrowseResult *result;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (user_data));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));
        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
                gtk_tree_model_get (model, &iter, 0, &result, -1);

		kf_register_window_x (result->jid);
        }
}

void        on_browse_search_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	GtkTreeSelection *selection;
        GtkTreeIter iter;
        GtkTreeModel *model;
	KfBrowseResult *result;
//	GSList *list;

	g_object_steal_data (G_OBJECT (user_data), "garbage");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (user_data));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));
        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
                gtk_tree_model_get (model, &iter, 0, &result, -1);

		kf_search_window_x (result->jid);
        }
}

void        on_browse_join_clicked         (GtkButton *button,
                                            gpointer user_data)
{
	GtkTreeSelection *selection;
        GtkTreeIter iter;
        GtkTreeModel *model;
	KfBrowseResult *result;
//	GSList *list;

	g_object_steal_data (G_OBJECT (user_data), "garbage");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (user_data));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));
        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
                gtk_tree_model_get (model, &iter, 0, &result, -1);

	//	kf_search_window_x (result->jid);
		kf_muc_join_dialog_jid (result->jid);
        }

}
void        on_browse_info_clicked         (GtkButton *button,
                                            gpointer user_data)
{
	GtkTreeSelection *selection;
        GtkTreeIter iter;
        GtkTreeModel *model;
	KfBrowseResult *result;
//	GSList *list;

	g_object_steal_data (G_OBJECT (user_data), "garbage");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (user_data));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));
        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
                gtk_tree_model_get (model, &iter, 0, &result, -1);

		kf_vcard_get (result->jid);
        }
}



void        on_browse_ok_clicked           (GtkButton *button,
                                            gpointer user_data)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

static
void kf_browse_jid (const gchar *jid, GtkWidget *window) {
	LmMessageHandler *hendel;
	LmMessage *msg;
	LmMessageNode *node;

	hendel = lm_message_handler_new (browse_hendel, (gpointer *) gtk_widget_ref (window), NULL);
	msg = lm_message_new_with_sub_type (jid, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	node = lm_message_node_add_child (msg->node, "item", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:browse");

	lm_connection_send_with_reply (kf_jabber_connection, msg, hendel, NULL);
//	g_object_set_data (G_OBJECT (window), "jud", userdir);
	lm_message_unref (msg);
}

static LmHandlerResult browse_hendel        (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *node;
	GladeXML *browser;
	GtkWidget *tv;
	GtkTreeStore *store;
	KfBrowseContext context;
	
	lm_message_handler_unref (handler);
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
				"An error occured while browsing:\n#%d: %s",
				errcode, errstr);
		gtk_dialog_run (GTK_DIALOG (msg));
		gtk_widget_destroy (msg);
		/* Zamknijmy te¿ okno nadrzêdne... */
		/* nie, nie robmy tego... */
//		gtk_widget_destroy (GTK_WIDGET (user_data));
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	} 
	
//	node = lm_message_node_get_child (message->node, "service") || lm_message_node_get_child (message->node, "item" || lm_message_node_get_child (message->node, "headline")) ;
	node = message->node->children;
	if (!node) {
		/* Nast±pi³ b³±d podczas rejestracji... */
		GtkWidget *msg;

		msg = gtk_message_dialog_new (GTK_WINDOW (user_data),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("This JID can't be browsed..."));
		gtk_dialog_run (GTK_DIALOG (msg));
		gtk_widget_destroy (msg);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	} 

//	name = lm_message_node_get_attribute (node, "name");
//	jid = lm_message_node_get_attribute (node, "jid");


	browser = glade_get_widget_tree (GTK_WIDGET (user_data));
	tv = glade_xml_get_widget (browser, "browse_treeview");
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tv)));
	gtk_tree_store_clear (store);

	context.store = store;
	context.garbage = NULL;
	kf_browse_display_results (message->node->children, NULL, &context);

	g_object_set_data_full (G_OBJECT (user_data), "garbage", context.garbage, (GDestroyNotify) kf_browse_gc);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (tv));

	gtk_widget_unref (GTK_WIDGET (user_data));
	
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static void kf_browse_gc (GSList *list) {
	while (list) {
		kf_browse_result_free ((KfBrowseResult *) list->data);
		list = list->next;
	}
	g_slist_free (list);
}

static
void kf_browse_display_results (LmMessageNode *node, GtkTreeIter *parent, KfBrowseContext *c) {
	while (node) {
		const gchar *name, *jid;
		GtkTreeIter iter;
		KfBrowseResult *result;

//		if (strcmp (node->name, "item") && strcmp (node->name, "service") &&
//				strcmp (node->name, "headline")) {
//			node = node->next;
//			continue;
//		}

		name = lm_message_node_get_attribute (node, "name");
		jid = lm_message_node_get_attribute (node, "jid");
		if (jid == NULL) {
			node=node->next;
			continue;
		}


		result = kf_browse_result_new (name, jid);
		result->type = g_strdup (lm_message_node_get_attribute (node, "type"));
		c->garbage = g_slist_prepend (c->garbage, result);
		
		g_print ("-> %s \t<%s> \t(name: %s, type: %s)\n", name, jid, node->name, result->type);

		gtk_tree_store_append (c->store, &iter, parent);
		gtk_tree_store_set (c->store, &iter, 0, result, -1);

		kf_browse_display_results (node->children, &iter, c);
		node = node->next;
	}
}

KfBrowseResult *kf_browse_result_new (const gchar *name, const gchar *jid) {
	KfBrowseResult *result;

	result = g_new (KfBrowseResult, 1);

	result->name = g_strdup (name);
	result->jid = g_strdup (jid);
	result->category = NULL;
	result->type = NULL;
	
	return result;
}

void kf_browse_result_free (KfBrowseResult *result) {
	g_free (result->name);
	g_free (result->jid);
	g_free (result->category);
	g_free (result->type);
	g_free (result);
}

gboolean on_browse_treeview_button_press_event (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data)
{
	GtkTreeView *view = (GtkTreeView *) widget;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	KfBrowseResult *item;

	if (event->window == gtk_tree_view_get_bin_window (view) &&
		gtk_tree_view_get_path_at_pos (view, event->x, event->y,  &path, NULL,
			NULL, NULL)) {
		/* W miejscu klikniêcia jest co¶ */
		model = gtk_tree_view_get_model (view);

		if (!gtk_tree_model_get_iter (model, &iter, path))
			return FALSE;

		gtk_tree_model_get (model, &iter, 0, &item, -1);

		/* To wszystko na pocz±tek */
		
		if (item && event->type == GDK_BUTTON_PRESS && event->button == 3) {
			/* Klikniêcie 3 klawiszem na kontakcie */
		}
		gtk_tree_path_free (path);
	}
	return FALSE;
}
