/* file archive_viewer.c */
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
#include <libxml/tree.h>
#include <time.h>
#include <string.h>
#include "kf.h"
#include "preferences.h"
#include "jabber.h"
#include "archive_viewer.h"
#include "emoticons.h"
#include "foogc.h"

/* Local function prototypes */

/* Data */
/*
static GList *chatsi	= NULL;
static GList *msgs_in	= NULL;
static GList *msgs_out	= NULL;
*/

//static GList *glista;

/*
 * TODO:
 * - Garbage Collector - free memory - somehow it works...
 */

typedef enum {
	KF_ARCHIVE_MODE_CHAT,
	KF_ARCHIVE_MODE_MSG
} KfArchiveMode;

typedef struct {
	KfArchiveMode mode;
	GtkWidget *window;
	GtkWidget *folders;
	GtkTreeStore *folders_store;
	GtkWidget *conversations;
	GtkListStore *conversations_store;
	GtkWidget *history;
	GtkWidget *history_view;
	GtkWidget *messages;
	GtkListStore *messages_store;

	/* Message view */
	GtkWidget *vpaned;
	GtkWidget *msg_info;
	GtkWidget *subject;
	GtkWidget *from;
	GtkWidget *date;

	GtkTextTag *header_in;
	GtkTextTag *header_out;
} KfArchive;

typedef enum {
	KF_ARCHIVE_FOLDER_KLASS_CHAT,
	KF_ARCHIVE_FOLDER_KLASS_MESSAGES_IN,
	KF_ARCHIVE_FOLDER_KLASS_MESSAGES_OUT,
	KF_ARCHIVE_FOLDER_KLASS_CHATS,
	KF_ARCHIVE_FOLDER_KLASS_MESSAGES,
	KF_ARCHIVE_FOLDER_KLASS_NONE = 999
} KfFolderKlass;

typedef struct {
	KfFolderKlass klass;
	gchar *jid;
	gchar *name;
	gchar *filename;
	xmlDocPtr doc;
} KfArchiveFolder;

typedef struct {
	gchar *name;
	const gchar *nick;
	guint32 start;
	gchar *start_str;
	xmlNodePtr node;
} KfArchiveConversation;

static KfArchive *kf_archive_new (const gchar *jid);
static xmlDocPtr load_data_file (const gchar *name);
static void kf_archive_create_folders (KfArchive *archive);
static void kf_archive_create_conversations (KfArchive *archive, KfArchiveFolder *folder);
static void kf_archive_update_conversations (KfArchive *archive, KfArchiveFolder *folder, gboolean);
static void kf_archive_conversation_free (KfArchiveConversation *conv);
static KfArchiveFolder *kf_archive_folder_new (KfFolderKlass klass);
static void kf_archive_update_folders (KfArchive *archive, const gchar *jid);
static xmlDocPtr load_data_file (const gchar *name);
static void kf_archive_display_conversation (KfArchive *archive, KfArchiveConversation *conversation);
static void kf_archive_create_textview (KfArchive *archive);
static void kf_archive_display_message (KfArchive *archive, GtkTextBuffer *buffer, xmlNodePtr message, const gchar *nick);

static void folders_text_data               (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void
folders_selection_changed (GtkTreeSelection *selection, gpointer data);
static void conversations_text_data         (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void
conversations_selection_changed (GtkTreeSelection *selection, gpointer data);

static void kf_archive_set_mode (KfArchive *self, KfArchiveMode mode);
static void kf_archive_create_messages (KfArchive *self);
static void kf_archive_update_messages (KfArchive *self, const gchar *file, gboolean yours);
static void add_message (GtkListStore *store, xmlNodePtr node, gboolean yours);
static void messages_selection_changed (GtkTreeSelection *selection, gpointer data);
static void	myErrorSAXFunc			(void * ctx, 
					 const char * msg, 
					 ...);

//////////////////////////////////////////////////////////////////////////////
//..........................................................................//
//..........................................................................//
//..........................................................................//
//////////////////////////////////////////////////////////////////////////////

/* Functions */

void kf_archive_viewer (void) {
	kf_archive_new (NULL);
}
void kf_archive_viewer_jid (const gchar *jid) {
	KfArchive *a;
	
	a = kf_archive_new (jid);
//	kf_archive_select_jid (a, jid);
}

static KfArchive *kf_archive_new (const gchar *jid) {
	GladeXML *glade = glade_xml_new (kf_find_file ("archive.glade"), NULL, NULL);
	KfArchive *archive = g_new0 (KfArchive, 1);
	

	kf_glade_get_widgets (glade,
			"archive", &archive->window,
			"folders", &archive->folders,
			"history", &archive->history,
			"conversations", &archive->conversations,
			"messages", &archive->messages,
			"vpaned2", &archive->vpaned,
			"msg_info", &archive->msg_info,
			"subject", &archive->subject,
			"from", &archive->from,
			"date", &archive->date,
			"scrolledwindow1", &archive->history_view,
			NULL);
	g_object_unref (G_OBJECT (glade));
	kf_archive_set_mode (archive, KF_ARCHIVE_MODE_CHAT);

	if (! kf_preferences_get_int ("enableMessageArchive")) {
		/* Archive is turned off */
		GtkWidget *msg;

		msg = gtk_message_dialog_new (GTK_WINDOW (archive->window),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_INFO,
						GTK_BUTTONS_OK,
						_("Message Archive has been turned off. Go to Settings window to change that."));
		g_signal_connect_swapped (GTK_OBJECT (msg), "response",
				G_CALLBACK (gtk_widget_destroy),
				GTK_OBJECT (msg));
		gtk_widget_show (msg);
	}


	/* Setup List store */

	kf_archive_create_folders (archive);
	kf_archive_create_conversations (archive, NULL);
	kf_archive_create_messages (archive);
	kf_emo_init_textview (GTK_TEXT_VIEW (archive->history));
	kf_archive_create_textview (archive);
	kf_archive_update_folders (archive, jid);
	return archive;
	//load_data ();
}

static void kf_archive_create_folders (KfArchive *archive) {
	GtkCellRenderer *renderer;
//	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	archive->folders_store = gtk_tree_store_new (1, G_TYPE_POINTER);
	gtk_tree_view_set_model (GTK_TREE_VIEW (archive->folders), GTK_TREE_MODEL (archive->folders_store));

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (
				GTK_TREE_VIEW (archive->folders),
				-1,
				_("Folders"),
				renderer,
				folders_text_data,
				NULL,
				NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive->folders));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (selection), "changed",
                  G_CALLBACK (folders_selection_changed),
                  archive);
}

static void folders_text_data               (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
	KfArchiveFolder *folder;

	static gchar *chats = NULL, *messages, *inbox, *outbox;
	if (chats == NULL) {
		chats = _("Chats");
		messages = _("Messages");
		inbox = _("Inbox");
		outbox = _("Outbox");
	}

	gtk_tree_model_get (tree_model, iter, 0, &folder, -1);
	if (folder->klass == KF_ARCHIVE_FOLDER_KLASS_CHAT) {
		g_object_set (G_OBJECT (renderer), "font", "normal", NULL);
		if (folder->name)
			g_object_set (G_OBJECT (renderer), "text", folder->name, "style", PANGO_STYLE_NORMAL, NULL);
		else
			g_object_set (G_OBJECT (renderer), "text", folder->jid, "style", PANGO_STYLE_ITALIC, NULL);
	} else if (folder->klass == KF_ARCHIVE_FOLDER_KLASS_CHATS) {
		g_object_set (G_OBJECT (renderer), "text", chats,"font", "bold", NULL);
	} else if (folder->klass == KF_ARCHIVE_FOLDER_KLASS_MESSAGES) {
		g_object_set (G_OBJECT (renderer), "text", messages,"font", "bold", NULL);
	} else if (folder->klass == KF_ARCHIVE_FOLDER_KLASS_MESSAGES_IN) {
		g_object_set (G_OBJECT (renderer), "text", inbox,"font", "normal", NULL);
	} else if (folder->klass == KF_ARCHIVE_FOLDER_KLASS_MESSAGES_OUT) {
		g_object_set (G_OBJECT (renderer), "text", outbox,"font", "normal", NULL);
	}
}

static void
folders_selection_changed (GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;
	KfArchive *archive = data;

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
		KfArchiveFolder *folder;
                gtk_tree_model_get (model, &iter, 0, &folder, -1);

                g_print ("You selected %s\n", folder->jid);
	
		if (folder->klass == KF_ARCHIVE_FOLDER_KLASS_CHAT) {
		//	if (folder->doc == NULL) {
			kf_archive_set_mode (data, KF_ARCHIVE_MODE_CHAT);
			folder->doc = load_data_file (folder->filename);
		//	}
			g_object_set_data_full (G_OBJECT (selection), "docc", "Selection is destroyed\n", 
					(GDestroyNotify) g_print);
			g_object_set_data_full (G_OBJECT (archive->window), "doc", folder->doc, 
					(GDestroyNotify) xmlFreeDoc);

			kf_archive_update_conversations (data, folder, FALSE);
		} else if (folder->klass == KF_ARCHIVE_FOLDER_KLASS_MESSAGES_OUT) {
			kf_archive_set_mode (data, KF_ARCHIVE_MODE_MSG);
			kf_archive_update_messages (data, kf_config_file ("archive/msgs/outbox.xml"), TRUE);
		} else if (folder->klass == KF_ARCHIVE_FOLDER_KLASS_MESSAGES_IN) {
			kf_archive_set_mode (data, KF_ARCHIVE_MODE_MSG);
			kf_archive_update_messages (data, kf_config_file ("archive/msgs/inbox.xml"), FALSE);
		}
        }
}

static void kf_archive_create_conversations (KfArchive *archive, KfArchiveFolder *folder) {
	GtkCellRenderer *renderer;
//	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	archive->conversations_store = gtk_list_store_new (1, G_TYPE_POINTER);
	gtk_tree_view_set_model (GTK_TREE_VIEW (archive->conversations), GTK_TREE_MODEL (archive->conversations_store));

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (
				GTK_TREE_VIEW (archive->conversations),
				-1,
				_("Conversations"),
				renderer,
				conversations_text_data,
				NULL,
				NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive->conversations));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (selection), "changed",
                  G_CALLBACK (conversations_selection_changed),
                  archive);
}

static void kf_archive_update_conversations (KfArchive *archive, KfArchiveFolder *folder, gboolean open_last) {
	xmlNodePtr node;
//	gint i = 0;
	FooGC *gc;

	gc = foo_gc_new ((GDestroyNotify) kf_archive_conversation_free);

	gtk_list_store_clear (archive->conversations_store);
	node = xmlDocGetRootElement (folder->doc);
	if (xmlStrcmp (node->name, (const xmlChar *) "archive") != 0) {
		foo_debug ("node->name = '%s'\n", node->name);
		return;
	}
	for (node = node->children; node; node = node->next) {
		foo_debug ("Reading item %s\n", node->name);
		KfArchiveConversation *conversation = NULL;
		if (xmlStrcmp (node->name, (const xmlChar *) "item") == 0) {
			GtkTreeIter iter;
			xmlChar *name;
			xmlChar *stamp;
			struct tm *czas;
			gchar bufor[1024];
			KfJabberRosterItem *item;

			conversation = g_new (KfArchiveConversation, 1);
			name = xmlGetProp (node, "name");
			if (name) {
				conversation->name = g_strdup (name);
				xmlFree (name);
			} else {
				//conversation->name = g_strdup_printf ("Conv %d", i++);
				conversation->name = NULL;
			}

			stamp = xmlGetProp (node, "start");
			conversation->start = (stamp)?x_stamp_to_time (stamp):0;
			xmlFree (stamp);

			czas = localtime ((time_t *) &(conversation->start));
			//strftime (bufor, 1024, "%x", czas);
			strftime (bufor, 1024, "%d %b %Y", czas);
			/* TODO: do the error handling */
			conversation->start_str = g_locale_to_utf8 (bufor, -1, NULL, NULL, NULL);
			//conversation->start_str = g_strdup (bufor);
			
			conversation->node = node;
			foo_debug ("folder: %p, folder->jid: %s\n", folder, folder->jid);
			if (folder->jid) {
				if ((item = kf_jabber_roster_item_get (folder->jid)))
					conversation->nick = item->display_name;
				else
					conversation->nick = folder->jid;
			} else
				conversation->nick = folder->jid;
			foo_gc_add (gc, conversation);

			
			gtk_list_store_append (archive->conversations_store, &iter);
			gtk_list_store_set (archive->conversations_store, &iter, 0, conversation, -1);
		}
		if (open_last && conversation) {
			kf_archive_display_conversation (archive, conversation);
		}
	}

	g_object_set_data_full (G_OBJECT (archive->window), "convGC", gc, 
			(GDestroyNotify) foo_gc_free);
}

static void kf_archive_conversation_free (KfArchiveConversation *conv) {
	foo_debug ("Freeing conversation %s...\n", conv->start_str);
	g_free (conv->name);
	g_free (conv->start_str);
	g_free (conv);
}

static void conversations_text_data         (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
	KfArchiveConversation *folder;

	gtk_tree_model_get (tree_model, iter, 0, &folder, -1);
	if (folder->name) {
		g_object_set (G_OBJECT (renderer), "text", folder->name, NULL);
	} else {
		/* TODO */
		g_object_set (G_OBJECT (renderer), "text", folder->start_str, NULL);
	}
}

static void
conversations_selection_changed (GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
		KfArchiveConversation *conversation;
                gtk_tree_model_get (model, &iter, 0, &conversation, -1);

		if (conversation) {
	                g_print ("You selected conversation %s\n", conversation->name);

			kf_archive_display_conversation (data, conversation);
		}
        }
}

static void kf_archive_display_conversation (KfArchive *archive, KfArchiveConversation *conversation)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (archive->history));
	xmlNodePtr node;
	GtkTextIter start, end;

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_delete (buffer, &start, &end);
	for (node = conversation->node->children; node; node = node->next) {
		foo_debug ("Processing <%s>\n", node->name);
		if (strcmp (node->name, "message") == 0) {
//			xmlNodePtr child;

			/* Here we shound display message */

			kf_archive_display_message (archive, buffer, node, conversation->nick);
		}
	}
}

static KfArchiveFolder *kf_archive_folder_new (KfFolderKlass klass) {
		KfArchiveFolder *folder;
		folder = g_new (KfArchiveFolder, 1);
		folder->klass = klass;
		folder->name = NULL;
		folder->jid = NULL;
		folder->filename = NULL;
		folder->doc = NULL;
		return folder;
}

static void kf_archive_folder_free (KfArchiveFolder *folder) {
	foo_debug ("Freeing folder %s...\n", folder->jid);
	g_free (folder->jid);
	g_free (folder->filename);
	g_free (folder);
}


/* Loads archive data into memory */
static void kf_archive_update_folders (KfArchive *archive, const gchar *display_jid) {
	GDir *dir;
	const gchar *file;
	const gchar *path;
	KfArchiveFolder *folder;
	GtkTreeIter parent, msg;
	FooGC *gc;

	path = kf_config_file ("archive/chats");

	dir = g_dir_open (path, 0, NULL);
	g_return_if_fail (dir);

	gc = foo_gc_new ((GDestroyNotify) kf_archive_folder_free);

	folder = kf_archive_folder_new (KF_ARCHIVE_FOLDER_KLASS_CHATS);
	gtk_tree_store_append (archive->folders_store, &parent, NULL);
	gtk_tree_store_set (archive->folders_store, &parent, 0, folder, -1);

	while ((file = g_dir_read_name (dir))) {
		gchar *filename;
		KfArchiveFolder *folder;
		GtkTreeIter iter;
		KfJabberRosterItem *item;
		gint len;
		
		/* TODO */
		/* Check file extension - it should be *.xml */

		filename = g_strdup_printf ("%s/%s", path, file);
		folder = kf_archive_folder_new (KF_ARCHIVE_FOLDER_KLASS_CHAT);
		folder->jid = g_strdup (file);
		folder->filename = filename;
		foo_gc_add (gc, folder);

		len = strlen (folder->jid);
		if (len > 4)
			*(folder->jid + len - 4) = '\0';

		item = kf_jabber_roster_item_get (folder->jid);
		if (item && item->name)
			folder->name = g_strdup (item->name);
		else
			folder->name = NULL;

		gtk_tree_store_append (archive->folders_store, &iter, &parent);
		gtk_tree_store_set (archive->folders_store, &iter, 0, folder, -1);

		if (display_jid && strcmp (folder->jid, display_jid) == 0) {
			folder->doc = load_data_file (folder->filename);
			g_object_set_data_full (G_OBJECT (archive->window), "doc", folder->doc, 
					(GDestroyNotify) xmlFreeDoc);
			kf_archive_update_conversations (archive, folder, TRUE);
		}
		
//		load_data_file (filename);
//		g_free (filename);
	}
	folder = kf_archive_folder_new (KF_ARCHIVE_FOLDER_KLASS_MESSAGES);
	gtk_tree_store_append (archive->folders_store, &parent, NULL);
	gtk_tree_store_set (archive->folders_store, &parent, 0, folder, -1);

	folder = kf_archive_folder_new (KF_ARCHIVE_FOLDER_KLASS_MESSAGES_IN);
	gtk_tree_store_append (archive->folders_store, &msg, &parent);
	gtk_tree_store_set (archive->folders_store, &msg, 0, folder, -1);
	
	folder = kf_archive_folder_new (KF_ARCHIVE_FOLDER_KLASS_MESSAGES_OUT);
	gtk_tree_store_append (archive->folders_store, &msg, &parent);
	gtk_tree_store_set (archive->folders_store, &msg, 0, folder, -1);

	g_object_set_data_full (G_OBJECT (archive->window), "folderGC", gc, 
			(GDestroyNotify) foo_gc_free);

	g_dir_close (dir);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (archive->folders));
	foo_debug ("[0]");
}


/* Loads data from filename xml file */
static xmlDocPtr load_data_file (const gchar *name) {
	xmlDocPtr doc = NULL;
	xmlNodePtr node;

	
	FILE *f;
	foo_debug (" * reading '%s'\n", name);

            f = fopen(name, "r");
            if (f != NULL) {
                int res, size = 1024;
                char chars[1024];
                xmlParserCtxtPtr ctxt;

           //     res = fread(chars, 1, 4, f);
        //        if (res > 0) {
                    //ctxt = xmlCreatePushParserCtxt(NULL, NULL,
                    //            chars, res, NULL);
                    ctxt = xmlCreatePushParserCtxt(NULL, NULL,
                                NULL, 0, NULL);
		    ctxt->sax->warning = myErrorSAXFunc;
		    ctxt->sax->error = myErrorSAXFunc;
                //    xmlParseChunk(ctxt, "<archive>", 9, 0);
                    while ((res = fread(chars, 1, size, f)) > 0) {
	//		    g_print ("chunk:\n%.1024s\n", chars);
                        xmlParseChunk(ctxt, chars, res, 0);
                    }
                    xmlParseChunk(ctxt, chars, 0, 1);
                //    xmlParseChunk(ctxt, "</item>",7, 0);
                //    xmlParseChunk(ctxt, "</archive>", 10, 0);
                    doc = ctxt->myDoc;
                    xmlFreeParserCtxt(ctxt);
          //      }
            }

	if (doc) {
    node = xmlDocGetRootElement(doc);
//    xmlDocDump (stdout, doc);
	}
	return doc;
}

static void	myErrorSAXFunc			(void * ctx, 
					 const char * msg, 
					 ...)
{
	/* Skip all these evil warnings! */
//	g_print ("Bubu ", msg);
}

static void kf_archive_create_textview (KfArchive *archive) {
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (archive->history));

	archive->header_in = gtk_text_buffer_create_tag (buffer, "his_text",
			"pixels-above-lines", 4,
			"foreground", kf_preferences_get_string ("colorChatHe"),
			NULL);
	archive->header_out = gtk_text_buffer_create_tag (buffer, "my_text",
			"pixels-above-lines", 4,
			"foreground", kf_preferences_get_string ("colorChatMe"),
			NULL);
}

static void kf_archive_display_message (KfArchive *archive, GtkTextBuffer *buffer, xmlNodePtr message, const gchar *nick) {
	xmlNodePtr child;
	xmlChar *from;
	xmlChar *to;
	xmlChar *stamp;
	time_t stamp_t;
//	gchar *caption;
	struct tm *time;
//	GtkTextIter iter;
	gboolean outcoming;
	xmlChar *body = NULL;
	KfEmoTagTable style = {NULL, archive->header_in, archive->header_out};

	from = xmlGetProp (message, "from");
	to = xmlGetProp (message, "to");
	stamp = xmlGetProp (message, "stamp");

	if (stamp) {
		stamp_t = atoi (stamp);
	} else {
		stamp_t = 0;
	}
	time = localtime ((time_t *) &(stamp_t));

	outcoming = (gboolean) (from == NULL);
	
/*	caption = g_strdup_printf ("[%02d:%02d] %s:\n", time->tm_hour, time->tm_min,
			(outcoming)?((const gchar *) _("Me")):((const gchar *) from));

	gtk_text_buffer_get_end_iter (buffer, &iter);
	gtk_text_buffer_insert_with_tags (buffer, &iter, caption, -1,
			outcoming?(archive->header_out):(archive->header_in), NULL);

	foo_debug ("  message from='%s' to='%s' kf:stamp='%s'\n", from, to, stamp);
*/
	for (child = message->children; child; child = child->next) {
	//	foo_debug ("  Processing <%s>\n", child->name);
		if (xmlStrcmp (child->name, "body") == 0) {
			xmlChar *text;
			text = xmlNodeGetContent (child);
			body = text;
//			xmlFree (text);
//			gtk_text_buffer_get_end_iter (buffer, &iter);
//			gtk_text_buffer_insert (buffer, &iter, (const gchar *)text, -1);
//			emo_insert (buffer, (const gchar *)text);
//			gtk_text_buffer_get_end_iter (buffer, &iter);
//			gtk_text_buffer_insert (buffer, &iter, "\n", 1);
//			xmlFree (text);
		}
	}

	kf_display_message (buffer, stamp_t,
			(outcoming)?((const gchar *) _("Me")):(nick?nick:(const gchar *) from),
			body, outcoming, &style);

	xmlFree (from);
	xmlFree (to);
	xmlFree (stamp);
	xmlFree (body);
}

static void kf_archive_set_mode (KfArchive *archive, KfArchiveMode mode) {
//	if (archive->mode == mode)
//		return;

	archive->mode = mode;

	if (mode == KF_ARCHIVE_MODE_CHAT) {
		gtk_widget_hide (gtk_widget_get_parent (archive->messages));
		gtk_widget_hide (archive->msg_info);
	} else {
		gtk_widget_show (gtk_widget_get_parent (archive->messages));
		gtk_widget_hide (archive->msg_info);
	}
}

static void kf_archive_create_messages (KfArchive *self) {
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	
	self->messages_store = gtk_list_store_new (4,
			G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (self->messages),
			GTK_TREE_MODEL (self->messages_store));

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Subject"),
			renderer,
			"text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->messages), col);

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("JID"),
			renderer,
			"text", 2, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->messages), col);

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Date"),
			renderer,
			"text", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->messages), col);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->messages));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (selection), "changed",
                  G_CALLBACK (messages_selection_changed),
                  self);
}

static void kf_archive_update_messages (KfArchive *self, const gchar *file, gboolean yours) {
	xmlDocPtr doc;
	xmlNodePtr node;
	gtk_list_store_clear (self->messages_store);
	
	doc = load_data_file (file);
	if (doc == NULL)
		return;
	node = xmlDocGetRootElement (doc);
	if (strcmp (node->name, "archive") != 0) {
		foo_debug ("Bad format!\n");
		return;
	}
	for (node = node->children; node; node = node->next) {
		foo_debug ("Proessing <%s>\n", node->name);
		if (strcmp (node->name, "message") == 0) {
			add_message (self->messages_store, node, yours);
		} else if (strcmp (node->name, "item") == 0) {
			xmlNodePtr child;

			for (child = node->children; child; child = child->next) {
			foo_debug (" Proessing <%s>\n", child->name);
				if (strcmp (child->name, "message") == 0) {
					add_message (self->messages_store, child, yours);
				}
			}
		}
	}
	g_object_set_data_full (G_OBJECT (self->window), "MsgDoc", doc, (GDestroyNotify) xmlFreeDoc);
//	xmlDocDump (stdout, doc);
}

static void add_message (GtkListStore *store, xmlNodePtr node, gboolean yours) {
	GtkTreeIter iter;
	gchar *subject = NULL, *from;
	xmlNode *node2;
	xmlChar *stamp;
	time_t stamp_t;
	struct tm *time;
	gchar date_buffer[48];

	for (node2 = node->children; node2; node2 = node2->next) {
		if (strcmp (node2->name, "subject") == 0) {
			subject = xmlNodeGetContent (node2);
		}
	}
	from = xmlGetProp (node, yours?"to":"from");

	stamp = xmlGetProp (node, "stamp");
	if (stamp) {
		stamp_t = atoi (stamp);
		time = localtime ((time_t *) &(stamp_t));
		strftime (date_buffer, 48, "%d %b %Y %T", time);
	} else {
		stamp_t = 0;
	}
	
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, node,
			1, subject?subject:_("No subject"),
			2, from?from:"Who knows...",
			3, stamp_t?date_buffer:"Sometime...",
			-1);

	if (subject) xmlFree (subject);
	if (from) xmlFree (from);
	if (stamp) xmlFree (stamp);
}

static void
messages_selection_changed (GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;
	KfArchive *archive = data;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (archive->history));
	GtkTextIter start, end;
	gboolean with_emoticons = kf_preferences_get_int ("useEmoticons");

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
		xmlNodePtr node, child;
		gchar *subject, *from, *date;
                gtk_tree_model_get (model, &iter, 0, &node, 1, &subject, 2, &from, 3, &date, -1);

		gtk_label_set_text (GTK_LABEL (archive->subject), subject);
		gtk_label_set_text (GTK_LABEL (archive->from), from);
		gtk_label_set_text (GTK_LABEL (archive->date), date);
		gtk_widget_show (archive->msg_info);

		g_free (subject);
		g_free (from);
		g_free (date);

		gtk_text_buffer_get_bounds (buffer, &start, &end);
		gtk_text_buffer_delete (buffer, &start, &end);

		for (child = node->children; child; child = child->next) {
			if (strcmp (child->name, "body") == 0) {
				xmlChar *body;

				body = xmlNodeGetContent (child);
				emo_insert (buffer, (const gchar *)body, with_emoticons);
				xmlFree (body);
			}
		}
        }
}

