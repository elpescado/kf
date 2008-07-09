/* file privacy_list.c */
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
#include <stdlib.h>
#include <stdio.h>
#include "kf.h"
#include "privacy_list.h"

typedef enum {
	KF_RULE_TYPE_JID,
	KF_RULE_TYPE_GROUP,
	KF_RULE_TYPE_SUBSCRIPTION
} KfPrivacyRuleType;

typedef struct {
	gchar *name;
	gchar *value;
	gint order;
	gint type	: 3;
	gint action	: 1;	/* Deny = 0, Allow = 1 */
	gint message	: 1;
	gint presence_in: 1;
	gint presence_out: 1;
	gint iq		: 1;
} KfPrivacyRule;


typedef struct {
	GtkWidget *window;
	GtkWidget *rules;
	GtkListStore *store;
	GtkWidget *name;
	GtkWidget *add;
	GtkWidget *del;
	GtkWidget *up;
	GtkWidget *down;
	GtkWidget *properties;
	GtkWidget *jid;
	GtkWidget *group;
	GtkWidget *subscription;
	GtkWidget *type[4];
	GtkWidget *action[4];
	GtkWidget *allow;
	GtkWidget *deny;
	GtkWidget *ok;
	GtkWidget *cancel;

	gchar *list_name;
	gboolean waiting;
	KfPrivacyRule *rule;
} KfPrivacyListEditor;


static KfPrivacyListEditor *kf_privacy_list_editor_new (const gchar *name);
static void kf_privacy_list_editor_free (KfPrivacyListEditor *self);
static void kf_privacy_list_editor_set_waiting (KfPrivacyListEditor *self, gboolean value);
static KfPrivacyRule *kf_privacy_rule_new (void);
static void kf_privacy_rule_free (KfPrivacyRule *self);
static void kf_privacy_rule_update (KfPrivacyRule *self);
static void selection_changed (GtkTreeSelection *selection, gpointer data);
static void kf_privacy_list_editor_display_rule (KfPrivacyListEditor *self, KfPrivacyRule *rule);
static void kf_privacy_list_editor_fetch_list (KfPrivacyListEditor *self);
static LmHandlerResult kf_privacy_list_editor_fetch_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data);
static gint compare_function (gconstpointer a, gconstpointer b);
static void kf_privacy_list_editor_submit (KfPrivacyListEditor *self);
static LmHandlerResult kf_privacy_list_editor_submit_cb  (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);

static void on_add_clicked (GtkButton *button, gpointer data);
static void on_del_clicked (GtkButton *button, gpointer data);
static void on_up_clicked (GtkButton *button, gpointer data);
static void on_down_clicked (GtkButton *button, gpointer data);
static void on_ok_clicked (GtkButton *button, gpointer data);
static void on_cancel_clicked (GtkButton *button, gpointer data);
static void on_type_toggled (GtkToggleButton *toggle, gpointer data);
static void on_action_toggled (GtkToggleButton *toggle, gpointer data);
static void on_allow_toggled (GtkToggleButton *toggle, gpointer data);
static gboolean update_entry               (GtkWidget *widget,
                                            GdkEventFocus *event,
                                            gpointer user_data);
static void on_subscription_type_changed   (GtkOptionMenu *optionmenu,
                                            gpointer user_data);

void kf_privacy_edit_list (const gchar *name) {
	kf_privacy_list_editor_new (name);
}

static KfPrivacyListEditor *kf_privacy_list_editor_new (const gchar *name) {
	KfPrivacyListEditor *self = g_new0 (KfPrivacyListEditor, 1);
	GladeXML *glade = glade_xml_new (kf_find_file ("privacy_list.glade"), NULL, NULL);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	gint i;
	
	if (name)
		self->list_name = g_strdup (name);
	
	kf_glade_get_widgets (glade,
			"window", &self->window,
			"rules", &self->rules,
			"name", &self->name,
			"add", &self->add,
			"del", &self->del,
			"up", &self->up,
			"down", &self->down,
			"properties", &self->properties,
			"jid", &self->jid,
			"group", &self->group,
			"subscription", &self->subscription,
			"type_jid", &self->type[0],
			"type_group", &self->type[1],
			"type_subscription", &self->type[2],
			"type_other", &self->type[3],
			"messages", &self->action[0],
			"presence-in", &self->action[1],
			"presence-out", &self->action[2],
			"iq", &self->action[3],
			"action_allow", &self->allow,
			"action_deny", &self->deny,
			"ok", &self->ok,
			"cancel", &self->cancel,
			NULL);
	g_object_unref (G_OBJECT (glade));

	if (name) {
		gtk_entry_set_text (GTK_ENTRY (self->name), name);
		gtk_entry_set_editable (GTK_ENTRY (self->name), FALSE);
	}

	self->store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_view_set_model (GTK_TREE_VIEW (self->rules), GTK_TREE_MODEL (self->store));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Rules",
                                                   renderer,
                                                   "text", 0,
                                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->rules), column);
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->rules));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
        	G_CALLBACK (selection_changed),
        	self);

	/* Connect signals */
	g_signal_connect (G_OBJECT (self->add), "clicked",
			G_CALLBACK (on_add_clicked), self);
	g_signal_connect (G_OBJECT (self->del), "clicked",
			G_CALLBACK (on_del_clicked), self);
	g_signal_connect (G_OBJECT (self->up), "clicked",
			G_CALLBACK (on_up_clicked), self);
	g_signal_connect (G_OBJECT (self->down), "clicked",
			G_CALLBACK (on_down_clicked), self);
	for (i = 0; i < 4; i++) {
		g_signal_connect (G_OBJECT (self->action[i]), "toggled",
			G_CALLBACK (on_action_toggled), self);
	}
	for (i = 0; i < 4; i++) {
		g_signal_connect (G_OBJECT (self->type[i]), "toggled",
			G_CALLBACK (on_type_toggled), self);
	}
	g_signal_connect (G_OBJECT (self->allow), "toggled",
			G_CALLBACK (on_allow_toggled), self);
	
	g_signal_connect (G_OBJECT (self->ok), "clicked",
			G_CALLBACK (on_ok_clicked), self);
	g_signal_connect (G_OBJECT (self->cancel), "clicked",
			G_CALLBACK (on_cancel_clicked), self);
	g_signal_connect (G_OBJECT (self->jid), "focus-out-event",
			G_CALLBACK (update_entry), self);
	g_signal_connect (G_OBJECT (self->group), "focus-out-event",
			G_CALLBACK (update_entry), self);
	g_signal_connect (G_OBJECT (self->subscription), "changed",
			G_CALLBACK (on_subscription_type_changed), self);

	if (name)
		kf_privacy_list_editor_fetch_list (self);
	g_object_set_data_full (G_OBJECT (self->window), "self", self,
			(GDestroyNotify) kf_privacy_list_editor_free);
	return self;
}
	
static void kf_privacy_list_editor_free (KfPrivacyListEditor *self) {
	GtkTreeIter iter;
	
	g_free (self->list_name);
	
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->store), &iter)) {
		do {
			KfPrivacyRule *rule;
	        	gtk_tree_model_get (GTK_TREE_MODEL (self->store), &iter, 1, &rule, -1);
	        	gtk_list_store_set (self->store, &iter, 1, NULL, -1);
			kf_privacy_rule_free (rule);
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->store), &iter));
	}
	
	g_free (self);
}


static void kf_privacy_list_editor_set_waiting (KfPrivacyListEditor *self, gboolean value) {
	self->waiting = value;
	gtk_widget_set_sensitive (self->window, ! value);
}

static KfPrivacyRule *kf_privacy_rule_new (void) {
	KfPrivacyRule *self = g_new0 (KfPrivacyRule, 1);
	return self;
}

static void kf_privacy_rule_free (KfPrivacyRule *self) {
	g_free (self->name);
	if (self->type != 2)
		g_free (self->value);
	g_free (self);
}

/*
 * Updates rule's title
 */
static void kf_privacy_rule_update (KfPrivacyRule *self) {
	gchar packets[4096];
	gint i = 0;
	static gchar *types[] = {
		N_("JabberID is"),
		N_("group"),
		N_("subscription"),
		N_("everything fails;-)")};
	/* TODO: duplicate */
	static gchar *subs[] = {"both", "to", "from", "none"};
		
	
	g_free (self->name);
//	if (self->type == 0) {
//		/* JID-based rule */
///		self->name = g_strdup_printf ("JID-based rule (%d)", self->order);
//	} else if (self->type == 1) {
//		self->name = g_strdup_printf ("Group-based rule (%d)", self->order);
//	} else if (self->type == 2) {
//		self->name = g_strdup_printf ("Subscription based rule (%d)", self->order);
//	} else {
//		self->name = g_strdup_printf ("Fall-back rule (%d)", self->order);
//	}
	*packets = '\0';
	if (self->message) {
		strcat (packets, _("messages"));
		i++;
	}
	if (self->presence_in || self->presence_out) {
		if (i)
			strcat (packets, ", ");
		strcat (packets, _("status messages"));
		i++;
	}
	if (self->iq) {
		if (i)
			strcat (packets, ", ");
		strcat (packets, _("IQs"));
		i++;
	}
	if (i == 0) {
		strcpy (packets, _("all traffic"));
	}

	self->name = g_strdup_printf ("%s %s if %s%s%s", self->action?_("Allow"):_("Deny"),
		packets,
		_(types[self->type]),
		(self->type<3)?" is ":"",
		(self->type<2)?self->value:(self->type==2)?subs[GPOINTER_TO_INT (self->value)]:""
		);
}
	
static void
selection_changed (GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;
	KfPrivacyListEditor *self = data;

	foo_debug ("Selection changed... ");
//	g_free (self->selected);
        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
		KfPrivacyRule *rule;
        	gtk_tree_model_get (model, &iter, 1, &rule, -1);
		if (rule) {
			foo_debug ("Got '%s'\n", rule->name);
			self->rule = rule;
			kf_privacy_list_editor_display_rule (self, rule);
			gtk_widget_set_sensitive (self->properties, TRUE);
		}
	} else {
		self->rule = NULL;
		foo_debug ("Got nothing:-(");
		gtk_widget_set_sensitive (self->properties, FALSE);
	}
/*	self->selected = name;

	gtk_widget_set_sensitive (self->set_default, TRUE);
	gtk_widget_set_sensitive (self->del, name != NULL);
	gtk_widget_set_sensitive (self->modify, name != NULL);
	gtk_widget_set_sensitive (self->set_active, name != NULL);*/
}


static void kf_privacy_list_editor_display_rule (KfPrivacyListEditor *self, KfPrivacyRule *rule) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->type[rule->type]), TRUE);
	switch (rule->type) {
		case 0:	/* JID */
			gtk_entry_set_text (GTK_ENTRY (self->jid), rule->value);
			gtk_entry_set_text (GTK_ENTRY (self->group), "");
			gtk_option_menu_set_history (GTK_OPTION_MENU (self->subscription),
					0);
			break;
		case 1: /* Group */
			gtk_entry_set_text (GTK_ENTRY (self->group), rule->value);
			gtk_entry_set_text (GTK_ENTRY (self->jid), "");
			gtk_option_menu_set_history (GTK_OPTION_MENU (self->subscription),
					0);
			break;
		case 2: /* Subscription */
			gtk_option_menu_set_history (GTK_OPTION_MENU (self->subscription),
					GPOINTER_TO_INT (rule->value));
			gtk_entry_set_text (GTK_ENTRY (self->group), "");
			gtk_entry_set_text (GTK_ENTRY (self->jid), "");
			break;
	}

	foo_debug ("rule->action=%d\n", rule->action);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->allow), rule->action);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->deny), ! rule->action);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->action[0]), rule->message);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->action[1]), rule->presence_in);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->action[2]), rule->presence_out);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->action[3]), rule->iq);
}


static void kf_privacy_list_editor_fetch_list (KfPrivacyListEditor *self) {
	LmMessage *msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ,
			LM_MESSAGE_SUB_TYPE_GET);
	LmMessageNode *query = lm_message_node_add_child (msg->node, "query", NULL);
	LmMessageNode *node = lm_message_node_add_child (query, "list", NULL);
	LmMessageHandler *h = lm_message_handler_new (kf_privacy_list_editor_fetch_cb,self, NULL);
	lm_message_node_set_attribute (query, "xmlns", "jabber:iq:privacy");
	lm_message_node_set_attribute (node, "name", self->list_name);


	lm_connection_send_with_reply (kf_jabber_get_connection (), msg, h, NULL);
	lm_message_unref (msg);
//	self->waiting = TRUE;
	kf_privacy_list_editor_set_waiting (self, TRUE);
}


static LmHandlerResult kf_privacy_list_editor_fetch_cb  (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data) {
	LmMessageNode *n;
	KfPrivacyListEditor *self = user_data;
	GList *items = NULL;

	foo_debug ("Got response");
	
	for (n = message->node->children; n; n = n->next) {
		const gchar *xmlns = lm_message_node_get_attribute (n, "xmlns");
		/* Find <query xmlns="jabber:iq:privacy"> */
		if (xmlns && strcmp (n->name, "query") == 0 && strcmp (xmlns, "jabber:iq:privacy") == 0) {
			LmMessageNode *n2;

			for (n2 = n->children; n2; n2 = n2->next) {
				/* Find <list name=""> */
				if (strcmp (n2->name, "list") == 0) {
					LmMessageNode *node;
					for (node = n2->children; node; node = node->next) {
						/* Get list rules */
						if (strcmp (node->name, "item") == 0) {
							const gchar *type = lm_message_node_get_attribute
								(node, "type");
							const gchar *value = lm_message_node_get_attribute
								(node, "value");
							const gchar *action = lm_message_node_get_attribute
								(node, "action");
							const gchar *order = lm_message_node_get_attribute
								(node, "order");

							KfPrivacyRule *rule = kf_privacy_rule_new ();
							LmMessageNode *child;
							/* Get rule type */
							if (type == NULL) {
								rule->type = 3;
							} else if (strcmp (type, "jid") == 0) {
								rule->type = 0;
								rule->value = g_strdup (value);
							} else if (strcmp (type, "group") == 0) {
								rule->type = 1;
								rule->value = g_strdup (value);
							} else if (strcmp (type, "subscription") == 0) {
								rule->type = 2;
								
								if (strcmp (value, "both") == 0) {
									rule->value = GINT_TO_POINTER (0);
								} else if (strcmp (value, "to") == 0) {
									rule->value = GINT_TO_POINTER (1);
								} else if (strcmp (value, "from") == 0) {
									rule->value = GINT_TO_POINTER (2);
								} else if (strcmp (value, "none") == 0) {
									rule->value = GINT_TO_POINTER (3);
								} else {
									/* IMPOSSIBLE */
								}
							} else {
								/* IMPOSSIBLE */
							} /* if (rule type) */
							rule->order = atoi (order);
							rule->action = (strcmp (action, "allow") == 0);

							for (child = node->children; child; child = child->next) {
								const gchar *value = child->name;

								if (strcmp (value, "message") == 0) {
									rule->message = 1;
								} else if (strcmp (value, "presence-in") == 0) {
									rule->presence_in = 1;
								} else if (strcmp (value, "presence-out") == 0) {
									rule->presence_out = 1;
								} else if (strcmp (value, "iq") == 0) {
									rule->iq = 1;
								}
							}

							kf_privacy_rule_update (rule);

							items = g_list_prepend (items, rule);
						} /* if (node == "item") */
					} /* for (iterate over <list> children) */
				} /* if (node == "list") */
			} /* for (iterate over <query> children) */
		} /* if (node = <query xmlns="jabber:iq:privacy">) */
	} /* for (iterate over <iq> children) */

	if (items) {
		GList *tmp;
		items = g_list_sort (items, compare_function);

		for (tmp = items; tmp; tmp = tmp->next) {
			GtkTreeIter iter;
			KfPrivacyRule *rule = tmp->data;

			gtk_list_store_append (self->store, &iter);
			gtk_list_store_set (self->store, &iter, 0, rule->name, 1, rule, -1);
		}
		g_list_free (items);
	}
	kf_privacy_list_editor_set_waiting (self, FALSE);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static gint compare_function                (gconstpointer a,
                                             gconstpointer b) {
/*	const KfPrivacyRule *aa;
	const KfPrivacyRule *bb;
	aa = *a;
	bb = *b;
	if (aa && bb)
		return aa->order - bb->order;
	else*/ return 0;
}


static void kf_privacy_list_editor_submit (KfPrivacyListEditor *self) {
	GtkTreeIter iter;
	gint index = 1;
	LmMessage *msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ,
			LM_MESSAGE_SUB_TYPE_SET);
	LmMessageNode *query = lm_message_node_add_child (msg->node, "query", NULL);
	LmMessageNode *node = lm_message_node_add_child (query, "list", NULL);
	LmMessageHandler *h = lm_message_handler_new (kf_privacy_list_editor_submit_cb, self, NULL);
	lm_message_node_set_attribute (query, "xmlns", "jabber:iq:privacy");
	lm_message_node_set_attribute (node, "name", self->list_name);

	/* Get Rules */
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->store), &iter)) {
		do {
			KfPrivacyRule *rule;
			LmMessageNode *item;
			static gchar *types[] = {"jid", "group", "subscription"};
			static gchar *subs[] = {"both", "to", "from", "none"};
			gchar index_str[8];	/* Should be enough... */
			
	        	gtk_tree_model_get (GTK_TREE_MODEL (self->store), &iter, 1, &rule, -1);
			item = lm_message_node_add_child (node, "item", NULL);

			snprintf (index_str, 8, "%d", index++);
			lm_message_node_set_attribute (item, "order", index_str);
			
			if (rule->type < 3)
				lm_message_node_set_attribute (item, "type", types[rule->type]);
			
			if (rule->type == 0 /* JID */ || rule->type == 1 /* Group */)
				lm_message_node_set_attribute (item, "value", rule->value);
			else if (rule->type == 2 /* Subscription */)
				lm_message_node_set_attribute (item, "value", subs[GPOINTER_TO_INT (rule->value)]);

			lm_message_node_set_attribute (item, "action", rule->action?"allow":"deny");
			
			if (rule->message)
				lm_message_node_add_child (item, "message", NULL);
			if (rule->presence_in)
				lm_message_node_add_child (item, "presence-in", NULL);
			if (rule->presence_out)
				lm_message_node_add_child (item, "presence-out", NULL);
			if (rule->iq)
				lm_message_node_add_child (item, "iq", NULL);

		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->store), &iter));
	}



	lm_connection_send_with_reply (kf_jabber_get_connection (), msg, h, NULL);
	lm_message_unref (msg);
//	self->waiting = TRUE;
	kf_privacy_list_editor_set_waiting (self, TRUE);

}

static LmHandlerResult kf_privacy_list_editor_submit_cb  (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer data) {
	KfPrivacyListEditor *self = data;
	kf_privacy_list_editor_set_waiting (self, FALSE);
	if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_RESULT)
		gtk_widget_destroy (self->window);
	else 
		kf_gui_alert ("Error");
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}
/* Event handlers */

static void on_add_clicked (GtkButton *button, gpointer data)
{
	KfPrivacyListEditor *self = data;
	KfPrivacyRule *rule = kf_privacy_rule_new ();
	GtkTreeIter iter;

	gtk_list_store_prepend (self->store, &iter);
	gtk_list_store_set (self->store, &iter, 0, rule->name, 1, rule, -1);
}


static void on_del_clicked (GtkButton *button, gpointer data)
{
	KfPrivacyListEditor *self = data;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->rules));

	if (selection) {
		GtkTreeIter iter;
		GtkTreeModel *model;
		
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			KfPrivacyRule *rule;
	        	gtk_tree_model_get (model, &iter, 1, &rule, -1);

			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			kf_privacy_rule_free (rule);
		}
	}
}


static void on_up_clicked (GtkButton *button, gpointer data)
{
#if GTK_CHECK_VERSION(2,2,0)
	KfPrivacyListEditor *self = data;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->rules));

	if (selection) {
		GtkTreeIter iter;
		GtkTreeModel *model;
		
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
			GtkTreeIter moveto;

			if (gtk_tree_path_prev (path) && gtk_tree_model_get_iter (model, &moveto, path)) {
				gtk_list_store_swap (GTK_LIST_STORE (model),
							&iter, &moveto);
			}
			gtk_tree_path_free (path);
		}
	}	
#endif
}


static void on_down_clicked (GtkButton *button, gpointer data)
{
#if GTK_CHECK_VERSION(2,2,0)
	KfPrivacyListEditor *self = data;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->rules));

	if (selection) {
		GtkTreeIter iter;
		GtkTreeModel *model;
		
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			GtkTreeIter moveto;

			moveto = iter;
			if (gtk_tree_model_iter_next (model, &moveto)) {
				gtk_list_store_swap (GTK_LIST_STORE (model),
						&iter, &moveto);
			}
		}
	}	
#endif
}


static void on_ok_clicked (GtkButton *button, gpointer data)
{
	KfPrivacyListEditor *self = data;
	if (self->list_name == NULL)
		self->list_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->name)));
	kf_privacy_list_editor_submit (self);
}


static void on_cancel_clicked (GtkButton *button, gpointer data)
{
	KfPrivacyListEditor *self = data;
	if (! self->waiting) {
		gtk_widget_destroy (self->window);
	}
}


static void on_type_toggled (GtkToggleButton *toggle, gpointer data)
{
	KfPrivacyListEditor *self = data;
	gint found = -1;	/* Value */
	gint i;

	if (! self->rule)
		return;
	
	if (! gtk_toggle_button_get_active (toggle))
		return;

	/* Find out which button was toggled */
	for (i = 0; i < 4; i++) {
		if ((gpointer) toggle == (gpointer) self->type[i]) {
			found = i;
			break;
		}
	}
	foo_debug ("Found=%d\n", found);
	gtk_widget_set_sensitive (self->jid, found == 0);
	gtk_widget_set_sensitive (self->group, found == 1);
	gtk_widget_set_sensitive (self->subscription, found == 2);

	if (self->rule->type != KF_RULE_TYPE_SUBSCRIPTION && found == KF_RULE_TYPE_SUBSCRIPTION) {
		g_free (self->rule->value);
		self->rule->value = NULL;
	}

	if (i >= 0) {
		self->rule->type = found;
	}
}


static void on_action_toggled (GtkToggleButton *toggle, gpointer data)
{
	KfPrivacyListEditor *self = data;
	gint found = -1;	/* Value */
	gint i;
	gint state;		/* Button state */

	if (! self->rule)
		return;
	
	state = gtk_toggle_button_get_active (toggle);

	/* Find out which button was toggled */
	for (i = 0; i < 4; i++) {
		if ((gpointer) toggle == (gpointer) self->action[i]) {
			found = i;
			break;
		}
	}
	foo_debug ("Found=%d\n", found);

	/* Could be done in a better way, perhaps */
	switch (found) {
		case 0:
			self->rule->message = state;
			break;
		case 1:
			self->rule->presence_in = state;
			break;
		case 2:
			self->rule->presence_out = state;
			break;
		case 3:
			self->rule->iq = state;
			break;
	}
}


static void on_allow_toggled (GtkToggleButton *toggle, gpointer data)
{
	KfPrivacyListEditor *self = data;

	if (! self->rule)
		return;

	self->rule->action = gtk_toggle_button_get_active (toggle);
}


static gboolean update_entry               (GtkWidget *widget,
                                            GdkEventFocus *event,
                                            gpointer data)
{
	KfPrivacyListEditor *self = data;
	const gchar *value;
	
	if (self->rule == NULL)
		return FALSE;

	value = gtk_entry_get_text (GTK_ENTRY (widget));

	if (widget == self->jid) {
		/* Check if rule type is JID */
		if (self->rule->type != 0)
			return FALSE;

		g_free (self->rule->value);
		self->rule->value = g_strdup (value);
	} else if (widget == self->group) {
		/* Check if rule type is group */
		if (self->rule->type != 1)
			return FALSE;

		g_free (self->rule->value);
		self->rule->value = g_strdup (value);
	}

	foo_debug ("focus-out-event\n");
	return FALSE;
}


static void on_subscription_type_changed   (GtkOptionMenu *optionmenu,
                                            gpointer data)
{
	KfPrivacyListEditor *self = data;

	if (self->rule == NULL)
		return;

	if (self->rule->type != 2)
		return;

	self->rule->value = GINT_TO_POINTER (gtk_option_menu_get_history (optionmenu));
}
