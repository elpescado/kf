/* file new_account.c */
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
#include <loudmouth/loudmouth.h>
#include <glade/glade.h>
#include <string.h>
#include "kf.h"
#include "preferences.h"
#include "new_account.h"
#include "gui.h"

typedef enum {
	KF_ACCOUNT_REG_NONE,
	KF_ACCOUNT_REG_CONNECTING,
	KF_ACCOUNT_REG_REGISTERING,
	KF_ACCOUNT_REG_OK
} KfAccountRegState;

typedef struct {
	GtkWidget *window;
	GladeXML *glade;
	LmConnection *connection;
	KfAccountRegState state;

	/* Widgets */
	GtkWidget *ok;
	GtkWidget *cancel;
	GtkWidget *server;
	GtkWidget *user;
	GtkWidget *pass;
	GtkWidget *pass2;
	GtkWidget *add;
	GtkWidget *name;

	/* Fields */
	gchar *xserver;
	gchar *xuser;
	gchar *xpass;
} KfAccountReg;

static void _register (KfAccountReg *reg, const gchar *server, const gchar *user, const gchar *pass);
static void open_cb (LmConnection *connection, gboolean success, gpointer data);
static LmHandlerResult reg_hendel            (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer user_data);
static void reg_add_account (KfAccountReg *reg);

static gboolean reg_delete_cb (GtkWidget *widget, GdkEvent *event, gpointer data);
static void reg_ok_cb (GtkButton *button, gpointer data);
static void reg_cancel_cb (GtkButton *button, gpointer data);
static void reg_changed_cb                 (GtkEditable *editable, gpointer data);

static void reg_free (KfAccountReg *reg);







void kf_new_account_button_cb (GtkButton *button, gpointer data) {
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	kf_new_account ();
}

void kf_new_account (void) {
	KfAccountReg *reg;

	reg = g_new (KfAccountReg,1);

	reg->glade = glade_xml_new (kf_find_file ("new_account.glade"), NULL, NULL);
	reg->window = glade_xml_get_widget (reg->glade, "new_account");
	reg->connection = NULL; /* We don't need it at this stage */
	reg->state = KF_ACCOUNT_REG_NONE;

	kf_glade_get_widgets (reg->glade,
			"ok", &reg->ok,
			"cancel", &reg->cancel,
			"server", &reg->server,
			"username", &reg->user,
			"pass", &reg->pass,
			"pass_re", &reg->pass2,
			"add", &reg->add,
			"name", &reg->name,
			NULL);

	reg->xserver = NULL;
	reg->xuser = NULL;
	reg->xpass = NULL;

	g_signal_connect (G_OBJECT (reg->window), "delete_event", G_CALLBACK (reg_delete_cb), reg);
	g_signal_connect (G_OBJECT (reg->server), "changed", G_CALLBACK (reg_changed_cb), reg);
	g_signal_connect (G_OBJECT (reg->user), "changed", G_CALLBACK (reg_changed_cb), reg);
	g_signal_connect (G_OBJECT (reg->pass), "changed", G_CALLBACK (reg_changed_cb), reg);
	g_signal_connect (G_OBJECT (reg->pass2), "changed", G_CALLBACK (reg_changed_cb), reg);
	g_signal_connect (G_OBJECT (reg->ok), "clicked", G_CALLBACK (reg_ok_cb), reg);
	g_signal_connect (G_OBJECT (reg->cancel), "clicked", G_CALLBACK (reg_cancel_cb), reg);

	g_object_set_data_full (G_OBJECT (reg->window), "reg", reg, (GDestroyNotify) reg_free);
	g_object_set_data_full (G_OBJECT (reg->window), "glade", reg->glade, g_object_unref);
}


static void _register (KfAccountReg *reg, const gchar *server, const gchar *user, const gchar *pass) {
	GError *error;
	reg->connection = lm_connection_new (server);
	g_print ("Connecting...\n");
	lm_connection_open (reg->connection, open_cb, reg, NULL, &error);
	
	reg->state = KF_ACCOUNT_REG_CONNECTING;
}

static void open_cb (LmConnection *connection, gboolean success, gpointer data) {
	KfAccountReg *reg = data;
	if (success) {
		LmMessage *msg;
		LmMessageNode *node;
		LmMessageHandler *hendel;
		g_print ("Connected...\n");
		
		hendel = lm_message_handler_new (reg_hendel, reg, NULL);

		msg = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
		node = lm_message_node_add_child (msg->node, "query", NULL);
		lm_message_node_set_attribute (node, "xmlns", "jabber:iq:register");

		/*subnode =*/ lm_message_node_add_child (node, "username", reg->xuser);

		/*subnode =*/ lm_message_node_add_child (node, "password", reg->xpass);

		lm_connection_send_with_reply (reg->connection, msg, hendel, NULL);
		lm_message_unref (msg);


		reg->state = KF_ACCOUNT_REG_REGISTERING;
	} else {
		g_print ("Failed...\n");
	}
}

static LmHandlerResult reg_hendel            (LmMessageHandler *handler,
                                             LmConnection *connection,
                                             LmMessage *message,
                                             gpointer data)
{
	KfAccountReg *reg = data;
	LmMessageNode *error;
	g_print (lm_message_node_to_string (message->node));
	if ((error = lm_message_node_find_child (message->node, "error"))) {
		gchar *text;
		const gchar *code, *status;
		GtkWidget *table;
		GtkWidget *status_label;
		
		code = lm_message_node_get_attribute (error, "code");
		status = lm_message_node_get_value (error);
		text = g_strdup_printf (_("Error #%s: %s"), code, status);
		kf_gui_alert (text);
		g_free (text);

		kf_glade_get_widgets (reg->glade, "table",
				&table, "status_label", &status_label, NULL);
		gtk_widget_set_sensitive (table, TRUE);
		gtk_label_set_text (GTK_LABEL (status_label), "");
		reg->state = KF_ACCOUNT_REG_NONE;
	} else {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (reg->add))) {
			reg_add_account (reg);
		}
		gtk_widget_destroy (reg->window);
		kf_gui_alert (_("Account registered."));
		reg->state = KF_ACCOUNT_REG_OK;
	}
	lm_connection_close (connection, NULL);
	//lm_connection_unref (connection);
	lm_message_handler_unref (handler);	
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

/* Add this account to accounts list */
static void reg_add_account (KfAccountReg *reg) {
	const gchar *name;

	name = gtk_entry_get_text (GTK_ENTRY (reg->name));
	if (name && *name) {
		KfPrefAccount *acc;
		GtkWidget *pass_save = NULL;
		extern GList *kf_preferences_accounts;
		
		acc = kf_pref_account_new (name);
		acc->uname = g_strdup (reg->xuser);
		acc->server = g_strdup (reg->xserver);
		pass_save = glade_xml_get_widget (reg->glade, "pass_save");
		if (pass_save && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pass_save))) {
			acc->save_password = TRUE;
			acc->pass = g_strdup (reg->xpass);
		}
		kf_preferences_accounts = g_list_prepend (kf_preferences_accounts, acc);
		/* Let's save config file */
		kf_preferences_write (kf_config_file ("config.xml"));
	}
}

/* Event handlers */


static gboolean reg_delete_cb (GtkWidget *widget, GdkEvent *event, gpointer data) {
	KfAccountReg *reg = data;

	if (reg->state == KF_ACCOUNT_REG_NONE || reg->state == KF_ACCOUNT_REG_OK)
		return FALSE;
	else
		return TRUE;

}

static void reg_ok_cb (GtkButton *button, gpointer data) {
	KfAccountReg *reg = data;

	if (reg->state == KF_ACCOUNT_REG_NONE) {
		/* Register */
		const gchar *user;
		const gchar *server;
		const gchar *pass;
		const gchar *pass2;

		GtkWidget *table;
		GtkWidget *status_label;

		server = gtk_entry_get_text (GTK_ENTRY (reg->server));
		user = gtk_entry_get_text (GTK_ENTRY (reg->user));
		pass = gtk_entry_get_text (GTK_ENTRY (reg->pass));
		pass2 = gtk_entry_get_text (GTK_ENTRY (reg->pass2));

		if (strcmp (pass, pass2)) {
			kf_gui_alert ("Passwords doesn't match!");
			return;
		}
		reg->xserver = g_strdup (server);
		reg->xuser = g_strdup (user);
		reg->xpass = g_strdup (pass);

		_register (reg, server, user, pass);
		
		kf_glade_get_widgets (reg->glade, "table",
				&table, "status_label", &status_label, NULL);
		gtk_widget_set_sensitive (table, FALSE);
		gtk_label_set_markup (GTK_LABEL (status_label), "<i>Registering account. Please wait...</i>");
	} else if (reg->state == KF_ACCOUNT_REG_OK) {
		/* Just close window */
		gtk_widget_destroy (reg->window);
	}
}


static void reg_cancel_cb (GtkButton *button, gpointer data) {
	KfAccountReg *reg = data;

	if (reg->state == KF_ACCOUNT_REG_NONE || reg->state == KF_ACCOUNT_REG_OK)
		gtk_widget_destroy (reg->window);
}

/*
 * Check if there is anything entered in 'username' and 'server'
 * fields, if not, disable 'connect' button
 */
static void reg_changed_cb                 (GtkEditable *editable,
                                            gpointer data) {


	KfAccountReg *reg = data;
	const gchar *s1, *s2, *s3, *s4;
	s1 = gtk_entry_get_text (GTK_ENTRY (reg->server));
	s2 = gtk_entry_get_text (GTK_ENTRY (reg->user));
	s3 = gtk_entry_get_text (GTK_ENTRY (reg->pass));
	s4 = gtk_entry_get_text (GTK_ENTRY (reg->pass2));
//	g_print ("[%s] :: [%s]\n", sname, sserver);
	if (*s1 && *s2 && *s3 && *s4) {
		gtk_widget_set_sensitive (reg->ok, TRUE);
	} else {
		gtk_widget_set_sensitive (reg->ok, FALSE);
	}
}


static void reg_free (KfAccountReg *reg) {
	g_free (reg->xserver);
	g_free (reg->xuser);
	g_free (reg->xpass);
	g_free (reg);
}
