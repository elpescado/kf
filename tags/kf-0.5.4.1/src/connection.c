/* file connection.c */
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


#include <loudmouth/loudmouth.h>
#include <time.h>
#include "kf.h"
#include "jabber.h"
#include "connection.h"

/* Old Stuff */
extern KfJabberConnection kf_jabber_connection_settings;

/* Local variables */
static LmProxy *connection_proxy = NULL;
static LmSSL *connection_ssl = NULL;



static LmSSLResponse connection_ssl_cb      (LmSSL *ssl,
                                             LmSSLStatus status,
                                             gpointer user_data);


/* Real stuff ********************************************************* */



void kf_connection_set_server (const gchar *server) {
	return;
}

const gchar *kf_connection_get_server (void) {
	return kf_jabber_connection_settings.server;
}

void kf_connection_set_port (gint port) {
	kf_jabber_connection_settings.port = port;
}

gint kf_connection_get_port (void) {
	return kf_jabber_connection_settings.port;
}

void kf_connection_set_priority (gint priority) {
	kf_jabber_connection_settings.priority = priority;
}

gint kf_connection_get_priority (void) {
	return kf_jabber_connection_settings.priority;
}

void kf_connection_set_proxy   (const gchar *server, gint port,
				const gchar *uname, const gchar *passwd) {
	LmProxy *proxy;
	foo_debug ("Setting proxy '%s'...", server);
	if (server) {
		proxy = lm_proxy_new_with_server (LM_PROXY_TYPE_HTTP, server, port);
		if (*uname == '\0') uname = NULL;
		if (*passwd == '\0') passwd = NULL;
		lm_proxy_set_username (proxy, uname);
		lm_proxy_set_password (proxy, passwd);
		g_print ("OK\n");
	} else {
		proxy = NULL;
	}

	connection_proxy = proxy;
}

/* Don't use it; internal */
LmProxy *kf_connection_get_lm_proxy (void) {
	return connection_proxy;
}
	
/* SSL Stuff */
void kf_connection_set_ssl (gboolean set) {
	if (set) {
		LmSSL *ssl;

		ssl = lm_ssl_new (NULL, connection_ssl_cb, NULL, NULL);
		connection_ssl = ssl;
	} else {
		connection_ssl = NULL;
	}
}

/* Dont use it, internal */
LmSSL *kf_connection_get_lm_ssl (void) {
	return connection_ssl;
}

/* SSl Callback function */
static LmSSLResponse connection_ssl_cb      (LmSSL *ssl,
                                             LmSSLStatus status,
                                             gpointer user_data) {
	g_printerr ("SSL Warning #%d\n", status);
	return LM_SSL_RESPONSE_CONTINUE;
}

void kf_connection_set_manual_host (const gchar *host) {
	g_free (kf_jabber_connection_settings.manual_host);
	kf_jabber_connection_settings.manual_host = g_strdup (host);
}


time_t kf_connection_get_time (void) {
	extern time_t kf_connection_time;
	return kf_connection_time;
}
