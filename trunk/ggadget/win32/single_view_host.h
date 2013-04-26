/*
  Copyright 2011 Google Inc.

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

#ifndef GGADGET_WIN32_SINGLE_VIEW_HOST_H_
#define GGADGET_WIN32_SINGLE_VIEW_HOST_H_

#include <string>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/win32/gdiplus_canvas.h>

namespace Gdiplus {
  class Bitmap;
}

namespace ggadget {

class Connection;

namespace win32 {

class MenuBuilder;
class PrivateFontDatabase;

// An implementation of @c ViewHostInterface based on atl window.
// This host can only show one View in single window.
class SingleViewHost : public ViewHostInterface {
 public:
  // @param type The view host type to this host.
  // @param zoom Zoom factor used by the Graphics object.
  // @param debug_mode The debug mode of view drawing.
  // @param private_font_database_ a PrivateFontDatabase pointer that process
  //     all private font resources installed by the current host.
  // @param window_class_style The window class style used in WNDCLASS.dwStyle.
  // @param window_style The window style used in ::CreateWindowEx.
  // @param window_exstyle The extended window style used in ::CreateWindowEx.
  SingleViewHost(ViewHostInterface::Type type,
                 double zoom,
                 int debug_mode,
                 const PrivateFontDatabase* private_font_database_,
                 int window_class_style,
                 int window_style,
                 int window_exstyle);
  virtual ~SingleViewHost();
  virtual Type GetType() const;
  virtual void Destroy();
  virtual void SetView(ViewInterface* view);
  virtual ViewInterface* GetView() const;
  virtual GraphicsInterface* NewGraphics() const;
  virtual void* GetNativeWidget() const;
  virtual void ViewCoordToNativeWidgetCoord(
      double x, double y, double* widget_x, double* widget_y) const;
  virtual void NativeWidgetCoordToViewCoord(
      double x, double y, double* view_x, double* view_y) const;
  virtual void QueueDraw();
  virtual void QueueResize();
  virtual void EnableInputShapeMask(bool enable);
  virtual void SetResizable(ViewInterface::ResizableMode mode);
  virtual void SetCaption(const std::string &caption);
  virtual void SetShowCaptionAlways(bool always);
  virtual void SetCursor(ViewInterface::CursorType type);
  virtual void ShowTooltip(const std::string &tooltip);
  virtual void ShowTooltipAtPosition(const std::string &tooltip,
                                     double x, double y);
  virtual bool ShowView(bool modal,
                        int flags,
                        Slot1<bool, int>* feedback_handler);
  virtual void CloseView();
  virtual bool ShowContextMenu(int button);
  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest);
  virtual void BeginMoveDrag(int button);
  virtual void Alert(const ViewInterface* view, const char* message);
  virtual ConfirmResponse Confirm(const ViewInterface* view,
                                  const char* message,
                                  bool cancel_button);
  virtual std::string Prompt(const ViewInterface* view,
                             const char* message,
                             const char* default_value);
  virtual int GetDebugMode() const;

  // Makes the window always on top of @param topmost is true.
  void SetTopMost(bool topmost);
  bool GetTopMost();
  void SetWindowPosition(int x, int y);
  void GetWindowPosition(int* x, int* y);
  void GetWindowSize(int* width, int* height);
  // Returns a Gdiplus::Bitmap object that contains the image of the view.
  // The caller may not delete the return value because the bitmap is also used
  // in view host.
  const Gdiplus::Bitmap* GetViewContent();
  void EnableAlwaysOnTopMenu(bool enable);
  // Sets if the window is enabled.
  void Enable(bool enabled);
  // Sets the opacity of the whole window. the opacity will be multiplied to the
  // alpha value in the view.
  void SetOpacity(double opacity);
  // Connect a slot to OnEndMoveDrag signal.
  // The slot will be called when the user release the mouse button when
  // dragging to moving to window.
  // The two parameters are the new x and y position of the top level window's
  // top left corner, related to the screen.
  // The OnEndMoveDrag signal is actually in GadgetWindow class. As the
  // GadgeWindow object is created in the SetView method, this method should be
  // called after SetView is called or after the initialization of gadget.
  Connection* ConnectOnEndMoveDrag(Slot2<void, int, int>* handler);

  // Connect a slot to OnShowContextMenu signal.
  // The slot will be called when the view is about to show a context menu. If
  // the slot returns false, the viewhost will not show the context menu.
  Connection* ConnectOnShowContextMenu(Slot1<bool, MenuInterface*>* handler);
  void SetZoom(double zoom);
  double GetZoom();
  void SetFontScale(double scale);

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(SingleViewHost);
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_SINGLE_VIEW_HOST_H_
