/* file jisp.c */
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

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>	/* for mkdir () */
#include <sys/types.h>	/*              */
#include "kf.h"
#include "jisp.h"

static KfJispIcon str2jispicon (const gchar *name);
static void kf_jisp_update_preview (KfJisp *jisp);


static gchar *names[] = {"status/online", "status/chat", "status/away", "status/xa", "status/dnd", "status/invisible", "status/offline", NULL};



KfJisp *kf_jisp_new (void) {
	KfJisp *jisp = g_new0 (KfJisp, 1);
	jisp->ref_count = 1;

	g_print ("kf_jisp_new ()\n");

	return jisp;
}


KfJisp *kf_jisp_ref (KfJisp *jisp) {
	jisp->ref_count++;
	return jisp;
}


void kf_jisp_unref (KfJisp *jisp) {
	if (--(jisp->ref_count) == 0)
		kf_jisp_free (jisp);
}


void kf_jisp_free (KfJisp *jisp) {
	int i;

	foo_debug ("Freeing JISP %s (%s)\n", jisp->name, jisp->filename);

	for (i = 0; i < JISP_N_ICONS; i++)
		if (jisp->icons[i])
			gdk_pixbuf_unref (jisp->icons[i]);
	
	gdk_pixbuf_unref (jisp->preview);
	g_free (jisp->name);
	g_free (jisp->desc);
	if (jisp->authors) {
		GList *tmp;

		for (tmp = jisp->authors; tmp; tmp = tmp->next)
			kf_jisp_author_free ((KfJispAuthor *)tmp->data);
		g_list_free (jisp->authors);
	}
	g_free (jisp->filename);
	if (jisp->emoticons) {
		GList *tmp;
		for (tmp = jisp->emoticons; tmp; tmp = tmp->next)
			kf_jisp_emo_free ((KfJispEmo *) tmp->data);
		g_list_free (jisp->emoticons);
	}
	g_free (jisp);
}


KfJispEmo *kf_jisp_emo_new (void) {
	KfJispEmo *emo = g_new0 (KfJispEmo, 1);
	return emo;
}


void kf_jisp_emo_free (KfJispEmo *emo) {
	GSList *tmp;
	gdk_pixbuf_unref (emo->pix);
	for (tmp = emo->texts; tmp; tmp = tmp->next)
		g_free (tmp->data);
	g_slist_free (emo->texts);
	g_free (emo);
}


KfJisp *kf_jisp_new_from_file (const gchar *file) {
	KfJisp *jisp;
	xmlDocPtr doc;
	xmlNodePtr node;
	gchar *path;
	int i;

	foo_debug ("kf_jisp_new_from_file (%s)\n", file);

	jisp = kf_jisp_new ();
	jisp->filename = g_strdup (file);

	doc = xmlParseFile (file);
	if (doc == NULL) {
		xmlFreeDoc (doc);
		foo_debug ("kf_jisp_new_from_file: Unable to parse document, exiting\n");
		return NULL;
	}

	node = xmlDocGetRootElement (doc);
	if (node == NULL) {
		xmlFreeDoc (doc);
		foo_debug ("kf_jisp_new_from_file: Unable find root element of document, exiting\n");
		return NULL;
	}
	if (xmlStrcmp (node->name, (const xmlChar *) "icondef")) {
		xmlFreeDoc (doc);
		foo_debug ("kf_jisp_new_from_file: Root element is not 'icondef', exiting\n");
		return NULL;
	}

	path = g_path_get_dirname (file);

	for (node = node->xmlChildrenNode; node; node = node->next) {

//		foo_debug (" * processing <%s>\n", node->name);
		if (node->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (xmlStrcmp (node->name, (const xmlChar *) "meta") == 0) {
			/* Meta-information about package */
			xmlNodePtr child;

			for (child = node->xmlChildrenNode; child; child = child->next) {
//				foo_debug ("   * processing <%s>\n", child->name);
				xmlChar *value = xmlNodeGetContent (child);
				if (xmlStrcmp (child->name, (const xmlChar *) "name") == 0) {
					jisp->name = g_strdup (value);
//				} else if (xmlStrcmp (child->name, (const xmlChar *) "") == 0) {
//					jisp-> = g_strdup (value);
				} else if (xmlStrcmp (child->name, (const xmlChar *) "description") == 0) {
					jisp->desc = g_strdup (value);
				} else if (xmlStrcmp (child->name, (const xmlChar *) "author") == 0) {
					KfJispAuthor *author = kf_jisp_author_new (value);
					xmlChar *jid = xmlGetProp (child, "jid");
					xmlChar *email = xmlGetProp (child, "email");
					xmlChar *www = xmlGetProp (child, "www");

					if (jid) author->jid = g_strdup (jid);
					if (email) author->email = g_strdup (email);
					if (www) author->www = g_strdup (www);

					xmlFree (jid);
					xmlFree (email);
					xmlFree (www);

					jisp->authors = g_list_append (jisp->authors, author);
				}
				xmlFree (value);
			}
			
		} else if (xmlStrcmp (node->name, (const xmlChar *) "icon") == 0) {
			/* Icon */
			xmlChar *name = NULL;
			xmlChar *filename = NULL;
			xmlNodePtr child;
			GSList *texts = NULL;
			
			for (child = node->xmlChildrenNode; child; child = child->next) {
//				foo_debug ("   * processing <%s>\n", child->name);
				if (xmlStrcmp (child->name, (const xmlChar *) "x") == 0) {
					/* Type of icon */
					xmlChar *key;
					key = xmlGetProp (node, "xmlns");
						name = xmlNodeGetContent (child);
					xmlFree (key);
				} else if (xmlStrcmp (child->name, (const xmlChar *) "text") == 0) {
					/* Text string representing emoticon, eg ':)' */
					gchar *text = xmlNodeGetContent (child);
					texts = g_slist_prepend (texts, g_strdup (text));
					xmlFree (text);
				} else if (xmlStrcmp (child->name, (const xmlChar *) "object") == 0) {
					/* Filename of image */
					filename = xmlNodeGetContent (child);
				}
			}
//			foo_debug ("        icon name='%s' file='%s'\n", name, filename);
			if (name && filename) {
				/* Probably roster icon */
				KfJispIcon i;

				/* get index */
				i = str2jispicon (name);
//				foo_debug ("str2jispicon (%s) = %d\n", name, i);
				if (i >= 0 && i < JISP_N_ICONS) {
					/* Load image */
					GdkPixbuf *image;
					GError *error = NULL;
					gchar *img_path = g_strdup_printf ("%s/%s", path, filename);
					foo_debug ("Trying to load image %s... ", img_path);
					image = gdk_pixbuf_new_from_file (img_path, &error);

					if (image) {
						jisp->icons[i] = image;
						foo_debug ("loaded\n");
					} else {
						foo_debug ("failed to load\n");
					}
					g_free (img_path);
				}
			} else if (texts && filename) {
				/* Probably emoticon */
				GdkPixbuf *image;
				GError *error = NULL;
				gchar *img_path = g_strdup_printf ("%s/%s", path, filename);
				foo_debug ("Trying to load image %s... ", img_path);
				image = gdk_pixbuf_new_from_file (img_path, &error);

				if (image) {
					KfJispEmo *emo;

					emo = kf_jisp_emo_new ();
					texts = g_slist_reverse (texts);
					emo->pix = image;
					emo->texts = texts;
					foo_debug ("loaded\n");
				} else {
					foo_debug ("failed to load\n");
				}
				g_free (img_path);
			}
			xmlFree (name);
			xmlFree (filename);
		}
	}

	/* Check whether JISP has roster icons */
	for (i = 0; i < JISP_N_ICONS; i++)
		if (jisp->icons[i])
			jisp->roster = TRUE;

	kf_jisp_update_preview (jisp);

	g_free (path);
	xmlFreeDoc (doc);
	xmlCleanupParser ();

	return jisp;
}


KfJisp *kf_jisp_install (const gchar *zipfile) {
	FILE *p;	/* Pipe */
	GSList *files = NULL;	/* Files contained in archive */
	gchar *file;	/* quoted zipfile */
	gchar *cmd;	/* Command */
	KfJisp *jisp;
	gchar buffer[4096];
	const gchar *root = NULL;	/* Root path of zipfile */
	const gchar *icondef = NULL;
	gboolean good_jisp = TRUE;
	const gchar *path;
	gchar *icondef_absolute;
	
	foo_debug ("kf_jisp_install (%s)\n", zipfile);
	
	if (! g_file_test (zipfile, G_FILE_TEST_EXISTS)) {
		foo_debug ("kf_jisp_install: file '%s' not found, exiting\n", zipfile);
		return NULL;
	}

	file = g_shell_quote (zipfile);
	cmd = g_strdup_printf ("zipinfo -1 %s", file);
	p = popen (cmd, "r");

	if (p == NULL) {
		g_free (file);
		foo_debug ("kf_jisp_install: unable to pipe '%s', exiting\n", cmd);
		g_free (cmd);
		return NULL;
	}
	g_free (cmd);
	
	while (fgets (buffer, 4096, p)) {
		gchar *line;
		/* Get rid of '\n' at the end of line */
		gchar *c = buffer;
		while (*c != '\n')
			c++;
		*c = '\0';

		foo_debug (" * file %s\n", buffer);

		line = g_strdup (buffer);
		if (root == NULL) {
			root = line;
		} else {
			/* Check if there is more than one directory */
			if (strstr (line, root) != line) {
				foo_debug ("Line '%s' is corrupted!\n", line);
				good_jisp = FALSE;
			}
			/* Check if we have icondef.xml in this
			 * archive */
			if (strstr (line, "icondef.xml") != NULL)
				icondef = line;
		}
		files = g_slist_prepend (files, line);
	}
	pclose (p);
	
	if (files == NULL) {
		foo_debug ("kf_jisp_install: archive '%s' contains no files, exiting\n", file);
		g_free (file);
		return NULL;
	}

	foo_debug (" * good_jisp=%d icondef='%s'\n", good_jisp, icondef);
	if (! good_jisp || ! icondef) {
		/* It's no good file :-/ */
		foo_debug ("kf_jisp_install: archive '%s' is broken, exiting\n", file);
		g_free (file);
		return NULL;
	}
	foo_debug ("X");

	path = kf_config_file ("icons");
	if (! g_file_test (path, G_FILE_TEST_IS_DIR)) {
		foo_debug (" * Creating %s\n", path);
		mkdir (path, 00753);
	}

	/* Extract files */
	cmd = g_strdup_printf ("unzip -d %s %s", path, file);
	foo_debug (" * running '%s'\n", cmd);
	system (cmd);
	g_free (cmd);

	icondef_absolute = g_strdup_printf ("%s/%s", path, icondef);
	jisp = kf_jisp_new_from_file (icondef_absolute);
	g_free (icondef_absolute);

	if (jisp == NULL) {
		/* Get rid of useless files */
		GSList *tmp;

		for (tmp = files; tmp; tmp=tmp->next) {
			gchar *absolute = g_strdup_printf ("%s/%s", path, (gchar *) tmp->data);
			remove (absolute);
			g_free (absolute);
			g_free (tmp->data);
		}
	} else {
		GSList *tmp;

		for (tmp = files; tmp; tmp=tmp->next) {
			g_free (tmp->data);
		}
	}
	g_slist_free (files);

	g_free (file);
	return jisp;
}


/* Uninstall Jisp file form filesystem */
void kf_jisp_uninstall (KfJisp *jisp) {
	gchar *path;
	GDir *dir;
	const gchar *file;

	path = g_path_get_dirname (jisp->filename);
	dir = g_dir_open (path, 0, NULL);
	while ((file = g_dir_read_name (dir)) != NULL)
		remove (file);
	g_dir_close (dir);
	remove (path);

	g_free (path);
	kf_jisp_unref (jisp);
}

/* Convert name in icondef.xml file to KfJispIcon */
static KfJispIcon str2jispicon (const gchar *name) {
	gint i = 0;
	while (names[i] != NULL) {
		if (strcmp (name, names[i]) == 0)
			break;
		i++;
	}
	if (names[i] == NULL)
		return -1;
	else
		return i;
}


static void kf_jisp_update_preview (KfJisp *jisp) {
	gint width = 0;
	gint height = 0;
	GdkPixbuf *preview;
	gint i;
	gint x = 0;

	foo_debug ("kf_jisp_update_preview ()\n");

	/* Compute preview size */
	for (i = 0; i < KF_ICON_UNAVAILABLE; i++) {
		if (jisp->icons[i]) {
			width += gdk_pixbuf_get_width (jisp->icons[i]) + 2;
			height = MAX (height, gdk_pixbuf_get_height (jisp->icons[i]));
		}
	}

	preview = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
			TRUE, 8,
			width, height);
	if (preview == NULL) {
		foo_debug ("unable to create preview\n");
		return;
	}
	/* Fill everything with transparent color;-) */
	gdk_pixbuf_fill (preview, 0x00000000);
	for (i = 0; i < KF_ICON_UNAVAILABLE; i++) {
		if (jisp->icons[i]) {
			gdk_pixbuf_copy_area (jisp->icons[i],
				0, 0,
				gdk_pixbuf_get_width (jisp->icons[i]),
				gdk_pixbuf_get_height (jisp->icons[i]),
				preview,
				x, 0);
			x += gdk_pixbuf_get_width (jisp->icons[i]) + 2;
		}
	}
	if (jisp->preview)
		gdk_pixbuf_unref (jisp->preview);
	jisp->preview = preview;
	foo_debug ("kf_jisp_update_preview... done\n");
}

KfJispAuthor *kf_jisp_author_new (const gchar *name) {
	KfJispAuthor *author = g_new0 (KfJispAuthor, 1);
	author->name = g_strdup (name);
	return author;
}


void kf_jisp_author_free (KfJispAuthor *author) {
	g_free (author->name);
	g_free (author->jid);
	g_free (author->email);
	g_free (author->www);
	g_free (author);
}
