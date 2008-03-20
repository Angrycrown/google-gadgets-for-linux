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

#ifndef GGADGET_TESTS_MOCKED_XML_HTTP_REQUEST_H__
#define GGADGET_TESTS_MOCKED_XML_HTTP_REQUEST_H__

#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "ggadget/scriptable_helper.h"
#include "ggadget/signals.h"
#include "ggadget/xml_http_request_interface.h"

// Because of the following global variables, this header file is only for
// testing, and can only be included from one source file if there are multiple
// files linked together.
unsigned short g_mocked_xml_http_request_return_status = 200;
std::string g_mocked_xml_http_request_return_data;

// Records the last requested URL.
std::string g_mocked_xml_http_request_requested_url;

class MockedXMLHttpRequest : public ggadget::ScriptableHelperNativeOwned<
    ggadget::XMLHttpRequestInterface> {
public:
  DEFINE_CLASS_ID(0x5868a91c86574dca, ggadget::XMLHttpRequestInterface);
  MockedXMLHttpRequest(unsigned short return_status,
                       const std::string &return_data)
      : state_(UNSENT),
        return_status_(return_status),
        return_data_(return_data) {
  }

  void ChangeState(State new_state) {
    state_ = new_state;
    statechange_signal_();
  }

  virtual ggadget::Connection *ConnectOnReadyStateChange(
      ggadget::Slot0<void> *handler) {
    return statechange_signal_.Connect(handler);
  } 
  virtual State GetReadyState() { return state_; }
  virtual ExceptionCode Open(const char *method, const char *url, bool async,
                             const char *user, const char *password) {
    requested_url_ = url;
    g_mocked_xml_http_request_requested_url = url;
    ChangeState(OPENED);
    return NO_ERR;
  }
  virtual ExceptionCode SetRequestHeader(const char *header,
                                         const char *value) { return NO_ERR; }
  virtual ExceptionCode Send(const char *data, size_t size) {
    ChangeState(HEADERS_RECEIVED);
    ChangeState(LOADING);
    ChangeState(DONE);
    return NO_ERR;
  }
  virtual ExceptionCode Send(const ggadget::DOMDocumentInterface *data) {
    return Send(NULL, 0);
  }
  virtual void Abort() { ChangeState(DONE); }
  virtual ExceptionCode GetAllResponseHeaders(const char **result) {
    return NO_ERR;
  }
  virtual ExceptionCode GetResponseHeader(const char *header,
                                          const char **result) {
    return NO_ERR;
  }
  virtual ExceptionCode GetResponseText(const char **result) { return NO_ERR; }
  virtual ExceptionCode GetResponseBody(const char **result,
                                        size_t *size) {
    *result = return_data_.c_str();
    *size = return_data_.size();
    return NO_ERR;
  }
  virtual ExceptionCode GetResponseXML(ggadget::DOMDocumentInterface **result) {
    return NO_ERR;
  }
  virtual ExceptionCode GetStatus(unsigned short *result) {
    *result = return_status_;
    return NO_ERR;
  }
  virtual ExceptionCode GetStatusText(const char **result) { return NO_ERR; }
  virtual ExceptionCode GetResponseBody(std::string *result) {
    *result = return_data_;
    return NO_ERR;
  }

  State state_;
  unsigned short return_status_;
  std::string return_data_;
  std::string requested_url_;
  ggadget::Signal0<void> statechange_signal_;
};

// Set the above global variables before an XMLHttpRequestInstance instance
// is to be created, to make the instance do the desired things.
inline ggadget::XMLHttpRequestInterface *MockedXMLHttpRequestFactory(
    ggadget::XMLParserInterface *parser) {
  return new MockedXMLHttpRequest(g_mocked_xml_http_request_return_status,
                                  g_mocked_xml_http_request_return_data);
}

#endif // GGADGET_TESTS_MOCKED_XML_HTTP_REQUEST_H__