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

#include <cstdio>
#include "unittest/gunit.h"
#include "ggadget/elements.h"
#include "ggadget/element_interface.h"
#include "ggadget/view_interface.h"
#include "ggadget/element_factory_interface.h"
#include "ggadget/slot.h"
#include "mocked_element.h"
#include "mocked_view.h"

// The total count of elements_.
int count = 0;

class Muffin : public MockedElement {
 public:
  DEFINE_CLASS_ID(0xabb1c79164a742aa, MockedElement);

  Muffin(ggadget::ElementInterface *parent,
         ggadget::ViewInterface *view,
         const char *name)
      : MockedElement(parent, view, name) {
    ++count;
  }

  virtual ~Muffin() {
    --count;
  }

 public:
  virtual const char *GetTagName() const {
    return "muffin";
  }
};

class Pie : public MockedElement {
 public:
  DEFINE_CLASS_ID(0x21a2b2ba7c794058, MockedElement);

  Pie(ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name)
      : MockedElement(parent, view, name) {
    ++count;
  }

  virtual ~Pie() {
    --count;
  }

 public:
  virtual const char *GetTagName() const {
    return "pie";
  }
};

class MockedElementFactory : public ggadget::ElementFactoryInterface {
  virtual ggadget::ElementInterface *CreateElement(
      const char *GetTagName,
      ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name) {
    if (strcmp(GetTagName, "muffin") == 0)
      return new Muffin(parent, view, name);
    if (strcmp(GetTagName, "pie") == 0)
      return new Pie(parent, view, name);
    return NULL;
  }

  virtual bool RegisterElementClass(
      const char *GetTagName,
      ggadget::ElementInterface *(*creator)(ggadget::ElementInterface *,
                                            ggadget::ViewInterface *,
                                            const char *)) {
    return true;
  }
};

class ElementsTest : public testing::Test {
 protected:
  virtual void SetUp() {
    factory_ = new MockedElementFactory();
    view_ = new MockedView(factory_);
    muffin_ = new Muffin(NULL, view_, NULL);
    elements_ = new ggadget::Elements(factory_, muffin_, view_);
  }

  virtual void TearDown() {
    delete elements_;
    muffin_->Destroy();
    delete factory_;
    delete view_;
    ASSERT_EQ(0, count);
  }

  MockedView *view_;
  MockedElementFactory *factory_;
  ggadget::Elements *elements_;
  Muffin *muffin_;
};

TEST_F(ElementsTest, TestCreate) {
  ggadget::ElementInterface *e1 = elements_->AppendElement("muffin", NULL);
  ASSERT_TRUE(e1 != NULL);
  ggadget::ElementInterface *e2 = elements_->AppendElement("pie", NULL);
  ASSERT_TRUE(e2 != NULL);
  ggadget::ElementInterface *e3 = elements_->AppendElement("bread", NULL);
  ASSERT_TRUE(e3 == NULL);
}

TEST_F(ElementsTest, TestOrder) {
  ggadget::ElementInterface *e1 = elements_->AppendElement("muffin", NULL);
  ggadget::ElementInterface *e2 = elements_->AppendElement("pie", NULL);
  ggadget::ElementInterface *e3 = elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3, elements_->GetCount());
  ASSERT_TRUE(e1 == elements_->GetItemByIndex(0));
  ASSERT_TRUE(e2 == elements_->GetItemByIndex(1));
  ASSERT_TRUE(e3 == elements_->GetItemByIndex(2));
  ASSERT_TRUE(NULL == elements_->GetItemByIndex(3));
}

TEST_F(ElementsTest, TestGetByName) {
  ggadget::ElementInterface *e1 = elements_->AppendElement("muffin", "muffin1");
  ggadget::ElementInterface *e2 = elements_->AppendElement("pie", "pie2");
  ggadget::ElementInterface *e3 = elements_->AppendElement("pie", "pie3");
  ggadget::ElementInterface *e4 = elements_->AppendElement("pie", "pie3");
  ASSERT_TRUE(e4 != e3);
  ASSERT_EQ(4, elements_->GetCount());
  ASSERT_TRUE(e1 == elements_->GetItemByName("muffin1"));
  ASSERT_TRUE(e2 == elements_->GetItemByName("pie2"));
  ASSERT_TRUE(e3 == elements_->GetItemByName("pie3"));
  ASSERT_TRUE(NULL == elements_->GetItemByName("hungry"));
  ASSERT_TRUE(NULL == elements_->GetItemByName(""));
}

TEST_F(ElementsTest, TestInsert) {
  ggadget::ElementInterface *e1 = elements_->InsertElement("muffin", NULL, NULL);
  ggadget::ElementInterface *e2 = elements_->InsertElement("pie", e1, NULL);
  ggadget::ElementInterface *e3 = elements_->InsertElement("pie", e2, NULL);
  ggadget::ElementInterface *e4 = elements_->InsertElement("bread", e2, NULL);
  ASSERT_EQ(3, elements_->GetCount());
  ASSERT_TRUE(e1 == elements_->GetItemByIndex(2));
  ASSERT_TRUE(e2 == elements_->GetItemByIndex(1));
  ASSERT_TRUE(e3 == elements_->GetItemByIndex(0));
  ASSERT_TRUE(NULL == e4);
}

TEST_F(ElementsTest, TestRemove) {
  ggadget::ElementInterface *e1 = elements_->AppendElement("muffin", NULL);
  ggadget::ElementInterface *e2 = elements_->AppendElement("pie", NULL);
  ggadget::ElementInterface *e3 = elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3, elements_->GetCount());
  ASSERT_TRUE(elements_->RemoveElement(e2));
  ASSERT_EQ(2, elements_->GetCount());
  ASSERT_TRUE(elements_->GetItemByIndex(0) == e1);
  ASSERT_TRUE(elements_->GetItemByIndex(1) == e3);
  ASSERT_TRUE(elements_->RemoveElement(e1));
  ASSERT_FALSE(elements_->RemoveElement(e1));
  ASSERT_TRUE(elements_->GetItemByIndex(0) == e3);
}

TEST_F(ElementsTest, TestRemoveAll) {
  elements_->AppendElement("muffin", NULL);
  elements_->AppendElement("pie", NULL);
  elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3, elements_->GetCount());
  elements_->RemoveAllElements();
  ASSERT_EQ(0, elements_->GetCount());
}

int main(int argc, char *argv[]) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
