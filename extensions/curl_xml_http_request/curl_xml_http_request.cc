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

#include <algorithm>
#include <cstring>
#include <curl/curl.h>
#include <pthread.h>
#include <vector>

#include <ggadget/gadget_consts.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/logger.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/signals.h>
#include <ggadget/string_utils.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_http_request_utils.h>
#include <ggadget/xml_dom_interface.h>
#include <ggadget/xml_parser_interface.h>

namespace ggadget {
namespace curl {

static const long kMaxRedirections = 10;
static const long kConnectTimeoutSec = 20;

static const Variant kOpenDefaultArgs[] = {
  Variant(), Variant(),
  Variant(true),
  Variant(static_cast<const char *>(NULL)),
  Variant(static_cast<const char *>(NULL))
};

static const Variant kSendDefaultArgs[] = { Variant("") };

#if 0 // Don't support newlines in header values.
// Process a user inputed header value, add a space after newline.
static std::string ReformatHttpHeaderValue(const char *value) {
  std::string tmp;
  if (value == NULL) return tmp;
  int n_crlf = 0;
  while (*value) {
    if (*value == '\r' || *value == '\n') {
      ++n_crlf;
    } else {
      if (n_crlf > 0) {
        // We have meeted some newline char(s). Both '\r' and '\n' are
        // recognised as newline chars, and continued newline chars
        // are taken as one. According the rfc2616, the server MAY
        // replace / *\r\n +/ to " ", so this will work fine.
        tmp += "\r\n ";
        n_crlf = 0;
      }
      tmp.push_back(*value);
    }
    ++value;
  }
  return tmp;
}
#else
static const char *ReformatHttpHeaderValue(const char *value) {
  return value;
}
#endif

class XMLHttpRequest : public ScriptableHelper<XMLHttpRequestInterface> {
 public:
  DEFINE_CLASS_ID(0xda25f528f28a4319, XMLHttpRequestInterface);

  XMLHttpRequest(CURLSH *share, MainLoopInterface *main_loop,
                 XMLParserInterface *xml_parser,
                 const std::string &default_user_agent)
      : curl_(NULL),
        share_(share),
        main_loop_(main_loop),
        xml_parser_(xml_parser),
        async_(false),
        method_(HTTP_GET),
        state_(UNSENT),
        send_flag_(false),
        request_headers_(NULL),
        status_(0),
        succeeded_(false),
        response_dom_(NULL),
        default_user_agent_(default_user_agent) {
    VERIFY_M(EnsureXHRBackoffOptions(main_loop->GetCurrentTime()),
             ("Required options module have not been loaded"));
    pthread_attr_init(&thread_attr_);
    pthread_attr_setdetachstate(&thread_attr_, PTHREAD_CREATE_DETACHED);
  }

  virtual void DoClassRegister() {
    RegisterClassSignal("onreadystatechange",
                        &XMLHttpRequest::onreadystatechange_signal_);
    RegisterProperty("readyState",
                     NewSlot(&XMLHttpRequest::GetReadyState), NULL);
    RegisterMethod("open",
        NewSlotWithDefaultArgs(NewSlot(&XMLHttpRequest::ScriptOpen),
                               kOpenDefaultArgs));
    RegisterMethod("setRequestHeader",
                   NewSlot(&XMLHttpRequest::ScriptSetRequestHeader));
    RegisterMethod("send",
        NewSlotWithDefaultArgs(NewSlot(&XMLHttpRequest::ScriptSend),
                               kSendDefaultArgs));
    RegisterMethod("abort", NewSlot(&XMLHttpRequest::Abort));
    RegisterMethod("getAllResponseHeaders",
                   NewSlot(&XMLHttpRequest::ScriptGetAllResponseHeaders));
    RegisterMethod("getResponseHeader",
                   NewSlot(&XMLHttpRequest::ScriptGetResponseHeader));
    RegisterProperty("responseStream",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseBody),
                     NULL);
    RegisterProperty("responseBody",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseBody),
                     NULL);
    RegisterProperty("responseText",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseText),
                     NULL);
    RegisterProperty("responseXML",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseXML),
                     NULL);
    RegisterProperty("status", NewSlot(&XMLHttpRequest::ScriptGetStatus),
                     NULL);
    RegisterProperty("statusText",
                     NewSlot(&XMLHttpRequest::ScriptGetStatusText), NULL);
  }

  ~XMLHttpRequest() {
    Abort();
    pthread_attr_destroy(&thread_attr_);
  }

  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler) {
    return onreadystatechange_signal_.Connect(handler);
  }

  virtual State GetReadyState() {
    return state_;
  }

  bool ChangeState(State new_state) {
    DLOG("XMLHttpRequest: ChangeState from %d to %d this=%p",
         state_, new_state, this);
    state_ = new_state;
    onreadystatechange_signal_();
    // ChangeState may re-entered during the signal, so the current state_
    // may be different from the input parameter.
    return state_ == new_state;
  }

  // The maximum data size of this class can process.
  static const size_t kMaxDataSize = 8 * 1024 * 1024;

  static bool CheckSize(size_t current, size_t num_blocks, size_t block_size) {
    return current < kMaxDataSize && block_size > 0 &&
           (kMaxDataSize - current) / block_size > num_blocks;
  }

  virtual ExceptionCode Open(const char *method, const char *url, bool async,
                             const char *user, const char *password) {
    Abort();
    if (!method || !url)
      return NULL_POINTER_ERR;

    bool is_https = false;
    if (0 != strncasecmp(url, kHttpUrlPrefix, arraysize(kHttpUrlPrefix) - 1)) {
      if (0 != strncasecmp(url, kHttpsUrlPrefix,
                           arraysize(kHttpsUrlPrefix) - 1)) {
        return SYNTAX_ERR;
      } else {
        is_https = true;
      }
    }

    if (!GetUsernamePasswordFromURL(url).empty()) {
      // GDWin Compatibility.
      DLOG("Username:password in URL is not allowed: %s", url);
      return SYNTAX_ERR;
    }

    url_ = url;
    host_ = GetHostFromURL(url);
    curl_ = curl_easy_init();
    if (!curl_) {
      DLOG("XMLHttpRequest: curl_easy_init failed");
      // TODO: Send errors.
      return OTHER_ERR;
    }

    if (is_https) {
      curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1);
      curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2);
      // Older versions of libcurl's ca bundle file is also very old, so add
      // OpenSSL's cert directory. Only for Linux and libcurl-openssl config.
      curl_easy_setopt(curl_, CURLOPT_CAPATH, "/etc/ssl/certs");
    }

    if (!default_user_agent_.empty())
      curl_easy_setopt(curl_, CURLOPT_USERAGENT, default_user_agent_.c_str());

    // Disable curl using signals because we use curl in multiple threads.
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1);
    if (share_)
      curl_easy_setopt(curl_, CURLOPT_SHARE, share_);
    // Enable cookies, but don't write them into any file.
    curl_easy_setopt(curl_, CURLOPT_COOKIEFILE, "");

    if (strcasecmp(method, "HEAD") == 0) {
      curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1);
      curl_easy_setopt(curl_, CURLOPT_NOBODY, 1);
      method_ = HTTP_HEAD;
    } else if (strcasecmp(method, "GET") == 0) {
      curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1);
      method_ = HTTP_GET;
    } else if (strcasecmp(method, "POST") == 0) {
      curl_easy_setopt(curl_, CURLOPT_POST, 1);
      method_ = HTTP_POST;
    } else if (strcasecmp(method, "PUT") == 0) {
      curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1);
      method_ = HTTP_PUT;
    } else {
      LOG("XMLHttpRequest: Unsupported method: %s", method);
      return SYNTAX_ERR;
    }
    curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());

    if (user || password) {
      std::string user_pwd;
      if (user)
        user_pwd = user;
      user_pwd += ':';
      if (password)
        user_pwd += password;
      curl_easy_setopt(curl_, CURLOPT_USERPWD, user_pwd.c_str());
    }

    // Disable the default "Expect: 100-continue" request header.
    request_headers_ = curl_slist_append(request_headers_, "Expect:");

    async_ = async;
    ChangeState(OPENED);
    return NO_ERR;
  }

  virtual ExceptionCode SetRequestHeader(const char *header,
                                         const char *value) {
    if (state_ != OPENED || send_flag_) {
      LOG("XMLHttpRequest: SetRequestHeader: Invalid state: %d", state_);
      return INVALID_STATE_ERR;
    }

    if (!IsValidHTTPToken(header)) {
      LOG("XMLHttpRequest::SetRequestHeader: Invalid header %s", header);
      return SYNTAX_ERR;
    }

    if (!IsValidHTTPHeaderValue(value)) {
      LOG("XMLHttpRequest::SetRequestHeader: Invalid value: %s", value);
      return SYNTAX_ERR;
    }

    if (IsForbiddenHeader(header)) {
      DLOG("XMLHttpRequest::SetRequestHeader: Forbidden header %s", header);
      return NO_ERR;
    }

    std::string whole_header(header);
    whole_header += ": ";
    whole_header += ReformatHttpHeaderValue(value);
    request_headers_ = curl_slist_append(request_headers_,
                                         whole_header.c_str());
    return NO_ERR;
  }

  struct WorkerContext {
    WorkerContext(XMLHttpRequest *a_this_p, CURL *a_curl, bool a_async,
                  curl_slist *a_request_headers,
                  const char *a_request_data, size_t a_request_size)
        : this_p(a_this_p), curl(a_curl), async(a_async),
          request_headers(a_request_headers),
          request_offset(0) {
      if (a_request_data && a_request_size > 0)
        request_data.assign(a_request_data, a_request_size);
    }
    XMLHttpRequest *this_p;
    CURL *curl;
    bool async;
    curl_slist *request_headers;
    std::string request_data;
    size_t request_offset;
  };

  virtual ExceptionCode Send(const char *data, size_t size) {
    if (state_ != OPENED || send_flag_) {
      LOG("XMLHttpRequest: Send: Invalid state: %d", state_);
      return INVALID_STATE_ERR;
    }

    if (!CheckSize(size, 0, 512)) {
      LOG("XMLHttpRequest: Send: Size too big: %zu", size);
      return SYNTAX_ERR;
    }

    // As described in the spec, here don't change the state, but send
    // an event for historical reasons.
    if (!ChangeState(OPENED))
      return INVALID_STATE_ERR;

    // Do backoff checking to avoid DDOS attack to the server.
    if (!IsXHRBackoffRequestOK(main_loop_->GetCurrentTime(),
                               host_.c_str())) {
      Abort();
      if (async_) {
        // Don't raise exception here because async callers might not expect
        // this kind of exception.
        ChangeState(DONE);
        return NO_ERR;
      }
      return ABORT_ERR;
    }

    WorkerContext *context = new WorkerContext(this, curl_, async_,
                                               request_headers_,
                                               data, size);
    request_headers_ = NULL;

    if (data && size > 0) {
      if (method_ == HTTP_POST) {
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE_LARGE,
                         static_cast<curl_off_t>(size));
        // CURLOPT_COPYPOSTFIELDS is better, but requires libcurl version 7.17.
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS,
                         context->request_data.c_str());
      } else if (method_ == HTTP_PUT) {
        curl_easy_setopt(curl_, CURLOPT_READFUNCTION, ReadCallback);
        curl_easy_setopt(curl_, CURLOPT_READDATA, context);
        curl_easy_setopt(curl_, CURLOPT_INFILESIZE_LARGE,
                         static_cast<curl_off_t>(size));
      }
    }

  #ifdef _DEBUG
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1);
  #endif
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, context->request_headers);
    curl_easy_setopt(curl_, CURLOPT_FRESH_CONNECT, 1);
    curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 1);
    curl_easy_setopt(curl_, CURLOPT_AUTOREFERER, 1);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, kMaxRedirections);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSec);

    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, context);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteBodyCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, context);

    if (async_) {
      // Add an internal reference when this request is working to prevent
      // this object from being GC'ed during the request.
      Ref();
      send_flag_ = true;
      pthread_t thread;
      if (pthread_create(&thread, &thread_attr_, Worker, context) != 0) {
        DLOG("Failed to create worker thread");
        Unref();
        send_flag_ = false;
        Abort();
        if (context->request_headers) {
          curl_slist_free_all(context->request_headers);
          context->request_headers = NULL;
        }
        delete context;
        return ABORT_ERR;
      }
    } else {
      send_flag_ = true;
      // Run the worker directly in this thread.
      void *result = Worker(context);
      CURLcode code = *reinterpret_cast<CURLcode *>(&result);
      send_flag_ = false;
      if (code != CURLE_OK)
        return NETWORK_ERR;
    }
    return NO_ERR;
  }

  virtual ExceptionCode Send(const DOMDocumentInterface *data) {
    if (!data)
      return Send(static_cast<char *>(NULL), 0);

    std::string xml = data->GetXML();
    return Send(xml.c_str(), xml.size());
  }

  // If async, this method runs in a separate thread.
  static void *Worker(void *arg) {
    WorkerContext *context = static_cast<WorkerContext *>(arg);

    CURLcode code = curl_easy_perform(context->curl);
    long curl_status = 0;
    curl_easy_getinfo(context->curl, CURLINFO_RESPONSE_CODE, &curl_status);
    unsigned short status = static_cast<unsigned short>(curl_status);

    if (context->request_headers) {
      curl_slist_free_all(context->request_headers);
      context->request_headers = NULL;
    }

    if (code != CURLE_OK) {
      DLOG("XMLHttpRequest: Send: curl_easy_perform failed: %s",
           curl_easy_strerror(code));
    }

    WorkerDone(status, context, code == CURLE_OK);
    delete context;
    return reinterpret_cast<void *>(code);
  }

  static size_t ReadCallback(void *ptr, size_t size, size_t mem_block,
                             void *user_p) {
    size_t data_size = size * mem_block;
    WorkerContext *context = static_cast<WorkerContext *>(user_p);
    ASSERT(context->request_data.size() >= context->request_offset);
    size_t bytes_left = context->request_data.size() - context->request_offset;
    DLOG("XMLHttpRequest: ReadCallback: %zu*%zu this=%p left=%zu",
         size, mem_block, context->this_p, bytes_left);
    if (bytes_left == 0)
      return 0;

    if (context->async &&
        context->this_p->curl_ != context->curl) {
      // The current XMLHttpRequest has been aborted, so abort the
      // curl request.
      return CURL_READFUNC_ABORT;
    }

    data_size = std::min(data_size, context->request_data.size() -
                                    context->request_offset);
    memcpy(ptr, context->request_data.c_str(), data_size);
    context->request_offset += data_size;
    return data_size;
  }

  // Passes the WriteHeader() request from worker thread to the main thread.
  class WriteHeaderTask : public WatchCallbackInterface {
   public:
    WriteHeaderTask(const void *ptr, size_t size,
                    const WorkerContext *worker_context)
        : data_(static_cast<const char *>(ptr), size),
          worker_context_(*worker_context) {
    }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      if (worker_context_.this_p->curl_ == worker_context_.curl)
        worker_context_.this_p->WriteHeader(data_);
      return false;
    }
    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      delete this;
    }

    std::string data_;
    WorkerContext worker_context_;
  };

  // Passes the WriteBody() request from worker thread to the main thread.
  class WriteBodyTask : public WriteHeaderTask {
   public:
    WriteBodyTask(const void *ptr, size_t size, unsigned short status,
                  const WorkerContext *worker_context)
        : WriteHeaderTask(ptr, size, worker_context),
          status_(status) {
    }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      if (worker_context_.this_p->curl_ == worker_context_.curl)
        worker_context_.this_p->WriteBody(data_, status_);
      return false;
    }

    unsigned short status_;
  };

  // Passes the Done() request from worker thread to the main thread.
  class DoneTask : public WriteBodyTask {
   public:
    DoneTask(unsigned short status, const WorkerContext *worker_context,
             bool succeeded)
          // Write blank data to ensure the header is parsed.
        : WriteBodyTask("", 0, status, worker_context),
          succeeded_(succeeded) {
    }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      curl_easy_cleanup(worker_context_.curl);
      // This cleanup of share handle will only succeed if this request is the
      // final request that was active when the belonging session has been
      // destroyed before this request finishes.
      if (curl_share_cleanup(worker_context_.this_p->share_) == CURLSHE_OK) {
        worker_context_.this_p->share_ = NULL;
        DLOG("Hangover share handle successfully cleaned up");
      }

      WriteBodyTask::Call(main_loop, watch_id);
      if (worker_context_.this_p->curl_ == worker_context_.curl)
        worker_context_.this_p->Done(false, succeeded_);
      // Remove the internal reference that was added when the request was
      // started.
      worker_context_.this_p->Unref();
      return false;
    }

    bool succeeded_;
  };

  static void WorkerDone(unsigned short status, WorkerContext *context,
                         bool succeeded) {
    if (context->async) {
      // Do actual work in the main thread. AddTimeoutWatch() is threadsafe.
      context->this_p->main_loop_->AddTimeoutWatch(
          0, new DoneTask(status, context, succeeded));
    } else {
      context->this_p->Done(false, succeeded);
    }
  }

  static size_t WriteHeaderCallback(void *ptr, size_t size,
                                    size_t mem_block, void *user_p) {
    if (!CheckSize(0, size, mem_block))
      return 0;

    size_t data_size = size * mem_block;
    WorkerContext *context = static_cast<WorkerContext *>(user_p);
    // DLOG("XMLHttpRequest: WriteHeaderCallback: %zu*%zu this=%p",
    //      size, mem_block, context->this_p);
    if (context->async) {
      if (context->this_p->curl_ != context->curl) {
        // The current XMLHttpRequest has been aborted, so abort the
        // curl request.
        return 0;
      }

      // Do actual work in the main thread. AddTimeoutWatch() is threadsafe.
      context->this_p->main_loop_->AddTimeoutWatch(
          0, new WriteHeaderTask(ptr, data_size, context));
      return size * mem_block;
    } else {
      return context->this_p->WriteHeader(
          std::string(static_cast<char *>(ptr), data_size));
    }
  }

  size_t WriteHeader(const std::string &data) {
    ASSERT(state_ == OPENED && send_flag_);
    size_t size = data.length();
    if (CheckSize(response_headers_.length(), size, 1))
      response_headers_ += data;
    else
      size = 0;
    return size;
  }

  static size_t WriteBodyCallback(void *ptr, size_t size, size_t mem_block,
                                  void *user_p) {
    if (!CheckSize(0, size, mem_block))
      return 0;

    size_t data_size = size * mem_block;
    WorkerContext *context = static_cast<WorkerContext *>(user_p);
    // DLOG("XMLHttpRequest: WriteBodyCallback: %zu*%zu this=%p",
    //      size, mem_block, context->this_p);

    long curl_status = 0;
    curl_easy_getinfo(context->curl, CURLINFO_RESPONSE_CODE, &curl_status);
    unsigned short status = static_cast<unsigned short>(curl_status);

    if (context->async) {
      if (context->this_p->curl_ != context->curl) {
        // The current XMLHttpRequest has been aborted, so abort the
        // curl request.
        return 0;
      }

      // Do actual work in the main thread. AddTimeoutWatch() is threadsafe.
      context->this_p->main_loop_->AddTimeoutWatch(
          0, new WriteBodyTask(ptr, data_size, status, context));
      return data_size;
    } else {
      return context->this_p->WriteBody(
          std::string(static_cast<char *>(ptr), data_size), status);
    }
  }

  size_t WriteBody(const std::string &data, unsigned short status) {
    if (state_ == OPENED) {
      status_ = status;
      SplitStatusFromResponseHeaders(&response_headers_, &status_text_);
      ParseResponseHeaders(response_headers_,
                           &response_headers_map_,
                           &response_content_type_,
                           &response_encoding_);
      if (!ChangeState(HEADERS_RECEIVED) || !ChangeState(LOADING))
        return 0;
    }

    ASSERT(state_ == LOADING && send_flag_);
    size_t size = data.length();
    if (CheckSize(response_body_.length(), size, 1))
      response_body_ += data;
    else
      size = CURLE_WRITE_ERROR;
    return size;
  }

  void Done(bool aborting, bool succeeded) {
    if (curl_) {
      if (!send_flag_) {
        // This cleanup only happens if an XMLHttpRequest is opened but
        // no send() is called. For an active request, the curl handle will
        // be cleaned up when the request finishes or is aborted by error
        // return value of WriteHeader() and WriteBody().
        curl_easy_cleanup(curl_);
      }
      curl_ = NULL;
    }

    if (request_headers_) {
      curl_slist_free_all(request_headers_);
      request_headers_ = NULL;
    }

    bool save_send_flag = send_flag_;
    // Set send_flag_ to false early, to prevent problems when Done() is
    // re-entered.
    send_flag_ = false;
    succeeded_ = succeeded;
    bool no_unexpected_state_change = true;
    if ((state_ == OPENED && save_send_flag) ||
        state_ == HEADERS_RECEIVED || state_ == LOADING) {
      uint64_t now = main_loop_->GetCurrentTime();
      if (!aborting &&
          XHRBackoffReportResult(now, host_.c_str(), status_)) {
        SaveXHRBackoffData(now);
      }
      // The caller may call Open() again in the OnReadyStateChange callback,
      // which may cause Done() re-entered.
      no_unexpected_state_change = ChangeState(DONE);
    }

    if (aborting && no_unexpected_state_change) {
      // Don't dispatch this state change event, according to the spec.
      state_ = UNSENT;
    }
  }

  virtual void Abort() {
    response_headers_.clear();
    response_headers_map_.clear();
    response_body_.clear();
    response_text_.clear();
    status_ = 0;
    status_text_.clear();
    if (response_dom_) {
      response_dom_->Unref();
      response_dom_ = NULL;
    }

    Done(true, false);
  }

  virtual ExceptionCode GetAllResponseHeaders(const char **result) {
    ASSERT(result);
    if (state_ == HEADERS_RECEIVED || state_ == LOADING || state_ == DONE) {
      *result = response_headers_.c_str();
      return NO_ERR;
    }

    *result = NULL;
    LOG("XMLHttpRequest: GetAllResponseHeaders: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseHeader(const char *header,
                                          const char **result) {
    ASSERT(result);
    if (!header)
      return NULL_POINTER_ERR;

    *result = NULL;
    if (state_ == HEADERS_RECEIVED || state_ == LOADING || state_ == DONE) {
      CaseInsensitiveStringMap::iterator it = response_headers_map_.find(
          header);
      if (it != response_headers_map_.end())
        *result = it->second.c_str();
      return NO_ERR;
    }
    LOG("XMLHttpRequest: GetRequestHeader: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  void DecodeResponseText() {
    std::string encoding;
    response_dom_ = xml_parser_->CreateDOMDocument();
    response_dom_->Ref();
    if (!xml_parser_->ParseContentIntoDOM(response_body_, NULL, url_.c_str(),
                                          response_content_type_.c_str(),
                                          response_encoding_.c_str(),
                                          kEncodingFallback,
                                          response_dom_,
                                          &encoding, &response_text_) ||
        !response_dom_->GetDocumentElement()) {
      response_dom_->Unref();
      response_dom_ = NULL;
    }
  }

  virtual ExceptionCode GetResponseText(const char **result) {
    ASSERT(result);

    if (state_ == LOADING) {
      // Though the spec allows getting responseText while loading, we can't
      // afford this because we rely on XML/HTML parser to get the encoding.
      *result = "";
      return NO_ERR;
    } else if (state_ == DONE) {
      if (response_text_.empty() && !response_body_.empty())
        DecodeResponseText();

      *result = response_text_.c_str();
      return NO_ERR;
    }

    *result = NULL;
    LOG("XMLHttpRequest: GetResponseText: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseBody(const char **result, size_t *size) {
    ASSERT(result);
    ASSERT(size);

    if (state_ == LOADING || state_ == DONE) {
      *size = response_body_.length();
      *result = response_body_.c_str();
      return NO_ERR;
    }

    *size = 0;
    *result = NULL;
    LOG("XMLHttpRequest: GetResponseBody: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseBody(std::string *result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = response_body_;
      return NO_ERR;
    }

    result->clear();
    LOG("XMLHttpRequest: GetResponseBody: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseXML(DOMDocumentInterface **result) {
    ASSERT(result);

    if (state_ == DONE) {
      if (!response_dom_ && !response_body_.empty())
        DecodeResponseText();

      *result = response_dom_;
      return NO_ERR;
    }

    result = NULL;
    LOG("XMLHttpRequest: GetResponseXML: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatus(unsigned short *result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = status_;
      return NO_ERR;
    }

    *result = 0;
    LOG("XMLHttpRequest: GetStatus: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatusText(const char **result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = status_text_.c_str();
      return NO_ERR;
    }

    *result = NULL;
    LOG("XMLHttpRequest: GetStatusText: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual bool IsSuccessful() {
    return succeeded_;
  }

  // Used in the methods for script to throw an script exception on errors.
  bool CheckException(ExceptionCode code) {
    if (code != NO_ERR) {
      DLOG("XMLHttpRequest: Set pending exception: %d this=%p", code, this);
      SetPendingException(new XMLHttpRequestException(code));
      return false;
    }
    return true;
  }

  void ScriptOpen(const char *method, const char *url, bool async,
                  const char *user, const char *password) {
    CheckException(Open(method, url, async, user, password));
  }

  void ScriptSetRequestHeader(const char *header, const char *value) {
    CheckException(SetRequestHeader(header, value));
  }

  void ScriptSend(const Variant &v_data) {
    std::string data;
    if (v_data.ConvertToString(&data)) {
      CheckException(Send(data.c_str(), data.length()));
    } else if (v_data.type() == Variant::TYPE_SCRIPTABLE) {
      ScriptableInterface *scriptable =
          VariantValue<ScriptableInterface *>()(v_data);
      if (!scriptable ||
          scriptable->IsInstanceOf(DOMDocumentInterface::CLASS_ID)) {
        CheckException(Send(down_cast<DOMDocumentInterface *>(scriptable)));
      } else {
        CheckException(SYNTAX_ERR);
      }
    } else {
      CheckException(SYNTAX_ERR);
    }
  }

  const char *ScriptGetAllResponseHeaders() {
    const char *result = NULL;
    CheckException(GetAllResponseHeaders(&result));
    return result;
  }

  const char *ScriptGetResponseHeader(const char *header) {
    const char *result = NULL;
    CheckException(GetResponseHeader(header, &result));
    return result;
  }

  // We can't return std::string here, because the response body may be binary
  // and can't be converted from UTF-8 to UTF-16 by the script adapter.
  ScriptableBinaryData *ScriptGetResponseBody() {
    const char *result = NULL;
    size_t size = 0;
    if (CheckException(GetResponseBody(&result, &size)))
      return result ? new ScriptableBinaryData(result, size) : NULL;
    return NULL;
  }

  const char *ScriptGetResponseText() {
    const char *result = NULL;
    CheckException(GetResponseText(&result));
    return result;
  }

  DOMDocumentInterface *ScriptGetResponseXML() {
    DOMDocumentInterface *result = NULL;
    CheckException(GetResponseXML(&result));
    return result;
  }

  unsigned short ScriptGetStatus() {
    unsigned short result = 0;
    CheckException(GetStatus(&result));
    return result;
  }

  const char *ScriptGetStatusText() {
    const char *result = NULL;
    CheckException(GetStatusText(&result));
    return result;
  }

  CURL *curl_;
  CURLSH *share_;
  MainLoopInterface *main_loop_;
  XMLParserInterface *xml_parser_;
  Signal0<void> onreadystatechange_signal_;

  std::string url_, host_;
  bool async_;
  enum HTTPMethod { HTTP_HEAD, HTTP_GET, HTTP_POST, HTTP_PUT };
  HTTPMethod method_;

  State state_;
  // Required by the specification.
  // It will be true after send() is called in async mode.
  bool send_flag_;

  curl_slist *request_headers_;
  std::string response_headers_;
  std::string response_content_type_;
  std::string response_encoding_;
  unsigned short status_;
  bool succeeded_;
  std::string status_text_;
  std::string response_body_;
  std::string response_text_;
  DOMDocumentInterface *response_dom_;
  CaseInsensitiveStringMap response_headers_map_;
  pthread_attr_t thread_attr_;
  std::string default_user_agent_;
};

class XMLHttpRequestFactory : public XMLHttpRequestFactoryInterface {
 public:
  XMLHttpRequestFactory() : next_session_id_(1) {
  }

  virtual int CreateSession() {
    CURLSH *share = curl_share_init();
    if (share) {
      curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
      curl_share_setopt(share, CURLSHOPT_LOCKFUNC, Lock);
      curl_share_setopt(share, CURLSHOPT_UNLOCKFUNC, Unlock);
      int result = next_session_id_++;
      Session *session = &sessions_[result];
      session->share = share;
      session->share_ref = curl_easy_init();
      // Add a reference from "share_ref" to "share" to prevent "share" be
      // cleaned up by XMLHttpRequest instances.
      curl_easy_setopt(session->share_ref, CURLOPT_SHARE, share);
      return result;
    }
    return -1;
  }

  virtual void DestroySession(int session_id) {
    Sessions::iterator it = sessions_.find(session_id);
    if (it != sessions_.end()) {
      Session *session = &it->second;
      curl_easy_setopt(session->share_ref, CURLOPT_SHARE, NULL);
      // Cleanup the share_ref to prevent memory leak.
      curl_easy_cleanup(session->share_ref);
      // This cleanup will fail if there is still active requests. It'll be
      // actually cleaned up when the requests finish.
      CURLSHcode code = curl_share_cleanup(session->share);
      if (code != CURLSHE_OK) {
        DLOG("XMLHttpRequestFactory: Failed to DestroySession(): %s",
             curl_share_strerror(code));
      }
      sessions_.erase(it);
    } else {
      DLOG("XMLHttpRequestFactory::DestroySession Invalid session: %d",
           session_id);
    }
  }

  virtual XMLHttpRequestInterface *CreateXMLHttpRequest(
      int session_id, XMLParserInterface *parser) {
    if (session_id == 0) {
      return new XMLHttpRequest(NULL, GetGlobalMainLoop(), parser,
                                default_user_agent_);
    }

    Sessions::iterator it = sessions_.find(session_id);
    if (it != sessions_.end()) {
      return new XMLHttpRequest(it->second.share, GetGlobalMainLoop(), parser,
                                default_user_agent_);
    }

    DLOG("XMLHttpRequestFactory::CreateXMLHttpRequest: "
         "Invalid session: %d", session_id);
    return NULL;
  }

  virtual void SetDefaultUserAgent(const char *user_agent) {
    if (user_agent)
      default_user_agent_ = user_agent;
  }

  static void Lock(CURL *handle, curl_lock_data data,
                   curl_lock_access access, void *userptr) {
    // This synchronization scope is bigger than optimal, but is much simpler.
    pthread_mutex_lock(&mutex_);
  }

  static void Unlock(CURL *handle, curl_lock_data data, void *userptr) {
    pthread_mutex_unlock(&mutex_);
  }

 private:
  struct Session {
    CURLSH *share;
    CURL *share_ref;
  };

  typedef std::map<int, Session> Sessions;
  Sessions sessions_;
  int next_session_id_;
  std::string default_user_agent_;
  static pthread_mutex_t mutex_;
};

pthread_mutex_t XMLHttpRequestFactory::mutex_ = PTHREAD_MUTEX_INITIALIZER;

} // namespace curl
} // namespace ggadget

#define Initialize curl_xml_http_request_LTX_Initialize
#define Finalize curl_xml_http_request_LTX_Finalize

static ggadget::curl::XMLHttpRequestFactory gFactory;

extern "C" {
  bool Initialize() {
    LOGI("Initialize curl_xml_http_request extension.");
    return ggadget::SetXMLHttpRequestFactory(&gFactory);
  }

  void Finalize() {
    LOGI("Finalize curl_xml_http_request extension.");
  }
}
