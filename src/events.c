/* file events.c */
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
#include <time.h>
#include <string.h>
#include "kf.h"
#include "gui.h"
#include "jabber.h"
#include "events.h"
#include "chat.h"
#include "message.h"
#include "preferences.h"
#include "connection.h"
#include "sound.h"
#include "gtktip.h"


/* Notify window */
static GtkWidget *event_box = NULL;
/* ID of timeout, that hides notify window */
static gint timeout_id = 0;


static gboolean on_event_box_button_press  (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data);
static KfJabberRosterItem *event_item (KfEventClass klass, gpointer user_data);
static gchar *event_text (KfEventClass klass,KfJabberRosterItem *item,  gpointer user_data);
static GdkPixbuf *event_pixbuf (KfEventClass klass, KfJabberRosterItem *item);
static gboolean event_box_timeout (gpointer user_data);
static void kf_event_sound (KfEventClass klass);

void kf_event (KfEventClass klass, gpointer user_data) {
	GtkWidget *tip;
	gchar *text;
	gint screen_width, screen_height;
	gint tip_width, tip_height;
	KfJabberRosterItem *item;
	static gchar *keys[] = {"enablePopupMsg", "enablePopupChat","enablePopupPresence",NULL,"enablePopupOffline"};

	item = event_item (klass, user_data);
	
	if (klass == KF_EVENT_CLASS_PRESENCE && item && item->presence == KF_JABBER_PRESENCE_TYPE_UNAVAILABLE)
		kf_event_sound (KF_EVENT_CLASS_OFFLINE);
	else
		kf_event_sound (klass);

	if (klass == KF_EVENT_CLASS_PRESENCE && item) {
		time_t czas;
		struct tm *tajm;
		gchar *text;
		text = g_strdup_printf (_("%s is now %s"), item->display_name,
			kf_jabber_presence_type_to_human_string (item->presence));
		czas = time (NULL);
		tajm = localtime (&czas);
		g_print (" * [%02d:%02d] %s\n", tajm->tm_hour, tajm->tm_min, text);
		g_free (text);

		if (item->status && *(item->status)) {
			g_print ("   %s\n", item->status);
		}
	}
	
	if (! kf_preferences_get_int ("enablePopups"))
		return;

	if (time (NULL) < kf_connection_get_time () + 10)
		/* User has logged in in last 10 seconds */
		return;

	/* Additional checks */
	if (klass <= KF_EVENT_CLASS_OFFLINE && keys[klass] && kf_preferences_get_int (keys[klass])) {
		text = event_text (klass, item, user_data);
		tip = kf_tip_new (event_pixbuf (klass, item), text);
		g_free (text);

		/* Place tip window */
		gtk_tip_get_size (GTK_TIP (tip), &tip_width, &tip_height);
		screen_width = gdk_screen_width ();
		screen_height = gdk_screen_height ();
		gtk_window_move (GTK_WINDOW (tip),	screen_width - tip_width - 16,
						screen_height - tip_height - 16);

		/* Hide notify window in 10 seconds */
		if (timeout_id != 0) {
			gtk_timeout_remove (timeout_id);
			gtk_widget_destroy (event_box);
		}
		timeout_id = gtk_timeout_add (kf_preferences_get_int ("tooltipTimeout"),
				event_box_timeout, tip);
		gtk_widget_add_events (tip, GDK_BUTTON_PRESS_MASK);
		g_signal_connect (G_OBJECT (tip), "button-press-event",
				G_CALLBACK (on_event_box_button_press), NULL);
		gtk_widget_show (tip);	
		event_box = tip;
	}
}


static gboolean on_event_box_button_press  (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data){
	gtk_widget_destroy (widget);
	gtk_timeout_remove (timeout_id);
	timeout_id = 0;
	return TRUE;
}


static KfJabberRosterItem *event_item (KfEventClass klass, gpointer user_data) {
	KfJabberRosterItem *item = NULL;
	if (klass == KF_EVENT_CLASS_MESSAGE || klass == KF_EVENT_CLASS_CHAT) {
		KfJabberMessage *msg;
		gchar *jid;

		msg = user_data;
		jid = g_strdup (msg->from);
		kf_jabber_jid_crop (jid);

		item = kf_jabber_roster_item_get (jid);
		g_free (jid);
	} else if (klass == KF_EVENT_CLASS_PRESENCE) {
		item = user_data;
	}
	return item;
}

static gchar *event_text (KfEventClass klass, KfJabberRosterItem *item, gpointer user_data) {
	gchar *text = NULL;
	if (klass == KF_EVENT_CLASS_MESSAGE || klass == KF_EVENT_CLASS_CHAT) {
		KfJabberMessage *msg = user_data;
		gchar *head;
		gchar *body;
		gint len;
		
		len = g_utf8_strlen (msg->body, -1);
		if (len > 63) {
			gchar buffer[124];
			g_utf8_strncpy (buffer, msg->body, 60);
			g_strlcat (buffer, "...", 124);

			body = g_markup_escape_text (buffer, -1);
		} else {
			body = g_markup_escape_text (msg->body, -1);	
		}

		head = g_strdup_printf (klass == KF_EVENT_CLASS_MESSAGE ? _("Message from %s") : _("Chat with %s"),
				item?item->display_name:msg->from);
		text = g_strdup_printf ("<big>%s</big>\n<i>%s</i>", head, body);
		g_free (head);
		g_free (body);
	} else if (klass == KF_EVENT_CLASS_PRESENCE) {
		gchar *head;
		gchar *body;
		head = g_strdup_printf (_("%s is now %s"), item->display_name,
				kf_jabber_presence_type_to_human_string (item->presence));
		body = g_markup_escape_text (item->status?item->status:"", -1);

		text = g_strdup_printf ("<big>%s</big>\n<i>%s</i>", head, body);
		g_free (head);
		g_free (body);
	}
	return text;
}


static GdkPixbuf *event_pixbuf (KfEventClass klass, KfJabberRosterItem *item) {
	if (klass == KF_EVENT_CLASS_MESSAGE) {
		return kf_gui_pixmap_get_status (8);
	} else if (klass == KF_EVENT_CLASS_CHAT) {
		return kf_gui_pixmap_get_status (9);
	} else if (klass == KF_EVENT_CLASS_PRESENCE) {
		return kf_gui_pixmap_get_status (item->presence);
	}
	return NULL;
}


static gboolean event_box_timeout (gpointer user_data) {
	gtk_widget_destroy (user_data);
	timeout_id = 0;
	return FALSE;
}

static void kf_event_sound (KfEventClass klass) {
	static gchar *ids[] = {"Msg", "Chat", "Presence", "", "Offline"};
	gchar *pref_name;
	static time_t last_sound = 0;
	gint delay = kf_preferences_get_int ("soundSmartDelay");

	if (time (NULL) - last_sound <= delay && last_sound != 0)
		return;
	
	pref_name = g_strdup_printf ("sound%sUse", ids[klass]);
	if (kf_preferences_get_int (pref_name)) {
		gchar *pref_file = g_strdup_printf ("sound%s", ids[klass]);

		kf_sound_play (kf_preferences_get_string (pref_file));
		g_free (pref_file);
	}
	g_free (pref_name);

	last_sound = time (NULL);
}


