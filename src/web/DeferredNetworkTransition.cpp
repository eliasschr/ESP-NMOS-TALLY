#include "web/DeferredNetworkTransition.h"

namespace web {

bool DeferredNetworkTransition::schedule() {
  if (pending_) return false;
  pending_ = true;
  return true;
}

bool DeferredNetworkTransition::consume() {
  if (!pending_) return false;
  pending_ = false;
  return true;
}

}  // namespace web
