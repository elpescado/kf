/* file settings.c */
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
#include "settings.h"
#include "autoaway.h"
#include "settings_statuses.h"
#include "accounts.h"
#include "settings_blacklist.h"
#include "settings_icons.h"
#include "gui.h"
#include "dock.h"
#include "emoticons.h"

typedef struct {
	gchar *image;
	gchar *name;
	gint page;

	GdkPixbuf *pixbuf;
	gchar *tname;
} KfSettingsPage;

static KfSettingsPage kf_settings_pages[] = {
	{"pix_chat.png", N_("General"), 0},
	{NULL, N_("Conversations"), 1},
	{NULL, N_("Events"), 9},
//	{NULL, "Contact List", 9},
	{"online.png", N_("Accounts"), 2},
	{"xa.png", N_("AutoAway"), 3},
	{NULL, N_("Statuses"), 4},
	{NULL, N_("Icons"), 10},
	{NULL, N_("Sounds"), 5},
	{NULL, N_("Block List"), 6},
	{NULL, N_("Privacy"), 7},
	{NULL, N_("Advanced"), 8},
	{NULL, NULL, -1}
};

static void        kf_settings_toggle             (GtkToggleButton *togglebutton,
                                            gpointer user_data);
static void        kf_settings_changed     (GtkEditable *editable,
                                            gpointer user_data);
static void        kf_settings_spin_changed(GtkSpinButton *togglebutton,
                                            gpointer user_data);
static void        kf_settings_text_changed     (GtkTextBuffer *buffer,
                                            gpointer user_data);
static void kf_settings_connect (GladeXML *glade, const gchar *wname, gchar *option);
static void kf_settings_connect_entry (GladeXML *glade, const gchar *wname, gchar *option);
static void kf_settings_connect_spin (GladeXML *glade, const gchar *wname, gchar *option);
static void kf_settings_connect_textview (GladeXML *glade,  const gchar *wname, gchar *option);

static void kf_settings_setup_treeview (GtkTreeView *tv, GladeXML *glade);
static void
on_pixbuf_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void
on_text_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);

static void connect_file (GladeXML *glade, const gchar *entry, const gchar *button);
static void file_selector_cb (GtkButton *button, gpointer data);
static void file_selector_ok_cb (GtkButton *button, gpointer data);
static void file_selector_cancel_cb (GtkButton *button, gpointer data);
static void update_sample_conversation (GladeXML *glade);
static void        dock_enable_toggled     (GtkToggleButton *togglebutton,
                                            gpointer user_data);
static gboolean kf_settings_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data);

/*
 * Callback dla pstryczka w okienku konfiguracji
 */
static void        kf_settings_toggle             (GtkToggleButton *togglebutton,
                                            gpointer user_data) {
	kf_preferences_set_int ((const gchar *) user_data, gtk_toggle_button_get_active (togglebutton));
	kf_preferences_write (kf_config_file ("config.xml"));
}

/*
 * Callback dla GtkEntry w okienku konfiguracji
 */
static void        kf_settings_changed     (GtkEditable *editable,
                                            gpointer user_data) {
	const gchar *text;

	text = gtk_entry_get_text (GTK_ENTRY (editable));
	
	kf_preferences_set ((const gchar *) user_data, text);
	kf_preferences_write (kf_config_file ("config.xml"));
}

/*
 * Callback dla GtkSpinButton w okienku konfiguracji
 */
static void        kf_settings_spin_changed(GtkSpinButton *togglebutton,
                                            gpointer user_data) {
	kf_preferences_set_int ((const gchar *) user_data, gtk_spin_button_get_value_as_int (togglebutton));
	kf_preferences_write (kf_config_file ("config.xml"));
}

static void        kf_settings_text_changed     (GtkTextBuffer *buffer,
                                            gpointer user_data) {
	gchar *text;
	GtkTextIter start, end;

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	
	kf_preferences_set ((const gchar *) user_data, text);
	kf_preferences_write (kf_config_file ("config.xml"));
	g_free (text);
}



void kf_settings (void) {
	GtkWidget *win;
	GtkWidget *treeview;
	extern GladeXML *iface;
	static gint x = 0;

	win = glade_xml_get_widget (iface, "preferences");

	if (x == 0) {
		GtkWidget *label;
		PangoFontDescription *font;
		GtkWidget *check;

		label = glade_xml_get_widget (iface, "preferences_page_title");
		font = pango_font_description_from_string ("bold");
		gtk_widget_modify_font (label, font);

		kf_settings_connect (iface, "checkbutton4", "useEmoticons");
		kf_settings_connect (iface, "radiobutton2", "messageDefault");
		kf_settings_connect (iface, "checkbutton5", "autoPopup");
		kf_settings_connect (iface, "checkbutton6", "soundEnabled");
		kf_settings_connect (iface, "checkbutton7", "tabDefault");
		kf_settings_connect (iface, "checkbutton8", "autoAwayEnabled");
		kf_settings_connect (iface, "checkbutton_tray", "minimizeToTrayOnClose");
		kf_settings_connect (iface, "pref_statuses", "rosterShowStatus");
		kf_settings_connect (iface, "pref_dock_enable", "dockEnable");
		kf_settings_connect_entry (iface, "preferences_browser", "browser");
		kf_settings_connect_entry (iface, "preferences_player", "soundPlayer");
		kf_settings_connect_spin (iface, "spinbutton2", "autoAwayDelay");
		kf_settings_connect_spin (iface, "spinbutton3", "autoXADelay");
		kf_settings_connect_textview (iface, "textview1", "autoAwayText");
		kf_settings_connect_textview (iface, "textview2", "autoXAText");
		
		kf_settings_connect (iface, "checkbutton_popup", "enablePopups");
		kf_settings_connect (iface, "checkbutton_popup_msg", "enablePopupMsg");
		kf_settings_connect (iface, "checkbutton_popup_chat", "enablePopupChat");
		kf_settings_connect (iface, "checkbutton_popup_online", "enablePopupPresence");
		kf_settings_connect (iface, "checkbutton_popup_offline", "enablePopupOffline");

		/* Sounds */
		kf_settings_connect (iface, "sound_presence_toggle", "soundPresenceUse");
		kf_settings_connect_entry (iface, "sound_presence", "soundPresence");
		kf_settings_connect (iface, "sound_offline_toggle", "soundOfflineUse");
		kf_settings_connect_entry (iface, "sound_offline", "soundOffline");
		kf_settings_connect (iface, "sound_chat_toggle", "soundChatUse");
		kf_settings_connect_entry (iface, "sound_chat", "soundChat");
		kf_settings_connect (iface, "sound_msg_toggle", "soundMsgUse");
		kf_settings_connect_entry (iface, "sound_msg", "soundMsg");

		/* TODO change these names... */
		connect_file (iface, "sound_presence", "button3");
		connect_file (iface, "sound_offline", "button6");
		connect_file (iface, "sound_chat", "button4");
		connect_file (iface, "sound_msg", "button5");
		
		kf_settings_connect (iface, "enable_archive", "enableMessageArchive");
		kf_settings_connect (iface, "log_messages", "logMessages");
		
		kf_settings_connect (iface, "pref_stamps", "chatShowStamps");
		kf_settings_connect (iface, "pref_irc_style", "chatIrcStyle");
		check = glade_xml_get_widget (iface, "checkbutton4");
		g_signal_connect_data (G_OBJECT (check), "toggled",
				G_CALLBACK (update_sample_conversation), iface, NULL,
				G_CONNECT_AFTER | G_CONNECT_SWAPPED);
		check = glade_xml_get_widget (iface, "pref_stamps");
		g_signal_connect_data (G_OBJECT (check), "toggled",
				G_CALLBACK (update_sample_conversation), iface, NULL,
				G_CONNECT_AFTER | G_CONNECT_SWAPPED);
		check = glade_xml_get_widget (iface, "pref_irc_style");
		g_signal_connect_data (G_OBJECT (check), "toggled",
				G_CALLBACK (update_sample_conversation), iface, NULL,
				G_CONNECT_AFTER | G_CONNECT_SWAPPED);
		
		check = glade_xml_get_widget (iface, "pref_dock_enable");
		g_signal_connect_data (G_OBJECT (check), "toggled",
				G_CALLBACK (dock_enable_toggled), iface, NULL,
				G_CONNECT_AFTER);
		
		check = glade_xml_get_widget (iface, "pref_statuses");
		g_signal_connect_data (G_OBJECT (check), "toggled",
				G_CALLBACK (kf_gui_update_roster), iface, NULL,
				G_CONNECT_AFTER);
		
//		kf_signal_connect (iface, "checkbutton8", "toggled",
//				G_CALLBACK (kf_settings_autoaway_callback), NULL);
//		kf_signal_connect (iface, "spinbutton2", "value-changed",
//				G_CALLBACK (kf_settings_autoaway_callback), NULL);
//		kf_signal_connect (iface, "spinbutton3", "value-changed",
//				G_CALLBACK (kf_settings_autoaway_callback), NULL);
		update_sample_conversation (iface);

		x = 1;

		g_signal_connect (G_OBJECT (win), "delete-event",
				G_CALLBACK (kf_settings_window_delete_event),
				NULL);
	}

	kf_accounts_editor (iface);
	kf_statuses_editor (iface);
	kf_blacklist_manager (iface);
	kf_jisp_manager (iface);
	treeview = glade_xml_get_widget (iface, "preferences_treeview");
	kf_settings_setup_treeview (GTK_TREE_VIEW (treeview), iface);
	
	kf_gui_show (win);
}

static void kf_settings_connect (GladeXML *glade, const gchar *wname, gchar *option) {
	GtkWidget *widget;

	widget = glade_xml_get_widget (glade, wname);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
			kf_preferences_get_int (option));
	g_signal_connect (GTK_OBJECT (widget), "toggled",
                     G_CALLBACK (kf_settings_toggle), option);
}

static void kf_settings_connect_entry (GladeXML *glade, const gchar *wname, gchar *option) {
	GtkWidget *widget;

	widget = glade_xml_get_widget (glade, wname);
	gtk_entry_set_text (GTK_ENTRY (widget),
			kf_preferences_get_string (option));
	g_signal_connect (GTK_OBJECT (widget), "changed",
                     G_CALLBACK (kf_settings_changed), option);
}

static void kf_settings_connect_spin (GladeXML *glade, const gchar *wname, gchar *option) {
	GtkWidget *widget;

	widget = glade_xml_get_widget (glade, wname);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
			(gdouble) kf_preferences_get_int (option));
	g_signal_connect (GTK_OBJECT (widget), "value-changed",
                     G_CALLBACK (kf_settings_spin_changed), option);
}

static void kf_settings_connect_textview (GladeXML *glade,  const gchar *wname, gchar *option) {
	GtkWidget *widget;
	GtkTextBuffer *buffer;

	widget = glade_xml_get_widget (glade, wname);
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	gtk_text_buffer_set_text (buffer, kf_preferences_get_string (option), -1);
	g_signal_connect (G_OBJECT (buffer), "changed",
                     G_CALLBACK (kf_settings_text_changed), option);
}

/*
static void kf_settings_autoaway_callback (GtkWidget *widget, gpointer data) {
	kf_autoaway_set (
		kf_preferences_get_int ("autoAwayEnabled"),	
		kf_preferences_get_int ("autoAwayDelay"),	
		kf_preferences_get_int ("autoXADelay"));	
}*/

static void kf_settings_setup_treeview (GtkTreeView *tv, GladeXML *glade) {
	GtkListStore *store;
	KfSettingsPage *page;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	static gboolean set = FALSE;
	

	if (set)
		return;

	store = gtk_list_store_new (1, G_TYPE_POINTER);
	for (page = kf_settings_pages; page->name; page++) {
		GtkTreeIter iter;
		page->pixbuf = page->image ? gdk_pixbuf_new_from_file (kf_find_file (page->image), NULL) : NULL;
		page->tname = _(page->name);

		gtk_list_store_append (store, &iter);  /* Acquire an iterator */

		gtk_list_store_set (store, &iter, 0, page, -1);
	}

		/* Ustawienie GtkTreeView */	
	column = gtk_tree_view_column_new ();
/*	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, on_pixbuf_data, NULL, NULL);*/
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, on_text_data, NULL, NULL);
	gtk_tree_view_append_column (tv, column);

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
                  G_CALLBACK (tree_selection_changed_cb),
                  glade_xml_get_widget (glade, "notebook1"));
	gtk_tree_view_set_model (tv, GTK_TREE_MODEL (store));
	set = TRUE;
}

/* This function displays a text in a TreeView */
static void
on_pixbuf_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
	KfSettingsPage *page;
	
	gtk_tree_model_get (tree_model, iter, 0, &page, -1);

	g_object_set (G_OBJECT (renderer), "pixbuf", page->pixbuf, NULL);

}

/* This function displays a text in a TreeView */
static void
on_text_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
	KfSettingsPage *page;
	
	gtk_tree_model_get (tree_model, iter, 0, &page, -1);

	g_object_set (G_OBJECT (renderer), "text", page->tname, NULL);

}

static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
 		KfSettingsPage *page;
		GtkNotebook *notebook = data;
		GtkWidget *label;
		GladeXML *glade;
	
		gtk_tree_model_get (model, &iter, 0, &page, -1);

		gtk_notebook_set_current_page (notebook, page->page);

		glade = glade_get_widget_tree (GTK_WIDGET (notebook));
		label = glade_xml_get_widget (glade, "preferences_page_title");
		gtk_label_set_text (GTK_LABEL (label), page->tname);
        }
}


/*
 * File selectors
 */

static void connect_file (GladeXML *glade, const gchar *entry, const gchar *button) {
	GtkWidget *e, *b;

	e = glade_xml_get_widget (glade, entry);
	b = glade_xml_get_widget (glade, button);
	g_signal_connect (GTK_OBJECT (b), "clicked", G_CALLBACK (file_selector_cb), e);
}

static void file_selector_cb (GtkButton *button, gpointer data) {
	GtkWidget *entry = data;
	GtkWidget *filesel;
	const gchar *file;
	

	filesel = gtk_file_selection_new (_("Choose file"));
	file = gtk_entry_get_text (GTK_ENTRY (entry));
	if (file && *file != '/') {
		file = kf_find_file (file);
	}
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), file);
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button), "clicked",
			G_CALLBACK (file_selector_ok_cb), data);
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button), "clicked",
			G_CALLBACK (file_selector_cancel_cb), data);
	gtk_widget_show (filesel);
}

static void file_selector_ok_cb (GtkButton *button, gpointer data) {
	GtkWidget *entry = data;

	gtk_entry_set_text (GTK_ENTRY (entry), 
		gtk_file_selection_get_filename (GTK_FILE_SELECTION (
				gtk_widget_get_toplevel (GTK_WIDGET (button)))));
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

static void file_selector_cancel_cb (GtkButton *button, gpointer data) {
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

static void update_sample_conversation (GladeXML *glade) {
	GtkWidget *view = glade_xml_get_widget (glade, "sample_conversation");
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	static KfEmoTagTable *style = NULL;

	if (style == NULL) {
		kf_emo_init_textview (GTK_TEXT_VIEW (view));
		style = g_new (KfEmoTagTable, 1);
		style->out = gtk_text_buffer_create_tag (buffer, "my_text",
			"pixels-above-lines", 4,
			"foreground", kf_preferences_get_string ("colorChatMe"),
			NULL);
		style->in = gtk_text_buffer_create_tag (buffer, "his_text",
			"pixels-above-lines", 4,
			"foreground", kf_preferences_get_string ("colorChatHe"),
			NULL);
	}

	gtk_text_buffer_set_text (buffer, "", 0);
	kf_display_message (buffer, 1103815567L, "Me", "Hello! ;-)", TRUE, style);
	kf_display_message (buffer, 1103815667L, "Elvis", "Hello!", FALSE, style);
}

static void        dock_enable_toggled     (GtkToggleButton *togglebutton,
                                            gpointer user_data) {
	if (gtk_toggle_button_get_active (togglebutton))
		kf_dock_show ();
	else
		kf_dock_hide ();
}


static gboolean kf_settings_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_widget_hide (widget);
	return TRUE;
}


