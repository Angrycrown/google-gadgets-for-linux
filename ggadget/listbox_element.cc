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

#include <cmath>

#include "event.h"
#include "item_element.h"
#include "listbox_element.h"
#include "list_elements.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "view.h"

namespace ggadget {

static const char kOnChangeEvent[] = "onchange";

class ListBoxElement::Impl {
 public:
  Impl(ListBoxElement *owner, View *view) : elements_(NULL) { 
    ASSERT(owner->GetChildren()->IsInstanceOf(ListElements::CLASS_ID));
    elements_ = down_cast<ListElements *>(owner->GetChildren());
  }

  ~Impl() {
  }

  ListElements *elements_;
  EventSignal onchange_event_;
};

ListBoxElement::ListBoxElement(BasicElement *parent, View *view,
                               const char *tag_name, const char *name)
    : DivElement(parent, view, tag_name, name,
                 new ListElements(view->GetElementFactory(), this, view)),
      impl_(new Impl(this, view)) {
  SetEnabled(true);  

  RegisterProperty("background",
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::GetBackground),
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::SetBackground));
  RegisterProperty("autoscroll",
                   NewSlot(implicit_cast<ScrollingElement *>(this), 
                           &ScrollingElement::IsAutoscroll),
                   NewSlot(implicit_cast<ScrollingElement *>(this), 
                           &ScrollingElement::SetAutoscroll));
  RegisterProperty("itemHeight",
                   NewSlot(impl_->elements_, &ListElements::GetItemHeight),
                   NewSlot(impl_->elements_, &ListElements::SetItemHeight));
  RegisterProperty("itemWidth",
                   NewSlot(impl_->elements_, &ListElements::GetItemWidth),
                   NewSlot(impl_->elements_, &ListElements::SetItemWidth));
  RegisterProperty("itemOverColor",
                   NewSlot(impl_->elements_, &ListElements::GetItemOverColor),
                   NewSlot(impl_->elements_, &ListElements::SetItemOverColor));
  RegisterProperty("itemSelectedColor",
                   NewSlot(impl_->elements_, &ListElements::GetItemSelectedColor),
                   NewSlot(impl_->elements_, &ListElements::SetItemSelectedColor));
  RegisterProperty("itemSeparator",
                   NewSlot(impl_->elements_, &ListElements::HasItemSeparator),
                   NewSlot(impl_->elements_, &ListElements::SetItemSeparator));
  RegisterProperty("multiSelect",
                   NewSlot(impl_->elements_, &ListElements::IsMultiSelect),
                   NewSlot(impl_->elements_, &ListElements::SetMultiSelect));
  RegisterProperty("selectedIndex",
                   NewSlot(impl_->elements_, &ListElements::GetSelectedIndex),
                   NewSlot(impl_->elements_, &ListElements::SetSelectedIndex));
  RegisterProperty("selectedItem",
                   NewSlot(impl_->elements_, &ListElements::GetSelectedItem),
                   NewSlot(impl_->elements_, &ListElements::SetSelectedItem));

  RegisterMethod("clearSelection",
                 NewSlot(impl_->elements_, &ListElements::ClearSelection));

  // Version 5.5 newly added methods and properties.
  RegisterProperty("itemSeparatorColor",
                   NewSlot(impl_->elements_, &ListElements::GetItemSeparatorColor),
                   NewSlot(impl_->elements_, &ListElements::SetItemSeparatorColor));
  RegisterMethod("appendString",
                 NewSlot(impl_->elements_, &ListElements::AppendString));
  RegisterMethod("insertStringAt",
                 NewSlot(impl_->elements_, &ListElements::InsertStringAt));
  RegisterMethod("removeString",
                 NewSlot(impl_->elements_, &ListElements::RemoveString));

  RegisterSignal(kOnChangeEvent, &impl_->onchange_event_);
}

ListBoxElement::~ListBoxElement() {
  delete impl_;
  impl_ = NULL;
}

Connection *ListBoxElement::ConnectEvent(const char *event_name,
                                           Slot0<void> *handler) {
  if (GadgetStrCmp(event_name, kOnChangeEvent) == 0)
    return impl_->onchange_event_.Connect(handler);
  return BasicElement::ConnectEvent(event_name, handler);
}

void ListBoxElement::ScrollToIndex(int index) {
  SetScrollYPosition(index * 
                     static_cast<int>(impl_->elements_->GetItemPixelHeight()));
}

Connection *ListBoxElement::ConnectOnChangeEvent(Slot0<void> *slot) {
  return impl_->onchange_event_.Connect(slot);
}

void ListBoxElement::FireOnChangeEvent() {
  Event event(Event::EVENT_CHANGE);
  ScriptableEvent s_event(&event, this, NULL);
  GetView()->FireEvent(&s_event, impl_->onchange_event_);
}

EventResult ListBoxElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                     BasicElement **fired_element,
                                     BasicElement **in_element) {
  // Interecept mouse wheel events from Item elements and send to Div 
  // directly to enable wheel scrolling.
  bool wheel = (event.GetType() == Event::EVENT_MOUSE_WHEEL);
  return DivElement::OnMouseEvent(event, wheel ? true : direct, 
                                  fired_element, in_element);  
}

BasicElement *ListBoxElement::CreateInstance(BasicElement *parent, View *view,
                                             const char *name) {
  return new ListBoxElement(parent, view, "listbox", name);
}

} // namespace ggadget
