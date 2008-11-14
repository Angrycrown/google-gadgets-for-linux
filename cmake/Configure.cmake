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

SET(PRODUCT_NAME google-gadgets)

PROJECT(google_gadgets_for_linux)

IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0002 OLD)
  CMAKE_POLICY(SET CMP0003 OLD)
  CMAKE_POLICY(SET CMP0005 OLD)
ENDIF(COMMAND CMAKE_POLICY)

SET(LIB_SUFFIX ""
  CACHE STRING "Define suffix of directory name (32/64)"
  FORCE)
SET(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}
  CACHE PATH  "Base directory for executables and libraries"
  FORCE)

# The following are directories where stuff will be installed to.
SET(BIN_INSTALL_DIR "${EXEC_INSTALL_PREFIX}/bin"
  CACHE PATH "The binary install dir (default prefix/bin)"
  FORCE)
SET(LIB_INSTALL_DIR "${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX}"
  CACHE PATH "The subdirectory relative to the install prefix where libraries will be installed (default is /lib${LIB_SUFFIX})"
  FORCE)
SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include/ggadget"
  CACHE PATH "The subdirectory to the header prefix"
  FORCE)

SET(GGL_OEM_BRAND ""
  CACHE STRING "The OEM brand code")

SET(GGL_DEFAULT_BROWSER_PLUGINS_DIR "${LIB_INSTALL_DIR}/firefox/plugins"
  CACHE PATH "The directory of browser plugins.")

SET(LIBGGADGET_BINARY_VERSION \"1.0.0\")
SET(GGL_MAJOR_VERSION 0)
SET(GGL_MINOR_VERSION 10)
SET(GGL_MICRO_VERSION 3)
SET(GGL_VERSION \"${GGL_MAJOR_VERSION}.${GGL_MINOR_VERSION}.${GGL_MICRO_VERSION}\")
SET(GGL_EPOCH "-1.0")

# The library version number is different than package version.
# The number should follow the rule defined by libtool and use the same number
# defined in configure.ac, eg.:
# LIBGGADGET_CURRENT, LIBGGADGET_REVISION and LIBGGADGET_AGE
# It's just a temporary value, because the API is not stable yet.
SET(GGL_LIB_VERSION "0.0.0")
SET(GGL_LIB_SOVERSION "0")

# This string is used in auto update request. It should be updated to the
# time of a release build is made. Its format is yymmdd-HHMMSS.
SET(GGL_VERSION_TIMESTAMP \"081107-000000\")

# Define the version of Google Desktop Gadget API that this release supports.
SET(GGL_API_MAJOR_VERSION 5)
SET(GGL_API_MINOR_VERSION 8)
SET(GGL_API_VERSION \"${GGL_API_MAJOR_VERSION}.${GGL_API_MINOR_VERSION}.0.0\")

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release")
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(GGL_PLATFORM_SHORT \"linux\")
SET(GGL_PLATFORM \"linux\")

INCLUDE(CheckFunctionExists)
INCLUDE(FindPkgConfig)

INCLUDE(CheckTypeSize)
SET(HAVE_STDDEF_H 1)
CHECK_TYPE_SIZE(char GGL_SIZEOF_CHAR)
CHECK_TYPE_SIZE(short GGL_SIZEOF_SHORT_INT)
CHECK_TYPE_SIZE(int GGL_SIZEOF_INT)
CHECK_TYPE_SIZE(long GGL_SIZEOF_LONG_INT)
CHECK_TYPE_SIZE(size_t GGL_SIZEOF_SIZE_T)
CHECK_TYPE_SIZE(double GGL_SIZEOF_DOUBLE)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/ggadget/sysdeps.h.in
               ${CMAKE_BINARY_DIR}/ggadget/sysdeps.h)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    AND EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)
  INCLUDE(Dart)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    AND EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)

INCLUDE(CopyUtils)
INCLUDE(GenerateOutput)
INCLUDE(PkgConfigEx)
INCLUDE(TestSuite)
INCLUDE(ZipUtils)

SET(GGL_PREFIX "${CMAKE_INSTALL_PREFIX}")
SET(GGL_EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
SET(GGL_LIBDIR "${LIB_INSTALL_DIR}")
SET(GGL_INCLUDEDIR "${GGL_PREFIX}/include")
SET(GGL_DATAROOTDIR "${GGL_PREFIX}/share")
SET(GGL_DATADIR "${GGL_DATAROOTDIR}")
SET(GGL_SYSCONFDIR "${GGL_PREFIX}/etc")

SET(GGL_MODULE_DIR "${LIB_INSTALL_DIR}/${PRODUCT_NAME}/modules")
SET(GGL_RESOURCE_DIR "${CMAKE_INSTALL_PREFIX}/share/${PRODUCT_NAME}")
SET(GGL_LIBEXEC_DIR "${LIB_INSTALL_DIR}/${PRODUCT_NAME}")
SET(GGL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include/${PRODUCT_NAME}")
SET(GGL_SYSDEPS_INCLUDE_DIR "${LIB_INSTALL_DIR}/${PRODUCT_NAME}/include")

ADD_DEFINITIONS(
  -DUNIX
  -DPREFIX=${CMAKE_INSTALL_PREFIX}
  -DPRODUCT_NAME=${PRODUCT_NAME}
  # For stdint.h macros like INT64_C etc.
  -D__STDC_CONSTANT_MACROS
  # TODO: only for Linux by now
  -DGGL_HOST_LINUX
  -DGGL_MODULE_DIR=\\\"${GGL_MODULE_DIR}\\\"
  -DGGL_RESOURCE_DIR=\\\"${GGL_RESOURCE_DIR}\\\"
  -DGGL_LIBEXEC_DIR=\\\"${GGL_LIBEXEC_DIR}\\\")

IF(GGL_OEM_BRAND)
  ADD_DEFINITIONS(-DGGL_OEM_BRAND=\"\\\"${GGL_OEM_BRAND}\\\"\")
ENDIF(GGL_OEM_BRAND)

IF(GGL_DEFAULT_BROWSER_PLUGINS_DIR)
  ADD_DEFINITIONS(-DGGL_DEFAULT_BROWSER_PLUGINS_DIR=\\\"${GGL_DEFAULT_BROWSER_PLUGINS_DIR}\\\")
ENDIF(GGL_DEFAULT_BROWSER_PLUGINS_DIR)


INCLUDE(CheckCCompilerFlag)
IF(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Werror -Wconversion")
  # No "-Wall -Werror" for C flags, to avoid third_party code break.
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  SET(PROJECT_RESOURCE_DIR share/${PRODUCT_NAME})
ELSE(UNIX)
  SET(PROJECT_RESOURCE_DIR resource)
ENDIF(UNIX)
# SET(CMAKE_SKIP_RPATH ON)
ENABLE_TESTING()

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    OR "${CMAKE_BUILD_TYPE}" STREQUAL "debugfull"
    OR "${CMAKE_BUILD_TYPE}" STREQUAL "debug")
  ADD_DEFINITIONS(-D_DEBUG)
  CHECK_C_COMPILER_FLAG(-fstack-protector-all GCC_STACK_PROTECTOR_ALL)
  IF(GCC_STACK_PROTECTOR_ALL)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")
  ENDIF(GCC_STACK_PROTECTOR_ALL)
ELSE("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    OR "${CMAKE_BUILD_TYPE}" STREQUAL "debugfull"
    OR "${CMAKE_BUILD_TYPE}" STREQUAL "debug")
  ADD_DEFINITIONS(-DNDEBUG)
  CHECK_C_COMPILER_FLAG(-fstack-protector GCC_STACK_PROTECTOR)
  IF(GCC_STACK_PROTECTOR)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector")
  ENDIF(GCC_STACK_PROTECTOR)
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    OR "${CMAKE_BUILD_TYPE}" STREQUAL "debugfull"
    OR "${CMAKE_BUILD_TYPE}" STREQUAL "debug")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

SET(GGL_BUILD_GTK_HOST 1)
SET(GGL_BUILD_QT_HOST 0)
SET(GGL_BUILD_LIBGGADGET_DBUS 1)
SET(GGL_BUILD_LIBGGADGET_GTK 1)
SET(GGL_BUILD_LIBGGADGET_QT 0)
SET(GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT 0)
SET(GGL_BUILD_QT_SCRIPT_RUNTIME 0)
SET(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT 1)
SET(GGL_BUILD_GST_AUDIO_FRAMEWORK 1)
SET(GGL_BUILD_GST_VIDEO_ELEMENT 1)
SET(GGL_BUILD_SMJS_SCRIPT_RUNTIME 1)
SET(GGL_BUILD_CURL_XML_HTTP_REQUEST 1)
SET(GGL_BUILD_LIBXML2_XML_PARSER 1)
SET(GGL_BUILD_LINUX_SYSTEM_FRAMEWORK 1)

# Check if flex is installed
FIND_PROGRAM(FLEX flex)
IF(NOT FLEX)
  MESSAGE(FATAL_ERROR "flex is needed to build ggl")
ENDIF(NOT FLEX)

# Check if zip is installed
FIND_PROGRAM(ZIP zip)
IF(NOT ZIP)
  MESSAGE(FATAL_ERROR "zip is needed to build ggl")
ENDIF(NOT ZIP)

# Check necessary libraries.

# Check if libltdl-dev is installed
FIND_FILE(LTDL ltdl.h)
IF(NOT LTDL)
  MESSAGE(FATAL_ERROR "libltdl development package is needed to build ggl")
ENDIF(NOT LTDL)

FIND_PACKAGE(ZLIB REQUIRED)

FIND_PACKAGE(Threads)
IF(CMAKE_USE_PTHREADS_INIT)
  SET(PTHREAD_FOUND 1)
  SET(PTHREAD_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
  ADD_DEFINITIONS(-DHAVE_PTHREAD=1)
ENDIF(CMAKE_USE_PTHREADS_INIT)

FIND_PACKAGE(X11)
IF(X11_FOUND)
  GET_CONFIG(xt 1.0 XT XT_FOUND)
  ADD_DEFINITIONS(-DHAVE_X11=1)
ENDIF(X11_FOUND)

GET_CONFIG(libxml-2.0 2.6.0 LIBXML2 LIBXML2_FOUND)
IF(NOT LIBXML2_FOUND AND GGL_BUILD_LIBXML2_XML_PARSER)
  MESSAGE("Library libxml2 is not available, libxml2-xml-parser extension won't be built.")
  SET(GGL_BUILD_LIBXML2_XML_PARSER 0)
ENDIF(NOT LIBXML2_FOUND AND GGL_BUILD_LIBXML2_XML_PARSER)

GET_CONFIG(libcurl 7.15 LIBCURL LIBCURL_FOUND)
IF(NOT LIBCURL_FOUND)
  IF(GGL_BUILD_CURL_XML_HTTP_REQUEST)
    MESSAGE("Library curl is not available, curl-xml-http-request extension won't be built.")
    SET(GGL_BUILD_CURL_XML_HTTP_REQUEST 0)
  ENDIF(GGL_BUILD_CURL_XML_HTTP_REQUEST)
ENDIF(NOT LIBCURL_FOUND)

GET_CONFIG(gstreamer-0.10 0.10.0 GSTREAMER GSTREAMER_FOUND)
GET_CONFIG(gstreamer-plugins-base-0.10 0.10.0 GSTREAMER_PLUGINS_BASE GSTREAMER_PLUGINS_BASE_FOUND)
IF(GSTREAMER_FOUND AND GSTREAMER_PLUGINS_BASE_FOUND)
  FIND_LIBRARY(VIDEOSINK_LIBRARY NAMES gstvideo-0.10 PATH /usr/lib /usr/lib/gstreamer-0.10 /usr/local/lib /usr/local/lib/gstreamer-0.10)
  IF(VIDEOSINK_LIBRARY)
    SET(GSTREAMER_VIDEO_LIBRARIES "${GSTREAMER_LIBRARIES}  -lgstvideo-0.10")
  ELSE(VIDEOSINK_LIBRARY)
    MESSAGE("Libgstreamer-plugins-base-dev is not available, gst-video-element extension won't be built.")
    SET(GGL_BUILD_GST_VIDEO_ELEMENT 0)
  ENDIF(VIDEOSINK_LIBRARY)
ELSE(GSTREAMER_FOUND AND GSTREAMER_PLUGINS_BASE_FOUND)
  IF(GGL_BUILD_GST_AUDIO_FRAMEWORK)
    MESSAGE("Library gstreamer-plugins-base(>=0.10.0) is not available, gst-audio-framework extension won't be built.")
    SET(GGL_BUILD_GST_AUDIO_FRAMEWORK 0)
  ENDIF(GGL_BUILD_GST_AUDIO_FRAMEWORK)
  IF(GGL_BUILD_GST_VIDEO_ELEMENT)
    MESSAGE("Library gstreamer-plugins-base(>=0.10.0) is not available, gst-video-element extension won't be built.")
    SET(GGL_BUILD_GST_VIDEO_ELEMENT 0)
  ENDIF(GGL_BUILD_GST_VIDEO_ELEMENT)
ENDIF(GSTREAMER_FOUND AND GSTREAMER_PLUGINS_BASE_FOUND)

GET_CONFIG(pango 1.10.0 PANGO PANGO_FOUND)
GET_CONFIG(cairo 1.0.0 CAIRO CAIRO_FOUND)
GET_CONFIG(gdk-2.0 2.8.0 GDK2 GDK2_FOUND)
GET_CONFIG(gtk+-2.0 2.8.0 GTK2 GTK2_FOUND)
GET_CONFIG(gthread-2.0 2.8.0 GTHREAD GTHREAD_FOUND)
IF(NOT PANGO_FOUND OR NOT CAIRO_FOUND OR NOT GDK2_FOUND OR NOT GTK2_FOUND OR NOT GTHREAD_FOUND)
  SET(GGL_BUILD_LIBGGADGET_GTK 0)
  SET(GGL_BUILD_GTK_HOST 0)
  MESSAGE("Library cairo, pando or gtk-2.0 are not available, gtk-system-framework extension, libggadget-gtk, gtk host won't be built.")
ENDIF(NOT PANGO_FOUND OR NOT CAIRO_FOUND OR NOT GDK2_FOUND OR NOT GTK2_FOUND OR NOT GTHREAD_FOUND)

GET_CONFIG(librsvg-2.0 2.14 RSVG RSVG_FOUND)
IF(RSVG_FOUND)
  LIST(APPEND RSVG_DEFINITIONS "-DHAVE_RSVG_LIBRARY")
ENDIF(RSVG_FOUND)

# Check Qt

SET(QT_MIN_VERSION 4.3.0)
SET(QT_USE_QTWEBKIT 1)
SET(QT_USE_QTNETWORK 1)
FIND_PACKAGE(Qt4)

IF(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND)
  INCLUDE(${QT_USE_FILE})
  SET(QT_LINK_DIR ${QT_LIBRARY_DIR})
  SET(QT_LIBRARIES ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
  SET(GGL_BUILD_LIBGGADGET_QT 1)
  SET(GGL_BUILD_QT_HOST 1)

  PKG_CHECK_MODULES(QT_QTSCRIPT QtScript>=4.4.0)
  IF(QT_QTSCRIPT_FOUND)
    LIST(APPEND QT_LIBRARIES ${QT_QTSCRIPT_LIBRARIES})
    SET(GGL_BUILD_QT_SCRIPT_RUNTIME 1)
  ELSE(QT_QTSCRIPT_FOUND)
    MESSAGE("Qt 4.4 or above is not available, QtScriptRuntime won't be built.")
  ENDIF(QT_QTSCRIPT_FOUND)

  IF(NOT QT_QTWEBKIT_FOUND)
    PKG_CHECK_MODULES(QT_QTWEBKIT QtWebKit)
    IF(QT_QTWEBKIT_FOUND)
      SET(QT_QTWEBKIT_LIBRARY ${QT_QTWEBKIT_LIBRARIES})
    ENDIF(QT_QTWEBKIT_FOUND)
  ENDIF(NOT QT_QTWEBKIT_FOUND)

  IF(QT_QTWEBKIT_FOUND)
    LIST(APPEND QT_LIBRARIES ${QT_QTWEBKIT_LIBRARIES})
    INCLUDE_DIRECTORIES(${QT_QTWEBKIT_INCLUDE_DIRS})
    SET(GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT 1)
  ELSE(QT_QTWEBKIT_FOUND)
    MESSAGE("QtWebKit is needed to build qtwebkit_browser_element")
  ENDIF(QT_QTWEBKIT_FOUND)
ELSE(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND)
  MESSAGE("Library qt-4.3 or above is not available, libggadget-qt, qt host and qt related extensions won't be built.")
ENDIF(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND)

GET_CONFIG(dbus-1 1.0 DBUS DBUS_FOUND)
IF(NOT DBUS_FOUND AND GGL_BUILD_LIBGGADGET_DBUS)
  SET(GGL_BUILD_LIBGGADGET_DBUS 0)
  MESSAGE("Library D-Bus is not available, libggadget-dbus won't be built.")
ELSE(NOT DBUS_FOUND AND GGL_BUILD_LIBGGADGET_DBUS)
  LIST(APPEND DBUS_DEFINITIONS "-DHAVE_DBUS_LIBRARY")
  SET(CMAKE_REQUIRED_INCLUDES ${DBUS_INCLUDE_DIR})
  SET(CMAKE_REQUIRED_LIBRARIES ${DBUS_LIBRARIES})
  CHECK_FUNCTION_EXISTS(dbus_watch_get_unix_fd DBUS_UNIX_FD_FUNC_FOUND)
  IF(DBUS_UNIX_FD_FUNC_FOUND)
    LIST(APPEND DBUS_DEFINITIONS "-DHAVE_DBUS_WATCH_GET_UNIX_FD")
  ENDIF(DBUS_UNIX_FD_FUNC_FOUND)
ENDIF(NOT DBUS_FOUND AND GGL_BUILD_LIBGGADGET_DBUS)

GET_CONFIG(NetworkManager 0.7 NETWORK_MANAGER NETWORK_MANAGER_FOUND)
IF(NETWORK_MANAGER_FOUND)
  SET(GGL_HAVE_NETWORK_MANAGER 1)
  LIST(APPEND NETWORK_MANAGER_DEFINITIONS
    "-DHAVE_NETWORK_MANAGER" "-DHAVE_NEW_NETWORK_MANAGER_API")
ELSE(NETWORK_MANAGER_FOUND)
  GET_CONFIG(NetworkManager 0.6 NETWORK_MANAGER NETWORK_MANAGER_OLD_FOUND)
  IF(NETWORK_MANAGER_OLD_FOUND)
    SET(GGL_HAVE_NETWORK_MANAGER 1)
    LIST(APPEND NETWORK_MANAGER_DEFINITIONS "-DHAVE_NETWORK_MANAGER")
  ENDIF(NETWORK_MANAGER_OLD_FOUND)
ENDIF(NETWORK_MANAGER_FOUND)

# Check gtkmozembed (Please refer to configure.ac)
# Check priority:
#  xulrunner 1.9: libxul-embedding-unstable and mozilla-js
#  xulrunner 1.8: xulrunner-gtkmozembed and xulrunner-js
#  firefox 2.0: firefox2-gtkmozembed and firefox2-js
#  firefox 1.5: firefox-gtkmozembed and firefox-js
#
# Add more rules here if your distribution doesn't support above rules.
MACRO(HASH_PUT _hash _key _value)
  LIST(APPEND ${_hash} ${_key})
  SET(__${_hash}_${_key}__ ${_value})
ENDMACRO(HASH_PUT _hash _key _value)

MACRO(HASH_GET _hash _key _value)
  SET(${_value} ${__${_hash}_${_key}__})
ENDMACRO(HASH_GET _hash _key _value)

# We need to check xul if smjs runtime is to be built because we prefer
# to using smjs through glue mechanism provided by xul
IF(GGL_BUILD_LIBGGADGET_GTK OR GGL_BUILD_SMJS_SCRIPT_RUNTIME)
  HASH_PUT(xul_hash libxul-embedding-unstable mozilla-js)
  HASH_PUT(xul_hash libxul-embedding mozilla-js)
  HASH_PUT(xul_hash xulrunner-gtkmozembed xulrunner-js)
  HASH_PUT(xul_hash firefox2-gtkmozembed firefox2-js)
  HASH_PUT(xul_hash firefox-gtkmozembed firefox-js)

  FOREACH(pkg ${xul_hash})
    IF(NOT GTKMOZEMBED_FOUND)
      GET_CONFIG(${pkg} 0 GTKMOZEMBED GTKMOZEMBED_FOUND)
      IF(GTKMOZEMBED_FOUND)
        PKG_GET_VARIABLE(${pkg} "includedir" EMBED_INCDIR)
        LIST(APPEND GTKMOZEMBED_INCLUDE_DIR
          ${EMBED_INCDIR}
          ${EMBED_INCDIR}/content
          ${EMBED_INCDIR}/dom
          ${EMBED_INCDIR}/xpconnect
          ${EMBED_INCDIR}/widget
          ${EMBED_INCDIR}/gtkembedmoz
          ${EMBED_INCDIR}/xpcom)
        HASH_GET(xul_hash "${pkg}" LIBSMJS)
        GET_CONFIG(${LIBSMJS} 0 SMJS SMJS_FOUND)
        IF(SMJS_FOUND)
          SET(SMJS_NAME ${LIBSMJS})
        ENDIF(SMJS_FOUND)
      ENDIF(GTKMOZEMBED_FOUND)
    ENDIF(NOT GTKMOZEMBED_FOUND)
  ENDFOREACH(pkg ${PKG_NAMES})

  IF(NOT GTKMOZEMBED_FOUND AND GGL_BUILD_GTKMOZ_BROWSER_ELEMENT)
    SET(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT 0)
    MESSAGE("Library GtkMozEmbed is not available, gtkmoz-browser-element extension won't be built.")
  ENDIF(NOT GTKMOZEMBED_FOUND AND GGL_BUILD_GTKMOZ_BROWSER_ELEMENT)

ENDIF(GGL_BUILD_LIBGGADGET_GTK OR GGL_BUILD_SMJS_SCRIPT_RUNTIME)

IF(NOT SMJS_FOUND)
  SET(SMJS_LIST
    mozilla-js
    xulrunner-js
    firefox2-js
    firefox-js
  )
  FOREACH(js ${SMJS_LIST})
    IF(NOT SMJS_FOUND)
      GET_CONFIG(${js} 0 SMJS SMJS_FOUND)
      IF(SMJS_FOUND)
        SET(SMJS_NAME ${LIBSMJS})
      ENDIF(SMJS_FOUND)
    ENDIF(NOT SMJS_FOUND)
  ENDFOREACH(js ${SMJS_LIST})
ENDIF(NOT SMJS_FOUND)

# Do more test with spidermonkey
INCLUDE(SpiderMonkey)

# Hell, some distributions' package of smjs is broken. We have to fix it here
IF(SMJS_FOUND)
  STRING(REGEX MATCH "/stable" RESULT "${SMJS_INCLUDE_DIR}")
  IF("${RESULT}" STREQUAL "/stable")
    PKG_GET_VARIABLE(${SMJS_NAME} "includedir" SMJS_INC)
    FIND_FILE(JSAPI_PATH jsapi.h "${SMJS_INC}/unstable")
    IF(JSAPI_PATH)
      LIST(APPEND SMJS_INCLUDE_DIR "${SMJS_INC}/unstable")
    ENDIF(JSAPI_PATH)
  ENDIF("${RESULT}" STREQUAL "/stable")
ENDIF(SMJS_FOUND)

IF(SMJS_FOUND AND GTKMOZEMBED_FOUND)
  STRING(REGEX MATCH "XPCOM_GLUE" RESULT "${GTKMOZEMBED_DEFINITIONS}")
  IF("${RESULT}" STREQUAL "XPCOM_GLUE")
    MESSAGE("Using XPCOM_GLUE for SMJS")
    LIST(APPEND SMJS_DEFINITIONS ${GTKMOZEMBED_DEFINITIONS})
    LIST(APPEND SMJS_INCLUDE_DIR ${GTKMOZEMBED_INCLUDE_DIR})
    SET(SMJS_LIBRARIES ${GTKMOZEMBED_LIBRARIES})
    SET(SMJS_LINK_DIR ${GTKMOZEMBED_LINK_DIR})
  ENDIF("${RESULT}" STREQUAL "XPCOM_GLUE")
ENDIF(SMJS_FOUND AND GTKMOZEMBED_FOUND)

IF(NOT SMJS_FOUND AND GGL_BUILD_SMJS_SCRIPT_RUNTIME)
  SET(GGL_BUILD_SMJS_SCRIPT_RUNTIME 0)
  MESSAGE("Library SpiderMonkey is not available, smjs-script-runtime extension won't be built.")
ENDIF(NOT SMJS_FOUND AND GGL_BUILD_SMJS_SCRIPT_RUNTIME)

MESSAGE("
Build options:
  Version                          ${GGL_VERSION}
  Build type                       ${CMAKE_BUILD_TYPE}
  OEM brand                        ${GGL_OEM_BRAND}

 Libraries:
  GTK SVG Support                  ${RSVG_FOUND}
  Build libggadget-gtk             ${GGL_BUILD_LIBGGADGET_GTK}
  Build libggadget-qt              ${GGL_BUILD_LIBGGADGET_QT}
  Build libggadget-dbus            ${GGL_BUILD_LIBGGADGET_DBUS}

 Extensions:
  Build dbus-script-class          ${GGL_BUILD_LIBGGADGET_DBUS}
  Build gtkmoz-browser-element     ${GGL_BUILD_GTKMOZ_BROWSER_ELEMENT}
  Build gst-audio-framework        ${GGL_BUILD_GST_AUDIO_FRAMEWORK}
  Build gst-video-element          ${GGL_BUILD_GST_VIDEO_ELEMENT}
  Build linux-system-framework     ${GGL_BUILD_LINUX_SYSTEM_FRAMEWORK}
  Build smjs-script-runtime        ${GGL_BUILD_SMJS_SCRIPT_RUNTIME}
  Build curl-xml-http-request      ${GGL_BUILD_CURL_XML_HTTP_REQUEST}
  Build libxml2-xml-parser         ${GGL_BUILD_LIBXML2_XML_PARSER}
  Build gtk-edit-element           ${GGL_BUILD_LIBGGADGET_GTK}
  Build gtk-system-framework       ${GGL_BUILD_LIBGGADGET_GTK}
  Build qt-edit-element            ${GGL_BUILD_LIBGGADGET_QT}
  Build qtwebkit-browser-element   ${GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT}
  Build qt-xml-http-request        ${GGL_BUILD_LIBGGADGET_QT}
  Build qt-script-runtime          ${GGL_BUILD_QT_SCRIPT_RUNTIME}
  Build qt-system-framework        ${GGL_BUILD_LIBGGADGET_QT}

 Hosts:
  Build gtk host                   ${GGL_BUILD_GTK_HOST}
  Build qt host                    ${GGL_BUILD_QT_HOST}
")
