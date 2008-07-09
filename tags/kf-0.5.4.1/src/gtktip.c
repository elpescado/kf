/* file gtktip.c */
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
 * GtkTip - a small tooltip window...
 *
 * (c) 2004 Przemys³aw Sitek <psitek@rams.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include "gtktip.h"

#define	BORDER_WIDTH	8
#define SPACING		8

static void gtk_tip_class_init (GtkTipClass *klass);
static void gtk_tip_init (GtkTip *tip);
static void gtk_tip_destroy (GtkTip *tip);
static gboolean gtk_tip_expose (GtkTip *widget, GdkEventExpose *event);
	
GtkType		gtk_tip_get_type	(void) {
	static GtkType tip_type = 0;

	if (! tip_type) {
		static const GtkTypeInfo tip_info = {
			"GtkTip",
			sizeof (GtkTip),
			sizeof (GtkTipClass),
			(GtkClassInitFunc) gtk_tip_class_init,
			(GtkObjectInitFunc) gtk_tip_init,
			/* reserved_1 */ NULL,
			/* reserved_1 */ NULL,
			(GtkClassInitFunc) NULL
		};

		tip_type = gtk_type_unique (GTK_TYPE_WINDOW, &tip_info);
	}

	return tip_type;
}

static void gtk_tip_class_init (GtkTipClass *klass) {
	GtkObjectClass *object_class = (GtkObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;
//	GtkWindowClass *window_class = (GtkWindowClass *) klass;

	/* This is useless */
//	widget_class->realize	= gtk_tip_realize;
	object_class->destroy		= gtk_tip_destroy;
	widget_class->expose_event	= gtk_tip_expose;

}

static void gtk_tip_init (GtkTip *tip) {
//	gtk_window_set_decorated (GTK_WINDOW (tip), FALSE);
	tip->bg_pix = NULL;
	GTK_WINDOW (tip)->type = GTK_WINDOW_POPUP;
//	g_object_set (G_OBJECT (tip), "type", GTK_WINDOW_POPUP, NULL);	
	gtk_widget_set_name (GTK_WIDGET (tip), "gtk-tooltips");
	gtk_widget_set_app_paintable (GTK_WIDGET (tip), TRUE);
//	g_signal_connect(G_OBJECT(tip), "expose_event",
//			G_CALLBACK(gtk_tip_expose), NULL);
}

static void gtk_tip_destroy (GtkTip *tip) {
	g_return_if_fail (tip != NULL);
	g_return_if_fail (GTK_IS_TIP (tip));

	g_object_unref (tip->pix);
	g_object_unref (tip->bg_pix);
	g_object_unref (tip->layout);
}

GtkWidget *gtk_tip_new (GdkPixbuf *pix, const gchar *markup) {
	GtkTip *tip;
	PangoContext *context;
	gint w, h;

	tip = gtk_type_new (gtk_tip_get_type ());

	GTK_WINDOW (tip)->type = GTK_WINDOW_POPUP;
	tip->pix = g_object_ref (pix);
	context = gtk_widget_get_pango_context (GTK_WIDGET (tip));
//	tip->layout = pango_layout_new (gdk_pango_context_get());
	tip->layout = pango_layout_new (context);
	pango_layout_set_markup (tip->layout, markup, -1);
//	pango_layout_set_wrap (tip->layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_wrap (tip->layout, PANGO_WRAP_WORD);
	pango_layout_set_width (tip->layout, 200 * PANGO_SCALE);
	
	gtk_tip_get_size (tip, &w, &h);
	gtk_window_resize (GTK_WINDOW (tip), w, h);

	return GTK_WIDGET (tip);
}

void gtk_tip_set_bg_pixbuf (GtkTip *tip, GdkPixbuf *pix) {
	tip->bg_pix = g_object_ref (pix);
}


static gboolean gtk_tip_expose (GtkTip *tip, GdkEventExpose *event) {
	GtkWidget *widget = GTK_WIDGET (tip);
	gint bg_width = 48, bg_height = 48, pix_width, pix_height;
	gint layout_width, layout_height;
	gint pix_x = BORDER_WIDTH, pix_y = BORDER_WIDTH;
	gint width, height;

//	g_print ("Expose [%ld]...\n", time (NULL));
	pix_width = gdk_pixbuf_get_width (tip->pix);
	pix_height = gdk_pixbuf_get_height (tip->pix);
		
	if (tip->bg_pix) {
		bg_width = gdk_pixbuf_get_width (tip->bg_pix);
		bg_height = gdk_pixbuf_get_height (tip->bg_pix);
		
		gdk_pixbuf_render_to_drawable (tip->bg_pix,
			widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
			0, 0,
			BORDER_WIDTH, BORDER_WIDTH,
			-1, -1,
			GDK_RGB_DITHER_NONE, 0, 0);

		pix_x = BORDER_WIDTH + bg_width - pix_width - 4;
		pix_y = BORDER_WIDTH + bg_height - pix_height - 4;
	}
	
	gdk_pixbuf_render_to_drawable (tip->pix,
			widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
			0, 0,
			pix_x, pix_y,
			-1, -1,
			GDK_RGB_DITHER_NONE, 0, 0);
	
//	gdk_draw_rectangle (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
//			FALSE,
//			8, 8,
//			16, 16);
//	gdk_pixbuf_render_to_drawable (GTK_TIP (widget)->pix,
//			widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
//			0, 0,
//			8, 8,
//			-1, -1,
//			GDK_RGB_DITHER_NONE, 0, 0);
	gdk_draw_layout (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
			bg_width + BORDER_WIDTH + SPACING, BORDER_WIDTH,
			GTK_TIP (widget)->layout);

	pango_layout_get_pixel_size (tip->layout, &layout_width, &layout_height);

	width = 2 * BORDER_WIDTH + bg_width + layout_width + SPACING;	
	height = 2 * BORDER_WIDTH + MAX (bg_height, layout_height);	
	
	gdk_draw_rectangle (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
			FALSE,
			0, 0,
			width, height);
	return TRUE;
/*	This causes tip window to be exposed over and over taking a lot of CPU power...
	gtk_window_resize (GTK_WINDOW (tip), width + 1, height + 1);

	return TRUE;*/
}

void gtk_tip_get_size (GtkTip *tip, gint *w, gint *h) {
	gint bg_width = 48, bg_height = 48;
	gint layout_width, layout_height;
	gint width, height;

	if (tip->bg_pix) {
		bg_width = gdk_pixbuf_get_width (tip->bg_pix);
		bg_height = gdk_pixbuf_get_height (tip->bg_pix);
	}
	
	pango_layout_get_pixel_size (tip->layout, &layout_width, &layout_height);

	width = 2 * BORDER_WIDTH + bg_width + layout_width + SPACING;	
	height = 2 * BORDER_WIDTH + MAX (bg_height, layout_height);	
	
	*w = width + 1;
	*h = height + 1;
}
