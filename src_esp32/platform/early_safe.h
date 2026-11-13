#pragma once
// Ранний safe-state до Serial/I2C (§11.1).
// M0: каркас. M1 выставит direct GPIO в OFF/pull-down по pinmap (DRAFT).

namespace platform {

void earlyApplicationSafe();

}  // namespace platform
