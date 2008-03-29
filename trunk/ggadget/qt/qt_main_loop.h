#ifndef GGADGET_QT_QT_MAIN_LOOP_H__
#define GGADGET_QT_QT_MAIN_LOOP_H__
#include <QSocketNotifier>
#include <QTimer>
#include <ggadget/main_loop_interface.h>

namespace ggadget {
namespace qt {

// QtMainLoop is a qt4 implementation of MainLoopInterface interface.
class QtMainLoop : public MainLoopInterface {
 public:
  QtMainLoop();
  virtual ~QtMainLoop();
  virtual int AddIOReadWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddIOWriteWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddTimeoutWatch(int interval, WatchCallbackInterface *callback);
  virtual WatchType GetWatchType(int watch_id);
  virtual int GetWatchData(int watch_id);
  virtual void RemoveWatch(int watch_id);
  virtual void Run();
  virtual bool DoIteration(bool may_block);
  virtual void Quit();
  virtual bool IsRunning() const;
  virtual uint64_t GetCurrentTime() const;

 private:
  class Impl;
  Impl *impl_;
};

class WatchNode : public QObject {
  Q_OBJECT
 public:
  MainLoopInterface::WatchType type_;
  bool calling_, removing_;
  MainLoopInterface *main_loop_;
  WatchCallbackInterface *callback_;
  QObject *object_;   // pointer to QSocketNotifier or QTimer
  int watch_id_;
  int data_;      // For IO watch, it's fd, for timeout watch, it's interval.

  WatchNode() {
    calling_ = removing_ = false;
    main_loop_ = NULL;
    callback_ = NULL;
    object_ = NULL;
    watch_id_ = -1;
  }

 public slots:
  void OnTimeout() {
    if (!calling_ && !removing_) {
      calling_ = true;
      bool ret = callback_->Call(main_loop_, watch_id_);
      calling_ = false;
      if (!ret || removing_) {
        main_loop_->RemoveWatch(watch_id_);
        QTimer* timer = reinterpret_cast<QTimer *>(object_);
        timer->stop();
      }
    }
  }

  void OnIOEvent(int fd) {
    if (!calling_ && !removing_) {
      calling_ = true;
      bool ret = callback_->Call(main_loop_, watch_id_);
      calling_ = false;
      if (!ret || removing_) {
        main_loop_->RemoveWatch(watch_id_);
        QSocketNotifier *notifier =
            reinterpret_cast<QSocketNotifier*>(object_);
        notifier->setEnabled(false);
      }
    }
  }
};

} // namespace qt
} // namespace ggadget
#endif  // GGADGET_QT_QT_MAIN_LOOP_H__
