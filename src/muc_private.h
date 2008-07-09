/* file muc_private.h */
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


void kf_muc_init (void);
void kf_muc_join_room (const gchar *jid, const gchar *nick);
void kf_muc_join_room_with_password (const gchar *jid, const gchar *nick, const gchar *pass);
static KfMUC *kf_muc_new (const gchar *jid, const gchar *nick, const gchar *pass);
static KfMUCRoster *kf_muc_roster_new (KfMUC *muc);
static KfMUCRosterItem *kf_muc_roster_item_new (KfMUCRoster *roster, const gchar *name);
static KfMUC *kf_muc_ref (KfMUC *self);
static KfMUCRoster *kf_muc_roster_ref (KfMUCRoster *self);
static KfMUCRosterItem *kf_muc_roster_item_ref (KfMUCRosterItem *self);
static void kf_muc_unref (KfMUC *self);
static void kf_muc_roster_unref (KfMUCRoster *self);
static void kf_muc_roster_item_unref (KfMUCRosterItem *self);
static void kf_muc_free (KfMUC *self);
static void kf_muc_roster_free (KfMUCRoster *self);
static void kf_muc_roster_item_free (KfMUCRosterItem *self);
static KfMUCRoster *kf_muc_get_roster (KfMUC *self);
static void kf_muc_enter_room (KfMUC *self);
static void kf_muc_leave_room (KfMUC *self);
static KfMUC *kf_muc_get (const gchar *jid);
static LmHandlerResult kf_muc_mhandler_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data);
static LmHandlerResult kf_muc_phandler_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data);
static LmHandlerResult kf_muc_ihandler_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data);
static void kf_muc_display_msg_simple (KfMUC *self, const gchar *from, const gchar *body, gulong);
static void kf_muc_display_msg_event_simple (KfMUC *self, const gchar *from, const gchar *body);
static void kf_muc_display_msg_error_simple (KfMUC *self, const gchar *from, gint num, const gchar *body);
static void kf_muc_send_msg_simple (KfMUC *self, const gchar *body);
static void send_cb (GtkButton *button, gpointer data);
static void kf_muc_roster_update (KfMUCRoster *self);
static void kf_muc_roster_init_gui (KfMUCRoster *self);
static void on_text_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static void on_pixbuf_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static KfMUCRosterItem *kf_muc_roster_get_item (KfMUCRoster *self, const gchar *name);
static void kf_muc_topic_changed (KfMUC *self, const gchar *topic);
static void kf_muc_toolbar_connect (KfMUC *self);
static void button_change_cb (GtkButton *button, gpointer data);
static gboolean textview_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static void kf_muc_error (KfMUC *self, LmMessage *msg);
static gboolean treeview_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void kf_muc_popup_menu_connect (GtkWidget *menu);
static void popup_menu_cb (GtkMenuItem *menuitem, gpointer data);
static void menu_set_role_cb (GtkMenuItem *menuitem, gpointer data);
static void menu_set_aff_cb (GtkMenuItem *menuitem, gpointer data);
void kf_muc_roster_item_set_role (KfMUCRosterItem *item, KfMUCRole role);
void kf_muc_roster_item_set_affiliation (KfMUCRosterItem *item, KfMUCAffiliation aff);
static const gchar *kf_muc_role_to_string (KfMUCRole role);
static const gchar *kf_muc_affiliation_to_string (KfMUCAffiliation aff);
static void popup_menu_change_role (GtkMenuItem *item, gpointer data);
static void button_configure_cb (GtkButton *button, gpointer data);
static LmHandlerResult configure_form_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer data);
static void conf_ok_clicked (GtkWidget *widget, gpointer data);
static LmHandlerResult conf_submit (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer data);
static void conf_cancel_clicked (GtkWidget *widget, gpointer data);
static gboolean conf_close_window (GtkWidget *widget, GdkEvent *event, gpointer data);
static void conf_done (KfMUC *self);
static void find_clicked_cb (GtkButton *button, gpointer data);

static KfMUCRole kf_muc_role_from_string (const gchar *str);
static KfMUCAffiliation kf_muc_affiliation_from_string (const gchar *str);
