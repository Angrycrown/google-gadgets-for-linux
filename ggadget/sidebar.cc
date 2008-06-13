/*
  Copyright 2008 Google Inc.

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

#include <vector>
#include <list>

#include "sidebar.h"

#include "button_element.h"
#include "canvas_interface.h"
#include "div_element.h"
#include "element_factory.h"
#include "elements.h"
#include "gadget_consts.h"
#include "gadget.h"
#include "file_manager_factory.h"
#include "host_interface.h"
#include "img_element.h"
#include "math_utils.h"
#include "scriptable_binary_data.h"
#include "view_element.h"
#include "view.h"
#include "messages.h"

namespace ggadget {

class SideBar::Impl : public View {
 public:
  class SideBarViewHost : public ViewHostInterface {
   public:
    SideBarViewHost(SideBar::Impl *owner,
                    ViewHostInterface::Type type,
                    ViewHostInterface *real_viewhost,
                    int index)
        : owner_(owner),
          private_view_(NULL),
          view_element_(NULL),
          real_viewhost_(real_viewhost),
          index_(index) {
    }
    virtual ~SideBarViewHost() {
      if (view_element_ && !owner_->RemoveViewElement(view_element_)) {
        delete view_element_;
      }
      view_element_ = NULL;
      DLOG("SideBarViewHost Dtor: %p", this);
    }
    virtual ViewHostInterface::Type GetType() const {
      return ViewHostInterface::VIEW_HOST_MAIN;
    }
    virtual void Destroy() { delete this; }
    virtual void SetView(ViewInterface *view) {
      if (view_element_) {
        if (!owner_->RemoveViewElement(view_element_))
          delete view_element_;
        view_element_ = NULL;
        private_view_ = NULL;
      }
      if (!view) return;
      view_element_ = new ViewElement(owner_->main_div_, owner_,
                                      down_cast<View *>(view), true);
      // set disable before ShowView is called
      view_element_->SetEnabled(false);
      // insert the view element in proper place
      owner_->InsertViewElement(index_, view_element_);
      private_view_ = view_element_->GetChildView();
      QueueDraw();
    }
    virtual ViewInterface *GetView() const {
      return private_view_;
    }
    virtual GraphicsInterface *NewGraphics() const {
      return real_viewhost_->NewGraphics();
    }
    virtual void *GetNativeWidget() const {
      return owner_->GetNativeWidget();
    }
    virtual void ViewCoordToNativeWidgetCoord(double x, double y,
                                              double *widget_x,
                                              double *widget_y) const {
      double vx = x, vy = y;
      if (view_element_)
        view_element_->SelfCoordToViewCoord(x, y, &vx, &vy);
      if (real_viewhost_)
        real_viewhost_->ViewCoordToNativeWidgetCoord(vx, vy, widget_x, widget_y);
    }
    virtual void NativeWidgetCoordToViewCoord(
        double x, double y, double *view_x, double *view_y) const {
      double vx = x, vy = y;
      if (real_viewhost_)
        real_viewhost_->NativeWidgetCoordToViewCoord(x, y, &vx, &vy);
      if (view_element_)
        view_element_->ViewCoordToSelfCoord(vx, vy, view_x, view_y);
    }
    virtual void QueueDraw() {
      if (view_element_)
        view_element_->QueueDraw();
    }
    virtual void QueueResize() {
      resize_event_();
    }
    virtual void EnableInputShapeMask(bool /* enable */) {
      // Do nothing.
    }
    virtual void SetResizable(ViewInterface::ResizableMode mode) {}
    virtual void SetCaption(const char *caption) {}
    virtual void SetShowCaptionAlways(bool always) {}
    virtual void SetCursor(int type) {
      real_viewhost_->SetCursor(type);
    }
    virtual void SetTooltip(const char *tooltip) {
      real_viewhost_->SetTooltip(tooltip);
    }
    virtual bool ShowView(bool modal, int flags,
                          Slot1<void, int> *feedback_handler) {
      if (view_element_) {
        view_element_->SetEnabled(true);
        owner_->Layout();
      }
      if (feedback_handler) {
        (*feedback_handler)(flags);
        delete feedback_handler;
      }
      return true;
    }
    virtual void CloseView() {
      if (view_element_) {
        view_element_->SetEnabled(false);
        owner_->Layout();
      }
    }
    virtual bool ShowContextMenu(int button) {
      DLOG("Sidebar viewhost's ShowContextMenu");
      return real_viewhost_->ShowContextMenu(button);
    }
    virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest) {}
    virtual void BeginMoveDrag(int button) {}
    virtual void Alert(const char *message) {
      real_viewhost_->Alert(message);
    }
    virtual bool Confirm(const char *message) {
      return real_viewhost_->Confirm(message);
    }
    virtual std::string Prompt(const char *message,
                               const char *default_value) {
      return real_viewhost_->Prompt(message, default_value);
    }
    virtual int GetDebugMode() const {
      return real_viewhost_->GetDebugMode();
    }
    Connection *ConnectOnResize(Slot0<void> *slot) {
      return resize_event_.Connect(slot);
    }
    int index() const {
      return index_;
    }
    void SetIndex(int index) {
      index_ = index;
    }
   private:
    SideBar::Impl *owner_;
    View *private_view_;
    ViewElement *view_element_;
    ViewHostInterface *real_viewhost_;
    EventSignal resize_event_;
    int index_;
  };

  Impl(SideBar *owner, ViewHostInterface *view_host)
      : View(view_host, NULL, NULL, NULL),
        owner_(owner),
        view_host_(view_host),
        null_element_(NULL),
        popout_element_(NULL),
        blank_height_(0),
        mouse_move_event_x_(-1),
        mouse_move_event_y_(-1),
        hit_element_bottom_(false),
        hit_element_normal_part_(false),
        hit_sidebar_border_(false),
        hittest_(HT_CLIENT),
        background_(NULL),
        icon_(NULL),
        main_div_(NULL) {
    SetResizable(ViewInterface::RESIZABLE_TRUE);
    // EnableCanvasCache(false);
    SetupDecorator();
  }
  ~Impl() {
  }

 public:
  virtual EventResult OnMouseEvent(const MouseEvent &event) {
    hittest_ = HT_CLIENT;

    // the mouse down event after expand event should file unexpand event
    if (ShouldFirePopInEvent(event)) {
      popin_event_();
    }

    EventResult result = EVENT_RESULT_UNHANDLED;
    // don't sent mouse event to view elements when layouting or resizing
    if (!hit_element_bottom_)
      result = View::OnMouseEvent(event);

    if (result == EVENT_RESULT_UNHANDLED) {
      if (event.GetX() >= 0 && event.GetX() < kBoderWidth) {
        hittest_ = HT_LEFT;
        SetCursor(CURSOR_SIZEWE);
      } else if (event.GetX() < GetWidth() &&
               event.GetX() >= GetWidth() - kBoderWidth) {
        hittest_ = HT_RIGHT;
        SetCursor(CURSOR_SIZEWE);
      }
    }

    if (result != EVENT_RESULT_UNHANDLED ||
        event.GetButton() != MouseEvent::BUTTON_LEFT) {
      return result;
    }

    int index = 0;
    double x, y;
    BasicElement *element = NULL;
    ViewElement *focused = owner_->GetMouseOverElement();
    double offset = mouse_move_event_y_ - event.GetY();
    switch (event.GetType()) {
      case Event::EVENT_MOUSE_DOWN:
        DLOG("Mouse down at (%f,%f)", event.GetX(), event.GetY());
        mouse_move_event_x_ = event.GetX();
        mouse_move_event_y_ = event.GetY();

        if (hittest_ != HT_CLIENT) {
          hit_sidebar_border_ = true;
          return result;
        }

        if (!focused) return result;
        focused->ViewCoordToSelfCoord(event.GetX(), event.GetY(), &x, &y);
        switch (focused->GetHitTest(x, y)) {
          case HT_BOTTOM:
            hit_element_bottom_ = true;
            // record the original height of each view elements
            for (; index < main_div_->GetChildren()->GetCount(); ++index) {
              element = main_div_->GetChildren()->GetItemByIndex(index);
              elements_height_.push_back(element->GetPixelHeight());
            }
            if (element) {
              blank_height_ = main_div_->GetPixelHeight() -
                  element->GetPixelY() - element->GetPixelHeight();
            }
            break;
          case HT_CLIENT:
            if (focused != popout_element_)
              hit_element_normal_part_ = true;
            break;
          default:
            break;
        }
        return result;
        break;
      case Event::EVENT_MOUSE_UP:
        ResetState();
        return result;
        break;
      case Event::EVENT_MOUSE_MOVE:  // handle it soon
        if (mouse_move_event_x_ < 0 && mouse_move_event_y_ < 0) {
          return EVENT_RESULT_HANDLED;
        }
        if (!focused) {
          // if mouse over null_element_, BasicElement::GetMouseOverElement()
          // will not return it. Check it specially
          if (null_element_) {
            double x, y;
            null_element_->ViewCoordToSelfCoord(event.GetX(), event.GetY(),
                                                &x, &y);
            if (y >= 0 && y <= null_element_->GetPixelHeight()) {
              return EVENT_RESULT_HANDLED;
            }
          }
          return result;
        }
        if (hit_element_bottom_) {
          // set cursor so that user understand that it's still in layout process
          SetCursor(CURSOR_SIZENS);
          int index = GetIndex(focused);
          if (offset < 0) {
            if (DownResize(false, index + 1, &offset) &&
                UpResize(true, index, &offset)) {
              DownResize(true, index + 1, &offset);
              QueueDraw();
            }
          } else {
            UpResize(true, index, &offset);
            Layout();
          }
        } else if (hit_element_normal_part_ &&
                   (std::abs(offset) > kUndockDragThreshold ||
                    std::abs(event.GetX() - mouse_move_event_x_) >
                    kUndockDragThreshold)) {
          undock_event_(mouse_move_event_x_, mouse_move_event_y_);
          ResetState();
        } else if (hit_sidebar_border_) {
          return EVENT_RESULT_UNHANDLED;
        }
        break;
      default:
        return result;
    }

    return EVENT_RESULT_HANDLED;
  }

  virtual HitTest GetHitTest() const {
    // Always return HT_CLIENT except when mouse cursor is on left or right
    // border.
    return hittest_;
  }

  virtual bool OnAddContextMenuItems(MenuInterface *menu) {
    if (GetMouseOverElement() &&
        GetMouseOverElement()->IsInstanceOf(ViewElement::CLASS_ID)) {
      GetMouseOverElement()->OnAddContextMenuItems(menu);
    } else {
      system_menu_event_(menu);
    }
    // In sidebar mode, view host shouldn't add any host level menu items.
    return false;
  }
  virtual bool OnSizing(double *width, double *height) {
    return kSideBarMinWidth < *width && *width < kSideBarMaxWidth;
  }
  virtual void SetSize(double width, double height) {
    View::SetSize(width, height);

    main_div_->SetPixelWidth(width - 2 * kBoderWidth);
    main_div_->SetPixelHeight(height - 2 * kBoderWidth - kIconHeight);

    button_array_[0]->SetPixelX(width - 3 * kIconHeight - 2 - kBoderWidth);
    button_array_[1]->SetPixelX(width - 2 * kIconHeight - 1 - kBoderWidth);
    button_array_[2]->SetPixelX(width - kIconHeight - kBoderWidth);

    Layout();
  }
  void ResetState() {
    if (GetPopupElement()) {
      GetPopupElement()->SetOpacity(1);
      SetPopupElement(NULL);
      Layout();
    }
    mouse_move_event_x_ = -1;
    mouse_move_event_y_ = -1;
    hit_element_bottom_ = false;
    hit_element_normal_part_ = false;
    hit_sidebar_border_ = false;
    blank_height_ = 0;
    elements_height_.clear();
  }
  void SetupDecorator() {
    background_ = new ImgElement(NULL, this, NULL);
    background_->SetSrc(Variant(kVDMainBackground));
    background_->SetStretchMiddle(true);
    background_->SetOpacity(kOpacityFactor);
    background_->SetPixelX(0);
    background_->SetPixelY(0);
    background_->SetRelativeWidth(1);
    background_->SetRelativeHeight(1);
    background_->EnableCanvasCache(true);
    GetChildren()->InsertElement(background_, NULL);

    SetupButtons();

    main_div_ = new DivElement(NULL, this, NULL);
    GetChildren()->InsertElement(main_div_, NULL);
    main_div_->SetPixelX(kBoderWidth);
    main_div_->SetPixelY(kBoderWidth + kIconHeight);
  }
  void SetupButtons() {
    icon_ = new ImgElement(NULL, this, NULL);
    GetChildren()->InsertElement(icon_, NULL);
    icon_->SetSrc(Variant(kSideBarGoogleIcon));
    icon_->EnableCanvasCache(true);
    icon_->SetPixelX(kBoderWidth);
    icon_->SetPixelY(kBoderWidth);

    button_array_[0] = new ButtonElement(NULL, this, NULL);
    button_array_[0]->SetImage(Variant(kSBButtonAddUp));
    button_array_[0]->SetDownImage(Variant(kSBButtonAddDown));
    button_array_[0]->SetOverImage(Variant(kSBButtonAddOver));
    button_array_[0]->SetPixelY(kBoderWidth + (kIconHeight - kButtonWidth) / 2);
    button_array_[0]->SetTooltip(GM_("SIDEBAR_ADD_GADGETS_TOOLTIP"));
    GetChildren()->InsertElement(button_array_[0], NULL);

    button_array_[1] = new ButtonElement(NULL, this, NULL);
    button_array_[1]->SetImage(Variant(kSBButtonMenuUp));
    button_array_[1]->SetDownImage(Variant(kSBButtonMenuDown));
    button_array_[1]->SetOverImage(Variant(kSBButtonMenuOver));
    button_array_[1]->SetPixelY(kBoderWidth + (kIconHeight - kButtonWidth) / 2);
    button_array_[1]->ConnectOnClickEvent(NewSlot(
        this, &Impl::HandleConfigureButtonClick));
    button_array_[1]->SetTooltip(GM_("SIDEBAR_MENU_BUTTON_TOOLTIP"));
    GetChildren()->InsertElement(button_array_[1], NULL);

    button_array_[2] = new ButtonElement(NULL, this, NULL);
    button_array_[2]->SetImage(Variant(kSBButtonMinimizeUp));
    button_array_[2]->SetDownImage(Variant(kSBButtonMinimizeDown));
    button_array_[2]->SetOverImage(Variant(kSBButtonMinimizeOver));
    button_array_[2]->SetPixelY(kBoderWidth + (kIconHeight - kButtonWidth) / 2);
    button_array_[2]->SetTooltip(GM_("SIDEBAR_MINIMIZE_BUTTON_TOOLTIP"));
    GetChildren()->InsertElement(button_array_[2], NULL);
  }
  bool ShouldFirePopInEvent(const MouseEvent &event) {
    if (!popout_element_ ||
        event.GetType() != MouseEvent::EVENT_MOUSE_DOWN ||
        GetMouseOverElement() == popout_element_)
      return false;
    return true;
  }
  void HandleConfigureButtonClick() {
    view_host_->ShowContextMenu(MouseEvent::BUTTON_LEFT);
  }
  int GetIndex(const BasicElement *element) const {
    ASSERT(element->IsInstanceOf(ViewElement::CLASS_ID));
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i)
      if (element == main_div_->GetChildren()->GetItemByIndex(i)) return i;
    return -1;
  }
  void InsertViewElement(int index, BasicElement *element) {
    ASSERT(element && element->IsInstanceOf(ViewElement::CLASS_ID));
    ASSERT(index >= 0);
    // insert the element by the order of their SideBarViewHost
    int i = 0;
    for (; i < main_div_->GetChildren()->GetCount(); ++i) {
      BasicElement *e = main_div_->GetChildren()->GetItemByIndex(i);
      int cur_index = i;
      ViewInterface *view = down_cast<ViewElement *>(e)->GetChildView();
      // for element that has not view, only judge by the element order
      if (view)
        cur_index = down_cast<SideBarViewHost *>(view->GetViewHost())->index();
      if (cur_index >= index) {
        if (e != element) {
          main_div_->GetChildren()->InsertElement(element, e);
          if (element->IsEnabled()) Layout();
        }
        return;
      }
    }
    // not find a proper position, append to the end
    if (down_cast<ViewElement *>(element)->GetChildView())
      main_div_->GetChildren()->InsertElement(element, NULL);
    else
      ClearPlaceHolder();
  }
  bool RemoveViewElement(BasicElement *element) {
    ASSERT(element && element->IsInstanceOf(ViewElement::CLASS_ID));
    bool enabled = element->IsEnabled();
    bool r = main_div_->GetChildren()->RemoveElement(element);
    if (enabled && r) Layout();
    return r;
  }
  void Layout() {
    double y = kGadgetSpacing;
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i) {
      ViewElement *element =
        down_cast<ViewElement *>(main_div_->GetChildren()->GetItemByIndex(i));
      if (!element->IsEnabled()) continue;
      double width = main_div_->GetPixelWidth();
      double height = ceil(element->GetPixelHeight());
      if (element->IsVisible() && element->OnSizing(&width, &height)) {
        element->SetSize(width, ceil(height));
      }
      element->SetPixelX(0);
      element->SetPixelY(ceil(y));
      y += element->GetPixelHeight() + kGadgetSpacing;
    }
    QueueDraw();
  }
  ViewElement *FindViewElementByView(ViewInterface *view) const {
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i) {
      ViewElement *element = down_cast<ViewElement *>(
          main_div_->GetChildren()->GetItemByIndex(i));
      // they may be not exactly the same view, but they should be owned by
      // the same gadget...
      if (element->GetChildView() &&
          element->GetChildView()->GetGadget() == view->GetGadget())
        return element;
    }
    return NULL;
  }
  void InsertPlaceholder(int index, double height) {
    // only one null element is allowed
    if (!null_element_) {
      null_element_ = new ViewElement(main_div_, this, NULL, true);
      null_element_->SetVisible(false);
    }
    null_element_->SetPixelHeight(height);
    InsertViewElement(index, null_element_);
  }
  void ClearPlaceHolder() {
    if (null_element_) {
      main_div_->GetChildren()->RemoveElement(null_element_);
      null_element_ = NULL;
      Layout();
    }
  }
  // *offset could be any value
  bool UpResize(bool do_resize, int index, double *offset) {
    double sign = *offset > 0 ? 1 : -1;
    double count = 0;
    while (*offset * sign > count * sign && index >= 0) {
      ViewElement *element = down_cast<ViewElement *>(
          main_div_->GetChildren()->GetItemByIndex(index));
      double w = element->GetPixelWidth();
      double h = elements_height_[index] + count - *offset;
      // don't send non-positive resize request
      if (h <= .0) h = 1;
      if (element->OnSizing(&w, &h)) {
        double diff = std::min(sign * (elements_height_[index] - h),
                               sign * (*offset - count)) * sign;
        if (do_resize)
          element->SetSize(w, ceil(elements_height_[index] - diff));
        count += diff;
      } else {
        double oh = element->GetPixelHeight();
        double diff = std::min(sign * (elements_height_[index] - oh),
                               sign * (*offset - count)) * sign;
        if (diff > 0) count += diff;
      }
      index--;
    }
    if (do_resize)
      // recover upmost elements' size
      while (index >= 0) {
        ViewElement *element = down_cast<ViewElement *>(
            main_div_->GetChildren()->GetItemByIndex(index));
        element->SetSize(main_div_->GetPixelWidth(), elements_height_[index]);
        index--;
      }
    DLOG("original: at last off: %.1lf, count: %.1lf", *offset, count);
    if (count == 0) return false;
    *offset = count;
    return true;
  }
  bool DownResize(bool do_resize, int index, double *offset) {
    double count = 0;
    if (blank_height_ > 0) count = std::max(-blank_height_, *offset);
    while (*offset < count && index < main_div_->GetChildren()->GetCount()) {
      ViewElement *element = down_cast<ViewElement *>(
          main_div_->GetChildren()->GetItemByIndex(index));
      double w = element->GetPixelWidth();
      double h = elements_height_[index] + *offset - count;
      // don't send non-positive resize request
      if (h <= .0) h = 1;
      if (element->OnSizing(&w, &h) && h < elements_height_[index]) {
        double diff = std::min(elements_height_[index] - h, count - *offset);
        if (do_resize) {
          element->SetSize(w, ceil(elements_height_[index] - diff));
        }
        count -= diff;
      } else {
        double oh = element->GetPixelHeight();
        double diff = std::min(elements_height_[index] - oh, count - *offset);
        if (diff > 0) count -= diff;
      }
      index++;
    }
    if (do_resize) {
      // recover upmost elemnts' size
      while (index < main_div_->GetChildren()->GetCount()) {
        ViewElement *element = down_cast<ViewElement *>(
            main_div_->GetChildren()->GetItemByIndex(index));
        element->SetSize(main_div_->GetPixelWidth(), elements_height_[index]);
        index++;
      }
      Layout();
    }
    if (count == 0) return false;
    *offset = count;
    return true;
  }
  inline double GetBlankHeight() const {
    int index = main_div_->GetChildren()->GetCount();
    if (!index) return GetHeight();
    BasicElement *element = main_div_->GetChildren()->GetItemByIndex(index - 1);
    return GetHeight() - element->GetPixelY() - element->GetPixelHeight();
  }

 public:
  SideBar *owner_;
  ViewHostInterface *view_host_;

  ViewElement *null_element_;
  ViewElement *popout_element_;
  ViewElement *popout_details_view_element_;

  std::vector<double> elements_height_;
  double blank_height_;
  double mouse_move_event_x_;
  double mouse_move_event_y_;
  bool hit_element_bottom_;
  bool hit_element_normal_part_;
  bool hit_sidebar_border_;
  HitTest hittest_;

  // elements of sidebar decorator
  ImgElement *background_;
  ImgElement *icon_;
  DivElement *main_div_;
  ButtonElement *button_array_[3];

  EventSignal popin_event_;
  EventSignal size_event_;
  Signal1<bool, MenuInterface *> system_menu_event_;
  Signal2<void, double, double> undock_event_;

  static const double kGadgetSpacing = 1;
  static const double kUndockDragThreshold = 2;
  static const double kOpacityFactor = 0.618;
  static const double kSideBarMinWidth = 50;
  static const double kSideBarMaxWidth = 999;
  static const double kBoderWidth = 3;
  static const double kButtonWidth = 18;
  static const double kIconHeight = 22;
};

const double SideBar::Impl::kGadgetSpacing;
const double SideBar::Impl::kUndockDragThreshold;
const double SideBar::Impl::kOpacityFactor;
const double SideBar::Impl::kSideBarMinWidth;
const double SideBar::Impl::kSideBarMaxWidth;
const double SideBar::Impl::kBoderWidth;
const double SideBar::Impl::kButtonWidth;
const double SideBar::Impl::kIconHeight;

SideBar::SideBar(ViewHostInterface *view_host)
  : impl_(new Impl(this, view_host)) {
}

SideBar::~SideBar() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface *SideBar::NewViewHost(int index) {
  Impl::SideBarViewHost *vh =
      new Impl::SideBarViewHost(impl_, ViewHostInterface::VIEW_HOST_MAIN,
                                impl_->view_host_, index);
  vh->ConnectOnResize(NewSlot(impl_, &Impl::Layout));
  return vh;
}

ViewHostInterface *SideBar::GetSideBarViewHost() const {
  return impl_->GetViewHost();
}

void SideBar::SetSize(double width, double height) {
  impl_->SetSize(width, height);
}

double SideBar::GetWidth() const {
  return impl_->GetWidth();
}

double SideBar::GetHeight() const {
  return impl_->GetHeight();
}

int SideBar::GetIndexFromHeight(double height) const {
  int i = 0;
  BasicElement *e = NULL;
  for (; i < impl_->main_div_->GetChildren()->GetCount(); ++i) {
    e = impl_->main_div_->GetChildren()->GetItemByIndex(i);
    ViewInterface *view = down_cast<ViewElement *>(e)->GetChildView();
    // ignore place holder
    if (!view) continue;
    double middle = e->GetPixelY() + e->GetPixelHeight() / 2;
    if (height - impl_->main_div_->GetPixelY() < middle)
      return down_cast<Impl::SideBarViewHost *>(view->GetViewHost())->index();
  }
  return e ? down_cast<Impl::SideBarViewHost *>(down_cast<ViewElement *>(
      e)->GetChildView()->GetViewHost())->index() + 1 : 0;
}

void SideBar::InsertPlaceholder(int index, double height) {
  return impl_->InsertPlaceholder(index, height);
}

void SideBar::ClearPlaceHolder() {
  impl_->ClearPlaceHolder();
}

void SideBar::Layout() {
  impl_->Layout();
}

void SideBar::UpdateElememtsIndex() {
  for (int i = 0; i < impl_->main_div_->GetChildren()->GetCount(); ++i) {
    BasicElement *e = impl_->main_div_->GetChildren()->GetItemByIndex(i);
    ViewInterface *v = down_cast<ViewElement *>(e)->GetChildView();
    if (!v) continue;
    down_cast<Impl::SideBarViewHost* >(v->GetViewHost())->SetIndex(i);
  }
}

ViewElement *SideBar::GetMouseOverElement() const {
  BasicElement *element = impl_->GetMouseOverElement();
  if (element && element->IsInstanceOf(ViewElement::CLASS_ID))
    return down_cast<ViewElement *>(element);
  return NULL;
}

ViewElement *SideBar::FindViewElementByView(ViewInterface *view) const {
  return impl_->FindViewElementByView(view);
}

ViewElement *SideBar::GetViewElementByIndex(int index) const {
  return down_cast<ViewElement *>(
      impl_->main_div_->GetChildren()->GetItemByIndex(index));
}

ViewElement *SideBar::SetPopOutedView(ViewInterface *view) {
  if (view) {
    ViewElement *element = impl_->FindViewElementByView(view);
    // popin previous element before set other popout one
    if (impl_->popout_element_ && impl_->popout_element_ != element)
      impl_->popin_event_();
    impl_->popout_element_ = element;
  } else {
    impl_->popout_element_ = NULL;
  }
  return impl_->popout_element_;
}

Connection *SideBar::ConnectOnUndock(Slot2<void, double, double> *slot) {
  return impl_->undock_event_.Connect(slot);
}

Connection *SideBar::ConnectOnPopIn(Slot0<void> *slot) {
  return impl_->popin_event_.Connect(slot);
}

Connection *SideBar::ConnectOnAddGadget(Slot0<void> *slot) {
  return impl_->button_array_[0]->ConnectOnClickEvent(slot);
}

Connection *SideBar::ConnectOnMenuOpen(Slot1<bool, MenuInterface *> *slot) {
  return impl_->system_menu_event_.Connect(slot);
}

Connection *SideBar::ConnectOnClose(Slot0<void> *slot) {
  return impl_->button_array_[2]->ConnectOnClickEvent(slot);
}

Connection *SideBar::ConnectOnSizeEvent(Slot0<void> *slot) {
  return impl_->ConnectOnSizeEvent(slot);
}

}  // namespace ggadget
