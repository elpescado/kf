/* file jabber_msg.c */
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
#include <time.h>
#include "kf.h"
#include "archive.h"
#include "jabber.h"
#include "jabber_msg.h"
#include "filter.h"
#include "message.h"

void kf_message_log (KfJabberMessage *msg, gboolean yours);
LmHandlerResult kf_contacts_recv_msg_handle (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);

/* Odbiornik wiadomo¶ci */
LmHandlerResult kf_jabber_message_hendel    (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data)
{
	LmMessageNode *node;
	const gchar *from;
	KfJabberMessage *msg;
	LmMessageSubType subtype;


	subtype = lm_message_get_sub_type (message);
	from = lm_message_node_get_attribute (message->node, "from");
	if (kf_filter_is_blocked (from)) {
		foo_debug ("Messages from '%s' are blocked!!!\n", from);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
/*	node = lm_message_node_get_child (message->node, "subject");
	subject = node?node->value:NULL;
	node = lm_message_node_get_child (message->node, "body");
	body = node?node->value:NULL;
*/

//	g_printerr ("> %s\n",lm_message_node_to_string (message->node));

	msg = kf_jabber_message_new ();
	msg->type = (subtype == LM_MESSAGE_SUB_TYPE_CHAT)?KF_JABBER_MESSAGE_TYPE_CHAT:KF_JABBER_MESSAGE_TYPE_NORMAL;
	msg->from = g_strdup (from);

	msg->special_me = FALSE;
//	msg->subject = g_strdup (subject);
//	msg->body = g_strdup (body);

	for (node = message->node->children; node; node = node->next) {
		if (!strcmp (node->name, "subject")) {
			msg->subject = g_strdup (node->value);
		} else if (!strcmp (node->name, "body")) {
			msg->body = g_strdup (node->value);

			if (strstr (node->value, "/me") == node->value) {
//				g_print ("meeeee....\n");
				msg->special_me = TRUE;
			}
		} else if (!strcmp (node->name, "x")) {
			const gchar *xmlns;

			xmlns = lm_message_node_get_attribute (node, "xmlns");
			if (!strcmp (xmlns, "jabber:x:delay")) {
				const gchar *stamp;

				stamp = lm_message_node_get_attribute (node, "stamp");
//				g_print ("\n---------------------------> %s\n\n", stamp);
				msg->stamp = x_stamp_to_time (stamp);
			} else if (strcmp (xmlns, "jabber:x:roster") == 0) {
				kf_jabber_message_unref (msg);
				return kf_contacts_recv_msg_handle (handler,
						connection,
						message,
						node);
			} else if (strcmp (xmlns, "jabber:x:oob") == 0) {
				LmMessageNode *child;
				const gchar *url = NULL;
				const gchar *desc = NULL;
				const gchar **link = NULL;

				for (child = node->children; child; child = child->next) {
					if (strcmp (child->name, "url") == NULL)
						url = lm_message_node_get_value (child);
					else if (strcmp (child->name, "desc") == NULL)
						desc = lm_message_node_get_value (child);
				}

				if (url) {
					link = g_new (gchar, 2);
					link[0] = g_strdup (url);
					link[1] = desc?g_strdup (desc):url;
					msg->urls = g_slist_append (msg->urls, link);
				}
			}

		}
	}

	if (msg->body) {
		kf_archive_msg (message, FALSE, msg->stamp);

		kf_message_process (msg);
	}

	kf_jabber_message_unref (msg);
	
//	kf_gui_message_normal_display (from, subject, body);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

/* Wys³anie wiadomo¶ci */
gboolean kf_jabber_message_send (KfJabberMessage *jmsg) {
	LmMessage *msg;
	extern LmConnection *kf_jabber_connection;
	GError *error;
	
	kf_message_log (jmsg, TRUE);

	if (jmsg->type == KF_JABBER_MESSAGE_TYPE_CHAT) {
	//	lm_message_node_set_attribute (msg->node, "type", "chat");
		msg = lm_message_new_with_sub_type (jmsg->to,
				LM_MESSAGE_TYPE_MESSAGE,
				LM_MESSAGE_SUB_TYPE_CHAT);
	} else {
		msg = lm_message_new_with_sub_type (jmsg->to,
				LM_MESSAGE_TYPE_MESSAGE,
				LM_MESSAGE_SUB_TYPE_NORMAL);
	}
	
	if (jmsg->subject)
		lm_message_node_add_child (msg->node, "subject", jmsg->subject);
	if (jmsg->body)
		lm_message_node_add_child (msg->node, "body", jmsg->body);


	if (!lm_connection_send (kf_jabber_connection, msg, &error)) {
		g_error ("Nie mo¿na wys³aæ wiadomo¶ci: %s\n", error->message);
	}

	kf_archive_msg (msg, TRUE, time (NULL));
	lm_message_unref (msg);	/* Mam nadziejê ¿e to zwalnia pamiêæ */
	return TRUE;
}


