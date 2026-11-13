#include "domain/mode_manager.h"

void ModeManager::begin(uint32_t nowMs) {
    (void)nowMs;
    mode_ = RunMode::BootSafe;
}

void ModeManager::finishSelfCheck(uint32_t nowMs, bool ok) {
    (void)nowMs;
    if (mode_ != RunMode::BootSafe) {
        return;
    }
    mode_ = ok ? RunMode::Off : RunMode::Fault;
}

bool ModeManager::requestManual(uint32_t nowMs, bool requested) {
    (void)nowMs;
    if (requested && mode_ == RunMode::Off) {
        mode_ = RunMode::Manual;
        return true;
    }
    if (!requested && mode_ == RunMode::Manual) {
        mode_ = RunMode::Off;
        return true;
    }
    return false;
}

void ModeManager::leaveManual(uint32_t nowMs) {
    (void)nowMs;
    if (mode_ == RunMode::Manual) {
        mode_ = RunMode::Off;
    }
}

bool ModeManager::requestAuto(uint32_t nowMs, bool permitProcess, bool latched) {
    (void)nowMs;
    if (mode_ != RunMode::Off) {
        return false;
    }
    if (!permitProcess || latched) {
        return false;
    }
    mode_ = RunMode::Auto;
    return true;
}

void ModeManager::forceFault(uint32_t nowMs) {
    (void)nowMs;
    // Fault — отражение BurnerState::Fault; в Manual/Auto допустимо,
    // BootSafe/Off/Latched не перетираем.
    if (mode_ == RunMode::Auto || mode_ == RunMode::Off) {
        mode_ = RunMode::Fault;
    }
}

void ModeManager::recordLatch(uint32_t nowMs) {
    (void)nowMs;
    if (mode_ == RunMode::BootSafe) {
        return;
    }
    // Запись о срабатывании. Postpurge и kAllOff остаются за BurnerState.
    mode_ = RunMode::Latched;
}

bool ModeManager::resetLatch(uint32_t nowMs) {
    (void)nowMs;
    if (mode_ != RunMode::Latched && mode_ != RunMode::Fault) {
        return false;
    }
    // M0: проверки §7.4 (resetRequest, свежий lease, постпродувка, активные
    // условия) добавит M4. Здесь — только переход режима.
    mode_ = RunMode::Off;
    return true;
}

void ModeManager::forceOff(uint32_t nowMs) {
    (void)nowMs;
    if (mode_ != RunMode::BootSafe) {
        mode_ = RunMode::Off;
    }
}
