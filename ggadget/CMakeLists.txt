#
# Copyright 2007 Google Inc.
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

INCLUDE(LibXml2Config)

ADD_SUBDIRECTORY(gtk)
ADD_SUBDIRECTORY(linux)
ADD_SUBDIRECTORY(smjs)
ADD_SUBDIRECTORY(tests)

PKGCONFIG_EX(libcurl 7.16.0 LIBCURL_LIBRARIES)
LIBXML2_CONFIG(2.6.20 LIBXML2_LIBRARIES)

SET(SRCS
  basic_element.cc
  details_view.cc
  display_window.cc
  element_factory.cc
  elements.cc
  file_manager.cc
  gadget.cc
  image.cc
  math_utils.cc
  scriptable_array.cc
  scriptable_event.cc
  scriptable_file_system.cc
  scriptable_framework.cc
  scriptable_helper.cc
  scriptable_menu.cc
  scriptable_options.cc
  signals.cc
  slot.cc
  string_utils.cc
  texture.cc
  text_frame.cc
  unicode_utils.cc
  variant.cc
  view.cc
  windows_locales.cc
  xml_dom.cc
  xml_http_request.cc
  xml_utils.cc

  anchor_element.cc
  button_element.cc
  checkbox_element.cc
  combobox_element.cc
  div_element.cc
  edit_element.cc
  img_element.cc
  item_element.cc
  label_element.cc
  listbox_element.cc
  progressbar_element.cc
  scrollbar_element.cc
)

SET(LIBS
  unzip
  ${LIBCURL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
)

ADD_LIBRARY(ggadget SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget ${LIBS})
OUTPUT_LIBRARY(ggadget)