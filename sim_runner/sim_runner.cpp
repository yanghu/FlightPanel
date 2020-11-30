#include "sim_runner/sim_runner.h"

#include <memory>

#include "absl/synchronization/notification.h"
#include "absl/time/clock.h"
#include "absl_helper/status_macros.h"
#include "data_dispatcher/data_dispatcher.h"
#include "data_reader/data_reader.h"
#include "sim_bridge/dispatch_handler.h"
#include "sim_bridge/sim_bridge.h"
#include "spdlog/spdlog.h"

namespace flight_panel {
namespace {
using data_dispatcher::DataDispatcher;
using sim_bridge::DispatchHandler;
using sim_bridge::SimBridge;

// Sim events to subscribe to.
constexpr absl::Duration kRetryInterval = absl::Seconds(10);
constexpr absl::Duration kLoopInterval = absl::Milliseconds(10);

// Add a handler to quite on stop.
class RunnerDispatchHandler : public DispatchHandler {
 public:
  RunnerDispatchHandler(SimRunner* runner)
      : runner_(runner) {}
  virtual absl::Status OnStart() override { return absl::OkStatus(); }
  virtual absl::Status OnStop() override {
    runner_->Stop();
    return absl::OkStatus();
  }
  virtual absl::Status OnData(int req_id, void* pData) override {
    return absl::OkStatus();
  }

 private:
  SimRunner* runner_;
};

class SimRunnerImpl : public SimRunner {
 public:
  static constexpr int kDataReadRequestID = 0;
  static constexpr int kDataReadDefID = 0;
  SimRunnerImpl(std::unique_ptr<SimBridge> bridge,
                std::unique_ptr<DataDispatcher> dispatcher);
  // Inherited via SimRunner
  virtual absl::Status Init() override;
  virtual absl::Status Run() override;
  virtual absl::Status Stop() override;

  absl::Status CleanUp();

  virtual sim_bridge::SimBridge* sim_bridge() override { return bridge_.get(); }
  virtual data_dispatcher::DataDispatcher* data_dispatcher() override {
    return dispatcher_.get();
  }

 private:
  absl::Status SubscribeEvents();

  bool ShouldQuit();
  bool connected_;
  std::unique_ptr<sim_bridge::GroupedDispatchHandler> dispatch_handler_;
  std::unique_ptr<RunnerDispatchHandler> runner_dispatch_handler_;
  std::unique_ptr<absl::Notification> stop_notification_;
  std::unique_ptr<DataReader> reader_;
  std::unique_ptr<sim_bridge::SimBridge> bridge_;
  std::unique_ptr<data_dispatcher::DataDispatcher> dispatcher_;
};

SimRunnerImpl::SimRunnerImpl(std::unique_ptr<SimBridge> bridge,
                             std::unique_ptr<DataDispatcher> dispatcher)
    : bridge_(std::move(bridge)),
      dispatcher_(std::move(dispatcher_)),
      connected_(false),
      reader_(CreateDataReader(kDataReadRequestID, kDataReadDefID, sim_bridge(),
                               data_dispatcher())) {
  // Initialize dispatch handler
  dispatch_handler_ = absl::make_unique<sim_bridge::GroupedDispatchHandler>();
  // This is the handler to stop the runner when stop signal is received from
  // the game.
  runner_dispatch_handler_ =
      absl::make_unique<RunnerDispatchHandler>(this);

  // Add reader to dispatch handler list.
  dispatch_handler_->AddHandler(reader_.get());
  dispatch_handler_->AddHandler(runner_dispatch_handler_.get());
}

absl::Status SimRunnerImpl::Init() {
  // Retry to connect
  while (!bridge_->Connect().ok()) {
    absl::SleepFor(kRetryInterval);
  }
  connected_ = true;
  RETURN_IF_ERROR(reader_->RegisterDataDef());
  RETURN_IF_ERROR(SubscribeEvents());
  return absl::OkStatus();
}

absl::Status SimRunnerImpl::Run() {
  stop_notification_ = absl::make_unique<absl::Notification>();
  // Start request data.
  bridge_->RequestData(kDataReadRequestID, kDataReadDefID,
                       sim_bridge::RefreshPeriod::VISUAL_FRAME);
  // Start a loop to read data.
  while (!ShouldQuit()) {
    // Process incoming data with dispatch handler.
    bridge_->CallDispatch(
        (sim_bridge::DispatchHandler*)dispatch_handler_.get());
    absl::SleepFor(kLoopInterval);
  }

  // Clean up before finishing.
  CleanUp();
  return absl::OkStatus();
}

// Stops the loop.
absl::Status SimRunnerImpl::Stop() {
  stop_notification_->Notify();
  return absl::OkStatus();
}

absl::Status SimRunnerImpl::CleanUp() {
  // Stop requesting data.
  RETURN_IF_ERROR(bridge_->RequestData(kDataReadRequestID, kDataReadDefID,
                                       sim_bridge::RefreshPeriod::NEVER));
  RETURN_IF_ERROR(bridge_->Close());
  return absl::Status();
}

absl::Status SimRunnerImpl::SubscribeEvents() {
  // Subscribe to start and stop events.
  RETURN_IF_ERROR(bridge_->SubscribeSystemEvent(data::SIM_START, "SimStart"));
  RETURN_IF_ERROR(bridge_->SubscribeSystemEvent(data::SIM_STOP, "SimStop"));
  return absl::OkStatus();
}

bool SimRunnerImpl::ShouldQuit() {
  return stop_notification_->HasBeenNotified();
}

}  // namespace
}  // namespace flight_panel
