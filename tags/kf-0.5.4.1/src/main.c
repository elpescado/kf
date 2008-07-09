/* file main.c */
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
*/

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <libintl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include "jabber.h"
#include "gui.h"
#include "preferences.h"
#include "kf.h"
#include "emoticons.h"
#include "accounts.h"

static void parse_cmdline (int argc, char *argv[]);

static struct option long_options[] = {
	{"help",	0, NULL, 'h'},
	{"version",	0, NULL, 'v'},
	{"verbose",	0, NULL, '\1'},
	{"select-account", 0, NULL, 's'},
	{0, 0, 0, 0}
};

static time_t start_time;
time_t kf_get_start_time (void);
void kf_account_connect (KfPrefAccount *acc);

static gboolean select_account = FALSE;
static gboolean be_verbose = FALSE;

/*
 *  We don't want error messages from libglade etc, want we?
 * (if we do, pass some command line option... TODO)
 */
void        kf_log_callback                 (const gchar *log_domain,
                                             GLogLevelFlags log_level,
                                             const gchar *message,
                                             gpointer user_data);


int main (int argc, char *argv[]) {
	KfPrefAccount *acc;

	foo_debug ("Debug on\n");
#ifdef ENABLE_BINRELOC
	foo_debug ("BinReloc on\n");
#endif

	gtk_set_locale ();
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
//	bindtextdomain (GETTEXT_PACKAGE, "../po");
	textdomain (GETTEXT_PACKAGE);
#endif
	gtk_init (&argc, &argv);
	parse_cmdline (argc, argv);

	if (! be_verbose) {
		/* Get rid of useless error messages */
		g_log_set_handler ("libglade", G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL
         	            | G_LOG_FLAG_RECURSION, kf_log_callback, NULL);
	}

	start_time = time (NULL);

	kf_preferences_load (kf_config_file ("config.xml"));
//	kf_preferences_write ("config.xml");
	kf_jabber_init ();
	kf_gui_init ();
	kf_emoticons_init ();
//	kf_muc_init ();
	
	/* Auto connection */
	if ((acc = kf_account_get_autoconnect ()) && select_account == FALSE) {
		kf_account_connect (acc);
	} else {
		kf_accounts_connection_settings ();
	}

	gtk_main ();

	kf_gui_shutdown ();
	kf_preferences_write (kf_config_file ("config.xml"));
	return 0;
}


static void parse_cmdline (int argc, char *argv[]) {
	int c, option_index;
//	extern char *klucz, *input, *output;
//	extern char action;

	while ((c = getopt_long (argc, argv, "cdk:hvi:o:s", long_options,
					&option_index)) != -1) {
		switch (c) {
			case 'h':
				g_print (_("kf\n--\nSimple Jabber messenger\n\n(c) 2003 Przemyslaw Sitek"));
				exit (0);
				break;
			case 'v':
				g_print (_("kf version %s\n"), VERSION);
				exit (0);
				break;
			case 's':
				select_account = TRUE;
		}
	}
}

time_t kf_get_start_time (void) {
	return start_time;
}


void        kf_log_callback                 (const gchar *log_domain,
                                             GLogLevelFlags log_level,
                                             const gchar *message,
                                             gpointer user_data)
{
}
