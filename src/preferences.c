/* file preferences.c */
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


#include <glib.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kf.h"
#include "preferences.h"

//static void kf_preferences_dir (void);
static void kf_preferences_dump (void);
static void kf_preferences_load_accounts (xmlDocPtr doc, xmlNodePtr node);
static void kf_preferences_load_statuses (xmlDocPtr doc, xmlNodePtr node);
static GSList *kf_preferences_load_list (xmlDocPtr doc, xmlNodePtr node, GSList *list);
static void kf_preferences_write_list (xmlNodePtr node, const gchar *name, GSList *list);
static void kf_preferences_load_muc_bookmarks (xmlDocPtr doc, xmlNodePtr node);
static void kf_preferences_create_default_colors (void);

gchar *base64_encode(const gchar *buf);
gchar *base64_decode(const gchar *buf);

typedef enum {
	KF_PREF_TYPE_INT,
	KF_PREF_TYPE_STRING
} KfPrefType;

typedef struct {
	gchar *name;
	KfPrefType type;
	gint value_int;
	gchar *value_string;
} KfPref;

/*
 * Ustawienia programu
 */
static KfPref kf_preferences[] = {
	{"ArchiveSizeX",	KF_PREF_TYPE_INT,	0, NULL},
	{"ArchiveSizeY",	KF_PREF_TYPE_INT,	0, NULL},
	{"autoLogin",	KF_PREF_TYPE_INT,	0, NULL},
	{"autoPopup",	KF_PREF_TYPE_INT,	1, NULL},
	{"autoAwayEnabled",	KF_PREF_TYPE_INT,	1, NULL},
	{"autoAwayDelay",	KF_PREF_TYPE_INT,	5, NULL},
	{"autoXADelay",	KF_PREF_TYPE_INT,	15, NULL},
	{"autoAwayText",KF_PREF_TYPE_STRING,	0, "AutoAway"},
	{"autoXAText",	KF_PREF_TYPE_STRING,	0, "AutoXA"},
	{"browser",	KF_PREF_TYPE_STRING,	0, "mozilla"},
	{"chatSizeX",	KF_PREF_TYPE_INT,	0, NULL},
	{"chatSizeY",	KF_PREF_TYPE_INT,	0, NULL},
	{"chatShowStamps",	KF_PREF_TYPE_INT,	1, NULL},
	{"chatShowNotify",	KF_PREF_TYPE_INT,	1, NULL},
	{"chatIrcStyle",	KF_PREF_TYPE_INT,	0, NULL},
	{"colorChatHe",	KF_PREF_TYPE_STRING,	0, "#6D0D0D"},
	{"colorChatMe",	KF_PREF_TYPE_STRING,	0, "#0A1F5F"},
	{"colorChatNotify",	KF_PREF_TYPE_STRING,	0, "#A0A0A0"},
	{"colorChatStamp",	KF_PREF_TYPE_STRING,	0, "#A0A0A0"},
	{"colorChatUrl",KF_PREF_TYPE_STRING,	0, "blue"},
	{"colourRows",	KF_PREF_TYPE_INT,	0, NULL},
	{"dockEnable",	KF_PREF_TYPE_INT,	1, NULL},
	{"enablePopups",	KF_PREF_TYPE_INT,	1, NULL},
	{"enablePopupMsg",	KF_PREF_TYPE_INT,	1, NULL},
	{"enablePopupChat",	KF_PREF_TYPE_INT,	1, NULL},
	{"enablePopupPresence",	KF_PREF_TYPE_INT,	1, NULL},
	{"enablePopupOffline",	KF_PREF_TYPE_INT,	1, NULL},
	{"enableMessageArchive",	KF_PREF_TYPE_INT,	1, NULL},
	{"expertMode",		KF_PREF_TYPE_INT,	0, NULL},
	{"historyViewer",	KF_PREF_TYPE_STRING,	0, NULL},
	{"logMessages",	KF_PREF_TYPE_INT,	0, NULL},
	{"messageDefault",	KF_PREF_TYPE_INT,	0, NULL},
	{"minimizeToTrayOnClose",	KF_PREF_TYPE_INT,	0, NULL},
	{"mucHideStatus",	KF_PREF_TYPE_INT,	0, NULL},
	{"mucLastServer",	KF_PREF_TYPE_STRING,	0, NULL},
	{"mucLastRoom", KF_PREF_TYPE_STRING,	0, NULL},
	{"mucLastNick", KF_PREF_TYPE_STRING,	0, NULL},
	{"myNick", KF_PREF_TYPE_STRING,	0, NULL},
	{"posX",	KF_PREF_TYPE_INT,	0, NULL},
	{"posY",	KF_PREF_TYPE_INT,	0, NULL},
	{"sizeX",	KF_PREF_TYPE_INT,	0, NULL},
	{"sizeY",	KF_PREF_TYPE_INT,	0, NULL},
	{"statusType",	KF_PREF_TYPE_STRING,	0, NULL},
	{"statusTypeInt",	KF_PREF_TYPE_INT,	0, NULL},
	{"statusText",	KF_PREF_TYPE_STRING,	0, NULL},
	{"showOffline",	KF_PREF_TYPE_INT,	0, NULL},
	{"showToolbar",	KF_PREF_TYPE_INT,	1, NULL},
	{"rosterJisp",	KF_PREF_TYPE_STRING,	0, NULL},
	{"rosterShowStatus",	KF_PREF_TYPE_INT,	1, NULL},
	{"soundEnabled",KF_PREF_TYPE_INT,	1, NULL},
	{"soundPlayer",	KF_PREF_TYPE_STRING,	0, "play"},
	{"soundPresence",KF_PREF_TYPE_STRING,	0, ""},
	{"soundPresenceUse", KF_PREF_TYPE_INT,	1,	NULL},
	{"soundOffline",KF_PREF_TYPE_STRING,	0, ""},
	{"soundOfflineUse", KF_PREF_TYPE_INT,	1,	NULL},
	{"soundChat",KF_PREF_TYPE_STRING,	0, ""},
	{"soundChatUse", KF_PREF_TYPE_INT,	1,	NULL},
	{"soundMsg",KF_PREF_TYPE_STRING,	0, ""},
	{"soundMsgUse", KF_PREF_TYPE_INT,	1,	NULL},
	{"soundSmartDelay", KF_PREF_TYPE_INT,	2,	NULL},
	{"tabDefault",	KF_PREF_TYPE_INT,	0, NULL},
	{"tabPosition",	KF_PREF_TYPE_INT,	3, NULL},
	{"terminal",	KF_PREF_TYPE_STRING,	0, "rxvt"},
	{"tooltipDelay",	KF_PREF_TYPE_INT,	800, NULL},
	{"tooltipTimeout",	KF_PREF_TYPE_INT,	10000, NULL},
	{"useEmoticons",	KF_PREF_TYPE_INT,	1, NULL},
	{NULL, 0, 0, NULL}
};

GList *kf_preferences_accounts = NULL;
GList *kf_preferences_statuses = NULL;

GSList *kf_preferences_blocked_jids = NULL;
GSList *kf_preferences_groups_collapsed = NULL;
GSList *kf_preferences_muc_colors = NULL;
GList *kf_preferences_muc_bookmarks = NULL;

/*
 * £aduje preferencje z pliku
 */
void kf_preferences_load (const gchar *filename) {
	xmlDocPtr doc;
	xmlNodePtr node;

	if (! g_file_test (filename, G_FILE_TEST_EXISTS))
		/* Config file does not exist */
		return;

	doc = xmlParseFile (filename);
	if (doc == NULL) {
		xmlFreeDoc (doc);
		return;
	}

	node = xmlDocGetRootElement (doc);
	if (node == NULL) {
		xmlFreeDoc (doc);
		return;
	}
	if (xmlStrcmp (node->name, (const xmlChar *) "configuration")) {
		xmlFreeDoc (doc);
		return;
	}

	node = node->xmlChildrenNode;
	while (node) {
		if (node->type != XML_ELEMENT_NODE) {
			node = node->next;
			continue;
		}
		if (!xmlStrcmp (node->name, (const xmlChar *) "option")) {
			xmlChar *key, *value;
			key = xmlGetProp (node, "name");
//			value = xmlGetProp (node, "value");
			value = xmlNodeGetContent (node);
			if (key && value) {
				kf_preferences_set ((const gchar *) key, (const gchar *) value);
				xmlFree (key);
				xmlFree (value);
		}
		} else if (!xmlStrcmp (node->name, (const xmlChar *) "accounts")) {
			kf_preferences_load_accounts (doc, node->xmlChildrenNode);
		} else if (!xmlStrcmp (node->name, (const xmlChar *) "statuses")) {
			kf_preferences_load_statuses (doc, node->xmlChildrenNode);
		} else if (!xmlStrcmp (node->name, (const xmlChar *) "muc")) {
			kf_preferences_load_muc_bookmarks (doc, node->xmlChildrenNode);
		} else if (!xmlStrcmp (node->name, (const xmlChar *) "blacklist")) {
			 kf_preferences_blocked_jids = kf_preferences_load_list 
				 (doc, node->xmlChildrenNode, kf_preferences_blocked_jids);
		} else if (!xmlStrcmp (node->name, (const xmlChar *) "groups-collapsed")) {
			 kf_preferences_groups_collapsed = kf_preferences_load_list 
				 (doc, node->xmlChildrenNode, kf_preferences_groups_collapsed);
		} else if (!xmlStrcmp (node->name, (const xmlChar *) "muc-colors")) {
			 kf_preferences_muc_colors = kf_preferences_load_list 
				 (doc, node->xmlChildrenNode, kf_preferences_muc_colors);
		}
		node = node->next;
	}

//	kf_preferences_dump ();
	xmlFreeDoc (doc);
	xmlCleanupParser ();

	if (kf_preferences_muc_colors == NULL)
		kf_preferences_create_default_colors ();
}

/*
 * Ustawia zmienn± konfiguracyjn± name na warto¶æ value
 */
void kf_preferences_set (const gchar *name, const gchar *value) {
	KfPref *pref;

	for (pref = kf_preferences; pref && pref->name; pref++) {
		if (!strcmp (name, pref->name)) {
			if (pref->type == KF_PREF_TYPE_INT) {
				pref->value_int = atoi (value);
			} else if (pref->type == KF_PREF_TYPE_STRING) {
				pref->value_string = g_strdup (value);
			}
			break;
		}
	}
}

void kf_preferences_set_string (const gchar *name, gchar *value) {
	KfPref *pref;

	for (pref = kf_preferences; pref && pref->name; pref++) {
		if (!strcmp (name, pref->name)) {
			if (pref->type == KF_PREF_TYPE_INT) {
				pref->value_int = atoi (value);
			} else if (pref->type == KF_PREF_TYPE_STRING) {
				pref->value_string = value;
			}
			break;
		}
	}
}

void kf_preferences_set_int (const gchar *name, gint value) {
	KfPref *pref;

	for (pref = kf_preferences; pref && pref->name; pref++) {
		if (!strcmp (name, pref->name)) {
			if (pref->type == KF_PREF_TYPE_INT) {
				pref->value_int = value;
			} else if (pref->type == KF_PREF_TYPE_STRING) {
				pref->value_string = g_strdup_printf ("%d", value);
			}
			break;
		}
	}
}

gint kf_preferences_get_int (const gchar *name) {
	KfPref *pref;
//	gint value;

	for (pref = kf_preferences; pref && pref->name; pref++) {
		if (!strcmp (name, pref->name)) {
			if (pref->type == KF_PREF_TYPE_INT) {
				return pref->value_int;
			} else if (pref->type == KF_PREF_TYPE_STRING) {
//				pref->value_string = g_strdup (value);
//				return 0;
				return atoi (pref->value_string);
			}
			break;
		}
	}
	return 0;
}

const gchar *kf_preferences_get_string (const gchar *name) {
	KfPref *pref;
//	gchar *string;

	for (pref = kf_preferences; pref && pref->name; pref++) {
		if (!strcmp (name, pref->name)) {
			if (pref->type == KF_PREF_TYPE_INT) {
//				return g_strdup_printf ("%d", pref->value_int);
				return NULL;
			} else if (pref->type == KF_PREF_TYPE_STRING) {
				return pref->value_string;
			}
			break;
		}
	}
	return NULL;
}

gpointer kf_preferences_get_ptr (const gchar *name) {
	KfPref *pref;
//	gchar *string;

	for (pref = kf_preferences; pref && pref->name; pref++) {
		if (!strcmp (name, pref->name)) {
			if (pref->type == KF_PREF_TYPE_INT) {
				return &(pref->value_int);
			} else if (pref->type == KF_PREF_TYPE_STRING) {
				return pref->value_string;
			}
			break;
		}
	}
	return NULL;
}


/* Wypisuje warto¶æ wszystkich opcji - debug */
static void kf_preferences_dump (void) {
	KfPref *pref;

	for (pref = kf_preferences; pref && pref->name; pref++) {
		if (pref->type == KF_PREF_TYPE_INT) {
			g_print ("  <o n='%s' v='%d'/>\n", pref->name, pref->value_int);
		} else if (pref->type == KF_PREF_TYPE_STRING) {
			g_print ("  <o n='%s' v='%s'/>\n", pref->name, pref->value_string);
		}
	}

	if (1) {
		gchar **element;
		GList *list;
		list = kf_preferences_statuses;
		while (list) {
			element = list->data;
			g_print ("[[[ %s | %s ]]]\n", element[0], element[1]);
			list = list->next;
		}
	}
}

gboolean kf_preferences_write (const gchar *filename) {
	xmlDocPtr doc;
	xmlNodePtr root, node;
	KfPref *pref;
	/* Accounts */
	GList *tmp;
	KfPrefAccount *acc;
	
	doc = xmlNewDoc ("1.0");
	root = xmlNewNode (NULL, "configuration");
	xmlDocSetRootElement (doc, root);
	
	/* General preferences */
	for (pref = kf_preferences; pref && pref->name; pref++) {
		if (pref->type == KF_PREF_TYPE_INT) {
			gchar *buffer;

			buffer = g_strdup_printf ("%d", pref->value_int);
			node = xmlNewTextChild (root, NULL, "option", buffer);
			g_free (buffer);
			xmlNewProp (node, "name", pref->name);
		} else if (pref->type == KF_PREF_TYPE_STRING && pref->value_string) {
			node = xmlNewTextChild (root, NULL, "option", pref->value_string);
			xmlNewProp (node, "name", pref->name);
		}
	}

	/* Accounts */
	node = xmlNewChild (root, NULL, "accounts", NULL);
	for (tmp = kf_preferences_accounts; tmp; tmp = tmp->next) {
		xmlNodePtr child;
		
		acc = tmp->data;
		
		child = xmlNewChild (node, NULL, "account", NULL);
		xmlNewProp (child, "name", acc->name);
		xmlNewProp (child, "username", acc->uname);
		xmlNewProp (child, "server", acc->server);
		if (acc->save_password) {
			gchar *pass = base64_encode (acc->pass);
			xmlNewTextChild (child, NULL, "password", pass);
			g_free (pass);
		}

		if (acc->port != -1) {
			gchar *text;

			text = g_strdup_printf ("%d", acc->port);
			xmlNewProp (child, "port", text);
			g_free (text);
		}
		if (acc->resource)
			xmlNewProp (child, "resource", acc->resource);
		
		if (acc->priority > -128 && acc->priority < 128) {
			gchar *text;

			text = g_strdup_printf ("%d", acc->priority);
			xmlNewProp (child, "priority", text);
			g_free (text);
		}

		if (acc->proxy_use) {
			xmlNodePtr proxy;
			gchar port[6];

			proxy = xmlNewChild (child, NULL, "proxy", NULL);
			xmlNewTextChild (proxy, NULL, "enable", acc->proxy_use?"yes":"no");
			xmlNewTextChild (proxy, NULL, "server", acc->proxy_server);

			g_snprintf (port, 6, "%d", acc->proxy_port);
			
			xmlNewTextChild (proxy, NULL, "port", port);
			if (acc->proxy_user)
				xmlNewTextChild (proxy, NULL, "user", acc->proxy_user);
			if (acc->proxy_pass)
				xmlNewTextChild (proxy, NULL, "password", acc->proxy_pass);
		}

		if (acc->use_manual_host) {
			xmlNodePtr n;
			n = xmlNewTextChild (child, NULL, "manual-host", acc->manual_host);
			xmlNewProp (n, "use", "yes");
		}

		xmlNewProp (child, "secure", acc->secure?"yes":"no");
		xmlNewProp (child, "save_passwd", acc->save_password?"yes":"no");
		if (acc->autoconnect) {
			xmlNewChild (child, NULL, "autoconnect", NULL);
		}
	}

	/* Status templates */
	node = xmlNewChild (root, NULL, "statuses", NULL);
	for (tmp = kf_preferences_statuses; tmp; tmp=tmp->next) {
		gchar **status = tmp->data;
		xmlNodePtr child;

		child = xmlNewTextChild (node, NULL, "status", status[1]);
		xmlNewProp (child, "name", status[0]);
	}
	
	kf_preferences_write_list (root, "blacklist", kf_preferences_blocked_jids);
	kf_preferences_write_list (root, "groups-collapsed", kf_preferences_groups_collapsed);
	kf_preferences_write_list (root, "muc_colors", kf_preferences_muc_colors);
	
	/* MUC Bookmarks */
	node = xmlNewChild (root, NULL, "muc", NULL);
	for (tmp = kf_preferences_muc_bookmarks; tmp; tmp = tmp->next) {
		KfPrefMUCBookmark *bookmark = tmp->data;
		xmlNodePtr child;

		child = xmlNewChild (node, NULL, "bookmark", NULL);
		if (bookmark->server) xmlNewTextChild (child, NULL, "server", bookmark->server);
		if (bookmark->room) xmlNewTextChild (child, NULL, "room", bookmark->room);
		if (bookmark->nick) xmlNewTextChild (child, NULL, "nick", bookmark->nick);
		if (bookmark->pass) xmlNewTextChild (child, NULL, "pass", bookmark->pass);
	}


	xmlSaveFormatFileEnc (filename, doc, "UTF-8", 1);
	xmlFreeDoc (doc);
	xmlCleanupParser ();
	return TRUE;
}


static void kf_preferences_write_list (xmlNodePtr root, const gchar *name, GSList *list)
{
	xmlNodePtr node;
	GSList *tmps;
	
	node = xmlNewChild (root, NULL, name, NULL);
	for (tmps = list; tmps; tmps=tmps->next) {
		xmlNewTextChild (node, NULL, "item", (const gchar *) tmps->data);
	}
	
}


static void kf_preferences_load_accounts (xmlDocPtr doc, xmlNodePtr node) {
	
	while (node) {
		xmlChar *name, *uname, *serv, *pass, *port, *res, *priority, *secure, *save_passwd;
		
		if (xmlStrcmp (node->name, (const xmlChar *) "account")) {
			node = node->next;
			continue;
		}

		name = xmlGetProp (node, "name");
		uname = xmlGetProp (node, "username");
		serv = xmlGetProp (node, "server");
		pass = xmlGetProp (node, "password");
		
		port = xmlGetProp (node, "port");
		res = xmlGetProp (node, "resource");
		priority = xmlGetProp (node, "priority");
		secure = xmlGetProp (node, "secure");
		save_passwd = xmlGetProp (node, "save_passwd");
		
		if (name && uname && serv) {
			KfPrefAccount *account;
			xmlNodePtr child;
			
		//	account = g_malloc (sizeof (KfPrefAccount));
			account = kf_pref_account_new (name);
			if (account == NULL)
				return;

//			account->name = g_strdup ((const gchar *) name);
			account->uname = g_strdup ((const gchar *) uname);
			account->server = g_strdup ((const gchar *) serv);
			account->pass = g_strdup ((const gchar *) pass);

			account->port = port?atoi (port):5222;
			account->resource = g_strdup (res);
			account->priority = priority?atoi (priority):5;
			
			if (secure)
				account->secure = (*secure == 'y');

			if (save_passwd)
				account->save_password = (*save_passwd == 'y');

			for (child = node->children; child; child = child->next) {
				if (xmlStrcmp (child->name, (const xmlChar *) "proxy") == 0) {
					xmlNodePtr node;

					for (node = child->children; node; node = node->next) {
						xmlChar *content;

						content = xmlNodeGetContent (node);
						if (xmlStrcmp (node->name, (const xmlChar *) "enable") == 0) {
							account->proxy_use = (content && *content == 'y') ? TRUE:FALSE;
						} else if (xmlStrcmp (node->name, (const xmlChar *) "server") == 0) {
							account->proxy_server = g_strdup (content);
						} else if (xmlStrcmp (node->name, (const xmlChar *) "port") == 0) {
							account->proxy_port = atoi (content);
						} else if (xmlStrcmp (node->name, (const xmlChar *) "user") == 0) {
							account->proxy_user = g_strdup (content);
						} else if (xmlStrcmp (node->name, (const xmlChar *) "password") == 0) {
							account->proxy_pass = g_strdup (content);
						}
						xmlFree (content);
					}
				//	if (*(xmlGetProp (child, "enable")) == 'y')
				//		account->proxy_use = TRUE;
				//	
					
				} else if (xmlStrcmp (child->name, (const xmlChar *) "autoconnect") == 0) {
					account->autoconnect = TRUE;
				} else if (xmlStrcmp (child->name, (const xmlChar *) "manual-host") == 0) {
					xmlChar *host;
					xmlChar *use;

					host = xmlNodeGetContent (child);
					account->manual_host = g_strdup ((gchar *) host);
					
					use = xmlGetProp (child, "use");
					if (use && xmlStrcmp (use, (const xmlChar *) "yes") == 0)
						account->use_manual_host = TRUE;

					xmlFree (host);
					xmlFree (use);
				} else if (xmlStrcmp (child->name, (const xmlChar *) "password") == 0) {
					xmlChar *chars = xmlNodeGetContent (child);
					g_free (account->pass);
					account->pass = base64_decode (chars);
					xmlFree (chars);
				}
					
			}

			kf_preferences_accounts = g_list_append
				(kf_preferences_accounts, account);
		}
		xmlFree (name);
		xmlFree (uname);
		xmlFree (serv);
		xmlFree (pass);

		xmlFree (port);
		xmlFree (res);
		xmlFree (secure);
		xmlFree (save_passwd);

		node = node->next;
	}
}

KfPrefMUCBookmark *kf_pref_muc_bookmark_new (void) {
	KfPrefMUCBookmark *bookmark;

	bookmark = g_new (KfPrefMUCBookmark, 1);
	bookmark->title = NULL;
	bookmark->server = NULL;
	bookmark->room = NULL;
	bookmark->nick = NULL;
	bookmark->pass = NULL;

	return bookmark;
}

void kf_pref_muc_bookmark_free (KfPrefMUCBookmark *bookmark) {
	g_free (bookmark->title);
	g_free (bookmark->server);
	g_free (bookmark->room);
	g_free (bookmark->nick);
	g_free (bookmark->pass);
	g_free (bookmark);
}

static void kf_preferences_load_muc_bookmarks (xmlDocPtr doc, xmlNodePtr node) {
	for (;node;node=node->next) {
		xmlNodePtr child;
		xmlChar *server = NULL;
		xmlChar *room = NULL;
		xmlChar *nick = NULL;
		xmlChar *pass = NULL;

		if (xmlStrcmp (node->name, (const xmlChar *) "bookmark"))
			continue;

		for (child = node->children; child; child=child->next) {
			if (xmlStrcmp (child->name, (const xmlChar *) "server") == 0)
				server = xmlNodeGetContent (child);
			else if (xmlStrcmp (child->name, (const xmlChar *) "room") == 0)
				room = xmlNodeGetContent (child);
			else if (xmlStrcmp (child->name, (const xmlChar *) "nick") == 0)
				nick = xmlNodeGetContent (child);
			else if (xmlStrcmp (child->name, (const xmlChar *) "pass") == 0)
				pass = xmlNodeGetContent (child);
			
		}	
		if (server || room) {
			KfPrefMUCBookmark *bookmark;

			bookmark = kf_pref_muc_bookmark_new ();
			bookmark->server = server?g_strdup (server):NULL;
			bookmark->room = room?g_strdup (room):NULL;
			bookmark->nick = nick?g_strdup(nick):NULL;
			bookmark->pass = pass?g_strdup(pass):NULL;

			kf_preferences_muc_bookmarks = g_list_append (kf_preferences_muc_bookmarks,
					bookmark);
		}

		if (server) xmlFree (server);
		if (room) xmlFree (room);
		if (nick) xmlFree (nick);
		if (pass) xmlFree (pass);
	}
}

GList *kf_preferences_muc_bookmarks_get (void) {
	return kf_preferences_muc_bookmarks;
}

void kf_preferences_muc_bookmark_add (KfPrefMUCBookmark *bookmark) {
	kf_preferences_muc_bookmarks = g_list_append (kf_preferences_muc_bookmarks, bookmark);
}

void kf_preferences_muc_bookmark_del (KfPrefMUCBookmark *bookmark) {
	kf_preferences_muc_bookmarks = g_list_remove (kf_preferences_muc_bookmarks, bookmark);
	kf_pref_muc_bookmark_free (bookmark);
}

static void kf_preferences_load_statuses (xmlDocPtr doc, xmlNodePtr node) {
	xmlChar *name, *text;
	
	while (node) {
		if (xmlStrcmp (node->name, (const xmlChar *) "status")) {
			node = node->next;
			continue;
		}

		name = xmlGetProp (node, "name");
		if (name) {
			gchar **element;

			element = (gchar **) g_malloc (2 * sizeof (gchar *));
			text = xmlNodeListGetString (doc, node->xmlChildrenNode, 1);

			element[0] = g_strdup (name);
			element[1] = g_strdup (text);

			xmlFree (name);
			xmlFree (text);
			
			kf_preferences_statuses = g_list_append
				(kf_preferences_statuses, element);
		}
		node = node->next;
	}
}

static GSList *kf_preferences_load_list (xmlDocPtr doc, xmlNodePtr node, GSList *list) {
	xmlChar *text;
	
	while (node) {
		if (xmlStrcmp (node->name, (const xmlChar *) "item")) {
			node = node->next;
			continue;
		}

		text = xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
		list = g_slist_append (list, g_strdup (text));
		xmlFree (text);
			
		node = node->next;
	}
	return list;
}


/*
const gchar *kf_config_file (const gchar *name) {
	static gchar path[1024];

	g_snprintf (path, 1024, "%s/.kf/%s", g_get_home_dir (), name);
	return path;
}
*/
void kf_preferences_add_status (const gchar *name, const gchar *text) {
	gchar **element;

	element = (gchar **) g_malloc (2 * sizeof (gchar *));

	element[0] = g_strdup (name);
	element[1] = g_strdup (text);

	kf_preferences_statuses = g_list_append
		(kf_preferences_statuses, element);

}

KfPrefAccount *kf_pref_account_new (const gchar *name) {
	KfPrefAccount *acc;

	acc = g_new (KfPrefAccount, 1);
	acc->name= g_strdup (name);
	acc->uname = NULL;
	acc->server = NULL;
	acc->pass = NULL;
	acc->save_password = FALSE;

	acc->resource = NULL;
	acc->port = 5222;
	acc->secure = FALSE;

	acc->proxy_use = FALSE;
	acc->proxy_server = NULL;
	acc->proxy_port = 8080;
	acc->proxy_user = NULL;
	acc->proxy_pass = NULL;

	acc->autoconnect = FALSE;

	acc->use_manual_host = FALSE;
	acc->manual_host = NULL;

	return acc;
}
	
void kf_pref_account_set_proxy (KfPrefAccount *acc, gboolean use,
				const gchar *server, gint port,
				const gchar *uname, const gchar *passwd) {
	acc->proxy_use = use;
	
	g_free (acc->proxy_server);
	acc->proxy_server = g_strdup (server);
	
	acc->proxy_port = port;
	
	g_free (acc->proxy_user);
	acc->proxy_user = g_strdup (uname);
	
	g_free (acc->proxy_pass);
	acc->proxy_pass = g_strdup (passwd);
}

KfPrefAccount *kf_account_get_autoconnect (void) {
	GList *tmp;
	
	for (tmp = kf_preferences_accounts; tmp; tmp = tmp->next) {
		KfPrefAccount *acc = tmp->data;

		if (acc->autoconnect)
			return acc;

	}
	return NULL;
}

void kf_account_set_autoconnect (KfPrefAccount *xacc) {
	GList *tmp;
	
	for (tmp = kf_preferences_accounts; tmp; tmp = tmp->next) {
		KfPrefAccount *acc = tmp->data;
		
		acc->autoconnect = FALSE;
	}
	if (xacc)
		xacc->autoconnect = TRUE;
}

gboolean kf_pref_group_is_collapsed (const gchar *grp) {
	GSList *tmp;

	if (grp == NULL)
		return FALSE;

	for (tmp = kf_preferences_groups_collapsed; tmp; tmp=tmp->next) {
		gchar *name = tmp->data;

		if (strcmp (name, grp) == 0)
			return TRUE;
	}
	return FALSE;
}

void kf_pref_group_set_collapsed (const gchar *grp, gboolean set) {
	GSList *tmp;

	if (grp == NULL)
		return;
	
	for (tmp = kf_preferences_groups_collapsed; tmp; tmp=tmp->next) {
		gchar *name = tmp->data;

		if (name && strcmp (name, grp) == 0) {
			if (set) return;
			else {
				g_free (name);
				tmp->data = NULL;
			}
		}
	}
	kf_preferences_groups_collapsed = g_slist_remove_all (kf_preferences_groups_collapsed, NULL);
	if (set)
		kf_preferences_groups_collapsed = g_slist_prepend (kf_preferences_groups_collapsed,
				g_strdup (grp));
}

static void kf_preferences_create_default_colors (void) {
	kf_preferences_muc_colors = g_slist_prepend (kf_preferences_muc_colors, "brown");
	kf_preferences_muc_colors = g_slist_prepend (kf_preferences_muc_colors, "#00007f");
	kf_preferences_muc_colors = g_slist_prepend (kf_preferences_muc_colors, "orange");
	kf_preferences_muc_colors = g_slist_prepend (kf_preferences_muc_colors, "#7f00ff");
	kf_preferences_muc_colors = g_slist_prepend (kf_preferences_muc_colors, "#55aa7f");
	kf_preferences_muc_colors = g_slist_prepend (kf_preferences_muc_colors, "#55aaff");
}

/*
 * BASE64
 *
 * Following code was written by EKG team and was released
 * under GNU GPL license
 * http://dev.null/ekg/
 */

static char base64_charset[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * base64_encode()
 *
 * zapisuje ci±g znaków w base64.
 *
 *  - buf - ci±g znaków.
 *
 * zaalokowany bufor.
 */
gchar *base64_encode(const gchar *buf)
{
	gchar *out, *res;
	int i = 0, j = 0, k = 0, len = strlen(buf);
	
	res = out = g_malloc((len / 3 + 1) * 4 + 2);

	if (!res)
		return NULL;
	
	while (j <= len) {
		switch (i % 4) {
			case 0:
				k = (buf[j] & 252) >> 2;
				break;
			case 1:
				if (j < len)
					k = ((buf[j] & 3) << 4) | ((buf[j + 1] & 240) >> 4);
				else
					k = (buf[j] & 3) << 4;

				j++;
				break;
			case 2:
				if (j < len)
					k = ((buf[j] & 15) << 2) | ((buf[j + 1] & 192) >> 6);
				else
					k = (buf[j] & 15) << 2;

				j++;
				break;
			case 3:
				k = buf[j++] & 63;
				break;
		}
		*out++ = base64_charset[k];
		i++;
	}

	if (i % 4)
		for (j = 0; j < 4 - (i % 4); j++, out++)
			*out = '=';
	
	*out = 0;
	
	return res;
}

/*
 * base64_decode()
 *
 * dekoduje ci±g znaków z base64.
 *
 *  - buf - ci±g znaków.
 *
 * zaalokowany bufor.
 */
gchar *base64_decode(const gchar *buf)
{
	gchar *res, *save, *foo, val;
	const gchar *end;
	gint index = 0;

	if (!buf)
		return NULL;
	
	save = res = g_new0(gchar, (strlen(buf) / 4 + 1) * 3 + 2);

	if (!save)
		return NULL;

	end = buf + strlen(buf);

	while (*buf && buf < end) {
		if (*buf == '\r' || *buf == '\n') {
			buf++;
			continue;
		}
		if (!(foo = strchr(base64_charset, *buf)))
			foo = base64_charset;
		val = (int)(foo - base64_charset);
		buf++;
		switch (index) {
			case 0:
				*res |= val << 2;
				break;
			case 1:
				*res++ |= val >> 4;
				*res |= val << 4;
				break;
			case 2:
				*res++ |= val >> 2;
				*res |= val << 6;
				break;
			case 3:
				*res++ |= val;
				break;
		}
		index++;
		index %= 4;
	}
	*res = 0;
	
	return save;
}


