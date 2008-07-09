/* file chat.h */
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


#ifdef GLADE_H
typedef enum {
	KF_CHAT_WINDOW,
	KF_CHAT_TAB
} KfChatMode;

typedef struct {
	gchar *jid;			/* JabberID */
	KfJabberRosterItem *item;	/* Roster item related to above JID */
	gint unread;			/* Number of incoming messages */
	
	GladeXML *glade;		/* Glade interface */
	GtkWidget *content;		/* Content of chat window */

	KfChatMode mode;		/* Whether a chat is inside a window or tab */
	GtkWidget *window;		/* Window */
	GtkWidget *container;		/* Direct parent of a chat */
	GtkWidget *label;		/* Label of a tab - may be NULL */

	GtkTextView *view;		/* TextView of history */
	GtkTextBuffer *buffer;		/* History text buffer */
	GtkTextBuffer *my_buffer;	/* Input text buffer */
	GtkTextTag *my_text;		/* Text tag for your headers */
	GtkTextTag *his_text;		/* Text tag for other headers */
	GtkTextTag *url;		/* Text tag for URLs */
	GtkTextTag *stamp;		/* Text tag for Time Stamps */
	GtkTextTag *notify;		/* Text tag for status notifications */
	GtkAdjustment *adj;		/* Chat's adjustment, obsolete */
	GtkImage *status_image;		/* Status image */

	glong	last_activity;		/* Last activity of a chat, for Garbage Collector */
} KfChat;

//KfChat *kf_chat_new (const gchar *jid); 
#endif
void kf_chat_init (void);
void kf_chat_show (KfJabberMessage *msg);
void kf_chat_window (const gchar *jid);
void kf_chat_update_status (const gchar *jid, KfJabberPresenceType type, const gchar *status);
