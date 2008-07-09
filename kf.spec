%define name kf
%define version 0.1.8
%define release 1mdk

Name: %{name}
Version: %{version}
Release: %{release}
Summary: A GTK+ based Jabber IM client

Source0: %{name}-%{version}.tar.gz
Source1: %{name}-%{version}.tar.gz
#Source1: %name-icons.tar.bz2
URL: http://www.habazie.rams.pl/kf
Packager: Przemys³aw Sitek <psitek@rams.pl>
Group: Applications/Internet
BuildRoot: %{_tmppath}/%{name}-buildroot
License: GPL
Requires: gtk2 >= 2.0.0
#BuildRequires: gtk2-devel >= 2.0.4
#BuildRequires: libglade2-devel >= 2.0.0


%description
kf linux jabber client is a simple but powerful Jabber client.


%prep
rm -rf $RPM_BUILD_ROOT
%setup -q
%setup -q -T -D -a1
%build
%configure
%make
%install

mkdir -p %RPM_BUILD_ROOT%_menudir
cat > %RPM_BUILD_ROOT%_menudir/%name << EOF

?package(%name): \
	command="%_bindir/%name" \
	needs="x11" \
	icon="%name.png" \
	section="Network/Instant messaging" \
	title="kf linux jabber client" \
	longtitle="%summary"

EOF


%__install -D -m 644 %{name}48.png %buildroot/%_liconsdir/%name.png
%__install -D -m 644 %{name}32.png %buildroot/%_iconsdir/%name.png
%__install -D -m 644 %{name}16.png %buildroot/%_miconsdir/%name.png

rm -rf $RPM_BUILD_ROOT
%makeinstall

%post
%update_menus
%postun
%{clean_menus}

%files
%defattr(-,root,root,0755)
%doc README NEWS COPYING AUTHORS
%{_bindir}/kf
%{_datadir}/kf/*

%changelog
* Sat Nov 13 2004 Przemys³aw Sitek <psitek@rams.pl>
- Initial build
