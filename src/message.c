/* file message.c */
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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>	/* for mkdir () */
#include <sys/types.h>	/*              */
#include "preferences.h"
#include "jabber.h"
#include "gui.h"
#include "message.h"
#include "new_message.h"
#include "chat.h"
#include "events.h"
#include "xevent.h"
#include "kf.h"

static KfMsg *kf_msg_new (const gchar *jid);
void        on_msg_recv_reply_clicked      (GtkButton *button,
                                            gpointer user_data);

static void kf_message_normal_show_real (KfJabberMessage *msg, KfMsg *mesg);
static void kf_message_set_from (GtkLabel *label, const gchar *from);
void kf_message_log (KfJabberMessage *msg, gboolean yours);

static GHashTable *kf_messages = NULL;

void kf_message_init (void) {
	if (kf_messages == NULL) {
		kf_messages = g_hash_table_new (g_str_hash, g_str_equal);
	}
}

static KfMsg *kf_msg_new (const gchar *jid) {
	KfMsg *msg;

	msg = (KfMsg *) g_malloc (sizeof (KfMsg));
	msg->jid = g_strdup (jid);
	msg->glade = glade_xml_new (kf_find_file ("kf.glade"), "message_recv_window", NULL);
	msg->window = glade_xml_get_widget (msg->glade, "message_recv_window");
	g_object_set_data (G_OBJECT (msg->window), "object", (gpointer) msg);
	glade_xml_signal_autoconnect (msg->glade);

	return msg;
}

void kf_message_normal_show (KfJabberMessage *msg) {
	KfMsg *mesg;

//	kf_message_init ();

//	if ((mesg = g_hash_table_lookup (kf_messages, msg->from)) == NULL) {
		mesg = kf_msg_new (msg->from);
//		g_hash_table_insert (kf_messages, mesg->jid, mesg);
//	}

	kf_message_normal_show_real (msg, mesg);
}

void        on_msg_recv_close_clicked      (GtkButton *button,
                                            gpointer user_data)
{
	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


gboolean    on_message_recv_window_delete_event
					   (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data)
{
	gtk_widget_hide (widget);
	return TRUE;
}
void        on_msg_recv_reply_clicked      (GtkButton *button,
                                            gpointer user_data)
{
//	extern GladeXML *iface;
	KfMsg *msg;
	GladeXML *iface;
	GtkWidget *entry;
	const gchar *from;

	msg = (KfMsg *) g_object_get_data (G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (button))), "object");
	iface = msg->glade;

	entry = glade_xml_get_widget (iface, "recv_from_label");
	from = gtk_label_get_text (GTK_LABEL (entry));
	kf_new_message (from);
}


static void kf_message_normal_show_real (KfJabberMessage *msg, KfMsg *mesg) {
//	extern GladeXML *iface;
	GladeXML *iface;
	GtkWidget *win;
	GtkWidget *label;
	GtkWidget *textview;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar date_buffer[60];
	gchar *date_utf8;
	guint date_len;
	GSList *tmp;
	GtkTextTag *tag;
//	gchar *date;
	iface = mesg->glade;

	win = mesg->window;
//	win = glade_xml_get_widget (iface, "message_recv_window");

	label = glade_xml_get_widget (iface, "recv_from_label");
	kf_message_set_from (GTK_LABEL (label), msg->from);
	label = glade_xml_get_widget (iface, "recv_subject_label");
	gtk_label_set_text (GTK_LABEL (label), msg->subject?msg->subject:_("(no topic)"));
	label = glade_xml_get_widget (iface, "recv_date_label");
	date_len = strftime (date_buffer, 60, "%a, %d %b %Y, %X",
			localtime ((time_t *) &(msg->stamp)));
	date_utf8 = g_locale_to_utf8 (date_buffer, date_len, NULL, NULL, NULL);
	//gtk_label_set_text (GTK_LABEL (label), ctime(&(msg->stamp)));
	gtk_label_set_text (GTK_LABEL (label), date_utf8);

//	strftime (date_buffer, 60, "", 	);

	textview = glade_xml_get_widget (iface, "recv_msg_textview");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	kf_emo_init_textview (textview);
/*	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_delete (buffer, &start, &end);
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_insert (buffer, &start, msg->body, -1);
*/
	emo_insert (buffer, msg->body, kf_preferences_get_int ("useEmoticons"));

	tag = gtk_text_buffer_create_tag (buffer, NULL,
			"weight", PANGO_WEIGHT_BOLD,
			"scale", PANGO_SCALE_MEDIUM, NULL);
	
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_insert_with_tags (buffer, &end, _("\n\nAttached URLs\n"), -1, tag, NULL);

	for (tmp = msg->urls; tmp; tmp = tmp->next) {
		gchar **link = tmp->data;
		if (link[1]) {
			emo_insert (buffer, link[1], FALSE);
			emo_insert (buffer, ": ", FALSE);
		}
		emo_insert (buffer, link[0], FALSE);
		emo_insert (buffer, "\n", FALSE);
	}
		

	if (kf_preferences_get_int ("autoPopup")) {
		kf_gui_show (win);
	} else {
		gchar *xjid;

		xjid = kf_jabber_jid_crop (g_strdup (msg->from));
		kf_x_event_push (xjid, KF_EVENT_CLASS_MESSAGE, win);
		g_free (xjid);
	}
}

static void kf_message_set_from (GtkLabel *label, const gchar *from) {
	gchar *fromx;
	KfJabberRosterItem *item;

	fromx = kf_jabber_jid_crop (g_strdup (from));
	item = kf_jabber_roster_item_get (fromx);
	if (item && item->name) {
		guint nick_len, jid_len;
		gchar *text;
		PangoAttrList *list;
		PangoAttribute *italic_attr, *small_attr;

		nick_len = strlen (item->name);
		jid_len = strlen (item->jid);

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

		text = g_strdup_printf ("%s <%s>", item->name, item->jid);

		gtk_label_set_text (label, text);
		gtk_label_set_attributes (label, list);

		g_free (text);
		pango_attr_list_unref (list);
	} else {
		foo_debug ("Nie ma nic...");
		gtk_label_set_text (label, from);
	}
	g_free (fromx);
}

void kf_message_process (KfJabberMessage *msg) {
	kf_message_log (msg, FALSE);

	
	if (msg->type == KF_JABBER_MESSAGE_TYPE_CHAT) {
		kf_event (KF_EVENT_CLASS_CHAT, msg);
		kf_chat_show (msg);
	} else {
		kf_event (KF_EVENT_CLASS_MESSAGE, msg);
		kf_message_normal_show (msg);
	}
}

void kf_message_log (KfJabberMessage *msg, gboolean yours) {
	FILE *f;
	gchar date_buffer[64];
	gchar *path;
	gint date_len;
	gchar *date_utf8;
	gchar *jid;

	/* We can turn it off:-) */
	if (! kf_preferences_get_int ("logMessages"))
		return;

	if (!g_file_test (kf_config_file ("log"), G_FILE_TEST_IS_DIR)) {
		mkdir (kf_config_file ("log"), 00753);
	}

	jid = g_strdup ((yours)?(msg->to):(msg->from));
	kf_jabber_jid_crop (jid);	
	path = g_strdup_printf ("%s/%s.txt", kf_config_file ("log"), jid);
	g_free (jid);
	f = fopen (path, "a");
	g_free (path);
	if (f == NULL) {
		return;
	}
	
	date_len = strftime (date_buffer, 64, "%a, %d %b %Y, %X",
			localtime ((time_t *) &(msg->stamp)));
	date_utf8 = g_locale_to_utf8 (date_buffer, date_len, NULL, NULL, NULL);
	fprintf (f, "%s :: %s :: %s\n%s\n%s\n\n", yours?"OUT":"IN", date_utf8, yours?"You":msg->from, msg->subject?msg->subject:"", msg->body);
	g_free (date_utf8);
	fclose (f);
}
