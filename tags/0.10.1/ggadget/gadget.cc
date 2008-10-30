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

#include "gadget.h"
#include "contentarea_element.h"
#include "content_item.h"
#include "details_view_data.h"
#include "display_window.h"
#include "element_factory.h"
#include "extension_manager.h"
#include "file_manager_interface.h"
#include "file_manager_factory.h"
#include "file_manager_wrapper.h"
#include "localized_file_manager.h"
#include "gadget_consts.h"
#include "logger.h"
#include "main_loop_interface.h"
#include "menu_interface.h"
#include "messages.h"
#include "host_interface.h"
#include "options_interface.h"
#include "permissions.h"
#include "script_context_interface.h"
#include "script_runtime_manager.h"
#include "scriptable_array.h"
#include "scriptable_framework.h"
#include "scriptable_helper.h"
#include "scriptable_menu.h"
#include "scriptable_options.h"
#include "scriptable_view.h"
#include "system_utils.h"
#include "view_host_interface.h"
#include "view.h"
#include "xml_http_request_interface.h"
#include "xml_parser_interface.h"
#include "xml_utils.h"
#include "messages.h"

namespace ggadget {

// Maximum allowed idle time (in milliseconds) after user interaction.
// IsInUserInteraction() returns true within this idle time.
static const uint64_t kMaxAllowedUserInteractionIdleTime = 10000;

class Gadget::Impl : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x6a3c396b3a544148, ScriptableInterface);

  /**
   * A class to bundles View, ScriptableView, ScriptContext, and
   * DetailsViewData together.
   */
  class ViewBundle {
   public:
    ViewBundle(ViewHostInterface *view_host,
               Gadget *gadget,
               ElementFactory *element_factory,
               ScriptableInterface *prototype,
               DetailsViewData *details,
               bool support_script)
      : context_(NULL),
        view_(NULL),
        scriptable_(NULL),
        details_(details) {
      if (support_script) {
        // Only xml based views have standalone script context.
        // FIXME: ScriptContext instance should be created on-demand, according
        // to the type of script files shipped in the gadget.
        // Or maybe we should add an option in gadget.gmanifest to specify what
        // ScriptRuntime implementation is required.
        // We may support multiple different script languages later.
        context_ = ScriptRuntimeManager::get()->CreateScriptContext("js");
        if (context_) {
          context_->ConnectScriptBlockedFeedback(
              NewSlot(this, &ViewBundle::OnScriptBlocked));
          ConnectContextLogListener(
              context_, NewSlot(gadget->impl_, &Impl::OnContextLog, context_));
        }
      }

      view_ = new View(view_host, gadget, element_factory, context_);
      if (details_)
        details_->Ref();
      if (context_)
        scriptable_ = new ScriptableView(view_, prototype, context_);
    }

    ~ViewBundle() {
      if (details_) {
        details_->Unref();
        details_ = NULL;
      }
      delete scriptable_;
      scriptable_ = NULL;
      delete view_;
      view_ = NULL;
      if (context_) {
        RemoveLogContext(context_);
        context_->Destroy();
        context_ = NULL;
      }
    }

    ScriptContextInterface *context() { return context_; }
    View *view() { return view_; }
    ScriptableView *scriptable() { return scriptable_; }
    DetailsViewData *details() { return details_; }

    bool OnScriptBlocked(const char *filename, int lineno) {
      ViewHostInterface *view_host = view_->GetViewHost();
      if (!view_host)
        return true; // Maybe in test environment, let the script continue.

      return !view_host->Confirm(
          view_, StringPrintf(GM_("SCRIPT_BLOCKED_MESSAGE"),
                              filename, lineno).c_str());
    }

   private:
    ScriptContextInterface *context_;
    View *view_;
    ScriptableView *scriptable_;
    DetailsViewData *details_;
  };

  Impl(Gadget *owner,
       HostInterface *host,
       const char *base_path,
       const char *options_name,
       int instance_id,
       const Permissions &global_permissions,
       DebugConsoleConfig debug_console_config)
      : owner_(owner),
        host_(host),
        element_factory_(new ElementFactory()),
        extension_manager_(ExtensionManager::CreateExtensionManager()),
        file_manager_(new FileManagerWrapper()),
        options_(CreateOptions(options_name)),
        scriptable_options_(new ScriptableOptions(options_, false)),
        main_view_(NULL),
        details_view_(NULL),
        old_details_view_(NULL),
        permissions_(global_permissions),
        base_path_(base_path),
        instance_id_(instance_id),
        initialized_(false),
        has_options_xml_(false),
        plugin_flags_(0),
        display_target_(TARGET_FLOATING_VIEW),
        xml_http_request_session_(GetXMLHttpRequestFactory()->CreateSession()),
        in_user_interaction_(false),
        remove_me_timer_(0),
        debug_console_config_(debug_console_config),
        last_user_interaction_time_(0) {
    // Checks if necessary objects are created successfully.
    ASSERT(host_);
    ASSERT(element_factory_);
    ASSERT(extension_manager_);
    ASSERT(file_manager_);
    ASSERT(options_);
    ASSERT(scriptable_options_);
  }

  ~Impl() {
    delete old_details_view_;
    old_details_view_ = NULL;
    delete details_view_;
    details_view_ = NULL;
    delete main_view_;
    main_view_ = NULL;
    delete scriptable_options_;
    scriptable_options_ = NULL;
    delete options_;
    options_ = NULL;
    delete file_manager_;
    file_manager_ = NULL;
    if (extension_manager_) {
      extension_manager_->Destroy();
      extension_manager_ = NULL;
    }
    delete element_factory_;
    element_factory_ = NULL;
    GetXMLHttpRequestFactory()->DestroySession(xml_http_request_session_);
    xml_http_request_session_ = 0;
  }

  static bool ExtractFileFromFileManager(FileManagerInterface *fm,
                                         const char *file,
                                         std::string *path) {
    path->clear();
    return fm->ExtractFile(file, path);
  }

  // Do real initialize.
  bool Initialize() {
    if (!host_ || !element_factory_ || !file_manager_ || !options_ ||
        !scriptable_options_)
      return false;

    // Create gadget FileManager
    FileManagerInterface *fm = CreateGadgetFileManager(base_path_.c_str());
    if (fm == NULL) return false;
    file_manager_->RegisterFileManager("", fm);

    // Create system FileManager
    fm = CreateFileManager(kDirSeparatorStr);
    if (fm) file_manager_->RegisterFileManager(kDirSeparatorStr, fm);

    // Load strings and manifest.
    if (!ReadStringsAndManifest(file_manager_, &strings_map_,
                                &manifest_info_map_))
      return false;

    std::string min_version = GetManifestInfo(kManifestMinVersion);
    DLOG("Gadget min version: %s", min_version.c_str());
    DLOG("Gadget id: %s", GetManifestInfo(kManifestId).c_str());
    DLOG("Gadget name: %s", GetManifestInfo(kManifestName).c_str());
    DLOG("Gadget description: %s",
         GetManifestInfo(kManifestDescription).c_str());

    int compare_result = 0;
    if (!CompareVersion(min_version.c_str(), GGL_API_VERSION,
                        &compare_result) ||
        compare_result > 0) {
      LOG("Gadget required version %s higher than supported version %s",
          min_version.c_str(), GGL_API_VERSION);
      return false;
    }

    // Load permissions information at very beginning, in case following
    // initialization code requires it.
    // permissions_ contains the global permissions.
    Permissions global_permissions = permissions_;

    // Clear permissions_.
    permissions_ = Permissions();
    Variant value = options_->GetInternalValue(kPermissionsOption);
    if (value.type() == Variant::TYPE_STRING) {
      permissions_.FromString(VariantValue<const char *>()(value));
      GetGadgetRequiredPermissions(&manifest_info_map_, &permissions_);
    } else {
      GetGadgetRequiredPermissions(&manifest_info_map_, &permissions_);
      // Grants all required permissions, and use it as initial permissions.
      permissions_.GrantAllRequired();
      Permissions temp = permissions_;
      temp.RemoveAllRequired();
      // Don't save required permissions.
      options_->PutInternalValue(kPermissionsOption, Variant(temp.ToString()));
    }

    // Denies all permissions which are denied explicitly in global
    // permissions.
    permissions_.SetGrantedByPermissions(global_permissions, false);
    DLOG("Permissions: %s", permissions_.ToString().c_str());

    // main view must be created before calling RegisterProperties();
    main_view_ = new ViewBundle(
        host_->NewViewHost(owner_, ViewHostInterface::VIEW_HOST_MAIN),
        owner_, element_factory_, &global_, NULL, true);
    ASSERT(main_view_);

    if (debug_console_config_ == DEBUG_CONSOLE_INITIAL)
      host_->ShowGadgetDebugConsole(owner_);

    // Register scriptable properties.
    RegisterProperties();
    RegisterStrings(&strings_map_, &global_);
    RegisterStrings(&strings_map_, &strings_);

    // load fonts and objects and check platform
    for (StringMap::const_iterator i = manifest_info_map_.begin();
         i != manifest_info_map_.end(); ++i) {
      const std::string &key = i->first;
      if (SimpleMatchXPath(key.c_str(), kManifestInstallFontSrc)) {
        const char *font_name = i->second.c_str();
        std::string path;
        // ignore return, error not fatal
        if (ExtractFileFromFileManager(file_manager_, font_name, &path))
          host_->LoadFont(path.c_str());
      } else if (SimpleMatchXPath(key.c_str(), kManifestInstallObjectSrc) &&
                 extension_manager_) {
        if (permissions_.IsRequired(Permissions::ALL_ACCESS) &&
            permissions_.IsGranted(Permissions::ALL_ACCESS)) {
          // Only trusted gadget can load local extensions.
          const char *module_name = i->second.c_str();
          std::string path;
          if (ExtractFileFromFileManager(file_manager_, module_name, &path))
            extension_manager_->LoadExtension(path.c_str(), false);
        } else {
          LOG("Local extension module is forbidden for untrusted gadgets.");
        }
      } else if (SimpleMatchXPath(key.c_str(), kManifestPlatformSupported)) {
        if (i->second == "no") {
          LOG("Gadget doesn't support platform %s", GGL_PLATFORM);
          return false;
        }
      } else if (SimpleMatchXPath(key.c_str(), kManifestPlatformMinVersion)) {
        if (!CompareVersion(i->second.c_str(), GGL_VERSION, &compare_result) ||
            compare_result > 0) {
          LOG("Gadget required platform version %s higher than supported "
              "version %s", i->second.c_str(), GGL_VERSION);
          return false;
        }
      }
    }

    // Register extensions
    const ExtensionManager *global_manager =
        ExtensionManager::GetGlobalExtensionManager();
    MultipleExtensionRegisterWrapper register_wrapper;
    ElementExtensionRegister element_register(element_factory_);
    FrameworkExtensionRegister framework_register(&framework_, owner_);
    FileManagerExtensionRegister fm_register(file_manager_);

    register_wrapper.AddExtensionRegister(&element_register);
    register_wrapper.AddExtensionRegister(&framework_register);
    register_wrapper.AddExtensionRegister(&fm_register);

    if (global_manager)
      global_manager->RegisterLoadedExtensions(&register_wrapper);
    if (extension_manager_)
      extension_manager_->RegisterLoadedExtensions(&register_wrapper);

    // Initialize main view.
    std::string main_xml;
    if (!file_manager_->ReadFile(kMainXML, &main_xml)) {
      LOG("Failed to load main.xml.");
      return false;
    }

    main_view_->view()->SetCaption(GetManifestInfo(kManifestName).c_str());
    RegisterScriptExtensions(main_view_->context());

    if (!main_view_->scriptable()->InitFromXML(main_xml, kMainXML)) {
      LOG("Failed to setup the main view");
      return false;
    }

    has_options_xml_ = file_manager_->FileExists(kOptionsXML, NULL);
    DLOG("Initialized View(%p) size: %f x %f", main_view_->view(),
         main_view_->view()->GetWidth(), main_view_->view()->GetHeight());

    // Connect signals to monitor display state change.
    main_view_->view()->ConnectOnMinimizeEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_MINIMIZED)));
    main_view_->view()->ConnectOnRestoreEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_RESTORED)));
    main_view_->view()->ConnectOnPopOutEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_POPPED_OUT)));
    // FIXME: Is it correct to send RESTORED when popped in?
    main_view_->view()->ConnectOnPopInEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_RESTORED)));
    main_view_->view()->ConnectOnSizeEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_RESIZED)));

    // Let gadget know the initial display target.
    ondisplaytargetchange_signal_(display_target_);
    return true;
  }

  void OnDisplayStateChanged(int state) {
    ondisplaystatechange_signal_(state);
  }

  // Register script extensions for a specified script context.
  // This method shall be called for all views' script contexts.
  void RegisterScriptExtensions(ScriptContextInterface *context) {
    ASSERT(context);
    const ExtensionManager *global_manager =
        ExtensionManager::GetGlobalExtensionManager();
    ScriptExtensionRegister script_register(context, owner_);

    if (global_manager)
      global_manager->RegisterLoadedExtensions(&script_register);
    if (extension_manager_)
      extension_manager_->RegisterLoadedExtensions(&script_register);
  }

  // Register all scriptable properties.
  void RegisterProperties() {
    RegisterConstant("debug", &debug_);
    RegisterConstant("storage", &storage_);

    // Register properties of gadget.debug.
    debug_.RegisterMethod("trace", NewSlot(this, &Impl::ScriptLog, LOG_TRACE));
    debug_.RegisterMethod("info", NewSlot(this, &Impl::ScriptLog, LOG_INFO));
    debug_.RegisterMethod("warning", NewSlot(this, &Impl::ScriptLog,
                                             LOG_WARNING));
    debug_.RegisterMethod("error", NewSlot(this, &Impl::ScriptLog, LOG_ERROR));

    // Register properties of gadget.storage.
    storage_.RegisterMethod("extract", NewSlot(this, &Impl::ExtractFile));
    storage_.RegisterMethod("openText", NewSlot(this, &Impl::OpenTextFile));

    // Register properties of plugin.
    plugin_.RegisterProperty("plugin_flags", NULL, // No getter.
                NewSlot(this, &Impl::SetPluginFlags));
    plugin_.RegisterProperty("title", NULL, // No getter.
                NewSlot(main_view_->view(), &View::SetCaption));
    plugin_.RegisterProperty("window_width",
                NewSlot(main_view_->view(), &View::GetWidth), NULL);
    plugin_.RegisterProperty("window_height",
                NewSlot(main_view_->view(), &View::GetHeight), NULL);

    plugin_.RegisterMethod("RemoveMe",
                NewSlot(this, &Impl::RemoveMe));
    plugin_.RegisterMethod("ShowDetailsView",
                NewSlot(this, &Impl::ShowDetailsViewProxy));
    plugin_.RegisterMethod("CloseDetailsView",
                NewSlot(this, &Impl::CloseDetailsView));
    plugin_.RegisterMethod("ShowOptionsDialog",
                NewSlot(this, &Impl::ShowOptionsDialog));

    plugin_.RegisterSignal("onShowOptionsDlg",
                           &onshowoptionsdlg_signal_);
    plugin_.RegisterSignal("onAddCustomMenuItems",
                           &onaddcustommenuitems_signal_);
    plugin_.RegisterSignal("onCommand",
                           &oncommand_signal_);
    plugin_.RegisterSignal("onDisplayStateChange",
                           &ondisplaystatechange_signal_);
    plugin_.RegisterSignal("onDisplayTargetChange",
                           &ondisplaytargetchange_signal_);

    // Deprecated or unofficial properties and methods.
    plugin_.RegisterProperty("about_text", NULL, // No getter.
                             NewSlot(this, &Impl::SetAboutText));
    plugin_.RegisterMethod("SetFlags", NewSlot(this, &Impl::SetFlags));
    plugin_.RegisterMethod("SetIcons", NewSlot(this, &Impl::SetIcons));

    // Register properties and methods for content area.
    plugin_.RegisterProperty("contant_flags", NULL, // Write only.
                             NewSlot(this, &Impl::SetContentFlags));
    plugin_.RegisterProperty("max_content_items",
                             NewSlot(this, &Impl::GetMaxContentItems),
                             NewSlot(this, &Impl::SetMaxContentItems));
    plugin_.RegisterProperty("content_items",
                             NewSlot(this, &Impl::GetContentItems),
                             NewSlot(this, &Impl::SetContentItems));
    plugin_.RegisterProperty("pin_images",
                             NewSlot(this, &Impl::GetPinImages),
                             NewSlot(this, &Impl::SetPinImages));
    plugin_.RegisterMethod("AddContentItem",
                           NewSlot(this, &Impl::AddContentItem));
    plugin_.RegisterMethod("RemoveContentItem",
                           NewSlot(this, &Impl::RemoveContentItem));
    plugin_.RegisterMethod("RemoveAllContentItems",
                           NewSlot(this, &Impl::RemoveAllContentItems));

    // Register global properties.
    global_.RegisterConstant("gadget", this);
    global_.RegisterConstant("options", scriptable_options_);
    global_.RegisterConstant("strings", &strings_);
    global_.RegisterConstant("plugin", &plugin_);
    global_.RegisterConstant("pluginHelper", &plugin_);

    // As an unofficial feature, "gadget.debug" and "gadget.storage" can also
    // be accessed as "debug" and "storage" global objects.
    global_.RegisterConstant("debug", &debug_);
    global_.RegisterConstant("storage", &storage_);

    // Properties and methods of framework can also be accessed directly as
    // globals.
    global_.RegisterConstant("framework", &framework_);
    global_.SetInheritsFrom(&framework_);
  }

  class RemoveMeWatchCallback : public WatchCallbackInterface {
   public:
    RemoveMeWatchCallback(HostInterface *host, Gadget *owner, bool save_data)
      : host_(host), owner_(owner), save_data_(save_data) {
    }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      host_->RemoveGadget(owner_, save_data_);
      return false;
    }
    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      delete this;
    }
   private:
    HostInterface *host_;
    Gadget *owner_;
    bool save_data_;
  };

  void RemoveMe(bool save_data) {
    if (!remove_me_timer_) {
      remove_me_timer_ = GetGlobalMainLoop()->AddTimeoutWatch(
          0, new RemoveMeWatchCallback(host_, owner_, save_data));
    }
  }

  void AboutMenuCallback(const char *) {
    host_->ShowGadgetAboutDialog(owner_);
  }

  void OptionsMenuCallback(const char *) {
    ShowOptionsDialog();
  }

  void RemoveMenuCallback(const char *) {
    RemoveMe(true);
  }

  void DebugConsoleMenuCallback(const char *) {
    host_->ShowGadgetDebugConsole(owner_);
  }

  void OnAddCustomMenuItems(MenuInterface *menu) {
    ScriptableMenu *smenu = new ScriptableMenu(owner_, menu);
    smenu->Ref();
    onaddcustommenuitems_signal_(smenu);
    // Some of the menu handler slots may still hold the reference.
    smenu->Unref();
    if (HasOptionsDialog()) {
      menu->AddItem(GM_("MENU_ITEM_OPTIONS"), 0,
                    NewSlot(this, &Impl::OptionsMenuCallback),
                    MenuInterface::MENU_ITEM_PRI_GADGET);
      menu->AddItem(NULL, 0, NULL, MenuInterface::MENU_ITEM_PRI_GADGET);
    }
    bool disable_about = GetManifestInfo(kManifestAboutText).empty() &&
                         !oncommand_signal_.HasActiveConnections();
    if (debug_console_config_ != DEBUG_CONSOLE_DISABLED) {
      menu->AddItem(GM_("MENU_ITEM_DEBUG_CONSOLE"), 0,
                    NewSlot(this, &Impl::DebugConsoleMenuCallback),
                    MenuInterface::MENU_ITEM_PRI_GADGET);
    }
    menu->AddItem(GM_("MENU_ITEM_ABOUT"),
                  disable_about ? MenuInterface::MENU_ITEM_FLAG_GRAYED : 0,
                  NewSlot(this, &Impl::AboutMenuCallback),
                  MenuInterface::MENU_ITEM_PRI_GADGET);
    menu->AddItem(GM_("MENU_ITEM_REMOVE"), 0,
                  NewSlot(this, &Impl::RemoveMenuCallback),
                  MenuInterface::MENU_ITEM_PRI_GADGET);
  }

  void SetDisplayTarget(DisplayTarget target) {
    // Fire the signal no matter whether the target is changed or not.
    // gtkmoz browser element relies on this behaviour.
    display_target_ = target;
    ondisplaytargetchange_signal_(target);
  }

  void SetPluginFlags(int flags) {
    bool changed = (flags != plugin_flags_);
    plugin_flags_ = flags;
    if (changed)
      onpluginflagschanged_signal_(flags);
  }

  void SetFlags(int plugin_flags, int content_flags) {
    SetPluginFlags(plugin_flags);
    SetContentFlags(content_flags);
  }

  void SetIcons(const Variant &param1, const Variant &param2) {
    LOG("pluginHelper.SetIcons is no longer supported. "
        "Please specify icons in the manifest file.");
  }

  void SetContentFlags(int flags) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->SetContentFlags(flags);
  }

  size_t GetMaxContentItems() {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    return content_area ? content_area->GetMaxContentItems() : 0;
  }

  void SetMaxContentItems(size_t max_content_items) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->SetMaxContentItems(max_content_items);
  }

  ScriptableArray *GetContentItems() {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    return content_area ? content_area->ScriptGetContentItems() : NULL;
  }

  void SetContentItems(ScriptableInterface *array) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->ScriptSetContentItems(array);
  }

  ScriptableArray *GetPinImages() {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    return content_area ? content_area->ScriptGetPinImages() : NULL;
  }

  void SetPinImages(ScriptableInterface *array) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->ScriptSetPinImages(array);
  }

  void AddContentItem(ContentItem *item,
                      ContentAreaElement::DisplayOptions options) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->AddContentItem(item, options);
  }

  void RemoveContentItem(ContentItem *item) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->RemoveContentItem(item);
  }

  void RemoveAllContentItems() {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->RemoveAllContentItems();
  }

  void SetAboutText(const char *about_text) {
    manifest_info_map_[kManifestAboutText] = about_text;
  }

  std::string OnContextLog(LogLevel level, const char *filename, int line,
                           const std::string &message,
                           ScriptContextInterface *context) {
    std::string real_message;
    std::string script_filename;
    int script_line;
    if (context)
      context->GetCurrentFileAndLine(&script_filename, &script_line);
    if (script_filename.empty() ||
        strncmp(script_filename.c_str(), message.c_str(),
                script_filename.size()) == 0) {
      real_message = message;
    } else {
      StringAppendPrintf(&real_message, "%s:%d: %s",
                         script_filename.c_str(), script_line, message.c_str());
    }
    log_signal_(level, real_message);
    return real_message;
  }

  void ScriptLog(const char *message, LogLevel level) {
    LogHelper(level, NULL, 0)("%s", message);
  }

  // ExtractFile and OpenTextFile only allow accessing gadget local files.
  static bool FileNameIsLocal(const char *filename) {
    return filename && filename[0] != '/' && filename[0] != '\\' &&
           strchr(filename, ':') == NULL;
  }

  std::string ExtractFile(const char *filename) {
    std::string extracted_file;
    return FileNameIsLocal(filename) &&
           file_manager_->ExtractFile(filename, &extracted_file) ?
        extracted_file : "";
  }

  std::string OpenTextFile(const char *filename) {
    std::string data;
    std::string result;
    if (FileNameIsLocal(filename) &&
        file_manager_->ReadFile(filename, &data) &&
        !DetectAndConvertStreamToUTF8(data, &result, NULL)) {
      LOG("gadget.storage.openText() failed to read text file: %s", filename);
    }
    return result;
  }

  std::string GetManifestInfo(const char *key) {
    GadgetStringMap::const_iterator it = manifest_info_map_.find(key);
    if (it == manifest_info_map_.end())
      return std::string();
    return it->second;
  }

  bool HasOptionsDialog() {
    return has_options_xml_ || onshowoptionsdlg_signal_.HasActiveConnections();
  }

  static void OptionsDialogCallback(int flag, ViewBundle *options_view) {
    if (options_view) {
      SimpleEvent event((flag == ViewInterface::OPTIONS_VIEW_FLAG_OK) ?
                        Event::EVENT_OK : Event::EVENT_CANCEL);
      options_view->view()->OnOtherEvent(event);
    }
  }

  bool ShowOptionsDialog() {
    bool ret = false;
    int flags = ViewInterface::OPTIONS_VIEW_FLAG_OK |
                ViewInterface::OPTIONS_VIEW_FLAG_CANCEL;

    if (onshowoptionsdlg_signal_.HasActiveConnections()) {
      ViewBundle options_view(
          host_->NewViewHost(owner_, ViewHostInterface::VIEW_HOST_OPTIONS),
          owner_, element_factory_, NULL, NULL, false);
      View *view = options_view.view();
      DisplayWindow *window = new DisplayWindow(view);
      Variant result = onshowoptionsdlg_signal_(window);
      if ((result.type() != Variant::TYPE_BOOL ||
           VariantValue<bool>()(result)) && window->AdjustSize()) {
        view->SetResizable(ViewInterface::RESIZABLE_FALSE);
        if (view->GetCaption().empty())
          view->SetCaption(main_view_->view()->GetCaption().c_str());
        ret = view->ShowView(true, flags,
                             NewSlot(OptionsDialogCallback, &options_view));
      } else {
        DLOG("gadget cancelled the options dialog.");
      }
      delete window;
    } else if (has_options_xml_) {
      ret = ShowXMLOptionsDialog(flags, kOptionsXML, NULL);
    } else {
      LOG("Failed to show options dialog because there is neither options.xml"
          "nor OnShowOptionsDlg handler");
    }
    return ret;
  }

  bool ShowXMLOptionsDialog(int flags, const char *xml_file,
                            ScriptableInterface *param) {
    bool ret = false;
    std::string xml;
    if (file_manager_->ReadFile(xml_file, &xml)) {
      ViewBundle options_view(
          host_->NewViewHost(owner_, ViewHostInterface::VIEW_HOST_OPTIONS),
          owner_, element_factory_, &global_, NULL, true);
      View *view = options_view.view();
      RegisterScriptExtensions(options_view.context());
      std::string full_path = file_manager_->GetFullPath(xml_file);
      if (options_view.scriptable()->InitFromXML(xml, full_path.c_str())) {
        // Allow XML options dialog to resize, but not zoom.
        if (view->GetResizable() == ViewInterface::RESIZABLE_ZOOM)
          view->SetResizable(ViewInterface::RESIZABLE_FALSE);
        if (view->GetCaption().empty())
          view->SetCaption(main_view_->view()->GetCaption().c_str());

        if (param) {
          // Set up the param variable in the opened options view.
          options_view.context()->AssignFromNative(NULL, "", "optionsViewData",
                                                   Variant(param));
        }

        ret = view->ShowView(true, flags,
                             NewSlot(OptionsDialogCallback, &options_view));
      } else {
        LOG("Failed to setup the XML view: %s", xml_file);
      }
    } else {
      LOG("Failed to load %s file from gadget package.", xml_file);
    }
    return ret;
  }

  bool ShowDetailsViewProxy(DetailsViewData *details_view_data,
                            const char *title, int flags,
                            Slot *callback) {
    Slot1<void, int> *feedback_handler =
        callback ? new SlotProxy1<void, int>(callback) : NULL;
    return ShowDetailsView(details_view_data, title, flags, feedback_handler);
  }

  bool ShowDetailsView(DetailsViewData *details_view_data,
                       const char *title, int flags,
                       Slot1<void, int> *feedback_handler) {
    // Reference details_view_data to prevent it from being destroyed by
    // JavaScript GC.
    if (details_view_data)
      details_view_data->Ref();

    CloseDetailsView();
    details_view_ = new ViewBundle(
        host_->NewViewHost(owner_, ViewHostInterface::VIEW_HOST_DETAILS),
        owner_, element_factory_, &global_, details_view_data, true);

    // details_view_data is now referenced by details_view_, so it's safe to
    // remove the reference.
    if (details_view_data)
      details_view_data->Unref();

    ScriptContextInterface *context = details_view_->context();
    ScriptableOptions *scriptable_data = details_view_->details()->GetData();
    OptionsInterface *data = scriptable_data->GetOptions();

    // Register script extensions.
    RegisterScriptExtensions(context);

    // Set up the detailsViewData variable in the opened details view.
    context->AssignFromNative(NULL, "", "detailsViewData",
                              Variant(scriptable_data));

    std::string xml;
    std::string xml_file;
    if (details_view_data->GetContentIsHTML() ||
        !details_view_data->GetContentIsView()) {
      if (details_view_data->GetContentIsHTML()) {
        xml_file = kHTMLDetailsView;
        ScriptableInterface *ext_obj = details_view_data->GetExternalObject();
        context->AssignFromNative(NULL, "", "external", Variant(ext_obj));
        data->PutValue("contentType", Variant("text/html"));
      } else {
        xml_file = kTextDetailsView;
        data->PutValue("contentType", Variant("text/plain"));
      }
      data->PutValue("content", Variant(details_view_data->GetText()));
      GetGlobalFileManager()->ReadFile(xml_file.c_str(), &xml);
    } else {
      xml_file = details_view_data->GetText();
      file_manager_->ReadFile(xml_file.c_str(), &xml);
    }

    if (xml.empty() ||
        !details_view_->scriptable()->InitFromXML(xml, xml_file.c_str())) {
      LOG("Failed to load details view from %s", xml_file.c_str());
      delete details_view_;
      details_view_ = NULL;
      return false;
    }

    // For details view, the caption set in xml file will be discarded.
    if (title && *title) {
      details_view_->view()->SetCaption(title);
    } else if (details_view_->view()->GetCaption().empty()) {
      details_view_->view()->SetCaption(
          main_view_->view()->GetCaption().c_str());
    }

    details_view_->view()->ShowView(title, flags, feedback_handler);
    return true;
  }

  void CloseDetailsView() {
    delete old_details_view_;

    if (details_view_)
      details_view_->view()->CloseView();

    old_details_view_ = details_view_;
    details_view_ = NULL;
  }

  bool SetInUserInteraction(bool in_user_interaction) {
    bool old_value = in_user_interaction_;
    in_user_interaction_ = in_user_interaction;
    // Record time when ending user interaction.
    if (old_value && !in_user_interaction_) {
      last_user_interaction_time_ = GetGlobalMainLoop()->GetCurrentTime();
    }
    return old_value;
  }

  bool IsInUserInteraction() {
    uint64_t now = GetGlobalMainLoop()->GetCurrentTime();
    return in_user_interaction_ || (now - last_user_interaction_time_) <=
        kMaxAllowedUserInteractionIdleTime;
  }

  bool OpenURL(const char *url) {
    Variant open_url;
    if (framework_.GetPropertyInfo("openUrl", &open_url) == PROPERTY_METHOD &&
        open_url.type() == Variant::TYPE_SLOT) {
      Variant argv[1] = { Variant(url) };
      ResultVariant result =
          VariantValue<Slot *>()(open_url)->Call(&framework_, 1, argv);
      return VariantValue<bool>()(result.v());
    }

    DLOG("No method to open url.");
    return false;
  }

  static void RegisterStrings(const StringMap *strings,
                              ScriptableHelperNativeOwnedDefault *scriptable) {
    for (StringMap::const_iterator it = strings->begin();
         it != strings->end(); ++it) {
      scriptable->RegisterConstant(it->first.c_str(), it->second);
    }
  }

  static bool ReadStringsAndManifest(FileManagerInterface *file_manager,
                                     StringMap *strings_map,
                                     StringMap *manifest_info_map) {
    // Load string table.
    std::string strings_data;
    if (file_manager->ReadFile(kStringsXML, &strings_data)) {
      std::string full_path = file_manager->GetFullPath(kStringsXML);
      if (!GetXMLParser()->ParseXMLIntoXPathMap(strings_data, NULL,
                                                full_path.c_str(),
                                                kStringsTag,
                                                NULL, kEncodingFallback,
                                                strings_map)) {
        return false;
      }
    }

    std::string manifest_contents;
    if (!file_manager->ReadFile(kGadgetGManifest, &manifest_contents))
      return false;

    std::string manifest_path = file_manager->GetFullPath(kGadgetGManifest);
    if (!GetXMLParser()->ParseXMLIntoXPathMap(manifest_contents,
                                              strings_map,
                                              manifest_path.c_str(),
                                              kGadgetTag,
                                              NULL, kEncodingFallback,
                                              manifest_info_map)) {
      return false;
    }
    return true;
  }

  static FileManagerInterface *CreateGadgetFileManager(const char *base_path) {
    std::string path, filename;
    SplitFilePath(base_path, &path, &filename);

    // Uses the parent path of base_path if it refers to a manifest file.
    if (filename != kGadgetGManifest)
      path = base_path;

    FileManagerInterface *fm = CreateFileManager(path.c_str());
    return fm ? new LocalizedFileManager(fm) : NULL;
  }

  NativeOwnedScriptable<UINT64_C(0x4edfd94b70f04da6)> global_;
  NativeOwnedScriptable<UINT64_C(0xb13b9595da304041)> debug_;
  NativeOwnedScriptable<UINT64_C(0xaf77f40a271f41d4)> storage_;
  NativeOwnedScriptable<UINT64_C(0x3f7cd747988f4ad0)> plugin_;
  NativeOwnedScriptable<UINT64_C(0x50bbf15b460e48c5)> framework_;
  NativeOwnedScriptable<UINT64_C(0x8ef87d420c384a46)> strings_;

  Signal1<Variant, DisplayWindow *> onshowoptionsdlg_signal_;
  Signal1<void, ScriptableMenu *> onaddcustommenuitems_signal_;
  Signal1<void, int> oncommand_signal_;
  Signal1<void, int> ondisplaystatechange_signal_;
  Signal1<void, int> ondisplaytargetchange_signal_;
  Signal1<void, int> onpluginflagschanged_signal_;
  Signal2<void, LogLevel, const std::string &> log_signal_;

  StringMap manifest_info_map_;
  StringMap strings_map_;

  Gadget *owner_;
  HostInterface *host_;
  ElementFactory *element_factory_;
  ExtensionManager *extension_manager_;
  FileManagerWrapper *file_manager_;
  OptionsInterface *options_;
  ScriptableOptions *scriptable_options_;

  ViewBundle *main_view_;
  ViewBundle *details_view_;
  ViewBundle *old_details_view_;

  Permissions permissions_;

  std::string base_path_;
  int instance_id_;
  bool initialized_;
  bool has_options_xml_;
  int plugin_flags_;
  DisplayTarget display_target_;
  int xml_http_request_session_;
  bool in_user_interaction_;
  int remove_me_timer_;
  int debug_console_config_;
  uint64_t last_user_interaction_time_;
};

Gadget::Gadget(HostInterface *host,
               const char *base_path,
               const char *options_name,
               int instance_id,
               const Permissions &initial_permissions,
               DebugConsoleConfig debug_console_config)
    : impl_(new Impl(this, host, base_path, options_name, instance_id,
                     initial_permissions, debug_console_config)) {
  // Let the gadget become a log context, so that all logs related to this
  // gadget can be outputted to correct debug console.
  ConnectContextLogListener(
      this, NewSlot(impl_, &Impl::OnContextLog,
                    static_cast<ScriptContextInterface *>(NULL)));

  // Initialize the gadget.
  ScopedLogContext log_context(this);
  impl_->initialized_ = impl_->Initialize();
}

Gadget::~Gadget() {
  delete impl_;
  impl_ = NULL;
  RemoveLogContext(this);
}

HostInterface *Gadget::GetHost() const {
  return impl_->host_;
}

void Gadget::RemoveMe(bool save_data) {
  impl_->RemoveMe(save_data);
}

bool Gadget::IsValid() const {
  return impl_->initialized_;
}

int Gadget::GetInstanceID() const {
  return impl_->instance_id_;
}

int Gadget::GetPluginFlags() const {
  return impl_->plugin_flags_;
}

Gadget::DisplayTarget Gadget::GetDisplayTarget() const {
  return impl_->display_target_;
}

void Gadget::SetDisplayTarget(DisplayTarget target) {
  impl_->SetDisplayTarget(target);
}

FileManagerInterface *Gadget::GetFileManager() const {
  return impl_->file_manager_;
}

OptionsInterface *Gadget::GetOptions() const {
  return impl_->options_;
}

View *Gadget::GetMainView() const {
  return impl_->main_view_ ? impl_->main_view_->view() : NULL;
}

std::string Gadget::GetManifestInfo(const char *key) const {
  return impl_->GetManifestInfo(key);
}

bool Gadget::ParseLocalizedXML(const std::string &xml,
                               const char *filename,
                               DOMDocumentInterface *xmldoc) const {
  return GetXMLParser()->ParseContentIntoDOM(xml, &impl_->strings_map_,
                                             filename, NULL,
                                             NULL, kEncodingFallback,
                                             xmldoc, NULL, NULL);
}

bool Gadget::ShowMainView() {
  ASSERT(IsValid());
  return impl_->main_view_->view()->ShowView(false, 0, NULL);
}

void Gadget::CloseMainView() {
  impl_->main_view_->view()->CloseView();
}

bool Gadget::HasOptionsDialog() const {
  return impl_->HasOptionsDialog();
}

bool Gadget::ShowOptionsDialog() {
  return impl_->ShowOptionsDialog();
}

bool Gadget::ShowXMLOptionsDialog(int flags, const char *xml_file,
                                  ScriptableInterface *param) {
  return impl_->ShowXMLOptionsDialog(flags, xml_file, param);
}

bool Gadget::ShowDetailsView(DetailsViewData *details_view_data,
                             const char *title, int flags,
                             Slot1<void, int> *feedback_handler) {
  return impl_->ShowDetailsView(details_view_data, title, flags,
                                feedback_handler);
}

void Gadget::CloseDetailsView() {
  return impl_->CloseDetailsView();
}

void Gadget::OnAddCustomMenuItems(MenuInterface *menu) {
  impl_->OnAddCustomMenuItems(menu);
}

void Gadget::OnCommand(Command command) {
  impl_->oncommand_signal_(command);
}

Connection *Gadget::ConnectOnDisplayStateChanged(Slot1<void, int> *handler) {
  return impl_->ondisplaystatechange_signal_.Connect(handler);
}

Connection *Gadget::ConnectOnDisplayTargetChanged(Slot1<void, int> *handler) {
  return impl_->ondisplaytargetchange_signal_.Connect(handler);
}

Connection *Gadget::ConnectOnPluginFlagsChanged(Slot1<void, int> *handler) {
  return impl_->onpluginflagschanged_signal_.Connect(handler);
}

XMLHttpRequestInterface *Gadget::CreateXMLHttpRequest() {
  if (impl_->permissions_.IsRequiredAndGranted(Permissions::NETWORK)) {
      return GetXMLHttpRequestFactory()->CreateXMLHttpRequest(
          impl_->xml_http_request_session_, GetXMLParser());
  }
  LOG("No permission to access network.");
  return NULL;
}

bool Gadget::SetInUserInteraction(bool in_user_interaction) {
  return impl_->SetInUserInteraction(in_user_interaction);
}

bool Gadget::IsInUserInteraction() const {
  return impl_->IsInUserInteraction();
}

bool Gadget::OpenURL(const char *url) const {
  return impl_->OpenURL(url);
}

Connection *Gadget::ConnectLogListener(
    Slot2<void, LogLevel, const std::string &> *listener) {
  return impl_->log_signal_.Connect(listener);
}

const Permissions* Gadget::GetPermissions() const {
  return &impl_->permissions_;
}

// static methods
bool Gadget::GetGadgetManifest(const char *base_path, StringMap *data) {
  ASSERT(base_path);
  ASSERT(data);

  FileManagerInterface *file_manager = Impl::CreateGadgetFileManager(base_path);
  if (!file_manager)
    return false;

  StringMap strings_map;
  bool result = Impl::ReadStringsAndManifest(file_manager, &strings_map, data);
  delete file_manager;
  return result;
}

bool Gadget::GetGadgetRequiredPermissions(const StringMap *manifest,
                                          Permissions *required) {
  ASSERT(manifest);
  ASSERT(required);
  StringMap::const_iterator it = manifest->begin();
  bool has_permissions = false;
  size_t prefix_length = arraysize(kManifestPermissions) - 1;

  required->RemoveAllRequired();
  for (; it != manifest->end(); ++it) {
    if (GadgetStrNCmp(it->first.c_str(), kManifestPermissions,
                      prefix_length) == 0) {
      if (it->first[prefix_length] == 0) {
        has_permissions = true;
      } else if (has_permissions && it->first[prefix_length] == '/') {
        int permission =
            Permissions::GetByName(it->first.c_str() + prefix_length + 1);
        if (permission >= 0) {
          required->SetRequired(permission, true);
        } else {
          DLOG("Invalid permission node: %s", it->first.c_str());
        }
      } else {
        DLOG("Invalid permission node: %s", it->first.c_str());
      }
    }
  }

  // If there is no permissions node in manifest, then requires <allaccess/>
  // explicitly.
  if (!has_permissions)
    required->SetRequired(Permissions::ALL_ACCESS, true);

  return has_permissions;
}

} // namespace ggadget