/* file xevent.c */
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
#include <string.h>
#include "kf.h"
#include "events.h"
#include "xevent.h"
#include "gui.h"

/* This is not connected with X Window System... */
typedef struct _KfXEventBox KfXEventBox;
typedef struct _KfXEvent KfXEvent;
typedef struct _KfXEventItem KfXEventItem;

struct _KfXEventBox {
	gint ref_count;
	GSList *events;
	KfXEventItem *last;
};

struct _KfXEvent {
	KfXEventBox *box;
	gchar *jid;
	GSList *items;
};

struct _KfXEventItem {
	KfEventClass klass;
	gpointer data;
};


static KfXEventBox *kf_x_event_box_new (void);
static void kf_x_event_box_unref (KfXEventBox *self);
static KfXEvent *kf_x_event_box_get_event (KfXEventBox *box, const gchar *jid);
static KfXEventItem *kf_x_event_push_item (KfXEvent *event, KfEventClass klass, gpointer data);
static void kf_x_event_item_unref (KfXEventItem *item);
static void kf_x_event_item_release (KfXEventItem *item);
static gboolean kf_x_event_box_timeout (gpointer data);

static KfXEventBox *box = NULL;

/* Actual code starts here... */

/* First acces functions */

/* An XEvent ocurred */
void kf_x_event_push (const gchar *jid, KfEventClass klass, gpointer data) {
	KfXEvent *event;

	foo_debug ("kf_x_event_push (%s, %d, %p);\n", jid, klass, data);
	
	if (box == NULL) {
		box = kf_x_event_box_new ();
	}

	event = kf_x_event_box_get_event (box, jid);
	kf_x_event_push_item (event, klass, data);
	kf_update_status_by_jid (event->jid, TRUE);	
}

void kf_x_event_release_all (void) {
	KfXEvent *event;
	GSList *l;
	
	foo_debug ("kf_x_event_release_all ()\n");
	
	if (box == NULL) {
		box = kf_x_event_box_new ();
	}

	for (l = box->events; l; l = l->next) {
		GSList *tmp;
		event = l->data;
	
		for (tmp = event->items; tmp; tmp = tmp->next) {
			KfXEventItem *item = tmp->data;

			kf_x_event_item_release (item);
			kf_x_event_item_unref (item);
		}
		g_slist_free (event->items);
		event->items = NULL;
		kf_update_status_by_jid (event->jid, TRUE);
	}
	box->last = NULL;
}

void kf_x_event_release (const gchar *jid) {
	KfXEvent *event;
	GSList *tmp;
	GSList *l;
	
	foo_debug ("kf_x_event_release (%s)\n", jid);

	if (box == NULL) {
		box = kf_x_event_box_new ();
		return;
	}

	event = kf_x_event_box_get_event (box, jid);
	
	for (tmp = event->items; tmp; tmp = tmp->next) {
		KfXEventItem *item = tmp->data;

		kf_x_event_item_release (item);
		kf_x_event_item_unref (item);
	}
	g_slist_free (event->items);
	event->items = NULL;
	box->last = NULL;
	for (l = box->events; l; l = l->next) {
		event = l->data;

		if (event->items)
			box->last = event->items->data;
	}
	kf_update_status_by_jid (event->jid, TRUE);	
}


KfEventClass kf_x_event_last (const gchar *jid) {
	KfXEvent *event;

	if (box == NULL) {
		box = kf_x_event_box_new ();
	}

	if (jid) {
		event = kf_x_event_box_get_event (box, jid);
	
		if (event->items) {
			KfXEventItem *item = event->items->data;
			return item->klass;
		} else {
			return -1;
		}
	} else {
		return (box->last) ? (box->last->klass) : (-1);
	}
}


/* Then actual implementation */

static KfXEventBox *kf_x_event_box_new (void) {
	KfXEventBox *self;

	self = g_new (KfXEventBox,1);
	self->ref_count = 1;
	self->events = NULL;
	self->last = NULL;
	
	gtk_timeout_add (1000, kf_x_event_box_timeout, self);
	
	return self;
}

static void kf_x_event_box_unref (KfXEventBox *self) {
	if (--self->ref_count) {
		g_slist_free (self->events);
		g_free (self);
	}
}

static KfXEvent *kf_x_event_box_get_event (KfXEventBox *box, const gchar *jid) {
	KfXEvent *event = NULL;
	GSList *tmp;

	for (tmp = box->events; tmp; tmp = tmp->next) {
		KfXEvent *ev = tmp->data;
		if (strcmp (ev->jid, jid) == 0) {
			event = ev;
			break;
		}
	}

	if (event == NULL) {
		event = g_new (KfXEvent, 1);
		event->jid = g_strdup (jid);
		event->items = NULL;
		event->box = box;

		box->events = g_slist_prepend (box->events, event);
	}

	return event;
}

static KfXEventItem *kf_x_event_push_item (KfXEvent *event, KfEventClass klass, gpointer data) {
	KfXEventItem *item;

	item = g_new (KfXEventItem, 1);
	item->klass = klass;
	item->data = data;

	event->items = g_slist_prepend (event->items, item);
	event->box->last = item;

	return item;
}

static void kf_x_event_item_unref (KfXEventItem *item) {
	g_free (item);
}

static void kf_x_event_item_release (KfXEventItem *item) {
	gtk_widget_show (item->data);
}

/* This funstion animates XEvents in roster :-) */
/* Eye-candy */
static gboolean kf_x_event_box_timeout (gpointer data) {
	KfXEventBox *box = data;
	GSList *tmp;
	
	for (tmp = box->events; tmp; tmp = tmp->next) {
		KfXEvent *event = tmp->data;
		if (event->items) {
			kf_update_status_by_jid (event->jid, FALSE);	
		}
	}


	return TRUE;	
}
