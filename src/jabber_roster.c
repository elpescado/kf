/* file jabber_roster.c */
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


#include <loudmouth/loudmouth.h>
#include <string.h>
#include "kf.h"
#include "jabber.h"
#include "gui.h"
#include "preferences.h"

GPtrArray *kf_jabber_roster = NULL;	/* Hash(int) zawieraj±cy wpisy z rostera */
gint	kf_jabber_roster_count = 0;	/* Indeks ostatniego elementu */
void kf_gui_update_status (KfJabberRosterItem *item, gboolean strong);
LmHandlerResult kf_jabber_roster_hendel     (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);
gint kf_jabber_roster_cmp (gconstpointer *a, gconstpointer *b);

/* Uruchomienie rostera */
void kf_jabber_roster_init (void) {
	if (kf_jabber_roster) {
		/* Roster ju¿ istnieje */
		g_ptr_array_free (kf_jabber_roster, FALSE);
	}
	
	kf_jabber_roster = g_ptr_array_new();
	kf_jabber_roster_count = 0;
}

void kf_jabber_get_roster (void) {
	LmMessage *msg;
	LmMessageNode *node;
	LmMessageHandler *h;
	extern LmConnection *kf_jabber_connection;

	msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	h = lm_message_handler_new (kf_jabber_roster_hendel, "dupsko", NULL);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");
	lm_connection_send_with_reply (kf_jabber_connection, msg, h, NULL);
}

/* Takie co¶ dostaje siê po odebraniu rostera */
LmHandlerResult kf_jabber_roster_hendel     (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *root, *node, *group_node; /* Wêze³ki w XMLu */
	const gchar *name;
	gchar *jid;
	gchar *group; /* Parametry kontaktu */
	const gchar *id;	/* ID of the message */
	KfJabberRosterItem *item; /* Nowy element Rostera */
	
//	g_printerr (">> %s\n",lm_message_node_to_string (message->node));

	root = message->node;
	node = lm_message_node_find_child (root, "item");
	id = lm_message_node_get_attribute (root, "id");
	while (node) {
		const gchar *subscription = NULL;
		jid = g_strdup (lm_message_node_get_attribute (node, "jid"));
		kf_jabber_jid_crop (jid);

		if ((subscription = lm_message_node_get_attribute (node, "subscription"))) {
	//		sub = kf_jabber_subscription_type_from_string (subscription);
		}
		
		if (subscription && !strcmp (subscription, "remove")) {
			/* Serwer informuje nas o usuniêciu takiego kontaktu */
			item = NULL;
			kf_jabber_roster_delete (jid);
			node = node->next;
			g_free (jid);
			continue;
		}
		
		name = lm_message_node_get_attribute (node, "name");
		group_node = lm_message_node_get_child (node, "group");
		if (group_node)
			group = group_node->value;
		else
			group = NULL;

		if (user_data || (item = kf_jabber_roster_item_get (jid)) == NULL) {
			item = kf_jabber_roster_item_new (jid, name);
			g_ptr_array_add (kf_jabber_roster, item);
		} else {
			g_free (item->name);
			item->name = name?g_strdup (name):NULL;
			item->display_name = (name && *name)?item->name:item->jid;
		//	kf_jabber_roster_delete (jid);
		//	item = NULL;
		//	item = kf_jabber_roster_item_new (jid, name);
		//	g_ptr_array_add (kf_jabber_roster, item);
		}
			item->group = NULL;
			item->groups = NULL;
			item->subscription =  kf_jabber_subscription_type_from_string (subscription);
			kf_jabber_roster_count++;
		
			if (group_node) {
				item->group = g_strdup (group_node->value);
			}
			
			group_node = node->children;
			while (group_node) {
				if (!strcmp (group_node->name, "group") && group_node->value) {
					/* Potential memory-leak... */
					item->groups = g_list_append (item->groups, g_strdup (group_node->value));
				}
				group_node=group_node->next;
			}			
//		} else {
//			g_free (item->name);
//			item->name = g_strdup (name);
//			item->subscription =  kf_jabber_subscription_type_from_string (subscription);
//		}
		
		g_free (jid);
		node = node->next;
	}
	g_ptr_array_sort (kf_jabber_roster, (GCompareFunc) kf_jabber_roster_cmp);
	kf_gui_update_roster ();

	if (user_data) {
		kf_jabber_send_presence (
				kf_preferences_get_string ("statusType"),
				kf_preferences_get_string ("statusText"));
 	} else {
 		if (id && lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_SET) {
 			LmMessage *msg;
 			
 			msg = lm_message_new_with_sub_type (kf_connection_get_server (),
 					LM_MESSAGE_TYPE_IQ,
 					LM_MESSAGE_SUB_TYPE_RESULT);
 			lm_message_node_set_attribute (msg->node, "id", id);
 			kf_connection_send (NULL, msg, NULL);
 		}
 	}

	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

/* Utworzenie nowego elementu spisu kontaktów */
KfJabberRosterItem *kf_jabber_roster_item_new (const gchar *jid, const gchar *name) {
	KfJabberRosterItem *item;	/* Nowy element */

	/* Akt Kreacji */
	item = (KfJabberRosterItem *) g_malloc (sizeof (KfJabberRosterItem));
	if (item == NULL) {
		return NULL;
	}

	item->fulljid = NULL;
	item->subscription = KF_JABBER_SUBSCRIPTION_TYPE_UNKNOWN;
	item->in_roster = TRUE;

	if (!jid)
		item->type = KF_JABBER_CONTACT_TYPE_UNKNOWN;
	else {
		const gchar *c;
		
		item->type = KF_JABBER_CONTACT_TYPE_AGENT;
		for (c = jid; *c != '\0'; c++) {
			if (*c == '@') {
				item->type = KF_JABBER_CONTACT_TYPE_CONTACT;
				break;
			}
		}
	}

	/* Ustawienie w³asno¶ci */
	item->jid = (jid)?g_strdup(jid):NULL;
	item->fulljid = (jid)?g_strdup (item->jid):NULL;
	item->name = (name && *name)?g_strdup(name):NULL;
	item->display_name = item->name?item->name:item->jid;
	item->groups = NULL;
	item->presence = KF_JABBER_PRESENCE_TYPE_UNAVAILABLE;
	item->status = NULL;
	
	return item;
}

/* Zwalniamy wszystko, co jest w JabberRosterItem */
gpointer kf_jabber_roster_item_free (KfJabberRosterItem *item) {
	g_free (item->jid);
	g_free (item->name);
	g_list_free (item->groups);
	g_free (item->group);
	g_free (item->status);
	
	g_free (item);
	
	return NULL;
}

gboolean kf_jabber_roster_update_status (const gchar *jid, KfJabberPresenceType type, const gchar *status) {
	KfJabberRosterItem *item;

	item = kf_jabber_roster_item_get (jid);
	if (item) {
		item->presence = type;
		g_free (item->status);
		if (status)
			item->status = g_strdup (status);
		else
			item->status = NULL;
		
		kf_gui_update_status (item, TRUE);
	}
	return TRUE;
}
gint kf_jabber_roster_cmp (gconstpointer *a, gconstpointer *b) {
	const KfJabberRosterItem *aa, *bb;
	aa = *a;
	bb = *b;
	if (*a && *b)
		return g_ascii_strcasecmp (aa->display_name, bb->display_name);	
	else
		return -1;
}

KfJabberRosterItem *kf_jabber_roster_item_get (const gchar *jid) {
	guint current;
	KfJabberRosterItem *item;
	KfJabberRosterItem *found = NULL;
	
	if (kf_jabber_roster == NULL) {
		foo_debug ("Roster is NULL\n");
		return NULL;
	}
	
	for (current = 0; current < kf_jabber_roster->len; current++) {
		item = g_ptr_array_index (kf_jabber_roster, current);
		if (item == NULL)
			continue;

		if (strcmp (jid, item->jid) == 0) {
			found = item;
			break;
		}
	}
	return found;
}

void kf_jabber_roster_delete (const gchar *jid) {
	guint current;
	KfJabberRosterItem *item;
	for (current = 0; current < kf_jabber_roster->len; current++) {
		item = g_ptr_array_index (kf_jabber_roster, current);
		if (item && strcmp (jid, item->jid) == 0) {
			(kf_jabber_roster->pdata)[current] = NULL;
			return;
		}
	}
}

//KfJabberPresenceType kf_jabber_presence_type_from_string () {
//};

void kf_jabber_send_presence_to_jid (const gchar *type, const gchar *jid) {
	LmMessage *msg;
	extern LmConnection *kf_jabber_connection;
	extern GError *error;

	msg = lm_message_new (jid, LM_MESSAGE_TYPE_PRESENCE);
	if (type)
		lm_message_node_set_attribute (msg->node, "type", type);
	if (!lm_connection_send (kf_jabber_connection, msg, &error)) {
		g_error ("Nie mo¿na wys³aæ wiadomo¶ci: %s\n", error->message);
	}
//	g_printerr ("presence:\n< %s\n\n",lm_message_node_to_string (msg->node));
	lm_message_unref (msg);
}

void kf_jabber_roster_free (void) {
	KfJabberRosterItem *item;
	while ((item = g_ptr_array_remove_index_fast (kf_jabber_roster, 0))) {
		kf_jabber_roster_item_free (item);
	}
}

gchar *kf_jabber_jid_crop (gchar *jid) {
	char *s;
	s = jid;
	while (*jid != '\0' && *jid != '/')
		jid++;
	*jid = '\0';
	return s;
}

KfJabberSubscriptionType kf_jabber_subscription_type_from_string (const gchar *str) {
	if (!strcmp (str, "none"))
		return KF_JABBER_SUBSCRIPTION_TYPE_NONE;
	else if (!strcmp (str, "from"))
		return KF_JABBER_SUBSCRIPTION_TYPE_FROM;
	else if (!strcmp (str, "to"))
		return KF_JABBER_SUBSCRIPTION_TYPE_TO;
	else if (!strcmp (str, "both"))
		return KF_JABBER_SUBSCRIPTION_TYPE_BOTH;
	else 
		return KF_JABBER_SUBSCRIPTION_TYPE_UNKNOWN;
}
