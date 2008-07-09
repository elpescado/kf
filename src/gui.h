/* file gui.h */
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


typedef struct {
	gchar *name;
	GList *items;
	gboolean expanded;
	guint total;
	guint visible;
	guint available;
} KfGuiRosterGroup;

#ifdef GDK_PIXBUF_VERSION 
typedef struct {
	gchar *filename;
	GdkPixbuf *pixbuf;
} KfGuiPixmap;
GdkPixbuf *kf_gui_pixmap_get_status (guint status);
#endif

#ifndef __GTK_H__ 
# include <gtk/gtk.h>
#endif

/* gui.c */
gboolean kf_gui_init (void);
void kf_gui_shutdown (void);
void kf_gui_show (GtkWidget *wid);
void kf_gui_show_connection_settings (void);
void kf_gui_select_account (GtkMenuItem *menuitem, gpointer user_data);
void kf_gui_setup_treeview (GtkTreeView *tv);
gboolean kfGtkTreeViewSearchEqualFunc (GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer search_data);
void kf_gui_update_roster (void);
KfGuiRosterGroup *kf_gui_group_new (const gchar *name);
void kf_gui_message_normal_display (const gchar *from, const gchar *subject, const gchar *body);
const gchar *kf_gui_get_selected_jid (void);

void on_button_toggle_show_inactive_toggled (GtkToggleButton *togglebutton, gpointer user_data);
const gchar *kf_gui_find_file (const gchar *name);
void kf_gui_disconnected (gboolean error);
void on_roster_tree_view_row_expanded (GtkTreeView *treeview, GtkTreeIter *arg1, GtkTreePath *arg2, gpointer user_data);
void on_roster_tree_view_row_collapsed (GtkTreeView *treeview, GtkTreeIter *arg1, GtkTreePath *arg2, gpointer user_data);
gboolean kf_gui_toggle_show_offline (void);
gboolean kf_gui_set_show_offline (gboolean show);

/* gui_pixmap.c */
gboolean kf_gui_pixmap_init (void);

/* gui_status.c */
void kf_gui_change_status (const gchar *type);
void kf_gui_change_status_select_template (GtkMenuItem *menuitem, gpointer user_data);
void on_status_anuluj_clicked (GtkButton *button, gpointer user_data);
void on_status_ok_clicked (GtkButton *button, gpointer user_data);
void on_add_status_toggled (GtkToggleButton *togglebutton, gpointer user_data);

void kf_gui_alert (const gchar *text);
GList *kf_gui_get_groups (void);

void kf_update_status_by_jid (const gchar *jid, gboolean strong); 

void kf_status_changed (gint type, const gchar *text);

void kf_app_hide (void);
void kf_app_unhide (void);
