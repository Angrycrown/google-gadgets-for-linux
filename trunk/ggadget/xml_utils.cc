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

#include <cstring>
#include <cmath>

#include "xml_utils.h"
#include "basic_element.h"
#include "elements.h"
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "logger.h"
#include "script_context_interface.h"
#include "unicode_utils.h"
#include "view.h"
#include "xml_dom_interface.h"
#include "xml_parser_interface.h"

namespace ggadget {

static void SetScriptableProperty(ScriptableInterface *scriptable,
                                  ScriptContextInterface *script_context,
                                  const char *filename, int row, int column,
                                  const char *name, const char *value,
                                  const char *tag_name) {
  int id;
  Variant prototype;
  bool is_method;
  bool result = scriptable->GetPropertyInfoByName(name, &id,
                                                  &prototype, &is_method);
  if (!result || is_method ||
      id == ScriptableInterface::kConstantPropertyId ||
      id == ScriptableInterface::kDynamicPropertyId) {
    LOG("%s:%d:%d Can't set property %s for %s", filename, row, column,
        name, tag_name);
    return;
  }

  Variant str_value_variant(value);
  Variant property_value;
  switch (prototype.type()) {
    case Variant::TYPE_BOOL: {
      bool b;
      if (str_value_variant.ConvertToBool(&b)) {
        property_value = Variant(b);
      } else {
        LOG("%s:%d:%d: Invalid bool '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
        property_value = Variant(GadgetStrCmp("true", value) == 0 ?
                               true : false);
        return;
      }
      break;
    }
    case Variant::TYPE_INT64: {
      int64_t i;
      if (str_value_variant.ConvertToInt64(&i)) {
        property_value = Variant(i);
      } else {
        LOG("%s:%d:%d: Invalid Integer '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
        return;
      }
      break;
    }
    case Variant::TYPE_DOUBLE: {
      double d;
      if (str_value_variant.ConvertToDouble(&d)) {
        property_value = Variant(d);
      } else {
        LOG("%s:%d:%d: Invalid double '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
        return;
      }
      break;
    }
    case Variant::TYPE_STRING:
      property_value = str_value_variant;
      break;

    case Variant::TYPE_VARIANT: {
      int64_t i;
      double d;
      bool b;
      if (!*value) {
        property_value = str_value_variant;
      } else if (strchr(value, '.') == NULL &&
                 str_value_variant.ConvertToInt64(&i)) {
        property_value = Variant(i);
      } else if (str_value_variant.ConvertToDouble(&d)) {
        property_value = Variant(d);
      } else if (str_value_variant.ConvertToBool(&b)) {
        property_value = Variant(b);
      } else {
        property_value = str_value_variant;
      }
      break;
    }
    case Variant::TYPE_SLOT:
      property_value = Variant(script_context->Compile(value, filename, row));
      break;

    default:
      LOG("%s:%d:%d: Unsupported type %s when setting property %s for %s",
          filename, row, column, prototype.Print().c_str(), name, tag_name);
      return;
  }

  if (!scriptable->SetProperty(id, property_value))
    LOG("%s:%d:%d: Can't set readonly property %s for %s",
        filename, row, column, name, tag_name);
}

static void SetupScriptableProperties(ScriptableInterface *scriptable,
                                      ScriptContextInterface *script_context,
                                      const char *filename,
                                      const DOMElementInterface *xml_element) {
  std::string tag_name = xml_element->GetTagName();
  const DOMNamedNodeMapInterface *attributes = xml_element->GetAttributes();
  size_t length = attributes->GetLength();
  for (size_t i = 0; i < length; i++) {
    const DOMAttrInterface *attr = down_cast<const DOMAttrInterface *>(
        attributes->GetItem(i));
    std::string name = attr->GetName();
    std::string value = attr->GetValue();
    if (GadgetStrCmp(kInnerTextProperty, name.c_str()) == 0) {
      LOG("%s is not allowed in XML as an attribute: ", kInnerTextProperty);
      continue;
    }

    if (GadgetStrCmp(kNameAttr, name.c_str()) != 0) {
      SetScriptableProperty(scriptable, script_context, filename,
                            attr->GetRow(), attr->GetColumn(),
                            name.c_str(), value.c_str(), tag_name.c_str());
    }
  }
  delete attributes;
  // "innerText" property is set in InsertElementFromDOM().
}

static void HandleScriptElement(ScriptContextInterface *script_context,
                                FileManagerInterface *file_manager,
                                const char *filename,
                                const DOMElementInterface *xml_element) {
  int lineno = xml_element->GetRow();
  std::string script;
  std::string src = xml_element->GetAttribute(kSrcAttr);

  if (!src.empty()) {
    if (file_manager->ReadFile(src.c_str(), &script)) {
      filename = src.c_str();
      lineno = 1;
      std::string temp;
      if (ConvertStreamToUTF8ByBOM(script, &temp, NULL) == script.length())
        script = temp;
    }
  } else {
    // Uses the Windows version convention, that inline scripts should be
    // quoted in comments.
    for (const DOMNodeInterface *child = xml_element->GetFirstChild();
         child; child = child->GetNextSibling()) {
      if (child->GetNodeType() == DOMNodeInterface::COMMENT_NODE) {
        script = child->GetTextContent();
        break;
      } else if (child->GetNodeType() != DOMNodeInterface::TEXT_NODE ||
                 !TrimString(child->GetTextContent()).empty()) {
        // Other contents are not allowed under <script></script>.
        LOG("%s:%d:%d: This content is not allowed in script element",
            filename, child->GetRow(), child->GetColumn());
      }
    }
  }

  if (!script.empty())
    script_context->Execute(script.c_str(), filename, lineno);
}

static void HandleAllScriptElements(View *view, const char *filename,
                                    const DOMElementInterface *xml_element) {
  for (const DOMNodeInterface *child = xml_element->GetFirstChild();
       child; child = child->GetNextSibling()) {
    if (child->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
      const DOMElementInterface *child_ele =
          down_cast<const DOMElementInterface *>(child);
      if (GadgetStrCmp(child_ele->GetTagName().c_str(), kScriptTag) == 0) {
        HandleScriptElement(view->GetScriptContext(),
                            view->GetFileManager(),
                            filename, child_ele);
      } else {
        HandleAllScriptElements(view, filename, child_ele);
      }
    }
  }
}

static BasicElement *InsertElementFromDOM(
    View *view, Elements *elements, const char *filename,
    const DOMElementInterface *xml_element,
    const BasicElement *before) {
  std::string tag_name = xml_element->GetTagName();
  if (GadgetStrCmp(tag_name.c_str(), kScriptTag) == 0)
    return NULL;

  std::string name = xml_element->GetAttribute(kNameAttr);
  BasicElement *element = elements->InsertElement(tag_name.c_str(),
                                                      before,
                                                      name.c_str());
  if (!element) {
    LOG("%s:%d:%d: Failed to create element %s", filename,
        xml_element->GetRow(), xml_element->GetColumn(), tag_name.c_str());
    return element;
  }

  SetupScriptableProperties(element, view->GetScriptContext(),
                            filename, xml_element);
  Elements *children = element->GetChildren();
  std::string text;
  for (const DOMNodeInterface *child = xml_element->GetFirstChild();
       child; child = child->GetNextSibling()) {
    DOMNodeInterface::NodeType type = child->GetNodeType();
    if (type == DOMNodeInterface::ELEMENT_NODE) {
      InsertElementFromDOM(view, children, filename,
                           down_cast<const DOMElementInterface *>(child),
                           NULL);
    } else if (type == DOMNodeInterface::TEXT_NODE ||
               type == DOMNodeInterface::CDATA_SECTION_NODE) {
      text += down_cast<const DOMTextInterface *>(child)->GetTextContent();
    }
  }
  // Set the "innerText" property.
  text = TrimString(text);
  if (!text.empty()) {
    SetScriptableProperty(element, view->GetScriptContext(), filename,
                          xml_element->GetRow(), xml_element->GetColumn(),
                          kInnerTextProperty, text.c_str(), tag_name.c_str());
  }
  return element;
}

bool SetupViewFromXML(View *view, const std::string &xml,
                      const char *filename) {
  DOMDocumentInterface *xmldoc = GetXMLParser()->CreateDOMDocument();
  xmldoc->Ref();
  if (!GetXMLParser()->ParseContentIntoDOM(xml, filename, NULL, NULL,
                                           xmldoc, NULL, NULL)) {
    xmldoc->Unref();
    return false;
  }

  DOMElementInterface *view_element = xmldoc->GetDocumentElement();
  if (!view_element ||
      GadgetStrCmp(view_element->GetTagName().c_str(), kViewTag) != 0) {
    LOG("No valid root element in view file: %s", filename);
    xmldoc->Unref();
    return false;
  }

  view->EnableEvents(false);
  SetupScriptableProperties(view, view->GetScriptContext(),
                            filename, view_element);

  Elements *children = view->GetChildren();
  for (const DOMNodeInterface *child = view_element->GetFirstChild();
       child; child = child->GetNextSibling()) {
    if (child->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
      InsertElementFromDOM(view, children, filename,
                           down_cast<const DOMElementInterface *>(child),
                           NULL);
    }
  }

  view->EnableEvents(true);
  HandleAllScriptElements(view, filename, view_element);
  xmldoc->Unref();
  return true;
}

BasicElement *AppendElementFromXML(View *view, Elements *elements,
                                   const std::string &xml) {
  return InsertElementFromXML(view, elements, xml, NULL);
}

BasicElement *InsertElementFromXML(View *view, Elements *elements,
                                   const std::string &xml,
                                   const BasicElement *before) {
  DOMDocumentInterface *xmldoc = GetXMLParser()->CreateDOMDocument();
  xmldoc->Ref();
  if (!GetXMLParser()->ParseContentIntoDOM(xml, xml.c_str(), NULL, NULL,
                                                 xmldoc, NULL, NULL)) {
    xmldoc->Unref();
    return false;
  }

  DOMElementInterface *xml_element = xmldoc->GetDocumentElement();
  if (!xml_element) {
    LOG("No root element in xml definition: %s", xml.c_str());
    xmldoc->Unref();
    return NULL;
  }

  BasicElement *result = InsertElementFromDOM(view, elements, "",
                                              xml_element, before);
  xmldoc->Unref();
  return result;
}

bool ReplaceXMLEntities(const GadgetStringMap &entities, std::string *xml) {
  ASSERT(xml);
  std::string result;

  // Process text to replace all "&..;" entities with corresponding resources.
  std::string::size_type start = 0;
  std::string::size_type pos = 0;
  for (; pos < xml->size(); pos++) {
    char c = (*xml)[pos];
    // 1 byte at a time is OK here.
    if ('&' == c) {
      // Append the previous chunk.
      result.append(*xml, start, pos - start);
      start = pos;

      std::string entity_name;
      while (++pos < xml->size() && (c = (*xml)[pos]) != ';' && c != '\n')
        entity_name.push_back(c);

      if (c != ';') {
        LOG("Unterminated entity reference: %s.",
            xml->substr(start, pos - start).c_str());
        return false;
      }

      GadgetStringMap::const_iterator iter = entities.find(entity_name);
      if (iter != entities.end()) {
        start = pos + 1;  // Reset start for next chunk.
        result.append(GetXMLParser()->EncodeXMLString(iter->second.c_str()));
      }
      // Else: not a fatal error. Just leave the original entity reference
      // text in the current text chunk and let tinyxml deal with it.
    }
  }

  if (pos - start > 0)  // Append any remaining chars.
    result.append(*xml, start, pos - start);

  xml->swap(result);

  return true;
}

} // namespace ggadget