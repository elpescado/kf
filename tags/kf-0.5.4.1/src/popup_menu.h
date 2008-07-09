/* file popup_menu.h */
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


void        on_popup_add_contact_activate  (GtkMenuItem *menuitem,
                                            gpointer user_data);
void on_popup_message_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_popup_chat_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_popup_delete_activate (GtkMenuItem *item, gpointer user_data);
void on_popup_info_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_popup_presence_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_popup_log_in_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_popup_log_out_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_popup_history_activate (GtkMenuItem *menuitem, gpointer user_data);
void        on_popup_register_activate  (GtkMenuItem *menuitem,
                                            gpointer user_data);
void        on_popup_search_activate  (GtkMenuItem *menuitem,
                                            gpointer user_data);
void kf_popup_groups_menu_attach (GladeXML *glade, KfJabberRosterItem *roster_item);
void        on_popup_send_contacts_activate(GtkMenuItem *menuitem,
                                            gpointer user_data);
