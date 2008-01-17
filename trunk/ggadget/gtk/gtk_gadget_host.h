/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_GTK_GTK_GADGET_HOST_H__
#define GGADGET_GTK_GTK_GADGET_HOST_H__

#include <set>
#include <map>
#include <gtk/gtk.h>
#include <ggadget/gadget_host_interface.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/gtk/gtk_main_loop.h>

namespace ggadget {
namespace gtk {

class GtkMenuImpl;

/**
 * An implementation of @c GadgetHostInterface for the simple gadget host.
 */
class GtkGadgetHost : public GadgetHostInterface {
 public:
  GtkGadgetHost(ScriptRuntimeInterface *script_runtime,
                FrameworkInterface *framework,
                MainLoopInterface *main_loop,
                bool composited, bool useshapemask,
                double zoom, int debug_mode);
  virtual ~GtkGadgetHost();

  virtual ScriptRuntimeInterface *GetScriptRuntime(ScriptRuntimeType type);
  virtual FileManagerInterface *GetFileManager();
  virtual OptionsInterface *GetOptions();
  virtual FrameworkInterface *GetFramework();
  virtual MainLoopInterface *GetMainLoop();
  virtual XMLParserInterface *GetXMLParser();
  virtual GadgetInterface *GetGadget();
  virtual ViewHostInterface *NewViewHost(ViewType type, ViewInterface *view);

  virtual void SetPluginFlags(int plugin_flags);
  virtual void RemoveMe(bool save_data);

  virtual void DebugOutput(DebugLevel level, const char *message) const;
  virtual uint64_t GetCurrentTime() const;
  virtual bool OpenURL(const char *url) const;
  virtual bool LoadFont(const char *filename);
  virtual bool UnloadFont(const char *filename);
  virtual bool BrowseForFiles(const char *filter, bool multiple,
                              std::vector<std::string> *result);
  virtual void GetCursorPos(int *x, int *y) const;
  virtual void GetScreenSize(int *width, int *height) const;
  virtual std::string GetFileIcon(const char *filename) const;

  /**
   * Loads a gadget from file system and hosts it.
   * @param container the gtk widget to contain this gadget.
   * @param base_path the base path of this gadget. It can be a directory or
   *     path to a .gg file.
   * @param zoom zoom factor of this gadget.
   * @param debug_mode 0: no debug; 1: debugs container elements by drawing
   *     a bounding box for each container element; 2: debugs all elements.
   * @return the loaded gadget if succeeded, or @c NULL otherwise.
   */
  // TODO: store zoom (and debug_mode?) into options repository.
  bool LoadGadget(GtkBox *container, const char *base_path);

  /**
   * Returns a newly initialized context menu, so that the caller can add
   * menu items in it.
   */
  GtkMenuImpl *NewContextMenu();

  /**
   * Popups the current context menu with or without default menu items.
   * If the current context menu contains no items, the method does nothing.
   * @param default_items whether to add the default menu items.
   * @param button the mouse button which was pressed to initiate the popup.
   * @return @c false if the menu contains nothing and was not popped up.
   */
  bool PopupContextMenu(bool default_items, guint button);

  /** Destroy the current context menu. */
  void DestroyContextMenu();

 private:
  void ReportScriptError(const char *message);

  static gboolean DispatchTimer(gpointer data);
  static gboolean DispatchIOWatch(GIOChannel *source,
                                  GIOCondition cond,
                                  gpointer data);

  static void OnMenuClicked(GtkButton *button, gpointer user_data);
  void PopupMenu();
  static void OnBackClicked(GtkButton *button, gpointer user_data);
  static void OnForwardClicked(GtkButton *button, gpointer user_data);
  static void OnDetailsClicked(GtkButton *button, gpointer user_data);
  static void OnCollapseActivate(GtkMenuItem *menu_item, gpointer user_data);
  static void OnOptionsActivate(GtkMenuItem *menu_item, gpointer user_data);
  static void OnAboutActivate(GtkMenuItem *menu_item, gpointer user_data);
  static void OnDockActivate(GtkMenuItem *menu_item, gpointer user_data);

  ScriptRuntimeInterface *script_runtime_;
  FrameworkInterface *framework_;
  MainLoopInterface *main_loop_;

  XMLParserInterface *xml_parser_;
  FileManagerInterface *resource_file_manager_;
  FileManagerInterface *global_file_manager_;
  FileManagerInterface *file_manager_;
  OptionsInterface *options_;
  GadgetInterface *gadget_;

  int plugin_flags_;

  // Maps original font filename to temp font filename
  std::map<std::string, std::string> loaded_fonts_;
  bool composited_;
  bool useshapemask_;
  double zoom_;
  int debug_mode_;

  GtkBox *toolbox_;
  GtkWidget *menu_button_;
  GtkWidget *back_button_;
  GtkWidget *forward_button_;
  GtkWidget *details_button_;
  GtkMenuImpl *menu_;

  DISALLOW_EVIL_CONSTRUCTORS(GtkGadgetHost);
};

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_GTK_GADGET_HOST_H__
