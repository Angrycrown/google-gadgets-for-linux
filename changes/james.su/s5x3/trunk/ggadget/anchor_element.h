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

#ifndef GGADGET_ANCHOR_ELEMENT_H__
#define GGADGET_ANCHOR_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class TextFrame;

class AnchorElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x50ef5c291807400c, BasicElement);

  AnchorElement(BasicElement *parent, View *view, const char *name);
  virtual ~AnchorElement();

 public:
  /**
   * Gets and sets the mouseover text color or texture image of the element.
   * The image is repeated if necessary, not stretched.
   */
  Variant GetOverColor() const;
  void SetOverColor(const Variant &color);

  /** Gets and sets the URL to be launched when this link is clicked. */
  std::string GetHref() const;
  void SetHref(const char *href);

  /** Gets the text frame containing the text content of this anchor. */
  TextFrame *GetTextFrame();

 public:
  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual void GetDefaultSize(double *width, double *height) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(AnchorElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_ANCHOR_ELEMENT_H__