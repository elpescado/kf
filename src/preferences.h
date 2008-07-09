/* file preferences.h */
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
	gchar *uname;
	gchar *server;
	gchar *pass;
	gboolean save_password;
	gint port;
	gchar *resource;
	gint priority;

	gboolean proxy_use;
	gchar *proxy_server;
	gint   proxy_port;
	gchar *proxy_user;
	gchar *proxy_pass;
	
	gboolean secure;

	gboolean autoconnect;

	gboolean use_manual_host;
	gchar *manual_host;
} KfPrefAccount;

typedef struct {
	gchar *title;
	gchar *server;
	gchar *room;
	gchar *nick;
	gchar *pass;
} KfPrefMUCBookmark;

void kf_preferences_load (const gchar *filename);
gboolean kf_preferences_write (const gchar *filename);
void kf_preferences_set (const gchar *name, const gchar *value);
void kf_preferences_set_int (const gchar *name, int value);
void kf_preferences_set_string (const gchar *name, gchar *value);
gint kf_preferences_get_int (const gchar *name);
const gchar *kf_preferences_get_string (const gchar *name);
gpointer kf_preferences_get_ptr (const gchar *name);
void kf_preferences_add_status (const gchar *name, const gchar *text);
KfPrefAccount *kf_pref_account_new (const gchar *name);

void kf_pref_account_set_proxy (KfPrefAccount *acc, gboolean use,
				const gchar *server, gint port,
				const gchar *uname, const gchar *passwd);

KfPrefAccount *kf_account_get_autoconnect (void);
void kf_account_set_autoconnect (KfPrefAccount *xacc);

gboolean kf_pref_group_is_collapsed (const gchar *grp);
void kf_pref_group_set_collapsed (const gchar *grp, gboolean set);

GList *kf_preferences_muc_bookmarks_get (void);
void kf_preferences_muc_bookmark_add (KfPrefMUCBookmark *bookmark);
void kf_preferences_muc_bookmark_del (KfPrefMUCBookmark *bookmark);
KfPrefMUCBookmark *kf_pref_muc_bookmark_new (void);
void kf_pref_muc_bookmark_free (KfPrefMUCBookmark *bookmark);
