/* file x_data.h */
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

typedef struct {
	KfXDataType type;
	gchar *var;
	GtkWidget *widget;
} KfXDataField;

GtkWidget *kf_x_data_form (LmMessageNode *node);
KfXDataField *kf_x_data_field_new (KfXDataType type,
	const gchar *var,
	GtkWidget *widget);
void kf_x_data_field_free (KfXDataField *field);
LmMessageNode *kf_x_data_create_node (LmMessageNode *parent, GSList *list);

/*************************/
/*                       */
/*  Basic forms support  */
/*                       */
/*************************/


GtkWidget *kf_basic_form_create (LmMessageNode *node);
LmMessageNode *kf_basic_create_node (LmMessageNode *parent, GSList *list);
