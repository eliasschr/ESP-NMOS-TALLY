#include "network/ConnectionState.h"

namespace network {

ConnectionStateMachine::ConnectionStateMachine()
    : state_(ConnectionState::Unconfigured), deadline_(0), nextAttemptAt_(0),
      backoffMs_(kInitialBackoffMs), attemptsInBurst_(0) {}

ConnectionAction ConnectionStateMachine::begin(bool configured, uint32_t now) {
  if (!configured) {
    state_ = ConnectionState::Unconfigured;
    return ConnectionAction::None;
  }
  state_ = ConnectionState::Connecting;
  deadline_ = now + kConnectTimeoutMs;
  attemptsInBurst_ = 1;
  return ConnectionAction::Connect;
}

ConnectionAction ConnectionStateMachine::update(bool connected, uint32_t now) {
  if (state_ == ConnectionState::Unconfigured) return ConnectionAction::None;
  if (connected) {
    state_ = ConnectionState::Connected;
    backoffMs_ = kInitialBackoffMs;
    attemptsInBurst_ = 0;
    return ConnectionAction::None;
  }
  if (state_ == ConnectionState::Connected) return begin(true, now);
  if (state_ == ConnectionState::Connecting && static_cast<int32_t>(now - deadline_) >= 0) {
    return scheduleRetry(now);
  }
  if (state_ == ConnectionState::Backoff && static_cast<int32_t>(now - nextAttemptAt_) >= 0) {
    state_ = ConnectionState::Connecting;
    deadline_ = now + kConnectTimeoutMs;
    ++attemptsInBurst_;
    return ConnectionAction::Connect;
  }
  return ConnectionAction::None;
}

ConnectionAction ConnectionStateMachine::scheduleRetry(uint32_t now) {
  state_ = ConnectionState::Backoff;
  if (attemptsInBurst_ >= kMaximumAttemptsPerBurst) {
    // Limit a failed burst, then cool down before permitting another bounded
    // recovery burst. This avoids continuous connection churn on bad credentials.
    nextAttemptAt_ = now + kMaximumBackoffMs;
    backoffMs_ = kInitialBackoffMs;
    attemptsInBurst_ = 0;
    return ConnectionAction::None;
  }
  nextAttemptAt_ = now + backoffMs_;
  if (backoffMs_ < kMaximumBackoffMs / 2) backoffMs_ *= 2;
  else backoffMs_ = kMaximumBackoffMs;
  return ConnectionAction::None;
}

}  // namespace network
