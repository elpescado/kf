/* file emoticons.h */
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

typedef void (*KfLinkCallback) (GtkTextBuffer *, const gchar *url, glong time, gpointer data);


void kf_emoticons_init (void);
void emo_insert (GtkTextBuffer *buffer, const gchar *text, gboolean with_emoticons);
void kf_emo_init_textview (GtkTextView *tv);

typedef struct {
	GtkTextTag *stamp;
	GtkTextTag *in;
	GtkTextTag *out;
	KfLinkCallback link_cb;
	gpointer link_cb_data;
} KfEmoTagTable;

void kf_emo_tag_table_init (KfEmoTagTable *);

void kf_display_message (GtkTextBuffer *buffer, glong stamp, const gchar *title, const gchar *body, gboolean yours, const KfEmoTagTable *style);
