#include "data_dispatcher/data_dispatcher.h"

#include <queue>
#include <thread>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/strings/str_format.h"
#include "absl/synchronization/mutex.h"
#include "absl/synchronization/notification.h"
#include "spdlog/spdlog.h"

namespace flight_panel {
namespace data_dispatcher {
namespace {
class DataDispatcherImpl : public DataDispatcher {
 public:
  DataDispatcherImpl(){};

  // Inherited via DataDispatcher
  virtual void Start() override {
    // Reset stop notification.
    stop_notification_ = absl::make_unique<absl::Notification>();
    std::thread([this] { RunWorker(); }).detach();
  }
  // Stop the worker thread, by calling "notify".
  virtual void Stop() override { stop_notification_->Notify(); }

  virtual absl::Status AddRecepient(DispatchCallback notify_callback)
      LOCKS_EXCLUDED(callbacks_lock) override {
    absl::MutexLock l(&callbacks_lock_);
    dispatch_callbacks_.push_back(notify_callback);
    return absl::OkStatus();
  }

  virtual absl::Status Notify(flight_panel::SimVars new_data)
      LOCKS_EXCLUDED(data_lock_) override {
    absl::MutexLock l(&data_lock_);
    sim_vars_.push(new_data);
    return absl::OkStatus();
  }

  virtual bool IsRunning() override {
    return stop_notification_ && !stop_notification_->HasBeenNotified();
  }

 private:
  void RunWorker();
  absl::Status DispatchData(const std::string& serialized_data);
  std::thread worker_;
  std::unique_ptr<absl::Notification> stop_notification_;
  absl::Mutex callbacks_lock_;
  std::vector<DispatchCallback> dispatch_callbacks_ GUARDED_BY(callbacks_lock_);
  // Lock for sim_vars_ vector.
  absl::Mutex data_lock_;
  std::queue<SimVars> sim_vars_ GUARDED_BY(data_lock_);
};

void DataDispatcherImpl::RunWorker() {
  spdlog::info("Started run worker");
  auto data_not_empty_or_stop = [this] {
    return stop_notification_->HasBeenNotified() || !sim_vars_.empty();
  };

  // Execute the loop until stop is notified.
  while (!stop_notification_->HasBeenNotified()) {
    data_lock_.Lock();
    spdlog::info("Waiting for new data....");
    data_lock_.Await(absl::Condition(&data_not_empty_or_stop));
    if (stop_notification_->HasBeenNotified()) {
      spdlog::info("Cancel notification received.");
      break;
    }
    spdlog::info("Reading new data");
    // Data is not empty, process new data.
    SimVars data = sim_vars_.front();
    sim_vars_.pop();
    data_lock_.Unlock();
    // TODO
    // Convert SimVars to proto message
    // Dispatch serialized proto data.
    spdlog::info("Dispatching dummy data");
    DispatchData("dummy data");
    continue;
  }
  return;
}
absl::Status DataDispatcherImpl::DispatchData(
    const std::string& serialized_data) {
  spdlog::info("Entered dispatching.");
  absl::MutexLock l(&callbacks_lock_);
  spdlog::info(
      absl::StrFormat("Size of callback list: %d", dispatch_callbacks_.size()));
  for (const auto callback : dispatch_callbacks_) {
    spdlog::info("Sending callback");
    callback(serialized_data);
  }
  spdlog::info("Finishing dispatching.");
  return absl::OkStatus();
}
}  // namespace

std::unique_ptr<DataDispatcher> CreateDispatcher() {
  return absl::make_unique<DataDispatcherImpl>();
}

}  // namespace data_dispatcher
}  // namespace flight_panel
