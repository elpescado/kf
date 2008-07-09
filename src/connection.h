/* file connection.h */
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
#include <time.h>

void kf_connection_set_server (const gchar *server);
const gchar *kf_connection_get_server (void);
void kf_connection_set_port (gint port);
gint kf_connection_get_port (void);
void kf_connection_set_priority (gint priority);
gint kf_connection_get_priority (void);

/* Proxy */

void kf_connection_set_proxy   (const gchar *server, gint port,
				const gchar *uname, const gchar *passwd);
#ifdef __LOUDMOUTH_H__
LmProxy *kf_connection_get_lm_proxy (void);

void kf_connection_set_ssl (gboolean set);
LmSSL *kf_connection_get_lm_ssl (void);
#endif
void kf_connection_set_manual_host (const gchar *host);
time_t kf_connection_get_time (void);
