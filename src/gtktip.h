/* file gtktip.h */
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
#ifndef __GTK_TIP_H__
#define __GTK_TIP_H__

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkwidget.h>

//G_BEGIN_DECLS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_TIP(obj)		GTK_CHECK_CAST (obj, gtk_tip_get_type (), GtkTip)
#define GTK_TIP_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gtk_tip_get_type (). GtkTipClass)
#define GTK_IS_TIP(obj)		GTK_CHECK_TYPE (obj, gtk_tip_get_type ())

typedef struct _GtkTip		GtkTip;
typedef struct _GtkTipClass	GtkTipClass;

struct _GtkTip {
	GtkWindow window;

	GdkPixbuf *pix;		/* small pixmap */
	PangoLayout *layout;	/* text */
	GdkPixbuf *bg_pix;	/* big pixmap */
};

struct _GtkTipClass {
	GtkWindowClass parent_class;
};

GtkWidget*	gtk_tip_new		(GdkPixbuf *pix, const gchar *markup);
GtkType		gtk_tip_get_type	(void);
void		gtk_tip_set_bg_pixbuf	(GtkTip *tip, GdkPixbuf *pix);
void		gtk_tip_get_size	(GtkTip *tip, gint *w, gint *h);

//G_END_DECLS
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_TIP_H__ */
