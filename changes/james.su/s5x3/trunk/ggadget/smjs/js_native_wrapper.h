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

#ifndef GGADGET_SMJS_JS_NATIVE_WRAPPER_H__
#define GGADGET_SMJS_JS_NATIVE_WRAPPER_H__

#include <jsapi.h>
#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {
namespace smjs {

/**
 * Wraps a JavaScript object into a native @c ScriptableInterface.
 */
class JSNativeWrapper : public ScriptableHelperOwnershipShared {
 public:
  DEFINE_CLASS_ID(0x65f4d888b7b749ed, ScriptableInterface);

  JSNativeWrapper(JSContext *js_context, JSObject *js_object);
  virtual ~JSNativeWrapper();

  virtual OwnershipPolicy Attach();
  virtual bool Detach();
  virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback);
  virtual bool EnumerateElements(EnumerateElementsCallback *callback);

  Variant GetProperty(const char *name);
  bool SetProperty(const char *name, const Variant &value);
  Variant GetElement(int index);
  bool SetElement(int index, const Variant &value);

  JSContext *js_context() { return js_context_; }
  JSObject *js_object() { return js_object_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSNativeWrapper);
  static void FinalizeTracker(JSContext *cx, JSObject *obj);

  static JSClass js_reference_tracker_class_;
  int ref_count_;
  JSContext *js_context_;
  JSObject *js_object_;
};

} // namespace smjs
} // namespace ggadget

#endif // GGADGET_SMJS_JS_NATIVE_WRAPPER_H__