#pragma once

#include <stdint.h>

namespace network {

enum class ConnectionState : uint8_t { Unconfigured, Connecting, Connected, Backoff };
enum class ConnectionAction : uint8_t { None, Connect };

// Arduino-independent reconnect policy. The controller performs the actual
// Wi-Fi calls; this class makes timeout and backoff decisions testable.
class ConnectionStateMachine {
 public:
  static constexpr uint32_t kConnectTimeoutMs = 20000;
  static constexpr uint32_t kInitialBackoffMs = 2000;
  static constexpr uint32_t kMaximumBackoffMs = 60000;
  static constexpr uint8_t kMaximumAttemptsPerBurst = 3;

  ConnectionStateMachine();
  ConnectionAction begin(bool configured, uint32_t now);
  ConnectionAction update(bool connected, uint32_t now);
  ConnectionState state() const { return state_; }
  uint32_t nextAttemptAt() const { return nextAttemptAt_; }
  uint16_t attemptCount() const { return attemptCount_; }

 private:
  ConnectionAction scheduleRetry(uint32_t now);

  ConnectionState state_;
  uint32_t deadline_;
  uint32_t nextAttemptAt_;
  uint32_t backoffMs_;
  uint8_t attemptsInBurst_;
  uint16_t attemptCount_;
};

}  // namespace network
