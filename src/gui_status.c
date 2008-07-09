/* file gui_status.c */
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
#include <string.h>
#include "kf.h"
#include "jabber.h"
#include "gui.h"
#include "preferences.h"
#include "dock.h"

#ifdef HAVE_GTKSPELL
#  include <gtkspell/gtkspell.h>
#endif

/* In muc.c */
void kf_muc_status_changed (gint type, const gchar *text);

static GSList *last_statuses = NULL;
static void kf_status_changed_menuitem (GtkMenuItem *menuitem);

void kf_gui_change_status (const gchar *type) {
	GtkWidget *win;
	extern GladeXML *iface;
	extern GList *kf_preferences_statuses;
	GtkWidget *menu, *optmenu, *add_status;
	GList *list;
	GSList *tmp;
	gint i = 1;
	static int first = 1;
	
	GtkWidget *textview;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	GtkTextMark *insert, *selection_bound;
	
	add_status = glade_xml_get_widget (iface, "add_status");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (add_status), FALSE);

	optmenu = glade_xml_get_widget (iface, "status_template_selector");
	if (!optmenu)
		g_error ("Nie ma menu!\n");
	if ((menu = GTK_WIDGET (gtk_option_menu_get_menu (GTK_OPTION_MENU (optmenu))))) {
		gtk_widget_destroy (menu);
	}


	win = glade_xml_get_widget (iface, "status_change_window");
	g_object_set_data_full (G_OBJECT (win), "kf_status", g_strdup (type), g_free);

	menu = gtk_menu_new ();
	for (tmp = last_statuses; tmp; tmp = tmp->next) {
		GtkWidget *item;
		gchar *text;

		text = g_strdup_printf (_("Last #%d"), i++);
		item = gtk_menu_item_new_with_label (text);
		g_free (text);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate",
				G_CALLBACK (kf_gui_change_status_select_template), tmp->data);
		gtk_widget_show (item);
	}
	for (list = kf_preferences_statuses; list; list=list->next) {
		gchar **element;
		GtkWidget *item;
		
		element = list->data;
		item = gtk_menu_item_new_with_label (element[0]);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate",
				G_CALLBACK (kf_gui_change_status_select_template), element[1]);
		gtk_widget_show (item);
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (optmenu), GTK_WIDGET (menu));
	gtk_widget_show (menu);

	textview = glade_xml_get_widget (iface, "status_textview");
	

	if (first) {
		GError *spell_err = NULL;

#ifdef HAVE_GTKSPELL
		if ( ! gtkspell_new_attach(GTK_TEXT_VIEW(textview), NULL, &spell_err)) {
			foo_debug ("Unable to use GtkSpell: %s\n", spell_err->message);
		}
#endif
		first = 0;
	}

	if (textview == NULL) 
		foo_debug ("XXXX No textview !!!\n");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(textview));
	gtk_text_buffer_set_text (buffer, kf_preferences_get_string ("statusText"), -1);
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	selection_bound = gtk_text_buffer_get_selection_bound (buffer);
	gtk_text_buffer_move_mark (buffer, selection_bound, &start);
	insert = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_move_mark (buffer, insert, &end);

	gtk_widget_grab_focus (textview);
	
	kf_gui_show (win);
}

void  kf_gui_change_status_select_template (GtkMenuItem *menuitem,
                                            gpointer user_data) {
	extern GladeXML *iface;
	GtkWidget *tv;		/* . */
	GtkTextBuffer *buffer;	/* . */
	
	/* Pobranie tre¶ci wiadomo¶ci */
	tv = glade_xml_get_widget (iface, "status_textview");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tv));
	gtk_text_buffer_set_text (buffer, user_data, -1);
}


void        on_status_anuluj_clicked             (GtkButton *button,
                                            gpointer user_data)
{
	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

void        on_status_ok_clicked             (GtkButton *button,
                                            gpointer user_data)
{
	extern GladeXML *iface;
	GtkWidget *tv;		/* . */
	GtkWidget *toggle;
	GtkTextBuffer *buffer;	/* . */
	GtkTextIter start, end;
	gchar *body, *status;
	
	status = g_object_get_data (G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (button))),
			"kf_status");
	
	/* Pobranie tre¶ci wiadomo¶ci */
	tv = glade_xml_get_widget (iface, "status_textview");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tv));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	body = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

	kf_jabber_send_presence (status, body);
	if (status && ! strcmp (status, "unavailable")) {
		kf_jabber_disconnect ();
	} else {
		kf_preferences_set ("statusType", status);
		kf_preferences_set_int ("statusTypeInt", kf_jabber_presence_type_from_string (status));
		kf_preferences_set ("statusText", body);
	}
	
	toggle = glade_xml_get_widget (iface, "add_status");
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle))) {
		GtkWidget *entry;
		const gchar *name;

		entry = glade_xml_get_widget (iface, "status_name");
		name = gtk_entry_get_text (GTK_ENTRY (entry));

		kf_preferences_add_status (name, body);
	}

	if (last_statuses) {
		GSList *tmp;

		while ((tmp = g_slist_nth (last_statuses, 4))) {
			g_free (tmp->data);
			last_statuses = g_slist_delete_link (last_statuses, tmp);
		}
	}
	last_statuses = g_slist_prepend (last_statuses, body);
//	g_free (body);
	
	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

void        on_add_status_toggled          (GtkToggleButton *togglebutton,
                                            gpointer user_data)
{
	GladeXML *glade;
	GtkWidget *entry;

	glade = glade_get_widget_tree (GTK_WIDGET (togglebutton));
	entry = glade_xml_get_widget (glade, "status_name");
	if (gtk_toggle_button_get_active (togglebutton)) {
		gtk_widget_show (entry);
		gtk_entry_set_text (GTK_ENTRY (entry), "Enter name here...");
		gtk_widget_grab_focus (entry);
	} else {
		gtk_widget_hide (entry);
	}
}

//void kf_status_changed (KfJabberPresenceType type, const gchar *text) {
void kf_status_changed (gint type, const gchar *text) {
	static gchar *widget_names[] = {
		"status_available", "status_chatty", "status_away",
		"status_xa", "status_dnd", "status_invisible", "status_disconnect"};
	
	extern GladeXML *iface;
	GtkWidget *menuitem;

	menuitem = glade_xml_get_widget (iface, widget_names[type]);
	kf_status_changed_menuitem (GTK_MENU_ITEM (menuitem));
	kf_dock_update_icon (type, text);
	kf_muc_status_changed (type, text);
}

static void kf_status_changed_menuitem (GtkMenuItem *menuitem) {
	GtkLabel *label;
	GtkImage *image;
	const gchar *text;
	GdkPixbuf *pixbuf;
	GtkImage *status;
	GtkMenuItem *item1;
	GtkLabel *item1_label;
	extern GladeXML *iface;
	
	label = GTK_LABEL (gtk_bin_get_child (GTK_BIN (menuitem)));
	text = gtk_label_get_text (label);
	
	image = GTK_IMAGE (gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (menuitem)));
	pixbuf = gtk_image_get_pixbuf (image);

	status = GTK_IMAGE (glade_xml_get_widget (iface, "my_status_image"));
	item1 = GTK_MENU_ITEM (glade_xml_get_widget (iface, "menuitem1"));
	item1_label = GTK_LABEL (gtk_bin_get_child (GTK_BIN (item1)));
	
	gtk_image_set_from_pixbuf (status, pixbuf);
	gtk_label_set_text (item1_label, text);

//	g_free (text);
}


