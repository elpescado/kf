/* file new_message.c */
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
#include "kf.h"
#include "jabber.h"
#include "new_message.h"

#ifdef HAVE_GTKSPELL
#  include <gtkspell/gtkspell.h>
#endif


/* Funkcje takie lokalne, do obs³ugi okienka */
void        on_msg_ok_clicked              (GtkButton *button,
                                            gpointer user_data);
void        on_msg_anuluj_clicked          (GtkButton *button,
                                            gpointer user_data);
gboolean    on_new_msg_delete_event        (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data);


void kf_new_message (const gchar *to) {
	GladeXML *glade;
	GtkWidget *to_entry;
	GtkWidget *win;
	GError *spell_err = NULL;
	GtkWidget *tv;

	glade = glade_xml_new (kf_find_file ("msg.glade"), NULL, NULL);
	if (to) {
		to_entry = glade_xml_get_widget (glade, "to_jid");
		gtk_entry_set_text (GTK_ENTRY (to_entry), to);
	}
	win = glade_xml_get_widget (glade, "new_msg");
	glade_xml_signal_autoconnect (glade);
	g_object_set_data (G_OBJECT (win), "GladeXML", glade);

	/* GtkSpell Support */
#ifdef HAVE_GTKSPELL
	tv = glade_xml_get_widget (glade, "body");
	if ( ! gtkspell_new_attach(GTK_TEXT_VIEW(tv), NULL, &spell_err)) {
		foo_debug ("Unable to use GtkSpell: %s\n", spell_err->message);
	}
#endif

}

void        on_msg_ok_clicked              (GtkButton *button,
                                            gpointer user_data)
{
	GtkWidget *win;
	GladeXML *glade;
	KfJabberMessage *msg;	/* Wiadomo¶æ */
	GtkWidget *entry;	/* Widgety w oknie */
	GtkWidget *tv;		/* . */
	GtkTextBuffer *buffer;	/* . */
	GtkTextIter start, end;
	gchar *to, *subject, *body;

	win = gtk_widget_get_toplevel (GTK_WIDGET (button));
	glade = g_object_get_data (G_OBJECT (win), "GladeXML");
	entry = glade_xml_get_widget (glade, "to_jid");
	to = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	entry = glade_xml_get_widget (glade, "topic");
	subject = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

	/* Pobranie tre¶ci wiadomo¶ci */
	tv = glade_xml_get_widget (glade, "body");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tv));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	body = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	
	/* Kreacja wiadomo¶ci */
	msg = kf_jabber_message_new ();
	msg->to = to;
	msg->subject = subject;
	msg->body = body;
	msg->type = KF_JABBER_MESSAGE_TYPE_NORMAL;

	kf_jabber_message_send (msg);
	kf_jabber_message_free (msg);
	
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


void        on_msg_anuluj_clicked          (GtkButton *button,
                                            gpointer user_data)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


gboolean    on_new_msg_delete_event        (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data)
{
	return FALSE;
}
