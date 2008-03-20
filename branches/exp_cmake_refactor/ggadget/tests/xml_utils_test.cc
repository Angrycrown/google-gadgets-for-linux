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

#include <iostream>
#include <locale.h>

#include "ggadget/xml_dom.h"
#include "ggadget/xml_utils.h"
#include "unittest/gunit.h"

using namespace ggadget;

const char *xml =
  "<?xml version=\"1.0\" encoding=\"iso8859-1\"?>"
  "<?pi value?>"
  "<!DOCTYPE root [\n"
  "  <!ENTITY test \"Test Entity\">\n"
  "]>"
  "<root a=\"v\" a1=\"v1\">\n"
  " <s aa=\"vv\" aa1=\"vv1\">s content</s>\n"
  " <s b=\"bv\" b1=\"bv1\"/>\n"
  " <s1 c=\"cv\" c1=\"cv1\">s1 content</s1>\n"
  " <s aa=\"vv\" aa1=\"vv1\">s content1</s>\n"
  " <s1 c=\"cv\" c1=\"cv1\">\n"
  "   s1 content1 &test;\n"
  "   <!-- comments -->\n"
  "   <s11>s11 content</s11>\n"
  "   <![CDATA[ cdata ]]>\n"
  " </s1>\n"
  " <s2/>\n"
  "</root>";

TEST(XMLUtils, ParseXMLIntoXPathMap) {
  GadgetStringMap map;
  ASSERT_TRUE(ParseXMLIntoXPathMap(xml, "TheFileName", "root", &map));
  ASSERT_EQ(19U, map.size());
  EXPECT_STREQ("v", map.find("@a")->second.c_str());
  EXPECT_STREQ("v1", map.find("@a1")->second.c_str());
  EXPECT_STREQ("s content", map.find("s")->second.c_str());
  EXPECT_STREQ("vv", map.find("s@aa")->second.c_str());
  EXPECT_STREQ("s1 content", map.find("s1")->second.c_str());
  EXPECT_STREQ("", map.find("s[2]")->second.c_str());
  EXPECT_STREQ("s content1", map.find("s[3]")->second.c_str());
  EXPECT_STREQ("vv", map.find("s[3]@aa")->second.c_str());
  EXPECT_STREQ("", map.find("s2")->second.c_str());

}

TEST(XMLUtils, ParseXMLIntoXPathMap_InvalidRoot) {
  GadgetStringMap map;
  ASSERT_FALSE(ParseXMLIntoXPathMap(xml, "TheFileName", "another", &map));
}

TEST(XMLUtils, ParseXMLIntoXPathMap_InvalidXML) {
  GadgetStringMap map;
  ASSERT_FALSE(ParseXMLIntoXPathMap("<a></b>", "Bad", "a", &map));
}

TEST(XMLUtils, CheckXMLName) {
  ASSERT_TRUE(CheckXMLName("abcde:def_.123-456"));
  ASSERT_TRUE(CheckXMLName("\344\270\200-\344\270\201"));
  ASSERT_FALSE(CheckXMLName("&#@Q!#"));
  ASSERT_FALSE(CheckXMLName("Invalid^Name"));
  ASSERT_FALSE(CheckXMLName(NULL));
  ASSERT_FALSE(CheckXMLName(""));
}

// This test case only tests if xml_utils can convert an XML string into DOM
// correctly.  Test cases about DOM itself are in xml_dom_test.cc.
TEST(XMLUtils, ParseXMLIntoDOM) {
  DOMDocumentInterface *domdoc = CreateDOMDocument();
  std::string encoding;
  domdoc->Attach();
  ASSERT_TRUE(ParseXMLIntoDOM(xml, "TheFileName", domdoc, &encoding));
  ASSERT_STREQ("iso8859-1", encoding.c_str());
  DOMElementInterface *doc_ele = domdoc->GetDocumentElement();
  ASSERT_TRUE(doc_ele);
  EXPECT_STREQ("root", doc_ele->GetTagName());
  EXPECT_STREQ("v", doc_ele->GetAttribute("a"));
  EXPECT_STREQ("v1", doc_ele->GetAttribute("a1"));
  DOMNodeListInterface *children = doc_ele->GetChildNodes();
  EXPECT_EQ(13U, children->GetLength());

  DOMNodeInterface *sub_node = children->GetItem(9);
  ASSERT_TRUE(sub_node);
  ASSERT_EQ(DOMNodeInterface::ELEMENT_NODE, sub_node->GetNodeType());
  DOMElementInterface *sub_ele = down_cast<DOMElementInterface *>(sub_node);
  DOMNodeListInterface *sub_children = sub_ele->GetChildNodes();
  EXPECT_EQ(7U, sub_children->GetLength());
  EXPECT_EQ(DOMNodeInterface::TEXT_NODE, sub_children->GetItem(0)->GetNodeType());
  EXPECT_STREQ("\n   s1 content1 Test Entity\n   ",
               sub_children->GetItem(0)->GetNodeValue());
  EXPECT_EQ(DOMNodeInterface::COMMENT_NODE,
            sub_children->GetItem(1)->GetNodeType());
  EXPECT_STREQ(" comments ", sub_children->GetItem(1)->GetNodeValue());
  EXPECT_EQ(DOMNodeInterface::CDATA_SECTION_NODE,
            sub_children->GetItem(5)->GetNodeType());
  EXPECT_STREQ(" cdata ", sub_children->GetItem(5)->GetNodeValue());

  DOMNodeInterface *pi_node = domdoc->GetFirstChild();
  EXPECT_EQ(DOMNodeInterface::PROCESSING_INSTRUCTION_NODE,
            pi_node->GetNodeType());
  EXPECT_STREQ("pi", pi_node->GetNodeName());
  EXPECT_STREQ("value", pi_node->GetNodeValue());
  delete children;
  delete sub_children;
  domdoc->Detach();
}

TEST(XMLUtils, ParseXMLIntoDOM_InvalidXML) {
  DOMDocumentInterface *domdoc = CreateDOMDocument();
  domdoc->Attach();
  ASSERT_FALSE(ParseXMLIntoDOM("<a></b>", "Bad", domdoc, NULL));
  domdoc->Detach();
}

TEST(XMLUtils, ConvertStringToUTF8) {
  const char *src = "ASCII string, no BOM";
  std::string output;
  ASSERT_TRUE(ConvertStringToUTF8(src, strlen(src), NULL, &output));
  ASSERT_STREQ(src, output.c_str());
  ASSERT_TRUE(ConvertStringToUTF8(std::string(src), NULL, &output));
  ASSERT_STREQ(src, output.c_str());

  src = "\xEF\xBB\xBFUTF8 String, with BOM";
  ASSERT_TRUE(ConvertStringToUTF8(src, strlen(src), NULL, &output));
  ASSERT_STREQ(src, output.c_str());

  // The last '\0' omitted, because the compiler will add it for us. 
  const char utf16le[] = "\xFF\xFEU\0T\0F\0001\0006\0 \0S\0t\0r\0i\0n\0g";
  const char *dest = "\xEF\xBB\xBFUTF16 String";
  ASSERT_TRUE(ConvertStringToUTF8(utf16le, sizeof(utf16le), NULL, &output));
  ASSERT_STREQ(dest, output.c_str());

  src = "\xBA\xBA\xD7\xD6";
  dest = "\xE6\xB1\x89\xE5\xAD\x97";
  ASSERT_TRUE(ConvertStringToUTF8(src, strlen(src), "GB2312", &output));
  ASSERT_STREQ(dest, output.c_str());

  ASSERT_FALSE(ConvertStringToUTF8(src, strlen(src), NULL, &output));
  ASSERT_STREQ("", output.c_str());

  src = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\n"
        "<root>\xBA\xBA\xD7\xD6</root>\n";
  ASSERT_FALSE(ConvertStringToUTF8(src, strlen(src), NULL, &output));
  ASSERT_STREQ("", output.c_str());
}

TEST(XMLUtils, EncodeXMLString) {
  ASSERT_STREQ("", EncodeXMLString(NULL).c_str());
  ASSERT_STREQ("", EncodeXMLString("").c_str());
  ASSERT_STREQ("&lt;&gt;", EncodeXMLString("<>").c_str());
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}