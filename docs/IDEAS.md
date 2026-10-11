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

---

## 3. PV-/Strompreis-Steuerung der WP über SG Ready (Sollzustand-Referenz)

**Ziel:** Wärmepumpe bevorzugt mit PV-Überschuss bzw. in günstigen Tarif-/CO₂-Fenstern laufen
lassen, **nie auf Batterie** heizen. Regellogik liegt in **evcc**; die WP wird über den ESP
(CAN) angesteuert.

**Zustandsmodell (evcc → ESP via MQTT, ein Topic):**

| evcc-Zustand | MQTT (`heatingpump/MANAGER/SG_READY_STATE/set`) | Wirkung |
|---|---|---|
| Aus / unter Schwelle | `1 - EVU Sperre` | `PROGRAMMSCHALTER` = **Bereitschaftsbetrieb** (Frostschutz aktiv ✓), Solltwerte zurückgesetzt |
| Ein (Freigabe) | `2 - Normal` | **Komfortbetrieb** + Original-Solltwerte wiederhergestellt |
| Überschuss | `3 - Empfohlen` | Komfort + `SPEICHERSOLLTEMP` (+`BOOST_STATE3`) |
| viel Überschuss | `4 - Zwang` | Komfort + stärkerer WW-/Raum-Boost (`BOOST_STATE4`) |

Boost-Stärke: `heatingpump/MANAGER/SG_READY_BOOST_STATE3/set` (z.B. `5.0`) und `..._STATE4/set`.

**Design-Leitplanken (vom 2026-10-11-Review):**
- „Normal" = **Programmierbetrieb** (WPM-Eigenschema) als Baseline, damit die WP bei
  Komplett-Ausfall des ESP/evcc in ihren konfigurierten Zeitfenstern weiterheizt.
  Fail-Richtung = „Heizung läuft", nie „Heizung tot".
- „Aus" nur für **kurze, bewusste Fenster** (Legionellenschutz >60 °C zyklisch beachten).
- Kein schnelles Aus-/An-Pendeln: evcc `enable`/`disable`-**Delay** ≥10–15 min.
- SG-Ready-Kontakt/ISG-Modus-Einschränkung gilt für den Modbus-Pfad; unser CAN-Pfad
  schreibt die Betriebsart direkt (funktioniert in jedem Modus).

**Failover in Ebenen:**
1. **HA-Monitor (scharfer, kontextualer Watchdog)** — primäre Absicherung, sieht PV/Netz:
   Automation (5-min-Scan): wenn `select.heatingpump_sg_ready_state` in `3/4` seit **≥ 1 h**
   UND PV **< 200 W** → zurück auf `2 - Normal` (+ optional Notification). Zusätzlich
   MQTT-LWT `heatingpump/status` == `offline` → Benachrichtigung, optional Shelly auf Freigabe.
2. **Firmware-Watchdog** (`checkSgReadyWatchdog`, **3 h**, **nur bei Boost 3/4**):
   greift nur, wenn ein Boost länger als 3 h ohne Befehl aktiv ist → zurück auf
   `2 - Normal`. **Standby (1) wird nie automatisch beendet** (gewollter Langzeitzustand).
   Letztes Netz bei gleichzeitigem Ausfall von evcc+HA (Wrap in < 1 Tag begrenzt).
3. **WPM-Zeitprogramm** deckt ESP-Spannungsverlust ab („Normal" idealerweise als
   Programmierbetrieb, sodass die WP in ihren Zeitfenstern weiterheizt).

**evcc-Konfigurationswege:**
- **sgready-relay** (`type: sgready-relay`, boost/dim-Relais): Shelly-gekoppelt, WPM nutzt
  eigenen SG-Ready-Kontakt — kein ESP nötig, evcc-nativ.
- **custom → MQTT → ESP** (relaislos, voller CAN-Umfang): evcc-Custom-Gerät publiziert die
  vier Zustände auf das `/set`-Topic. Dann Watchdog Pflicht.
- **Modbus/ISG** (`template: stiebel-wpm`): nur falls ISG vorhanden; keine Leistungswerte,
  SG-Ready nur im WW+Auto-Modus.