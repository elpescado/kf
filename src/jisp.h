/* file jisp.h */
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#define JISP_N_ICONS 7 

typedef enum {
	KF_ICON_ONLINE,
	KF_ICON_CHAT,
	KF_ICON_AWAY,
	KF_ICON_XA,
	KF_ICON_DND,
	KF_ICON_INVISIBLE,
	KF_ICON_UNAVAILABLE,
	KF_ICON_MESSAGE,	/* Unsupported	*/
	KF_ICON_CHAT2,		/* Unsupported	*/
	KF_ICON_PRESENCE,	/* Unsupported	*/
	KF_ICON_SYSTEM		/* Unsupported	*/
} KfJispIcon;

/* Jabber Icon Set Package */
typedef struct {
	GdkPixbuf *icons[JISP_N_ICONS];	/* Roster icon pixbufs	*/
	gchar	*name;			/* JISP name		*/
	gchar	*desc;			/* JISP description	*/
	GList	*authors;		/* JISP authors		*/
	gchar	*filename;		/* file containing JISP	*/
	gboolean active;		/* Whether this JISP should be a source of emoticons	*/
	GdkPixbuf *preview;		/* Preview of roster icons		*/
	gboolean roster;		/* Whether JISP contains roster icons	*/
	GList	*emoticons;		/* List of LfJispEmo emoticons		*/
	gint	ref_count;		/* Reference count	*/
} KfJisp;

/* Information about one emoticon stored in a JISP */
typedef struct {
	GSList *texts;	/* List of texts (gchar *)	*/
	GdkPixbuf *pix;	/* image			*/
} KfJispEmo;

/* Information about author of a JISP */
typedef struct {
	gchar *name;
	gchar *jid;
	gchar *email;
	gchar *www;
} KfJispAuthor;

KfJisp *kf_jisp_new (void);
KfJisp *kf_jisp_ref (KfJisp *jisp);
void kf_jisp_unref (KfJisp *jisp);
void kf_jisp_free (KfJisp *jisp);
KfJispEmo *kf_jisp_emo_new (void);
void kf_jisp_emo_free (KfJispEmo *emo);
KfJisp *kf_jisp_new_from_file (const gchar *file);
KfJisp *kf_jisp_install (const gchar *zipfile);
void kf_jisp_uninstall (KfJisp *jisp);

KfJispAuthor *kf_jisp_author_new (const gchar *name);
void kf_jisp_author_free (KfJispAuthor *author);
