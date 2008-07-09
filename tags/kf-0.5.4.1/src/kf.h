/* file kf.h */
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


/* Config.h */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* Uncomment this to get a lot of debug messages in console */
//#define DEBUG

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
//#warning "ENABLE_NLS\n"
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
//#warning "no NLS\n"
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

const gchar *kf_find_file (const gchar *name);
const gchar *kf_config_file (const gchar *name);

/* Prototypes for Glade */
#ifdef GLADE_H
gint kf_signal_connect (GladeXML *glade, const gchar *name, const gchar *signal, GCallback callback,
		gpointer data);
void   kf_close_button                     (GtkWidget *button,
                                            gpointer user_data);
void kf_destroy_toplevel (GtkWidget *button, gpointer data);
gint kf_glade_get_widgets (GladeXML *glade, ...);
void kf_history (const gchar *jid);
GtkWidget *kf_tip_new (GdkPixbuf *pix, const gchar *text);

#endif

/* Debug macros */
#ifdef DEBUG
#  define foo_debug(args...) g_print (args)
#else
//#  define foo_debug(args...) (args)
#  define foo_debug(args...) ;
#endif

/* For passing strings that may contain NULL. Cool, isn't it? */
#define ISEMPTY(str) (str?str:"")
