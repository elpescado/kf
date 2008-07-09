/* file callbacks.c */
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
#include "callbacks.h"
#include "jabber.h"
#include "gui.h"
#include "new_message.h"
#include "muc.h"
#include "muc_join.h"
#include "kf.h"
#include "preferences.h"
#include "settings.h"
#include "events.h"
#include "xevent.h"
#include "vcard.h"
#include "chat.h"
#include "archive_viewer.h"
#include "browse.h"
#include "search.h"
#include "register.h"
#include "popup_menu.h"
#include "change_password.h"	/* Almost all header files :-/ */

static void status_selected (GtkMenuItem *menuitem);
static void popup_menu_for_item (KfJabberRosterItem *item, GdkEventButton *event);
void kf_gui_add_contact_full (const gchar *fulljid,
		const gchar *name, const gchar *group);
static void invite_to_menu_cb (GtkMenuItem *menuitem, gpointer data);

gboolean    on_main_window_delete_event    (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data)
{
	if (kf_preferences_get_int ("minimizeToTrayOnClose")) {
		kf_app_hide ();
		return TRUE;
	} else {
		gtk_main_quit ();
		return TRUE;
	}
}

void        on_status_available_activate   (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_gui_change_status (NULL); 
}

void        on_status_away_activate        (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_gui_change_status ("away"); 
}

void        on_status_xa_activate          (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_gui_change_status ("xa"); 
}

void        on_status_chatty_activate      (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_gui_change_status ("chat"); 
}

void        on_status_dnd_activate         (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_gui_change_status ("dnd"); 
}

void        on_status_invisible_activate   (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_gui_change_status ("invisible"); 
}

void        on_status_disconnect_activate(GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	status_selected (menuitem);
	kf_jabber_disconnect ();
}

void        on_status_disconnect_x_activate(GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_gui_change_status ("unavailable"); 
}


static void status_selected (GtkMenuItem *menuitem) {
	GtkLabel *label;
	GtkImage *image;
	const gchar *text;
	GdkPixbuf *pixbuf;
	GtkImage *status;
	GtkMenuItem *item1;
	GtkLabel *item1_label;
	extern GladeXML *iface;
	
	label = GTK_LABEL (gtk_bin_get_child (GTK_BIN (menuitem)));
	text = gtk_label_get_text (label);
	
	image = GTK_IMAGE (gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (menuitem)));
	pixbuf = gtk_image_get_pixbuf (image);

	status = GTK_IMAGE (glade_xml_get_widget (iface, "my_status_image"));
	item1 = GTK_MENU_ITEM (glade_xml_get_widget (iface, "menuitem1"));
	item1_label = GTK_LABEL (gtk_bin_get_child (GTK_BIN (item1)));
	
	gtk_image_set_from_pixbuf (status, pixbuf);
	gtk_label_set_text (item1_label, text);
}

/* Guzik "Pomoc" w oknie ustawieñ po³±czenia */
void        on_connection_help_clicked     (GtkButton *button,
                                            gpointer user_data)
{
}

void        on_menu_new_msg_activate       (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_connected ();
	kf_new_message (NULL);
}

void        on_menu_quit_activate          (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	gtk_main_quit ();
}


void        on_menu_prefs_activate         (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_settings ();
}

void        on_menu_archive_activate         (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_archive_viewer ();
}

void        on_menu_vcard_activate         (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_connected ();
	kf_vcard_my ();
}


void        on_menu_groupchat_activate     (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_connected ();
	kf_muc_join_dialog ();
}


void        on_menu_add_contact_activate   (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_connected ();
	kf_gui_add_contact (NULL);
}


void        on_menu_info_activate          (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	GtkWidget *win;
	extern GladeXML *iface;
	GtkWidget *version_label;
	gchar *text;

	win = glade_xml_get_widget (iface, "info_window");
	
	version_label = glade_xml_get_widget (iface, "info_label");
	text = g_strdup_printf (gtk_label_get_text (GTK_LABEL (version_label)), VERSION);
	gtk_label_set_text (GTK_LABEL (version_label), text);
	g_free (text);
	
	kf_gui_show (win);
}

void        on_menu_xml_activate           (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	GtkWidget *win;
	extern GladeXML *iface;

	kf_connected ();
	win = glade_xml_get_widget (iface, "xml_console");
	kf_gui_show (win);
}


void        on_menu_show_offline_activate  (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_gui_toggle_show_offline ();
}


void        on_menu_show_toolbar_activate  (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	GtkWidget *toolbar;
	extern GladeXML *iface;

	toolbar = glade_xml_get_widget (iface, "toolbar1");
	g_return_if_fail (toolbar);
	if (! gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
//	if (GTK_WIDGET_VISIBLE (toolbar)) {
		kf_preferences_set_int ("showToolbar", (gint) FALSE);
		gtk_widget_hide (toolbar);
	} else {
		kf_preferences_set_int ("showToolbar", (gint) TRUE);
		gtk_widget_show (toolbar);
	}
}




/* Zamkniêcie okna ustawieñ po³±czenia */
gboolean    on_any_window_delete_event    (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data)
{
	gtk_widget_hide (widget);
	return TRUE;
}

void        on_any_close_window_clicked    (GtkButton *button,
                                            gpointer user_data)
{
	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

void close_window                          (GtkButton *button,
                                            gpointer user_data)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

/*
 * Obs³uga m.in. lewego klikniêcia
 */
gboolean    on_roster_tree_view_button_press_event (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data)
{
	GtkTreeView *view = (GtkTreeView *) widget;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *jid, *label;
	KfJabberRosterItem *item;

	if (event->window == gtk_tree_view_get_bin_window (view) &&
		gtk_tree_view_get_path_at_pos (view, event->x, event->y,  &path, NULL,
			NULL, NULL)) {
		/* W miejscu klikniêcia jest co¶ */
		model = gtk_tree_view_get_model (view);

		if (!gtk_tree_model_get_iter (model, &iter, path))
			return FALSE;

		gtk_tree_model_get (model, &iter, 0, &label, 1, &jid, 2, &item, -1);
		/* To wszystko na pocz±tek */
		
		if (jid && event->type == GDK_BUTTON_PRESS && event->button == 3) {
			/* Klikniêcie 3 klawiszem na kontakcie */

			popup_menu_for_item (item, event);

		} else if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
			/* Podwójne klikniêcie 1 guzikiem */
			if (jid && item && item->fulljid) {
				
				if (kf_x_event_last (item->jid) != -1) {
					kf_x_event_release (item->jid);
				} else {				
					/* To jest jaki¶ tam kontakt */
					if (kf_preferences_get_int ("messageDefault"))
						kf_new_message (item->fulljid);
					else
						kf_chat_window (item->fulljid);
				}

			} else if (!jid) {
				/* Grupa */
				if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget), path)) {
					gtk_tree_view_collapse_row (GTK_TREE_VIEW (widget), path);
				} else {
					gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), path, FALSE);
				}
			}
		}
		gtk_tree_path_free (path);
	}
	return FALSE;
}

static void popup_menu_for_item (KfJabberRosterItem *item, GdkEventButton *event) {
	extern GladeXML *iface;
	GtkWidget *menu;
//	GtkWidget *log_in, *log_out;
	GtkWidget *add;
	GtkWidget *invite;
	GList *conferences;

	/* Items to be hidden in contact popup and shown in agent popup */
	const gchar *agent_items[] = {"log_in1", "log_out1",
		"popup_register", "popup_search", "agent_separator", NULL};
	int i;
	
	menu = glade_xml_get_widget (iface, "roster_popup_menu");
	for (i = 0; agent_items[i]; i++) {
		GtkWidget *w;

		w = glade_xml_get_widget (iface, agent_items[i]);
		if (w == NULL) {
			foo_debug ("Widget not found: %s\n", agent_items[i]);
			continue;
		}
		if (item->type == KF_JABBER_CONTACT_TYPE_CONTACT) {
			gtk_widget_hide (w);
		} else {
			gtk_widget_show (w);
		}
	}
	add = glade_xml_get_widget (iface, "popup_add_contact");
	if (item->in_roster) {
		gtk_widget_hide (add);
	} else {
		gtk_widget_show (add);
	}

	/* Setup Invite to... sub-menu */
	invite = glade_xml_get_widget (iface, "popup_invite");
	g_assert (invite);
	if ((conferences = kf_muc_get_list ()) != NULL) {
		GtkWidget *menu = gtk_menu_new ();
		GList *tmp;

		for (tmp = conferences; tmp; tmp = tmp->next) {
			KfMUC *muc = tmp->data;
			gchar *name = kf_muc_get_signature (muc);
			GtkWidget *item = gtk_menu_item_new_with_label (name);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			g_signal_connect (G_OBJECT (item), "activate",
					G_CALLBACK (invite_to_menu_cb), muc);
			gtk_widget_show (item);
			g_object_set_data_full (G_OBJECT(item), "title", name, g_free);
		}
		
		gtk_widget_show (menu);
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (invite), menu);
		
		gtk_widget_show (invite);
	} else {
		gtk_widget_hide (invite);
	}
		
	/* Setup 'Group' sub-menu */
	kf_popup_groups_menu_attach (iface, item);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, event->time);
}


/**
 * \brief Callabck for menu items in 'invite to' menu
 **/
static void invite_to_menu_cb (GtkMenuItem *menuitem, gpointer data)
{
	KfMUC *muc = data;
	kf_muc_invite_jid (muc, kf_gui_get_selected_jid ());
}


void        on_xml_send_clicked            (GtkButton *button,
                                            gpointer user_data)
{
	extern GladeXML *iface;
	GtkWidget *tv;		/* . */
	GtkTextBuffer *buffer;	/* . */
	GtkTextIter start, end;
	gchar *body;
	
	kf_connected ();
	/* Pobranie tre¶ci wiadomo¶ci */
	tv = glade_xml_get_widget (iface, "xml_console_text_view");
	if (tv == NULL)
		g_error ("Ni mo widoku na tekst!\n");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tv));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	body = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	gtk_text_buffer_delete (buffer, &start, &end);

	kf_jabber_send_raw (body);

	g_free (body);
}

void        on_add_contact_ok_clicked      (GtkButton *button,
                                            gpointer user_data)
{
	extern GladeXML *iface;
	GtkWidget *entry;
	const gchar *jid, *name, *group;
	gboolean subscribe = TRUE;

	kf_connected ();
	entry = glade_xml_get_widget (iface, "add_jid");
	jid = gtk_entry_get_text (GTK_ENTRY (entry));
	entry = glade_xml_get_widget (iface, "add_name");
	name = gtk_entry_get_text (GTK_ENTRY (entry));
	entry = glade_xml_get_widget (iface, "add_group");
	group = gtk_entry_get_text (GTK_ENTRY (entry));
	entry = glade_xml_get_widget (iface, "add_subscription");
	subscribe = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (entry));

	kf_jabber_add_contact (jid, name, group, subscribe);
	
	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

void        on_button_add_contact_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	kf_connected ();
	kf_gui_add_contact (NULL);
}


void        on_button_browse_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	kf_connected ();
	kf_browse_window ();
}


void        on_button_search_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	kf_connected ();
	kf_search_window ();
}
void        on_button_join_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	kf_connected ();
	kf_muc_join_dialog ();
}


void        on_button_register_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	kf_connected ();
	kf_register_window ();
}

void        on_menu_browse_activate        (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_connected ();
	kf_browse_window ();
}

void        on_menu_search_activate        (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_connected ();
	kf_search_window ();
}

void        on_menu_register_activate      (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_connected ();
	kf_register_window ();
}


void        on_change_password_activate        (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	kf_connected ();
	kf_change_password ();
}





void kf_gui_add_contact (const gchar *fulljid) {
	kf_gui_add_contact_full (fulljid, NULL, NULL);
}

void kf_gui_add_contact_full (const gchar *fulljid,
		const gchar *name, const gchar *group) {
	GtkWidget *win;
	extern GladeXML *iface;
	GtkWidget *combo;
	GtkWidget *entry;
	GList *groups;
	gchar *jid = NULL;

	kf_connected ();
	win = glade_xml_get_widget (iface, "add_contact_window");
	
	if (fulljid) {
		jid = g_strdup (fulljid);
		kf_jabber_jid_crop (jid);
	}
	
	entry = glade_xml_get_widget (iface, "add_jid");
	gtk_entry_set_text (GTK_ENTRY (entry), jid?jid:"");
	g_free (jid);
	
	entry = glade_xml_get_widget (iface, "add_group");
	gtk_entry_set_text (GTK_ENTRY (entry), group?group:"");
	
	entry = glade_xml_get_widget (iface, "add_name");
	gtk_entry_set_text (GTK_ENTRY (entry), name?name:"");
	
        combo = glade_xml_get_widget (iface, "combo1");
	groups = kf_gui_get_groups ();
	groups = g_list_prepend (groups, "");
        gtk_combo_set_popdown_strings (GTK_COMBO (combo), groups);
	g_list_free (groups);
	kf_gui_show (win);
}
