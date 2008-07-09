/* file archive.c */
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
#include <loudmouth/loudmouth.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "kf.h"
#include "jabber.h"
#include "preferences.h"
#include "archive.h"

time_t kf_get_start_time (void);

static void kf_archive_real (LmMessage *msg, const gchar *jid, time_t stamp, gboolean yours);

/* Puts a LmMessage into archive.
 * set yours to TRUE for outcoming messages */
void kf_archive_msg (LmMessage *msg, gboolean yours, long timestamp) {
//	LmMessageNode *node;
	const gchar *jid;
	gchar *cjid;
	gchar *stamp;

	if (! kf_preferences_get_int ("enableMessageArchive"))
		return;

	g_return_if_fail (msg->node);

//	g_print ("***\n%s\n***\n", lm_message_node_to_string (msg->node));
	if (yours)
		jid = lm_message_node_get_attribute (msg->node, "to");
	else
		jid = lm_message_node_get_attribute (msg->node, "from");

	g_return_if_fail (jid);

	cjid = g_strdup (jid);
	kf_jabber_jid_crop (cjid);

	stamp = g_strdup_printf ("%ld", timestamp);
	//stamp = g_strdup_printf ("%d", time (NULL));
	lm_message_node_set_attribute (msg->node, "kf:stamp", stamp);
	g_free (stamp);

	lm_message_node_set_attribute (msg->node, "kf:direction", yours?"out":"in");

	kf_archive_real (msg, cjid, time (NULL), yours);

	g_free (cjid);
}

static void kf_archive_real (LmMessage *msg, const gchar *jid, time_t stamp, gboolean yours) {
	FILE *f;
	const gchar *path;
	gchar *filename;
	struct tm *time;
	LmMessageSubType type;
	gchar *text;

	path = kf_config_file ("archive");
	if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
		foo_debug ("Creating %s\n", path);
		mkdir (path, 00700);
	}
	
	type = lm_message_get_sub_type (msg);
	//foo_debug ("Type: %d\n", type);
	
	if (type == LM_MESSAGE_SUB_TYPE_NORMAL) {
		path = kf_config_file ("archive/msgs");
	} else if (type == LM_MESSAGE_SUB_TYPE_CHAT) {
		path = kf_config_file ("archive/chats");
	} else if (type == LM_MESSAGE_SUB_TYPE_GROUPCHAT) {
		path = kf_config_file ("archive/muc");
	} else if (type == LM_MESSAGE_SUB_TYPE_HEADLINE) {
		path = kf_config_file ("archive/news");
	} else {
		/* Let's treat these messages as 'normal' */
		path = kf_config_file ("archive/msgs");
	}

	/* Let's create archive directory, if it doesn't exist */
	if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
		mkdir (path, 00700);
	}

	if (type == LM_MESSAGE_SUB_TYPE_NORMAL || type == LM_MESSAGE_SUB_TYPE_NOT_SET) {
		filename = g_strdup_printf ("%s/%s.xml", path, yours?"outbox":"inbox");
	} else {
		filename = g_strdup_printf ("%s/%s.xml", path, jid);
	}

	time = localtime ((time_t *) &(stamp));
	if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
		f = fopen (filename, "ab");

		if (f == NULL) {
			foo_debug ("Unable to open file %s\n", filename);
			g_free (filename);
			return;
		}

		fputs ("<?xml version='1.0'?>\n", f);
		fputs ("<archive xmlns='http://jabber.org/protocol/archive'>\n", f);
		fprintf (f, "<item start=\"%04d%02d%02dT%02d:%02d:%02d\">\n",
			time->tm_year+1900, time->tm_mon+1, time->tm_mday,
			time->tm_hour, time->tm_min, time->tm_sec);
	} else {
		struct stat stat_buffer;
		stat (filename, &stat_buffer);

		f = fopen (filename, "ab");

		if (f == NULL) {
			foo_debug ("Unable to open file %s\n", filename);
			g_free (filename);
			return;
		}

		if (stat_buffer.st_mtime < kf_get_start_time ()) {
			/* File has not been modified during this session */
			foo_debug ("Creating new <item>\n");

			fputs ("</item>\n", f);
			fprintf (f, "<item start=\"%04d%02d%02dT%02d:%02d:%02d\">\n",
			time->tm_year+1900, time->tm_mon+1, time->tm_mday,
			time->tm_hour, time->tm_min, time->tm_sec);
		}
	}
	g_return_if_fail (f);


//	if (fseek (f, -20L, SEEK_END)) {
//		g_print ("AAAA");
//	}

	text = lm_message_node_to_string (msg->node);
	fputs (text, f);
//	fputs ("</archive>\n", f);
	g_free (text);

	fclose (f);

	g_free (filename);
}
