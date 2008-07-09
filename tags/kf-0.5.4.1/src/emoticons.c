/* file emoticons.c */
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "kf.h"
#include "emoticons.h"
#include "preferences.h"
#include "www.h"

/* Animated emoticons */
//#define ANIM

typedef struct {
	gchar *name;
	gchar *filename;
	gchar *path;
	GdkPixbuf *pixbuf;
} KfEmoticon;

void kf_emoticons_insert_text (GtkTextBuffer *buffer, const gchar *text);
GList *kf_emoticons_load (const gchar *filename);
KfEmoticon *kf_emoticon_new (const gchar *name, const gchar *path, const gchar *file);
//void kf_emoticons_init (void);
static gboolean textview_clicked_cb (GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void textview_popup_cb (GtkTextView *textview, GtkMenu *menu, gpointer user_data);
static void open_link_cb (GtkMenuItem *menuitem, gpointer user_data);

GList *kf_emoticons_list = NULL;


static void emo_insert_emo (GtkTextBuffer *buffer, const gchar *text, gint len);


/* Ha! kf has now its own emoticon engine!!! */

/* This function inserts text into TextView, with emoticons and links underlined... */
void emo_insert (GtkTextBuffer *buffer, const gchar *text, gboolean with_emoticons) {
	gboolean done = FALSE;

	while (! done) {
		gchar *link, *end;
		GtkTextIter iter;

		link = strstr (text, "http://");
			/* Put all words before link */
		gtk_text_buffer_get_end_iter (buffer, &iter);
				
		//g_print ("DEBUG: link: %p, text: %p, link-text: %d\n", link, text, link-text);
		
		if (with_emoticons) {
			if (link)
				emo_insert_emo (buffer, text, link-text);
			else
				emo_insert_emo (buffer, text, strlen (text));
		} else {
			gtk_text_buffer_get_end_iter (buffer, &iter);
			gtk_text_buffer_insert (buffer, &iter, text, link?link-text:-1);
		}
		
		if (link) {
			for (end = link; ! isspace (*end) && *end; end++)
				;
			gtk_text_buffer_get_end_iter (buffer, &iter);
			gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					link, end-link, "link", NULL);

		/*
			if (table->link_cb) {
				gchar *url = g_new (gchar, end-link+1);
				strncpy (url, link, end-link);
				table->link_cb (buffer, url, time (NULL), table->link_cb_data);
				g_free (url);
			}
		*/
				
			if ((text = end) == '\0')
			done = TRUE;
		} else {
			done = TRUE;
		}
	}
}

/* This function inserts actually text into TextBuffer with emoticons...
 * to be called from above function */
static void emo_insert_emo (GtkTextBuffer *buffer, const gchar *text, gint len) {
	gboolean done = FALSE;

	//g_print ("Text to insert: %s, len %d\n", text, len);
	while (! done) {
		GList *tmp;
		gint first;
		KfEmoticon *femo = NULL;

		if (text == NULL)
			break;

		first = len;
		
		for (tmp = kf_emoticons_list; tmp; tmp=tmp->next) {
			KfEmoticon *emo = tmp->data;
			gchar *sub;
			gint pos;	/* Emo position */

			sub = strstr (text, emo->name);
			pos = sub - text;
			if (sub && pos <= len) {
				if (pos < first) {
					//first = MIN (first, pos);
					first = pos;
					femo = emo;
				}
			}
		}

		if (first > 0) {
			GtkTextIter iter;
//			g_print ("In: '%s'   (%d)\n",text, first);
			gtk_text_buffer_get_end_iter (buffer, &iter);
			gtk_text_buffer_insert (buffer, &iter, text, first);
			text += first;
			len -= first;
		}

		if (femo) {
			GtkTextIter iter;
			gint emo_len;
//			g_print ("In: '%s'   [emo]\n", femo->name);
			gtk_text_buffer_get_end_iter (buffer, &iter);
			gtk_text_buffer_insert_pixbuf (buffer, &iter, femo->pixbuf);
			emo_len = strlen (femo->name);
			text += emo_len;
			len -= emo_len;
		}

		if (len <= 0)
			done = TRUE;
	}
}

GList *kf_emoticons_load (const gchar *filename) {
	FILE *f;
	gchar line[1024];
	gchar *path;
	KfEmoticon *emot;

	path = g_path_get_dirname (filename);
	f = fopen (filename, "r");
	if (f == NULL)
		return kf_emoticons_list;
	
	while (fgets (line, 1024, f)) {
		gchar *e_name, *e_file, *tmp;
		e_name = line;

		for (tmp = line; *tmp != '\t' && *tmp != '\0' && *tmp != '\n'; tmp++)
			;
		if (*tmp == '\0' || *tmp == '\n')
			continue;
		else
			*tmp = '\0';
		
		/* Skip whitespace characters */
		++tmp;
		while (isspace (*tmp) && *tmp != '\0')
			tmp++;

		/* Get file name */
		e_file = tmp;

		for (; *tmp != '\0' && *tmp != '\n'; tmp++)
			;
		*tmp = '\0';

//		g_print ("Czytam emoticon %s (%s)... ", e_name, e_file);

		emot = kf_emoticon_new (e_name, path, e_file);
		if (emot) {
			kf_emoticons_list = g_list_append (kf_emoticons_list, emot);
		//	g_print ("ok\n");
		} else {
			//g_print ("failed to load emoticon %s (%s)\n", e_name, e_file);
		}
	}
	fclose (f);
	
	g_free (path);

	return kf_emoticons_list;
}

/* Creates a new Emoticon object */
KfEmoticon *kf_emoticon_new (const gchar *name, const gchar *path, const gchar *file) {
	gchar *fullpath;
	KfEmoticon *emot;
	GdkPixbuf *pixbuf;

	fullpath = g_build_filename (path, file, NULL);
	/* FIXME: I don't know whether to share a single GdkPixbuf among all
	 * with the same filename isn't a better idea; however it would
	 * need a hash of GdkPixbufs indexed by their filenames, thus it
	 * would slow all engine down... So now every kind of emoticon
	 * has its own GdkPixbuf.
	 */

#ifndef ANIM
	pixbuf = gdk_pixbuf_new_from_file (fullpath, NULL);
	g_free (fullpath);
	if (pixbuf == NULL)
		return NULL;
#else
	if (!g_file_test (fullpath, G_FILE_TEST_EXISTS))
		return NULL;
#endif

	emot = g_new (KfEmoticon, 1);
	emot->name = g_strdup (name);
	emot->filename = g_strdup (file);
	emot->pixbuf = pixbuf;

#ifdef ANIM
	emot->path = fullpath;
#endif
	

	return emot;
}

void kf_emoticons_init (void) {
	kf_emoticons_list = kf_emoticons_load (kf_find_file ("/emoticons/emoticons.def"));
	kf_emoticons_list = kf_emoticons_load (kf_config_file ("/emoticons/emoticons.def"));
}

void kf_emo_init_textview (GtkTextView *tv) {
	GtkTextTag *url;
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (tv);

	url = gtk_text_buffer_create_tag (buffer, "link",
			"foreground", kf_preferences_get_string ("colorChatUrl"),
			"underline", PANGO_UNDERLINE_SINGLE,
			NULL);
	gtk_text_buffer_create_tag (buffer, "stamp",
			"foreground", kf_preferences_get_string ("colorChatStamp"),
			NULL);

	g_signal_connect (G_OBJECT (tv), 
                      "button_press_event",
                      G_CALLBACK (textview_clicked_cb),
                      url);
	g_signal_connect (G_OBJECT (tv), 
                      "populate-popup",
                      G_CALLBACK (textview_popup_cb),
                      NULL);
}
	
/* Klikanie na ekranie... 
 *
 * We check if user clicked on a link... if so, we launch a browser:-)
 *
 * Or, if user clicked 3rd mouse button we store the URL
 * 
 * */
static gboolean textview_clicked_cb        (GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data) {
	gint x, y;
	GtkTextIter iter;
	GtkTextTag *url = user_data;

	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_WIDGET,
			event->x, event->y,
			&x, &y);
	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (widget), &iter, x, y);

	if (gtk_text_iter_has_tag (&iter, url)) {
		GtkTextIter start, end;
		gchar *text;

		start = end = iter;

		while (! gtk_text_iter_begins_tag (&start, url))
			gtk_text_iter_backward_char (&start);

		while (! gtk_text_iter_ends_tag (&end, url))
			gtk_text_iter_forward_char (&end);

		text = gtk_text_iter_get_text (&start, &end);

		/* text is our link */
		if (event->button == 1) {
			kf_www_open (text);
		} else if (event->button == 3) {
			g_object_set_data_full (G_OBJECT (widget), "URL", g_strdup (text), g_free);
		}
		g_free (text);

		return FALSE;
	}

  return FALSE;
}

/*
 * Chceck if user clicked on an URL, then add "open link" entry to popup menu
 */
static void textview_popup_cb              (GtkTextView *textview,
                                            GtkMenu *menu,
                                            gpointer user_data) {
	gchar *text;

	if ((text = g_object_get_data (G_OBJECT (textview), "URL"))) {
		GtkWidget *item;

		item = gtk_separator_menu_item_new ();
		gtk_widget_show (item);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

		item = gtk_menu_item_new_with_label (_("Open link"));
		gtk_widget_show (item);
		g_signal_connect (G_OBJECT (item), "activate",
				G_CALLBACK (open_link_cb), text);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	}
}

static void open_link_cb                   (GtkMenuItem *menuitem,
                                            gpointer user_data) {
	kf_www_open (user_data);
}


void kf_display_message (GtkTextBuffer *buffer, glong stamp, const gchar *title, const gchar *body, gboolean yours, const KfEmoTagTable *style) {
	GtkTextIter iter;
	gint offset = 0;
	gchar *caption;	/* Should be enough... */

	if (kf_preferences_get_int ("chatShowStamps")) {
		gchar *sstamp;
		struct tm *stime;
		stime = localtime ((time_t *) &stamp);

		gtk_text_buffer_get_end_iter (buffer, &iter);
		if (stamp < time (NULL) - 3600 * 24) {
			gchar buffer[64];
			gint len;
			
			len = strftime (buffer, 64, "[%d %b %Y %H:%M] ", stime);
			sstamp = g_locale_to_utf8 (buffer, len, NULL, NULL, NULL);
//			sstamp = g_strdup_printf ("[%02d.%02d.%04d %02d:%02d] ",
//					stime->tm_mday, stime->tm_mon, stime->tm_year +1900, stime->tm_hour, stime->tm_min);
		} else
			sstamp = g_strdup_printf ("[%02d:%02d] ", stime->tm_hour, stime->tm_min);
		
//		gtk_text_buffer_insert_with_tags (buffer, &iter, stamp, -1, style->stamp, NULL);
		gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, sstamp, -1, "stamp", NULL);

		g_free (sstamp);
	}
	
	if (strstr (body, "/me ") == body) {
		/* '/me' style message */
		offset = 4;
		caption = g_strdup_printf ("* %s ", title);
	} else {
		if (kf_preferences_get_int ("chatIrcStyle")) {
			caption = g_strdup_printf ("<%s> ", title);
		} else {
			caption = g_strdup_printf ("%s:\n", title);
		}
	}
	
	gtk_text_buffer_get_end_iter (buffer, &iter);
	gtk_text_buffer_insert_with_tags (buffer, &iter, caption, -1,
			yours?style->out:style->in, NULL);

	g_free (caption);

	if (kf_preferences_get_int ("useEmoticons")) {
		//kf_emoticons_insert_text (chat->buffer, msg->body + offset);
		emo_insert (buffer, body + offset, TRUE);
	} else {
		emo_insert (buffer, body + offset, FALSE);
		//gtk_text_buffer_insert (buffer, &iter, body + offset, -1);	
	}
	
	gtk_text_buffer_get_end_iter (buffer, &iter);
	gtk_text_buffer_insert (buffer, &iter, "\n", 1);

}


/**
 * \brief Initialize the \a KfEmoTagTable struct
 **/
void kf_emo_tag_table_init (KfEmoTagTable *table)
{
	table->stamp = NULL;
	table->in = NULL;
	table->out = NULL;
	table->link_cb = NULL;
	table->link_cb_data = NULL;
}
