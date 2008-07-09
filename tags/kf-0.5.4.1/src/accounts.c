/* file accounts.c */
/*
 * kf linux jabber client
 * ----------------------
 *
 * Copyright (C) 2003-2004 Przemys�aw Sitek <psitek@rams.pl> 
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
#include <glade/glade.h>
#include "kf.h"
#include "gui.h"
#include "jabber.h"
#include "connection.h"
#include "callbacks.h"
#include "preferences.h"
#include "disclosure-widget.h"
#include "accounts.h"

static void accounts_changed_cb            (GtkEditable *editable,
                                            GtkWidget *user_data);
static void  kf_select_account             (GtkWidget *menuitem,
                                            gpointer user_data);
static void  kf_save_account               (KfPrefAccount *acc, GladeXML *glade);
static const gchar *value_of (GladeXML *glade, const gchar *name);

static void
on_text_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *cell,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void
on_toggle_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *cell,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data);
static void on_autoconnect_toggled         (GtkCellRendererToggle *cellrenderertoggle,
                                            gchar *arg1,
                                            gpointer user_data);
void        on_connection_zapisz_clicked   (GtkButton *button,
                                            gpointer user_data);
void        on_accounts_add_clicked        (GtkButton *button,
                                            gpointer user_data);
void        on_accounts_edit_clicked       (GtkButton *button,
                                            gpointer user_data);
void        on_accounts_rename_clicked     (GtkButton *button,
                                            gpointer user_data);
void        on_accounts_remove_clicked     (GtkButton *button,
                                            gpointer user_data);
void        on_connection_quit_clicked     (GtkButton *button,
                                            gpointer user_data);
void        on_connection_ok_clicked       (GtkButton *button,
                                            gpointer user_data);
gboolean    on_connection_settings_window_delete_event (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data);
void        on_connection_dodaj_konto_clicked (GtkButton *button,
                                            gpointer user_data);

static KfPrefAccount *get_selection (GtkWidget *widget);
static void hide (GladeXML *glade, const gchar *name);
static void expander_toggled (GtkToggleButton *expander, gpointer data);
void kf_account_connect (KfPrefAccount *acc);

/* in vcard.c */
void setwidget (GladeXML *glade, const gchar *name, const gchar *value);

void kf_accounts_connection_settings (void) {
	GladeXML *glade;
	GtkWidget *cs; /* Connection Settings window */
	GtkWidget *menu;
	GtkWidget *item;
	extern GList *kf_preferences_accounts;
	GList *account;
	KfPrefAccount *acc;
	GtkOptionMenu *optmenu;
	GtkWidget *okbutton;
	GtkWidget *new;
	GtkWidget *expander;
	GtkWidget *table;
	GtkWidget *frame;

	GtkWidget *use_manual_host, *manual_host;

	glade = glade_xml_new (kf_find_file ("kf.glade"), "connection_settings_window", NULL);
	glade_xml_signal_autoconnect (glade);

	optmenu = GTK_OPTION_MENU (glade_xml_get_widget (glade, "connection_konto_selector"));
	if (!optmenu)
		g_error ("Nie ma menu!\n");
	if ((menu = gtk_option_menu_get_menu (optmenu))) {
		gtk_widget_destroy (menu);
	}

	menu = gtk_menu_new ();
	item = gtk_menu_item_new_with_label (_("[none]"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_object_set_data (G_OBJECT (item), "glade", glade);
	g_signal_connect (G_OBJECT (item), "activate",
			G_CALLBACK (kf_select_account), NULL);
	gtk_widget_show (item);
	for (account = kf_preferences_accounts; account; account = account->next) {
		acc = (KfPrefAccount *) account->data;

		item = gtk_menu_item_new_with_label (acc->name);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_object_set_data (G_OBJECT (item), "glade", glade);
		g_signal_connect (G_OBJECT (item), "activate",
				G_CALLBACK (kf_select_account), acc);
		gtk_widget_show (item);
	}

	gtk_option_menu_set_menu (optmenu, GTK_WIDGET (menu));
	gtk_widget_show (menu);

	cs = glade_xml_get_widget (glade, "connection_settings_window");
	g_object_set_data_full (G_OBJECT (cs), "glade", glade, g_object_unref);

	okbutton = glade_xml_get_widget (glade, "connection_ok");
	gtk_widget_set_sensitive (okbutton, FALSE);

	kf_signal_connect (glade, "connection_username", "changed",
			G_CALLBACK (accounts_changed_cb), okbutton);
	kf_signal_connect (glade, "connection_server", "changed",
			G_CALLBACK (accounts_changed_cb), okbutton);

	new = glade_xml_get_widget (glade, "register_new");
	gtk_widget_show (new);
	
	frame = glade_xml_get_widget (glade, "connection_frame");
	expander = cddb_disclosure_new (_("Show more options"),_("Hide more oprions"));
	g_signal_connect (G_OBJECT (expander), "toggled", G_CALLBACK (expander_toggled), frame);
	gtk_widget_show (expander);
	
	table = glade_xml_get_widget (glade, "table1");
	gtk_table_attach (GTK_TABLE (table), expander, 0, 3, 7, 8, GTK_FILL, 0, 0, 0);

	use_manual_host = glade_xml_get_widget (glade, "connection_use_manual_server");
	manual_host = glade_xml_get_widget (glade, "connection_manual_server");
#ifdef HAVE_LM_CONNECTION_SET_JID
		
#else
	gtk_widget_set_sensitive (use_manual_host, FALSE);
	gtk_widget_set_sensitive (manual_host, FALSE);
#endif
	
	kf_gui_show (cs);
}

/*
 * Check if there is anything entered in 'username' and 'server'
 * fields, if not, disable 'connect' button
 */
static void accounts_changed_cb            (GtkEditable *editable,
                                            GtkWidget *okbutton) {
	GladeXML *glade;
	GtkWidget *name, *server;
	const gchar *sname, *sserver;

	glade = glade_get_widget_tree (okbutton);
	name = glade_xml_get_widget (glade, "connection_username");
	sname = gtk_entry_get_text (GTK_ENTRY (name));
	server = glade_xml_get_widget (glade, "connection_server");
	sserver = gtk_entry_get_text (GTK_ENTRY (server));

//	g_print ("[%s] :: [%s]\n", sname, sserver);
	if (*sname && *sserver) {
		gtk_widget_set_sensitive (okbutton, TRUE);
	} else {
		gtk_widget_set_sensitive (okbutton, FALSE);
	}
}

static void  kf_select_account             (GtkWidget *menuitem,
                                            gpointer user_data) {
	KfPrefAccount *acc;
	GtkWidget *entry;
	GladeXML *glade = g_object_get_data (G_OBJECT (menuitem), "glade");
	g_assert (glade);
	entry = glade_xml_get_widget (glade, "connection_username");

	if (user_data) {
		acc = (KfPrefAccount *) user_data;
		gtk_entry_set_text (GTK_ENTRY (entry), ISEMPTY (acc->uname));
		entry = glade_xml_get_widget (glade, "connection_server");
		gtk_entry_set_text (GTK_ENTRY (entry), ISEMPTY (acc->server));
		entry = glade_xml_get_widget (glade, "connection_password");
		if (acc->save_password && acc->pass)
			gtk_entry_set_text (GTK_ENTRY (entry), ISEMPTY (acc->pass));
		else
			gtk_entry_set_text (GTK_ENTRY (entry), "");

		entry = glade_xml_get_widget (glade, "connection_save_password");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), acc->save_password);
		entry = glade_xml_get_widget (glade, "connection_port");
		if (acc->port > 0)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), acc->port);
		else
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), acc->secure?5223:5222);
		
		entry = glade_xml_get_widget (glade, "connection_priority");
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), acc->priority);
		
		entry = glade_xml_get_widget (glade, "connection_resource");
		gtk_entry_set_text (GTK_ENTRY (entry), ISEMPTY (acc->resource));
		entry = glade_xml_get_widget (glade, "connection_use_ssl");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), acc->secure);

		entry = glade_xml_get_widget (glade, "proxy_enable");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), acc->proxy_use);
		setwidget (glade, "proxy_server", ISEMPTY (acc->proxy_server));
		entry = glade_xml_get_widget (glade, "proxy_port");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), acc->proxy_port);
		setwidget (glade, "proxy_username", ISEMPTY (acc->proxy_user));
		setwidget (glade, "proxy_passwd", ISEMPTY (acc->proxy_pass));
		
		entry = glade_xml_get_widget (glade, "connection_use_manual_server");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), acc->use_manual_host);
		setwidget (glade, "connection_manual_server", ISEMPTY (acc->manual_host));
	} else {
		/* Reste to defaults */
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		entry = glade_xml_get_widget (glade, "connection_server");
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		entry = glade_xml_get_widget (glade, "connection_password");
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		entry = glade_xml_get_widget (glade, "connection_save_password");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), FALSE);
		entry = glade_xml_get_widget (glade, "connection_port");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), 5222);
		entry = glade_xml_get_widget (glade, "connection_priority");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), 5);
		entry = glade_xml_get_widget (glade, "connection_resource");
		gtk_entry_set_text (GTK_ENTRY (entry), "kf");
		entry = glade_xml_get_widget (glade, "connection_use_ssl");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), FALSE);

		entry = glade_xml_get_widget (glade, "proxy_enable");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), FALSE);
		setwidget (glade, "proxy_server", ISEMPTY (""));
		entry = glade_xml_get_widget (glade, "proxy_port");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), 8080);
		setwidget (glade, "proxy_username", ISEMPTY (""));
		setwidget (glade, "proxy_passwd", ISEMPTY (""));
		
		entry = glade_xml_get_widget (glade, "connection_use_manual_server");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), FALSE);
		setwidget (glade, "connection_manual_server", "");
	}

	g_object_set_data (G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (entry))), "account", user_data);
}

void        on_connection_quit_clicked     (GtkButton *button,
                                            gpointer user_data)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


void        on_connection_zapisz_clicked   (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *glade = glade_get_widget_tree (GTK_WIDGET (button));
	KfPrefAccount *acc;
	
	if ((acc = g_object_get_data (G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (button))), "account")))
		kf_save_account (acc, glade);
	
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


void        on_connection_ok_clicked       (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *iface = glade_get_widget_tree (GTK_WIDGET (button));
	GtkWidget *entry;
	const gchar *server, *username, *password, *resource; 
	gint port, priority;
	KfPrefAccount *acc;
	GtkWidget *proxy_use;
	GtkWidget *proxy_port;

	GtkWidget *use_manual_host, *manual_host;

	entry = glade_xml_get_widget (iface, "connection_server");
	server = gtk_entry_get_text (GTK_ENTRY (entry));
	entry = glade_xml_get_widget (iface, "connection_username");
	username = gtk_entry_get_text (GTK_ENTRY (entry));
	entry = glade_xml_get_widget (iface, "connection_password");
	password = gtk_entry_get_text (GTK_ENTRY (entry));
	entry = glade_xml_get_widget (iface, "connection_resource");
	resource = gtk_entry_get_text (GTK_ENTRY (entry));
	entry = glade_xml_get_widget (iface, "connection_port");
	port = gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry));
	entry = glade_xml_get_widget (iface, "connection_priority");
	priority = gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry));

	if (! *password) {
		extern GladeXML *iface;
		GtkWidget *dialog;
		GtkWidget *passwd_entry;
		gint response;

		dialog = glade_xml_get_widget (iface, "passwd_dialog");
		passwd_entry = glade_xml_get_widget (iface, "passwd_dialog_entry");
		gtk_entry_set_text (GTK_ENTRY (passwd_entry), "");
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_hide (dialog);
		if (response == GTK_RESPONSE_OK) {
			password = gtk_entry_get_text (GTK_ENTRY (passwd_entry));
		} else {
			return;
		}
	}

	kf_jabber_set_connection (server, username, password, resource, port);
	kf_connection_set_priority (priority);
	
	proxy_use = glade_xml_get_widget (iface, "proxy_enable");
	proxy_port = glade_xml_get_widget (iface, "proxy_port");
	kf_connection_set_proxy (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (proxy_use))?
		value_of (iface, "proxy_server"):NULL,
		(gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (proxy_port)),
		value_of (iface, "proxy_username"), value_of (iface, "proxy_passwd"));
	
	use_manual_host = glade_xml_get_widget (iface, "connection_use_manual_server");
	manual_host = glade_xml_get_widget (iface, "connection_manual_server");
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (use_manual_host))) {
		kf_connection_set_manual_host (gtk_entry_get_text (GTK_ENTRY (manual_host)));
	}

	kf_jabber_connect();

	if ((acc = g_object_get_data (G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (button))), "account"))) {
		kf_save_account (acc, iface);
	}

	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


/* Zamkniêcie okna ustawieñ po³±czenia */
gboolean    on_connection_settings_window_delete_event (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data)
{
	gtk_widget_destroy (widget);
	return TRUE;
}


static void  kf_save_account               (KfPrefAccount *acc, GladeXML *glade) {
	GtkWidget *save_password;
	GtkWidget *use_ssl;
	GtkWidget *port;
	GtkWidget *priority;
	GtkWidget *proxy_use;
	GtkWidget *proxy_port;

	GtkWidget *use_manual_host, *manual_host;

	save_password = glade_xml_get_widget (glade, "connection_save_password");
	use_ssl = glade_xml_get_widget (glade, "connection_use_ssl");
	port = glade_xml_get_widget (glade, "connection_port");
	priority = glade_xml_get_widget (glade, "connection_priority");
	
	g_free (acc->uname); acc->uname = g_strdup (value_of (glade, "connection_username"));
	g_free (acc->server); acc->server = g_strdup (value_of (glade, "connection_server"));
	g_free (acc->pass); 
	if ((acc->save_password = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (save_password)))) {
		acc->pass = g_strdup (value_of (glade, "connection_password"));
	} else {
		acc->pass = NULL;
	}

	acc->port = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (port));
	acc->priority = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (priority));
	acc->secure = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (use_ssl));
	
	proxy_use = glade_xml_get_widget (glade, "proxy_enable");
	proxy_port = glade_xml_get_widget (glade, "proxy_port");
	
	kf_pref_account_set_proxy (acc, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (proxy_use)),
			value_of (glade, "proxy_server"),
			(gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (proxy_port)),
			value_of (glade, "proxy_username"), value_of (glade, "proxy_passwd"));

	g_free (acc->resource); acc->resource = g_strdup (value_of (glade, "connection_resource"));

	use_manual_host = glade_xml_get_widget (glade, "connection_use_manual_server");
	manual_host = glade_xml_get_widget (glade, "connection_manual_server");
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (use_manual_host))) {
		acc->use_manual_host = TRUE;
	}
	g_free (acc->manual_host);
	acc->manual_host = g_strdup (gtk_entry_get_text (GTK_ENTRY (manual_host)));
	
	/* Let's save config file */
	kf_preferences_write (kf_config_file ("config.xml"));
}

static const gchar *value_of (GladeXML *glade, const gchar *name) {
	GtkWidget *widget;

	widget = glade_xml_get_widget (glade, name);
	return gtk_entry_get_text (GTK_ENTRY (widget));
}


void        on_connection_dodaj_konto_clicked (GtkButton *button,
                                            gpointer user_data)
{
	GladeXML *glade = glade_get_widget_tree (GTK_WIDGET (button));
	GtkWidget *win, *parent;
	GtkWidget *label, *entry;
	gint response;

	parent = glade_xml_get_widget (glade, "connection_settings_window");
	win = gtk_dialog_new_with_buttons (_("Enter name..."), GTK_WINDOW (parent), GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  NULL);

	label = gtk_label_new (_("New account name:"));
	gtk_widget_show (label);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (win)->vbox), label);

	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), _("Unnamed"));
	gtk_widget_show (entry);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (win)->vbox), entry);
	
	response = gtk_dialog_run (GTK_DIALOG (win));
	if (response != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy (win);
	} else {
		KfPrefAccount *acc;
		const gchar *name;
		GtkWidget *item, *menu, *option;
		extern GList *kf_preferences_accounts;

		name = gtk_entry_get_text (GTK_ENTRY (entry));

		acc = kf_pref_account_new (name);

		option = glade_xml_get_widget (glade, "connection_konto_selector");
		menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option));

		item = gtk_menu_item_new_with_label (acc->name);
		gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, 1);
		gtk_widget_show (item);
		g_object_set_data (G_OBJECT (item), "glade", glade);
		gtk_option_menu_set_history (GTK_OPTION_MENU (option), 1);
		g_signal_connect (G_OBJECT (item), "activate",
				G_CALLBACK (kf_select_account), acc);

		g_object_set_data (G_OBJECT (parent), "account", acc);
		kf_preferences_accounts = g_list_prepend (kf_preferences_accounts, acc);

		gtk_widget_destroy (win);
	}
}

/* Accounts tab in Settings window */
void kf_accounts_editor (GladeXML *glade) {
	extern GList *kf_preferences_accounts;
	GList *tmp;

	GtkWidget *tv;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	tv = glade_xml_get_widget (glade, "accounts_treeview");
	if ((model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv)))) {
		store = GTK_LIST_STORE (model);
		gtk_list_store_clear (store);
	} else {
		store = gtk_list_store_new (1, G_TYPE_POINTER);

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                   renderer,
                                                   NULL);
		gtk_tree_view_column_set_cell_data_func (column,
				renderer,
				on_text_data,
				NULL, NULL);				
		gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Autoconnect"),
                                                   renderer,
                                                   NULL);
		gtk_tree_view_column_set_cell_data_func (column,
				renderer,
				on_toggle_data,
				NULL, NULL);				
		gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);
		g_signal_connect (G_OBJECT (renderer), "toggled",
				G_CALLBACK (on_autoconnect_toggled), store);

		gtk_tree_view_set_model (GTK_TREE_VIEW (tv), GTK_TREE_MODEL (store));
	}

	for (tmp = kf_preferences_accounts; tmp; tmp = tmp->next) {
		KfPrefAccount *acc = tmp->data;
		GtkTreeIter iter;

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
                    0, acc,
                    -1);

	}
}

/* This function displays a text in a TreeView */
static void
on_text_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
	KfPrefAccount *acc;
	
	gtk_tree_model_get (tree_model, iter, 0, &acc, -1);

	if (acc) {
		g_object_set (G_OBJECT (renderer), "text", acc->name, NULL);
	}

}

/* This function displays a toggle in a TreeView */
static void
on_toggle_data			            (GtkTreeViewColumn *tree_column,
                                             GtkCellRenderer *renderer,
                                             GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
	KfPrefAccount *acc;
	
	gtk_tree_model_get (tree_model, iter, 0, &acc, -1);

	if (acc) {
//		g_object_set (G_OBJECT (renderer), "toggled", acc->name, NULL);
		gtk_cell_renderer_toggle_set_active (GTK_CELL_RENDERER_TOGGLE (renderer),
				acc->autoconnect);
	}

}

/* Toggled callback */
static void on_autoconnect_toggled         (GtkCellRendererToggle *cellrenderertoggle,
                                            gchar *arg1,
                                            gpointer data) {
	GtkTreeModel *model = (GtkTreeModel *) data;
	GtkTreePath *path = gtk_tree_path_new_from_string (arg1);
	GtkTreeIter iter;
	KfPrefAccount *acc;

	/* TODO: to be continued */
	foo_debug ("toggled: %s, %d\n", arg1, gtk_cell_renderer_toggle_get_active (cellrenderertoggle));

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &acc, -1);
	if (acc) {
		if (! gtk_cell_renderer_toggle_get_active (cellrenderertoggle)) {
			kf_account_set_autoconnect (acc);
		} else {
			acc->autoconnect = FALSE;
		}
	}
	gtk_tree_path_free (path);
}

void kf_account_connect (KfPrefAccount *acc) {
	const gchar *password = acc->pass;

	if (! acc->save_password) {
		extern GladeXML *iface;
		GtkWidget *dialog;
		GtkWidget *passwd_entry;
		gint response;

		dialog = glade_xml_get_widget (iface, "passwd_dialog");
		passwd_entry = glade_xml_get_widget (iface, "passwd_dialog_entry");
		gtk_entry_set_text (GTK_ENTRY (passwd_entry), "");
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_hide (dialog);
		if (response == GTK_RESPONSE_OK) {
			password = gtk_entry_get_text (GTK_ENTRY (passwd_entry));
		} else {
			return;
		}
	}

	kf_jabber_set_connection (acc->server, acc->uname, password, acc->resource, acc->port);
	kf_connection_set_priority (acc->priority);
	if (acc->proxy_use) {
		kf_connection_set_proxy (acc->proxy_server, acc->proxy_port,
				acc->proxy_user, acc->proxy_pass);
	}

	kf_jabber_connect();

}

void        on_accounts_add_clicked        (GtkButton *button,
                                            gpointer user_data) {
	GladeXML *glade = glade_get_widget_tree (GTK_WIDGET (button));
	GtkWidget *win, *parent;
	GtkWidget *label, *entry;
	gint response;

	parent = gtk_widget_get_toplevel (GTK_WIDGET (button)); 
	win = gtk_dialog_new_with_buttons (_("Enter name..."), GTK_WINDOW (parent), GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  NULL);

	label = gtk_label_new (_("New account name:"));
	gtk_widget_show (label);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (win)->vbox), label);

	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), _("Unnamed"));
	gtk_widget_show (entry);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (win)->vbox), entry);
	
	response = gtk_dialog_run (GTK_DIALOG (win));
	if (response != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy (win);
	} else {
		KfPrefAccount *acc;
		const gchar *name;
//		GtkWidget *item, *menu, *option;
		extern GList *kf_preferences_accounts;

		name = gtk_entry_get_text (GTK_ENTRY (entry));

		acc = kf_pref_account_new (name);

		kf_preferences_accounts = g_list_prepend (kf_preferences_accounts, acc);

		kf_accounts_editor (glade); 
		gtk_widget_destroy (win);
	}
}

/* Opens a window with account options */
void        on_accounts_edit_clicked       (GtkButton *button,
                                            gpointer user_data) {
	KfPrefAccount *acc;
	if ((acc = get_selection (GTK_WIDGET (button)))) {
		GladeXML *glade;
		GtkWidget *cs; /* Connection Settings window */
		GtkWidget *button;
		GtkWidget *expander;
		GtkWidget *table;
		GtkWidget *frame;
		GtkWidget *use_manual_host, *manual_host;

		glade = glade_xml_new (kf_find_file ("kf.glade"), "connection_settings_window", NULL);
		glade_xml_signal_autoconnect (glade);

		hide (glade, "label5");
		hide (glade, "connection_konto_selector");
		hide (glade, "connection_dodaj_konto");
		hide (glade, "connection_ok");

		button = glade_xml_get_widget (glade, "connection_zapisz");
		gtk_widget_show (button);

		cs = glade_xml_get_widget (glade, "connection_settings_window");
		g_object_set_data_full (G_OBJECT (cs), "glade", glade, g_object_unref);
		kf_select_account (cs, acc);

		frame = glade_xml_get_widget (glade, "connection_frame");
		expander = cddb_disclosure_new (_("Show more options"),_("Hide more oprions"));
		g_signal_connect (G_OBJECT (expander), "toggled", G_CALLBACK (expander_toggled), frame);
		gtk_widget_show (expander);
	
		table = glade_xml_get_widget (glade, "table1");
		gtk_table_attach (GTK_TABLE (table), expander, 0, 3, 7, 8, GTK_FILL, 0, 0, 0);
	
		use_manual_host = glade_xml_get_widget (glade, "connection_use_manual_server");
		manual_host = glade_xml_get_widget (glade, "connection_manual_server");
#ifdef HAVE_LM_CONNECTION_SET_JID
		
#else
		gtk_widget_set_sensitive (use_manual_host, FALSE);
		gtk_widget_set_sensitive (manual_host, FALSE);
#endif
		kf_gui_show (cs);
	}
}

void        on_accounts_rename_clicked     (GtkButton *button,
                                            gpointer user_data) {
	KfPrefAccount *acc;
	if ((acc = get_selection (GTK_WIDGET (button)))) {
		GladeXML *glade = glade_get_widget_tree (GTK_WIDGET (button));
		GtkWidget *win, *parent;
		GtkWidget *label, *entry;
		gint response;

		parent = gtk_widget_get_toplevel (GTK_WIDGET (button)); 
		win = gtk_dialog_new_with_buttons (_("Enter name..."), GTK_WINDOW (parent), GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  NULL);

		label = gtk_label_new (_("New account name:"));
		gtk_widget_show (label);
		gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (win)->vbox), label);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), acc->name);
		gtk_widget_show (entry);
		gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (win)->vbox), entry);
	
		response = gtk_dialog_run (GTK_DIALOG (win));
		if (response != GTK_RESPONSE_ACCEPT) {
			gtk_widget_destroy (win);
		} else {
			g_free (acc->name);
			acc->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

			kf_accounts_editor (glade); 
			gtk_widget_destroy (win);
		}
	}
}
void        on_accounts_remove_clicked     (GtkButton *button,
                                            gpointer user_data) {
	GtkWidget *win;
	gint response;

	win = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			"Do you really want to remove this account?");
	response = gtk_dialog_run (GTK_DIALOG (win));
	gtk_widget_destroy (win);

	if (response == GTK_RESPONSE_YES) {
		KfPrefAccount *acc;

		acc = get_selection (GTK_WIDGET (button));
		if (acc) {
			extern GList *kf_preferences_accounts;

			kf_preferences_accounts = g_list_remove (kf_preferences_accounts, acc);
			kf_accounts_editor (glade_get_widget_tree (GTK_WIDGET (button))); 
		}
	}
}

static KfPrefAccount *get_selection (GtkWidget *widget) {
	GladeXML *glade = glade_get_widget_tree (widget);
	GtkWidget *tv;
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	
	tv = glade_xml_get_widget (glade, "accounts_treeview");
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
		KfPrefAccount *acc;
		gtk_tree_model_get (model, &iter, 0, &acc, -1);

		return acc;
	} else {
		return NULL;
	}

}

/* Hides a widget; internal */
static void hide (GladeXML *glade, const gchar *name) {
	GtkWidget *widget = glade_xml_get_widget (glade, name);
	gtk_widget_hide (widget);
}


static void expander_toggled (GtkToggleButton *expander, gpointer data) {
	GtkWidget *w = GTK_WIDGET (data);

	if (gtk_toggle_button_get_active (expander)) {
		gtk_widget_show (w);
	} else {
		gtk_widget_hide (w);
		gtk_window_resize (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (expander))), 1, 1);
	}
}
