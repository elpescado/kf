/* file x_data.c */
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
#include "x_data.h"
/*
typedef enum {
	KF_X_DATA_TEXT_SINGLE,
	KF_X_DATA_TEXT_PRIVATE,
	KF_X_DATA_TEXT_MULTI,
	KF_X_DATA_LIST_SINGLE,
	KF_X_DATA_LIST_MULTI,
	KF_X_DATA_BOOLEAN,
	KF_X_DATA_FIXED,
	KF_X_DATA_HIDDEN,
	KF_X_DATA_JID_SINGLE,
	KF_X_DATA_JID_MULTI
} KfXDataType;
*/
KfXDataType kf_x_data_type_from_string (const gchar *str);
static GtkWidget *kf_x_data_create_list 
	(GtkWidget *table, const gchar *title,
	 KfXDataType type, LmMessageNode *node, guint field_id, GtkTooltips *tips);
static GtkWidget *kf_x_data_create_boolean 
	(GtkWidget *table, const gchar *title,
	 KfXDataType type, LmMessageNode *node, guint field_id);
static const gchar *get_value (KfXDataType type, GtkWidget *widget);
static const gchar *get_value_list_single (GtkWidget *widget);
static GtkWidget *kf_x_data_label_new (const gchar *text);

GtkWidget *kf_x_data_form (LmMessageNode *node) {
	GtkWidget *table;
	guint field_id = 2;
	GSList *list = NULL;
	GtkTooltips *tips;
//	GtkWidget *desc;
//	GtkWidget *field;
	GdkColor white = {0xffff, 0xffff, 0xffff, 0xffff};

	GtkWidget *eventbox;

	eventbox = gtk_event_box_new ();
	table = gtk_table_new (2, 2, FALSE);
	gtk_widget_modify_bg (eventbox, GTK_STATE_NORMAL, &white);
	gtk_container_add (GTK_CONTAINER (eventbox), table);
	gtk_widget_show (table);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);

	tips = gtk_tooltips_new ();

	foo_debug ("X:data:\n%s\n", lm_message_node_to_string (node->parent));
	while (node) {
//		KfXDataType type;

//		type = kf_x_data_type_from_string (node->name);

		if (!strcmp (node->name, "title")) {
			GtkWidget *label;
			PangoFontDescription *desc;

			label = gtk_label_new (lm_message_node_get_value (node));
			desc = pango_font_description_from_string ("bold");
			gtk_widget_modify_font (label, desc);
			gtk_widget_show (label);
			gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
			gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1, 0, 0, 0, 0);
			//gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 2, 0, 1);
		} else if (!strcmp (node->name, "instructions")) {
			GtkWidget *label;
			PangoFontDescription *desc;

			label = gtk_label_new (lm_message_node_get_value (node));
			desc = pango_font_description_from_string ("italic");
			gtk_widget_modify_font (label, desc);
			gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
			//gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 2, 1, 2);
			gtk_table_attach (GTK_TABLE (table), label, 0, 2, 1, 2, 0,0,0,0);
			gtk_widget_show (label);
		} else if (!strcmp (node->name, "field")) {
			LmMessageNode *value_node;
			const gchar *title; /* label */
			const gchar *type_str;
			const gchar *var;
			const gchar *value;
			const gchar *desc;
			KfXDataType type;
			GtkWidget *x = NULL;


			title = lm_message_node_get_attribute (node, "label");
			type_str = lm_message_node_get_attribute (node, "type");
			var = lm_message_node_get_attribute (node, "var");
			value_node = lm_message_node_get_child (node, "value");
			value = (value_node)?lm_message_node_get_value (value_node):NULL;
			value_node = lm_message_node_get_child (node, "description");
			desc = (value_node)?lm_message_node_get_value (value_node):NULL;
			
			type = kf_x_data_type_from_string (type_str);
			
			if (type == KF_X_DATA_FIXED && value) {
				/* Po prostu label */
				GtkWidget *label;
				PangoFontDescription *desc;
				
				label = gtk_label_new (value);
				desc = pango_font_description_from_string ("bold");
				gtk_widget_modify_font (label, desc);
			 	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
//				gtk_table_resize (GTK_TABLE (table), 2, field_id+1);
				gtk_table_attach (GTK_TABLE (table), label,
						0, 2, field_id, field_id+1, GTK_FILL, 0, 0, 0);
				field_id++;
				gtk_widget_show (label);
			} else if (	   type == KF_X_DATA_TEXT_SINGLE
					|| type == KF_X_DATA_TEXT_PRIVATE
					|| type == KF_X_DATA_JID_SINGLE
					|| type == KF_X_DATA_JID_MULTI) {
				GtkWidget *label;
				GtkWidget *entry;
				
				label = kf_x_data_label_new (title);
			 	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
				entry = gtk_entry_new ();
				if (type == KF_X_DATA_TEXT_PRIVATE) {
					gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
				}
				if (value) {
					gtk_entry_set_text (GTK_ENTRY (entry), value);
				}
				//gtk_table_attach_defaults (GTK_TABLE (table), label,
				//		0, 1, field_id, field_id+1);
				//gtk_table_attach_defaults (GTK_TABLE (table), entry,
				//		1, 2, field_id, field_id+1);
				gtk_table_attach (GTK_TABLE (table), label,
						0, 1, field_id, field_id+1, GTK_FILL,GTK_FILL,0,0);
				gtk_table_attach (GTK_TABLE (table), entry,
						1, 2, field_id, field_id+1, GTK_FILL,0,0,0);
				field_id++;
				list = g_slist_append (list, 
						kf_x_data_field_new (type, var, entry));
				gtk_widget_show (label);
				gtk_widget_show (entry);
				x= entry;
			} else if (type == KF_X_DATA_TEXT_MULTI) {
				/* Multiline text edit */
				GtkWidget *label;
				GtkWidget *tv;
				GtkTextBuffer *buffer;
				GtkWidget *sw;

				buffer = gtk_text_buffer_new (NULL);
				gtk_text_buffer_set_text (buffer, value, -1);
				tv = gtk_text_view_new_with_buffer (buffer);
				sw = gtk_scrolled_window_new (NULL, NULL);
				gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
						GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
				gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
						GTK_SHADOW_IN);
				gtk_container_add (GTK_CONTAINER (sw), tv);
				gtk_widget_show (sw);
				label = gtk_label_new (title);
				gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
			 	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
				gtk_table_attach (GTK_TABLE (table), label,
						0, 2, field_id, field_id+1, GTK_FILL, 0, 0, 0);
				gtk_table_attach (GTK_TABLE (table), sw,
						0, 2, field_id+1, field_id+2, GTK_FILL, GTK_FILL, 4, 0);
				field_id+=2;
				list = g_slist_append (list, 
						kf_x_data_field_new (type, var, tv));
				gtk_widget_show (label);
				gtk_widget_show (tv);
				x= tv;
			} else if (type == KF_X_DATA_LIST_SINGLE) {
				GtkWidget *widget;
				widget = kf_x_data_create_list (table, title, type, node,
						field_id++, tips);
				x = widget;
				list = g_slist_append (list, 
						kf_x_data_field_new (type, var, widget));
			/*	GtkWidget *label;
				GtkWidget *vbox;
				GtkWidget *opt = NULL;
				LmMessageNode *child;

				label = gtk_label_new (title);
			 	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
				vbox = gtk_vbox_new (FALSE, 0);

				child = node->children;
				while (child) {
					gchar *text;
					
					text = lm_message_node_get_attribute (child, "label");

					if (opt)
						opt = gtk_radio_button_new_with_label_from_widget
							(opt, text);
					else
						opt = gtk_radio_button_new_with_label
							(NULL, text);

					gtk_box_pack_start_defaults (GTK_BOX (vbox), opt);

					child = child->next;
				}
				
				gtk_table_attach_defaults (GTK_TABLE (table), label,
						0, 1, field_id, field_id+1);
				gtk_table_attach_defaults (GTK_TABLE (table), vbox,
						1, 2, field_id, field_id+1);
				field_id++;*/
			} else if (type == KF_X_DATA_BOOLEAN) {
				GtkWidget *widget;
				widget = kf_x_data_create_boolean (table, title, type, node, field_id++);
				if (value && *value == '1')
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
				x = widget;
				list = g_slist_append (list, 
						kf_x_data_field_new (type, var, widget));
			} else if (type == KF_X_DATA_HIDDEN) {
				LmMessageNode *value_node;
				const gchar *value = NULL;
				GtkWidget *widget;
				
				value_node = lm_message_node_get_child (node, "value");
				if (value_node)
					value = g_strdup (value_node->value);
				widget = gtk_label_new (value);
				gtk_table_attach_defaults (GTK_TABLE (table), widget,
					0, 1, field_id, field_id+1);
				field_id++;

				list = g_slist_append (list, 
						kf_x_data_field_new (type, var, widget));
			}
			if (x) {
				gtk_tooltips_set_tip (tips, x, desc, NULL);
			}
		}
		
		node = node->next;
	}
	g_object_set_data (G_OBJECT (eventbox), "Fields", (gpointer) list);
	return eventbox;
	return table;
}

static GtkWidget *kf_x_data_create_list 
	(GtkWidget *table, const gchar *title,
	 KfXDataType type, LmMessageNode *node, guint field_id, GtkTooltips *tips) {
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *opt = NULL;
	LmMessageNode *child;

	LmMessageNode *node_val, *node_desc;
	const gchar *desc = NULL;
	const gchar *main_value = NULL;

	label = kf_x_data_label_new (title);
 	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	vbox = gtk_vbox_new (FALSE, 0);

	node_val = lm_message_node_get_child (node, "value");
	node_desc = lm_message_node_get_child (node, "desc");
	if (node_desc) desc = lm_message_node_get_value (node_desc);
	if (node_val) main_value = lm_message_node_get_value (node_val);

//	foo_debug ("[x:data] value of list: %s\n", main_value);
	child = node->children;
	while (child) {
		LmMessageNode *value_node;
		const gchar *text;
		gchar *value = NULL;
		
		if (strcmp (child->name, "option")) {
			child = child->next;
			continue;
		}
		
		text = lm_message_node_get_attribute (child, "label");
		value_node = lm_message_node_get_child (child, "value");
		if (value_node)
			value = g_strdup (value_node->value);

		if (type == KF_X_DATA_LIST_MULTI) {
			opt = gtk_toggle_button_new_with_label (text);
		} else {
			if (opt)
				opt = gtk_radio_button_new_with_label_from_widget
					(GTK_RADIO_BUTTON (opt), text);
			else
				opt = gtk_radio_button_new_with_label
					(NULL, text);
		}
		if (main_value && strcmp (value, main_value) == 0) {
			foo_debug ("OK");
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (opt), TRUE);
		}
		g_object_set_data_full (G_OBJECT (opt), "value", value, g_free);

		if (desc)
			gtk_tooltips_set_tip (tips, opt, desc, NULL);

		gtk_box_pack_start_defaults (GTK_BOX (vbox), opt);
		gtk_widget_show (opt);

		child = child->next;
	}
	
	gtk_table_attach (GTK_TABLE (table), label,
			0, 1, field_id, field_id+1, GTK_FILL,GTK_FILL,0,0);
	gtk_table_attach (GTK_TABLE (table), vbox,
			1, 2, field_id, field_id+1, GTK_FILL,0,0,0);
	gtk_widget_show (vbox);
	gtk_widget_show (label);
	return vbox;
}

static GtkWidget *kf_x_data_create_boolean 
	(GtkWidget *table, const gchar *title,
	 KfXDataType type, LmMessageNode *node, guint field_id)
{
	GtkWidget *toggle;
	toggle = gtk_check_button_new_with_label (title);
	gtk_table_attach (GTK_TABLE (table), toggle,
		0, 2, field_id, field_id+1, GTK_FILL, 0,0,0);
//		0, 2, field_id, field_id+1, 0, 0,0,0);
	gtk_widget_show (toggle);
	return toggle;
}


KfXDataType kf_x_data_type_from_string (const gchar *type) {
//	KfXDataType type;

	if (!strcmp (type, "text-single"))
		return KF_X_DATA_TEXT_SINGLE;
	else if (!strcmp (type, "text-private"))
		return KF_X_DATA_TEXT_PRIVATE;
	else if (!strcmp (type, "text-multi"))
		return KF_X_DATA_TEXT_MULTI;
	else if (!strcmp (type, "list-single"))
		return KF_X_DATA_LIST_SINGLE;
	else if (!strcmp (type, "list-multi"))
		return KF_X_DATA_LIST_MULTI;
	else if (!strcmp (type, "boolean"))
		return KF_X_DATA_BOOLEAN;
	else if (!strcmp (type, "fixed"))
		return KF_X_DATA_FIXED;
	else if (!strcmp (type, "hidden"))
		return KF_X_DATA_HIDDEN;
	else if (!strcmp (type, "jid-single"))
		return KF_X_DATA_JID_SINGLE;
	else if (!strcmp (type, "jid-multi"))
		return KF_X_DATA_JID_MULTI;
	else
		return KF_X_DATA_TEXT_SINGLE;
//	return type;
}

KfXDataField *kf_x_data_field_new (KfXDataType type,
	const gchar *var,
	GtkWidget *widget) {
	KfXDataField *field;

	field = g_new (KfXDataField, 1);

	field->type = type;
	field->var = g_strdup (var);
	field->widget = widget;

	return field;
}

void kf_x_data_field_free (KfXDataField *field) {
	g_free (field->var);
	g_free (field);
}

LmMessageNode *kf_x_data_create_node (LmMessageNode *parent, GSList *list) {
	LmMessageNode *pnode = lm_message_node_add_child (parent, "x", NULL);
	lm_message_node_set_attribute (pnode, "xmlns", "jabber:x:data");
	lm_message_node_set_attribute (pnode, "type", "submit");
	foo_debug ("[x:data] Creating nodes...\n");
	while (list) {
		KfXDataField *field;
		const gchar *text;
		LmMessageNode *node;

		field = list->data;
		text = get_value (field->type, field->widget);

		foo_debug ("<field var='%s'><value>%s</value></field>\n", field->var, text);
		node = lm_message_node_add_child (pnode, "field", NULL);
		lm_message_node_set_attribute (node, "var", field->var);
		lm_message_node_add_child (node, "value", text);
		if (field->type == KF_X_DATA_TEXT_MULTI)
			g_free (field->var);
		list = list->next;
	}
	return NULL;
}

static const gchar *get_value (KfXDataType type, GtkWidget *widget) {
//	foo_debug ("[x:data] getting value...\n");
	switch (type) {
		case KF_X_DATA_HIDDEN:
			return gtk_label_get_text (GTK_LABEL (widget));
			break;
		case KF_X_DATA_TEXT_SINGLE:
		case KF_X_DATA_TEXT_PRIVATE:
		case KF_X_DATA_JID_SINGLE:
		case KF_X_DATA_JID_MULTI:
			return gtk_entry_get_text (GTK_ENTRY (widget));
			break;
		case KF_X_DATA_LIST_SINGLE:
			return get_value_list_single (widget);
		case KF_X_DATA_BOOLEAN:
			return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))?"1":"0";
		case KF_X_DATA_TEXT_MULTI:
			if (1) {
			GtkTextBuffer *buffer;
			GtkTextIter start, end;

			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
			gtk_text_buffer_get_bounds (buffer, &start, &end);
			return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
			}
		default:
			return NULL;
	}
//	foo_debug ("[x:data] End of get_value\n");
}

static const gchar *get_value_list_single (GtkWidget *widget) {
	GList *list = GTK_BOX (widget)->children;
	GtkBoxChild *child;

//	foo_debug ("[x:data] getting value of single-list...\n");
	while (list) {
		GtkRadioButton *opt;
		
		child = list->data;
		opt = GTK_RADIO_BUTTON (child->widget);
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (opt))) {
			return (gchar *) g_object_get_data (G_OBJECT (opt), "value");
		}
		list = list->next;
	}
	return NULL;
}

static GtkWidget *kf_x_data_label_new (const gchar *text) {
	GtkWidget *label;

	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);

	return label;
}


/*************************/
/*                       */
/*  Basic forms support  */
/*                       */
/*************************/


GtkWidget *kf_basic_form_create (LmMessageNode *node) {
	GtkWidget *table;
	GtkWidget *eventbox;
	guint field_id = 2;
	GSList *list = NULL;
	GdkColor white = {0xffff, 0xffff, 0xffff, 0xffff};

	eventbox = gtk_event_box_new ();
	table = gtk_table_new (1, 2, FALSE);
	gtk_widget_modify_bg (eventbox, GTK_STATE_NORMAL, &white);
	gtk_container_add (GTK_CONTAINER (eventbox), table);
	gtk_widget_show (table);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);

	for (; node; node = node->next) {
		if (strcmp (node->name, "instructions") == 0) {
			GtkWidget *label;
			PangoFontDescription *desc;

			label = gtk_label_new (lm_message_node_get_value (node));
			desc = pango_font_description_from_string ("italic");
			gtk_widget_modify_font (label, desc);
			gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
			gtk_table_attach (GTK_TABLE (table), label, 0, 2, 1, 2, 0,0,0,0);
			gtk_widget_show (label);
		} else if (strcmp (node->name, "registered") == 0) {
			continue;
		} else if (strcmp (node->name, "x") == 0) {
			continue;
		} else {
			GtkWidget *label;
			GtkWidget *entry;
				
			label = kf_x_data_label_new (node->name);
			gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
			entry = gtk_entry_new ();
			if (strcmp (node->name, "password") == 0)
				gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
			
			if (node->value)
				gtk_entry_set_text (GTK_ENTRY (entry), node->value);
			
			gtk_table_attach (GTK_TABLE (table), label,
					0, 1, field_id, field_id+1, GTK_FILL,GTK_FILL,0,0);
			gtk_table_attach (GTK_TABLE (table), entry,
					1, 2, field_id, field_id+1, GTK_FILL,0,0,0);
			field_id++;
			list = g_slist_append (list, 
					kf_x_data_field_new (-1, node->name, entry));
			gtk_widget_show (label);
			gtk_widget_show (entry);
		}
	}
	g_object_set_data (G_OBJECT (eventbox), "Fields", (gpointer) list);
	
	return eventbox;
}

LmMessageNode *kf_basic_create_node (LmMessageNode *parent, GSList *list) {
	while (list) {
		KfXDataField *field = list->data;
		
		lm_message_node_add_child (parent, field->var,
				gtk_entry_get_text (GTK_ENTRY (field->widget)));

		list = list->next;
	}

	return parent;
}

