#pragma once

namespace config {

enum class RecoverySource { None, Primary, Backup };

RecoverySource selectRecoverySource(bool primaryIsValid, bool backupIsValid);

}  // namespace config
