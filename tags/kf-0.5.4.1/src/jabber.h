/* file jabber.h */
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


/* file jabber_connection.c */

#define kf_connected() if (! kf_jabber_connected ()) {return ;}

//void        kf_jabber_connect_callback      (LmConnection *connection,
//                                             gboolean success,
//                                             gpointer user_data);
typedef enum {
	KF_JABBER_PRESENCE_TYPE_ONLINE,
	KF_JABBER_PRESENCE_TYPE_CHAT,
	KF_JABBER_PRESENCE_TYPE_AWAY,
	KF_JABBER_PRESENCE_TYPE_XA,
	KF_JABBER_PRESENCE_TYPE_DND,
	KF_JABBER_PRESENCE_TYPE_INVISIBLE,
	KF_JABBER_PRESENCE_TYPE_UNAVAILABLE
} KfJabberPresenceType;

typedef enum {
	KF_JABBER_SUBSCRIPTION_TYPE_NONE,
	KF_JABBER_SUBSCRIPTION_TYPE_FROM,
	KF_JABBER_SUBSCRIPTION_TYPE_TO,
	KF_JABBER_SUBSCRIPTION_TYPE_BOTH,
	KF_JABBER_SUBSCRIPTION_TYPE_UNKNOWN
} KfJabberSubscriptionType;

typedef struct {
	gboolean set;
	gchar *server;
	gchar *username;
	gchar *password;
	gchar *resource;
	gint priority;
	gint port;
	gboolean use_ssl;
	gchar *manual_host;
} KfJabberConnection;

typedef enum {
	KF_JABBER_CONTACT_TYPE_CONTACT,
	KF_JABBER_CONTACT_TYPE_AGENT,
	KF_JABBER_CONTACT_TYPE_UNKNOWN = 999
} KfJabberContactType;

typedef struct {
	KfJabberContactType type;
	KfJabberSubscriptionType subscription;
	gboolean in_roster;
	gchar *jid;
	gchar *fulljid;
	gchar *name;
	gchar *display_name;
	GList *groups;
	gchar *group;	/* Grupa - TODO: przej¶æ na GList groups... */
	KfJabberPresenceType presence;
	gchar *status;
	gpointer *events;
} KfJabberRosterItem;

typedef enum {
	KF_JABBER_MESSAGE_TYPE_NORMAL,
	KF_JABBER_MESSAGE_TYPE_CHAT,
	KF_JABBER_MESSAGE_TYPE_GROUPCHAT,
	KF_JABBER_MESSAGE_TYPE_UNKNOWN = 999
} KfJabberMessageType;

typedef struct {
	KfJabberMessageType type;
	gint ref_count;
	gchar *from;
	gchar *to;
	gchar *subject;
	guint32 stamp;
	gchar *body;

	int special_me : 1;
	GSList *urls;
} KfJabberMessage;

void kf_jabber_init (void);

gboolean kf_jabber_connect (void);
void kf_jabber_send_presence (const gchar *type, const gchar *status);
void kf_jabber_set_connection (const gchar *server, const gchar *username, const gchar *password,
		const gchar *resource, int port);
void kf_jabber_disconnect (void);

KfJabberPresenceType kf_jabber_presence_type_from_string (const gchar *string);
const gchar *kf_jabber_presence_type_to_string (KfJabberPresenceType type);
const gchar *kf_jabber_presence_type_to_human_string (KfJabberPresenceType type);

void kf_jabber_roster_init (void); 
void kf_jabber_get_roster (void);
KfJabberRosterItem *kf_jabber_roster_item_new (const gchar *jid, const gchar *name);
gpointer kf_jabber_roster_item_free (KfJabberRosterItem *item);
gboolean kf_jabber_roster_update_status (const gchar *jid, KfJabberPresenceType type, const gchar *status);
void kf_jabber_roster_free (void);
KfJabberRosterItem *kf_jabber_roster_item_get (const gchar *jid);
KfJabberSubscriptionType kf_jabber_subscription_type_from_string (const gchar *str);

#ifdef LM_CONNECTION
gboolean kf_connection_send (LmConnection *connection, LmMessage *message, GError **error);
LmConnection *kf_jabber_get_connection (void);
#endif


KfJabberMessage *kf_jabber_message_new (void);
KfJabberMessage *kf_jabber_message_ref (KfJabberMessage *);
void kf_jabber_message_unref (KfJabberMessage *);
gboolean kf_jabber_message_send (KfJabberMessage *jmsg);

void kf_jabber_add_contact (const gchar *jid, const gchar *name, const gchar *group, gboolean subscribe);
void kf_jabber_roster_delete (const gchar *jid);

void kf_jabber_send_presence_to_jid (const gchar *type, const gchar *jid);

gchar *kf_jabber_jid_crop (gchar *jid);
gboolean kf_jabber_connected (void);
gboolean kf_jabber_get_connected (void);
guint32 x_stamp_to_time (const gchar *);

gpointer kf_jabber_message_free (KfJabberMessage *msg);
gboolean kf_jabber_send_raw (const gchar *text);
void kf_jabber_remove_contact (const gchar *jid);
gboolean kf_jabber_get_connected (void);

