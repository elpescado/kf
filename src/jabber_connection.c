/* file jabber_connection.c */
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


/*
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
*/

#include <loudmouth/loudmouth.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "kf.h"
#include "jabber.h"
#include "jabber_msg.h"
#include "gui.h"
#include "message.h"
#include "chat.h"
#include "search.h"
#include "events.h"
#include "connection.h"
#include "statusbar.h"
#include "accounts.h"
#include "subscribe.h"

/* Zmienne */
gboolean kf_jabber_is_connected = FALSE;
LmConnection *kf_jabber_connection = NULL;
GError *error = NULL;
KfJabberConnection kf_jabber_connection_settings = {FALSE, NULL,NULL,NULL,NULL,5,0,FALSE, NULL};
/*
LmHandlerResult kf_jabber_message_hendel    (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);*/
LmHandlerResult kf_jabber_presence_hendel   (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);
LmHandlerResult kf_jabber_iq_hendel         (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);
LmHandlerResult kf_jabber_roster_hendel     (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);

guint32 x_stamp_to_time (const gchar *);

/* Prototypy funkcji lokalnych */
void        kf_jabber_connect_callback      (LmConnection *connection,
                                             gboolean success,
                                             gpointer user_data);
void        kf_jabber_auth_callback      (LmConnection *connection,
                                             gboolean success,
                                             gpointer user_data);

void        kf_jabber_disconnect_callback   (LmConnection *connection,
                                             LmDisconnectReason reason,
                                             gpointer user_data);
/*
LmHandlerResult kf_jabber_message_hendel    (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);*/
KfJabberPresenceType kf_jabber_presence_type_from_string (const gchar *string);

static void kf_jabber_send_presence_to_jidx (const gchar *type, const gchar *status, const gchar *jid);

/* Logowanie wiadomo¶ci */
extern void kf_message_log (KfJabberMessage *msg, gboolean yours);


/* Koniec deklaracji... */

/* Funkcja zwrotna wywo³ywana po po³±czeniu siê */
/* Obs³úguje logowanie siê na serwer */
void        kf_jabber_connect_callback      (LmConnection *connection,
                                             gboolean success,
                                             gpointer user_data) {
	if (success) {
		g_print ("Connected...\n");

		if (kf_jabber_connection_settings.resource == NULL ||
				*(kf_jabber_connection_settings.resource) == '\0')
			 kf_jabber_connection_settings.resource = "kf";
		
		lm_connection_authenticate (
				kf_jabber_connection,
				kf_jabber_connection_settings.username,
				kf_jabber_connection_settings.password,
				kf_jabber_connection_settings.resource,
				kf_jabber_auth_callback,
				NULL,
				NULL,
				&error);
		g_print ("Trying to log in...\n");
		kf_statusbar_set_text (_("Logging in..."));
	} else {
		kf_jabber_connection_settings.set = FALSE;
		kf_jabber_is_connected = FALSE;
		kf_gui_alert (_("I can't connect!!!")); /* ??? */
		g_print ("Disconnected...\n");
		kf_statusbar_set_text (NULL);
	}
}

time_t kf_connection_time = 0;

/* Funkcja zwrotna wywo³ywana po zalogowaniu siê na serwerze */
void        kf_jabber_auth_callback      (LmConnection *connection,
                                             gboolean success,
                                             gpointer user_data) {
	if (success) {
//		LmMessage *msg;
		g_print ("Logged in...\n");
		kf_jabber_is_connected = TRUE;

		kf_jabber_get_roster ();

		kf_connection_time = time (NULL);
	} else {
		kf_jabber_connection_settings.set = FALSE;
		kf_jabber_is_connected = FALSE;
		kf_gui_alert (_("I have failed to log in..."));
		g_print ("Error logging in...\n");
	}
		kf_statusbar_set_text (NULL);

}


/* Po³±czenie z serwerem */
gboolean kf_jabber_connect (void) {
	LmMessageHandler *h, *hp, *hi;

	if (!kf_jabber_connection_settings.set) {
		kf_accounts_connection_settings ();
		return FALSE;
	}
	
	if (kf_jabber_connection) {
		lm_connection_unref (kf_jabber_connection);
	}
	kf_jabber_connection = lm_connection_new (kf_jabber_connection_settings.server);
	lm_connection_set_port (kf_jabber_connection, kf_jabber_connection_settings.port);
	
#ifdef HAVE_LM_CONNECTION_SET_JID
	if (kf_jabber_connection_settings.manual_host) {
		gchar *jid;

		jid = g_strdup_printf ("%s@%s",
				kf_jabber_connection_settings.username,
				kf_jabber_connection_settings.server);

		lm_connection_set_server (kf_jabber_connection, kf_jabber_connection_settings.manual_host);
		lm_connection_set_jid (kf_jabber_connection, jid);
	
		foo_debug ("Connecting to %s, jid %s", kf_jabber_connection_settings.manual_host, jid);

		g_free (jid);
	}
#endif


	/* SSL Stuff */
	
	if (kf_jabber_connection_settings.use_ssl) {
		if (lm_ssl_is_supported ()) {
			lm_connection_set_ssl (kf_jabber_connection, kf_connection_get_lm_ssl ());
		} else {
			g_printerr ("SSL isn't supported!\n");
		}
	}

	/* Proxy Stuff */

	if (kf_connection_get_lm_proxy ())
		lm_connection_set_proxy (kf_jabber_connection, kf_connection_get_lm_proxy ());

//	if (lm_ssl_is_supported ())
//		lm_connection_set_ssl (kf_jabber_connection, kf_jabber_connection_settings.use_ssl);
	lm_connection_set_disconnect_function (kf_jabber_connection,
			kf_jabber_disconnect_callback, NULL, NULL);

	h = lm_message_handler_new (kf_jabber_message_hendel, NULL, NULL);
	hp = lm_message_handler_new (kf_jabber_presence_hendel, NULL, NULL);
	hi = lm_message_handler_new (kf_jabber_iq_hendel, NULL, NULL);
	lm_connection_register_message_handler (kf_jabber_connection, h, LM_MESSAGE_TYPE_MESSAGE, 2);
	lm_connection_register_message_handler (kf_jabber_connection, hp, LM_MESSAGE_TYPE_PRESENCE, 2);
	lm_connection_register_message_handler (kf_jabber_connection, hi, LM_MESSAGE_TYPE_IQ, 2);

	lm_connection_open (kf_jabber_connection,
			kf_jabber_connect_callback,
			NULL,
			NULL,
			&error);
	kf_statusbar_set_text (_("Connecting..."));
	return TRUE;
}

/* Wys³anie <presence> - ustawienie statusu i obecno¶ci */
/* type = typ obecno¶ci - online,away,xa,dnd,chat - NULL==online */
/* status = opcjonalny tekst opisuj±cy status, mo¿e byæ NULL */
void kf_jabber_send_presence (const gchar *type, const gchar *status) {
	kf_jabber_send_presence_to_jidx (type, status, NULL);
}
static void kf_jabber_send_presence_to_jidx (const gchar *type, const gchar *status, const gchar *jid) {
		LmMessage *msg;
		gchar *priority;
		
		if (!kf_jabber_is_connected) {
			kf_jabber_connect ();
			return;
		}

		msg = lm_message_new (jid, LM_MESSAGE_TYPE_PRESENCE);
		
		if (type) {
			lm_message_node_add_child (msg->node, "show", type);
			if ((!strcmp (type,"invisible"))
					|| (!strcmp (type,"unavailable"))) {
				lm_message_node_set_attribute (msg->node, "type", type);
			}
		} else {
			type = "online";
		}
		if (status)
			lm_message_node_add_child (msg->node, "status", status);

		priority = g_strdup_printf ("%d", kf_jabber_connection_settings.priority);
		lm_message_node_add_child (msg->node, "priority", priority);
		g_free (priority);
		
		if (!lm_connection_send (kf_jabber_connection, msg, &error)) {
			g_error ("Nie mo¿na wys³aæ wiadomo¶ci: %s\n", error->message);
		}
		lm_message_unref (msg);	/* Mam nadziejê ¿e to zwalnia pamiêæ */

		kf_status_changed (kf_jabber_presence_type_from_string (type), status);
}

/* Ustawia parametry po³±czenia */
/* parametry nie wymagaj± wyja¶nienia ;-) */
void kf_jabber_set_connection (const gchar *server, const gchar *username, const gchar *password,
		const gchar *resource, int port) {
	kf_jabber_connection_settings.set = TRUE; 
	g_free (kf_jabber_connection_settings.server);
	kf_jabber_connection_settings.server = g_strdup (server);
	g_free (kf_jabber_connection_settings.username);
	kf_jabber_connection_settings.username = g_strdup (username);
	g_free (kf_jabber_connection_settings.password);
	kf_jabber_connection_settings.password = g_strdup (password);
	g_free (kf_jabber_connection_settings.resource);
	kf_jabber_connection_settings.resource = g_strdup (resource);
	kf_jabber_connection_settings.port = port;
}

/*
void kf_jabber_set_connection_ssl (gboolean var) {
}
*/

/* Roz³±czenie */
void kf_jabber_disconnect (void) {
	
	lm_connection_close (kf_jabber_connection, &error);
}

/* Funkcja zwrotna wywo³ywana przy roz³±czeniu siê z serwerem */
void        kf_jabber_disconnect_callback          (LmConnection *connection,
                                             LmDisconnectReason reason,
                                             gpointer user_data)
{
	kf_jabber_connection_settings.set = FALSE;

	kf_jabber_is_connected = FALSE;
	kf_jabber_roster_free ();
	kf_gui_disconnected (reason != LM_DISCONNECT_REASON_OK);
}

/* Odbiornik wiadomo¶ci */
/*
LmHandlerResult kf_jabber_message_hendel    (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *node;
	const gchar *from, *subject;
	gchar *body;
	KfJabberMessage *msg;
	LmMessageSubType subtype;


	subtype = lm_message_get_sub_type (message);
	from = lm_message_node_get_attribute (message->node, "from");
//	node = lm_message_node_get_child (message->node, "subject");
//	subject = node?node->value:NULL;
//	node = lm_message_node_get_child (message->node, "body");
//	body = node?node->value:NULL;


	g_printerr ("> %s\n",lm_message_node_to_string (message->node));

	msg = kf_jabber_message_new ();
	msg->type = (subtype == LM_MESSAGE_SUB_TYPE_CHAT)?KF_JABBER_MESSAGE_TYPE_CHAT:KF_JABBER_MESSAGE_TYPE_NORMAL;
	msg->from = g_strdup (from);
//	msg->subject = g_strdup (subject);
//	msg->body = g_strdup (body);

	for (node = message->node->children; node; node = node->next) {
		if (!strcmp (node->name, "subject")) {
			msg->subject = g_strdup (node->value);
		} else if (!strcmp (node->name, "body")) {
			msg->body = g_strdup (node->value);
		} else if (!strcmp (node->name, "x")) {
			const gchar *xmlns;

			xmlns = lm_message_node_get_attribute (node, "xmlns");
			if (!strcmp (xmlns, "jabber:x:delay")) {
				const gchar *stamp;

				stamp = lm_message_node_get_attribute (node, "stamp");
				g_print ("\n---------------------------> %s\n\n", stamp);
				msg->stamp = x_stamp_to_time (stamp);
			}
		}
	}

	kf_message_process (msg);

	kf_jabber_message_unref (msg);
	
//	kf_gui_message_normal_display (from, subject, body);
	
}
*/
/* Converts jabber:x:delay timestamp to standard UNIX timestamp */
guint32 x_stamp_to_time (const gchar *x){
	gint year, month, day, hour, minute, second;

	if (sscanf (x, "%4d%2d%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second) == 6) {
		struct tm stime;

		stime.tm_sec = second;
		stime.tm_min = minute;
		stime.tm_hour = hour;
		stime.tm_mday = day;
		stime.tm_mon = month -1;
		stime.tm_year = year -1900;

		return mktime (&stime);
	} else {
		foo_debug ("Unable to interpret '%s'\n", x);
		return time (NULL);
	}

}

/* Przetwornik obecno¶ci ;-) */
LmHandlerResult kf_jabber_presence_hendel   (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	gchar *jid;
	gchar *string;
	gchar *show;
	gchar *status;
	LmMessageNode *node;
	KfJabberPresenceType type;
	const gchar *st;
	gchar *fulljid;
	KfJabberRosterItem *item;

//	g_printerr ("> %s\n",lm_message_node_to_string (message->node));

	jid = g_strdup (lm_message_node_get_attribute (message->node, "from"));
	if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_SUBSCRIBE) {
		kf_subscription_process (jid);	

		g_free (jid);

		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	fulljid = g_strdup (jid);
	for (string = jid; *string != '\0' && *string != '/'; string++); /* Usuniêcie zasobu */
	*string = '\0';

	item = kf_jabber_roster_item_get (jid);
	if (!item) {
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	
	if (item->fulljid && strcmp (fulljid, item->fulljid)) {
		g_free (item->fulljid);
		item->fulljid = fulljid;
	} else {
		g_free (fulljid);
	}

	if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_ERROR) {
		node = lm_message_node_get_child (message->node, "error");
		type = KF_JABBER_PRESENCE_TYPE_UNAVAILABLE;
		status = g_strdup_printf ("Error: %s", node->value);
	} else {
		st = lm_message_node_get_attribute (message->node, "type");
		if (st && !strcmp (st, "unavailable")) {
			type = KF_JABBER_PRESENCE_TYPE_UNAVAILABLE;
			node = lm_message_node_get_child (message->node, "show");
			show = node?(node->value):"online";
			node = lm_message_node_get_child (message->node, "status");
			status = node?(node->value):NULL;
		} else {
			node = lm_message_node_get_child (message->node, "show");
			show = node?(node->value):"online";
			node = lm_message_node_get_child (message->node, "status");
			status = node?(node->value):NULL;
			type = kf_jabber_presence_type_from_string (show);
		}
	}
	
	kf_jabber_roster_update_status (jid, type, status);
	kf_chat_update_status (item->fulljid, type, status);

	kf_event (KF_EVENT_CLASS_PRESENCE, item);
	
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

/* Przetwornik IQ */
LmHandlerResult kf_jabber_iq_hendel         (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *node;
	const gchar *xmlns = NULL;
	
//	g_printerr ("> %s\n",lm_message_node_to_string (message->node));

	node = lm_message_node_get_child (message->node, "query");
	if (node) {
		xmlns = lm_message_node_get_attribute (node, "xmlns");
		if (!xmlns)
			return LM_HANDLER_RESULT_REMOVE_MESSAGE;
		
		if (!strcmp (xmlns, "jabber:iq:roster")) {
			return kf_jabber_roster_hendel (handler, connection, message, user_data);
		} else if (!strcmp (xmlns, "jabber:iq:version")) {
			const gchar *from, *id;
			LmMessage *msg;
			LmMessageNode *node;

			from = lm_message_node_get_attribute (message->node, "from");
			id = lm_message_node_get_attribute (message->node, "id");

			msg = lm_message_new_with_sub_type (from, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_RESULT);
			lm_message_node_set_attribute (msg->node, "id", id);
			node = lm_message_node_add_child (msg->node, "query", NULL);
			lm_message_node_set_attribute (node, "xmlns", "jabber:iq:version");
			lm_message_node_add_child (node, "name", "kf linux jabber client");
			lm_message_node_add_child (node, "version", VERSION);
			lm_message_node_add_child (node, "os", "GNU/Linux");

			lm_connection_send (connection, msg, NULL);
			lm_message_unref (msg);
			return LM_HANDLER_RESULT_REMOVE_MESSAGE;
		} else if (!strcmp (xmlns, "jabber:iq:search")) {
			return search_result_callback (handler, connection, message, NULL);
		}
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

KfJabberPresenceType kf_jabber_presence_type_from_string (const gchar *string) {
	if (string == NULL)
		return KF_JABBER_PRESENCE_TYPE_ONLINE;

	if (!strcmp (string, "online"))
		return KF_JABBER_PRESENCE_TYPE_ONLINE;
	else if (!strcmp (string, "chat"))
		return KF_JABBER_PRESENCE_TYPE_CHAT;
	else if (!strcmp (string, "away"))
		return KF_JABBER_PRESENCE_TYPE_AWAY;
	else if (!strcmp (string, "xa"))
		return KF_JABBER_PRESENCE_TYPE_XA;
	else if (!strcmp (string, "dnd"))
		return KF_JABBER_PRESENCE_TYPE_DND;
	else if (!strcmp (string, "invisible"))
		return KF_JABBER_PRESENCE_TYPE_INVISIBLE;
	else if (!strcmp (string, "unavailable"))
		return KF_JABBER_PRESENCE_TYPE_UNAVAILABLE;
	else
//		return KF_JABBER_PRESENCE_TYPE_UNAVAILABLE;
		return KF_JABBER_PRESENCE_TYPE_ONLINE;
}

const gchar *kf_jabber_presence_type_to_string (KfJabberPresenceType type) {
	static gchar *str;

	switch (type) {
		case KF_JABBER_PRESENCE_TYPE_ONLINE:
			str = "online";
			break;
		case KF_JABBER_PRESENCE_TYPE_CHAT:
			str = "chat";
			break;
		case KF_JABBER_PRESENCE_TYPE_AWAY:
			str = "away";
			break;
		case KF_JABBER_PRESENCE_TYPE_XA:
			str = "xa";
			break;
		case KF_JABBER_PRESENCE_TYPE_DND:
			str = "dnd";
			break;
		case KF_JABBER_PRESENCE_TYPE_UNAVAILABLE:
			str = "unavailable";
			break;
		default:
			str = "online";
	}
	return str;
}

/**
 * \brief Converts enumerated \a KfJabberPresenceType to human-readable string translated
 *	according to current locale
 * \param type Presence type
 * \returns string in natural language. This string should not be modified or freed.
 **/
const gchar *kf_jabber_presence_type_to_human_string (KfJabberPresenceType type) {
	static gchar *texts[] = {
		N_("available"),
		N_("free for chat"),
		N_("away"),
		N_("eXtended away"),
		N_("busy"),
		N_("invisible"),
		N_("offline")};
	if (type <= KF_JABBER_PRESENCE_TYPE_UNAVAILABLE)
		return _(texts[type]);
	else
		/* FIXME */
		return "error";
}

/* Stworzenie nowej wiadomo¶ci */
KfJabberMessage *kf_jabber_message_new (void) {
	KfJabberMessage *msg;

	msg = (KfJabberMessage *) g_malloc (sizeof (KfJabberMessage));
	if (msg == NULL)
		return NULL;

	msg->ref_count = 1;
	
	msg->from = msg->to = msg->subject = msg->body = NULL;
	msg->stamp = time (NULL);
	msg->body = NULL;

	msg->urls = NULL;
	
	return msg;
}

/* Usuniêcie wiadomo¶ci */
gpointer kf_jabber_message_free (KfJabberMessage *msg) {
	GSList *tmp;
	
	g_free (msg->from);
	g_free (msg->to);
	g_free (msg->subject);
	g_free (msg->body);
	g_free (msg);

	for (tmp = msg->urls; tmp; tmp = tmp->next) {
		gchar **link = tmp->data;
		g_free (link[0]);
		g_free (link[1]);
		g_free (link);
	}
	g_slist_free (msg->urls);

	return NULL;
}

KfJabberMessage *kf_jabber_message_ref (KfJabberMessage *msg) {
	msg->ref_count++;

	return msg;
}

void kf_jabber_message_unref (KfJabberMessage *msg) {
	msg->ref_count--;

	if (msg->ref_count <= 0)
		kf_jabber_message_free (msg);
}

/* Wys³anie wiadomo¶ci *//*
gboolean kf_jabber_message_send (KfJabberMessage *jmsg) {
	LmMessage *msg;
	
	kf_message_log (jmsg, TRUE);

	msg = lm_message_new (jmsg->to, LM_MESSAGE_TYPE_MESSAGE);
	if (jmsg->subject)
		lm_message_node_add_child (msg->node, "subject", jmsg->subject);
	if (jmsg->body)
		lm_message_node_add_child (msg->node, "body", jmsg->body);

	if (jmsg->type == KF_JABBER_MESSAGE_TYPE_CHAT)
		lm_message_node_set_attribute (msg->node, "type", "chat");

	if (!lm_connection_send (kf_jabber_connection, msg, &error)) {
		g_error ("Nie mo¿na wys³aæ wiadomo¶ci: %s\n", error->message);
	}
	lm_message_unref (msg);*/	/* Mam nadziejê ¿e to zwalnia pamiêæ */
//}

gboolean kf_jabber_send_raw (const gchar *text) {
	return lm_connection_send_raw (kf_jabber_connection, text, &error);
}

void kf_jabber_add_contact (const gchar *jid, const gchar *name, const gchar *group, gboolean subscribe) {
	LmMessage *msg;
	LmMessageNode *node;

	msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");
	node = lm_message_node_add_child (node, "item", NULL);
	lm_message_node_set_attributes (node, "jid", jid, "name", name, NULL);
	if (group)
		lm_message_node_add_child (node, "group", group);


	if (!lm_connection_send (kf_jabber_connection, msg, &error)) {
		g_error ("Nie mo¿na wys³aæ wiadomo¶ci: %s\n", error->message);
	}
	lm_message_unref (msg);	/* Mam nadziejê ¿e to zwalnia pamiêæ */
	
	if (subscribe) {
		LmMessage *xmsg;

		xmsg = lm_message_new_with_sub_type (jid, LM_MESSAGE_TYPE_PRESENCE,
				LM_MESSAGE_SUB_TYPE_SUBSCRIBE);
		if (!lm_connection_send (kf_jabber_connection, xmsg, &error)) {
			g_error ("Nie mo¿na wys³aæ wiadomo¶ci: %s\n", error->message);
		}

		lm_message_unref (xmsg);
	}
}

void kf_jabber_remove_contact (const gchar *jid) {
	LmMessage *msg;
	LmMessageNode *node;

	msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");
	node = lm_message_node_add_child (node, "item", NULL);
	lm_message_node_set_attributes (node, "jid", jid, "subscription", "remove", NULL);

	if (!lm_connection_send (kf_jabber_connection, msg, &error)) {
		g_error ("Nie mo¿na wys³aæ wiadomo¶ci: %s\n", error->message);
	}
	lm_message_unref (msg);	/* Mam nadziejê ¿e to zwalnia pamiêæ */
}

gboolean kf_connection_send (LmConnection *connection, LmMessage *message, GError **error) {
	if (connection == NULL) {
		connection = kf_jabber_connection;
	}
//	if (lm_connection_is_open (connection) && lm_connection_is_authenticated (connection))
		return lm_connection_send (connection, message, error);
//	else {
//		g_printerr ("Connection is closed!!!");
//		return FALSE;
//	}
}

gboolean kf_jabber_connected (void) {
	if (!kf_jabber_is_connected) {
		kf_gui_alert (_("You have to be connected to do that..."));
	}
	return kf_jabber_is_connected;
}

gboolean kf_jabber_get_connected (void) {
	return kf_jabber_is_connected;
}

LmConnection *kf_jabber_get_connection (void) {
	return kf_jabber_connection;
}
