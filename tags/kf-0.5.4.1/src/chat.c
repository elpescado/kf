/* file chat.c */
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
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "kf.h"
#include "jabber.h"
#include "gui.h"
#include "chat.h"
#include "filter.h"
#include "events.h"
#include "xevent.h"
#include "preferences.h"
#include "emoticons.h"
#include "callbacks.h"
#include "new_message.h"
#include "vcard.h"
#include "archive_viewer.h"

#ifdef HAVE_GTKSPELL
#  include <gtkspell/gtkspell.h>
#endif

static GHashTable *kf_chat_windows = NULL;
static GtkTooltips *kf_chat_tooltips = NULL;

void kf_chat_init (void);
static void kf_chat_window_real (KfChat *chat);
KfChat *kf_chat_new (const gchar *jid);
gboolean on_my_textview_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static void kf_chat_send_msg (KfChat *chat);
static void kf_chat_display (KfChat *chat, KfJabberMessage *msg, gboolean moja);
gboolean on_chat_eventbox_realize (GtkWidget *widget, GdkEvent *event, gpointer user_data);
gboolean on_chat_eventbox_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean    on_tab_eventbox_button_press_event (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data);
void on_chat_menu_add_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_chat_menu_message_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_chat_menu_clear_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_chat_menu_history_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_chat_menu_info_activate (GtkMenuItem *menuitem, gpointer user_data);
gboolean on_chat_window_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data);

void on_chat_tabify_activate     (GtkMenuItem *menuitem,
                                            gpointer user_data);
static void kf_chat_tabify (KfChat *chat);
void on_chat_windowify_activate     (GtkMenuItem *menuitem,
                                            gpointer user_data);
static void kf_chat_windowify (KfChat *chat);
gboolean    on_kf_chat_focused             (GtkWidget *widget,
                                            GdkEventFocus *event,
                                            gpointer user_data);
static void kf_chat_update (KfChat *chat);
static void kf_chat_label_set (GtkLabel *label, const gchar *jid, const gchar *name);
void        on_chat_menu_block_activate    (GtkMenuItem *menuitem,
                                            gpointer user_data);
void on_close_tab1_activate                (GtkMenuItem *menuitem,
                                            gpointer user_data);
void        on_roster_notebook_switch_page                  (GtkNotebook *notebook,
                                            GtkNotebookPage *page,
                                            guint page_num,
                                            gpointer user_data);
void        tab_position_activate          (GtkMenuItem *menuitem,
                                            gpointer user_data);


static void kf_chat_gc_touch (KfChat *chat);
static void kf_chat_position_menu          (GtkMenu *menu,
                                             gint *x,
                                             gint *y,
                                             gboolean *push_in,
                                             gpointer user_data);
/*
static gboolean textview_clicked_cb        (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data);
static void textview_popup_cb              (GtkTextView *textview,
                                            GtkMenu *arg1,
                                            gpointer user_data);
static void open_link_cb                   (GtkMenuItem *menuitem,
                                            gpointer user_data);
static gboolean textview_motion_cb        (GtkWidget *widget,
                                            GdkEventMotion *event,
                                            gpointer user_data);
					    */
static gboolean kf_chat_is_active (KfChat *chat);
static void kf_chat_window_resized (GtkWidget *window, GtkAllocation *allocation, gpointer data);
static gboolean kf_chat_window_resized2 (GtkWidget *window, GdkEventConfigure *event, gpointer data);
static void kf_chat_insert_status_text (KfChat *chat, KfJabberPresenceType type, const gchar *status);



static gint *kf_chat_width = NULL;
static gint *kf_chat_height = NULL;

void kf_chat_init (void) {
	if (kf_chat_windows == NULL) {
		kf_chat_windows = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	}
	if (kf_chat_tooltips == NULL) {
		kf_chat_tooltips = gtk_tooltips_new ();
	}

	if (kf_chat_width == NULL) {
		kf_chat_width = kf_preferences_get_ptr ("chatSizeX");
		kf_chat_height = kf_preferences_get_ptr ("chatSizeY");
	}
}

/* Shows a KfJabberMessage */
void kf_chat_show (KfJabberMessage *msg) {
	KfChat *chat;
	
//	kf_chat_init (); 
	chat = g_hash_table_lookup (kf_chat_windows, msg->from);
	if (chat == NULL) {
		gchar *jid = g_strdup (msg->from);
		kf_jabber_jid_crop (jid);

		chat = g_hash_table_lookup (kf_chat_windows, jid);
		if (chat) {
			GtkWidget *label;
			/* FIXME */
			g_free (chat->jid);
			chat->jid = g_strdup (jid);
			g_hash_table_insert (kf_chat_windows, g_strdup (chat->jid), chat);
		
			label = glade_xml_get_widget (chat->glade, "from_label");
			if (chat->item) 
				kf_chat_label_set (GTK_LABEL (label), jid, chat->item->display_name);
			else
				gtk_label_set_text (GTK_LABEL (label), jid);

		g_free (jid);
		} else {
			chat = kf_chat_new (msg->from);
			g_hash_table_insert (kf_chat_windows, g_strdup (chat->jid), chat);
		}
	}
		
	kf_chat_display (chat, msg, FALSE);
	if (! kf_chat_is_active (chat))
		chat->unread++;

	if (chat->mode == KF_CHAT_WINDOW) {
		if (kf_preferences_get_int ("autoPopup")) {
			gtk_widget_show (chat->window);
			gdk_window_raise ((chat->window)->window);
		} else {
			if (! GTK_WIDGET_VISIBLE (chat->window)) {
				/* Only if there's no window */
				gchar *xjid;

				xjid = kf_jabber_jid_crop (g_strdup (msg->from));
				kf_x_event_push (xjid, KF_EVENT_CLASS_CHAT, chat->window);
				g_free (xjid);
			}
		}
	}
	
	kf_chat_update (chat);
}

/* Displays a chat window for *jid, creates a new one if necessary */
void kf_chat_window (const gchar *jid) {
	KfChat *chat;

	kf_chat_init (); 
	if ((chat = g_hash_table_lookup (kf_chat_windows, jid)) == NULL) {
		chat = kf_chat_new (jid);
		g_hash_table_insert (kf_chat_windows, g_strdup (jid), chat);
	}

	kf_chat_window_real (chat);
}

/* Internal - displays a chat */
static void kf_chat_window_real (KfChat *chat) {
		
	if (chat->mode == KF_CHAT_WINDOW) {
		if (chat->window) {
			gtk_container_add (GTK_CONTAINER (chat->container), chat->content);

			gtk_widget_show (chat->window);
			gdk_window_raise ((chat->window)->window);
		} else {
			chat->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
			gtk_container_add (GTK_CONTAINER (chat->window), chat->content);

			gtk_widget_show (chat->window);
			gdk_window_raise ((chat->window)->window);
		}
	} else if (chat->mode == KF_CHAT_TAB) {
		extern GladeXML *iface;
		GtkWidget *notebook;
		gint page;

		notebook = glade_xml_get_widget (iface, "roster_notebook");
		page = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), chat->content);
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);

		if (page == -1) {
			GtkWidget *label;
			GtkWidget *eventbox;
//			GtkStyle *sn;
//			GtkStyle *se;

			eventbox = gtk_event_box_new ();
//			eventbox = gtk_hbox_new (FALSE, 2);
			g_signal_connect (G_OBJECT (eventbox), 
		                      "button_press_event",
                		      G_CALLBACK (on_tab_eventbox_button_press_event),
		                      chat);
			label = gtk_label_new (chat->jid);
			gtk_container_add (GTK_CONTAINER (eventbox), label);
//			gtk_box_pack_start (GTK_BOX (eventbox), label, FALSE, FALSE, 0);
			gtk_widget_show (label);

//			se = eventbox->style;
//			sn = notebook->style;

//			se->bg[GTK_STATE_PRELIGHT].red = 0xffff;
			//sn->bg[GTK_STATE_NORMAL];
//
			chat->label = label;
			gtk_notebook_append_page (GTK_NOTEBOOK (notebook), chat->content, eventbox);
		} else {
			gtk_widget_show (chat->content);
			gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page);
		}
	}
	kf_chat_update (chat);
}

/* Creates a new KfChat - a structure representing a chat window */
KfChat *kf_chat_new (const gchar *jid) {
	KfChat *chat;
	GtkWidget *textview, *scrolledwindow, *menu, *eventbox;
	GtkLabel *label;
	gchar *cropped_jid, *str;
	KfJabberRosterItem *item;
	GtkWidget *paned;
	GError *spell_err = NULL;

	cropped_jid = g_strdup (jid);
	for (str = cropped_jid; *str != '\0' && *str != '/'; str++); /* Usuniêcie zasobu */
	*str = '\0';
	item = kf_jabber_roster_item_get (cropped_jid);
	g_free (cropped_jid);

	chat = g_malloc (sizeof (KfChat));
	chat->item = item;
	chat->unread = 0;
	chat->jid = g_strdup (jid);
	chat->glade = glade_xml_new (kf_find_file ("chat.glade"), NULL, NULL); 
	glade_xml_signal_autoconnect (chat->glade);
	chat->content = gtk_widget_ref (glade_xml_get_widget (chat->glade, "vbox1"));
//	chat->window = NULL;
	chat->window = glade_xml_get_widget (chat->glade, "chat_window");
	chat->container = glade_xml_get_widget (chat->glade, "frame1");
	chat->label = NULL;
	textview = glade_xml_get_widget (chat->glade, "his_textview");
	kf_emo_init_textview (GTK_TEXT_VIEW (textview));
	chat->view = GTK_TEXT_VIEW (textview);
	chat->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	textview = glade_xml_get_widget (chat->glade, "my_textview");
	chat->my_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	g_object_set_data (G_OBJECT (textview), "Chat", chat);
	g_object_set_data (G_OBJECT (chat->content), "Chat", chat);
	scrolledwindow = glade_xml_get_widget (chat->glade, "scrolledwindow1");
	chat->adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow));
	chat->status_image = GTK_IMAGE (glade_xml_get_widget (chat->glade, "status_image"));
	chat->last_activity = time (NULL);

	menu = glade_xml_get_widget (chat->glade, "chat_menu");
	g_object_set_data (G_OBJECT (menu), "KfChat", chat);

	menu = glade_xml_get_widget (chat->glade, "tab_menu");
	g_object_set_data (G_OBJECT (menu), "KfChat", chat);

	label = GTK_LABEL (glade_xml_get_widget (chat->glade, "from_label"));
	if (item) 
		kf_chat_label_set (label, jid, item->display_name);
	else
		gtk_label_set_text (label, jid);

	chat->my_text = gtk_text_buffer_create_tag (chat->buffer, "my_text",
			"foreground", kf_preferences_get_string ("colorChatMe"),
			NULL);
	chat->his_text = gtk_text_buffer_create_tag (chat->buffer, "his_text",
			"foreground", kf_preferences_get_string ("colorChatHe"),
			NULL);
	chat->notify = gtk_text_buffer_create_tag (chat->buffer, "notify",
			"foreground", kf_preferences_get_string ("colorChatNotify"),
			NULL);
	
	/* GtkSpell Support */
#ifdef HAVE_GTKSPELL
	if ( ! gtkspell_new_attach(GTK_TEXT_VIEW(textview), NULL, &spell_err)) {
		foo_debug ("Unable to use GtkSpell: %s\n", spell_err->message);
	}
#endif
	
	if (kf_preferences_get_int ("tabDefault")) {
		chat->mode = KF_CHAT_TAB;
		kf_chat_tabify (chat);
	} else {
		chat->mode = KF_CHAT_WINDOW;
	}
	
	if (item) {
		gtk_image_set_from_pixbuf (chat->status_image,
				kf_gui_pixmap_get_status (item->presence));
	}

	eventbox = glade_xml_get_widget (chat->glade, "chat_eventbox");
	if (eventbox) {
		g_signal_connect (G_OBJECT (eventbox), 
                      "button_press_event",
                      G_CALLBACK (on_chat_eventbox_button_press_event),
                      chat);
	}

	if (kf_chat_width && kf_chat_height && *kf_chat_width && *kf_chat_height)
		gtk_window_resize (GTK_WINDOW (chat->window), *kf_chat_width, *kf_chat_height);
	
	paned = glade_xml_get_widget (chat->glade, "vpaned1");
	gtk_paned_set_position (GTK_PANED (paned), *kf_chat_height - 84);

	g_signal_connect (G_OBJECT (chat->window), "configure-event", 
		GTK_SIGNAL_FUNC (kf_chat_window_resized2), chat);

	if (chat->item && kf_preferences_get_int ("chatShowNotify")) {
			kf_chat_insert_status_text (chat, item->presence, item->status);
	}
	
	return chat;
}

/* Wstawianie \n przez Shift+Enter i wysy³anie przez Enter */
gboolean    on_my_textview_key_press_event (GtkWidget *widget,
                                            GdkEventKey *event,
                                            gpointer user_data)
{
	KfChat *chat;
	chat = g_object_get_data (G_OBJECT (widget), "Chat");
	if (event->keyval == GDK_Return) {
		/* Ugly, but works */
		if (event->state & GDK_SHIFT_MASK)
			return FALSE;

		kf_chat_send_msg (chat); 
		return TRUE;
	} else if (event->keyval == GDK_F10) {
		GtkWidget *menu = glade_xml_get_widget (chat->glade, "chat_menu");
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, kf_chat_position_menu, chat, 0, event->time);
	}
	return FALSE;
}

static void kf_chat_send_msg (KfChat *chat) {
	GtkTextIter start, end;
	gchar *body;
	KfJabberMessage *msg;	/* Wiadomo¶æ */
	

	gtk_text_buffer_get_bounds (chat->my_buffer, &start, &end);
	body = gtk_text_buffer_get_text (chat->my_buffer, &start, &end, FALSE);

	if (*body == '\0') {
		g_free (body);
		return;
	}
	gtk_text_buffer_delete (chat->my_buffer, &start, &end);

	/* Kreacja wiadomo¶ci */
	msg = kf_jabber_message_new ();
	msg->to = g_strdup (chat->jid);
	msg->body = body;
	msg->type = KF_JABBER_MESSAGE_TYPE_CHAT;

	kf_jabber_message_send (msg);
	kf_chat_display (chat, msg, TRUE);
//	kf_jabber_message_free (msg);
}

static void kf_chat_display (KfChat *chat, KfJabberMessage *msg, gboolean moja) {
	const gchar *nick;
	KfEmoTagTable style;
	
	if (moja) {
		if ((nick = kf_preferences_get_string ("myNick")) == NULL) {
			nick = _("Me");
		}
	} else {
		nick = ((chat->item)?chat->item->display_name:msg->from);
	}

	style.stamp = chat->stamp;
	style.in = chat->his_text;
	style.out = chat->my_text;
	
	kf_display_message (chat->buffer, msg->stamp, nick, msg->body, moja, &style);
	if (chat->adj->value == chat->adj->upper - chat->adj->page_size) {
		GtkTextIter iter;
		GtkTextMark *mark;
		gtk_text_buffer_get_end_iter (chat->buffer, &iter);

		mark = gtk_text_buffer_create_mark (chat->buffer, NULL, &iter, FALSE);
		
		gtk_text_view_scroll_to_mark (chat->view, 
			mark , 0.0, TRUE, 0.0, 1.0);
	}
	return;
}

/*
 * Ustawia ikonkê obecno¶ci w okienku czata
 */
void kf_chat_update_status (const gchar *jid, KfJabberPresenceType type, const gchar *status) {
	KfChat *chat;

	if ((chat = g_hash_table_lookup (kf_chat_windows, jid))) {
		gtk_image_set_from_pixbuf (chat->status_image, kf_gui_pixmap_get_status (type));
		if (status && *status) {
			gtk_tooltips_set_tip (kf_chat_tooltips, 
				gtk_widget_get_parent (GTK_WIDGET (chat->status_image)),
			       	status, NULL);
		} else {
			gtk_tooltips_set_tip (kf_chat_tooltips,
				gtk_widget_get_parent (GTK_WIDGET (chat->status_image)),
			       	kf_jabber_presence_type_to_human_string (type), NULL);
		}

		if (kf_preferences_get_int ("chatShowNotify")) {
			kf_chat_insert_status_text (chat, type, status);
		}
		
		return;
	}
}

gboolean    on_chat_eventbox_button_press_event (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data) {

	GtkWidget *menu;
	KfChat *chat;

	chat = (KfChat *) user_data;
	if (chat == NULL)
		g_error ("Nie ma chata!\n");

	menu = glade_xml_get_widget (chat->glade, "chat_menu");
	if (menu == NULL)
		g_error ("Nie ma menu!\n");
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, event->time);
	return TRUE;
}
gboolean    on_tab_eventbox_button_press_event (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data) {

	GtkWidget *menu;
	KfChat *chat;

	if (event->button == 3) {
		chat = (KfChat *) user_data;
		if (chat == NULL)
			g_error ("Nie ma chata!\n");

		menu = glade_xml_get_widget (chat->glade, "tab_menu");
		if (menu == NULL)
			g_error ("Nie ma menu!\n");

		if (! g_object_get_data (G_OBJECT (menu), "inited")) {
			GtkWidget *item;

			item = glade_xml_get_widget (chat->glade, "top");
			g_signal_connect (G_OBJECT (item), "activate",
					G_CALLBACK (tab_position_activate),
					GINT_TO_POINTER (GTK_POS_TOP));
			item = glade_xml_get_widget (chat->glade, "bottom");
			g_signal_connect (G_OBJECT (item), "activate",
					G_CALLBACK (tab_position_activate),
					GINT_TO_POINTER (GTK_POS_BOTTOM));
			item = glade_xml_get_widget (chat->glade, "left");
			g_signal_connect (G_OBJECT (item), "activate",
					G_CALLBACK (tab_position_activate),
					GINT_TO_POINTER (GTK_POS_LEFT));
			item = glade_xml_get_widget (chat->glade, "right");
			g_signal_connect (G_OBJECT (item), "activate",
					G_CALLBACK (tab_position_activate),
					GINT_TO_POINTER (GTK_POS_RIGHT));

			g_object_set_data (G_OBJECT (menu), "inited", menu);
		}
		
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, event->time);
		return TRUE;
	} else {
		return FALSE;
	}
}

/*
   Changes position of tabs in roster window
 */
void        tab_position_activate          (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	GtkPositionType type = GPOINTER_TO_INT (user_data);
	extern GladeXML *iface;
	GtkWidget *notebook;

	kf_preferences_set_int ("tabPosition", type);
	notebook = glade_xml_get_widget (iface, "roster_notebook");
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), type);
}


void        on_chat_menu_add_activate      (GtkMenuItem *menuitem,
                                            gpointer user_data)
{	
	KfChat *chat;
	GtkWidget *menu;

	menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

	chat = g_object_get_data (G_OBJECT (menu), "KfChat");
	if (chat) {
		kf_gui_add_contact (chat->jid);
	}
}

void        on_chat_menu_message_activate  (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	KfChat *chat;
	GtkWidget *menu;

	menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

	chat = g_object_get_data (G_OBJECT (menu), "KfChat");
	if (chat) 
		kf_new_message (chat->jid);
}

void        on_chat_menu_clear_activate    (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	KfChat *chat;
	GtkWidget *menu;

	menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

	chat = g_object_get_data (G_OBJECT (menu), "KfChat");

	if (chat) {
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds (chat->buffer, &start, &end);
		gtk_text_buffer_delete (chat->buffer, &start, &end);
	}
}

void        on_chat_menu_history_activate     (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	KfChat *chat;
	GtkWidget *menu;

	menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

	chat = g_object_get_data (G_OBJECT (menu), "KfChat");
	if (chat) {
//		kf_history (chat->jid);
		gchar *jidx;
		jidx = g_strdup (chat->jid);
		kf_jabber_jid_crop (jidx);
		kf_archive_viewer_jid (jidx);
		g_free (jidx);
	}
}


void        on_chat_menu_info_activate     (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	KfChat *chat;
	GtkWidget *menu;

	menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

	chat = g_object_get_data (G_OBJECT (menu), "KfChat");
	if (chat) 
		kf_vcard_get (chat->jid);
}

void        on_chat_menu_block_activate    (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	KfChat *chat;
	GtkWidget *menu;

	menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

	chat = g_object_get_data (G_OBJECT (menu), "KfChat");
	if (chat) {
		gchar *txt;
		
		kf_filter_block_user (chat->jid);

		txt = g_strdup_printf (_("User %s has been blocked. You won't receive any messages from him. To undo this open Preferences window and go to \"Block List\"."), chat->jid);
		kf_gui_alert (txt);
		g_free (txt);		
	}
}


gboolean	on_chat_window_key_press_event      (GtkWidget *widget,
                                            GdkEventKey *event,
                                            gpointer user_data) {

	/* ESC zamyka okienko */
	if (event->keyval == GDK_Escape) {
		GladeXML *glade;
		GtkWidget *content;
		KfChat *chat;

		glade = glade_get_widget_tree (widget);
		content = glade_xml_get_widget (glade, "vbox1");
		chat = g_object_get_data (G_OBJECT (content), "Chat");
		gtk_widget_hide (widget);
		kf_chat_gc_touch (chat);
	}
	return FALSE;
}

void on_chat_tabify_activate     (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	KfChat *chat;
	GtkWidget *menu;

	menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

	chat = g_object_get_data (G_OBJECT (menu), "KfChat");
	if (chat) 
		kf_chat_tabify (chat);
}

/* Moves a chat window to a tab */
static void kf_chat_tabify (KfChat *chat) {
	GtkWidget *container;
	chat->mode = KF_CHAT_TAB;

	container = gtk_widget_get_parent (chat->content);
	gtk_widget_ref (chat->content);
	gtk_container_remove (GTK_CONTAINER (container), chat->content);
	kf_chat_window_real (chat);
	gtk_widget_unref (chat->content);

	gtk_widget_hide (chat->window);
}

void on_chat_windowify_activate     (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	KfChat *chat;
	GtkWidget *menu;

	menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

	chat = g_object_get_data (G_OBJECT (menu), "KfChat");
	if (chat) 
		kf_chat_windowify (chat);
}


void on_close_tab1_activate                (GtkMenuItem *menuitem,
                                            gpointer user_data)
{
	KfChat *chat;
	GtkWidget *menu;

	menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));
	chat = g_object_get_data (G_OBJECT (menu), "KfChat");

	gtk_widget_hide (chat->content);

	kf_chat_gc_touch (chat);
}


/* Moves a chat window into a window */
static void kf_chat_windowify (KfChat *chat) {
	extern GladeXML *iface;
	gint page;
	GtkWidget *notebook;
	chat->mode = KF_CHAT_WINDOW;

	notebook = glade_xml_get_widget (iface, "roster_notebook");
	page = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), chat->content);
	
	gtk_widget_ref (chat->content);
	gtk_notebook_remove_page (GTK_NOTEBOOK (notebook), page);
	chat->label = NULL;
	kf_chat_window_real (chat);
	gtk_widget_unref (chat->content);
}

/* Event - chat window is focused */
gboolean    on_kf_chat_focused             (GtkWidget *widget,
                                            GdkEventFocus *event,
                                            gpointer user_data)
{
	GladeXML *glade;
	GtkWidget *content;
	KfChat *chat;
	
	glade = glade_get_widget_tree (widget);
	content = glade_xml_get_widget (glade, "vbox1");
	chat = g_object_get_data (G_OBJECT (content), "Chat");
	g_assert (chat);
	chat->unread = 0;
	kf_chat_update (chat);
	return FALSE;
}

/* Updates chat title/tab label */
static void kf_chat_update (KfChat *chat) {
	const gchar *nick;

//	g_print ("Chat is %sactive\n", kf_chat_is_active (chat)?"":"not ");
	nick = chat->item?chat->item->display_name:chat->jid;
	if (chat->mode == KF_CHAT_WINDOW) {
		gchar *text;

		if (chat->unread)
			text = g_strdup_printf ("* %s (%d)", nick, chat->unread);
		else
			text = g_strdup_printf ("%s", nick);

		gtk_window_set_title (GTK_WINDOW (chat->window), text);
		g_free (text);
	} else {
		extern GladeXML *iface;
		GtkWidget *notebook;
		gint page;

		notebook = glade_xml_get_widget (iface, "roster_notebook");
		page = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), chat->content);
		if (page != -1) {
			GtkWidget *label;
			gchar *text;
			GdkColor color = {0, 0xFFFF, 0, 0};
			GdkColor black = {0, 0, 0, 0};

//			label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (notebook), chat->content);
			label = chat->label;
			g_assert (label);
			if (chat->unread) {
				text = g_strdup_printf (_("%s (%d)"), nick, chat->unread);
				gtk_widget_modify_fg (label, GTK_STATE_NORMAL, &color);
			} else {
				text = g_strdup_printf ("%s", nick);
				gtk_widget_modify_fg (label, GTK_STATE_NORMAL, &black);
			}
			gtk_label_set_text (GTK_LABEL (label), text);
			g_free (text);
		}
	}
}

void        on_roster_notebook_switch_page                  (GtkNotebook *notebook,
                                            GtkNotebookPage *page,
                                            guint page_num,
                                            gpointer user_data)
{
	GtkWidget *content;
	KfChat *chat;
	
	if (page_num != 0) {
		content = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num);
		chat = g_object_get_data (G_OBJECT (content), "Chat");
		g_assert (chat);
		chat->unread = 0;
		kf_chat_update (chat);
	}
}


static void kf_chat_label_set (GtkLabel *label, const gchar *jid, const gchar *name) {
	guint nick_len, jid_len;
	gchar *text;
	PangoAttrList *list;
	PangoAttribute *italic_attr, *small_attr;

	nick_len = strlen (name);
	jid_len = strlen (jid);

	list = pango_attr_list_new ();

	italic_attr = pango_attr_style_new (PANGO_STYLE_ITALIC);
	italic_attr->start_index = nick_len+2;
	italic_attr->end_index = nick_len + jid_len+2;
	pango_attr_list_insert (list, italic_attr);

	//small_attr = pango_attr_scale_new (PANGO_SCALE_X_SMALL);
	small_attr = pango_attr_scale_new ((double) 0.9);
	small_attr->start_index = nick_len+1;
	small_attr->end_index = nick_len + jid_len+3;
	pango_attr_list_insert (list, small_attr);

	text = g_strdup_printf ("%s <%s>", name, jid);

	gtk_label_set_text (label, text);
	gtk_label_set_attributes (label, list);

	g_free (text);
	pango_attr_list_unref (list);
}

static void kf_chat_gc_touch (KfChat *chat) {
	chat->last_activity = time (NULL);
}

/* Klikanie na ekranie... 
 *
 * We check if user clicked on a link... if so, we launch a browser:-)
 *
 * Or, if user clicked 3rd mouse button we store the URL
 * 
 * */
/*
static gboolean textview_clicked_cb        (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data) {
	gint x, y;
	GtkTextIter iter;
	KfChat *chat = user_data;

	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_WIDGET,
			event->x, event->y,
			&x, &y);
	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (widget), &iter, x, y);

	if (gtk_text_iter_has_tag (&iter, chat->url)) {
		GtkTextIter start, end;
		gchar *text;

		start = end = iter;

		while (! gtk_text_iter_begins_tag (&start, chat->url))
			gtk_text_iter_backward_char (&start);

		while (! gtk_text_iter_ends_tag (&end, chat->url))
			gtk_text_iter_forward_char (&end);

		text = gtk_text_iter_get_text (&start, &end);

		* text is our link *
		if (event->button == 1) {
			kf_www_open (text);
		} else if (event->button == 3) {
			g_object_set_data_full (G_OBJECT (widget), "URL", g_strdup (text), g_free);
		}
		g_free (text);

		return FALSE;
	}

  return FALSE;
}
*/
/*
 * Chceck if user clicked on an URL, then add "open link" entry to popup menu
 */
/*
static void textview_popup_cb              (GtkTextView *textview,
                                            GtkMenu *menu,
                                            gpointer user_data) {
	gchar *text;

	if (text = g_object_get_data (G_OBJECT (textview), "URL")) {
		GtkWidget *item;

		item = gtk_separator_menu_item_new ();
		gtk_widget_show (item);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

		item = gtk_menu_item_new_with_label (_("Open link"));
		gtk_widget_show (item);
		g_signal_connect (G_OBJECT (item), "activate",
				G_CALLBACK (open_link_cb), text);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	}
}

static void open_link_cb                   (GtkMenuItem *menuitem,
                                            gpointer user_data) {
	kf_www_open (user_data);
}
*/


static gboolean kf_chat_is_active (KfChat *chat) {
	if (chat->mode == KF_CHAT_WINDOW) {
		if (gtk_major_version >= 2 && gtk_minor_version >= 2) {
			gboolean x;

//		g_object_get (G_OBJECT (chat->window), "", &x, NULL);
			g_object_get (G_OBJECT (chat->window), "is-active", &x, NULL);
//		g_object_get (G_OBJECT (chat->window), "has-toplevel-focus", &x, NULL);
			return x;
		} else {
			return FALSE;
		}
		
//		return gtk_window_activate_focus (chat->window);

//		return gtk_widget_is_focus (chat->window);
	} else if (chat->mode == KF_CHAT_TAB) {
		extern GladeXML *iface;
		GtkWidget *notebook;
		gint current, num;

		notebook = glade_xml_get_widget (iface, "roster_notebook");
		current = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
		num = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), chat->content);
		return (gboolean) current == num;
	} else {
		return FALSE;
	}
}


static void kf_chat_position_menu          (GtkMenu *menu,
                                             gint *x,
                                             gint *y,
                                             gboolean *push_in,
                                             gpointer user_data)
{
	KfChat *chat = user_data;
	GtkWidget *eventbox = glade_xml_get_widget (chat->glade, "chat_eventbox");
	gint wx,wy;
	gdk_window_get_origin (eventbox->window, &wx, &wy);

	*x = wx;
	*y = wy;
	*push_in = TRUE;		
}


/**
 * \brief Callback called when chat window is being resized
 **/
static void kf_chat_window_resized (GtkWidget *window, GtkAllocation *allocation, gpointer data)
{
	foo_debug ("%ld: window resized to %d x %d\n", time (NULL),
		allocation->height, allocation->width);
}

static gboolean kf_chat_window_resized2 (GtkWidget *window, GdkEventConfigure *event, gpointer data)
{
	foo_debug ("%ld: window resized to %d x %d\n", time (NULL),
		event->width, event->height);

//	kf_preferences_set_int ("chatSizeX", event->width);
//	kf_preferences_set_int ("chatSizeY", event->height);
	/* We'll manipulate by means of pointers to configuration values */
	*kf_chat_width = event->width;
	*kf_chat_height = event->height;

	return FALSE;
}


static void kf_chat_insert_status_text (KfChat *chat, KfJabberPresenceType type, const gchar *status)
{
	gchar *text;		/* Formatted text to be displayed */
	const gchar *nick;	/* Nick of a person who changed status */
	GtkTextIter iter;	/* Text iterator */
	
	if (kf_preferences_get_int ("chatShowStamps")) {
		gchar *sstamp;
		struct tm *stime;
		time_t t = time (NULL);
		stime = localtime (&t);

		gtk_text_buffer_get_end_iter (chat->buffer, &iter);
		sstamp = g_strdup_printf ("[%02d:%02d] ", stime->tm_hour, stime->tm_min);
		
		gtk_text_buffer_insert_with_tags_by_name (chat->buffer, &iter, sstamp, -1, "stamp", NULL);

		g_free (sstamp);
	}

	nick = (chat->item)?chat->item->display_name:chat->jid;
	if (status && *status)
		text = g_strdup_printf ("%s is now %s: %s\n", nick, 
				kf_jabber_presence_type_to_human_string (type),
				status);
	else
		text = g_strdup_printf ("%s is now %s\n", nick, 
				kf_jabber_presence_type_to_human_string (type));

	gtk_text_buffer_get_end_iter (chat->buffer, &iter);
	gtk_text_buffer_insert_with_tags (chat->buffer, &iter, text, -1,
			chat->notify, NULL);

	g_free (text);
	
	if (chat->adj->value == chat->adj->upper - chat->adj->page_size) {
		GtkTextIter iter;
		GtkTextMark *mark;
		gtk_text_buffer_get_end_iter (chat->buffer, &iter);

		mark = gtk_text_buffer_create_mark (chat->buffer, NULL, &iter, FALSE);
		
		gtk_text_view_scroll_to_mark (chat->view, 
			mark , 0.0, TRUE, 0.0, 1.0);
	}

}
