/* file callbacks.h */
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


gboolean on_main_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data);
void on_status_available_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_status_away_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_status_xa_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_status_chatty_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_status_dnd_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_status_invisible_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_status_disconnect_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_status_disconnect_x_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_connection_help_clicked (GtkButton *button, gpointer user_data);
void on_menu_new_msg_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_quit_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_prefs_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_archive_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_vcard_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_groupchat_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_add_contact_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_info_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_xml_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_show_offline_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_show_toolbar_activate (GtkMenuItem *menuitem, gpointer user_data);
gboolean on_any_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data);
void on_any_close_window_clicked (GtkButton *button, gpointer user_data);
void close_window (GtkButton *button, gpointer user_data);
gboolean on_roster_tree_view_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data);
void on_xml_send_clicked (GtkButton *button, gpointer user_data);
void on_add_contact_ok_clicked (GtkButton *button, gpointer user_data);
void on_button_add_contact_clicked (GtkButton *button, gpointer user_data);
void on_button_browse_clicked (GtkButton *button, gpointer user_data);
void on_button_search_clicked (GtkButton *button, gpointer user_data);
void on_button_join_clicked (GtkButton *button, gpointer user_data);
void on_button_register_clicked (GtkButton *button, gpointer user_data);
void on_menu_browse_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_search_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_menu_register_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_change_password_activate (GtkMenuItem *menuitem, gpointer user_data);
void kf_gui_add_contact (const gchar *fulljid);
