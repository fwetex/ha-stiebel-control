# SG-Ready-Steuerung der WPL 10 AC — Spickzettel

Kurzanleitung: **evcc regelt, HA überwacht, ESP (CAN) führt aus.** Stand: 2026-10-11.

---

## 1. Architektur

```
evcc (Regler)  ──MQTT──▶  ESP (ha-stiebel-control)  ──CAN B 50 kbps──▶  WPM3 / WPL 10 AC
                              │
HA-Monitor (scharfer Watchdog) ──MQTT──▶  select.heatingpump_sg_ready_state
```

| Ebene | Rolle |
|---|---|
| **evcc** | PV-/Netz-/Tarif-Logik; schreibt SG-Ready-Modus **nur bei Änderung** (kein Heartbeat nötig) |
| **ESP** | setzt `PROGRAMMSCHALTER` + Solltwerte; **Firmware-Watchdog**: nur Boost 3/4 → Normal nach **3 h** Stille; **Standby (1) wird nie automatisch beendet** |
| **HA** | kontextualer Watchdog (Boost > 1 h UND PV < Schwelle → Normal), ESP-offline-Erkennung, ggf. Shelly-Fallback |

---

## 2. Zustands-Mapping (evcc → WPM)

| evcc (`sgready` mode) | MQTT-Payload auf `heatingpump/MANAGER/SG_READY_STATE/set` | WPM-Wirkung |
|---|---|---|
| `1` reduced | `1 - EVU Sperre` | `PROGRAMMSCHALTER` = Bereitschaftsbetrieb (Standby, Frostschutz aktiv) |
| `2` normal | `2 - Normal` | Komfortbetrieb, Original-Solltwerte wiederhergestellt |
| `3` boost | `3 - Empfohlen` | Komfort + `SPEICHERSOLLTEMP` +`BOOST_STATE3` |
| (optional statt 3) | `4 - Zwang` | Komfort + stärkerer WW-/Raum-Boost (`BOOST_STATE4`) |

Boost-Stärke (via HA-Number/`/set`):
`heatingpump/MANAGER/SG_READY_BOOST_STATE3/set` (z.B. `5.0`), `..._STATE4/set` (z.B. `8.0`).

---

## 3. evcc.yaml (Custom-Gerät `sgready`)

```yaml
chargers:
  - name: wpl10
    type: sgready
    features:
      - integrateddevice
      - heating
      # - switchdevice     # ohne continuous: Modes Off/Smart/On → „Off" = reduced
    setmode:
      source: switch
      switch:
        - case: 1
          set:
            source: mqtt
            broker: <broker-ip>:1883     # deine secrets.yaml-Werte
            user: <user>
            password: <pass>
            topic: heatingpump/MANAGER/SG_READY_STATE/set
            payload: "1 - EVU Sperre"
        - case: 2
          set:
            source: mqtt
            topic: heatingpump/MANAGER/SG_READY_STATE/set
            payload: "2 - Normal"
        - case: 3
          set:
            source: mqtt
            topic: heatingpump/MANAGER/SG_READY_STATE/set
            payload: "3 - Empfohlen"
    # optional: aktuellen Modus für evcc-UI zurücklesen
    getmode:
      source: mqtt
      topic: heatingpump/MANAGER/SG_READY_STATE/state
      # map: "1 - …"→1, "2 - …"→2, "3/4 - …"→3

loadpoints:
  - title: Wärmepumpe
    charger: wpl10
    enable:
      threshold: -1300      # ab 1,3 kW Überschuss → ON/Boost
      delay: 5m             # Mindest-Laufzeit (Takten vermeiden!)
    disable:
      threshold: 150
      delay: 20m
```

Hinweis: ohne `continuous` bedeutet „Off" = reduced (= Standby). Im **Smart**-Modus entscheidet evcc
Überschuss/Börse → Boost. Damit realisierst du exakt „aus → Standby, Überschuss → Boost".

---

## 4. HA-Monitor (Watchdog + Offline-Erkennung)

**Boost-Watchdog** (Reset, wenn Boost > 1 h ohne Überschuss):

```yaml
alias: Wärmepumpe Boost-Watchdog
trigger:
  - platform: time_pattern
    minutes: "/5"
condition:
  - condition: state
    entity_id: select.heatingpump_sg_ready_state
    state: ["3 - Empfohlen", "4 - Zwang"]
    for: "01:00:00"
  - condition: numeric_state
    entity_id: sensor.evcc_pv       # oder dein PV-Sensor
    below: 200
action:
  - service: select.select_option
    entity_id: select.heatingpump_sg_ready_state
    option: "2 - Normal"
  - service: notify.mobile_app_<phone>
    data:
      title: "WP Boost-Watchdog"
      message: "Boost >1h ohne Überschuss — auf Normal zurückgesetzt"
mode: single
```

**ESP-offline** (LWT `heatingpump/status`):

```yaml
alias: Wärmepumpe ESP offline
trigger:
  - platform: mqtt
    topic: heatingpump/status
    payload: offline
action:
  - service: notify.mobile_app_<phone>
    data:
      title: "Wärmepumpe ESP offline"
      message: "CAN-Interface nicht erreichbar — WP läuft unverändert weiter (letzter Zustand)."
  # optional: Shelly als unabhängiger SG-Ready-Fallback auf „Freigabe" schalten
  # - service: switch.turn_on
  #   entity_id: switch.shelly_boost_relay   # nur wenn verdrahtet!
mode: single
```

---

## 5. Verhalten im Fehlerfall (Übersicht)

| Ausfall | Folge | Sicherung |
|---|---|---|
| evcc abgestürzt | letzter Zustand bleibt | HA-Monitor (Boost→Normal) + FW-Watchdog (Boost, 3 h) |
| ESP weich (Mqtt weg) | FW-Watchdog greift nur bei Boost | nach 3 h Boost → Normal |
| ESP tot (kein Strom) | WP behält letzten Zustand; WPM-Sicherheit läuft | WPM-Zeitprogramm (falls „Normal"=Programmierbetrieb) + HA-Offline-Notify |
| HA tot | evcc + WP laufen weiter | FW-Watchdog (Boost) bleibt aktiv |
| WP-Standby gewollt (1) | bleibt, auch bei evcc-Stille | kein Auto-Rückfall (bewusst) |

---

## 6. Testablauf nach dem Einrichten

1. `make upload`, kurz warten, Status `online`.
2. evcc starten/Log checken: `setmode` → `1/2/3` auf `heatingpump/MANAGER/SG_READY_STATE/set`.
3. Am ESP-Log/WPM-Display: Wechsel Komfort↔Standby/Boost sichtbar.
4. Boost auslösen → `heatingpump/MANAGER/SG_READY_STATE/state` zeigt `3 - Empfohlen`.
5. 1 h noon nichts → HA-Watchdog setzt `2 - Normal` (+ Notification).
6. Alle Writes ohne ESP-Eingriff: Zustand bleibt bis 3 h Boost ohne Befehl → FW-Watchdog greift (Log `SG_READY_WATCHDOG`).