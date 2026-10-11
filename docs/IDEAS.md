# Ideen & offene Punkte

Gesammelte Ideen aus dem Betrieb der WPL 10 AC (WPM3, CAN B / 50 kbps).

---

## 1. Zeit über MQTT einstellen (Uhr driftet massiv im Betrieb)

**Problem:** Die WPM3-Uhr driftet im Betrieb deutlich, die Kanäle `calculated/date` und
`calculated/time` weichen ab.

**Stand — VERIFIZIERT FUNKTIONSFÄHIG:**
- MQTT-Topic **`heatingpump/DATETIME/set`** mit Payload **`YYYY-MM-DD HH:MM:SS`** setzt
  Datum **und** Uhrzeit in einem Rutsch (`updateDateTime`). Strikt validiert (Trim + Split,
  keine Teil-Writes), Hex-Diagnose bei ungültigem Payload.
- **Button „WPM Datum & Uhrzeit übernehmen"**: liest die Systemzeit aus der ESPHome-`time:`
  Komponente (`platform: homeassistant`) und ruft `updateDateTime` — kein HA-Text-Sensor nötig.
- Writes werden von der WPM3 auf CAN-B (Sender 0x680) **angenommen** (am Display bestätigt).
- **Drift-Sync (optional):** HA-Automation, die (z.B. stündlich/Viertelstündlich) die
  HA-Uhrzeit per `heatingpump/DATETIME/set` pusht:
  ```yaml
  alias: WPM Uhrzeit synchronisieren
  trigger:
    - platform: time_pattern
      minutes: "/15"
  action:
    - service: mqtt.publish
      data:
        topic: heatingpump/DATETIME/set
        payload: "{{ now().strftime('%Y-%m-%d %H:%M:%S') }}"
  mode: single
  ```

---

## 2. Betriebsmodus über MQTT umschalten

**Stand — VERIFIZIERT FUNKTIONSFÄHIG:**
- HA-Select **`select.heatingpump_programmschalter`** („Programmschalter"), Via MQTT-Discovery.
  Optionen (WPM3): Notbetrieb / Bereitschaftsbetrieb / Programmierbetrieb / Komfortbetrieb /
  Eco-Modus / Warmwasser. Write über `heatingpump/MANAGER/PROGRAMMSCHALTER/set` → WPM nimmt
  ihn an (Display bestätigt).
- Modus-Namen sind exakt die `BetriebsartList`-Namen der WPM3 (`HA_MODEL_wpl10ac`).
- Für Automatisierungen (z.B. Eco nachts, Komfort bei Anwesenheit): einfach den Select
  per Dienst `select.select_option` schalten — oder direkt
  `mqtt.publish` auf `heatingpump/MANAGER/PROGRAMMSCHALTER/set` mit dem Modus-Namen.

---

## Offene Punkte / Bekanntes

- **`interval took a long time` (bis ~170 ms):** TWAI-Tx-Blocking auf dem EMI-anfälligen Bus
  (Bus-Errors). Bisher unregelmäßig/vereinzelt; falls es häufig wird: `send_data` non-blocking
  machen oder `MAX_REQUESTS_PER_ITERATION` drosseln.
- `calculated/time`-Feld: Die WPM3 liefert kein SEKUNDE (bereits optional behandelt);
  STUNDE-Readback kommt (bestätigt). `not initialized`-Warnung evtl. nur nach Boot.