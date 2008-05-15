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

#ifndef GGADGET_SCRIPTABLE_MAP_H__
#define GGADGET_SCRIPTABLE_MAP_H__

#include <map>
#include <string>
#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {

/**
 * This class is used to reflect a const native map to script. The life of
 * the native map must be longer than the life of this object.
 * The script can access this object by getting "count" property and "item"
 * method, or with an Enumerator.
 */
template <typename Value, typename CompareFn>
class ScriptableMap : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x1136ce531e9046cd, ScriptableInterface);

  typedef std::map<std::string, Value, CompareFn> Map;
  typedef typename Map::const_iterator MapConstIterator;

  ScriptableMap(const Map &map) : map_(map) {
    SetDynamicPropertyHandler(NewSlot(this, &ScriptableMap::GetProperty), NULL);
  }

 public:
  Variant GetProperty(const char *property_name) const {
    MapConstIterator it = map_.find(property_name);
    return it == map_.end() ? Variant() : Variant(it->second);
  }

  /**
   * This method is overriden to make this object act like normal
   * JavaScript arrays for C++ users.
   */
  virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback) {
    for (MapConstIterator it = map_.begin(); it != map_.end(); ++it) {
      if (!(*callback)(ScriptableInterface::kDynamicPropertyId,
                       it->first.c_str(), Variant(it->second), false))
        return false;
    }
    return true;
  }

  const std::map<std::string, Value, CompareFn> &map() const { return map_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableMap);
  const std::map<std::string, Value, CompareFn> &map_;
};

/** Creates a @c ScriptableMap instance. */
template <typename Value, typename CompareFn>
ScriptableMap<Value, CompareFn> *
NewScriptableMap(const std::map<std::string, Value, CompareFn> &map) {
  return new ScriptableMap<Value, CompareFn>(map);
}

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_MAP_H__
