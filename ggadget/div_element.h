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

#ifndef GGADGETS_DIV_ELEMENT_H__
#define GGADGETS_DIV_ELEMENT_H__

#include <stdlib.h>
#include "basic_element.h"

namespace ggadget {

class DivElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xfca426268a584176, BasicElement);

  DivElement(ElementInterface *parent,
             ViewInterface *view,
             const char *name);
  virtual ~DivElement();

  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);
  virtual const char *GetTagName() const { return "div"; }

 public:
  /**
   * Gets and sets the autoscroll property.
   * @c true if the div automatically shows scrollbars if necessary; @c false
   * if it doesn't show scrollbars. Default is @c false. 
   */
  bool IsAutoscroll() const;
  void SetAutoscroll(bool autoscroll);

  /**
   * Gets and sets the background color or image of the element. The image is
   * repeated if necessary, not stretched.
   */
  const char *GetBackground() const;
  void SetBackground(const char *background);

  virtual bool OnMouseEvent(MouseEvent *event);
  virtual void OnKeyEvent(KeyboardEvent *event);

 public:
  static ElementInterface *CreateInstance(ElementInterface *parent,
                                          ViewInterface *view,
                                          const char *name);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DivElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGETS_DIV_ELEMENT_H__