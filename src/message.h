/* file message.h */
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

typedef struct {
	gchar *jid;
	GladeXML *glade;
	GtkWidget *window;
} KfMsg;

void        on_msg_recv_close_clicked      (GtkButton *button,
                                            gpointer user_data);
gboolean    on_message_recv_window_delete_event
					   (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data);
#endif
void kf_message_init (void);
void kf_message_normal_show (KfJabberMessage *msg);
void kf_message_process (KfJabberMessage *msg);
