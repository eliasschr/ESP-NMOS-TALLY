#include "config/ConfigRecovery.h"

namespace config {

RecoverySource selectRecoverySource(bool primaryIsValid, bool backupIsValid) {
  if (primaryIsValid) return RecoverySource::Primary;
  if (backupIsValid) return RecoverySource::Backup;
  return RecoverySource::None;
}

}  // namespace config
