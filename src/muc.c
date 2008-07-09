/* file muc.c */
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
#include <gdk/gdkkeysyms.h>
#include <loudmouth/loudmouth.h>
#include <glade/glade.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "kf.h"
#include "connection.h"
#include "jabber.h"
#include "emoticons.h"
#include "gui.h" /* for kf_gui_pixmap_get_status (guint status); */
#include "muc.h"
#include "x_data.h"
#include "gtktip.h"
#include "preferences.h"

#ifdef HAVE_GTKSPELL
#  include <gtkspell/gtkspell.h>
#endif

/* No topic */
#define NO_TOPIC ""

void kf_new_message (const gchar *to);
void kf_chat_window (const gchar *jid);
void kf_vcard_get (const gchar *jid);

//typedef struct _KfMUC KfMUC;
typedef struct _KfMUCRoster KfMUCRoster;
typedef struct _KfMUCRosterItem KfMUCRosterItem;

typedef enum {
	KF_MUC_ROLE_NONE,
	KF_MUC_ROLE_MODERATOR,
	KF_MUC_ROLE_PARTICIPANT,
	KF_MUC_ROLE_VISITOR
} KfMUCRole;

typedef enum {
	KF_MUC_AFFILIATION_NONE,
	KF_MUC_AFFILIATION_OWNER,
	KF_MUC_AFFILIATION_ADMIN,
	KF_MUC_AFFILIATION_MEMBER,
	KF_MUC_AFFILIATION_OUTCAST
} KfMUCAffiliation;

struct _KfMUC {
	guint ref_count;
	gchar *jid;
	gchar *nick;
	gchar *pass;

	GladeXML *glade;
	GtkWidget *window;
	GtkWidget *menu;
	GtkWidget *history_view;
	GtkTextBuffer *input;
	GtkTextBuffer *history;
	GtkWidget *treeview;
	GtkWidget *topic_entry;
	KfMUCRoster *roster;
	GtkWidget *change_buttonbox;
	GtkWidget *change_enable;
	GtkWidget *button_menu;
	GtkTooltips *tips;
	GtkAdjustment *adj;

	GtkTextTag *notify;
	GtkTextTag *my_text;
	GtkTextTag *their_text;
	GtkTextTag *error_text;

	gchar *topic;

	GtkWidget *configure;
	GtkWidget *conf_status;
	GtkWidget *conf_view;

	/* Tooltip */
	gint tooltip_x;
	gint tooltip_y;
	GdkRectangle tooltip_cell;
	gint tooltip_timeout;
	GtkWidget *tooltip_widget;

	GList *urls;
};

struct _KfMUCRoster {
	guint ref_count;
	KfMUC *muc;

	GHashTable *items;
	GList *litems;

	GtkListStore *store;
};

struct _KfMUCRosterItem {
	guint ref_count;
	KfMUCRoster *roster;

	gchar *jid;
	gchar *name;

	gint presence;
	gchar *status;

	KfMUCRole role;
	KfMUCAffiliation affiliation;
	GtkTextTag *tag;
};

typedef struct {
	gchar *nick;
	gchar *url;
	gulong time;
} KfURL;

struct MUCtrans{
	gint code;
	gchar *str;
	gchar *name;
	gchar *tname;
};

/* Conversion tables */
static struct MUCtrans roles[] = {
	{KF_MUC_ROLE_NONE,	"none", "none", NULL},
	{KF_MUC_ROLE_MODERATOR,	"moderator", "moderator", NULL},
	{KF_MUC_ROLE_PARTICIPANT,	"participant", "participant", NULL},
	{KF_MUC_ROLE_VISITOR,	"visitor", "visitor", NULL},
	{-1, "", "", NULL}};

static struct MUCtrans affiliations[] = {
	{KF_MUC_AFFILIATION_NONE,	"none",	"", NULL},
	{KF_MUC_AFFILIATION_OWNER,	"owner",	"", NULL},
	{KF_MUC_AFFILIATION_ADMIN,	"admin",	"", NULL},
	{KF_MUC_AFFILIATION_MEMBER,	"member",	"", NULL},
	{KF_MUC_AFFILIATION_OUTCAST,	"outcast",	"", NULL},
	{-1, "", "", NULL}};

/* Structure used to pass parameters to g_hash_table_foreach function
   in order to broadcast your status to all conferences */
typedef struct {
	gint type;
	const char *type_str;
	const char *status;
} KfMUCStatusParams;

#include "muc_private.h"

/* Local variables */
static GHashTable *kf_muc_rooms = NULL;	/* Hash-table of all rooms, indexed by their JIDs */
static LmMessageHandler *kf_muc_mhandler = NULL;
static LmMessageHandler *kf_muc_phandler = NULL;
static LmMessageHandler *kf_muc_ihandler = NULL;

/* I'm too lazy to write comments ;'( */

static void button_change_enable_cb (GtkButton *button, gpointer data);
static void button_change_cancel_cb (GtkButton *button, gpointer data);

static gboolean kf_muc_roster_motion_notify (GtkWidget *tv, GdkEventMotion *event, gpointer data);
static gboolean kf_muc_roster_leave_notify (GtkWidget *tv, GdkEventCrossing *event, gpointer data);
static gboolean kf_muc_roster_tooltip_timeout (gpointer data);
static GtkWidget *show_tooltip (KfMUCRosterItem *item);
static void hide_tooltip (GtkWidget *tooltip);
static gchar *tooltip_text (KfMUCRosterItem *item);
static void kf_muc_status_changed_foreach   (gpointer key,
                                             gpointer value,
                                             gpointer user_data);
static void kf_muc_tab_completion (KfMUC *muc, GtkTextView *view);

static void on_button_menu_clicked  (GtkButton *button, gpointer data);

static void kf_muc_close (KfMUC *self);


static gboolean muc_window_delete (GtkWidget *tv, GdkEvent *event, gpointer data);


static void kf_muc_menu_connect_signals (KfMUC *self, GladeXML *glade);
static void menu_change_nick_clicked (GtkButton *button, gpointer data);
static void menu_url_grabber_clicked (GtkButton *button, gpointer data);
static void menu_quit_clicked (GtkButton *button, gpointer data);

static gchar *kf_muc_pick_color (void);

static void kf_muc_get_list_foreach (gpointer key, gpointer value, gpointer user_data);


/* Here we can init MUC */
void kf_muc_init (void) {
	LmConnection *conn;

//	g_return_if_fail (kf_muc_rooms == NULL);
	if (kf_muc_rooms)
		return;
	foo_debug ("MUC init\n");

	conn = kf_jabber_get_connection ();

	kf_muc_rooms = g_hash_table_new (g_str_hash, g_str_equal);
	
	kf_muc_mhandler = lm_message_handler_new (kf_muc_mhandler_cb, NULL, NULL);
	kf_muc_phandler = lm_message_handler_new (kf_muc_phandler_cb, NULL, NULL);
	kf_muc_ihandler = lm_message_handler_new (kf_muc_ihandler_cb, NULL, NULL);

	lm_connection_register_message_handler (conn, kf_muc_mhandler, LM_MESSAGE_TYPE_MESSAGE, 3);
	lm_connection_register_message_handler (conn, kf_muc_phandler, LM_MESSAGE_TYPE_PRESENCE, 3);
	lm_connection_register_message_handler (conn, kf_muc_ihandler, LM_MESSAGE_TYPE_IQ, 3);
}

void kf_muc_join_room (const gchar *jid, const gchar *nick) {
	kf_muc_new (jid, nick, NULL);
}
void kf_muc_join_room_with_password (const gchar *jid, const gchar *nick, const gchar *pass) {
	kf_muc_new (jid, nick, pass);
}

/* Here are constructors */

static KfMUC *kf_muc_new (const gchar *jid, const gchar *nick, const gchar *pass) {
	KfMUC *self;
	GtkWidget *input, *send;
	GError *spell_err = NULL;
	GtkWidget *button_find, *find_dialog, *find_search, *find_close;
	const gchar *current_title;
	gchar *title;
	GtkWidget *scrolledwindow;

	kf_muc_init ();
	if (g_hash_table_lookup (kf_muc_rooms, jid)) {
		/* User has already joined this groupchat */
		return NULL;
	}

	self = g_new (KfMUC, 1);
	self->ref_count = 1;
	self->jid = g_strdup (jid);
	self->nick = g_strdup (nick);
	self->pass = g_strdup (pass);
	self->topic = NULL;
	self->tooltip_widget = NULL;
	self->tooltip_timeout = -1;

	self->glade = glade_xml_new (kf_find_file ("muc.glade"), NULL, NULL);
	kf_glade_get_widgets (self->glade,
			"muc", &self->window,
			"input", &input,
			"history", &self->history_view,
			"send", &send,
			"roster", &self->treeview,
			"topic_entry", &self->topic_entry,
			"configure", &self->configure,
			"status", &self->conf_status,
			"viewport", &self->conf_view,
			"change_buttonbox", &self->change_buttonbox,
			"button_change_enable", &self->change_enable,
			"muc_menu", &self->menu,

			"button_find", &button_find,
			"find_dialog", &find_dialog,
			"find_search", &find_search,
			"find_close", &find_close,
			
			"button_menu", &self->button_menu,
			"scrolledwindow2", &scrolledwindow,
			NULL);
	kf_muc_menu_connect_signals (self, self->glade);
	self->input = gtk_text_view_get_buffer (GTK_TEXT_VIEW (input));
	self->history = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->history_view));
	kf_emo_init_textview (GTK_TEXT_VIEW (self->history_view));
	g_object_set_data_full (G_OBJECT (self->window), "MUC", self, (GDestroyNotify) kf_muc_unref);
	g_hash_table_insert (kf_muc_rooms, self->jid, self);

	self->adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow));

	self->urls = NULL;
	self->tips = gtk_tooltips_new ();

	self->roster = kf_muc_roster_new (self);
	g_signal_connect (G_OBJECT (send), "clicked", G_CALLBACK (send_cb), self);
	g_signal_connect (G_OBJECT (input), "key-press-event",
			G_CALLBACK (textview_press_event), self);
	g_signal_connect (G_OBJECT (self->treeview), "button-press-event",
			G_CALLBACK (treeview_button_press_event), self);

	g_signal_connect_swapped (G_OBJECT (button_find), "clicked",
			G_CALLBACK (gtk_widget_show), find_dialog);
	g_signal_connect (G_OBJECT (find_dialog), "delete-event",
			G_CALLBACK (gtk_true), NULL);
	g_signal_connect (G_OBJECT (find_search), "clicked",
			G_CALLBACK (find_clicked_cb), self);
	g_signal_connect (G_OBJECT (find_close), "clicked",
			G_CALLBACK (kf_close_button), NULL);
	
	g_signal_connect (G_OBJECT (self->button_menu), "clicked",
			G_CALLBACK (on_button_menu_clicked), self);
	
	g_signal_connect (G_OBJECT (self->window), "delete-event",
			G_CALLBACK (muc_window_delete), self);
	kf_muc_toolbar_connect (self);

	/* Tooltip */
	g_signal_connect (G_OBJECT (self->treeview), "motion-notify-event", G_CALLBACK (kf_muc_roster_motion_notify), self);
	g_signal_connect (G_OBJECT (self->treeview), "leave-notify-event", G_CALLBACK (kf_muc_roster_leave_notify), self);


	self->notify = gtk_text_buffer_create_tag (self->history, "notify",
			"foreground", "#336699",
			"justification", GTK_JUSTIFY_CENTER,
			NULL);
	self->my_text = gtk_text_buffer_create_tag (self->history, "my_text",
			"foreground", "red",
			NULL);
	self->their_text = gtk_text_buffer_create_tag (self->history, "their_text",
			"foreground", "blue",
			NULL);
	self->error_text = gtk_text_buffer_create_tag (self->history, "error",
			"foreground", "red",
			NULL);
	
#ifdef HAVE_GTKSPELL
	if ( ! gtkspell_new_attach(GTK_TEXT_VIEW(input), NULL, &spell_err)) {
		foo_debug ("Unable to use GtkSpell: %s\n", spell_err->message);
	}
#endif
	
	kf_muc_enter_room (self);

	current_title = gtk_window_get_title (GTK_WINDOW (self->window));
	title = g_strdup_printf ("%s %s (%s)", current_title, jid, nick);
	gtk_window_set_title (GTK_WINDOW (self->window), title);
	g_free (title);
	
	return self;
}

static KfMUCRoster *kf_muc_roster_new (KfMUC *muc) {
	KfMUCRoster *self;

	self = g_new (KfMUCRoster, 1);
	self->ref_count = 1;
	self->muc = muc;

	self->items = g_hash_table_new (g_str_hash, g_str_equal);
	self->litems = NULL;

	self->store = gtk_list_store_new (1, G_TYPE_POINTER);
	kf_muc_roster_init_gui (self);
	return self;
}

static KfMUCRosterItem *kf_muc_roster_item_new (KfMUCRoster *roster, const gchar *name) {
	KfMUCRosterItem *self;

	self = g_new (KfMUCRosterItem, 1);
	self->ref_count = 1;
	self->roster = roster;
	self->jid = NULL;
	self->name = g_strdup (name);

	g_hash_table_insert (roster->items, self->name, self);
	roster->litems = g_list_prepend (roster->litems, self);

	self->presence = 0;
	self->status = NULL;
	self->tag = gtk_text_buffer_create_tag (roster->muc->history, NULL,
			"foreground", kf_muc_pick_color (),
			NULL);
	return self;
}

/* Here are reference managers & destructors */

static KfMUC *kf_muc_ref (KfMUC *self) {
	self->ref_count++;
	return self;
}
static KfMUCRoster *kf_muc_roster_ref (KfMUCRoster *self) {
	self->ref_count++;
	return self;
}
static KfMUCRosterItem *kf_muc_roster_item_ref (KfMUCRosterItem *self) {
	self->ref_count++;
	return self;
}

static void kf_muc_unref (KfMUC *self) {
	self->ref_count--;
	if (self->ref_count <= 0) {
		kf_muc_free (self);
	}
}
static void kf_muc_roster_unref (KfMUCRoster *self) {
	self->ref_count--;
	if (self->ref_count <= 0) {
		kf_muc_roster_free (self);
	}
}
static void kf_muc_roster_item_unref (KfMUCRosterItem *self) {
	self->ref_count--;
	if (self->ref_count <= 0) {
		kf_muc_roster_item_free (self);
	}
}

static void kf_muc_free (KfMUC *self) {
	kf_muc_leave_room (self);
	kf_muc_roster_unref (self->roster);
	g_hash_table_remove (kf_muc_rooms, self->jid);
	g_free (self->jid);
	g_free (self->nick);
	g_object_ref (G_OBJECT (self->tips));
	gtk_object_sink (G_OBJECT (self->tips));
	g_object_unref (G_OBJECT (self->tips));
	g_free (self);
}
static void kf_muc_roster_free (KfMUCRoster *self) {
	g_free (self);
}
static void kf_muc_roster_item_free (KfMUCRosterItem *self) {
	g_free (self);
}

static KfMUCRoster *kf_muc_get_roster (KfMUC *self) {
	return self->roster;
}

/* Internal run by kf_muc_new to join chatroom */

static void kf_muc_enter_room (KfMUC *self) {
	gchar *to;
	LmMessage *msg;
	GError *error = NULL;
	LmMessageNode *node;
	const gchar *status;
	const gchar *text;

	to = g_strdup_printf ("%s/%s", self->jid, self->nick);
	msg = lm_message_new (to, LM_MESSAGE_TYPE_PRESENCE);
	g_free (to);

	status = kf_preferences_get_string ("statusType");
	if (status && strcmp (status, "online") != 0)
		lm_message_node_add_child (msg->node, "show", status);

	text = kf_preferences_get_string ("statusText");
	if (text && ! kf_preferences_get_int ("mucHideStatus"))
		lm_message_node_add_child (msg->node, "status", text);

	if (self->pass) {
		lm_message_node_add_child (msg->node, "password", self->pass);
	}
	
	node = lm_message_node_add_child (msg->node, "x", NULL);
	lm_message_node_set_attribute (node, "xmlns", "http://jabber.org/protocol/muc");

	lm_connection_send (kf_jabber_get_connection (), msg, &error);
	lm_message_unref (msg);
}
static void kf_muc_leave_room (KfMUC *self) {
	gchar *to;
	LmMessage *msg;
	GError *error = NULL;

	to = g_strdup_printf ("%s/%s", self->jid, self->nick);
	msg = lm_message_new_with_sub_type (to,
			LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_UNAVAILABLE);
	g_free (to);

//	node = lm_message_node_add_child (msg->node, "x", NULL);
//	lm_message_node_set_attribute (node, "xmlns", "http://jabber.org/protocol/muc");

	lm_connection_send (kf_jabber_get_connection (), msg, &error);
	lm_message_unref (msg);
}


static KfMUC *kf_muc_get (const gchar *jid) {
	return g_hash_table_lookup (kf_muc_rooms, jid);
}


/* Message Handlers */
static LmHandlerResult kf_muc_mhandler_cb   (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data) {
	KfMUC *muc;
	gchar *from;
	gchar *room, *nick;
	LmMessageNode *node;
	const gchar *body;
	gulong timestamp = time (NULL);
	
	foo_debug ("MUC incoming message:\n%s\n", lm_message_node_to_string (message->node));
	if (lm_message_get_sub_type (message) != LM_MESSAGE_SUB_TYPE_GROUPCHAT &&
		lm_message_get_sub_type (message) != LM_MESSAGE_SUB_TYPE_ERROR) {
		foo_debug ("Not a MUC message!\n");
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	from = g_strdup (lm_message_node_get_attribute (message->node, "from"));
	room = from;

	for (nick = from; *nick != '\0' && *nick != '/'; nick++)
		;
	if (nick[0] == '\0') {
		nick = NULL;
	} else {
		*nick = '\0';
		nick++;
	}

	if ((muc = kf_muc_get (room)) == NULL) {
		/* No chat found */
		foo_debug ("No MUC found!\n");
		g_free (from);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	foo_debug ("Still in MUC control:-)\n");
	if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_ERROR) {
		kf_muc_error (muc, message);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

	node = lm_message_node_get_child (message->node, "subject");
	if (node) {
		kf_muc_topic_changed (muc, lm_message_node_get_value (node));
		node = NULL;
	}

	node = lm_message_node_get_child (message->node, "body");
	body = lm_message_node_get_value (node);

	/* Find any extra tags */
	for (node = message->node->children; node; node = node->next) {
		if (strcmp (node->name, "x") == 0) {
			const gchar *xmlns = lm_message_node_get_attribute (node, "xmlns");

			if (strcmp (xmlns, "jabber:x:delay") == 0) {
				const gchar *stamp = lm_message_node_get_attribute (node, "stamp");
				if (stamp)
					timestamp = x_stamp_to_time (stamp);
			}
		}
	}
	
	if (nick && *nick) 
		kf_muc_display_msg_simple (muc, nick, body, timestamp);
	else
		kf_muc_display_msg_event_simple (muc, nick, body);

	g_free (from);
		
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}


static LmHandlerResult kf_muc_phandler_cb   (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data) {
	KfMUC *muc;
	gchar *from;
	gchar *room, *nick;
	LmMessageNode *node;
	KfMUCRosterItem *item;
	const gchar *new_nick = NULL;
	gboolean nick_change = FALSE;
	
	foo_debug ("MUC incoming message:\n%s\n", lm_message_node_to_string (message->node));

	from = g_strdup (lm_message_node_get_attribute (message->node, "from"));
	room = from;

	for (nick = from; *nick != '\0' && *nick != '/'; nick++)
		;
	*nick = '\0';
	nick++;

	if ((muc = kf_muc_get (room)) == NULL) {
		/* No chat found */
		foo_debug ("No MUC found!\n");
		g_free (from);
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}
	foo_debug ("Still in MUC control:-)\n");

	if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_ERROR) {
		kf_muc_error (muc, message);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	foo_debug ("Looking for item %s...", nick);
	item = kf_muc_roster_get_item (muc->roster, nick);

	if (item && lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_UNAVAILABLE) {
		item->presence = 0;
		kf_muc_roster_update (muc->roster);

		g_hash_table_remove (muc->roster->items, item->name);
		muc->roster->litems = g_list_remove (muc->roster->litems, item);
		kf_muc_roster_item_unref (item);
		
		kf_muc_roster_update (muc->roster);
	
		/* TODO: we can't return here, in case user changes nickname we have to track that... */
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	
	if (item == NULL) {
		item = kf_muc_roster_item_new (muc->roster, nick);
		foo_debug ("Creating new one! [%s]\n", item->name);
	}
	foo_debug ("Found! [%s]\n", item->name);
	item->presence = 0;
	for (node = message->node->children; node; node = node->next) {
		if (strcmp (node->name, "show") == 0) {
			item->presence = kf_jabber_presence_type_from_string (
					lm_message_node_get_value (node));
		} else if (strcmp (node->name, "status") == 0) {
			g_free (item->status);
			item->status = g_strdup (lm_message_node_get_value (node));
		} else if (strcmp (node->name, "x") == 0) {
			const gchar *xmlns;

			xmlns = lm_message_node_get_attribute (node, "xmlns");
			if (strcmp (xmlns, "http://jabber.org/protocol/muc#user") == 0) {
				/* Do something ! */
				LmMessageNode *item_node;

				for (item_node = node->children; item_node; item_node = item_node->next) {
					const gchar *role;
					const gchar *affiliation;
					const gchar *jid;

					foo_debug ("Processing <%s>\n", item_node->name);
					
					if (strcmp (item_node->name, "item") == 0) {
						role = lm_message_node_get_attribute (item_node, "role");
						affiliation = lm_message_node_get_attribute (item_node, "affiliation");
						jid = lm_message_node_get_attribute (item_node, "jid");
						new_nick = lm_message_node_get_attribute (item_node, "nick");
					
						if (role)
							item->role = kf_muc_role_from_string (role);
						if (affiliation)
							item->affiliation = kf_muc_affiliation_from_string (affiliation);
						if (jid) {
							g_free (item->jid);
							item->jid = g_strdup (jid);
						}

						foo_debug ("item:\n");
						foo_debug ("  role: %s (%d)\n", role, item->role);
						foo_debug ("  affiliation: %s (%d)\n", affiliation, item->affiliation);
					} else if (strcmp (item_node->name, "status") == 0) {
						const gchar *code_str = lm_message_node_get_attribute (item_node, "code");
						gint code = atoi (code_str);
						
						if (code == 303) {
							/* Change nick */
							nick_change = TRUE;
							foo_debug ("nick_change = TRUE;\n");
						}						
					}
				}
			}
		}
	}
	foo_debug ("nick=%s;\nnew_nick=%s;\n", nick, new_nick);
	if (nick_change && strcmp (nick, muc->nick) == 0 && new_nick) {
		foo_debug ("X");
		g_free (muc->nick);
		muc->nick = g_strdup (new_nick);
	}
	foo_debug ("I am %s\n", muc->nick);
		
	kf_muc_roster_update (muc->roster);
	
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}
static LmHandlerResult kf_muc_ihandler_cb   (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data) {
	KfMUC *muc;
	const gchar *cfrom;
	gchar *from;
	gchar *room, *nick;
	foo_debug ("MUC incoming message:\n%s\n", lm_message_node_to_string (message->node));

	cfrom = lm_message_node_get_attribute (message->node, "from");
	if (cfrom == NULL)
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;

	from = g_strdup (cfrom);
	room = from;

	for (nick = from; *nick != '\0' && *nick != '/'; nick++)
		;
	*nick = '\0';
	nick++;

	if ((muc = kf_muc_get (room)) == NULL) {
		/* No chat found */
		foo_debug ("No MUC found!\n");
		g_free (from);
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}
	foo_debug ("Still in MUC control:-)\n");

	if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_ERROR) {
		kf_muc_error (muc, message);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

/* GUI functions */
static void kf_muc_display_msg_simple (KfMUC *self, const gchar *from, const gchar *body, gulong timestamp) {
	gboolean active = TRUE;
	gboolean incoming ;
	KfEmoTagTable style;
	KfMUCRosterItem *item;

	if (! body || ! *body)
		return;

	item = g_hash_table_lookup (self->roster->items, from);
	if (item == NULL) {
		foo_debug ("Unable to find %s\n", from);
	}
	
	style.stamp = NULL;
	style.in = item?item->tag:self->their_text;
	style.out = self->my_text;

	incoming = strcmp (from, self->nick);

	kf_display_message (self->history, timestamp, from, body, ! incoming, &style);

	/* Scroll only if window is active */
	g_object_get (G_OBJECT (self->window), "is-active", &active, NULL);
	if (self->adj->value == self->adj->upper - self->adj->page_size && active) {
		GtkTextIter iter;
		GtkTextMark *mark;
		gtk_text_buffer_get_end_iter (self->history, &iter);

		mark = gtk_text_buffer_create_mark (self->history, NULL, &iter, FALSE);
		
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (self->history_view), 
			mark , 0.0, TRUE, 0.0, 1.0);
	}

}
static void kf_muc_display_msg_event_simple (KfMUC *self, const gchar *from, const gchar *body) {
	GtkTextIter iter;
	gchar *text;
	gboolean active = TRUE;

	text = g_strdup_printf ("%s\n", body);
	gtk_text_buffer_get_end_iter (self->history, &iter);
	gtk_text_buffer_insert_with_tags (self->history, &iter, text, -1, self->notify, NULL);
//	gtk_text_buffer_get_end_iter (self->history, &iter);
//	gtk_text_buffer_get_end_iter (self->history, &iter);
//	gtk_text_buffer_insert_with_tags (self->history, &iter, "\n", 1, self->notify, NULL);
	g_free (text);

	g_object_get (G_OBJECT (self->window), "is-active", &active, NULL);
	if (active) {
		GtkTextIter iter;
		GtkTextMark *mark;
		gtk_text_buffer_get_end_iter (self->history, &iter);

		mark = gtk_text_buffer_create_mark (self->history, NULL, &iter, FALSE);
		
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (self->history_view), 
			mark , 0.0, TRUE, 0.0, 1.0);
	}
}
static void kf_muc_display_msg_error_simple (KfMUC *self, const gchar *from, gint num, const gchar *body) {
	GtkTextIter iter;
	gchar *text;
	gboolean active = TRUE;

	text = g_strdup_printf (_("Error #%d: %s\n"), num, body);
	gtk_text_buffer_get_end_iter (self->history, &iter);
	gtk_text_buffer_insert_with_tags (self->history, &iter, text, -1, self->error_text, NULL);
	g_free (text);

	g_object_get (G_OBJECT (self->window), "is-active", &active, NULL);
	if (active) {
		GtkTextIter iter;
		GtkTextMark *mark;
		gtk_text_buffer_get_end_iter (self->history, &iter);

		mark = gtk_text_buffer_create_mark (self->history, NULL, &iter, FALSE);
		
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (self->history_view), 
			mark , 0.0, TRUE, 0.0, 1.0);
	}
}




static void kf_muc_send_msg_simple (KfMUC *self, const gchar *body) {
	LmMessage *msg;
	LmMessageNode *node;
	GError *error = NULL;

	msg = lm_message_new_with_sub_type (self->jid,
			LM_MESSAGE_TYPE_MESSAGE, LM_MESSAGE_SUB_TYPE_GROUPCHAT);
	node = lm_message_node_add_child (msg->node, "body", body);

	lm_connection_send (kf_jabber_get_connection (), msg, &error);
	lm_message_unref (msg);
}

/* GUI Callbacks */
static void send_cb (GtkButton *button, gpointer data) {
	KfMUC *muc = data;
	GtkTextIter start, end;
	gchar *text;

	gtk_text_buffer_get_bounds (muc->input, &start, &end);
	text = gtk_text_buffer_get_text (muc->input, &start, &end, FALSE);
	if (*text != '\0') {
		kf_muc_send_msg_simple (muc, text);
		gtk_text_buffer_delete (muc->input, &start, &end);
	}
	g_free (text);
}

/* GUI Roster functions */
static void kf_muc_roster_update (KfMUCRoster *self) {
	GList *tmp;
	
	gtk_list_store_clear (self->store);
	for (tmp = self->litems; tmp; tmp = tmp->next) {
		GtkTreeIter iter;
		
		gtk_list_store_append (self->store, &iter);
		gtk_list_store_set (self->store, &iter, 0, tmp->data, -1);
	}
}

/* Roster Rendering functions */
static void kf_muc_roster_init_gui (KfMUCRoster *self) {
	GtkTreeView *view;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	view = GTK_TREE_VIEW (self->muc->treeview);
	/* Ustawienie GtkTreeView */	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, on_pixbuf_data, NULL, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, on_text_data, NULL, NULL);
	gtk_tree_view_append_column (view, column);

	gtk_tree_view_set_model (view, GTK_TREE_MODEL (self->store));
}

static
void on_text_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	KfMUCRosterItem *item;
	gtk_tree_model_get (model, iter, 0, &item, -1);
	if (item) {
		g_object_set (G_OBJECT (renderer), "text", item->name, NULL);
	} else {
		foo_debug ("No item found...\n");
	}
}

static
void on_pixbuf_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	KfMUCRosterItem *item;
	gtk_tree_model_get (model, iter, 0, &item, -1);
	if (item) {
		GdkPixbuf *pixbuf;
		
		pixbuf = kf_gui_pixmap_get_status (item->presence);
		g_object_set (G_OBJECT (renderer), "pixbuf", pixbuf, NULL);
	} else {
		foo_debug ("No item found...\n");
	}

}

/* Roster management functions */
static KfMUCRosterItem *kf_muc_roster_get_item (KfMUCRoster *self, const gchar *name) {
	return g_hash_table_lookup (self->items, name);
}


/* Again, MUC management functions */
static void kf_muc_topic_changed (KfMUC *self, const gchar *topic) {
	g_free (self->topic);
	self->topic = g_strdup (topic);

	gtk_entry_set_text (GTK_ENTRY (self->topic_entry), self->topic);
	gtk_tooltips_set_tip (self->tips, self->topic_entry, self->topic, NULL);
}

/* MUC Toolbar callbacks */

static void kf_muc_toolbar_connect (KfMUC *self) {
	GtkWidget *change;
	GtkWidget *configure;
	GtkWidget *change_enable;
	GtkWidget *change_cancel;

	kf_glade_get_widgets (self->glade,
			"button_change", &change,
			"button_configure", &configure,
			"button_change_enable", &change_enable,
			"button_change_cancel", &change_cancel,
			NULL);
	g_signal_connect (G_OBJECT (change), "clicked", G_CALLBACK (button_change_cb), self);
	g_signal_connect (G_OBJECT (change_enable), "clicked", G_CALLBACK (button_change_enable_cb), self);
	g_signal_connect (G_OBJECT (change_cancel), "clicked", G_CALLBACK (button_change_cancel_cb), self);
	g_signal_connect (G_OBJECT (configure), "clicked", G_CALLBACK (button_configure_cb), self);
}

static void button_change_enable_cb (GtkButton *button, gpointer data)
{
	KfMUC *self = data;

	gtk_widget_show (self->change_buttonbox);
	gtk_widget_hide (self->change_enable);
	gtk_entry_set_editable (GTK_ENTRY (self->topic_entry), TRUE);
	gtk_editable_select_region (GTK_EDITABLE (self->topic_entry), 0, -1);
	gtk_entry_set_has_frame (GTK_ENTRY (self->topic_entry), TRUE);
	gtk_widget_grab_focus (self->topic_entry);
}

static void button_change_cancel_cb (GtkButton *button, gpointer data)
{
	KfMUC *self = data;

	gtk_widget_hide (self->change_buttonbox);
	gtk_widget_show (self->change_enable);
	gtk_entry_set_editable (GTK_ENTRY (self->topic_entry), FALSE);
	gtk_editable_select_region (GTK_EDITABLE (self->topic_entry), 0, 0);
	gtk_entry_set_has_frame (GTK_ENTRY (self->topic_entry), FALSE);
	gtk_entry_set_text (GTK_ENTRY (self->topic_entry),
			self->topic?self->topic:NO_TOPIC);
}

static void button_change_cb (GtkButton *button, gpointer data) {
	KfMUC *self = data;
	LmMessage *msg;
	LmMessageNode *node;
	GError *error = NULL;
	const gchar *body;
//	gchar *from;
	gchar *text;

	body = gtk_entry_get_text (GTK_ENTRY (self->topic_entry));
	text = g_strdup_printf (_("/me has changed topic to '%s'"), body);
	msg = lm_message_new_with_sub_type (self->jid,
			LM_MESSAGE_TYPE_MESSAGE, LM_MESSAGE_SUB_TYPE_GROUPCHAT);
	node = lm_message_node_add_child (msg->node, "subject", body);
	node = lm_message_node_add_child (msg->node, "body", text);
	g_free (text);

	lm_connection_send (kf_jabber_get_connection (), msg, &error);
	lm_message_unref (msg);

	gtk_widget_hide (self->change_buttonbox);
	gtk_widget_show (self->change_enable);
	gtk_entry_set_editable (GTK_ENTRY (self->topic_entry), FALSE);
	gtk_editable_select_region (GTK_EDITABLE (self->topic_entry), 0, 0);
	gtk_entry_set_has_frame (GTK_ENTRY (self->topic_entry), FALSE);
	gtk_entry_set_text (GTK_ENTRY (self->topic_entry),
			self->topic?self->topic:NO_TOPIC);

}

static gboolean textview_press_event       (GtkWidget *widget,
                                            GdkEventKey *event,
                                            gpointer user_data)
{
	KfMUC *muc = user_data;
	if (event->keyval == GDK_Return) {
		
		/* Ugly, but works */
		if (event->state & GDK_SHIFT_MASK)	/* Shift */
			return FALSE;

		send_cb (NULL, muc);
		return TRUE;
	} else if (event->keyval == GDK_Tab) {
		/* Tab nickname completion */
		kf_muc_tab_completion (muc, GTK_TEXT_VIEW (widget));	
		return TRUE;		
	}
	return FALSE;
}

/* Error Handler */
static void kf_muc_error (KfMUC *self, LmMessage *msg) {
	LmMessageNode *node;
	const gchar *code;
	
	node = lm_message_node_get_child (msg->node, "error");
	code = lm_message_node_get_attribute (node, "code");
	if (node) {
		if (node->children)
			kf_muc_display_msg_error_simple (self, NULL, atoi (code), node->children->name);
		else
			kf_muc_display_msg_error_simple (self, NULL, atoi (code), node->value);
	} else {
		kf_muc_display_msg_error_simple (self, NULL, -1, "Generic error");
	}
}

/* Klikanei na rosterze */
static gboolean treeview_button_press_event(GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data)
{
	GtkTreeView *view = (GtkTreeView *) widget;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	KfMUCRosterItem *item;
	KfMUC *self = user_data;

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
			GtkWidget *menu;
			static gboolean connected = FALSE;

			menu = glade_xml_get_widget (self->glade, "menu");

			if (connected == FALSE) {
				kf_muc_popup_menu_connect (menu);
				connected = TRUE;
			}

//			gtk_menu_attach_to_widget (GTK_MENU (menu), self->topic_entry, gtk_false);
			gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, event->time);
			g_object_set_data (G_OBJECT (menu), "Item", item);
		}
		gtk_tree_path_free (path);
	}
	return FALSE;
}

static void kf_muc_popup_menu_connect (GtkWidget *menu) {
	GladeXML *glade;

	glade = glade_get_widget_tree (menu);
	kf_signal_connect (glade, "popup_message", "activate",
			G_CALLBACK (popup_menu_cb), GINT_TO_POINTER (1));
	kf_signal_connect (glade, "popup_chat", "activate",
			G_CALLBACK (popup_menu_cb), GINT_TO_POINTER (2));
	kf_signal_connect (glade, "popup_info", "activate",
			G_CALLBACK (popup_menu_cb), GINT_TO_POINTER (3));
	kf_signal_connect (glade, "popup_kick", "activate",
			G_CALLBACK (menu_set_role_cb), GINT_TO_POINTER (KF_MUC_ROLE_NONE));
	kf_signal_connect (glade, "popup_ban", "activate",
			G_CALLBACK (menu_set_aff_cb), GINT_TO_POINTER (KF_MUC_AFFILIATION_OUTCAST));
	kf_signal_connect (glade, "popup_grant_voice", "activate",
			G_CALLBACK (menu_set_role_cb), GINT_TO_POINTER (KF_MUC_ROLE_PARTICIPANT));
	kf_signal_connect (glade, "popup_grant_membership", "activate",
			G_CALLBACK (menu_set_aff_cb), GINT_TO_POINTER (KF_MUC_AFFILIATION_MEMBER));
	kf_signal_connect (glade, "popup_grant_moderator_privileges", "activate",
			G_CALLBACK (menu_set_role_cb), GINT_TO_POINTER (KF_MUC_ROLE_MODERATOR));
	kf_signal_connect (glade, "popup_grant_ownership", "activate",
			G_CALLBACK (menu_set_aff_cb), GINT_TO_POINTER (KF_MUC_AFFILIATION_OWNER));
	kf_signal_connect (glade, "popup_grant_administrator_privileges", "activate",
			G_CALLBACK (menu_set_aff_cb), GINT_TO_POINTER (KF_MUC_AFFILIATION_ADMIN));
	kf_signal_connect (glade, "popup_revoke_voice", "activate",
			G_CALLBACK (menu_set_role_cb), GINT_TO_POINTER (KF_MUC_ROLE_VISITOR));
	kf_signal_connect (glade, "popup_revoke_membership", "activate",
			G_CALLBACK (menu_set_aff_cb), GINT_TO_POINTER (KF_MUC_AFFILIATION_NONE));
	kf_signal_connect (glade, "popup_revoke_moderator_privileges", "activate",
			G_CALLBACK (menu_set_role_cb), GINT_TO_POINTER (KF_MUC_ROLE_PARTICIPANT));
	kf_signal_connect (glade, "popup_revoke_ownership", "activate",
			G_CALLBACK (menu_set_aff_cb), GINT_TO_POINTER (KF_MUC_AFFILIATION_ADMIN));
	kf_signal_connect (glade, "popup_revoke_administrator_privileges", "activate",
			G_CALLBACK (menu_set_aff_cb), GINT_TO_POINTER (KF_MUC_AFFILIATION_MEMBER));
}

/* Popup menu */

static void popup_menu_cb (GtkMenuItem *menuitem, gpointer data) {
	gint action = GPOINTER_TO_INT (data);
	KfMUCRosterItem *item;
	GladeXML *glade;
	GtkWidget *menu;
	gchar *jid;

	glade = glade_get_widget_tree (GTK_WIDGET (menuitem));
	menu = glade_xml_get_widget (glade, "menu");

	g_print ("ASSERT>>>");
	item = g_object_get_data (G_OBJECT (menu), "Item");
	g_assert (item != NULL);
	foo_debug ("Selected item: %s, %s\n", item->name, item->jid);

	if (item->jid) {
		jid = g_strdup (item->jid);
	} else {
		g_return_if_fail (item->roster);
		g_return_if_fail (item->roster->muc);
		g_return_if_fail (item->roster->muc->jid);
		jid = g_strdup_printf ("%s/%s", item->roster->muc->jid, item->name);
	}
	
	foo_debug ("jid='%s'\n", jid);
	switch (action) {
		case 1:	kf_new_message (jid);	break;
		case 2: kf_chat_window (jid);	break;
		case 3: kf_vcard_get (jid);	break;
		default:
			foo_debug ("Unknown action: %d\n", action);
	}

	g_free (jid);
}

static void menu_set_role_cb (GtkMenuItem *menuitem, gpointer data) {
	KfMUCRole role = GPOINTER_TO_INT (data);
	KfMUCRosterItem *item;
	GladeXML *glade;
	GtkWidget *menu;

	glade = glade_get_widget_tree (GTK_WIDGET (menuitem));
	menu = glade_xml_get_widget (glade, "menu");

	g_print ("ASSERT>>>");
	item = g_object_get_data (G_OBJECT (menu), "Item");
	g_assert (item != NULL);
	foo_debug ("Selected item: %s, %s\n", item->name, item->jid);
	kf_muc_roster_item_set_role (item, role);
}

static void menu_set_aff_cb (GtkMenuItem *menuitem, gpointer data) {
	KfMUCAffiliation aff = GPOINTER_TO_INT (data);
	KfMUCRosterItem *item;
	GladeXML *glade;
	GtkWidget *menu;

	glade = glade_get_widget_tree (GTK_WIDGET (menuitem));
	menu = glade_xml_get_widget (glade, "menu");

	g_print ("ASSERT>>>");
	item = g_object_get_data (G_OBJECT (menu), "Item");
	g_assert (item != NULL);
	foo_debug ("Selected item: %s, %s\n", item->name, item->jid);
	kf_muc_roster_item_set_affiliation (item, aff);
}


void kf_muc_roster_item_set_role (KfMUCRosterItem *item, KfMUCRole role) {
	LmMessage *msg;
	LmMessageNode *node;
	GError *error = NULL;

	msg = lm_message_new_with_sub_type (item->roster->muc->jid,
			LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "http://jabber.org/protocol/muc#admin");

	node = lm_message_node_add_child (node, "item", NULL);
	lm_message_node_set_attribute (node, "nick", item->name);
	lm_message_node_set_attribute (node, "role", kf_muc_role_to_string (role));

	foo_debug ("MUC message:\n%s\n", lm_message_node_to_string (msg->node));
	lm_connection_send (kf_jabber_get_connection (), msg, &error);
	lm_message_unref (msg);
}

void kf_muc_roster_item_set_affiliation (KfMUCRosterItem *item, KfMUCAffiliation aff) {
	LmMessage *msg;
	LmMessageNode *node;
	GError *error = NULL;

	msg = lm_message_new_with_sub_type (item->roster->muc->jid,
			LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "http://jabber.org/protocol/muc#admin");

	node = lm_message_node_add_child (node, "item", NULL);
	lm_message_node_set_attribute (node, "nick", item->name);
	lm_message_node_set_attribute (node, "affiliation", kf_muc_affiliation_to_string (aff));

	foo_debug ("MUC message:\n%s\n", lm_message_node_to_string (msg->node));
	lm_connection_send (kf_jabber_get_connection (), msg, &error);
	lm_message_unref (msg);
}


static const gchar *kf_muc_role_to_string (KfMUCRole role) {
	switch (role) {
		case KF_MUC_ROLE_NONE:
			return "none"; break;
		case KF_MUC_ROLE_MODERATOR:
			return "moderator"; break;
		case KF_MUC_ROLE_PARTICIPANT:
			return "participant"; break;
		case KF_MUC_ROLE_VISITOR:
			return "visitor"; break;
		default:
			return "none";
	}
}

static KfMUCRole kf_muc_role_from_string (const gchar *str) {
	gint i;
	
	for (i = 0; i < 3; i++) {
		if (strcmp (roles[i].str, str) == 0)
			return roles[i].code;
	}

	return KF_MUC_ROLE_NONE;
}

static const gchar *kf_muc_affiliation_to_string (KfMUCAffiliation aff) {
	switch (aff) {
		case KF_MUC_AFFILIATION_NONE:
			return "none"; break;
		case KF_MUC_AFFILIATION_OWNER:
			return "owner"; break;
		case KF_MUC_AFFILIATION_ADMIN:
			return "admin"; break;
		case KF_MUC_AFFILIATION_MEMBER:
			return "member"; break;
		case KF_MUC_AFFILIATION_OUTCAST:
			return "outcast"; break;
		default:
			return "none"; break;
	}
}

static KfMUCAffiliation kf_muc_affiliation_from_string (const gchar *str) {
	gint i;
	
	for (i = 0; i < 3; i++) {
		if (strcmp (affiliations[i].str, str) == 0)
			return affiliations[i].code;
	}

	return KF_MUC_AFFILIATION_NONE;
}


static void popup_menu_change_role (GtkMenuItem *item, gpointer data) {
}

static void button_configure_cb (GtkButton *button, gpointer data) {
	KfMUC *self = data;
	LmMessage *msg;
	LmMessageNode *node;
	LmMessageHandler *h;
	GError *error = NULL;
	static gboolean connected = FALSE;

	msg = lm_message_new_with_sub_type (self->jid,
			LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);

	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "http://jabber.org/protocol/muc#owner");
	h = lm_message_handler_new (configure_form_cb, self, NULL);
	lm_connection_send_with_reply (kf_jabber_get_connection (), msg, h, &error);
	lm_message_unref (msg);

	if (connected == FALSE) {
		GtkWidget *ok;
		GtkWidget *cancel;

		kf_glade_get_widgets (self->glade,
				"conf_ok", &ok,
				"conf_cancel", &cancel,
				NULL);

		g_signal_connect (G_OBJECT (self->configure), "delete-event", G_CALLBACK (conf_close_window), self);
		g_signal_connect (G_OBJECT (ok), "clicked", G_CALLBACK (conf_ok_clicked), self);
		g_signal_connect (G_OBJECT (cancel), "clicked", G_CALLBACK (conf_cancel_clicked), self);
		
		connected = TRUE;
	}

	gtk_widget_hide (self->conf_view);
	gtk_widget_show (self->conf_status);
	gtk_widget_show (self->configure);

	g_object_set_data_full (G_OBJECT (self->configure), "Handler", h, (GDestroyNotify) lm_message_handler_unref);
}

static LmHandlerResult configure_form_cb    (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer data) {
	KfMUC *self = data;
	LmMessageNode *node;
	GtkWidget *form = NULL;
	GtkRequisition req;
	gint width, height;
	LmMessageNode *error;
	
	foo_debug ("MUC reply message:\n%s\n", lm_message_node_to_string (message->node));
	
	error = lm_message_node_find_child (message->node, "error");
	if (error) {
		if (error->children && error->children->name &&
				strcmp (error->children->name, "forbidden") == 0) {
			kf_gui_alert (_("You are forbidden to configure this room"));
		} else if (error->value) {
			gchar *text;
			const gchar *code;
			
			code = lm_message_node_get_attribute (error, "code");
			text = g_strdup_printf (_("Error #%s: %s"), code, error->value);
			
			kf_gui_alert (text);
			g_free (text);
		} else {
			kf_gui_alert (_("An error occurred while trying to fetch form"));
		}
		gtk_widget_hide (self->configure);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	
	node = message->node->children;
	for (node = node->children; node; node = node->next) {
		const gchar *xmlns;

		xmlns = lm_message_node_get_attribute (node, "xmlns");
		
		if (strcmp (node->name, "x") == 0 && strcmp (xmlns, "jabber:x:data") == 0) {
			form = kf_x_data_form (node->children);
			gtk_widget_show (form);
			gtk_container_add (GTK_CONTAINER (self->conf_view), form);
		}
	}

	if (form == NULL) {
		kf_gui_alert ("There was no form...");
//		g_error ("No form!");
	}

	gtk_widget_size_request (form, &req);
	foo_debug ("Form is %dx%d\n", req.width, req.height);
	width = CLAMP (req.width + 28, 120, 600);
	height = CLAMP (req.height + 100, 120, 400);
	gtk_window_resize (GTK_WINDOW (self->configure), width, height);
	
	gtk_widget_hide (self->conf_status);
	gtk_widget_show (self->conf_view);
	g_object_set_data (G_OBJECT (self->configure), "Handler", NULL);
	g_object_set_data (G_OBJECT (self->configure), "Form", form);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static void conf_ok_clicked (GtkWidget *widget, gpointer data) {
	KfMUC *self = data;
	if (g_object_get_data (G_OBJECT (self->configure), "Handler")) {
	} else {
		LmMessage *msg;
		LmMessageNode *node;
		LmMessageHandler *h;
		GSList *list;
		GtkWidget *table;
		GError *error = NULL;

		
		msg = lm_message_new_with_sub_type (self->jid,
			LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
		node = lm_message_node_add_child (msg->node, "query", NULL);
		lm_message_node_set_attribute (node, "xmlns", "http://jabber.org/protocol/muc#owner");
		table = GTK_BIN (self->conf_view)->child;
		list = g_object_get_data (G_OBJECT (table), "Fields");
		
		g_assert (list);

		kf_x_data_create_node (node, list);
		foo_debug ("Submitting:\n%s\n", lm_message_node_to_string (msg->node));

		h = lm_message_handler_new (conf_submit, (gpointer *) self, NULL);


		lm_connection_send_with_reply (kf_jabber_get_connection (), msg, h, &error);
		lm_message_unref (msg);

		conf_done (self);
	}
}

static LmHandlerResult conf_submit          (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer data) {
	LmMessageNode *node;

	node = lm_message_node_find_child (message->node, "error");
	if (node) {
		kf_gui_alert (_("Error while submitting data..."));
	} else {
		kf_gui_alert (_("Room has been configured."));
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static void conf_cancel_clicked (GtkWidget *widget, gpointer data) {
	KfMUC *self = data;
	if (g_object_get_data (G_OBJECT (self->configure), "Handler")) {
	} else {
		gtk_widget_hide (self->configure);
		conf_done (self);
	}
}

static gboolean conf_close_window (GtkWidget *widget, GdkEvent *event, gpointer data) {
	KfMUC *self = data;
	if (g_object_get_data (G_OBJECT (self->configure), "Handler")) {
	} else {
		gtk_widget_hide (self->configure);
		conf_done (self);
	}
	return TRUE;
}

static void conf_done (KfMUC *self) {
	gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (self->conf_view)));
	g_object_set_data (G_OBJECT (self->configure), "Form", NULL);
	gtk_widget_hide (self->configure);
}

static void find_clicked_cb (GtkButton *button, gpointer data) {
	KfMUC *self = data;
	GladeXML *glade = glade_get_widget_tree (GTK_WIDGET (button));
	GtkWidget *entry = glade_xml_get_widget (glade, "find_entry");
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
	GtkTextIter iter, start, end;

	gtk_text_buffer_get_iter_at_mark (self->history, &iter, gtk_text_buffer_get_insert (self->history));
	if (gtk_text_iter_forward_search (&iter, text, 0, &start, &end, NULL) ) {
		GtkTextMark *insert, *selection_bound;

		selection_bound = gtk_text_buffer_get_selection_bound (self->history);
		gtk_text_buffer_move_mark (self->history, selection_bound, &start);
		insert = gtk_text_buffer_get_insert (self->history);
		gtk_text_buffer_move_mark (self->history, insert, &end);
	}
}







/* Tooltip */

static gboolean kf_muc_roster_motion_notify (GtkWidget *tv, GdkEventMotion *event, gpointer data)
{
	KfMUC *self = data;
	if (self->tooltip_timeout >= 0) {
		/* Remove old timeout */
		gtk_timeout_remove (self->tooltip_timeout);
		self->tooltip_timeout = -1;
	}
	if (self->tooltip_widget && (event->y < self->tooltip_cell.y || event->y > self->tooltip_cell.y + self->tooltip_cell.height)) {
		/* Pointer has left the list item */
		hide_tooltip (self->tooltip_widget);
		self->tooltip_widget = NULL;
	}
	
//	tooltip_timeout = gtk_timeout_add (kf_preferences_get_int ("tooltipDelay"),
	self->tooltip_timeout = gtk_timeout_add (800,
				kf_muc_roster_tooltip_timeout, self);
	self->tooltip_x = event->x;
	self->tooltip_y = event->y;
	return FALSE;

}


static gboolean kf_muc_roster_leave_notify (GtkWidget *tv, GdkEventCrossing *event, gpointer data)
{
	KfMUC *self = data;

	if (self->tooltip_timeout >= 0) {
		gtk_timeout_remove (self->tooltip_timeout);
		self->tooltip_timeout = -1;
	}
	if (self->tooltip_widget) {
		hide_tooltip (self->tooltip_widget);
		self->tooltip_widget = NULL;
	}
	return FALSE;

}


static gboolean kf_muc_roster_tooltip_timeout (gpointer data)
{
	KfMUC *self = data;
	gint x, y;
	gint tx, ty;
	GtkTreePath *path;

	if (self->tooltip_widget) {
		hide_tooltip (self->tooltip_widget);
		self->tooltip_widget = NULL;
	}
	x = self->tooltip_x;
	y = self->tooltip_y;

	gtk_tree_view_widget_to_tree_coords (GTK_TREE_VIEW (self->treeview), x,y, &tx,&ty);
//	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (data), tx, ty, &path, NULL, NULL, NULL)) {
	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (self->treeview), x, y, &path, NULL, NULL, NULL)) {
		GtkTreeIter iter;
		GtkTreeModel *model;
		KfMUCRosterItem *item = NULL;

		gtk_tree_view_get_cell_area (GTK_TREE_VIEW (self->treeview), path, NULL, &self->tooltip_cell);

		model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->treeview));
		if (!gtk_tree_model_get_iter (model, &iter, path))
			return FALSE;

		gtk_tree_model_get (model, &iter, 0, &item, -1);
		if (item) {
			self->tooltip_widget = show_tooltip (item);
		}
	}

	return FALSE;
}

static GtkWidget *show_tooltip (KfMUCRosterItem *item) {
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

static gchar *tooltip_text (KfMUCRosterItem *item) {
	gchar *text;

	text = g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>\n"
				"<b>JID:</b> %s\n"
				"<b>Role:</b> %s\n"
				"<b>Affiliation:</b> %s\n"
				"<b>Status:</b> %s",
				item->name,
				item->jid?item->jid:"<i>unknown</i>",
				kf_muc_role_to_string (item->role),
				kf_muc_affiliation_to_string (item->affiliation),
				item->status?kf_jabber_presence_type_to_string (item->presence):item->status);

	return text;
}


/* Status has changed... */
void kf_muc_status_changed (gint type, const gchar *text)
{
	KfMUCStatusParams params;
	
	params.type = type;
	params.type_str = (type > KF_JABBER_PRESENCE_TYPE_ONLINE && type < KF_JABBER_PRESENCE_TYPE_INVISIBLE)?kf_jabber_presence_type_to_string (type):NULL;
	params.status = kf_preferences_get_int ("mucHideStatus")?NULL:text;
	
	if (kf_muc_rooms == NULL)
		return;

	g_hash_table_foreach (kf_muc_rooms, kf_muc_status_changed_foreach, &params);
}

static void kf_muc_status_changed_foreach   (gpointer key,
                                             gpointer value,
                                             gpointer user_data)
{
	KfMUCStatusParams *params = user_data;
	KfMUC *self = value;
	LmMessage *msg;
	gchar *to;
	GError *error = NULL;
	
	to = g_strdup_printf ("%s/%s", self->jid, self->nick);
	msg = lm_message_new (to, LM_MESSAGE_TYPE_PRESENCE);
	g_free (to);
	
	if (params->type_str)
		lm_message_node_add_child (msg->node, "show", params->type_str);

	if (params->status)
		lm_message_node_add_child (msg->node, "status", params->status);

	lm_connection_send (kf_jabber_get_connection (), msg, &error);
	lm_message_unref (msg);
}

static void kf_muc_tab_completion (KfMUC *muc, GtkTextView *view) {
	gchar *nick;
	GtkTextIter insert, start;
	guint len = 0;
	gchar *match = NULL;
/*	
	gtk_text_buffer_get_iter_at_mark (muc->input, &insert,
			gtk_text_buffer_get_insert (muc->input));
	
	start = insert;
	
	while (! gtk_text_iter_starts_word (&start))
		gtk_text_iter_backward_char (&start);
*/
	GtkTextBuffer *buffer;
	gboolean is_start_of_buffer;
// buffer = muc->input;

//	nick = gtk_text_iter_get_text (&start, &insert);
	gtk_text_buffer_get_iter_at_mark (muc->input, &insert, gtk_text_buffer_get_insert (muc->input));
	gtk_text_buffer_get_iter_at_mark (muc->input, &start, gtk_text_buffer_get_insert (muc->input));
	gtk_text_iter_backward_word_start (&start);
	is_start_of_buffer = gtk_text_iter_is_start (&start);
	nick = gtk_text_buffer_get_text (muc->input, &start, &insert, FALSE);

	len = strlen (nick);
	foo_debug ("TAB completion for '%s'\n", nick);

	if (nick && *nick && muc->roster) {
		GList *tmp;

		/* Find matching strings */

		for (tmp = muc->roster->litems;	tmp; tmp = tmp->next) {
			KfMUCRosterItem *item = tmp->data;

			if (strncasecmp (nick, item->name, len) == 0) {
				foo_debug (" -> match: '%s'\n",
						item->name);
				match = item->name;
				break;
			}
		}
	}

	if (match) {
		/* Insert and select it */
		GtkTextMark *mark;
		GtkTextMark *selection_bound;
		
		mark = gtk_text_buffer_create_mark (muc->input, NULL, &insert, TRUE);
		selection_bound = gtk_text_buffer_get_selection_bound  (muc->input);
//		gtk_text_buffer_move_mark (muc->input, selection_bound, &insert);
		gtk_text_buffer_insert (muc->input, &insert, match + len, -1);
		if (is_start_of_buffer)
			gtk_text_buffer_insert (muc->input, &insert, ": ", 2);
/*		gtk_text_buffer_get_iter_at_mark (muc->input, &insert, mark);
		gtk_text_buffer_move_mark (muc->input, selection_bound, &insert);
		gtk_text_buffer_delete_mark (muc->input, mark);*/
	}
	
	g_free (nick);
}

static void kf_muc_icons_changed_foreach    (gpointer key,
                                             gpointer value,
                                             gpointer user_data);

/*
 * Notify that user has changed icon set, so that all
 * chatroom windows can change icons in their rosters
 */
void kf_muc_icons_changed (void) {
	if (kf_muc_rooms)
		g_hash_table_foreach (kf_muc_rooms, kf_muc_icons_changed_foreach, NULL);
}


static void kf_muc_icons_changed_foreach    (gpointer key,
                                             gpointer value,
                                             gpointer user_data)
{
	KfMUC *muc = value;
	kf_muc_roster_update (muc->roster);
}


/*
   Menu
 */

static void position_menu                   (GtkMenu *menu,
                                             gint *x,
                                             gint *y,
                                             gboolean *push_in,
                                             gpointer user_data);

static void on_button_menu_clicked  (GtkButton *button, gpointer data) {
	KfMUC *self = data;
	gtk_menu_popup (GTK_MENU (self->menu), NULL, NULL, position_menu, self, 0, 0);
}

static void position_menu                   (GtkMenu *menu,
                                             gint *x,
                                             gint *y,
                                             gboolean *push_in,
                                             gpointer data)
{
	KfMUC *self = data;
	gint wx,wy;
	gint height, width;
	GtkWidget *w = gtk_widget_get_parent (self->button_menu);
	gdk_window_get_origin (w->window, &wx, &wy);
	gdk_drawable_get_size (GDK_DRAWABLE (w->window), &width, &height);

	*x = wx;
	*y = wy + height;
	*push_in = TRUE;
}


static KfURL *kf_url_new (const gchar *nick, const gchar *url) {
	KfURL *kfurl = g_new0 (KfURL, 1);
	kfurl->nick = g_strdup (nick);
	kfurl->url = g_strdup (url);
	kfurl->time = time (NULL);

	return kfurl;
}


static void kf_url_free (KfURL *kfurl) {
	g_free (kfurl->nick);
	g_free (kfurl->url);
	g_free (kfurl);
}


static void kf_muc_grab_url (KfMUC *self, gchar *nick, gchar *url) {
}


static void kf_muc_close (KfMUC *self) {
	GtkWidget *win = gtk_message_dialog_new (GTK_WINDOW (self->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_OK_CANCEL,
		_("Are you sure you want to leave this conference?"));
	gtk_dialog_set_default_response (GTK_DIALOG (win), GTK_RESPONSE_CANCEL);
	gint response = gtk_dialog_run (GTK_DIALOG (win));
	gtk_widget_destroy (win);
	if (response == GTK_RESPONSE_OK)
		gtk_widget_destroy (self->window);
	
}


static gboolean muc_window_delete (GtkWidget *tv, GdkEvent *event, gpointer data) {
	KfMUC *self = data;
	kf_muc_close (self);
	return TRUE;
}


static void kf_muc_menu_connect_signals (KfMUC *self, GladeXML *glade) {
	GtkWidget *nick_change;
	GtkWidget *url_grabber;
	GtkWidget *quit;
	kf_glade_get_widgets (glade,
			"menu_change_nick", &nick_change,
			"menu_url_grabber", &url_grabber,
			"menu_quit", &quit,
			NULL);
	g_signal_connect (G_OBJECT (nick_change), "activate",
			G_CALLBACK (menu_change_nick_clicked), self);
	g_signal_connect (G_OBJECT (url_grabber), "activate",
			G_CALLBACK (menu_url_grabber_clicked), self);
	g_signal_connect (G_OBJECT (quit), "activate",
			G_CALLBACK (menu_quit_clicked), self);
}


static void menu_change_nick_clicked (GtkButton *button, gpointer data) {
	KfMUC *self = data;
}


static void menu_url_grabber_clicked (GtkButton *button, gpointer data) {
	KfMUC *self = data;
}


static void menu_quit_clicked (GtkButton *button, gpointer data) {
	KfMUC *self = data;
	kf_muc_close (self);
}


/**
 * \brief Pick a random color from list of MUC colors
 **/
static gchar *kf_muc_pick_color (void) {
	extern GSList *kf_preferences_muc_colors;
	const gchar *color_str;

	if (kf_preferences_muc_colors) {
		color_str = g_slist_nth_data (kf_preferences_muc_colors,
				g_random_int () % g_slist_length (kf_preferences_muc_colors));
		return color_str?color_str:"yellow";
	} else {
		return "green";
	}
}


/**
 * \brief Get a list of conferences
 * \return a newly-allocated GList of KfMUC representing active conferences
 **/
GList *kf_muc_get_list (void)
{
	GList *ret = NULL;
	if (kf_muc_rooms)
		g_hash_table_foreach (kf_muc_rooms, kf_muc_get_list_foreach, &ret);
	return ret;
}


/**
 * \brief Single iteration of a foreach 'loop' in \a kf_muc_get_list
 **/
static void kf_muc_get_list_foreach (gpointer key, gpointer value, gpointer user_data)
{
	GList **ret = user_data;
	*ret = g_list_prepend (*ret, value);
}


/**
 * \brief Get a string describing this room
 * \param muc Conference
 * \return a newly-allocated string, which should be freed with g_free when no longer needed
 **/
gchar *kf_muc_get_signature (KfMUC *muc)
{
	g_return_if_fail (muc);
	return g_strdup_printf ("%s (%s)", muc->jid, muc->nick);
}


/**
 * \brief Invite a contact to conference
 * \param muc Conference
 * \param jid JabberID of contact you wish to invite to this conference
 **/
void kf_muc_invite_jid (KfMUC *muc, const gchar *jid)
{
	LmMessage *msg;
	LmMessageNode *x;
	LmMessageNode *invite;
	GError *error = NULL;
//	LmMessageNode *body;	/* For compatibility */
	
	g_return_if_fail (muc);
	g_return_if_fail (jid);

	foo_debug ("Inviting %s to %s\n", jid, muc->jid);

	msg = lm_message_new (muc->jid, LM_MESSAGE_TYPE_MESSAGE);
	x = lm_message_node_add_child (msg->node, "x", NULL);
	lm_message_node_set_attribute (x, "xmlns", "http://jabber.org/protocol/muc#user");
	invite = lm_message_node_add_child (x, "invite", NULL);
	lm_message_node_set_attribute (invite, "to", jid);
	/* TODO: we could put a reason here */

	lm_connection_send (kf_jabber_get_connection (), msg, &error);
	lm_message_unref (msg);

}

