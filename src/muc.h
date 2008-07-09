/* file muc.h */
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

typedef struct _KfMUC KfMUC;

void kf_muc_join_room (const gchar *jid, const gchar *nick); 
void kf_muc_join_room_with_password (const gchar *jid, const gchar *nick, const gchar *pass);
void kf_muc_status_changed (gint type, const gchar *text);
void kf_muc_icons_changed (void);

GList *kf_muc_get_list (void);
gchar *kf_muc_get_signature (KfMUC *muc);
void kf_muc_invite_jid (KfMUC *muc, const gchar *jid);
