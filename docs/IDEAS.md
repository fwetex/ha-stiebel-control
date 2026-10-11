# Ideen & offene Punkte

Gesammelte Ideen aus dem Betrieb der WPL 10 AC (WPM3, CAN B / 50 kbps).

---

## 1. Zeit über MQTT einstellen (Uhr driftet massiv im Betrieb)

**Problem:** Die WPM3-Uhr driftet im Betrieb deutlich, die Kanäle `calculated/date` und
`calculated/time` weichen ab.

**Ist-Stand:** Datum/Uhrzeit werden manuell über die HA-Buttons „Update Datum"/„Update Uhrzeit"
gesetzt (Text-Sensoren an `input_datetime.*` → `updateDate()`/`updateTime()` →
`writeSignal(cm_manager, JAHR/MONAT/TAG/STUNDE/MINUTE/SEKUNDE)`).

**Idee:**
- MQTT-Topic zum Setzen der Uhrzeit, z.B. `heatingpump/MANAGER/DATETIME/set`
  (Payload `YYYY-MM-DD HH:MM:SS`) → im `on_message`-Handler parsen und `updateDate`/`updateTime`
  aufrufen (Pfad existiert bereits).
- Damit kann HA (oder ein NTP-Bridge) die Zeit **zyklisch synchronisieren**
  (z.B. jeden Tag/bei Drift-Abweichung > 30 s).
- Optional: Zeit zuerst vom WPM3 **lesen** und mit der HA-Zeit vergleichen
  (Drift erkennen) und nur bei Abweichung schreiben.

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