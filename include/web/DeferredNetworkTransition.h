#pragma once

namespace web {

// Keeps the response/transition ordering explicit: a successful HTTP handler
// schedules one later transition, and the main loop consumes it once.
class DeferredNetworkTransition {
 public:
  bool schedule();
  bool consume();
  bool pending() const { return pending_; }

 private:
  bool pending_ = false;
};

}  // namespace web
