/* file gui.c */
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
#include <string.h>
#include <time.h>

#include "kf.h"
#include "jabber.h"
#include "gui.h"
#include "preferences.h"
#include "dock.h"
#include "www.h"
#include "events.h"
#include "xevent.h"

#include "chat.h"
#include "message.h"
#include "autoaway.h"
#include "gtktip.h"

#define NO_GROUP "Niezgrupowane"
#define STATUS_COLOR "black"

GladeXML *iface;

/* Prototype for selection handler callback */
static void internet_menu_callback (GtkMenuItem *menuitem,
                                            gpointer user_data);
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static
void on_pixbuf_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static
void on_text_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data);

//static void kf_gui_init_pixmaps (void);
static void        roster_groups_clear      (gpointer key,
                                             gpointer value,
                                             gpointer user_data);
static gint kf_gui_roster_compare_callback   (gconstpointer a,
                                             gconstpointer b);
static void kf_gui_roster_add_group (GtkTreeStore *store, GtkTreeIter *iter, KfGuiRosterGroup *grp); 
static void kf_gui_roster_add_item (GtkTreeStore *store, GtkTreeIter *parent, KfJabberRosterItem *roster);
static void kf_gui_roster_set_pixbuf (GtkCellRenderer *renderer, KfJabberRosterItem *item);

gboolean kf_gui_roster_get_iter (GtkTreeIter *iter, KfJabberRosterItem *item);
void kf_gui_update_status (KfJabberRosterItem *item, gboolean strong);

static void kf_gui_init_internet_menu (GladeXML *glade);
static void connect (GladeXML *glade, const gchar *name, gpointer func, gpointer user_data);
void        on_popup_presence_activate    (GtkMenuItem *menuitem,
                                            gpointer user_data);

static void kf_gui_alert_cb                (GtkDialog *dialog,
                                            gint arg1,
                                            gpointer user_data);
static void groups_foreach                  (gpointer key,
                                             gpointer value,
                                             GList **user_data);

KfGuiRosterGroup *kf_gui_roster_groups_get_for_null (void);

static gboolean motion_notify_cb (GtkWidget *tv, GdkEventMotion *event, gpointer data);
static gboolean leave_notify_cb (GtkWidget *tv, GdkEventCrossing *event, gpointer data);
static gboolean tooltip_timeout_cb (gpointer data);

static GtkWidget *show_tooltip (KfJabberRosterItem *item);
static void hide_tooltip (GtkWidget *tooltip);
static gchar *tooltip_text (KfJabberRosterItem *item);

enum {
	NAME_COLUMN,
	JID_COLUMN,
	ITEM_COLUMN,
	N_COLS
};

static GHashTable *kf_gui_roster_groups = NULL;
static gchar *selected_jid = NULL;
static gboolean kf_gui_show_offline = TRUE;

GdkPixbuf *pixmaps[10];
GdkPixbuf *pix;

static gint tooltip_timeout = -1;
static GtkWidget *tooltip_widget = NULL;

static gchar *kf_gui_roster_colors[] = {
	"#0066cc",
	"#0099cc",
	"#004d80",
	"#004d4d",
	"#666633",
	"#666666",
	"#666666"
};

typedef struct {
	gchar *title;
	gchar *url;		/* Set to NULL to get separator */
	gboolean translatable;	/* Whether to translate title through gettext */
} KfUrl;

/* Links to put in menu */
KfUrl kf_urls[]  = {
	{N_("kf homepage"), "http://www.habazie.rams.pl/kf", TRUE},
	{N_("kf bug tracker"), "http://www.habazie.rams.pl/flyspray", TRUE},
	{"---",NULL,FALSE},
	{"JabberPL", "http://www.jabberpl.org",FALSE},
	{"Jabber.org", "http://www.jabber.org",FALSE},
	{NULL,NULL,FALSE}};

static gboolean *enable_roster_status;

gboolean kf_gui_init (void) {
	GtkWidget *tv, *toolbar, *win, *toggle, *notebook;
	
	iface = glade_xml_new (kf_find_file ("kf.glade"), NULL, NULL);
	if (iface == NULL) {
		g_error ("Nie ma interfejsu!!!\n");
	}

	tv = glade_xml_get_widget (iface, "roster_tree_view");
	kf_gui_setup_treeview (GTK_TREE_VIEW (tv));
	kf_gui_pixmap_init ();
	kf_chat_init ();
	kf_message_init ();
	kf_dock_init ();
	kf_gui_init_internet_menu (iface);

	kf_autoaway_init ();

	/* Setup interface */
	toolbar = glade_xml_get_widget (iface, "toolbar1");
	toggle = glade_xml_get_widget (iface, "menu_show_toolbar");
	if (!kf_preferences_get_int ("showToolbar")) {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (toggle), FALSE);
//		gtk_widget_hide (toolbar);
	} else {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (toggle), TRUE);
//		gtk_widget_show (toolbar);
	}
	kf_gui_show_offline = kf_preferences_get_int ("showOffline");

	if ((win = glade_xml_get_widget (iface, "main_window"))) {
		gint posX, posY, sizeX, sizeY;
		
		posX = kf_preferences_get_int ("posX");
		posY = kf_preferences_get_int ("posY");
		sizeX = kf_preferences_get_int ("sizeX");
		sizeY = kf_preferences_get_int ("sizeY");

		gtk_window_move (GTK_WINDOW (win), posX, posY);
		if (sizeX > 1 && sizeY > 1)
			gtk_window_resize (GTK_WINDOW (win), sizeX, sizeY);

		kf_gui_show (win);
	}	
	glade_xml_signal_autoconnect (iface);	

	enable_roster_status = kf_preferences_get_ptr ("rosterShowStatus");
	
	notebook = glade_xml_get_widget (iface, "roster_notebook");
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), kf_preferences_get_int ("tabPosition"));

	/* Connecxt signals in "Presence" popup menu */
	connect (iface, "popup_presence_online",  on_popup_presence_activate, NULL);
	connect (iface, "popup_presence_invisible",  on_popup_presence_activate, "invisible");
	connect (iface, "popup_presence_ask",  on_popup_presence_activate, "subscribe");
	connect (iface, "popup_presence_give",  on_popup_presence_activate, "subscribed");
	connect (iface, "popup_presence_deny",  on_popup_presence_activate, "unsubscribed");

	if (kf_preferences_get_int ("expertMode")) {
		GtkWidget *menu_xml;

		menu_xml = glade_xml_get_widget (iface, "menu_xml");
		if (menu_xml)
			gtk_widget_show (menu_xml);
		else
			foo_debug ("Nie ma menu_xml\n");
	}

	/* Use roster iconsets */
	update_menu_images (iface, "status");
	update_menu_images (kf_dock_glade (), "dock");

	return TRUE;
}

/* Creates Jabber->Internet submenu containing internet links */
static void kf_gui_init_internet_menu (GladeXML *glade) {
	GtkWidget *parent;
	GtkWidget *menu;
	GtkWidget *item;
	gint index;

	menu = gtk_menu_new ();
	for (index = 0; kf_urls[index].title; index++) {
		GtkWidget *image;

		if (kf_urls[index].url) {
		image = gtk_image_new_from_file (kf_find_file ("www.png"));
		gtk_widget_show (image);

		if (kf_urls[index].translatable)
			item = gtk_image_menu_item_new_with_label (_(kf_urls[index].title));
		else
			item = gtk_image_menu_item_new_with_label (kf_urls[index].title);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
		g_signal_connect (G_OBJECT (item), "activate",
				G_CALLBACK (internet_menu_callback), kf_urls[index].url);
		} else {
			item = gtk_separator_menu_item_new ();
		}
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		gtk_widget_show (item);
	}
	gtk_widget_show (menu);
	parent = glade_xml_get_widget (glade, "internet1");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (parent), menu);
	
}

static void internet_menu_callback (GtkMenuItem *menuitem,
                                            gpointer user_data) {
	kf_www_open (user_data);
}

void kf_gui_shutdown (void) {
	gint posX, posY, sizeX, sizeY;
	GtkWidget *win;

	win = glade_xml_get_widget (iface, "main_window");

	gtk_window_get_position (GTK_WINDOW (win), &posX, &posY); 
	gtk_window_get_size (GTK_WINDOW (win), &sizeX, &sizeY); 

	kf_preferences_set_int ("posX", posX);
	kf_preferences_set_int ("posY", posY);
	kf_preferences_set_int ("sizeX", sizeX);
	kf_preferences_set_int ("sizeY", sizeY);
}

void kf_gui_show (GtkWidget *wid) {
	gtk_widget_show (wid);
}

void kf_gui_show_connection_settings (void) {
	GtkWidget *cs; /* Connection Settings window */
	GtkWidget *menu;
	GtkWidget *item;
	extern GList *kf_preferences_accounts;
	GList *account;
	KfPrefAccount *acc;
	GtkOptionMenu *optmenu;

	optmenu = GTK_OPTION_MENU (glade_xml_get_widget (iface, "connection_konto_selector"));
	if (!optmenu)
		g_error ("Nie ma menu!\n");
	if ((menu = gtk_option_menu_get_menu (optmenu))) {
		gtk_widget_destroy (menu);
	}

	menu = gtk_menu_new ();
	item = gtk_menu_item_new_with_label ("(brak)");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate",
			G_CALLBACK (kf_gui_select_account), NULL);
	gtk_widget_show (item);
	for (account = kf_preferences_accounts; account; account = account->next) {
		acc = (KfPrefAccount *) account->data;

		item = gtk_menu_item_new_with_label (acc->name);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate",
				G_CALLBACK (kf_gui_select_account), acc);
		gtk_widget_show (item);
	}

	gtk_option_menu_set_menu (optmenu, GTK_WIDGET (menu));
	gtk_widget_show (menu);

	cs = glade_xml_get_widget (iface, "connection_settings_window");
	kf_gui_show (cs);
}

void  kf_gui_select_account                (GtkMenuItem *menuitem,
                                            gpointer user_data) {
	KfPrefAccount *acc;
	GtkWidget *entry;

	if (user_data) {
		acc = (KfPrefAccount *) user_data;
		entry = glade_xml_get_widget (iface, "connection_username");
		gtk_entry_set_text (GTK_ENTRY (entry), acc->uname);
		entry = glade_xml_get_widget (iface, "connection_server");
		gtk_entry_set_text (GTK_ENTRY (entry), acc->server);
		entry = glade_xml_get_widget (iface, "connection_password");
		gtk_entry_set_text (GTK_ENTRY (entry), acc->pass);
	}
}
void kf_gui_setup_treeview (GtkTreeView *tv) {
	GtkTreeStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;

	pix = gdk_pixbuf_new_from_file (kf_find_file ("online.png"), NULL);
	store = gtk_tree_store_new (N_COLS,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_POINTER);

	/* Ustawienie GtkTreeView */	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, on_pixbuf_data, NULL, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, on_text_data, NULL, NULL);
	gtk_tree_view_append_column (tv, column);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	gtk_tree_view_set_model (tv, GTK_TREE_MODEL (store));

	kf_gui_roster_groups = g_hash_table_new (g_str_hash, g_str_equal);

	/* Setup the selection handler */
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
                  G_CALLBACK (tree_selection_changed_cb),
                  NULL);

	/*
	g_object_set (G_OBJECT (tv), "rules-hint",
			(gboolean) kf_preferences_get_int ("colourRows"), NULL);
	*/

	gtk_tree_view_set_enable_search (tv, TRUE);
	gtk_tree_view_set_search_column (tv, NAME_COLUMN);
	gtk_tree_view_set_search_equal_func (tv, kfGtkTreeViewSearchEqualFunc, NULL, NULL);

	g_signal_connect (G_OBJECT (tv), "motion-notify-event", G_CALLBACK (motion_notify_cb), NULL);
	g_signal_connect (G_OBJECT (tv), "leave-notify-event", G_CALLBACK (leave_notify_cb), NULL);
}

gboolean    kfGtkTreeViewSearchEqualFunc   (GtkTreeModel *model,
                                             gint column,
                                             const gchar *key,
                                             GtkTreeIter *iter,
                                             gpointer search_data)
{
	gchar *name = NULL;
	gboolean equal;
	gint len;

	len = strlen (key);
	gtk_tree_model_get (model, iter, NAME_COLUMN, &name, -1);
	if (name == NULL)
		return TRUE;
	equal = strncmp (key, name, len);
	g_free (name);
	return equal;
}


void kf_gui_update_roster (void) {
	gint current;				/* Aktualna pozycja w rosterze */
	extern GPtrArray *kf_jabber_roster;	/* Tablica zawieraj±ca roster */
	KfJabberRosterItem *item;		/* Element Rostera */
	GtkTreeStore *store;
	GtkTreeView *tv;
	GtkTreeIter group_iter;
	GList *groups = NULL;			/* Lista wszystkich grup */
	GList *groups_item = NULL;		/* Pojedynczy element grupy */
	GList *item_groups = NULL;
	KfGuiRosterGroup *group;		/* Pojedynczy element grupy */
	gchar *group_name;	
		
	tv = GTK_TREE_VIEW (glade_xml_get_widget (iface, "roster_tree_view"));
	store = GTK_TREE_STORE (gtk_tree_view_get_model (tv));

	g_hash_table_foreach (kf_gui_roster_groups, roster_groups_clear, NULL);
	group = kf_gui_roster_groups_get_for_null ();
	roster_groups_clear (NULL, group, NULL);

	/* Pobranie wszystkich elementów rostera */
	for (current = 0; current < kf_jabber_roster->len; current++) {
		item = g_ptr_array_index (kf_jabber_roster, current);
		if (item == NULL)
			continue;

		item_groups = item->groups;
		do {
			//group_name = item_groups?(item_groups->data):NO_GROUP;

			if (item_groups == NULL) {
				group = kf_gui_roster_groups_get_for_null ();
			} else {
				group_name = item_groups->data;
				if ((group = g_hash_table_lookup (kf_gui_roster_groups, group_name)) == NULL) {
					group = kf_gui_group_new (group_name);
					g_hash_table_insert (kf_gui_roster_groups, group_name, group);
				}
			}

			(group->total)++;
			if (item->presence !=  KF_JABBER_PRESENCE_TYPE_UNAVAILABLE)
				group->available++;
			if (item->presence !=  KF_JABBER_PRESENCE_TYPE_UNAVAILABLE
					|| item->type == KF_JABBER_CONTACT_TYPE_AGENT
					|| kf_x_event_last (item->jid) != -1)
				group->visible++;

			
			if (g_list_index (groups, group) == -1)
				groups = g_list_append (groups, group);

			if (g_list_index (group->items, item) == -1)
				group->items = g_list_append (group->items, item);
			if (item_groups) item_groups = item_groups->next;
		} while (item_groups);
		
	}

	groups = g_list_sort (groups, kf_gui_roster_compare_callback);
	groups = g_list_first (groups);
	gtk_tree_store_clear (store);
	while (groups) {
		group = groups->data;
		if (kf_gui_show_offline || group->visible > 0) {
			kf_gui_roster_add_group (store, &group_iter, group);
		} else {
			groups = groups->next;
			continue;
		}
		groups_item = group->items;

		while (groups_item) {
			item = groups_item->data;
			kf_gui_roster_add_item (store, &group_iter, item);
			groups_item = groups_item->next;
		}
		g_list_free (groups_item);
		group->items = NULL;
		
		/* Rozwiniêcie wiersza */
		if (group->expanded) {
			GtkTreePath *path;

			path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &group_iter);
			gtk_tree_view_expand_row (tv, path, FALSE);
			gtk_tree_path_free (path);
		}
		
		groups = groups->next;
	}
	g_list_free (groups);
}

static void        roster_groups_clear      (gpointer key,
                                             gpointer value,
                                             gpointer user_data) {
	KfGuiRosterGroup *group = value;

	group->total = 0;
	group->visible = 0;
	group->available = 0;
}

static gint kf_gui_roster_compare_callback   (gconstpointer a,
                                             gconstpointer b)
{
	const KfGuiRosterGroup *aa = a, *bb = b;

	if (aa->name == NULL) {
		return 1;
	} else if (bb->name == NULL) {
		return -1;
	} else {
		return strcmp (aa->name, bb->name);
	}
}

KfGuiRosterGroup *kf_gui_group_new (const gchar *name) {
	KfGuiRosterGroup *group;

	group = (KfGuiRosterGroup *) g_malloc (sizeof (KfGuiRosterGroup));

	
	group->name = ((name)?(g_strdup (name)):(NULL));
	group->items = NULL;
	group->expanded = ! kf_pref_group_is_collapsed (name);

	group->available = 0;
	group->visible = 0;
	group->total = 0;

	return group;
}


static void kf_gui_roster_add_group (GtkTreeStore *store, GtkTreeIter *iter, KfGuiRosterGroup *grp)
{
	gtk_tree_store_append (store, iter, NULL);
	gtk_tree_store_set (store, iter,
			NAME_COLUMN, grp->name,
			JID_COLUMN, NULL,
			ITEM_COLUMN, grp, -1);
}

static void kf_gui_roster_add_item (GtkTreeStore *store, GtkTreeIter *parent, KfJabberRosterItem *item)
{
	if (item->presence != KF_JABBER_PRESENCE_TYPE_UNAVAILABLE
			|| kf_gui_show_offline
			|| item->type == KF_JABBER_CONTACT_TYPE_AGENT
			|| kf_x_event_last (item->jid) != -1) {
		GtkTreeIter child;
		gtk_tree_store_append (store, &child, parent);
		gtk_tree_store_set (store, &child,
				JID_COLUMN, item->jid,
				NAME_COLUMN, item->display_name,
				ITEM_COLUMN, item,
				-1);
	}
}

//void kf_gui_roster_item_update (guint n, const gchar *name, const gchar *status) {
//	extern GPtrArray *kf_jabber_roster;	/* Tablica zawieraj±ca roster */
//	KfJabberRosterItem *item;		/* Element Rostera */
//	KfJabberRosterItem *found;
//	guint current;
//	
//	item = g_ptr_array_index (kf_jabber_roster, n);
//}

void kf_gui_message_normal_display (const gchar *from, const gchar *subject, const gchar *body) {
	GtkWidget *win;
	GtkWidget *label;
	GtkWidget *textview;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;

	win = glade_xml_get_widget (iface, "message_recv_window");

	label = glade_xml_get_widget (iface, "recv_from_label");
	gtk_label_set_text (GTK_LABEL (label), from);
	label = glade_xml_get_widget (iface, "recv_subject_label");
	gtk_label_set_text (GTK_LABEL (label), subject?subject:"(brak tematu)");
	
	textview = glade_xml_get_widget (iface, "recv_msg_textview");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_delete (buffer, &start, &end);
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_insert (buffer, &start, body, -1);
	
	kf_gui_show (win);
}

static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *jid;

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
		KfJabberRosterItem *item;
		gtk_tree_model_get (model, &iter, JID_COLUMN, &jid, ITEM_COLUMN, &item, -1);

		if (jid)
			selected_jid = item->fulljid;
		else
			selected_jid = NULL;
        }
}

const gchar *kf_gui_get_selected_jid (void) {
	return selected_jid;
}

static
void on_pixbuf_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gchar *jid=NULL;
	KfJabberRosterItem *item;
	gtk_tree_model_get (model, iter, JID_COLUMN, &jid, ITEM_COLUMN, &item, -1);

	if (jid) {
		g_object_set (G_OBJECT (renderer), "visible", TRUE, NULL);
		kf_gui_roster_set_pixbuf (renderer, item);
	} else {
		g_object_set (G_OBJECT (renderer), "visible", FALSE, NULL);
		g_object_set (G_OBJECT (renderer), "pixbuf", NULL, NULL);
	}
}

static
void on_text_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gchar *name=NULL, *jid=NULL;
	gtk_tree_model_get (model, iter, NAME_COLUMN, &name, JID_COLUMN, &jid, -1);

	if (jid && TRUE) {
		KfJabberRosterItem *item;

		gtk_tree_model_get (model, iter, ITEM_COLUMN, &item, -1);
		if (item) {
			/* Nowy engine kolorowych statusów */
			gchar *text;
			guint nick_len, status_len;
			PangoColor color;
			PangoAttribute *foreground, *size;
			PangoAttrList *font_attr;

			nick_len = strlen (item->display_name);
			status_len = item->status?strlen (item->status):0;
			
//			if (time () % 2)
				pango_color_parse (&color, kf_gui_roster_colors[item->presence]);
//			else
//				pango_color_parse (&color, "orange");

			foreground = pango_attr_foreground_new (color.red, color.green, color.blue);
			foreground->start_index = 0;
			foreground->end_index = nick_len;

			font_attr = pango_attr_list_new ();
			pango_attr_list_insert (font_attr, foreground);
			
			text = item->display_name;
			if (item->status && *enable_roster_status) {
				PangoAttribute *gray;
				PangoAttribute *italic;
				
				size = pango_attr_scale_new (PANGO_SCALE_SMALL);
				size->start_index = nick_len;	/* +1 for '\n'; */
				size->end_index = nick_len + status_len + 1;

				/* Gray color */
				gray = pango_attr_foreground_new (0x6666, 0x6666, 0x6666);
				gray->start_index = nick_len;
				gray->end_index = nick_len + status_len + 1;

				italic = pango_attr_style_new (PANGO_STYLE_ITALIC);
				italic->start_index = nick_len;
				italic->end_index = nick_len + status_len + 1;
				
				pango_attr_list_insert (font_attr, size);
				pango_attr_list_insert (font_attr, gray);
				pango_attr_list_insert (font_attr, italic);
				
				text = g_strdup_printf ("%s\n%s", item->display_name, item->status);
			}

			g_object_set (G_OBJECT (renderer),
					"text",		text,
					"attributes",	font_attr,
//					"ellipsize",	PANGO_ELLIPSIZE_END,
					NULL);

			pango_attr_list_unref (font_attr);
			if (item->status && *enable_roster_status) {
				g_free (text);
			}
		}
	} else {
		KfGuiRosterGroup *group;
		const gchar *name;
		gchar *text;
		gint name_len;
		gint text_len;
		static gchar *no_group = NULL;

		PangoColor color;
		PangoAttribute *bold, *colour, *size;
		PangoAttrList *font;

		if (no_group == NULL)
			no_group = _("General");

		gtk_tree_model_get (model, iter, ITEM_COLUMN, &group, -1);
//		g_object_set (G_OBJECT (renderer), "text", group->name, "attributes", NULL, NULL);
		name = group->name?group->name:no_group;
		text = g_strdup_printf ("%s (%d/%d)", name,
				group->available, group->total);

		name_len = strlen (name);
		text_len = strlen (text);
		
		pango_color_parse (&color, "navy");
		bold = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
		bold->start_index = 0;
		bold->end_index = name_len;
		colour = pango_attr_foreground_new (color.red, color.green, color.blue);
		colour->start_index = name_len+1;
		colour->end_index = text_len;
		size = pango_attr_scale_new (PANGO_SCALE_SMALL);
		size->start_index = name_len+1;
		size->end_index = text_len;
		font = pango_attr_list_new ();
		pango_attr_list_insert (font, bold);
		pango_attr_list_insert (font, colour);
		pango_attr_list_insert (font, size);

		g_object_set (G_OBJECT (renderer), "text", text, "attributes", font, NULL);
		g_free (text);
		pango_attr_list_unref (font);
		//g_object_set (G_OBJECT (renderer), "markup", name, NULL);
	}
	
/*	if (!jid)
		g_object_set (G_OBJECT (renderer), "font", "bold", NULL);
	else
		g_object_set (G_OBJECT (renderer), "font", "normal", NULL);*/
}

static void kf_gui_roster_set_pixbuf (GtkCellRenderer *renderer, KfJabberRosterItem *item) {
	GdkPixbuf *pixb;
	gint index;
			
	if ((index = kf_x_event_last (item->jid)) != -1 && time(NULL) % 2 == 1) {
		pixb = kf_gui_pixmap_get_status (index + 8);
	} else {
		if (item->subscription == KF_JABBER_SUBSCRIPTION_TYPE_TO ||
				item->subscription == KF_JABBER_SUBSCRIPTION_TYPE_BOTH) {
			if (item->presence != KF_JABBER_PRESENCE_TYPE_UNAVAILABLE) {

					pixb = kf_gui_pixmap_get_status (item->presence);
						
			} else {
				pixb = NULL;
			}
		} else {
			pixb = kf_gui_pixmap_get_status (7);
		}
	}
	g_object_set (G_OBJECT (renderer), "pixbuf", pixb, NULL);
}
/*
void kf_gui_update_status_x (KfJabberRosterItem *item) {
	GtkTreeIter iter;

	if (!kf_gui_show_offline)
		kf_gui_update_roster ();
	if (kf_gui_roster_get_iter (&iter, item)) {
		GtkTreeStore *store;
		GtkTreeView *tv;
	
		tv = GTK_TREE_VIEW (glade_xml_get_widget (iface, "roster_tree_view"));
		store = GTK_TREE_STORE (gtk_tree_view_get_model (tv));

		gtk_tree_store_set (store, &iter, NAME_COLUMN, item->display_name, -1);
	} else {
	}
}
*/
gboolean kf_gui_roster_get_iter (GtkTreeIter *iter, KfJabberRosterItem *item) {
	GtkTreeStore *store;
	GtkTreeView *tv;
	gboolean x, y;
	KfJabberRosterItem *item2;
	GtkTreeIter group_iter;

//	if (!kf_gui_show_offline)
//		kf_gui_update_roster ();

	tv = GTK_TREE_VIEW (glade_xml_get_widget (iface, "roster_tree_view"));
	store = GTK_TREE_STORE (gtk_tree_view_get_model (tv));

	x = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &group_iter);
	while (x) {
		y = gtk_tree_model_iter_children (GTK_TREE_MODEL (store), iter, &group_iter);
		while (y) {
			gtk_tree_model_get (GTK_TREE_MODEL (store), iter, ITEM_COLUMN, &item2, -1);
			if (!strcmp (item->jid, item2->jid))
				return TRUE;
			y = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter);
		}
		x = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &group_iter);
	}
	return FALSE;
}

void kf_gui_update_status (KfJabberRosterItem *item, gboolean strong) {
	GtkTreeIter iter;
	GtkTreeStore *store;
	GtkTreeView *tv;
	gboolean x, y;
	KfJabberRosterItem *item2;
	GtkTreeIter group_iter;

	g_return_if_fail (item);
	
	if (!kf_gui_show_offline && strong)
		kf_gui_update_roster ();
	
	tv = GTK_TREE_VIEW (glade_xml_get_widget (iface, "roster_tree_view"));
	store = GTK_TREE_STORE (gtk_tree_view_get_model (tv));

	x = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &group_iter);
	while (x) {
		y = gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &iter, &group_iter);
		while (y) {
			gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, ITEM_COLUMN, &item2, -1);
			if (!strcmp (item->jid, item2->jid))
//				return TRUE;
				gtk_tree_store_set (store, &iter, NAME_COLUMN, item->display_name, -1);
	
			y = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
		}
		x = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &group_iter);
	}
}

void kf_update_status_by_jid (const gchar *jid, gboolean strong) {
	KfJabberRosterItem *item;

	item = kf_jabber_roster_item_get (jid);
	if (item == NULL) {
		extern GPtrArray *kf_jabber_roster;
		item = kf_jabber_roster_item_new (jid, NULL);
		g_ptr_array_add (kf_jabber_roster, item);
		item->in_roster = FALSE;

		kf_gui_update_roster ();
	}
	kf_gui_update_status (item, strong);
}

void        on_button_toggle_show_inactive_toggled      (GtkToggleButton *togglebutton,
                                            gpointer user_data) {
	kf_gui_toggle_show_offline ();
}


/*
   OBSOLETE!!!
const gchar *kf_gui_find_file (const gchar *name) {
	static gchar text[1024];

	g_snprintf (text, 1024, "../data/%s", name);
	if (g_file_test (text, G_FILE_TEST_EXISTS))
		return text;

	g_snprintf (text, 1024, "%s/kf/%s", PACKAGE_DATA_DIR, name);
	return text;
}
*/

void kf_gui_disconnected (gboolean error) {
	GtkTreeView *tv;
	GtkTreeStore *store;

	tv = GTK_TREE_VIEW (glade_xml_get_widget (iface, "roster_tree_view"));
	store = GTK_TREE_STORE (gtk_tree_view_get_model (tv));

	gtk_tree_store_clear (store);

	if (error) {
		GtkWidget *win;
		
		win = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
				_("You have been disconnected..."));
		gtk_dialog_run (GTK_DIALOG (win));
		gtk_widget_destroy (win);
	}

	kf_status_changed (KF_JABBER_PRESENCE_TYPE_UNAVAILABLE, _("Disconnected..."));
}

void on_roster_tree_view_row_expanded      (GtkTreeView *treeview,
                                            GtkTreeIter *arg1,
                                            GtkTreePath *arg2,
                                            gpointer user_data)
{
	GtkTreeModel *store;
	KfGuiRosterGroup *group;
	
	store = gtk_tree_view_get_model (treeview);
	gtk_tree_model_get (store, arg1, ITEM_COLUMN, &group, -1);

	group->expanded = TRUE;
	kf_pref_group_set_collapsed (group->name, FALSE);
}


void on_roster_tree_view_row_collapsed     (GtkTreeView *treeview,
                                            GtkTreeIter *arg1,
                                            GtkTreePath *arg2,
                                            gpointer user_data)
{
	GtkTreeModel *store;
	KfGuiRosterGroup *group;
	
	store = gtk_tree_view_get_model (treeview);
	gtk_tree_model_get (store, arg1, ITEM_COLUMN, &group, -1);

	group->expanded = FALSE;
	kf_pref_group_set_collapsed (group->name, TRUE);
}

gboolean kf_gui_toggle_show_offline (void) {
	kf_gui_show_offline = !kf_gui_show_offline;
	kf_gui_update_roster ();
	kf_preferences_set_int ("showOffline", kf_gui_show_offline);
	return kf_gui_show_offline;
}

gboolean kf_gui_set_show_offline (gboolean show) {
	if (kf_gui_show_offline != show) {
		kf_gui_show_offline = show;
		kf_gui_update_roster ();
		kf_preferences_set_int ("showOffline", kf_gui_show_offline);
	}
	return kf_gui_show_offline;
}

static void connect (GladeXML *glade, const gchar *name, gpointer func, gpointer user_data) {
	GtkWidget *widget;

	widget = glade_xml_get_widget (glade, name);
	g_signal_connect (G_OBJECT (widget), "activate",
			G_CALLBACK (func), user_data);
}

void kf_gui_alert (const gchar *text) {
		GtkWidget *win;
		
		win = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
				text);

		g_signal_connect (G_OBJECT (win), "response", G_CALLBACK (kf_gui_alert_cb), NULL);

		gtk_widget_show (win);
		
//		gtk_dialog_run (GTK_DIALOG (win));
//		gtk_widget_destroy (win);
}

static void kf_gui_alert_cb                (GtkDialog *dialog,
                                            gint arg1,
                                            gpointer user_data) {
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

GList *kf_gui_get_groups (void) {
	GList *list = NULL;

//	list = g_list_prepend (list, "");
	
	g_hash_table_foreach (kf_gui_roster_groups, (GHFunc) groups_foreach, &list);

	return list;
}

static void groups_foreach                  (gpointer key,
                                             gpointer value,
                                             GList **list)
{
	KfGuiRosterGroup *group;

	group = value;

	if (group->name) {
		*list = g_list_prepend (*list, group->name);
	}
	
}

KfGuiRosterGroup *kf_gui_roster_groups_get_for_null (void) {
	static KfGuiRosterGroup *group = NULL;

	if (group == NULL)
		group = kf_gui_group_new (NULL);

	return group;
}



/* Tooltip */

static gint tooltip_x, tooltip_y;
static GdkRectangle tooltip_cell;

static gboolean motion_notify_cb (GtkWidget *tv, GdkEventMotion *event, gpointer data) {
	if (tooltip_timeout >= 0) {
		/* Remove old timeout */
		gtk_timeout_remove (tooltip_timeout);
		tooltip_timeout = -1;
	}
	if (tooltip_widget && (event->y < tooltip_cell.y || event->y > tooltip_cell.y + tooltip_cell.height)) {
		/* Pointer has left the list item */
		hide_tooltip (tooltip_widget);
		tooltip_widget = NULL;
	}
	
//	tooltip_timeout = gtk_timeout_add (kf_preferences_get_int ("tooltipDelay"),
	tooltip_timeout = gtk_timeout_add (800,
				tooltip_timeout_cb, tv);
	tooltip_x = event->x;
	tooltip_y = event->y;
	return FALSE;
}


static gboolean leave_notify_cb (GtkWidget *tv, GdkEventCrossing *event, gpointer data) {
//	foo_debug ("leave-notify\n");
	if (tooltip_timeout >= 0) {
		gtk_timeout_remove (tooltip_timeout);
		tooltip_timeout = -1;
	}
	if (tooltip_widget) {
		hide_tooltip (tooltip_widget);
		tooltip_widget = NULL;
	}
	return FALSE;
}


static gboolean tooltip_timeout_cb (gpointer data) {
	gint x, y;
	gint tx, ty;
	GtkTreePath *path;
	foo_debug ("timeout\n");

	if (tooltip_widget) {
		hide_tooltip (tooltip_widget);
		tooltip_widget = NULL;
	}
	x = tooltip_x;
	y = tooltip_y;

	gtk_tree_view_widget_to_tree_coords (GTK_TREE_VIEW (data), x,y, &tx,&ty);
//	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (data), tx, ty, &path, NULL, NULL, NULL)) {
	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (data), x, y, &path, NULL, NULL, NULL)) {
		GtkTreeIter iter;
		GtkTreeModel *model;
		KfJabberRosterItem *item = NULL;
		gchar *jid = NULL;

		gtk_tree_view_get_cell_area (GTK_TREE_VIEW (data), path, NULL, &tooltip_cell);

		model = gtk_tree_view_get_model (GTK_TREE_VIEW (data));
		if (!gtk_tree_model_get_iter (model, &iter, path))
			return FALSE;

		gtk_tree_model_get (model, &iter, 1, &jid, 2, &item, -1);
		if (jid && item) {
			tooltip_widget = show_tooltip (item);
		}
	}

	return FALSE;
}

static GtkWidget *show_tooltip (KfJabberRosterItem *item) {
	GtkWidget *tooltip;
	gint x, y, tx, ty;
	gchar *text;

	gdk_window_get_pointer (NULL, &x, &y, NULL);
	text = tooltip_text (item);
	tooltip = kf_tip_new (kf_gui_pixmap_get_status (item->presence), text);
	gtk_tip_get_size (GTK_TIP (tooltip), &tx, &ty);
	if (x > gdk_screen_width () - tx - 16)
		x = gdk_screen_width () - tx - 16;
	else
		x = x - 16;
	gtk_window_move (GTK_WINDOW (tooltip), x, y + 16);
	gtk_widget_show (tooltip);
	g_free (text);
	return tooltip;
}

static void hide_tooltip (GtkWidget *tooltip) {
	gtk_widget_destroy (tooltip);
}

static gchar *tooltip_text (KfJabberRosterItem *item) {
	gchar *text;
	gchar *status;

	status = g_markup_escape_text (
			item->status?item->status:kf_jabber_presence_type_to_human_string (item->presence), -1);

	text = g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>\n"
				"<b>JID:</b> %s\n"
				"<b>Status:</b> %s",
				item->display_name,
				item->fulljid,
				status);
	g_free (status);

	return text;
}


void kf_app_hide (void) {
	GtkWidget *window = glade_xml_get_widget (iface, "main_window");
	gint x, y;

	gtk_window_get_position (GTK_WINDOW (window), &x, &y);
	kf_preferences_set_int ("posX", x);
	kf_preferences_set_int ("posY", y);

	gtk_widget_hide (window);
}


void kf_app_unhide (void) {
	GtkWidget *window = glade_xml_get_widget (iface, "main_window");
	gint x, y;

	x = kf_preferences_get_int ("posX");
	y = kf_preferences_get_int ("posY");
	gtk_widget_show (window);
	gtk_window_move (GTK_WINDOW (window), x, y);
}
