#include "data_reader/data_reader.h"

#include <queue>
#include <thread>
#include <vector>

#include "absl/status/statusor.h"
#include "data_def/proto/sim_data.pb.h"
#include "data_def/sim_vars.h"
#include "data_dispatcher/data_dispatcher.h"
#include "sim_bridge/sim_bridge.h"
#include "spdlog/spdlog.h"

namespace flight_panel {
namespace {

using data_dispatcher::DataDispatcher;
using sim_bridge::SimBridge;

class DataReaderImpl : public DataReader {
 public:
  // Request ID and Def ID are used to identify data registered by this reader.
  // Do not use the same IDs for other objects.
  DataReaderImpl(int request_id, int def_id, SimBridge* bridge,
                 DataDispatcher* dispatcher)
      : req_id_(request_id),
        def_id_(def_id),
        bridge_(bridge),
        dispatcher_(dispatcher),
        data_length_(0){};
  virtual absl::Status OnStart() override { return absl::Status(); }
  virtual absl::Status OnStop() override { return absl::Status(); }

  virtual absl::Status OnData(int req_id, void* pData) override {
    if (req_id != req_id_) return absl::OkStatus();
    // request ID matches. Start processing data.
    // Copy data to a SimVars struct
    data::SimVars data_buf;
    memcpy(&data_buf, pData, data_length_);
    return dispatcher_->Notify(data_buf);
  }

  // Add data definition to the bridge.
  virtual absl::Status RegisterDataDef() override {
    using data::SimVarDefs;
    for (int i = 0; SimVarDefs[i][0] == NULL; ++i) {
      auto status_or =
          bridge_->AddDataDef(def_id_, SimVarDefs[i][0], SimVarDefs[i][1]);
      if (!status_or.ok()) {
        return status_or.status();
      }
      data_length_ += status_or.value();
    }
    return absl::OkStatus();
  }

 private:
  int req_id_;
  int def_id_;
  int data_length_;
  // Dispatch the data read from SimBridge to clients. (WS, Serial, etc.)
  // This object doesn't own the dispatcher, since the clients also need to
  // attach themselves to the dispatcher.
  DataDispatcher* dispatcher_;
  SimBridge* bridge_;
};
}  // namespace

std::unique_ptr<DataReader> CreateDataReader(int request_id, int data_def_id,
                                             SimBridge* bridge,
                                             DataDispatcher* dispatcher) {
  return absl::make_unique<DataReaderImpl>(request_id, data_def_id, bridge,
                                           dispatcher);
}
// namespace
}  // namespace flight_panel
