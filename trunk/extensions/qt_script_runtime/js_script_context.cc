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
#include <map>
#include <string>
#include <ggadget/logger.h>
#include <ggadget/slot.h>
#include <ggadget/unicode_utils.h>
#include "js_script_context.h"
#include "js_function_slot.h"
#include "js_script_runtime.h"
#include "converter.h"

namespace ggadget {
namespace qt {

static std::map<QScriptEngine*, JSScriptContext::Impl*> g_data;
// String.substr is not ecma standard and qtscript doesn't provide it, so make
// our own
static QScriptValue substr(QScriptContext *context, QScriptEngine *engine) {
  QScriptValue self = context->thisObject();
  if (context->argumentCount() == 0) return self;
  int start = context->argument(0).toUInt32();
  int length = self.toString().length();
  if (context->argumentCount() >= 2)
    length = context->argument(1).toUInt32();
  return QScriptValue(engine, self.toString().mid(start, length));
}

class JSScriptContext::Impl {
 public:
  bool SetGlobalObject(ScriptableInterface *global_object) {
    resolver_ = new ResolverScriptClass(&engine_, global_object);
    engine_.globalObject().setPrototype(engine_.newObject(resolver_));
    QScriptValue string_prototype =
        engine_.globalObject().property("String").property("prototype");
    string_prototype.setProperty("substr", engine_.newFunction(substr));
    return true;
  }

  QScriptEngine engine_;
  std::map<std::string, Slot*> class_constructors_;
  Signal1<void, const char *> error_reporter_signal_;
  Signal2<bool, const char *, int> script_blocked_signal_;
  ResolverScriptClass *resolver_;
};

class SlotCallerWrapper : public QObject {
 public:
  SlotCallerWrapper(Slot* slot) : slot_(slot) {}
  Slot *slot_;
};

static QScriptValue SlotCaller(QScriptContext *context, QScriptEngine *engine) {
  QScriptValue callee = context->callee();
  SlotCallerWrapper *wrapper =
      static_cast<SlotCallerWrapper*>(callee.data().toQObject());
  ASSERT(wrapper);
  Variant *argv = NULL;
  bool ret = ConvertJSArgsToNative(context, wrapper->slot_, &argv);
  ASSERT(ret);
  ResultVariant res = wrapper->slot_->Call(wrapper->slot_->GetArgCount(), argv);
/*  if (argv) {
    for (int i = 0; i < context->argumentCount(); i++)
      FreeNativeValue(argv[i]);
    delete [] argv;
  }*/
  if (context->isCalledAsConstructor()) {
    ScriptableInterface *scriptable =
        VariantValue<ScriptableInterface *>()(res.v());
    ResolverScriptClass *resolver = new ResolverScriptClass(engine, scriptable);
    context->thisObject().setScriptClass(resolver);
    return engine->undefinedValue();
  } else {
    QScriptValue val;
    ret = ConvertNativeToJS(engine, res.v(), &val);
    ASSERT(ret);
    return val;
  }
}

ResolverScriptClass::ResolverScriptClass(QScriptEngine *engine,
                                         ScriptableInterface *object)
    : QScriptClass(engine), object_(object),
      on_reference_change_connection_(NULL) {
  if (object) {
    DLOG("Ref:%p, %p,%d", this, object_, object_->GetRefCount());
    object->Ref();
    on_reference_change_connection_ = object->ConnectOnReferenceChange(
        NewSlot(this, &ResolverScriptClass::OnRefChange));
  }
}

ResolverScriptClass::~ResolverScriptClass() {
  DLOG("ResolverScriptClass:Destructed");
  object_->Unref();
}

void ResolverScriptClass::OnRefChange(int ref_count, int change) {
  if (ref_count == 0 && change == 0) {
    // LOG("OnRefChange:%p, %p,%d", this, object_, object_->GetRefCount());
    object_->Unref(true);
    on_reference_change_connection_->Disconnect();
    object_ = NULL;
  }
}

QScriptClass::QueryFlags ResolverScriptClass::queryProperty(
    const QScriptValue &object,
    const QScriptString &property_name,
    QueryFlags flags,
    uint *id) {
  bool log = true;
  QString name = property_name.toString();
  std::string sname = name.toStdString();
  if (name.compare("debug", Qt::CaseInsensitive) == 0 ||
      name.compare("Trace", Qt::CaseInsensitive) == 0)
    log = false;
  if (log) DLOG("queryProperty %s", sname.c_str());
  JSScriptContext::Impl *impl = g_data[engine()];
  ASSERT(impl);
  if (impl->class_constructors_.find(sname) !=
      impl->class_constructors_.end()) {
    return HandlesReadAccess;
  }

  if (!object_) {
     DLOG("%s not found", sname.c_str());
    return 0;
  }

  ScriptableInterface::PropertyType pt =
      object_->GetPropertyInfo(sname.c_str(), NULL);
  if (pt == ScriptableInterface::PROPERTY_NOT_EXIST) {
    bool ok;
    name.toLong(&ok, 0);
    if (!ok) {
      DLOG("%s not found", sname.c_str());
      return 0;  // This property is not maintained by resolver
    }
    return HandlesReadAccess|HandlesWriteAccess; // Accessed as array
  }
  if (pt == ScriptableInterface::PROPERTY_CONSTANT)
    return HandlesReadAccess;
  return HandlesReadAccess|HandlesWriteAccess;
}

QScriptValue ResolverScriptClass::property(const QScriptValue & object,
                                           const QScriptString & name,
                                           uint id) {
  bool log = true;
  if (name.toString().compare("debug", Qt::CaseInsensitive) == 0||
      name.toString().compare("Trace", Qt::CaseInsensitive) == 0)
    log = false;
  std::string sname = name.toString().toStdString();
  if (log) DLOG("property %s", sname.c_str());
  JSScriptContext::Impl *impl = g_data[engine()];
  if (impl->class_constructors_.find(sname) !=
      impl->class_constructors_.end()) {
    if (log) DLOG("\tctor");
    Slot *slot = impl->class_constructors_[sname];
    QScriptValue value = engine()->newFunction(SlotCaller);
    QScriptValue data = engine()->newQObject(new SlotCallerWrapper(slot));
    value.setData(data);
    return value;
  }
  bool ok;
  long i = name.toString().toLong(&ok, 0);
  ResultVariant res;
  if (ok) {
    res = object_->GetPropertyByIndex(i);
  } else {
    res = object_->GetProperty(sname.c_str());
  }
  if (res.v().type() == Variant::TYPE_VOID) {
    return QScriptValue();
  } else if (res.v().type() == Variant::TYPE_SLOT) {
    QScriptValue value = engine()->newFunction(SlotCaller);
    Slot *slot = VariantValue<Slot *>()(res.v());
    if (log) DLOG("\tfun::%p", slot);
    QScriptValue data = engine()->newQObject(new SlotCallerWrapper(slot));
    value.setData(data);
    return value;
  } else if (res.v().type() == Variant::TYPE_SCRIPTABLE) {
    if (log) DLOG("\tscriptable");
    ScriptableInterface *s = VariantValue<ScriptableInterface*>()(res.v());
    if (s) {
      return engine()->newObject(new ResolverScriptClass(engine(), s));
    } else {
      return engine()->nullValue();
    }
  } else {
    if (log) DLOG("\tothers:%s", res.v().Print().c_str());
    QScriptValue qval;
    ConvertNativeToJS(engine(), res.v(), &qval);
    return qval;
  }
}
void ResolverScriptClass::setProperty(QScriptValue &object,
                                      const QScriptString &name,
                                      uint id,
                                      const QScriptValue &value) {
  std::string sname = name.toString().toStdString();
  DLOG("setProperty:%s", sname.c_str());
  Variant val;
  bool ok;
  long i = name.toString().toLong(&ok, 0);
  if (ok) {
    Variant proto(Variant::TYPE_INT64);
    ConvertJSToNative(engine(), proto, value, &val);
    object_->SetPropertyByIndex(i, val);
    DLOG("setPropertyByIndex:%s=%s", sname.c_str(), val.Print().c_str());
  } else {
    Variant proto;
    object_->GetPropertyInfo(sname.c_str(), &proto);
    ConvertJSToNative(engine(), proto, value, &val);
    DLOG("setProperty:proto:%s", proto.Print().c_str());
    object_->SetProperty(sname.c_str(), val);
    DLOG("setProperty:%s=%s", sname.c_str(), val.Print().c_str());
  }
}

JSScriptContext::JSScriptContext() : impl_(new Impl()){
  g_data[&impl_->engine_] = impl_;
}

JSScriptContext::~JSScriptContext() {
  delete impl_;
}

void JSScriptContext::Destroy() {
  delete this;
}

QScriptEngine *JSScriptContext::engine() const {
  return &impl_->engine_;
}

void JSScriptContext::Execute(const char *script,
                              const char *filename,
                              int lineno) {
  DLOG("Execute: (%s, %d)", filename, lineno);
  QScriptValue val = impl_->engine_.evaluate(script, filename, lineno);
  if (impl_->engine_.hasUncaughtException()) {
    QStringList bt = impl_->engine_.uncaughtExceptionBacktrace();
    LOG("Backtrace:");
    for (int i = 0; i < bt.size(); i++) {
      LOG("\t%s", bt[i].toStdString().c_str());
    }
  }
}

Slot *JSScriptContext::Compile(const char *script,
                               const char *filename,
                               int lineno) {
  DLOG("Compile: (%s, %d)", filename, lineno);
  DLOG("\t%s", script);
  return new JSFunctionSlot(NULL, &impl_->engine_, script, filename, lineno);
}

bool JSScriptContext::SetGlobalObject(ScriptableInterface *global_object) {
  return impl_->SetGlobalObject(global_object);
}

bool JSScriptContext::RegisterClass(const char *name, Slot *constructor) {
  ASSERT(constructor);
  ASSERT(constructor->GetReturnType() == Variant::TYPE_SCRIPTABLE);
  DLOG("RegisterClass: %s", name);
  impl_->class_constructors_[name] = constructor;
  return true;
}

bool JSScriptContext::AssignFromContext(ScriptableInterface *dest_object,
                                        const char *dest_object_expr,
                                        const char *dest_property,
                                        ScriptContextInterface *src_context,
                                        ScriptableInterface *src_object,
                                        const char *src_expr) {
  ASSERT(0);
  return false;
}

bool JSScriptContext::AssignFromNative(ScriptableInterface *object,
                                       const char *object_expr,
                                       const char *property,
                                       const Variant &value) {
  DLOG("AssignFromNative: o:%s,p:%s,v:%s", object_expr, property,
      value.Print().c_str());
  QScriptValue obj;
  if (!object_expr) {
    obj = impl_->engine_.globalObject();
  } else {
    obj = impl_->engine_.globalObject().property(object_expr);
    if (!obj.isValid()) return false;
  }
  QScriptValue qval;
  if (!ConvertNativeToJS(&impl_->engine_, value, &qval)) return false;
  obj.setProperty(property, qval);
  return true;
}

Variant JSScriptContext::Evaluate(ScriptableInterface *object,
                                  const char *expr) {
  DLOG("Evaluate: %s", expr);
  ASSERT(0);
  Variant result;
  return result;
}

Connection *JSScriptContext::ConnectScriptBlockedFeedback(
    ScriptBlockedFeedback *feedback) {
  return impl_->script_blocked_signal_.Connect(feedback);
}

void JSScriptContext::CollectGarbage() {
  impl_->engine_.collectGarbage();
}

void JSScriptContext::GetCurrentFileAndLine(std::string *fname, int *lineno) {
  fname->clear();
  lineno = 0;
}

} // namespace qt
} // namespace ggadget
