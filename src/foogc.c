/* file foogc.c */
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


/*
 * kf - Garbage Collector
 */

#include <glib.h>
#include "foogc.h"

/* Create a new Garbage Collector */
FooGC *foo_gc_new (GDestroyNotify notify) {
	FooGC *foo;

	foo = g_new (FooGC, 1);
	foo->items = NULL;
	foo->notify = notify;

	return foo;
}

/* Add an item to a Garbage Collector */
void foo_gc_add (FooGC *foo, gpointer data) {
	g_return_if_fail (foo);

	foo->items = g_slist_prepend (foo->items, data);
}

/* Destroy Garbage Collector with all associated items */
void foo_gc_free (FooGC *foo) {
	GSList *tmp;
	
	g_return_if_fail (foo);
	for (tmp = foo->items; tmp; tmp = tmp->next) {
		foo->notify (tmp->data);
	}
	g_slist_free (foo->items);
	g_free (foo);
}

/*
 * Remove an item from a FooGC
 */
void foo_gc_remove (FooGC *foo, gpointer data) {
	foo->items = g_slist_remove (foo->items, data);
}
