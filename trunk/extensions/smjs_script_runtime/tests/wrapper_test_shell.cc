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

#include "ggadget/tests/scriptables.h"
#include "../js_script_context.h"

using namespace ggadget;
using namespace ggadget::smjs;

class GlobalObject : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x7067c76cc0d84d11, ScriptableInterface);
  GlobalObject()
      // "scriptable" is native owned and strict.
      // "scriptable2" is native owned and not strict.
      : test_scriptable2(true, false) {
    RegisterConstant("scriptable", &test_scriptable1);
    RegisterConstant("scriptable2", &test_scriptable2);
    // For testing name overriding.
    RegisterConstant("s1", &test_scriptable1);
    RegisterProperty("s2", NewSlot(this, &GlobalObject::GetS2), NULL);
  }
  virtual bool IsStrict() const { return false; }

  ScriptableInterface *ConstructScriptable() {
    // Return shared ownership object.
    return test_scriptable2.NewObject(false, true);
  }

  ScriptableInterface *GetS2() { return &test_scriptable2; }

  TestScriptable1 test_scriptable1;
  TestScriptable2 test_scriptable2;
};

static GlobalObject *global;

// Called by the initialization code in js_shell.cc.
JSBool InitCustomObjects(JSScriptContext *context) {
  global = new GlobalObject();
  context->SetGlobalObject(global);
  context->RegisterClass("TestScriptable",
                         NewSlot(global, &GlobalObject::ConstructScriptable));
  return JS_TRUE;
}

void DestroyCustomObjects(JSScriptContext *context) {
  delete global;
}
