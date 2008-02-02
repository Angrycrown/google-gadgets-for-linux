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

#ifndef GGADGET_SCRIPTABLE_HOLDER_H__
#define GGADGET_SCRIPTABLE_HOLDER_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>

namespace ggadget {

/**
 * Holds a @c ScriptableInterface pointer. If the held object is about to be
 * deleted, the pointer will be automatically reset to @c NULL.  
 */
template <typename T>
class ScriptableHolder {
 private:
  // Checks at compile time if the argument T is ScriptableInterface or
  // derived from it.
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                 T_must_be_ScriptableInterface_or_derived_from_it);
 public:
  explicit ScriptableHolder(T* p = NULL)
      : ptr_(NULL), on_refchange_connection_(NULL) { Reset(p); }
  ~ScriptableHolder() { Reset(NULL); }

  void Reset(T* p) {
    if (ptr_ == p) return;
    if (ptr_) {
      ASSERT(on_refchange_connection_);
      on_refchange_connection_->Disconnect();
      on_refchange_connection_ = NULL;
      ptr_->Unref();
    } else {
      ASSERT(!ptr_);
    }
    ptr_ = p;
    if (p) {
      on_refchange_connection_ = p->ConnectOnReferenceChange(
          NewSlot(this, &ScriptableHolder::OnRefChange));
      p->Ref();
    }
  }

  T *Get() const { return ptr_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableHolder);
  void OnRefChange(int ref_count, int change) {
    if (ref_count == 0 && change == 0)
      Reset(NULL);
  }

  T *ptr_;
  Connection *on_refchange_connection_;
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_HOLDER_H__
