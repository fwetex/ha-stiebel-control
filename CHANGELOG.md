# Changelog

All notable changes are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions: [SemVer](https://semver.org/).

---

## [Unreleased]

### Added

- **`WW_SOLL` — combined DHW setpoint.** Replaces the two raw `EINSTELL_SPEICHERSOLLTEMP` /
  `EINSTELL_SPEICHERSOLLTEMP2` number entities and their `/set` topics. On the WPL 10 AC (WPM3)
  both Einstell registers are written together, because the active setpoint may follow either one.
- **`RAUM_SOLL` — room setpoint** on the main device. Same register as the `RAUMSOLLTEMP_I`
  entity, which stays on the sub-device; both are kept in sync, so either can be used.
- **Room boost offsets** `SG_READY_ROOM_BOOST_STATE3/4` (main device), the room-side counterpart
  of the DHW boost temperatures.
- **`heatingpump/DATETIME/set`** — strictly validated (`YYYY-MM-DD HH:MM:SS`) WPM clock set,
  replacing the `Update Datum` / `Update Uhrzeit` buttons.
- **ECO DHW setpoint is writable again** on every model except the WPL 10 AC, where it is kept in
  sync with `WW_SOLL` instead. A normal setpoint change never overwrites the ECO value.
- Operational diagnostics: uptime, reset reason and free-heap sensors; throttled logging for
  failed MQTT publishes; TWAI recovery return values checked instead of ignored. Reaching the
  entity cap (`MAX_PUBLISHED_SIGNALS`) now logs an **error** once per boot naming the dropped
  signal, and the periodic container-size log line reports the published-signal count against
  the cap, so silently missing entities are no longer possible.

### Changed

- **Renamed SG Ready boost entity names** — the friendly name changes, so Home Assistant entity
  IDs change and dashboards/automations referring to them must be updated:
  - `SG_READY_BOOST_STATE3`: "SG Ready Boost Zustand 3" → **"WW Boost Empfohlen"**
    (EN: "SG Ready Boost State 3" → **"DHW Boost Recommended"**)
  - `SG_READY_BOOST_STATE4`: "SG Ready Boost Zustand 4" → **"WW Boost Zwang"**
    (EN: "SG Ready Boost State 4" → **"DHW Boost Forced"**)
- CAN polling reworked: prebuilt poll slots replace the per-iteration string-keyed lookups and the
  per-frame heap allocations are gone. Invalid sentinel values (`-255`, `?`, `--`, `-32768`) are
  now skipped instead of creating permanently-unavailable entities — and when such a value arrives
  for an entity that already exists, it is published as `None` (unknown in Home Assistant) instead
  of silently keeping the last good value, which made a dead signal look like a live reading.
- Build/test: ESPHome 2026.9.1 in CI; the native test build tracks header dependencies.
- **The device now reboots after 10 minutes without an MQTT broker connection**
  (`mqtt: reboot_timeout: 600s`; previously `0s`, meaning never). Ordinary disconnects are still
  handled by the MQTT client's own reconnect — this is the recovery path for a client that is
  wedged (for example after heap starvation), which previously stayed dead until some unrelated
  reboot rescued it. Consequence to be aware of: while a broker outage genuinely lasts longer than
  10 minutes, expect one reboot per 10 minutes (each pausing CAN polling for ~30 s).
- **The time zone is configurable** via the `timezone` substitution (default `Europe/Berlin`)
  instead of being hard-coded. It is only the fallback for the clock written to the heat pump —
  Home Assistant 2026.3.0 and newer pushes its own zone on every time sync and the device adopts
  it. Users outside CET should set it; previously their heat pump clock was set to Berlin time.

- **The SG Ready watchdog window is configurable** — `sg_ready_watchdog_min` (minutes), default
  **720 = 12 h**, previously hard-coded to 3 h. An active boost that receives no command for that
  window falls back to Normal; standby is never auto-terminated. A small value such as `3` makes the
  watchdog observable on hardware within minutes (needs a recompile + re-flash). The longer default
  means a legitimate long boost from the control source is no longer cut off after 3 h — at the cost
  of a genuinely stuck boost persisting up to 12 h.

### Removed

- The `EINSTELL_SPEICHERSOLLTEMP` number entity and `/set` topic — use `WW_SOLL` instead.
- The `EINSTELL_SPEICHERSOLLTEMP2` number entity and `/set` topic **on the WPL 10 AC only**
  (merged into `WW_SOLL`); restored on all other models as the ECO setpoint.
- The `Update Datum` / `Update Uhrzeit` buttons and the `input_datetime.heatingpump_date` /
  `input_datetime.heatingpump_time` helpers — use `heatingpump/DATETIME/set` or the
  "WPM Datum & Uhrzeit übernehmen" button. The optional `packages/ha_stiebel_control.yaml`
  helper package is therefore obsolete; see [docs/INSTALLATION.md](docs/INSTALLATION.md).

### Fixed

- **CAN frames were silently dropped on the ESP32-S2 / MCP2515 build.** That driver refuses
  frames when its three TX buffers are busy (`ERROR_ALLTXBUSY`) instead of queueing them, and the
  return value was ignored — an SG Ready state change issues up to 8 frames from a single lambda,
  so everything past the third was lost with no log, no error, and Home Assistant still showing
  the new setpoints. `readSignal()`/`writeSignal()` now check the driver result and park refused
  frames for retry on the next 1 s tick. The ESP32-S3 build is unaffected: its TX queue is 64 deep
  and never refuses a frame.
- **A boost that survived a reboot was never rescued by the SG Ready watchdog.** The SG Ready
  state itself was never persisted (only the boost originals), so after a restart the state read
  back as *Normal* — the watchdog's "never commanded" guard also left it disarmed, and Home
  Assistant showed the wrong SG Ready state. The state is now stored alongside the boost
  originals, the watchdog is armed when a boost or standby is restored, and the time check moved
  into `SgReadyController::watchdogShouldRecover()` where it is unit-tested. Only a stuck boost
  (state 3/4) is recovered; standby is never auto-terminated. A survived boost is recovered one
  watchdog window after boot, since its real age is unknowable (the command timestamp is not
  persisted).
- **Scheduled tasks stalled for ~49 days after a `millis()` wrap.** `now >= deadline` on 32-bit
  millis values — the CAN poll schedule and the five calculated-sensor deadlines — reported a
  deadline set just before the wrap as far in the future. All six now use the wrap-safe
  `deadlineReached()` helper.
- **The CAN poll schedule always restarted at slot 0.** With one request per second, the
  lowest-indexed due signal won every iteration and the tail of the table was starved. The
  round-robin start index is restored, with a regression test.
- **An out-of-range date could be written to the heat pump clock.** `isValidDateString()` checked
  length, separators and digits only, so `2026-13-99` was accepted and sent to the WPM. Dates are
  now range-checked, including days per month and leap years (`2026-02-30`, `2025-02-29`).

### Migration

Friendly names, not entity IDs, are the stable reference here — Home Assistant derives the entity
ID from the name, so every rename above means a new entity ID.

| Old (name / topic) | New |
|---|---|
| "Speicher Soll Temperatur Einstellung" (`heatingpump/MANAGER/EINSTELL_SPEICHERSOLLTEMP/set`) | "WW Soll" (`heatingpump/MANAGER/WW_SOLL/set`) |
| "Speicher Soll Temperatur 2 Einstellung" (`…EINSTELL_SPEICHERSOLLTEMP2/set`) | WPL 10 AC: merged into "WW Soll". All other models: unchanged, writable again as the ECO setpoint |
| "Update Uhrzeit" / "Update Datum" buttons + `input_datetime` helpers | `heatingpump/DATETIME/set`, or the "WPM Datum & Uhrzeit übernehmen" button |
| "SG Ready Boost Zustand 3" | "WW Boost Empfohlen" |
| "SG Ready Boost Zustand 4" | "WW Boost Zwang" |
| — (new) | "Raum Soll", "Raum Boost Empfohlen", "Raum Boost Zwang" |

#### Removing entities from newly blacklisted registers

Registers that were published *before* they were blacklisted (e.g. `TEST_OBJEKT_*`, `ESTRICH_*`,
`FESTWERT`) keep their retained MQTT discovery config and go on showing their last value in Home
Assistant — the firmware simply stops publishing them, and a retained discovery config can only be
cleared from the broker side. Clear them once with ESPHome's built-in helper:

```sh
esphome clean-mqtt esphome/heatingpump.yaml --topic 'homeassistant/+/heatingpump/stiebel_kessel_estrich*/config'
esphome clean-mqtt esphome/heatingpump.yaml --topic 'homeassistant/+/heatingpump/stiebel_kessel_festwert/config'
esphome clean-mqtt esphome/heatingpump.yaml --topic 'homeassistant/+/heatingpump/stiebel_*_test_objekt_*/config'

esphome clean-mqtt esphome/heatingpump.yaml --topic 'heatingpump/KESSEL/ESTRICH_*/state'
esphome clean-mqtt esphome/heatingpump.yaml --topic 'heatingpump/KESSEL/FESTWERT/state'
esphome clean-mqtt esphome/heatingpump.yaml --topic 'heatingpump/*/TEST_OBJEKT_*/state'
```

Each command runs until you close it (Ctrl-C) — that is normal. **Always pass `--topic`**: without it
the helper targets `homeassistant/+/<name>/#`, i.e. every discovery config of the device.
Blacklisted registers are never re-published, so they stay gone; everything else reappears with the
next discovery republish (≤ 15 min).

---

## [2.1.1] — 2026-07-25

### Fixed

- **Compile error with ESPHome 2026.7** — `-DHA_DEVICE_MODEL='"wpl13e"'` relied on PlatformIO's
  shell-style quoting. The ESP-IDF/ninja build used since 2026.7 passes the flag through verbatim,
  so the quotes ended up in the source (`error: missing terminating ' character`). The flag is now
  quote-free and the string literal is built with `HA_DEVICE_MODEL_STR` in `config.h`. Closes #31.
- **`-Wformat` warnings** — `CanId` (`uint32_t`) printed with `%x`/`%u` warned under the
  ESP-IDF toolchain, where `uint32_t` is `long unsigned int`. Now cast explicitly.
- Self-heal from TWAI bus-off instead of requiring a manual reset.
- `PROGRAMMSCHALTER` option values are no longer translated (they are protocol values, not display
  names), and garbage CAN writes are rejected.
- Calculated/COP sensors and control sub-devices use the dynamic device id instead of a hard-coded
  model.
- `wpf10.yaml` was missing its `esphome: includes:` block.

## [2.1.0] — 2026-06-14

### Added

- **Multilingual entity names** — entity display names compile in German (default) or English.
  Set `language: "EN"` in `heatingpump.yaml` substitutions. Zero flash overhead for the unused
  language. Further languages can be added via `lang_XX.h` + one `#elif` in `language_select.h`.
  See `docs/CONTRIBUTING.md` for a step-by-step guide.
- **Writable room temperature setpoints** — `RAUMSOLLTEMP_II`, `RAUMSOLLTEMP_III`, and
  `RAUMSOLLTEMP_NACHT` now appear as writable `number` entities in HA (10–30 °C, 0.5 °C step),
  matching the existing `RAUMSOLLTEMP_I`. Closes #1.
- **CAN bus diagnostic sensors** (ESP32-S3 only) — four new sensors (`CAN TX Fehlerzähler`,
  `CAN RX Fehlerzähler`, `CAN Bus Fehler`, `CAN Bus Zustand`) published every 30 s using
  `twai_get_status_info()`. Appear under the diagnostic category, disabled by default in HA.
  Useful for debugging termination and ground issues without external tools. Closes #20.
- **Compressor runtime sensors** — `LZ_VERD_*` signals (`Laufzeit Verdichter`) now appear in HA
  with `device_class: duration`, `unit: h`, `state_class: total_increasing`. Polled every 10 min
  from both HEIZMODUL and MANAGER. Closes #3.

### Fixed

- `et_mil_val` signals incorrectly published as `device_class: temperature` / `unit: °C` — now
  correctly `device_class: duration` / `unit: h`. Resolves HA warning about missing unit.
- `LZ_VERD_*` signals had `isBlacklisted = true` in `ElsterTable.h`, silently dropping all CAN
  responses before MQTT publish.
- All-caps entity names for `ANTILEGIONELLEN`, `ANTILEGIONELLEN_ZEITPUNKT`, `BRENNER`,
  `GEBAEUDEART`, `HEIZKURVE`, `WW_ECO`, `WW_HYSTERSE` — now show proper display names.

### Improved

- **Test coverage** — 72 new unit test cases (392 assertions, up from 236). New coverage:
  MQTT discovery payload structure, `updateSensor` caching and global state dispatch, all
  calculated sensor publish functions, writable entity discovery, i18n macro resolution.
- **Documentation** — `language` substitution in both installation paths, localization
  contribution guide in `CONTRIBUTING.md`, updated entity reference tables in `CONFIGURATION.md`.

### Upgrade Notes

No breaking changes. Existing `heatingpump.yaml` files without a `language` substitution
compile with the German default — behaviour is unchanged. The new `LZ_VERD_*` sensors will
appear as new entities after flashing. CAN diagnostic sensors are disabled by default in HA.

---

## [2.0.0] — 2026-06-13

### Architecture

- **Multi-model architecture**: Signal request tables split into a universal base
  (`signal_requests_base.h`) and model-specific files. Adding a new heat pump model no
  longer requires editing shared code — only a new `.h` + `.yaml` file pair.
- **`device_model` substitution**: Controls the HA device identifier
  (`stiebel_eltron_<model>`). Existing WPL13E users are unaffected.
- **Generic sensors moved to `common.yaml`**: COP, energy accumulators, DHW temperature
  sensors, and operating mode sensors are now shared across all models.
  Model YAML files are now ~10 lines.

### SG Ready Refactor

- **`SgReadyController`**: Pure C++ class extracted from `ha-stiebel-control.h` with no
  ESPHome dependencies. All side effects (CAN writes, MQTT publishes, NVS persistence)
  injected via `ISgReadyIO` interface — fully unit-testable on the host.
- **Automatic temperature boost**: States 3 and 4 now apply DHW and room temperature
  boosts directly in firmware (no extra HA automations needed). Boost values configurable
  via `number.manager_sg_ready_boost_zustand_3` / `_4`.
- **NVS persistence**: Boost baseline temperatures survive device reboots.
- **Fixed `write→delay→read` ordering** in CAN signal writes.

### Testing

- **Native Catch2 test suite**: 138 test cases, 236 assertions covering `SgReadyController`,
  CAN protocol helpers (`generate_read_id`, `lookupCanMember`), Elster table functions
  (`SetValueType`, `TranslateString`, `GetElsterIndex`), and NUtils.
- Tests compile and run on the host with `make test` — no hardware or ESPHome toolchain needed.
- **MQTT smoke test harness** (`tests/test_mqtt_smoke.py`): post-OTA regression check.
  Subscribes for 120 seconds and verifies all required signals appear. Model-specific
  required topics in `tests/models/<model>_smoke.json`. Run with `make smoke-test`.

### New Models

- **WPF10 / WPF10M** (`wpf10.yaml`): community-contributed ground-source heat pump model
  based on PR #13 and issue #20. Signals marked unverified pending hardware validation.

### Release Infrastructure

- CI release job on `v*` tags: compiles both firmware variants, generates `manifest.json`,
  creates GitHub Release with firmware artifacts.
- `CHANGELOG.md`, `secrets.yaml.example`.

### Agentic Dev Setup

- `CLAUDE.md` importing `.github/copilot-instructions.md` — single source of truth for
  Claude Code and GitHub Copilot.
- `.claude/settings.json` — pre-approved `esphome`/`make`/`git` permissions.
- `Makefile` — `compile`, `compile-s2`, `check`, `test`, `smoke-test`, `upload`, `capture-baseline`.
- `.github/workflows/ci.yml` — compile + test + YAML lint on every push/PR.
- Issue templates (bug report, feature request).

### Documentation

- Full documentation split across `docs/`: INSTALLATION, CONFIGURATION, ARCHITECTURE,
  CONTRIBUTING, AGENTIC_DEVELOPMENT.
- README rewritten as a lean entry point with TOC.
- `SG_READY.md` translated to English and updated for current firmware behaviour.

### Bug Fixes

- **API encryption key**: Restored `encryption: key: !secret api_encryption_key` in
  `api:` block to maintain HA native API connection across OTA updates.

---

## [1.x] — 2023-08–2025-12

Major milestones in the 1.x series (see git log for full history):

### ESP32-S3 + Built-in CAN (late 2025)

- Added ESP32-S3 support with built-in TWAI CAN controller (256 RX queue vs 2 on MCP2515).
- Modular package architecture: `board_esp32s3.yaml`, `can_esp32.yaml`, `common.yaml`,
  `wpl13e.yaml` as separate composable packages.
- CAN buffer optimisation (256 RX / 64 TX entries).

### MQTT Auto-Discovery & Custom Discovery (2024–2025)

- Replaced ESPHome native MQTT discovery with custom C++ discovery publishing full HA
  metadata (device class, unit, state class, icon) per signal.
- Writable controls: MQTT number/select entities for temperature setpoints and operating mode.
- Calculated sensors: COP, Delta-T, compressor status, date/time, betriebsart.
- Frequency-based signal polling with round-robin scheduling and jitter.
- Signal blacklisting via `isBlacklisted` flag in `ElsterTable.h`.
- Per-signal HA metadata in `ElsterTable.h` (friendly names, units, device classes, icons).

### SG Ready v1 (late 2025)

- Initial SG Ready implementation with 4-state control.
- Boost amount configurable via HA number entities.
- NVS persistence for boost baseline temperatures.

### Elster Protocol Library (2023)

- Initial port of Jürg Müller's Elster protocol implementation to ESPHome.
- `ElsterTable.h` with 3800+ signal definitions.
- MCP2515 SPI CAN transceiver support.
- Initial WPL13E signal table.
