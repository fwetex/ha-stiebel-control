/*
 * SgReadyController — pure business logic for SG Ready state machine.
 *
 * No ESPHome dependencies. All side-effects (CAN writes, MQTT publishes,
 * NVS persistence) are injected via the ISgReadyIO interface so this class
 * can be unit-tested on the host without hardware.
 */

#ifndef SG_READY_CONTROLLER_H
#define SG_READY_CONTROLLER_H

#include <string>
#include <cstring>
#include <cstdint>

// SG Ready watchdog window, in MINUTES — overridable per installation via the
// `sg_ready_watchdog_min` substitution (build flag HA_SG_READY_WATCHDOG_MIN).
// 720 = 12 h (production default); a small value such as 3 makes the watchdog
// observable on hardware within minutes. The default below applies to builds that
// do not pass the flag (e.g. the native test build).
#ifndef HA_SG_READY_WATCHDOG_MIN
#define HA_SG_READY_WATCHDOG_MIN 720
#endif

// WPL 10 AC / WPM3 NOTE: the ACTIVE DHW setpoint (SPEICHERSOLLTEMP, 0x0003)
// follows the ECO setting (EINSTELL_SPEICHERSOLLTEMP2, 0x0a06) even in
// Komfortbetrieb — a WPM3 firmware quirk. (Writing the active register itself
// was tested and destabilized the WPM3's DHW setpoint handling; see git
// history.) Workaround: write the SAME DHW setpoint to BOTH Einstell
// registers (0x0013 and 0x0a06), so whichever mapping the WPM3 currently
// follows, boost and restore both apply. The two panel settings are
// deliberately kept in sync.
#ifdef HA_MODEL_wpl10ac
#define SG_READY_SYNC_DHW_SETTINGS
#endif

// ============================================================================
// IO INTERFACE — injected in production, stubbed in tests
// ============================================================================

class ISgReadyIO {
public:
    virtual ~ISgReadyIO() = default;

    // Write a CAN signal value (e.g. "RAUMSOLLTEMP_I" = "21.0")
    virtual void writeCanSignal(const char* signalName, const char* value) = 0;

    // Set the heat pump's operating mode by index into the model's
    // BetriebsartList (ElsterTable.h):
    //   0 Notbetrieb, 1 Bereitschaft, 2 Programmierbetrieb,
    //   3 Komfort, 4 Eco, 5 Warmwasser.
    // The label is MODEL-SPECIFIC — the WPM3 (WPL 10 AC) calls mode 1
    // "Bereitschaftsbetrieb", older WPM2 "Bereitschaft", and mode 3
    // "Komfortbetrieb" vs. "Tagbetrieb". The controller must therefore never
    // hard-code a label; resolving the index is the IO layer's job. (A literal
    // WPM2 name on a WPM3 build is refused by TranslateString(), so the write
    // silently never reaches the bus.)
    virtual void writeOperatingMode(int mode) = 0;

    // Publish a value to an MQTT state topic
    virtual void publishMqtt(const char* topic, const char* value) = 0;

    // Persist the SG Ready runtime state to NVS. `state` is the current SG Ready
    // state (1-4), stored alongside the boost originals so a boost or standby that
    // survives a reboot is still recognised afterwards.
    virtual void saveBoostState(float dhwTemp, float roomTemp, bool active, int state) = 0;

    // Load boost state from NVS; returns false if nothing stored
    virtual bool loadBoostState(float& dhwTemp, float& roomTemp, bool& active, int& state) = 0;

    // Read the device's current DHW / room setpoints (for capturing the
    // "original" values before a boost). Returns the values with 0.0 for any
    // setpoint that is unknown. Default implementation reports "unknown", so
    // test stubs and simple integrations keep the fallback behavior.
    virtual bool readCurrentSetpoints(float& dhwTemp, float& roomTemp) {
        dhwTemp = 0.0f;
        roomTemp = 0.0f;
        return false;
    }
};

// ============================================================================
// CONTROLLER
// ============================================================================

class SgReadyController {
public:
    // SG Ready state constants
    static constexpr int STATE_DISABLED = 0;
    static constexpr int STATE_EVU_LOCK  = 1;
    static constexpr int STATE_NORMAL    = 2;
    static constexpr int STATE_RECOMMENDED = 3;
    static constexpr int STATE_FORCED    = 4;

    // BetriebsartList indices used by SG Ready (model-independent — the label
    // behind them differs between WPM2 and WPM3, see writeOperatingMode()).
    static constexpr int MODE_STANDBY = 1;   // EVU lock
    static constexpr int MODE_COMFORT = 3;   // normal operation / boost

    // Default setpoint fallbacks (used when originals were never captured)
    static constexpr float DEFAULT_DHW_TEMP  = 48.0f;
    static constexpr float DEFAULT_ROOM_TEMP = 20.0f;

    // Temperature ceiling for DHW boost
    static constexpr float MAX_DHW_TEMP  = 60.0f;
    static constexpr float MAX_ROOM_TEMP = 25.0f;

    // Watchdog: only rescues from a stuck BOOST (3/4). Standby/EVU lock (1) is a
    // deliberate long-term state and never auto-terminated. If no SG Ready command
    // arrives for this window WHILE a boost is active, the controller returns to
    // Normal — protects against a permanently boosted heat pump if the control
    // source (evcc, HA automation, MQTT) stops talking.
    // The window is configured in minutes (see HA_SG_READY_WATCHDOG_MIN above).
    static_assert(HA_SG_READY_WATCHDOG_MIN <= 71582,
                  "sg_ready_watchdog_min is too large: it must fit the 32-bit millisecond counter (~49.7 days)");
    static constexpr uint32_t SG_READY_WATCHDOG_MS =
        static_cast<uint32_t>(HA_SG_READY_WATCHDOG_MIN) * 60u * 1000u;

    struct Config {
        float boostState3 = 5.0f;   // DHW boost for state 3 (°C)
        float boostState4 = 8.0f;   // DHW boost for state 4 (°C)
        float roomBoost3  = 1.0f;   // room boost for state 3 (°C)
        float roomBoost4  = 2.0f;   // room boost for state 4 (°C)
    };

    struct State {
        int   currentState    = STATE_NORMAL;
        bool  active          = false;
        float originalDhwTemp = 0.0f;
        float originalRoomTemp = 0.0f;
        // Last values written to NVS, so persistState() can skip unchanged writes (a control
        // source re-sending the same state would otherwise wear the flash for nothing).
        bool  nvsValid        = false;
        float nvsDhw          = 0.0f;
        float nvsRoom         = 0.0f;
        bool  nvsActive       = false;
        int   nvsState        = 0;
    };

    explicit SgReadyController(ISgReadyIO& io) : io_(io) {}

    // Called on boot: restore persisted boost state from NVS
    void loadFromNvs() {
        float dhw = 0.0f, room = 0.0f;
        bool active = false;
        int state = 0;
        if (io_.loadBoostState(dhw, room, active, state) && active) {
            state_.originalDhwTemp  = (dhw  > 0.0f) ? dhw  : DEFAULT_DHW_TEMP;
            state_.originalRoomTemp = (room > 0.0f) ? room : DEFAULT_ROOM_TEMP;
            state_.active = true;
        }
        // Restore the SG Ready state itself. Without it a boost (or standby) that
        // survived a reboot looks like Normal: the watchdog could never rescue a
        // stuck boost, and Home Assistant would show the wrong state.
        if (state >= STATE_EVU_LOCK && state <= STATE_FORCED) {
            state_.currentState = state;
        }
    }

    // Apply a new SG Ready state (1–4). Returns false for invalid state.
    bool applyState(int newState) {
        if (newState < STATE_EVU_LOCK || newState > STATE_FORCED) {
            return false;
        }

        state_.currentState = newState;

        // Publish new state to MQTT
        char topic[128];
        snprintf(topic, sizeof(topic), "heatingpump/MANAGER/SG_READY_STATE/state");
        io_.publishMqtt(topic, stateLabel(newState));

        switch (newState) {
            case STATE_EVU_LOCK:   applyEvuLock();    break;
            case STATE_NORMAL:     applyNormal();     break;
            case STATE_RECOMMENDED: applyRecommended(); break;
            case STATE_FORCED:     applyForced();     break;
        }
        return true;
    }

    // Update configurable boost amounts
    void setBoostState3(float boost) {
        if (boost >= 0.0f && boost <= 10.0f) {
            config_.boostState3 = boost;
        }
    }

    void setBoostState4(float boost) {
        if (boost >= 0.0f && boost <= 15.0f) {
            config_.boostState4 = boost;
        }
    }

    // Update configurable room boost amounts (°C)
    void setRoomBoost3(float boost) {
        if (boost >= 0.0f && boost <= 5.0f) {
            config_.roomBoost3 = boost;
        }
    }

    void setRoomBoost4(float boost) {
        if (boost >= 0.0f && boost <= 5.0f) {
            config_.roomBoost4 = boost;
        }
    }

    // Getters for inspection / sensor publishing
    int   currentState()    const { return state_.currentState; }
    bool  isActive()        const { return state_.active; }
    float boostState3()     const { return config_.boostState3; }
    float boostState4()     const { return config_.boostState4; }
    float roomBoost3()      const { return config_.roomBoost3; }
    float roomBoost4()      const { return config_.roomBoost4; }
    float originalDhwTemp() const { return state_.originalDhwTemp; }
    float originalRoomTemp() const { return state_.originalRoomTemp; }

    // Watchdog against a dead control source: should a boost be recovered?
    //
    // Only a stuck BOOST (states 3/4) is rescued. Standby (EVU lock) is a
    // deliberate long-term state and is never auto-terminated, and Normal needs no
    // recovery. `now` and `lastCommandTs` are passed in so this stays pure and
    // natively testable; the subtraction form is millis()-wrap safe.
    bool watchdogShouldRecover(uint32_t now, uint32_t lastCommandTs) const {
        if (state_.currentState == STATE_NORMAL) return false;
        if (state_.currentState == STATE_EVU_LOCK) return false;
        return (uint32_t)(now - lastCommandTs) >= SG_READY_WATCHDOG_MS;
    }

    // Human-readable label for a state value
    static const char* stateLabel(int state) {
        switch (state) {
            case STATE_EVU_LOCK:    return "1 - EVU Sperre";
            case STATE_NORMAL:     return "2 - Normal";
            case STATE_RECOMMENDED: return "3 - Empfohlen";
            case STATE_FORCED:     return "4 - Zwang";
            default:               return "Unbekannt";
        }
    }

    static const char* stateLabelFromString(const std::string& s) {
        // Parse "1 - ...", "2 - ...", etc.
        if (s.rfind("1 - ", 0) == 0) return stateLabel(STATE_EVU_LOCK);
        if (s.rfind("2 - ", 0) == 0) return stateLabel(STATE_NORMAL);
        if (s.rfind("3 - ", 0) == 0) return stateLabel(STATE_RECOMMENDED);
        if (s.rfind("4 - ", 0) == 0) return stateLabel(STATE_FORCED);
        return nullptr;
    }

    // Parse "1 - EVU Sperre" → 1, etc. Returns 0 on failure.
    static int parseStateString(const std::string& s) {
        if (s.rfind("1 - ", 0) == 0) return STATE_EVU_LOCK;
        if (s.rfind("2 - ", 0) == 0) return STATE_NORMAL;
        if (s.rfind("3 - ", 0) == 0) return STATE_RECOMMENDED;
        if (s.rfind("4 - ", 0) == 0) return STATE_FORCED;
        return 0;
    }

private:
    ISgReadyIO& io_;
    Config config_;
    State  state_;

    void captureOriginals() {
        // Prefer the device's CURRENT setpoints as the "original" values so a
        // boost/restore cycle returns to what the heat pump actually had —
        // not a hardcoded default. Unknown values (0.0) fall back to defaults.
        float curDhw = 0.0f, curRoom = 0.0f;
        io_.readCurrentSetpoints(curDhw, curRoom);

        if (!state_.active || state_.originalDhwTemp == 0.0f) {
            state_.originalDhwTemp = (curDhw > 0.0f) ? curDhw : DEFAULT_DHW_TEMP;
        }
        if (!state_.active || state_.originalRoomTemp == 0.0f) {
            state_.originalRoomTemp = (curRoom > 0.0f) ? curRoom : DEFAULT_ROOM_TEMP;
        }
    }

    // Write a DHW setpoint. On the WPL 10 AC / WPM3 the active setpoint may
    // follow either Einstell register (see note at the top) — write BOTH with
    // the same value so the panel settings stay in sync and boost/restore
    // apply regardless of which mapping the WPM3 currently uses.
    void writeDhwSetpoint(const char* value) {
        io_.writeCanSignal("EINSTELL_SPEICHERSOLLTEMP", value);
#ifdef SG_READY_SYNC_DHW_SETTINGS
        io_.writeCanSignal("EINSTELL_SPEICHERSOLLTEMP2", value);
#endif
    }

    void restoreDhw() {
        if (state_.active && state_.originalDhwTemp > 0.0f) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", state_.originalDhwTemp);
            writeDhwSetpoint(buf);
            state_.originalDhwTemp = 0.0f;
        }
    }

    void restoreRoom() {
        if (state_.active && state_.originalRoomTemp > 0.0f) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", state_.originalRoomTemp);
            io_.writeCanSignal("RAUMSOLLTEMP_I", buf);
            state_.originalRoomTemp = 0.0f;
        }
    }

    void persistState() {
        // Skip the NVS write when nothing changed. Every state application persists four
        // preferences; a repeated command (HA re-sending the same state) would otherwise cost
        // flash wear without changing anything.
        if (state_.nvsValid &&
            state_.nvsDhw == state_.originalDhwTemp &&
            state_.nvsRoom == state_.originalRoomTemp &&
            state_.nvsActive == state_.active &&
            state_.nvsState == state_.currentState) {
            return;
        }
        io_.saveBoostState(state_.originalDhwTemp, state_.originalRoomTemp, state_.active,
                           state_.currentState);
        state_.nvsValid = true;
        state_.nvsDhw = state_.originalDhwTemp;
        state_.nvsRoom = state_.originalRoomTemp;
        state_.nvsActive = state_.active;
        state_.nvsState = state_.currentState;
    }

    void applyEvuLock() {
        restoreDhw();
        restoreRoom();
        state_.active = true;
        persistState();
        io_.writeOperatingMode(MODE_STANDBY);
    }

    void applyNormal() {
        if (state_.active) {
            restoreDhw();
            restoreRoom();
            io_.writeOperatingMode(MODE_COMFORT);
            state_.active = false;
            persistState();
        }
    }

    void applyRecommended() {
        captureOriginals();
        state_.active = true;
        persistState();

        io_.writeOperatingMode(MODE_COMFORT);

        if (config_.boostState3 > 0.1f) {
            float boosted = state_.originalDhwTemp + config_.boostState3;
            if (boosted > MAX_DHW_TEMP) boosted = MAX_DHW_TEMP;
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", boosted);
            writeDhwSetpoint(buf);
        }

        if (config_.roomBoost3 > 0.1f) {
            float boostedRoom = state_.originalRoomTemp + config_.roomBoost3;
            if (boostedRoom > MAX_ROOM_TEMP) boostedRoom = MAX_ROOM_TEMP;
            char roomBuf[16];
            snprintf(roomBuf, sizeof(roomBuf), "%.1f", boostedRoom);
            io_.writeCanSignal("RAUMSOLLTEMP_I", roomBuf);
        }
    }

    void applyForced() {
        captureOriginals();
        state_.active = true;
        persistState();

        io_.writeOperatingMode(MODE_COMFORT);

        if (config_.boostState4 > 0.1f) {
            float boosted = state_.originalDhwTemp + config_.boostState4;
            if (boosted > MAX_DHW_TEMP) boosted = MAX_DHW_TEMP;
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", boosted);
            writeDhwSetpoint(buf);
        }

        if (config_.roomBoost4 > 0.1f) {
            float boostedRoom = state_.originalRoomTemp + config_.roomBoost4;
            if (boostedRoom > MAX_ROOM_TEMP) boostedRoom = MAX_ROOM_TEMP;
            char roomBuf[16];
            snprintf(roomBuf, sizeof(roomBuf), "%.1f", boostedRoom);
            io_.writeCanSignal("RAUMSOLLTEMP_I", roomBuf);
        }
    }
};

#endif // SG_READY_CONTROLLER_H
