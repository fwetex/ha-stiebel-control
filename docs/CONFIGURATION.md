# Configuration Reference

---

## `secrets.yaml` Keys

All keys are required unless marked optional.

| Key | Description | Example |
|-----|-------------|---------|
| `wifi_ssid` | Wi-Fi network name | `"MyNetwork"` |
| `wifi_password` | Wi-Fi password | `"MyPassword"` |
| `mqtt_broker` | MQTT broker IP or hostname | `192.168.1.10` |
| `mqtt_username` | MQTT username | `"iotuser"` |
| `mqtt_password` | MQTT password | `"secret"` |
| `api_encryption_key` | 32-byte base64 key for ESPHome native API | `"abc...=="` |
| `ota_password` | OTA update password | `"change_me"` |

Generate `api_encryption_key`:
```bash
python3 -c "import base64, os; print(base64.b64encode(os.urandom(32)).decode())"
```

The key must match what HA's ESPHome integration has stored for this device. If you change it, you need to re-adopt the device in HA (Settings → Devices & Services → ESPHome → delete entry, then re-add).

---

## `heatingpump.yaml` Substitutions

| Substitution | Description | Default |
|--------------|-------------|---------|
| `device_name` | ESPHome device hostname | `heatingpump` |
| `friendly_name` | Display name in HA | `"Stiebel Eltron Wärmepumpe"` |
| `device_model` | Heat pump model — selects signal table and HA device identifier | `"wpl13e"` |
| `language` | Display language for HA entity names: `DE` (German, default) or `EN` (English) | `"DE"` |
| `can_tx_pin` | CAN TX GPIO (ESP32-S3 built-in TWAI) | board-specific — see note below |
| `can_rx_pin` | CAN RX GPIO (ESP32-S3 built-in TWAI) | board-specific — see note below |
| `can_id_pc` | CAN bus ID for the ESP32 (PC role) | `"0x680"` |
| `can_clk_pin` | SPI CLK (MCP2515 only) | board-specific — see note below |
| `can_mosi_pin` | SPI MOSI (MCP2515 only) | board-specific — see note below |
| `can_miso_pin` | SPI MISO (MCP2515 only) | board-specific — see note below |
| `can_cs_pin` | SPI CS (MCP2515 only) | board-specific — see note below |

> **Pin assignments are board-specific.** Always verify against your board's schematic
> before wiring. Known working values for common boards:
>
> **Waveshare ESP32-S3-RS485-CAN** (recommended board):
> `can_tx_pin: GPIO15`, `can_rx_pin: GPIO16`
>
> **Generic ESP32 DevKit + MCP2515 module:**
> `can_clk_pin: GPIO18`, `can_mosi_pin: GPIO23`, `can_miso_pin: GPIO19`, `can_cs_pin: GPIO5`
>
> These are the values shipped in `heatingpump.yaml`. Other boards may use different pins.

---

## Writable Controls

These entities appear automatically in HA via MQTT discovery:

### Temperature Setpoints (number entities)

| Entity ID | Description | Range | Step |
|-----------|-------------|-------|------|
| `number.manager_ww_soll` | DHW target temperature (`WW_SOLL`) | 20–60°C | 1°C |
| `number.manager_speicher_soll_temperatur_2_einstellung` | DHW ECO target temperature — **not on the WPL 10 AC**, where `WW_SOLL` keeps that register in sync | 20–60°C | 1°C |
| `number.manager_raum_soll` | Room setpoint (`RAUM_SOLL`) — main-device alias of circuit 1 below | 10–30°C | 0.5°C |
| `number.manager_raum_soll_temperatur_i_einstellung` | Room setpoint circuit 1 | 10–30°C | 0.5°C |
| `number.manager_raum_soll_temperatur_ii_einstellung` | Room setpoint circuit 2 | 10–30°C | 0.5°C |
| `number.manager_raum_soll_temperatur_iii_einstellung` | Room setpoint circuit 3 | 10–30°C | 0.5°C |
| `number.manager_raum_soll_temperatur_nacht_einstellung` | Room setpoint night/reduced | 10–30°C | 0.5°C |
| `number.manager_ww_boost_empfohlen` | SG Ready DHW boost for state 3 | 0–10°C | 0.5°C |
| `number.manager_ww_boost_zwang` | SG Ready DHW boost for state 4 | 0–15°C | 0.5°C |
| `number.manager_raum_boost_empfohlen` | SG Ready room boost for state 3 | 0–5°C | 0.5°C |
| `number.manager_raum_boost_zwang` | SG Ready room boost for state 4 | 0–5°C | 0.5°C |

> **Changed in this version:** the two DHW setpoint entities were combined into `WW_SOLL`, the SG
> Ready boost entities were renamed ("SG Ready Boost Zustand 3/4" → "WW Boost Empfohlen/Zwang"),
> and the `Update Datum` / `Update Uhrzeit` buttons were replaced by `heatingpump/DATETIME/set`.
> Home Assistant derives entity IDs from the friendly name, so dashboards and automations that use
> the old entities need updating — see the migration table in [CHANGELOG.md](../CHANGELOG.md).

### Compressor Runtime Counters (WPL13E, sensor entities)

| Entity ID | Description | Unit |
|-----------|-------------|------|
| `sensor.heizmodul_lz_verd_1_heizbetrieb` | Compressor 1 heating runtime | h |
| `sensor.heizmodul_lz_verd_2_heizbetrieb` | Compressor 2 heating runtime | h |
| `sensor.heizmodul_lz_verd_1_2_heizbetrieb` | Compressor 1+2 heating runtime | h |
| `sensor.heizmodul_lz_verd_1_kuehlbetrieb` | Compressor 1 cooling runtime | h |
| `sensor.heizmodul_lz_verd_2_kuehlbetrieb` | Compressor 2 cooling runtime | h |
| `sensor.heizmodul_lz_verd_1_2_kuehlbetrieb` | Compressor 1+2 cooling runtime | h |

### CAN Bus Diagnostics (ESP32-S3 only, sensor entities)

Available automatically on ESP32-S3 / TWAI builds. Not present on MCP2515 builds.

| Entity ID | Description |
|-----------|-------------|
| `sensor.can_tx_fehlerzahler` | TX Error Counter (TEC) — rises on failed transmissions |
| `sensor.can_rx_fehlerzahler` | RX Error Counter (REC) — rises on receive errors |
| `sensor.can_bus_fehler` | Total bus error count (cumulative) |
| `sensor.can_bus_zustand` | Bus state: Läuft / Gestoppt / Bus-Off / Erholung |

TEC/REC above 96 indicates warning level; above 127 the controller enters bus-off.

### Mode Selects (select entities)

| Entity ID | Options |
|-----------|---------|
| `select.manager_programmschalter` | Model-dependent (from `BetriebsartList`): WPL 10 AC / WPM3 = Notbetrieb, **Bereitschaftsbetrieb**, Programmierbetrieb, **Komfortbetrieb**, Eco-Modus, Warmwasser; WPM2 = Notbetrieb, Bereitschaft, Automatik, Tagbetrieb, Absenkbetrieb, Warmwasser |
| `select.manager_sg_ready_zustand` | 1 - EVU Sperre, 2 - Normal, 3 - Empfohlen, 4 - Zwang |

---

## Read-Only Calculated Sensors

Published to `heatingpump/calculated/` MQTT topics and auto-discovered in HA:

| Sensor | Topic | Description |
|--------|-------|-------------|
| Date | `heatingpump/calculated/date/state` | `YYYY-MM-DD` from heat pump clock |
| Time | `heatingpump/calculated/time/state` | `HH:MM:SS` from heat pump clock |
| Betriebsart | `heatingpump/calculated/betriebsart/state` | "Sommerbetrieb" or "Normalbetrieb" |
| Delta T (continuous) | `heatingpump/calculated/delta_t_continuous/state` | Flow − return temp, always |
| Delta T (running) | `heatingpump/calculated/delta_t_running/state` | Flow − return temp, only when compressor active |
| Compressor active | `heatingpump/calculated/compressor_active/state` | on/off |
| COP hot water | `heatingpump/calculated/cop_ww/state` | Coefficient of performance (DHW) |
| COP heating | `heatingpump/calculated/cop_heiz/state` | Coefficient of performance (heating) |
| COP total | `heatingpump/calculated/cop_gesamt/state` | Overall coefficient of performance |

---

## MQTT Topic Structure

All raw CAN signal values:
```
heatingpump/{CAN_MEMBER}/{SIGNAL_NAME}/state     ← read
heatingpump/{CAN_MEMBER}/{SIGNAL_NAME}/set       ← write (writable signals only)
```

Examples:
```
heatingpump/MANAGER/PROGRAMMSCHALTER/state       → "Tagbetrieb"
heatingpump/MANAGER/WW_SOLL/set                  ← "55"
heatingpump/KESSEL/SPEICHERISTTEMP/state         → "51.8"
```

CAN members: `MANAGER` (0x480), `KESSEL` (0x180), `HEIZMODUL` (0x500)

Availability:
```
heatingpump/status    → "online" or "offline"
```

---

## Firmware Version & Build Date

The firmware carries the git version it was built from. `make compile`, `make upload` and
`make compile-s2` derive it automatically:

```bash
git describe --tags --match 'v*' --always --dirty   # e.g. v2.1.1-63-gdf48757
```

and export it as `FW_VERSION`; `common.yaml` reads it with `!env_var FW_VERSION "dev"` into
`esphome.project.version`. CI does the same. Where it shows up:

| Place | Content |
|---|---|
| HA device page (main device and all CAN member sub-devices) | `sw_version`: `v2.1.1-63-gdf48757 (ESPHome 2026.9.1)` |
| Diagnostic text sensor `Firmware` (on the ESPHome node device) | version + compile timestamp, e.g. `v2.1.1-63-gdf48757 · Oct 25 2026 18:42:07` |

Notes:

- A `-dirty` suffix means the build included uncommitted changes. The version is baked at build
  time, so rebuild after committing to get a clean version.
- A build started directly with `esphome compile` (ESPHome dashboard) has no `FW_VERSION` in its
  environment and reports `dev`. Use the Makefile (or export `FW_VERSION` yourself) for
  identifiable images.
- Override explicitly with `make compile FW_VERSION=my-build` if needed.

---

## Polling Frequencies

Edit `esphome/ha-stiebel-control/signal_requests_wpl13e.h` (or your model's file):

```cpp
const SignalRequest signalRequests[] = {
    SIGNAL_REQUESTS_BASE           // universal signals, fixed frequencies

    {"AUSSENTEMP",   FREQ_30S,  cm_kessel},    // every 30 seconds
    {"HEIZKURVE",    FREQ_10MIN, cm_manager},   // every 10 minutes
};
```

Available frequency constants:

| Constant | Value |
|----------|-------|
| `FREQ_10S` | 10 seconds |
| `FREQ_30S` | 30 seconds |
| `FREQ_1MIN` | 60 seconds |
| `FREQ_5MIN` | 5 minutes |
| `FREQ_10MIN` | 10 minutes |
| `FREQ_30MIN` | 30 minutes |
| `FREQ_60MIN` | 60 minutes |

---

## Blacklisting Signals

To permanently suppress a signal (prevent it from appearing in HA), set `isBlacklisted: true`
in its `ElsterTable.h` entry:

```cpp
{ "NOISY_SIGNAL", 0x0123, et_default, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  true,   // ← isBlacklisted
  false }
```

Blacklisted signals are skipped before any processing — they never generate MQTT messages
or HA entities.
