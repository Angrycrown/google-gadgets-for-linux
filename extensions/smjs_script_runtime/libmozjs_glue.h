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

// This file is generated by gen_libmozjs_glue.sh automatically.
// Don't edit it manually.
// Don't forget to regenerate this file if more/less libmozjs functions are
// used.
#ifndef EXTENSIONS_SMJS_SCRIPT_RUNTIME_LIBMOZJS_GLUE_
#define EXTENSIONS_SMJS_SCRIPT_RUNTIME_LIBMOZJS_GLUE_

#include <jsapi.h>
#include <jsdhash.h>

// This file only makes sense when XPCOM_GLUE is defined.
#ifdef XPCOM_GLUE

#include <nsXPCOMGlue.h>

namespace ggadget {
namespace libmozjs {

#define MOZJS_API(type, name, params) \
    typedef type (* name##FuncType) params; \
    union name##Type { name##FuncType func; NSFuncPtr ptr; }

MOZJS_API(JSBool, JS_AddNamedRootRT, (JSRuntime *rt, void *rp, const char *name));
MOZJS_API(JSBool, JS_AddRoot, (JSContext *cx, void *rp));
MOZJS_API(JSBool, JS_BufferIsCompilableUnit, (JSContext *cx, JSObject *obj, const char *bytes, size_t length));
MOZJS_API(JSBool, JS_CallFunctionName, (JSContext *cx, JSObject *obj, const char *name, uintN argc, jsval *argv, jsval *rval));
MOZJS_API(JSBool, JS_CallFunctionValue, (JSContext *cx, JSObject *obj, jsval fval, uintN argc, jsval *argv, jsval *rval));
MOZJS_API(void, JS_ClearPendingException, (JSContext *cx));
MOZJS_API(JSFunction *, JS_CompileFunction, (JSContext *cx, JSObject *obj, const char *name, uintN nargs, const char **argnames, const char *bytes, size_t length, const char *filename, uintN lineno));
MOZJS_API(JSFunction *, JS_CompileUCFunction, (JSContext *cx, JSObject *obj, const char *name, uintN nargs, const char **argnames, const jschar *chars, size_t length, const char *filename, uintN lineno));
MOZJS_API(JSScript *, JS_CompileUCScript, (JSContext *cx, JSObject *obj, const jschar *chars, size_t length, const char *filename, uintN lineno));
MOZJS_API(JSBool, JS_ConvertStub, (JSContext *cx, JSObject *obj, JSType type, jsval *vp));
MOZJS_API(JSFunction *, JS_DefineFunction, (JSContext *cx, JSObject *obj, const char *name, JSNative call, uintN nargs, uintN attrs));
MOZJS_API(JSBool, JS_DefineFunctions, (JSContext *cx, JSObject *obj, JSFunctionSpec *fs));
MOZJS_API(JSBool, JS_DefineProperty, (JSContext *cx, JSObject *obj, const char *name, jsval value, JSPropertyOp getter, JSPropertyOp setter, uintN attrs));
MOZJS_API(JSBool, JS_DeleteProperty, (JSContext *cx, JSObject *obj, const char *name));
MOZJS_API(void, JS_DestroyContext, (JSContext *cx));
MOZJS_API(void, JS_DestroyIdArray, (JSContext *cx, JSIdArray *ida));
MOZJS_API(void, JS_Finish, (JSRuntime *rt));
MOZJS_API(void, JS_DestroyScript, (JSContext *cx, JSScript *script));
MOZJS_API(uint32, JS_DHashTableEnumerate, (JSDHashTable *table, JSDHashEnumerator etor, void *arg));
MOZJS_API(JSBool, JS_EnterLocalRootScope, (JSContext *cx));
MOZJS_API(JSIdArray *, JS_Enumerate, (JSContext *cx, JSObject *obj));
MOZJS_API(JSBool, JS_EnumerateStub, (JSContext *cx, JSObject *obj));
MOZJS_API(JSBool, JS_EvaluateScript, (JSContext *cx, JSObject *obj, const char *bytes, uintN length, const char *filename, uintN lineno, jsval *rval));
MOZJS_API(JSBool, JS_EvaluateUCScript, (JSContext *cx, JSObject *obj, const jschar *chars, uintN length, const char *filename, uintN lineno, jsval *rval));
MOZJS_API(JSBool, JS_ExecuteScript, (JSContext *cx, JSObject *obj, JSScript *script, jsval *rval));
MOZJS_API(void, JS_GC, (JSContext *cx));
MOZJS_API(JSBool, JS_GetArrayLength, (JSContext *cx, JSObject *obj, jsuint *lengthp));
MOZJS_API(void *, JS_GetContextPrivate, (JSContext *cx));
MOZJS_API(JSBool, JS_GetElement, (JSContext *cx, JSObject *obj, jsint index, jsval *vp));
MOZJS_API(const char *, JS_GetFunctionName, (JSFunction *fun));
MOZJS_API(JSObject *, JS_GetFunctionObject, (JSFunction *fun));
MOZJS_API(JSObject *, JS_GetGlobalObject, (JSContext *cx));
MOZJS_API(uint32, JS_GetOptions, (JSContext *cx));
MOZJS_API(JSBool, JS_GetPendingException, (JSContext *cx, jsval *vp));
MOZJS_API(void *, JS_GetPrivate, (JSContext *cx, JSObject *obj));
MOZJS_API(JSBool, JS_GetProperty, (JSContext *cx, JSObject *obj, const char *name, jsval *vp));
MOZJS_API(JSBool, JS_GetReservedSlot, (JSContext *cx, JSObject *obj, uint32 index, jsval *vp));
MOZJS_API(JSRuntime *, JS_GetRuntime, (JSContext *cx));
MOZJS_API(char *, JS_GetStringBytes, (JSString *str));
MOZJS_API(jschar *, JS_GetStringChars, (JSString *str));
MOZJS_API(size_t, JS_GetStringLength, (JSString *str));
MOZJS_API(JSBool, JS_GetUCProperty, (JSContext *cx, JSObject *obj, const jschar *name, size_t namelen, jsval *vp));
MOZJS_API(JSBool, JS_IdToValue, (JSContext *cx, jsid id, jsval *vp));
MOZJS_API(JSObject *, JS_InitClass, (JSContext *cx, JSObject *obj, JSObject *parent_proto, JSClass *clasp, JSNative constructor, uintN nargs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs));
MOZJS_API(JSBool, JS_InitStandardClasses, (JSContext *cx, JSObject *obj));
MOZJS_API(JSBool, JS_IsArrayObject, (JSContext *cx, JSObject *obj));
MOZJS_API(JSBool, JS_IsExceptionPending, (JSContext *cx));
MOZJS_API(void, JS_LeaveLocalRootScope, (JSContext *cx));
MOZJS_API(void, JS_MarkGCThing, (JSContext *cx, void *thing, const char *name, void *arg));
MOZJS_API(JSObject *, JS_NewArrayObject, (JSContext *cx, jsint length, jsval *vector));
MOZJS_API(JSContext *, JS_NewContext, (JSRuntime *rt, size_t stackChunkSize));
MOZJS_API(jsdouble *, JS_NewDouble, (JSContext *cx, jsdouble d));
MOZJS_API(JSFunction *, JS_NewFunction, (JSContext *cx, JSNative call, uintN nargs, uintN flags, JSObject *parent, const char *name));
MOZJS_API(JSObject *, JS_NewObject, (JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent));
MOZJS_API(JSRuntime *, JS_Init, (uint32 maxbytes));
MOZJS_API(JSString *, JS_NewString, (JSContext *cx, char *bytes, size_t length));
MOZJS_API(JSString *, JS_NewStringCopyN, (JSContext *cx, const char *s, size_t n));
MOZJS_API(JSString *, JS_NewStringCopyZ, (JSContext *cx, const char *s));
MOZJS_API(JSString *, JS_NewUCStringCopyN, (JSContext *cx, const jschar *s, size_t n));
MOZJS_API(JSString *, JS_NewUCStringCopyZ, (JSContext *cx, const jschar *s));
MOZJS_API(JSBool, JS_PropertyStub, (JSContext *cx, JSObject *obj, jsval id, jsval *vp));
MOZJS_API(JSBool, JS_RemoveRoot, (JSContext *cx, void *rp));
MOZJS_API(JSBool, JS_RemoveRootRT, (JSRuntime *rt, void *rp));
MOZJS_API(void, JS_ReportError, (JSContext *cx, const char *format, ...));
MOZJS_API(void, JS_ReportErrorNumber, (JSContext *cx, JSErrorCallback errorCallback, void *userRef, const uintN errorNumber, ...));
MOZJS_API(JSBool, JS_ReportWarning, (JSContext *cx, const char *format, ...));
MOZJS_API(JSBool, JS_ResolveStub, (JSContext *cx, JSObject *obj, jsval id));
MOZJS_API(JSBranchCallback, JS_SetBranchCallback, (JSContext *cx, JSBranchCallback cb));
MOZJS_API(void, JS_SetContextPrivate, (JSContext *cx, void *data));
MOZJS_API(JSBool, JS_SetElement, (JSContext *cx, JSObject *obj, jsint index, jsval *vp));
MOZJS_API(JSErrorReporter, JS_SetErrorReporter, (JSContext *cx, JSErrorReporter er));
MOZJS_API(void, JS_SetGCParameter, (JSRuntime *rt, JSGCParamKey key, uint32 value));
MOZJS_API(void, JS_SetGlobalObject, (JSContext *cx, JSObject *obj));
MOZJS_API(void, JS_SetLocaleCallbacks, (JSContext *cx, JSLocaleCallbacks *callbacks));
#ifdef JS_OPERATION_WEIGHT_BASE
MOZJS_API(void, JS_SetOperationCallback, (JSContext *cx, JSOperationCallback callback, uint32 operationLimit));
#endif
MOZJS_API(uint32, JS_SetOptions, (JSContext *cx, uint32 options));
MOZJS_API(void, JS_SetPendingException, (JSContext *cx, jsval v));
MOZJS_API(JSBool, JS_SetPrivate, (JSContext *cx, JSObject *obj, void *data));
MOZJS_API(JSBool, JS_SetProperty, (JSContext *cx, JSObject *obj, const char *name, jsval *vp));
MOZJS_API(JSBool, JS_SetReservedSlot, (JSContext *cx, JSObject *obj, uint32 index, jsval v));
MOZJS_API(void, JS_SetRuntimePrivate, (JSRuntime *rt, void *data));
MOZJS_API(JSType, JS_TypeOfValue, (JSContext *cx, jsval v));
MOZJS_API(JSBool, JS_ValueToBoolean, (JSContext *cx, jsval v, JSBool *bp));
MOZJS_API(JSBool, JS_ValueToECMAInt32, (JSContext *cx, jsval v, int32 *ip));
MOZJS_API(JSFunction *, JS_ValueToFunction, (JSContext *cx, jsval v));
MOZJS_API(JSBool, JS_ValueToId, (JSContext *cx, jsval v, jsid *idp));
MOZJS_API(JSBool, JS_ValueToInt32, (JSContext *cx, jsval v, int32 *ip));
MOZJS_API(JSBool, JS_ValueToNumber, (JSContext *cx, jsval v, jsdouble *dp));
MOZJS_API(JSString *, JS_ValueToString, (JSContext *cx, jsval v));
#ifdef JS_THREADSAFE
MOZJS_API(JSClass *, JS_GetClass, (JSContext *cx, JSObject *obj));
#else
MOZJS_API(JSClass *, JS_GetClass, (JSObject *obj));
#endif

#undef MOZJS_API

#ifdef JS_OPERATION_WEIGHT_BASE
#define MOZJS_FUNC_JS_SetOoperationCallback \
  MOZJS_FUNC(JS_SetOperationCallback)
#else
#define MOZJS_FUNC_JS_SetOoperationCallback
#endif

#define MOZJS_FUNCTIONS \
  MOZJS_FUNC(JS_AddNamedRootRT) \
  MOZJS_FUNC(JS_AddRoot) \
  MOZJS_FUNC(JS_BufferIsCompilableUnit) \
  MOZJS_FUNC(JS_CallFunctionName) \
  MOZJS_FUNC(JS_CallFunctionValue) \
  MOZJS_FUNC(JS_ClearPendingException) \
  MOZJS_FUNC(JS_CompileFunction) \
  MOZJS_FUNC(JS_CompileUCFunction) \
  MOZJS_FUNC(JS_CompileUCScript) \
  MOZJS_FUNC(JS_ConvertStub) \
  MOZJS_FUNC(JS_DefineFunction) \
  MOZJS_FUNC(JS_DefineFunctions) \
  MOZJS_FUNC(JS_DefineProperty) \
  MOZJS_FUNC(JS_DeleteProperty) \
  MOZJS_FUNC(JS_DestroyContext) \
  MOZJS_FUNC(JS_DestroyIdArray) \
  MOZJS_FUNC(JS_Finish) \
  MOZJS_FUNC(JS_DestroyScript) \
  MOZJS_FUNC(JS_DHashTableEnumerate) \
  MOZJS_FUNC(JS_EnterLocalRootScope) \
  MOZJS_FUNC(JS_Enumerate) \
  MOZJS_FUNC(JS_EnumerateStub) \
  MOZJS_FUNC(JS_EvaluateScript) \
  MOZJS_FUNC(JS_EvaluateUCScript) \
  MOZJS_FUNC(JS_ExecuteScript) \
  MOZJS_FUNC(JS_GC) \
  MOZJS_FUNC(JS_GetArrayLength) \
  MOZJS_FUNC(JS_GetContextPrivate) \
  MOZJS_FUNC(JS_GetElement) \
  MOZJS_FUNC(JS_GetFunctionName) \
  MOZJS_FUNC(JS_GetFunctionObject) \
  MOZJS_FUNC(JS_GetGlobalObject) \
  MOZJS_FUNC(JS_GetOptions) \
  MOZJS_FUNC(JS_GetPendingException) \
  MOZJS_FUNC(JS_GetPrivate) \
  MOZJS_FUNC(JS_GetProperty) \
  MOZJS_FUNC(JS_GetReservedSlot) \
  MOZJS_FUNC(JS_GetRuntime) \
  MOZJS_FUNC(JS_GetStringBytes) \
  MOZJS_FUNC(JS_GetStringChars) \
  MOZJS_FUNC(JS_GetStringLength) \
  MOZJS_FUNC(JS_GetUCProperty) \
  MOZJS_FUNC(JS_IdToValue) \
  MOZJS_FUNC(JS_InitClass) \
  MOZJS_FUNC(JS_InitStandardClasses) \
  MOZJS_FUNC(JS_IsArrayObject) \
  MOZJS_FUNC(JS_IsExceptionPending) \
  MOZJS_FUNC(JS_LeaveLocalRootScope) \
  MOZJS_FUNC(JS_MarkGCThing) \
  MOZJS_FUNC(JS_NewArrayObject) \
  MOZJS_FUNC(JS_NewContext) \
  MOZJS_FUNC(JS_NewDouble) \
  MOZJS_FUNC(JS_NewFunction) \
  MOZJS_FUNC(JS_NewObject) \
  MOZJS_FUNC(JS_Init) \
  MOZJS_FUNC(JS_NewString) \
  MOZJS_FUNC(JS_NewStringCopyN) \
  MOZJS_FUNC(JS_NewStringCopyZ) \
  MOZJS_FUNC(JS_NewUCStringCopyN) \
  MOZJS_FUNC(JS_NewUCStringCopyZ) \
  MOZJS_FUNC(JS_PropertyStub) \
  MOZJS_FUNC(JS_RemoveRoot) \
  MOZJS_FUNC(JS_RemoveRootRT) \
  MOZJS_FUNC(JS_ReportError) \
  MOZJS_FUNC(JS_ReportErrorNumber) \
  MOZJS_FUNC(JS_ReportWarning) \
  MOZJS_FUNC(JS_ResolveStub) \
  MOZJS_FUNC(JS_SetBranchCallback) \
  MOZJS_FUNC(JS_SetContextPrivate) \
  MOZJS_FUNC(JS_SetElement) \
  MOZJS_FUNC(JS_SetErrorReporter) \
  MOZJS_FUNC(JS_SetGCParameter) \
  MOZJS_FUNC(JS_SetGlobalObject) \
  MOZJS_FUNC(JS_SetLocaleCallbacks) \
  MOZJS_FUNC(JS_SetOptions) \
  MOZJS_FUNC(JS_SetPendingException) \
  MOZJS_FUNC(JS_SetPrivate) \
  MOZJS_FUNC(JS_SetProperty) \
  MOZJS_FUNC(JS_SetReservedSlot) \
  MOZJS_FUNC(JS_SetRuntimePrivate) \
  MOZJS_FUNC(JS_TypeOfValue) \
  MOZJS_FUNC(JS_ValueToBoolean) \
  MOZJS_FUNC(JS_ValueToECMAInt32) \
  MOZJS_FUNC(JS_ValueToFunction) \
  MOZJS_FUNC(JS_ValueToId) \
  MOZJS_FUNC(JS_ValueToInt32) \
  MOZJS_FUNC(JS_ValueToNumber) \
  MOZJS_FUNC(JS_ValueToString) \
  MOZJS_FUNC(JS_GetClass) \
  MOZJS_FUNC_JS_SetOoperationCallback

#define MOZJS_FUNC(fname) extern fname##Type fname;

MOZJS_FUNCTIONS

#undef MOZJS_FUNC

#define JS_AddNamedRootRT ggadget::libmozjs::JS_AddNamedRootRT.func
#define JS_AddRoot ggadget::libmozjs::JS_AddRoot.func
#define JS_BufferIsCompilableUnit ggadget::libmozjs::JS_BufferIsCompilableUnit.func
#define JS_CallFunctionName ggadget::libmozjs::JS_CallFunctionName.func
#define JS_CallFunctionValue ggadget::libmozjs::JS_CallFunctionValue.func
#define JS_ClearPendingException ggadget::libmozjs::JS_ClearPendingException.func
#define JS_CompileFunction ggadget::libmozjs::JS_CompileFunction.func
#define JS_CompileUCFunction ggadget::libmozjs::JS_CompileUCFunction.func
#define JS_CompileUCScript ggadget::libmozjs::JS_CompileUCScript.func
#define JS_DefineFunction ggadget::libmozjs::JS_DefineFunction.func
#define JS_DefineFunctions ggadget::libmozjs::JS_DefineFunctions.func
#define JS_DefineProperty ggadget::libmozjs::JS_DefineProperty.func
#define JS_DeleteProperty ggadget::libmozjs::JS_DeleteProperty.func
#define JS_DestroyContext ggadget::libmozjs::JS_DestroyContext.func
#define JS_DestroyIdArray ggadget::libmozjs::JS_DestroyIdArray.func
#define JS_Finish ggadget::libmozjs::JS_Finish.func
#define JS_DestroyScript ggadget::libmozjs::JS_DestroyScript.func
#define JS_DHashTableEnumerate ggadget::libmozjs::JS_DHashTableEnumerate.func
#define JS_EnterLocalRootScope ggadget::libmozjs::JS_EnterLocalRootScope.func
#define JS_Enumerate ggadget::libmozjs::JS_Enumerate.func
#define JS_EvaluateScript ggadget::libmozjs::JS_EvaluateScript.func
#define JS_EvaluateUCScript ggadget::libmozjs::JS_EvaluateUCScript.func
#define JS_ExecuteScript ggadget::libmozjs::JS_ExecuteScript.func
#define JS_GC ggadget::libmozjs::JS_GC.func
#define JS_GetArrayLength ggadget::libmozjs::JS_GetArrayLength.func
#define JS_GetContextPrivate ggadget::libmozjs::JS_GetContextPrivate.func
#define JS_GetElement ggadget::libmozjs::JS_GetElement.func
#define JS_GetFunctionName ggadget::libmozjs::JS_GetFunctionName.func
#define JS_GetFunctionObject ggadget::libmozjs::JS_GetFunctionObject.func
#define JS_GetGlobalObject ggadget::libmozjs::JS_GetGlobalObject.func
#define JS_GetOptions ggadget::libmozjs::JS_GetOptions.func
#define JS_GetPendingException ggadget::libmozjs::JS_GetPendingException.func
#define JS_GetPrivate ggadget::libmozjs::JS_GetPrivate.func
#define JS_GetProperty ggadget::libmozjs::JS_GetProperty.func
#define JS_GetReservedSlot ggadget::libmozjs::JS_GetReservedSlot.func
#define JS_GetRuntime ggadget::libmozjs::JS_GetRuntime.func
#define JS_GetStringBytes ggadget::libmozjs::JS_GetStringBytes.func
#define JS_GetStringChars ggadget::libmozjs::JS_GetStringChars.func
#define JS_GetStringLength ggadget::libmozjs::JS_GetStringLength.func
#define JS_GetUCProperty ggadget::libmozjs::JS_GetUCProperty.func
#define JS_IdToValue ggadget::libmozjs::JS_IdToValue.func
#define JS_InitClass ggadget::libmozjs::JS_InitClass.func
#define JS_InitStandardClasses ggadget::libmozjs::JS_InitStandardClasses.func
#define JS_IsArrayObject ggadget::libmozjs::JS_IsArrayObject.func
#define JS_IsExceptionPending ggadget::libmozjs::JS_IsExceptionPending.func
#define JS_LeaveLocalRootScope ggadget::libmozjs::JS_LeaveLocalRootScope.func
#define JS_MarkGCThing ggadget::libmozjs::JS_MarkGCThing.func
#define JS_NewArrayObject ggadget::libmozjs::JS_NewArrayObject.func
#define JS_NewContext ggadget::libmozjs::JS_NewContext.func
#define JS_NewDouble ggadget::libmozjs::JS_NewDouble.func
#define JS_NewFunction ggadget::libmozjs::JS_NewFunction.func
#define JS_NewObject ggadget::libmozjs::JS_NewObject.func
#define JS_Init ggadget::libmozjs::JS_Init.func
#define JS_NewString ggadget::libmozjs::JS_NewString.func
#define JS_NewStringCopyN ggadget::libmozjs::JS_NewStringCopyN.func
#define JS_NewStringCopyZ ggadget::libmozjs::JS_NewStringCopyZ.func
#define JS_NewUCStringCopyN ggadget::libmozjs::JS_NewUCStringCopyN.func
#define JS_NewUCStringCopyZ ggadget::libmozjs::JS_NewUCStringCopyZ.func
#define JS_RemoveRoot ggadget::libmozjs::JS_RemoveRoot.func
#define JS_RemoveRootRT ggadget::libmozjs::JS_RemoveRootRT.func
#define JS_ReportError ggadget::libmozjs::JS_ReportError.func
#define JS_ReportErrorNumber ggadget::libmozjs::JS_ReportErrorNumber.func
#define JS_ReportWarning ggadget::libmozjs::JS_ReportWarning.func
#define JS_SetBranchCallback ggadget::libmozjs::JS_SetBranchCallback.func
#define JS_SetContextPrivate ggadget::libmozjs::JS_SetContextPrivate.func
#define JS_SetElement ggadget::libmozjs::JS_SetElement.func
#define JS_SetErrorReporter ggadget::libmozjs::JS_SetErrorReporter.func
#define JS_SetGCParameter ggadget::libmozjs::JS_SetGCParameter.func
#define JS_SetGlobalObject ggadget::libmozjs::JS_SetGlobalObject.func
#define JS_SetLocaleCallbacks ggadget::libmozjs::JS_SetLocaleCallbacks.func
#define JS_SetOperationCallback ggadget::libmozjs::JS_SetOperationCallback.func
#define JS_SetOptions ggadget::libmozjs::JS_SetOptions.func
#define JS_SetPendingException ggadget::libmozjs::JS_SetPendingException.func
#define JS_SetPrivate ggadget::libmozjs::JS_SetPrivate.func
#define JS_SetProperty ggadget::libmozjs::JS_SetProperty.func
#define JS_SetReservedSlot ggadget::libmozjs::JS_SetReservedSlot.func
#define JS_SetRuntimePrivate ggadget::libmozjs::JS_SetRuntimePrivate.func
#define JS_TypeOfValue ggadget::libmozjs::JS_TypeOfValue.func
#define JS_ValueToBoolean ggadget::libmozjs::JS_ValueToBoolean.func
#define JS_ValueToECMAInt32 ggadget::libmozjs::JS_ValueToECMAInt32.func
#define JS_ValueToFunction ggadget::libmozjs::JS_ValueToFunction.func
#define JS_ValueToId ggadget::libmozjs::JS_ValueToId.func
#define JS_ValueToInt32 ggadget::libmozjs::JS_ValueToInt32.func
#define JS_ValueToNumber ggadget::libmozjs::JS_ValueToNumber.func
#define JS_ValueToString ggadget::libmozjs::JS_ValueToString.func
#define JS_GetClass ggadget::libmozjs::JS_GetClass.func

// Stub functions are likely be used in static initialization code.
#define JS_ConvertStub ggadget::libmozjs::JS_ConvertStubProxy
#define JS_EnumerateStub ggadget::libmozjs::JS_EnumerateStubProxy
#define JS_PropertyStub ggadget::libmozjs::JS_PropertyStubProxy
#define JS_ResolveStub ggadget::libmozjs::JS_ResolveStubProxy

/**
 * Proxy function for JS_ConvertStub, to make sure it can be used in static
 * initialization code.
 */
JSBool JS_ConvertStubProxy(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

/**
 * Proxy function for JS_EnumerateStub, to make sure it can be used in static
 * initialization code.
 */
JSBool JS_EnumerateStubProxy(JSContext *cx, JSObject *obj);

/**
 * Proxy function for JS_PropertyStub, to make sure it can be used in static
 * initialization code.
 */
JSBool JS_PropertyStubProxy(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

/**
 * Proxy function for JS_ResolveStub, to make sure it can be used in static
 * initialization code.
 */
JSBool JS_ResolveStubProxy(JSContext *cx, JSObject *obj, jsval id);

/**
 * Starts up standalone libmozjs glue.
 * @return NS_OK if success.
 */
bool LibmozjsGlueStartup();

/**
 * Shuts down standalone libmozjs glue. libmozjs.so will be unloaded.
 */
void LibmozjsGlueShutdown();

/**
 * Starts up libmozjs glue along with XPCOM. Must be called after xpcom has
 * been started up.
 * @return NS_OK if success.
 */
nsresult LibmozjsGlueStartupWithXPCOM();

} // namespace libmozjs
} // namespace ggadget

#endif // XPCOM_GLUE
#endif // EXTENSIONS_SMJS_SCRIPT_RUNTIME_LIBMOZJS_GLUE_
