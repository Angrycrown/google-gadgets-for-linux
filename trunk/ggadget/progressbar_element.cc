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

#include "progressbar_element.h"
#include "canvas_interface.h"
#include "gadget_consts.h"
#include "image_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "view.h"

namespace ggadget {

static const char *kOrientationNames[] = {
  "vertical", "horizontal"
};

class ProgressBarElement::Impl {
 public:
  Impl(ProgressBarElement *owner)
      : owner_(owner),
        thumbover_(false), thumbdown_(false),
        emptyimage_(NULL), fullimage_(NULL), thumbdisabledimage_(NULL),
        thumbdownimage_(NULL), thumboverimage_(NULL), thumbimage_(NULL),
        // The values below are the default ones in Windows.
        min_(0), max_(100), value_(0),
        drag_delta_(0.),
        orientation_(ORIENTATION_HORIZONTAL) {
  }
  ~Impl() {
    DestroyImage(emptyimage_);
    DestroyImage(fullimage_);
    DestroyImage(thumbdisabledimage_);
    DestroyImage(thumbdownimage_);
    DestroyImage(thumboverimage_);
    DestroyImage(thumbimage_);
  }

  /**
   * Utility function for getting the int value from a position on the
   * progressbar. It does not check to make sure that the value is within range.
   * Assumes that thumb isn't NULL.
   */
  int GetValueFromLocation(double ownerwidth, double ownerheight,
                           ImageInterface *thumb, double x, double y) {
    int delta = max_ - min_;
    double position, denominator;
    if (orientation_ == ORIENTATION_HORIZONTAL) {
      denominator = ownerwidth;
      if (thumbdown_ && thumb) {
        denominator -= thumb->GetWidth();
      }
      if (denominator == 0) { // prevent overflow
        position = 0;
      } else {
        position = delta * (x - drag_delta_) / denominator;
      }
    } else { // Progressbar grows from the bottom in vertical orientation.
      denominator = ownerheight;
      if (thumbdown_ && thumb) {
        denominator -= thumb->GetHeight();
      }
      if (denominator == 0) { // prevent overflow
        position = 0;
      } else {
        position = delta - (delta * (y - drag_delta_) / denominator);
      }
    }

    int answer = static_cast<int>(position);
    answer += min_;
    return answer;
  }

  /** Returns the current value expressed as a fraction of the total progress. */
  double GetFractionalValue() {
    if (max_ == min_) {
      return 0; // handle overflow & corner cases
    }
    return (value_ - min_) / static_cast<double>(max_ - min_);
  }

  ImageInterface *GetThumbAndLocation(double ownerwidth, double ownerheight,
                             double fraction, double *x, double *y) {
    ImageInterface *thumb = GetCurrentThumbImage();
    if (!thumb) {
      *x = *y = 0;
      return NULL;
    }

    double imgw = thumb->GetWidth();
    double imgh = thumb->GetHeight();
    if (orientation_ == ORIENTATION_HORIZONTAL) {
      *x = fraction * (ownerwidth - imgw);
      *y = (ownerheight - imgh) / 2.;
    } else { // Thumb grows from bottom in vertical orientation.
      *x = (ownerwidth - imgw) / 2.;
      *y = (1. - fraction) * (ownerheight - imgh);
    }
    return thumb;
  }

  void SetValue(int value) {
    if (value > max_) {
      value = max_;
    } else if (value < min_) {
      value = min_;
    }

    if (value != value_) {
      value_ = value;
      DLOG("progress value: %d", value_);
      owner_->QueueDraw();
      SimpleEvent event(Event::EVENT_CHANGE);
      ScriptableEvent s_event(&event, owner_, NULL);
      owner_->GetView()->FireEvent(&s_event, onchange_event_);
    }
  }

  ImageInterface *GetCurrentThumbImage() {
    ImageInterface *img = NULL;
    if (!owner_->IsEnabled()) {
       img = thumbdisabledimage_;
    } else if (thumbdown_) {
      img = thumbdownimage_;
    } else if (thumbover_) {
      img = thumboverimage_;
    }

    if (!img) { // fallback
      img = thumbimage_;
    }
    return img;
  }

  ProgressBarElement *owner_;
  bool thumbover_, thumbdown_;
  ImageInterface *emptyimage_, *fullimage_, *thumbdisabledimage_,
                 *thumbdownimage_, *thumboverimage_, *thumbimage_;
  int min_, max_, value_;
  double drag_delta_;
  Orientation orientation_;
  EventSignal onchange_event_;
};

ProgressBarElement::ProgressBarElement(BasicElement *parent, View *view,
                                       const char *name)
    : BasicElement(parent, view, "progressbar", name, false),
      impl_(new Impl(this)) {
}

void ProgressBarElement::DoRegister() {
  BasicElement::DoRegister();
  RegisterProperty("emptyImage",
                   NewSlot(this, &ProgressBarElement::GetEmptyImage),
                   NewSlot(this, &ProgressBarElement::SetEmptyImage));
  RegisterProperty("max",
                   NewSlot(this, &ProgressBarElement::GetMax),
                   NewSlot(this, &ProgressBarElement::SetMax));
  RegisterProperty("min",
                   NewSlot(this, &ProgressBarElement::GetMin),
                   NewSlot(this, &ProgressBarElement::SetMin));
  RegisterStringEnumProperty("orientation",
                   NewSlot(this, &ProgressBarElement::GetOrientation),
                   NewSlot(this, &ProgressBarElement::SetOrientation),
                   kOrientationNames, arraysize(kOrientationNames));
  RegisterProperty("fullImage",
                   NewSlot(this, &ProgressBarElement::GetFullImage),
                   NewSlot(this, &ProgressBarElement::SetFullImage));
  RegisterProperty("thumbDisabledImage",
                   NewSlot(this, &ProgressBarElement::GetThumbDisabledImage),
                   NewSlot(this, &ProgressBarElement::SetThumbDisabledImage));
  RegisterProperty("thumbDownImage",
                   NewSlot(this, &ProgressBarElement::GetThumbDownImage),
                   NewSlot(this, &ProgressBarElement::SetThumbDownImage));
  RegisterProperty("thumbImage",
                   NewSlot(this, &ProgressBarElement::GetThumbImage),
                   NewSlot(this, &ProgressBarElement::SetThumbImage));
  RegisterProperty("thumbOverImage",
                   NewSlot(this, &ProgressBarElement::GetThumbOverImage),
                   NewSlot(this, &ProgressBarElement::SetThumbOverImage));
  RegisterProperty("value",
                   NewSlot(this, &ProgressBarElement::GetValue),
                   NewSlot(this, &ProgressBarElement::SetValue));

  RegisterSignal(kOnChangeEvent, &impl_->onchange_event_);
}

ProgressBarElement::~ProgressBarElement() {
  delete impl_;
  impl_ = NULL;
}

void ProgressBarElement::DoDraw(CanvasInterface *canvas) {
  // Drawing order: empty, full, thumb.
  // Empty and full images only stretch in one direction, and only if
  // element size is greater than that of the image. Otherwise the image is
  // cropped.
  double pxwidth = GetPixelWidth();
  double pxheight = GetPixelHeight();
  double fraction = impl_->GetFractionalValue();
  bool drawfullimage = impl_->fullimage_ && fraction > 0;

  double x = 0, y = 0, fw = 0, fh = 0, fy = 0;
  bool fstretch = false;
  if (drawfullimage) {
    // Need to calculate fullimage positions first in order to determine
    // clip rectangle for emptyimage.
    double imgw = impl_->fullimage_->GetWidth();
    double imgh = impl_->fullimage_->GetHeight();
    if (impl_->orientation_ == ORIENTATION_HORIZONTAL) {
      x = 0;
      fy = y = (pxheight - imgh) / 2.;
      fw = fraction * pxwidth;
      fh = imgh;
      fstretch = (imgw < fw);
    } else { // Progressbar grows from the bottom in vertical orientation.
      x = (pxwidth - imgw) / 2.;
      fw = imgw;
      fh = fraction * pxheight;
      y = pxheight - fh;
      fstretch = (imgh < fh);
      if (!fstretch) {
        fy = pxheight - imgh;
      }
    }
  }

  if (impl_->emptyimage_) {
    double ex, ey, ew, eh, clipx, cliph;
    double imgw = impl_->emptyimage_->GetWidth();
    double imgh = impl_->emptyimage_->GetHeight();
    bool estretch;
    if (impl_->orientation_ == ORIENTATION_HORIZONTAL) {
      ex = 0;
      ey = (pxheight - imgh) / 2.;
      ew = pxwidth;
      eh = imgh;
      estretch = (imgw < ew);
      clipx = fw;
      cliph = pxheight;
    } else {
      ex = (pxwidth - imgw) / 2.;
      ey = 0;
      ew = imgw;
      eh = pxheight;
      estretch = (imgh < eh);
      clipx = 0;
      cliph = y;
    }

    if (drawfullimage) { // This clip only sets the left/bottom border of image.
      canvas->PushState();
      canvas->IntersectRectClipRegion(clipx, 0, pxwidth, cliph);
    }

    if (estretch) {
      impl_->emptyimage_->StretchDraw(canvas, ex, ey, ew, eh);
    }
    else {
      // No need to set clipping since element border is crop border here.
      impl_->emptyimage_->Draw(canvas, ex, ey);
    }

    if (drawfullimage) {
      canvas->PopState();
    }
  }

  if (drawfullimage) {
    if (fstretch) {
      impl_->fullimage_->StretchDraw(canvas, x, y, fw, fh);
    } else {
      canvas->PushState();
      canvas->IntersectRectClipRegion(x, y, fw, fh);
      impl_->fullimage_->Draw(canvas, x, fy);
      canvas->PopState();
    }
  }

  // The thumb is never resized or cropped.
  ImageInterface *thumb = impl_->GetThumbAndLocation(pxwidth, pxheight,
                                                     fraction, &x, &y);
  if (thumb) {
    thumb->Draw(canvas, x, y);
  }
}

int ProgressBarElement::GetMax() const {
  return impl_->max_;
}

void ProgressBarElement::SetMax(int value) {
  if (value != impl_->max_) {
    impl_->max_ = value;
    if (impl_->value_ > value) {
      impl_->value_ = value;
    }
    QueueDraw();
  }
}

int ProgressBarElement::GetMin() const {
  return impl_->min_;
}

void ProgressBarElement::SetMin(int value) {
  if (value != impl_->min_) {
    impl_->min_ = value;
    if (impl_->value_ < value) {
      impl_->value_ = value;
    }
    QueueDraw();
  }
}

int ProgressBarElement::GetValue() const {
  return impl_->value_;
}

void ProgressBarElement::SetValue(int value) {
  impl_->SetValue(value);
}

ProgressBarElement::Orientation ProgressBarElement::GetOrientation() const {
  return impl_->orientation_;
}

void ProgressBarElement::SetOrientation(ProgressBarElement::Orientation o) {
  if (o != impl_->orientation_) {
    impl_->orientation_ = o;
    QueueDraw();
  }
}

Variant ProgressBarElement::GetEmptyImage() const {
  return Variant(GetImageTag(impl_->emptyimage_));
}

void ProgressBarElement::SetEmptyImage(const Variant &img) {
  DestroyImage(impl_->emptyimage_);
  impl_->emptyimage_ = GetView()->LoadImage(img, false);
  QueueDraw();
}

Variant ProgressBarElement::GetFullImage() const {
  return Variant(GetImageTag(impl_->fullimage_));
}

void ProgressBarElement::SetFullImage(const Variant &img) {
  DestroyImage(impl_->fullimage_);
  impl_->fullimage_ = GetView()->LoadImage(img, false);
  if (impl_->value_ != impl_->min_) { // not empty
    QueueDraw();
  }
}

Variant ProgressBarElement::GetThumbDisabledImage() const {
  return Variant(GetImageTag(impl_->thumbdisabledimage_));
}

void ProgressBarElement::SetThumbDisabledImage(const Variant &img) {
  DestroyImage(impl_->thumbdisabledimage_);
  impl_->thumbdisabledimage_ = GetView()->LoadImage(img, false);
  if (!IsEnabled()) {
    QueueDraw();
  }
}

Variant ProgressBarElement::GetThumbDownImage() const {
  return Variant(GetImageTag(impl_->thumbdownimage_));
}

void ProgressBarElement::SetThumbDownImage(const Variant &img) {
  DestroyImage(impl_->thumbdownimage_);
  impl_->thumbdownimage_ = GetView()->LoadImage(img, false);
  if (impl_->thumbdown_ && IsEnabled()) {
    QueueDraw();
  }
}

Variant ProgressBarElement::GetThumbImage() const {
  return Variant(GetImageTag(impl_->thumbimage_));
}

void ProgressBarElement::SetThumbImage(const Variant &img) {
  DestroyImage(impl_->thumbimage_);
  impl_->thumbimage_ = GetView()->LoadImage(img, false);
  QueueDraw(); // Always queue since this is the fallback.
}

Variant ProgressBarElement::GetThumbOverImage() const {
  return Variant(GetImageTag(impl_->thumboverimage_));
}

void ProgressBarElement::SetThumbOverImage(const Variant &img) {
  DestroyImage(impl_->thumboverimage_);
  impl_->thumboverimage_ = GetView()->LoadImage(img, false);
  if (impl_->thumbover_ && IsEnabled()) {
    QueueDraw();
  }
}
BasicElement *ProgressBarElement::CreateInstance(BasicElement *parent,
                                                 View *view,
                                                 const char *name) {
  return new ProgressBarElement(parent, view, name);
}

EventResult ProgressBarElement::HandleMouseEvent(const MouseEvent &event) {
  double pxwidth = GetPixelWidth();
  double pxheight = GetPixelHeight();
  double fraction = impl_->GetFractionalValue();
  double tx, ty;
  bool over = false;
  ImageInterface *thumb = impl_->GetThumbAndLocation(pxwidth, pxheight,
                                                     fraction, &tx, &ty);
  if (thumb) {
    over = IsPointInElement(event.GetX() - tx, event.GetY() - ty,
                            thumb->GetWidth(), thumb->GetHeight());
  }

  EventResult result = EVENT_RESULT_HANDLED;
  switch (event.GetType()) {
    case Event::EVENT_MOUSE_MOVE:
    case Event::EVENT_MOUSE_OUT:
    case Event::EVENT_MOUSE_OVER:
      if (event.GetButton() & MouseEvent::BUTTON_LEFT) {
        int value = impl_->GetValueFromLocation(pxwidth, pxheight, thumb,
                                                event.GetX(), event.GetY());
        SetValue(value); // SetValue will queue a draw.
      }

      if (over != impl_->thumbover_) {
        impl_->thumbover_ = over;
        QueueDraw();
      }
      break;
    case Event::EVENT_MOUSE_DOWN:
      if (event.GetButton() & MouseEvent::BUTTON_LEFT) {
        if (over) {
          // The drag delta setting here is tricky. If the button is held down
          // initially over the thumb, then the pointer should always stay
          // on top of the same location on the thumb when dragged, thus
          // reflecting the value indicated by the bottom-left corner of the
          // thumb, not the current position of the pointer.
          // If the mouse button is held down over any other part of the
          // progressbar, then the pointer should reflect the value of the
          // point under it.
          // This is different from scrollbar, where there's only a single
          // case for the drag delta setting. In the progressbar, the drag
          // delta depends on whether the initial mousedown is fired over the
          // thumb or not.
          if (impl_->orientation_ == ORIENTATION_HORIZONTAL) {
            impl_->drag_delta_ = event.GetX() - tx;
          }
          else {
            impl_->drag_delta_ = event.GetY() - ty;
          }

          impl_->thumbdown_ = true;
          QueueDraw(); // Redraw because of the thumbdown.
        } else {
          impl_->drag_delta_ = 0;
          int value = impl_->GetValueFromLocation(pxwidth, pxheight, thumb,
                                                  event.GetX(), event.GetY());
          SetValue(value); // SetValue will queue a draw.
        }
      }
      break;
    case Event::EVENT_MOUSE_UP:
      if (impl_->thumbdown_) {
        impl_->thumbdown_ = false;
        QueueDraw();
      }
      break;
    default:
      result = EVENT_RESULT_UNHANDLED;
      break;
  }
  return result;
}

Connection *ProgressBarElement::ConnectOnChangeEvent(Slot0<void> *handler) {
  return impl_->onchange_event_.Connect(handler);
}

void ProgressBarElement::GetDefaultSize(double *width, double *height) const {
  if (impl_->emptyimage_) {
    *width = impl_->emptyimage_->GetWidth();
    *height = impl_->emptyimage_->GetHeight();
  } else {
    *width = *height = 0;
  }
}

bool ProgressBarElement::HasOpaqueBackground() const {
  return impl_->fullimage_ && impl_->fullimage_->IsFullyOpaque() &&
         impl_->emptyimage_ && impl_->emptyimage_->IsFullyOpaque();
}

} // namespace ggadget