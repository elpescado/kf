/* file autoaway.c */
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
#include <gdk/gdkx.h>
#include "kf.h"
#include "preferences.h"
#include "autoaway.h"
#include "jabber.h"

#ifdef HAVE_XSCREENSAVER
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/extensions/scrnsaver.h>
#endif


typedef enum {
	KF_AUTOAWAY_NONE, 
	KF_AUTOAWAY_AWAY,
	KF_AUTOAWAY_XA
} KfAutoAwayType;

static gint kf_autoaway_get_idle (void);
static gboolean kf_autoaway_timeout (gpointer data);
static void kf_autoaway_set_presence (KfAutoAwayType state);

/* Local variables */
//static gboolean enable_aa = TRUE;	/* Enable autoaway */
//static gint aa_delay	= 10;		/* AutoAway delay */
//static gint axa_delay	= 15;		/* AutoXA delay */

static gboolean *enable_aa;	/* Enable autoaway */
static gint *aa_delay;		/* AutoAway delay */
static gint *axa_delay;		/* AutoXA delay */
static gint *aa_status;		/* Current status */


static gint kf_autoaway_get_idle (void) {
#ifdef HAVE_XSCREENSAVER
	static XScreenSaverInfo *info = NULL;
	int event_base, error_base;

	if (XScreenSaverQueryExtension (GDK_DISPLAY(), &event_base, &error_base)) {

		if (info == NULL) {
			info = XScreenSaverAllocInfo ();

			/* Get preferences */
			enable_aa = kf_preferences_get_ptr ("autoAwayEnabled");
			aa_delay = kf_preferences_get_ptr ("autoAwayDelay");
			axa_delay = kf_preferences_get_ptr ("autoXADelay");
			aa_status = kf_preferences_get_ptr ("statusTypeInt");
		}

		XScreenSaverQueryInfo (GDK_DISPLAY (), GDK_ROOT_WINDOW(), info);
		return info->idle / 1000;
	} else {
		foo_debug ("No extension...\n");
		return 0;
	}
#else /* ! HAVE_XSCREENSAVER */
	return 0;
#endif
}

void kf_autoaway_init (void) {
	gtk_timeout_add (1000, kf_autoaway_timeout, NULL);
}

static gboolean kf_autoaway_timeout (gpointer data) {
	gint idle;
	static KfAutoAwayType state = KF_AUTOAWAY_NONE;
	static gint last_idle = 0;
	idle = kf_autoaway_get_idle ();

	/*  v- we have to be connected                   v- We are "Online" or "Free for chat" */
	if (kf_jabber_get_connected () && *enable_aa && *aa_status <= 1) {
		if (idle > *aa_delay * 60 && idle < *axa_delay * 60 && state == KF_AUTOAWAY_NONE) {
			/* Auto away */
			kf_autoaway_set_presence (KF_AUTOAWAY_AWAY);
			state = KF_AUTOAWAY_AWAY;
		} else if (idle > *axa_delay * 60 && state != KF_AUTOAWAY_XA) {
			/* Auto XA */
			kf_autoaway_set_presence (KF_AUTOAWAY_XA);
			state = KF_AUTOAWAY_XA;
		} else {
			/* Do nothing;-) */

			if (state != KF_AUTOAWAY_NONE && last_idle > idle) {
				/* User has just done sth... */

				kf_autoaway_set_presence (KF_AUTOAWAY_NONE);
				state = KF_AUTOAWAY_NONE;
			}
		}

	}
	last_idle = idle;
	return TRUE;
}

static void kf_autoaway_set_presence (KfAutoAwayType state) {
	if (state == KF_AUTOAWAY_NONE) {
		kf_jabber_send_presence (
				kf_preferences_get_string ("statusType"),
				kf_preferences_get_string ("statusText"));
				
	} else if (state == KF_AUTOAWAY_AWAY) {
		kf_jabber_send_presence ("away", kf_preferences_get_string ("autoAwayText"));
	} else if (state == KF_AUTOAWAY_XA) {
		kf_jabber_send_presence ("xa", kf_preferences_get_string ("autoXAText"));
	}
}
/*
void kf_autoaway_set (gboolean enable, gint away, gint xa) {
	enable_aa = enable;
	aa_delay = away;
	axa_delay = xa;
}

void kf_autoaway_get (gboolean *enable, gint *away, gint *xa) {
	*enable = enable_aa;
	*away = aa_delay;
	*xa = axa_delay;
}
*/
	
