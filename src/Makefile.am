
INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = kf

kf_SOURCES = \
	main.c \
	accounts.c accounts.h \
	archive.c archive.h \
	archive_viewer.c archive_viewer.h \
	autoaway.c autoaway.h \
	change_password.c change_password.h \
	connection.c connection.h \
	contact_xchange.c contact_xchange.h \
	disclosure-widget.c disclosure-widget.h \
	dock.c dock.h \
	eggtrayicon.c eggtrayicon.h \
	events.c events.h \
	filter.c filter.h \
	foogc.c foogc.h \
	gtktip.c gtktip.h \
	jabber.c jabber.h \
	jabber_connection.c \
	jabber_msg.c jabber_msg.h \
	jabber_roster.c \
	jisp.c jisp.h \
	jispman.c jispman.h \
	kf.c kf.h \
	gui.c gui.h \
	callbacks.c callbacks.h \
	message.c message.h \
	muc.c muc.h muc_private.h \
	muc_join.c muc_join.h \
	new_account.c new_account.h \
	new_message.c new_message.h \
	gui_status.c \
	popup_group.c \
	popup_menu.c popup_menu.h \
	prefix.c prefix.h \
	privacy.c privacy.h \
	privacy_list.c privacy_list.h \
	chat.c chat.h \
	gui_pixmap.c \
	preferences.c preferences.h\
	subscribe.c subscribe.h \
	emoticons.c emoticons.h \
	browse.c browse.h \
	search.c search.h \
	settings.c settings.h \
	settings_blacklist.c settings_blacklist.h \
	settings_icons.c settings_icons.h \
	settings_statuses.c settings_statuses.h \
	sound.c sound.h \
	statusbar.c statusbar.h \
	register.c register.h\
	vcard.c vcard.h \
	www.c www.h \
	x_data.c x_data.h \
	xevent.c xevent.h

#kf_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)
AM_CFLAGS = @CFLAGS@ -Wall -Wmissing-declarations $(BINRELOC_CFLAGS)
AM_LDFLAGS = @LDFLAGS@ -g -Wl,--export-dynamic
kf_LDADD = @PACKAGE_LIBS@

