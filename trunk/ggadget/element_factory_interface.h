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

#ifndef GGADGET_ELEMENT_FACTORY_INTERFACE_H__
#define GGADGET_ELEMENT_FACTORY_INTERFACE_H__

#include <cstddef> // For declarations of size_t and NULL.

namespace ggadget {

class ElementInterface;

/**
 * Interface for creating an Element in the Gadget API.
 */
class ElementFactoryInterface {
 public:
  /**
   * Create an ElementInterface of the specified type.
   * @param type the type name of the object.
   * @param parent the parent object of the newly created object.
   * @return the pointer to the newly created object or @c NULL if failed.
   */
  virtual ElementInterface *CreateElement(const char *type,
                                          ElementInterface *parent) = 0;

  /**
   * Register a new subclass of ElementInterface.
   * @param type the type name of the subclass.
   * @param creator the function pointer of the creator, which returns a new
   *     instance of an object of this type.
   * @return @c true if registered successfully, or @c false if the specified
   *     type already exists.
   */
  virtual bool RegisterElementClass(
      const char *type, ElementInterface *(*creator)(ElementInterface *)) = 0;
};

} // namespace ggadget

#endif // GGADGET_ELEMENT_FACTORY_INTERFACE_H__
