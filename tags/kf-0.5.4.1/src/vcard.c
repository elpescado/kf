/* file vcard.c */
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
#include <loudmouth/loudmouth.h>
#include <string.h>
#include "kf.h"
#include "jabber.h"
#include "gui.h"
#include "vcard.h"
#include "foogc.h"

typedef struct {
	gchar *field1;
	gchar *field2;
	gchar *widget;
} KfVCardField;

static KfVCardField vcard_fields[] = {
	{"FN", NULL, "fn"},
	{"NICKNAME", NULL, "nickname"},
	{"BDAY",NULL, "bday"},
	{"URL",NULL, "www"},
	{"EMAIL", NULL, "email"},
	{"ORG","ORGNAME","orgname"},
	{"ORG","ORGUNIT","orgunit"},
	{"TITLE",NULL,"orgpos"},
	{"ROLE",NULL,"orgrole"},
	{"TEL","VOICE","phone"},
	{"ADR","STREET","street"},
	{"ADR","EXTADD","street2"},
	{"ADR","LOCALITY","city"},
	{"ADR","REGION","state"},
	{"ADR","PCODE","pcode"},
	{"ADR","COUNTRY","country"},
	{NULL,NULL,NULL}};

static void kf_vcard_retrieve (const gchar *jid, GtkWidget *win);
static void kf_vcard_retrieve_software (const gchar *jid, GtkWidget *window);
static
LmHandlerResult kf_vcard_hendel             (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);
static
LmHandlerResult kf_vcard_software_hendel             (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);

void        on_nick_change_clicked         (GtkButton *button,
                                            gpointer user_data);
static void nick_change_cb                 (GtkButton *button,
                                            gpointer user_data);
static void rename_jid_new (KfJabberRosterItem *item, const gchar *name);
//static void rename_jid (const gchar *jid, const gchar *name);
static void vcard_display (LmMessageNode *node, GladeXML *glade);
void setwidget (GladeXML *glade, const gchar *name, const gchar *value);
static void setabout (GladeXML *glade, const gchar *value);
static gchar *getabout (GladeXML *glade);
static void setsub (GladeXML *glade, KfJabberRosterItem *item);

static void enable_edit (GladeXML *glade);
static void save_cb (GtkButton *button, gpointer data);

static
LmHandlerResult vcard_save_sent_cb          (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);

void kf_vcard_get (const gchar *jid) {
	KfJabberRosterItem *item = NULL;
	GladeXML *glade;
	gchar *jidx = g_strdup (jid);
	GtkWidget *win, *jabber_id;
	const gchar *window_title;
	gchar *title;
	FooGC *gc;

	if (jid) {
		kf_jabber_jid_crop (jidx);
		item = kf_jabber_roster_item_get (jidx);
	}
	glade = glade_xml_new (kf_find_file ("vcard.glade"), NULL, NULL);
	win = glade_xml_get_widget (glade, "vcard_window");
	window_title = gtk_window_get_title (GTK_WINDOW (win));
	glade_xml_signal_autoconnect (glade);

//	item = kf_jabber_roster_item_get (jidx);
	if (item) {
		GtkWidget *entry;
		GtkWidget *button;

		entry = glade_xml_get_widget (glade, "vcard_name");
		gtk_entry_set_text (GTK_ENTRY (entry), item->display_name);

		button = glade_xml_get_widget (glade, "nick_change");
		g_assert (button);
		g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (nick_change_cb), item);

		setsub (glade, item);

		title = g_strdup_printf (window_title, item->display_name);
	} else {
		GtkWidget *widget;
		
		widget = glade_xml_get_widget (glade, "vcard_name");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (glade, "nick_change");
		gtk_widget_set_sensitive (widget, FALSE);
		
		title = g_strdup_printf (window_title, jidx);
	}

	gtk_window_set_title (GTK_WINDOW (win), title);
	g_free (title);

	jabber_id = glade_xml_get_widget (glade, "jabber_id");
	gtk_label_set_text (GTK_LABEL (jabber_id), jidx);

	g_object_set_data_full (G_OBJECT (win), "jid", jidx, g_free);

	/* Garbage collector, for message handlers */
	gc = foo_gc_new (lm_message_handler_invalidate);
	g_object_set_data_full (G_OBJECT (win), "gc", gc, foo_gc_free);
	
	kf_vcard_retrieve (jidx, win);
	kf_vcard_retrieve_software (jid, win);
}

static void kf_vcard_retrieve (const gchar *jid, GtkWidget *window) {
	extern LmConnection *kf_jabber_connection;
	LmMessage *msg;
	LmMessageNode *node;
	LmMessageHandler *h;
	FooGC *gc = g_object_get_data (G_OBJECT (window), "gc");

	h = lm_message_handler_new (kf_vcard_hendel, window, NULL);
	msg = lm_message_new_with_sub_type (jid, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	node = lm_message_node_add_child (msg->node, "vCard", NULL);
	lm_message_node_set_attribute (node, "xmlns", "vcard-temp");
	foo_gc_add (gc, h);

	lm_connection_send_with_reply (kf_jabber_connection, msg, h, NULL);
	lm_message_unref (msg);
}

static
LmHandlerResult kf_vcard_hendel             (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data) {
	GtkWidget *win = user_data;
	GladeXML *glade = glade_get_widget_tree (win);
	GtkWidget *label = glade_xml_get_widget (glade, "vcard_text");
	LmMessageNode *node;
	FooGC *gc = g_object_get_data (G_OBJECT (win), "gc");
	if (gc)
		foo_gc_remove (gc, handler);
	else
		foo_debug ("No GC!!!");
	lm_message_handler_unref (handler);

	gtk_label_set_text (GTK_LABEL (label), lm_message_node_to_string (message->node));
	
	node = message->node->children;
	while (node) {
		if (!g_ascii_strcasecmp (node->name, "vcard")) {
			vcard_display (node->children, glade);
		}

		node = node->next;
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static void nick_change_cb                 (GtkButton *button,
                                            gpointer user_data) {
	GladeXML *glade;
	GtkWidget *entry, *window;
	const gchar *name;


	glade = glade_get_widget_tree (GTK_WIDGET (button));
	entry = glade_xml_get_widget (glade, "vcard_name");
	window = glade_xml_get_widget (glade, "vcard_window");
	name = gtk_entry_get_text (GTK_ENTRY (entry));
	//jid = g_object_get_data (G_OBJECT (window), "jid");

	rename_jid_new ((KfJabberRosterItem *) user_data, name);
}
void        on_nick_change_clicked         (GtkButton *button,
                                            gpointer user_data)
{
/*	GladeXML *glade;
	GtkWidget *entry, *window;
	const gchar *jid, *name;

	glade = glade_get_widget_tree (GTK_WIDGET (button));
	entry = glade_xml_get_widget (glade, "vcard_name");
	window = glade_xml_get_widget (glade, "vcard_window");
	name = gtk_entry_get_text (GTK_ENTRY (entry));
	jid = g_object_get_data (G_OBJECT (window), "jid");

	rename_jid (jid, name);*/
}


static void rename_jid_new (KfJabberRosterItem *item, const gchar *name) {
	LmMessage *msg;
	LmMessageNode *node;
	LmConnection *con;
	GList *tmp;

	g_return_if_fail (item);
	
	con = kf_jabber_get_connection ();

	msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");

	node = lm_message_node_add_child (node, "item", NULL);
	lm_message_node_set_attribute (node, "name", name);
	lm_message_node_set_attribute (node, "jid", item->jid);

	for (tmp = item->groups; tmp; tmp=tmp->next) {
		const gchar *grp = tmp->data;

		lm_message_node_add_child (node, "group", grp);
	}

	kf_connection_send (con, msg, NULL);
	
	//g_print ("MSG:\n%s\n", lm_message_node_to_string (msg->node));

	lm_message_unref (msg);
}


/*
static void rename_jid (const gchar *jid, const gchar *name) {
	LmMessage *msg;
	LmMessageNode *node;

	msg = lm_message_new_with_sub_type (jid, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");

	node = lm_message_node_add_child (node, "item", NULL);
	lm_message_node_set_attribute (node, "name", name);
	lm_message_node_set_attribute (node, "jid", jid);

	kf_connection_send (NULL, msg, NULL);

	lm_message_unref (msg);
}*/

static void vcard_display (LmMessageNode *node, GladeXML *glade) {

	while (node) {
		KfVCardField *field;
		if (!g_ascii_strcasecmp (node->name, "desc")) {
			setabout (glade, node->value);
			node = node->next;
			continue;
		}
		for (field = vcard_fields; field->field1; field++) {
			if (!g_ascii_strcasecmp (node->name, field->field1)) {
				if (field->field2 == NULL) {
					setwidget (glade, field->widget, node->value);
					break;
				} else {
					LmMessageNode *child = node->children;

					while (child) {
						if (!g_ascii_strcasecmp (child->name, field->field2)) {
							setwidget (glade, field->widget, child->value);
							break;
						}
						child = child->next;
					}
				}
			}
		}
		
		node = node->next;
	}
	
}

void setwidget (GladeXML *glade, const gchar *name, const gchar *value) {
	GtkWidget *widget;

	widget = glade_xml_get_widget (glade, name);
	if (widget)
		gtk_entry_set_text (GTK_ENTRY (widget), value?value:"");
	else 
		g_print ("Cannot find widget %s\n", name);
}

static void setabout (GladeXML *glade, const gchar *value) {
	GtkWidget *tv;
	GtkTextBuffer *buffer;

	tv = glade_xml_get_widget (glade, "about");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tv));
	gtk_text_buffer_set_text (buffer, value, -1);
}

static void setsub (GladeXML *glade, KfJabberRosterItem *item) {
	GtkWidget *label;
	static gchar *subs[] = {
		N_("<b>none</b> - None of you can see each other's presence"),
		N_("<b>from</b> - They can see your presence, but you can't see theirs"),
		N_("<b>to</b> - You can see their presence, but they can't see yours"),
		N_("<b>both</b> - Both of you can see each other's presence"),
		N_("<b>unknown</b> - Presence is unknown")};
	
		g_return_if_fail (item);
		label = glade_xml_get_widget (glade, "subscription_text");
		gtk_label_set_markup (GTK_LABEL (label), _(subs[item->subscription]));
}

/* Retrieve one's own vcard */
void kf_vcard_my (void) {
	GladeXML *glade;
	GtkWidget *win, *page;
	
	glade = glade_xml_new (kf_find_file ("vcard.glade"), NULL, NULL);
	win = glade_xml_get_widget (glade, "vcard_window");
	glade_xml_signal_autoconnect (glade);

	gtk_window_set_title (GTK_WINDOW (win), _("My vCard"));
	
	page = glade_xml_get_widget (glade, "table1");
	gtk_widget_hide (page);

	enable_edit (glade);

	kf_vcard_retrieve (NULL, win);
}

/* Make all entries editable */
static void enable_edit (GladeXML *glade) {
	gint i;
	GtkWidget *textview, *save;
	
	for (i = 0; vcard_fields[i].field1; i++) {
		GtkWidget *widget;

		widget = glade_xml_get_widget (glade, vcard_fields[i].widget);
		gtk_editable_set_editable (GTK_EDITABLE (widget), TRUE);
	}

	textview = glade_xml_get_widget (glade, "about");
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), TRUE);
	save = glade_xml_get_widget (glade, "save_button");
	gtk_widget_show (save);
	
	g_signal_connect (G_OBJECT (save), "clicked", G_CALLBACK (save_cb), NULL);
}

/* Save your vCard */
static void save_cb (GtkButton *button, gpointer data) {
	gint i;
	GladeXML *glade = glade_get_widget_tree (GTK_WIDGET (button));
	LmMessage *msg;
	LmMessageNode *vcard;
	LmMessageHandler *h;
	extern LmConnection *kf_jabber_connection;
	gchar *about;

	msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	vcard = lm_message_node_add_child (msg->node, "vCard", NULL);
	lm_message_node_set_attribute (vcard, "xmlns", "vcard-temp");
	
	for (i = 0; vcard_fields[i].field1; i++) {
		GtkWidget *widget;
		const gchar *text;

		widget = glade_xml_get_widget (glade, vcard_fields[i].widget);
		text = gtk_entry_get_text (GTK_ENTRY (widget));
		if (vcard_fields[i].field2) {
			LmMessageNode *parent;
			
		//	g_print ("%s->%s: %s\n", vcard_fields[i].field1, vcard_fields[i].field2, text);
			parent = lm_message_node_get_child (vcard, vcard_fields[i].field1);
			if (! parent) {
				parent = lm_message_node_add_child (vcard, vcard_fields[i].field1, NULL);
			}
			lm_message_node_add_child (parent, vcard_fields[i].field2, text);
		} else {
			LmMessageNode *node;
		//	g_print ("%s: %s\n", vcard_fields[i].field1, text);
			node = lm_message_node_add_child (vcard, vcard_fields[i].field1, text);
		}
	}
	about = getabout (glade);
	lm_message_node_add_child (vcard, "DESC", about);
	g_free (about);


//	g_print ("Message: %s\n", lm_message_node_to_string (msg->node));
	h = lm_message_handler_new (vcard_save_sent_cb, NULL, NULL);
	//kf_connection_send (NULL, msg, NULL);
	lm_connection_send_with_reply (kf_jabber_connection, msg, h, NULL);
}

static gchar *getabout (GladeXML *glade) {
	GtkWidget *textview;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	
	textview = glade_xml_get_widget (glade, "about");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static
LmHandlerResult vcard_save_sent_cb          (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data) {

	LmMessageNode *node;

	node = lm_message_node_find_child (message->node, "error");
	if (node) {
		kf_gui_alert (_("There was an error!"));
	} else {
		kf_gui_alert (_("vCard updated successfully!"));
	}
	
	lm_message_handler_unref (handler);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}


/* Computer info */

typedef struct {
	GtkWidget *page;	/* Notebook page with software info */
	GtkWidget *client_name;	/* GtkLabels */
	GtkWidget *client_version;
	GtkWidget *operating_system;
} KfVCardComputer;

static void kf_vcard_retrieve_software (const gchar *jid, GtkWidget *window) {
	extern LmConnection *kf_jabber_connection;
	LmMessage *msg;
	LmMessageNode *node;
	LmMessageHandler *h;
	GladeXML *glade = glade_get_widget_tree (window);
	KfVCardComputer *comp;
	FooGC *gc = g_object_get_data (G_OBJECT (window), "gc");

	comp = g_new0(KfVCardComputer, 1);
	kf_glade_get_widgets (glade,
			"computer_page", &comp->page,
			"client_name", &comp->client_name,
			"client_version", &comp->client_version,
			"operating_system", &comp->operating_system,
			NULL);

	h = lm_message_handler_new (kf_vcard_software_hendel, comp, g_free);
	msg = lm_message_new_with_sub_type (jid, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	node = lm_message_node_add_child (msg->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:version");

	foo_gc_add (gc, h);
	lm_connection_send_with_reply (kf_jabber_connection, msg, h, NULL);
	lm_message_unref (msg);
}

static
LmHandlerResult kf_vcard_software_hendel             (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data) {
	LmMessageNode *node;
	KfVCardComputer *comp = user_data;
	gint items = 0;
	FooGC *gc = g_object_get_data (G_OBJECT (gtk_widget_get_toplevel (comp->page)), "gc");

	foo_gc_remove (gc, handler);
	lm_message_handler_unref (handler);
	foo_debug ("[jabber:iq:version] Got response\n");

	node = message->node->children;
	while (node) {
		if (strcmp (node->name, "query") == 0) {
			const gchar *xmlns = lm_message_node_get_attribute (node, "xmlns");
			
			if (strcmp (xmlns, "jabber:iq:version") == 0) {
				LmMessageNode *child;

				for (child = node->children; child; child = child->next) {
					const gchar *value = lm_message_node_get_value (child);
					if (strcmp (child->name, "name") == 0) {
						gtk_label_set_text (GTK_LABEL (comp->client_name), value);
						items++;
					} else if (strcmp (child->name, "version") == 0) {
						gtk_label_set_text (GTK_LABEL (comp->client_version), value);
						items++;
					} else if (strcmp (child->name, "os") == 0) {
						gtk_label_set_text (GTK_LABEL (comp->operating_system), value);
						items++;
					}					
				}
			}
		}

		node = node->next;
	}

	if (items > 0) {
		gtk_widget_show (comp->page);
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

