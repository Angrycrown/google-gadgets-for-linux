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

#include "button_element.h"
#include "canvas_interface.h"
#include "canvas_utils.h"
#include "image_interface.h"
#include "gadget_consts.h"
#include "string_utils.h"
#include "text_frame.h"
#include "view.h"

namespace ggadget {

static const double kButtonMargin = 2;

class ButtonElement::Impl {
 public:
  Impl(BasicElement *owner, View *view)
    : text_(owner, view),
      mousedown_(false), mouseover_(false),
      image_(NULL), down_image_(NULL),
      over_image_(NULL), disabled_image_(NULL),
      icon_image_(NULL), icon_disabled_image_(NULL),
      stretch_middle_(false),
      icon_position_(ICON_LEFT) {
    text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER);
    text_.SetAlign(CanvasInterface::ALIGN_CENTER);
    text_.SetVAlign(CanvasInterface::VALIGN_MIDDLE);
  }

  ~Impl() {
    DestroyImage(image_);
    DestroyImage(down_image_);
    DestroyImage(over_image_);
    DestroyImage(disabled_image_);
    DestroyImage(icon_image_);
    DestroyImage(icon_disabled_image_);
  }

  DEFINE_DELEGATE_GETTER(GetTextFrame,
                         &(down_cast<ButtonElement *>(src)->impl_->text_),
                         BasicElement, TextFrame);

  TextFrame text_;
  bool mousedown_;
  bool mouseover_;
  ImageInterface *image_;
  ImageInterface *down_image_;
  ImageInterface *over_image_;
  ImageInterface *disabled_image_;
  ImageInterface *icon_image_;
  ImageInterface *icon_disabled_image_;
  bool stretch_middle_;
  IconPosition icon_position_;
};

ButtonElement::ButtonElement(BasicElement *parent, View *view, const char *name)
    : BasicElement(parent, view, "button", name, false),
      impl_(new Impl(this, view)) {
  SetEnabled(true);
}

static const char *kButtonIconPositionNames[] = {
  "left", "right", "top", "bottom"
};

void ButtonElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  impl_->text_.RegisterClassProperties(Impl::GetTextFrame,
                                       Impl::GetTextFrameConst);
  RegisterProperty("image",
                   NewSlot(&ButtonElement::GetImage),
                   NewSlot(&ButtonElement::SetImage));
  RegisterProperty("downImage",
                   NewSlot(&ButtonElement::GetDownImage),
                   NewSlot(&ButtonElement::SetDownImage));
  RegisterProperty("overImage",
                   NewSlot(&ButtonElement::GetOverImage),
                   NewSlot(&ButtonElement::SetOverImage));
  RegisterProperty("disabledImage",
                   NewSlot(&ButtonElement::GetDisabledImage),
                   NewSlot(&ButtonElement::SetDisabledImage));

  RegisterProperty("caption",
                   NewSlot(&TextFrame::GetText, Impl::GetTextFrameConst),
                   NewSlot(&TextFrame::SetText, Impl::GetTextFrame));
  RegisterProperty("stretchMiddle",
                   NewSlot(&ButtonElement::IsStretchMiddle),
                   NewSlot(&ButtonElement::SetStretchMiddle));

  // iconImage, iconDisabledImage and iconPosition 
  // are currently only supported by GGL.
  RegisterProperty("iconImage",
                   NewSlot(&ButtonElement::GetIconImage),
                   NewSlot(&ButtonElement::SetIconImage));
  RegisterProperty("iconDisabledImage",
                   NewSlot(&ButtonElement::GetIconDisabledImage),
                   NewSlot(&ButtonElement::SetIconDisabledImage));
  RegisterStringEnumProperty("iconPosition",
                             NewSlot(&ButtonElement::GetIconPosition),
                             NewSlot(&ButtonElement::SetIconPosition),
                             kButtonIconPositionNames,
                             arraysize(kButtonIconPositionNames));
}

ButtonElement::~ButtonElement() {
  delete impl_;
  impl_ = NULL;
}

void ButtonElement::DoDraw(CanvasInterface *canvas) {
  ImageInterface *img = NULL;
  ImageInterface *icon = NULL;
  if (!IsEnabled()) {
    img = impl_->disabled_image_;
    icon = impl_->icon_disabled_image_;
  } else if (impl_->mouseover_) {
    img = impl_->mousedown_ ? impl_->down_image_ : impl_->over_image_;
  }

  if (!img) { // draw image_ as last resort
    img = impl_->image_;
  }

  if (!icon) {
    icon = impl_->icon_image_;
  }

  double width = GetPixelWidth();
  double height = GetPixelHeight();
  if (img && width > 0 && height > 0) {
    if (impl_->stretch_middle_) {
      StretchMiddleDrawImage(img, canvas, 0, 0, width, height, -1, -1, -1, -1);
    } else {
      img->StretchDraw(canvas, 0, 0, width, height);
    }
  }

  double text_x = kButtonMargin;
  double text_y = kButtonMargin;
  double text_width, text_height;
  impl_->text_.GetSimpleExtents(&text_width, &text_height);
  bool has_text = (text_width > 0 && text_height > 0);

  if (icon) {
    double icon_width = icon->GetWidth();
    double icon_height = icon->GetHeight();
    if (!has_text) {
      icon->Draw(canvas, (width - icon_width) / 2, (height - icon_height) / 2);
    } else if (impl_->icon_position_ == ButtonElement::ICON_LEFT) {
      icon->Draw(canvas, kButtonMargin, (height - icon_height) / 2);
      text_x = kButtonMargin * 2 + icon_width;
      width -= (kButtonMargin + icon_width);
    } else if (impl_->icon_position_ == ButtonElement::ICON_RIGHT) {
      icon->Draw(canvas, width - icon_width - kButtonMargin,
                 (height - icon_height) / 2);
      text_x = kButtonMargin;
      width -= (kButtonMargin + icon_width);
    } else if (impl_->icon_position_ == ButtonElement::ICON_TOP) {
      icon->Draw(canvas, (width - icon_width) / 2, kButtonMargin);
      text_y = kButtonMargin * 2 + icon_height;
      height -= (kButtonMargin + icon_height);
    } else {
      icon->Draw(canvas, (width - icon_width) / 2,
                 height - icon_height - kButtonMargin);
      text_y = kButtonMargin;
      height -= (kButtonMargin + icon_height);
    }
  }

  if (has_text) {
    width -= kButtonMargin * 2;
    height -= kButtonMargin * 2;

    if (width > 0 && height > 0)
      impl_->text_.Draw(canvas, text_x, text_y, width, height);
  }
}

void ButtonElement::UseDefaultImages() {
  DestroyImage(impl_->image_);
  impl_->image_ = GetView()->LoadImageFromGlobal(kButtonImage, false);
  DestroyImage(impl_->over_image_);
  impl_->over_image_ = GetView()->LoadImageFromGlobal(kButtonOverImage, false);
  DestroyImage(impl_->down_image_);
  impl_->down_image_ = GetView()->LoadImageFromGlobal(kButtonDownImage, false);
  // No default disabled image.
  DestroyImage(impl_->disabled_image_);
  impl_->disabled_image_ = NULL;
  SetStretchMiddle(true);
}

Variant ButtonElement::GetImage() const {
  return Variant(GetImageTag(impl_->image_));
}

void ButtonElement::SetImage(const Variant &img) {
  if (img != GetImage()) {
    DestroyImage(impl_->image_);
    impl_->image_ = GetView()->LoadImage(img, false);
    QueueDraw();
  }
}

Variant ButtonElement::GetDisabledImage() const {
  return Variant(GetImageTag(impl_->disabled_image_));
}

void ButtonElement::SetDisabledImage(const Variant &img) {
  if (img != GetDisabledImage()) {
    DestroyImage(impl_->disabled_image_);
    impl_->disabled_image_ = GetView()->LoadImage(img, false);
    if (!IsEnabled()) {
      QueueDraw();
    }
  }
}

Variant ButtonElement::GetOverImage() const {
  return Variant(GetImageTag(impl_->over_image_));
}

void ButtonElement::SetOverImage(const Variant &img) {
  if (img != GetOverImage()) {
    DestroyImage(impl_->over_image_);
    impl_->over_image_ = GetView()->LoadImage(img, false);
    if (impl_->mouseover_ && IsEnabled()) {
      QueueDraw();
    }
  }
}

Variant ButtonElement::GetDownImage() const {
  return Variant(GetImageTag(impl_->down_image_));
}

void ButtonElement::SetDownImage(const Variant &img) {
  if (img != GetDownImage()) {
    DestroyImage(impl_->down_image_);
    impl_->down_image_ = GetView()->LoadImage(img, false);
    if (impl_->mousedown_ && IsEnabled()) {
      QueueDraw();
    }
  }
}

Variant ButtonElement::GetIconImage() const {
  return Variant(GetImageTag(impl_->icon_image_));
}

void ButtonElement::SetIconImage(const Variant &img) {
  if (img != GetIconImage()) {
    DestroyImage(impl_->icon_image_);
    impl_->icon_image_ = GetView()->LoadImage(img, false);
    QueueDraw();
  }
}

Variant ButtonElement::GetIconDisabledImage() const {
  return Variant(GetImageTag(impl_->icon_disabled_image_));
}

void ButtonElement::SetIconDisabledImage(const Variant &img) {
  if (img != GetIconDisabledImage()) {
    DestroyImage(impl_->icon_disabled_image_);
    impl_->icon_disabled_image_ = GetView()->LoadImage(img, false);
    if (!IsEnabled()) {
      QueueDraw();
    }
  }
}

TextFrame *ButtonElement::GetTextFrame() {
  return &impl_->text_;
}

const TextFrame *ButtonElement::GetTextFrame() const {
  return &impl_->text_;
}

BasicElement *ButtonElement::CreateInstance(BasicElement *parent, View *view,
                                            const char *name) {
  return new ButtonElement(parent, view, name);
}

EventResult ButtonElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = EVENT_RESULT_HANDLED;
  switch (event.GetType()) {
   case Event::EVENT_MOUSE_DOWN:
    if (event.GetButton() & MouseEvent::BUTTON_LEFT) {
      impl_->mousedown_ = true;
      QueueDraw();
    }
    break;
   case Event::EVENT_MOUSE_UP:
    if (impl_->mousedown_) {
      impl_->mousedown_ = false;
      QueueDraw();
    }
    break;
   case Event::EVENT_MOUSE_OUT:
    impl_->mouseover_ = false;
    QueueDraw();
    break;
   case Event::EVENT_MOUSE_OVER:
    impl_->mouseover_ = true;
    QueueDraw();
    break;
   default:
    result = EVENT_RESULT_UNHANDLED;
    break;
  }
  return result;
}

void ButtonElement::GetDefaultSize(double *width, double *height) const {
  double text_width, text_height;
  double icon_width = 0;
  double icon_height = 0;
  impl_->text_.GetSimpleExtents(&text_width, &text_height);

  if (impl_->icon_image_) {
    icon_width = impl_->icon_image_->GetWidth();
    icon_height = impl_->icon_image_->GetHeight();
  }

  if (impl_->icon_disabled_image_) {
    icon_width =
        std::max(icon_width, impl_->icon_disabled_image_->GetWidth());
    icon_height =
        std::max(icon_height, impl_->icon_disabled_image_->GetHeight());
  }

  *width = text_width + kButtonMargin * 2;
  *height = text_height + kButtonMargin * 2;
  if (icon_width > 0 && icon_height > 0) {
    if (impl_->icon_position_ == ButtonElement::ICON_LEFT ||
        impl_->icon_position_ == ButtonElement::ICON_RIGHT) {
      *width += (icon_width + kButtonMargin);
      *height = std::max(*height, icon_height);
    } else {
      *width = std::max(*width, icon_width);
      *height += (icon_height + kButtonMargin);
    }
  }

  if (impl_->image_) {
    *width = std::max(*width, impl_->image_->GetWidth());
    *height = std::max(*height, impl_->image_->GetHeight());
  }
}

bool ButtonElement::IsStretchMiddle() const {
  return impl_->stretch_middle_;
}

void ButtonElement::SetStretchMiddle(bool stretch_middle) {
  if (stretch_middle != impl_->stretch_middle_) {
    impl_->stretch_middle_ = stretch_middle;
    QueueDraw();
  }
}

ButtonElement::IconPosition ButtonElement::GetIconPosition() const {
  return impl_->icon_position_;
}

void ButtonElement::SetIconPosition(ButtonElement::IconPosition position) {
  impl_->icon_position_ = position;
  if (impl_->icon_image_)
    QueueDraw();
}

bool ButtonElement::HasOpaqueBackground() const {
  ImageInterface *img = NULL;
  if (!IsEnabled()) {
    img = impl_->disabled_image_;
  } else if (impl_->mousedown_) {
    img = impl_->down_image_;
  } else if (impl_->mouseover_) {
    img = impl_->over_image_;
  }

  if (!img) { // draw image_ as last resort
    img = impl_->image_;
  }

  return img->IsFullyOpaque();
}

} // namespace ggadget