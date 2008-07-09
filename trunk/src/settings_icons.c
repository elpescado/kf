/* file settings_icons.c */
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
#include "jisp.h"
#include "preferences.h"
#include "settings_icons.h"
#include "gui.h"	/* We want to change icons in roster */
#include "muc.h"	/* We have to change icons in MUC too */
#include "foogc.h"

static void load_jisps (GtkListStore *store, const gchar *path);
static void install_cb (GtkWidget *button, GtkListStore *store);
static void uninstall_cb (GtkWidget *button, GtkListStore *store);
static void get_more_cb (GtkWidget *button, GtkListStore *store);
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void apply_jisp (KfJisp *jisp);
static void setup_textview (GtkTextView *textview);
void update_menu_images (GladeXML *glade, const gchar *prefix);

static FooGC *jisp_rubbish = NULL;


/* in gui_pixmap.c */
void kf_gui_pixmap_set_jisp (KfJisp *jisp);

void kf_jisp_manager (GladeXML *glade) {
	GtkWidget *tv;
	GtkListStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;

	tv = glade_xml_get_widget (glade, "icons_treeview");
	if ((model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv)))) {
		store = GTK_LIST_STORE (model);
		gtk_list_store_clear (store);
	} else {
		store = gtk_list_store_new (3,
				G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_POINTER);

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes ("Name",
                                                   renderer,
						   "text", 0,
                                                   NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);
		
		renderer = gtk_cell_renderer_pixbuf_new ();
		column = gtk_tree_view_column_new_with_attributes ("Preview",
                                                   renderer,
						   "pixbuf", 1,
                                                   NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

		gtk_tree_view_set_model (GTK_TREE_VIEW (tv), GTK_TREE_MODEL (store));

		kf_signal_connect (glade, "icon_install", "clicked",
			       G_CALLBACK (install_cb), store);
		kf_signal_connect (glade, "icon_uninstall", "clicked",
			       G_CALLBACK (uninstall_cb), store);
		kf_signal_connect (glade, "icon_get_more", "clicked",
			       G_CALLBACK (get_more_cb), store);

		select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
		gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
		g_signal_connect (G_OBJECT (select), "changed",
                	G_CALLBACK (tree_selection_changed_cb),
                	glade);

		setup_textview (GTK_TEXT_VIEW (glade_xml_get_widget (glade, "icons_textview")));
	}

	if (! GTK_IS_LIST_STORE (store)) {
		foo_debug ("evil things happen!\n");
	}
	
	GtkTreeIter iter;
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
		0, _("Default icon set"),
		1, NULL,
		2, NULL,
		-1);

	if (jisp_rubbish)
		foo_gc_free (jisp_rubbish);
	jisp_rubbish = foo_gc_new (kf_jisp_unref);

	/* FIXME */
	load_jisps (store, kf_find_file ("icons"));
	load_jisps (store, kf_config_file ("icons"));
}


static void load_jisps (GtkListStore *store, const gchar *path) {
	GDir *dir;
	const gchar *file;
	
	dir = g_dir_open (path, 0, NULL);
	if (dir == NULL)
		return;

	while ((file = g_dir_read_name (dir))) {
		gchar *filename;
		KfJisp *jisp;

		filename = g_strdup_printf ("%s/%s/icondef.xml", path, file);
		jisp = kf_jisp_new_from_file (filename);
		foo_debug ("Loaded jisp %s", filename);

		if (jisp) {
			GtkTreeIter iter;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
				0, jisp->name,
				1, jisp->preview,
				2, jisp,
				-1);
			foo_gc_add (jisp_rubbish, jisp);
			foo_debug ("Added to list\n");
		}

		g_free (filename);
	}
}


static void install_cb (GtkWidget *button, GtkListStore *store) {
	GtkWidget *win;
	gint response;

#if GTK_CHECK_VERSION(2,4,0)
	win = gtk_file_chooser_dialog_new (_("Select icon set file to open"),
			GTK_WINDOW (gtk_widget_get_toplevel (button)),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_OK,
			NULL);
	GtkFileFilter *filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Jabber icon sets"));
	gtk_file_filter_add_pattern (filter, "*.jisp");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (win), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("all files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (win), filter);
#else
	win = gtk_file_selection_new (_("Install icon set"));
#endif
	response = gtk_dialog_run (GTK_DIALOG (win));
	if (response == GTK_RESPONSE_OK) {
	#if GTK_CHECK_VERSION(2,4,0)
		gchar *filename;
		KfJisp *jisp;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (win));
		jisp = kf_jisp_install (filename);
		g_free (filename);
	#else	
		KfJisp *jisp = kf_jisp_install (
				gtk_file_selection_get_filename (GTK_FILE_SELECTION (win)));
	#endif
		
		if (jisp) {
			GtkTreeIter iter;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
				0, jisp->name,
				1, jisp->preview,
				2, jisp,
				-1);
		} else {
			kf_gui_alert ("Unable to install file");
		}
	}		
	gtk_widget_destroy (win);
}


/**
 * \brief Uninstall a jisp - button callback
 **/
static void uninstall_cb (GtkWidget *button, GtkListStore *store)
{
	foo_debug ("Uninstalling...");
	/* TODO */
}


/**
 * \brief Callback for clicking on 'Get More' button
 **/
static void get_more_cb (GtkWidget *button, GtkListStore *store)
{
	kf_www_open ("http://kf.jabberstudio.org/jisp");
}


static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;
        KfJisp *jisp;
	GladeXML *glade = data;

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
                gtk_tree_model_get (model, &iter, 2, &jisp, -1);
	
		if (jisp) {
			GtkWidget *textview = glade_xml_get_widget (glade, "icons_textview");
			GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
			GtkTextIter iter;
			GList *tmp;
			
			gtk_text_buffer_set_text (buffer, "", 0);
			gtk_text_buffer_get_end_iter (buffer, &iter);
			gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, jisp->name, -1, "h1", NULL);
			gtk_text_buffer_insert (buffer, &iter, "\n", 1);
			gtk_text_buffer_insert (buffer, &iter, jisp->desc, -1);
			gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, _("\nAuthors:\n"), -1, "b", NULL);
			for (tmp = jisp->authors; tmp; tmp = tmp->next) {
				KfJispAuthor *author = tmp->data;
				gtk_text_buffer_insert (buffer, &iter, author->name, -1);
				gtk_text_buffer_insert (buffer, &iter, "\n", 1);
				if (author->jid) {
					gtk_text_buffer_insert (buffer, &iter, "   JabberID: ", -1);
					gtk_text_buffer_insert (buffer, &iter, author->jid, -1);
					gtk_text_buffer_insert (buffer, &iter, "\n", 1);
				}
				if (author->email) {
					gtk_text_buffer_insert (buffer, &iter, "   e-mail: ", -1);
					gtk_text_buffer_insert (buffer, &iter, author->email, -1);
					gtk_text_buffer_insert (buffer, &iter, "\n", 1);
				}
				if (author->www) {
					gtk_text_buffer_insert (buffer, &iter, "   Homepage: ", -1);
					gtk_text_buffer_insert (buffer, &iter, author->www, -1);
					gtk_text_buffer_insert (buffer, &iter, "\n", 1);
				}
			}
		}
		apply_jisp (jisp);
        }
}

static void apply_jisp (KfJisp *jisp) {
	extern GladeXML *iface;
	kf_preferences_set_string ("rosterJisp", jisp?jisp->filename:NULL);
	kf_gui_pixmap_set_jisp (jisp);
	kf_gui_update_roster ();
	kf_muc_icons_changed ();
	kf_dock_icons_changed ();
	update_menu_images (iface, "status");
	update_menu_images (kf_dock_glade (), "dock");

	/* TODO: change status icon */
}


static void setup_textview (GtkTextView *textview) {
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (textview);
	gtk_text_buffer_create_tag (buffer, "h1",
			"scale", PANGO_SCALE_LARGE,
			"weight", PANGO_WEIGHT_BOLD,
			NULL);
	gtk_text_buffer_create_tag (buffer, "b",
			"weight", PANGO_WEIGHT_BOLD,
			NULL);
}


void update_menu_images (GladeXML *glade, const gchar *prefix) {
	int i;
	static gchar *names[] = {"available", "chatty", "away", "xa", "dnd", "invisible", "disconnect"};

	for (i = 0; i < 7; i++) {
		GtkWidget *widget;
		gchar widget_name[32];

		snprintf (widget_name, 32, "%s_%s", prefix, names[i]);
		widget = glade_xml_get_widget (glade, widget_name);
		if (widget) {
			GtkWidget *image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (widget));
			GdkPixbuf *pixbuf = kf_gui_pixmap_get_status (i);
			gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
		}
	}
}
