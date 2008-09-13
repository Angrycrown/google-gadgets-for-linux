#
# spec file for package google-gadgets (Version 0.10.1)
#
# Copyright 2008 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# norootforbuild

Name:           google-gadgets
Version:        0.10.1
Release:        1
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        Google Gadgets for Linux
Url:            http://code.google.com/p/google-gadgets-for-linux/
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  gcc-c++ zip autoconf update-desktop-files flex libtool
BuildRequires:  gtk2-devel >= 2.10.0
BuildRequires:  cairo-devel >= 1.2.0
BuildRequires:  libcurl-devel >= 7.15.0
BuildRequires:  libxml2-devel >= 2.6.0
BuildRequires:  zlib-devel >= 1.2.0
BuildRequires:  mozilla-xulrunner190-devel
BuildRequires:  gstreamer-0_10-devel
BuildRequires:  gstreamer-0_10-plugins-base-devel
BuildRequires:  libQtWebKit-devel >= 4.4.0
BuildRequires:  dbus-1-devel >= 1.0.2
BuildRequires:  libqt4-devel >= 4.4.0
Source0:        http://google-gadgets-for-linux.googlecode.com/files/google-gadgets-for-linux-%{version}.tar.bz2

%debug_package

%description
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-1_0-0
License:        Apache License Version 2.0
Group:          System/Libraries
Summary:        Google Gadgets main libraries
Requires:       libltdl
Requires:       dbus-1 >= 1.0.2

%description -n libggadget-1_0-0
This package contains the main Google Gadgets libraries, it is required by both
the GTK+ and QT versions of Google Gadgets.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-devel
License:        Apache License Version 2.0
Group:          Development/Libraries/C and C++
Summary:        Google Gadgets main development files

%description -n libggadget-devel
This package contains the development files assoicated with libggadget, it is
needed to write programs that utilise libggadget.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-gtk-1_0-0
License:        Apache License Version 2.0
Group:          System/Libraries
Summary:        Google Gadgets GTK+ library
Requires:       libggadget-1_0-0 = %{version}
Requires:       gtk2 >= 2.10.0
Requires:       cairo >= 1.2.0

%description -n libggadget-gtk-1_0-0
This package contains the GTK+ Google Gadgets library, it is required to run
the GTK+ version of Google Gadgets.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-gtk-devel
License:        Apache License Version 2.0
Group:          Development/Libraries/C and C++
Summary:        Google Gadgets GTK+ development files
Requires:       libggadget-devel = %{version}
Requires:       gtk2-devel >= 2.10.0
Requires:       cairo-devel >= 1.2.0

%description -n libggadget-gtk-devel
This package contains the development files assoicated with libggadget-gtk,
it is needed to write GTK+ programs that utilise libggadget.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-qt-1_0-0
License:        Apache License Version 2.0
Group:          System/Libraries
Summary:        Google Gadgets QT library
Requires:       libggadget-1_0-0 = %{version}
Requires:       libqt4 >= 4.4.0
Requires:       libQtWebKit4 >= 4.4.0

%description -n libggadget-qt-1_0-0
This package contains the QT Google Gadgets library, it is required to run
the QT version of Google Gadgets.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-qt-devel
License:        Apache License Version 2.0
Group:          Development/Libraries/C and C++
Summary:        Google Gadgets QT development files
Requires:       libggadget-devel = %{version}
Requires:       libqt4-devel >= 4.4.0
Requires:       libQtWebKit4-devel >= 4.4.0

%description -n libggadget-qt-devel
This package contains the development files assoicated with libggadget-qt,
it is needed to write QT programs that utilise libggadget.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>

%package common
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        Common files for QT and GTK+ versions of google-gadgets
Requires:       libggadget-1_0-0 = %{version}

%description common
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes files common to both GTK+ and QT versions.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package gtk
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        GTK+ Version of Google Gadgets
Requires:       libggadget-gtk-1_0-0 = %{version}
Requires:       google-gadgets-common = %{version}
Requires:       google-gadgets-gst = %{version}
Requires:       google-gadgets-xul = %{version}

%description gtk
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes the GTK+ version.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package qt
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        QT Version of Google Gadgets
Requires:       libggadget-qt-1_0-0 = %{version}
Requires:       google-gadgets-common = %{version}
Requires:       google-gadgets-gst = %{version}

%description qt
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes the QT version.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package gst
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        GStreamer modules for Google Gadgets
Requires:       libggadget-1_0-0 = %{version}
Requires:       gstreamer-0_10-plugins-base

%description gst
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes the GStreamer modules.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package xul
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        XULRunner modules for Google Gadgets
Requires:       libggadget-1_0-0 = %{version}
Requires:       mozilla-xulrunner190

%description xul
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes the XULRunner modules.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>

%prep
%setup -q -n google-gadgets-for-linux-%{version}

%build
%{suse_update_config -f}
autoreconf
%configure --disable-werror
make %{?jobs:-j%jobs}

%install
%makeinstall
# these are dynamic modules... we shouldn't be installing them
rm -f $RPM_BUILD_ROOT/%{_libdir}/google-gadgets/modules/*.la
rm -f $RPM_BUILD_ROOT/%{_libdir}/google-gadgets/modules/*.a
%suse_update_desktop_file ggl-gtk Network WebBrowser
%suse_update_desktop_file ggl-qt Network WebBrowser

MD5SUM=$(md5sum COPYING | sed 's/ .*//')
if test -f /usr/share/doc/licenses/md5/$MD5SUM ; then
  ln -sf /usr/share/doc/licenses/md5/$MD5SUM COPYING
fi


%post -n google-gadgets-common
if [ -f /usr/bin/update-mime-database ]; then
  /usr/bin/update-mime-database /usr/share/mime > /dev/null
fi
if [ -f /usr/bin/xdg-icon-resource ]; then
  /usr/bin/xdg-icon-resource forceupdate --theme hicolor > /dev/null
fi

%postun -n google-gadgets-common
if [ -f /usr/bin/update-mime-database ]; then
  /usr/bin/update-mime-database %{_datadir}/mime > /dev/null
fi
if [ -f /usr/bin/xdg-icon-resource ]; then
  /usr/bin/xdg-icon-resource forceupdate --theme hicolor > /dev/null
fi

%post -n libggadget-1_0-0 -p /sbin/ldconfig
%postun -n libggadget-1_0-0 -p /sbin/ldconfig

%post -n libggadget-gtk-1_0-0 -p /sbin/ldconfig
%postun -n libggadget-gtk-1_0-0 -p /sbin/ldconfig

%post -n libggadget-qt-1_0-0 -p /sbin/ldconfig
%postun -n libggadget-qt-1_0-0 -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files -n google-gadgets-common
%defattr(-, root, root)
%doc COPYING AUTHORS README NEWS
%dir %{_libdir}/google-gadgets/
%dir %{_libdir}/google-gadgets/modules/
%{_libdir}/google-gadgets/modules/default*.so
%{_libdir}/google-gadgets/modules/linux*.so
%{_libdir}/google-gadgets/modules/google-gadget-manager.so
%{_libdir}/google-gadgets/modules/libxml2*.so
%{_libdir}/google-gadgets/modules/curl*.so
%{_libdir}/google-gadgets/modules/dbus*.so
%dir %{_datadir}/google-gadgets/
%{_datadir}/google-gadgets/*.gg
%{_datadir}/mime/packages/*.xml
%{_datadir}/pixmaps/*.png
%{_datadir}/icons/hicolor/*/apps/*.png
%{_datadir}/icons/hicolor/*/mimetypes/*.png

%files -n libggadget-1_0-0
%defattr(-, root, root)
%{_libdir}/libggadget-1.0*.so.*
%{_libdir}/libggadget-dbus-1.0*.so.*
%{_libdir}/libggadget-js-1.0*.so.*
%{_libdir}/libggadget-xdg-1.0*.so.*

%files -n libggadget-devel
%defattr(-, root, root)
%dir %{_includedir}/google-gadgets/
%dir %{_includedir}/google-gadgets/ggadget
%{_includedir}/google-gadgets/ggadget/*.h
%dir %{_libdir}/google-gadgets/include/
%dir %{_libdir}/google-gadgets/include/ggadget/
%{_libdir}/google-gadgets/include/ggadget/sysdeps.h
%{_libdir}/libggadget-1.0*.so
%{_libdir}/libggadget-dbus-1.0*.so
%{_libdir}/libggadget-js-1.0*.so
%{_libdir}/libggadget-xdg-1.0*.so
%{_libdir}/libggadget-1.0*.a
%{_libdir}/libggadget-dbus-1.0*.a
%{_libdir}/libggadget-js-1.0*.a
%{_libdir}/libggadget-xdg-1.0*.a
%{_libdir}/libggadget-1.0*.la
%{_libdir}/libggadget-dbus-1.0*.la
%{_libdir}/libggadget-js-1.0*.la
%{_libdir}/libggadget-xdg-1.0*.la
%{_libdir}/pkgconfig/libggadget-1.0.pc
%{_libdir}/pkgconfig/libggadget-dbus-1.0.pc
%{_libdir}/pkgconfig/libggadget-js-1.0.pc
%{_libdir}/pkgconfig/libggadget-xdg-1.0.pc

%files -n libggadget-gtk-1_0-0
%defattr(-, root, root)
%{_libdir}/libggadget-gtk-1.0*.so.*

%files -n libggadget-gtk-devel
%defattr(-, root, root)
%dir %{_includedir}/google-gadgets/ggadget/gtk/
%{_includedir}/google-gadgets/ggadget/gtk/*.h
%{_libdir}/libggadget-gtk-1.0*.so
%{_libdir}/libggadget-gtk-1.0*.a
%{_libdir}/libggadget-gtk-1.0*.la
%{_libdir}/pkgconfig/libggadget-gtk-1.0.pc

%files -n libggadget-qt-1_0-0
%defattr(-, root, root)
%{_libdir}/libggadget-qt-1.0*.so.*

%files -n libggadget-qt-devel
%defattr(-, root, root)
%dir %{_includedir}/google-gadgets/ggadget/qt/
%{_includedir}/google-gadgets/ggadget/qt/*.h
%{_libdir}/libggadget-qt-1.0*.so
%{_libdir}/libggadget-qt-1.0*.a
%{_libdir}/libggadget-qt-1.0*.la
%{_libdir}/pkgconfig/libggadget-qt-1.0.pc

%files -n google-gadgets-gtk
%defattr(-, root, root)
%{_bindir}/ggl-gtk
%{_datadir}/applications/ggl-gtk.desktop
%{_libdir}/google-gadgets/modules/gtk-*.so

%files -n google-gadgets-qt
%defattr(-, root, root)
%{_bindir}/ggl-qt
%{_datadir}/applications/ggl-qt.desktop
%{_libdir}/google-gadgets/modules/qt*.so

%files -n google-gadgets-gst
%defattr(-, root, root)
%{_libdir}/google-gadgets/modules/gst*.so

%files -n google-gadgets-xul
%defattr(-, root, root)
%{_libdir}/google-gadgets/modules/smjs*.so
%{_libdir}/google-gadgets/modules/gtkmoz*.so
%{_libdir}/google-gadgets/gtkmoz-browser-child

%changelog
* Thu Sep 11 2008 james.su@gmail.com
- Initial release