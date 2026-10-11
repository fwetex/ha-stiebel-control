# Ideen & offene Punkte

Gesammelte Ideen aus dem Betrieb der WPL 10 AC (WPM3, CAN B / 50 kbps).

---

## 1. Zeit über MQTT einstellen (Uhr driftet massiv im Betrieb)

**Problem:** Die WPM3-Uhr driftet im Betrieb deutlich, die Kanäle `calculated/date` und
`calculated/time` weichen ab.

**Stand (umgesetzt):**
- MQTT-Topic **`heatingpump/DATETIME/set`** mit Payload **`YYYY-MM-DD HH:MM:SS`** setzt
  Datum **und** Uhrzeit in einem Rutsch (`updateDateTime`). Das Payload wird **strikt
  validiert** — bei ungültigem Format wird gar nichts geschrieben (keine Teil-Writes mehr).
- `updateTime`/`updateDate` (Buttons „Update Uhrzeit"/„Update Datum") validieren ebenfalls
  strikt (`HH:MM:SS` / `YYYY-MM-DD`) und verweigern kaputte HA-Inputs mit klarer Log-Meldung.
- **Test:** `mosquitto_pub -h <broker> -u iotuser -p <pass> -t heatingpump/DATETIME/set -m "2026-10-11 17:30:00"`
- **Idee für Drift-Sync:** HA-Automation, die (z.B. stündlich) die HA-Uhrzeit per
  `heatingpump/DATETIME/set` an die WPM pusht; oder erst die WPM-Zeit lesen, bei
  Abweichung >30 s schreiben.

---

## 2. Betriebsmodus über MQTT umschalten

**Ist-Stand:** Die Betriebsart (PROGRAMMSCHALTER) ist über MQTT schreibbar:
`heatingpump/MANAGER/PROGRAMMSCHALTER/set` mit den WPM3-Namen
(Notbetrieb / Bereitschaftsbetrieb / Programmierbetrieb / Komfortbetrieb / Eco-Modus / Warmwasser).
Discovery-Select existiert bereits in HA.

**Offene Punkte / Idee:**
- Auf der WPL 10 AC prüfen, ob der Write zuverlässig wirkt (Ziel-Member/Sender —
  ggf. Boiler/0x180 statt Manager/0x480 oder Sender-Override nötig).
- Ggf. zusätzliches bequemes Topic `heatingpump/BETRIEBSART/set` analog, das direkt die
  WPM3-Begriffe akzeptiert und den richtigen Schreiber-Pfad (Modell-spezifisch) wählt.
- Für Automatisierungen (z.B. Eco nachts, Komfort bei Anwesenheit, Abschaltung bei Abwesenheit):
  Topic + dokumentierte Payload-Werte bereitstellen.

---

## Notizen / Randnotizen

- Betriebsartnamen WPM3 vs. WPM2 sind modellspezifisch (`HA_MODEL_wpl10ac` → Komfortbetrieb/Eco-Modus).
- Uptime + Reset-Reason + Free-Heap-Diagnose-Sensoren sind aktiv.