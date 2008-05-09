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

#ifndef GGADGET_VIEW_ELEMENT_H__
#define GGADGET_VIEW_ELEMENT_H__

#include <ggadget/basic_element.h>

namespace ggadget {

/**
 * Internally used element used to allow a View to be a child of another View.
 * This element is not exposed in the API.
 */
class ViewElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x3be02fb3f45b405b, BasicElement);

  ViewElement(BasicElement *parent, View *parent_view,
              View *child_view);
  virtual ~ViewElement();

  void SetChildView(View *child_view);
  View *GetChildView() const;

  /**
   * Delegates to child view's OnSizing() method, but does some extra things if
   * child view is not resizable.
   *
   * @param[in/out] width The desired width, and the actual allowed width will
   *                be returned.
   * @param[in/out] height The desired height, and the actual allowed height
   *                will be returned.
   * @return true if the resize request is allowed.
   */
  bool OnSizing(double *width, double *height);

  /**
   * Delegates to child view's SetSize() method.
   */
  void SetSize(double width, double height);

  /**
   * Sets the scale level of child view, which is relative to the zoom level of
   * parent view.
   */
  void SetScale(double scale);

  /**
   * Gets the scale level of child view, which is relative to the zoom level of
   * parent view.
   */
  double GetScale() const;

  /**
   * Converts coordinates in the child view's space to coordinates in the
   * parent view's space.
   */
  void ChildViewCoordToViewCoord(double child_x, double child_y,
                                 double *parent_x, double *parent_y) const;
 public:
  virtual EventResult OnMouseEvent(const MouseEvent &event,
                                   bool direct,
                                   BasicElement **fired_element,
                                   BasicElement **in_element);
  virtual EventResult OnOtherEvent(const Event &event);
  virtual EventResult OnDragEvent(const DragEvent &event);
  virtual EventResult OnKeyEvent(const KeyboardEvent &event);

  /**
   * The size of ViewElement will always be synced with the size of child view.
   */
  virtual double GetPixelWidth() const;
  virtual double GetPixelHeight() const;

  /**
   * Delegates to child view's GetHitTest().
   */
  virtual ViewInterface::HitTest GetHitTest() const;

  virtual void MarkRedraw();
  virtual bool OnAddContextMenuItems(MenuInterface *menu);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);

  virtual void GetDefaultSize(double *width, double *height) const;

  // No CreateInstance() method since this class is internal.

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ViewElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_VIEW_ELEMENT_H__
