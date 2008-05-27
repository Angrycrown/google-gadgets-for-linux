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

#ifndef HOSTS_SIMPLE_GADGET_BROWSER_HOST_H__
#define HOSTS_SIMPLE_GADGET_BROWSER_HOST_H__

#include <ggadget/host_interface.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/utilities.h>

namespace hosts {
namespace gtk {

// A special Host for Gadget browser to show browser in a decorated window.
class GadgetBrowserHost : public ggadget::HostInterface {
 public:
  GadgetBrowserHost(ggadget::HostInterface *owner, int view_debug_mode)
    : owner_(owner), view_debug_mode_(view_debug_mode) {
  }
  virtual ViewHostInterface *NewViewHost(Gadget *gadget,
                                         ViewHostInterface::Type type) {
    return new ggadget::gtk::SingleViewHost(type, 1.0, true, true, true,
                                            view_debug_mode_);
  }
  virtual void RemoveGadget(Gadget *gadget, bool save_data) {
    ggadget::GetGadgetManager()->RemoveGadgetInstance(gadget->GetInstanceID());
  }
  virtual void DebugOutput(DebugLevel level, const char *message) const {
    owner_->DebugOutput(level, message);
  }
  virtual bool OpenURL(const char *url) const {
    return owner_->OpenURL(url);
  }
  virtual bool LoadFont(const char *filename) {
    return owner_->LoadFont(filename);
  }
  virtual void ShowGadgetAboutDialog(Gadget *gadget) {
    owner_->ShowGadgetAboutDialog(gadget);
  }
  virtual void Run() {}
 private:
  ggadget::HostInterface *owner_;
  int view_debug_mode_;
};

} // namespace gtk
} // namespace hosts

#endif // HOSTS_SIMPLE_GADGET_BROWSER_HOST_H__