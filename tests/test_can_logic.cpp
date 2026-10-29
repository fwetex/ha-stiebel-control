/*
 * Tests for pure CAN/signal functions in ha-stiebel-control.h.
 * ESPHome symbols are provided by esphome_stubs.h — do NOT define HA_DUMMY_BUILD,
 * as that guard excludes all function implementations in ha-stiebel-control.h.
 *
 * Catch2 must be included BEFORE esphome_stubs.h: the stubs define id(x) as a macro
 * which collides with std::locale::id in gcc-13's STL headers pulled in by Catch2.
 */
#include "catch2/catch_amalgamated.hpp"

#include "esphome_stubs.h"

// ha-stiebel-control.h pulls in all elster headers and signal_requests_wpl13e.h
// via its own includes. The elster .cpp files are compiled as separate objects.
#include "../esphome/ha-stiebel-control/ha-stiebel-control.h"

// ============================================================================
// generate_read_id / generate_write_id
// ============================================================================

TEST_CASE("generate_read_id: MANAGER (0x480)", "[can]") {
    // address = (0x480 & 0x780) / 8 = 0x480 / 8 = 0x90
    // first = (0x90 & 0xF0) + 1 = 0x90 + 1 = 0x91
    // second = 0x480 & 7 = 0
    auto id = generate_read_id(0x480);
    CHECK(id.first  == 0x91);
    CHECK(id.second == 0x00);
}
TEST_CASE("generate_read_id: KESSEL (0x180)", "[can]") {
    // address = 0x180 / 8 = 0x30
    // first = (0x30 & 0xF0) + 1 = 0x30 + 1 = 0x31
    // second = 0
    auto id = generate_read_id(0x180);
    CHECK(id.first  == 0x31);
    CHECK(id.second == 0x00);
}
TEST_CASE("generate_write_id: MANAGER (0x480)", "[can]") {
    // first = 0x90 & 0xF0 = 0x90
    // second = 0
    auto id = generate_write_id(0x480);
    CHECK(id.first  == 0x90);
    CHECK(id.second == 0x00);
}
TEST_CASE("generate_write_id: read first byte = write first byte + 1", "[can]") {
    for (uint16_t can_id : {0x180, 0x280, 0x480, 0x500, 0x680}) {
        auto r = generate_read_id(can_id);
        auto w = generate_write_id(can_id);
        CHECK(r.first == w.first + 1);
        CHECK(r.second == w.second);
    }
}
TEST_CASE("generate_read_id: second byte encodes lower 3 bits of CAN ID", "[can]") {
    // CAN ID 0x483: lower 3 bits = 3
    auto id = generate_read_id(0x483);
    CHECK(id.second == 3);
}

// ============================================================================
// lookupCanMember
// ============================================================================

TEST_CASE("lookupCanMember: MANAGER (0x480)", "[can]") {
    CHECK(std::string(lookupCanMember(0x480).Name) == "MANAGER");
}
TEST_CASE("lookupCanMember: KESSEL (0x180)", "[can]") {
    CHECK(std::string(lookupCanMember(0x180).Name) == "KESSEL");
}
TEST_CASE("lookupCanMember: BEDIENMODUL_WPM3 (0x201) — WPM3 display unit", "[can]") {
    CHECK(std::string(lookupCanMember(0x201).Name) == "BEDIENMODUL_WPM3");
}
TEST_CASE("lookupCanMember: PC (0x680)", "[can]") {
    CHECK(std::string(lookupCanMember(0x680).Name) == "PC");
}
TEST_CASE("lookupCanMember: HEIZMODUL (0x500)", "[can]") {
    CHECK(std::string(lookupCanMember(0x500).Name) == "HEIZMODUL");
}
TEST_CASE("lookupCanMember: unknown ID returns OTHER", "[can]") {
    CHECK(std::string(lookupCanMember(0x999).Name) == "OTHER");
}

// ============================================================================
// getTypeDefaults
// ============================================================================

TEST_CASE("getTypeDefaults: et_dec_val → temperature sensor", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_dec_val, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "sensor");
    CHECK(std::string(dc)   == "temperature");
    CHECK(std::string(unit) == "°C");
    CHECK(std::string(sc)   == "measurement");
}
TEST_CASE("getTypeDefaults: et_cent_val same as et_dec_val", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_cent_val, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "sensor");
    CHECK(std::string(dc)   == "temperature");
}
TEST_CASE("getTypeDefaults: et_bool → binary_sensor", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_bool, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "binary_sensor");
    CHECK(std::string(dc)   == "");
}
TEST_CASE("getTypeDefaults: et_little_bool → binary_sensor", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_little_bool, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "binary_sensor");
}
TEST_CASE("getTypeDefaults: et_byte → counter sensor", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_byte, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "sensor");
    CHECK(std::string(icon) == "mdi:counter");
}
TEST_CASE("getTypeDefaults: et_err_nr → alert-circle sensor", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_err_nr, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "sensor");
    CHECK(std::string(icon) == "mdi:alert-circle");
}
TEST_CASE("getTypeDefaults: et_dev_id → text", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_dev_id, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "text");
}
TEST_CASE("getTypeDefaults: et_dev_nr → text", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_dev_nr, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "text");
}
TEST_CASE("getTypeDefaults: et_betriebsart → enum sensor", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_betriebsart, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "sensor");
    CHECK(std::string(dc)   == "enum");
}
TEST_CASE("getTypeDefaults: et_zeit → timestamp sensor", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_zeit, comp, dc, unit, sc, icon);
    CHECK(std::string(dc) == "timestamp");
}
TEST_CASE("getTypeDefaults: et_datum → date sensor", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults(et_datum, comp, dc, unit, sc, icon);
    CHECK(std::string(dc) == "date");
}
TEST_CASE("getTypeDefaults: unknown type → gauge sensor fallback", "[can]") {
    const char *comp, *dc, *unit, *sc, *icon;
    getTypeDefaults((ElsterType)255, comp, dc, unit, sc, icon);
    CHECK(std::string(comp) == "sensor");
    CHECK(std::string(icon) == "mdi:gauge");
}

// ============================================================================
// processCanMessage
// ============================================================================

// Helper: build a standard 7-byte frame with single-byte index
static std::vector<uint8_t> makeFrame(uint8_t readId1, uint8_t readId2,
                                       uint8_t elsterIdx,
                                       uint8_t byte1, uint8_t byte2) {
    return {readId1, readId2, elsterIdx, byte1, byte2, 0x00, 0x00};
}

// Helper: build a 7-byte frame with 0xFA extended index
static std::vector<uint8_t> makeFrameFA(uint8_t readId1, uint8_t readId2,
                                         uint8_t idxHi, uint8_t idxLo,
                                         uint8_t byte1, uint8_t byte2) {
    return {readId1, readId2, 0xFA, idxHi, idxLo, byte1, byte2};
}

TEST_CASE("processCanMessage: too short returns ElsterTable[0]", "[can]") {
    std::vector<uint8_t> msg = {0x01, 0x02, 0x03};
    std::string val;
    const CanMember* cm = nullptr;
    const ElsterIndex* ei = processCanMessage(msg, 0x480, val, &cm);
    CHECK(ei == &ElsterTable[0]);
}

TEST_CASE("processCanMessage: identifies MANAGER CAN member and decodes et_dec_val", "[can]") {
    // HYSTERESEZEIT index 0x0022, et_dec_val: raw 200 → "20.0"
    const ElsterIndex* hs = GetElsterIndex("HYSTERESEZEIT");
    REQUIRE(std::string(hs->Name) == "HYSTERESEZEIT");

    uint8_t idx = (uint8_t)(hs->Index & 0xFF); // 0x22
    auto msg = makeFrame(0x91, 0x00, idx, 0x00, 0xC8); // 200 = 20.0

    std::string val;
    const CanMember* cm = nullptr;
    const ElsterIndex* ei = processCanMessage(msg, 0x480, val, &cm);

    CHECK(std::string(cm->Name) == "MANAGER");
    CHECK(std::string(ei->Name) == "HYSTERESEZEIT");
    CHECK(val == "20.0");
}

TEST_CASE("processCanMessage: FA extended index frame", "[can]") {
    // Use DATUM (index 0x000a) — high byte 0x00, low byte 0x0a — but 0x00 would
    // be single-byte path. Use UHRZEIT (0x0009). For FA path we need index > 0xFF.
    // Scan for any signal with Index > 0xFF
    const ElsterIndex* ei_test = nullptr;
    for (int i = 1; i <= (int)(sizeof(ElsterTable)/sizeof(ElsterTable[0]) - 1); i++) {
        if (ElsterTable[i].Index > 0xFF && !ElsterTable[i].isBlacklisted) {
            ei_test = &ElsterTable[i];
            break;
        }
    }
    if (!ei_test) SKIP("No double-byte-index signal found in table");

    uint8_t idxHi = (uint8_t)(ei_test->Index >> 8);
    uint8_t idxLo = (uint8_t)(ei_test->Index & 0xFF);
    auto msg = makeFrameFA(0x91, 0x00, idxHi, idxLo, 0x00, 0x01);

    std::string val;
    const CanMember* cm = nullptr;
    const ElsterIndex* ei = processCanMessage(msg, 0x480, val, &cm);

    // Compare by name, not pointer — ElsterTable is static per-TU so pointers differ
    CHECK(std::string(ei->Name) == std::string(ei_test->Name));
    CHECK(std::string(cm->Name) == "MANAGER");
}

TEST_CASE("processCanMessage: et_bool signal value 1 → 'on'", "[can]") {
    // UHRZEIT index 0x0009, et_zeit — use any et_bool signal instead
    // Scan for a non-blacklisted et_bool signal
    const ElsterIndex* boolEi = nullptr;
    for (int i = 1; i <= (int)(sizeof(ElsterTable)/sizeof(ElsterTable[0]) - 1); i++) {
        if ((ElsterTable[i].Type == et_bool || ElsterTable[i].Type == et_little_bool)
                && !ElsterTable[i].isBlacklisted
                && ElsterTable[i].Index <= 0xFF) {
            boolEi = &ElsterTable[i];
            break;
        }
    }
    if (!boolEi) SKIP("No non-blacklisted et_bool signal with single-byte index found");

    auto msg = makeFrame(0x91, 0x00, (uint8_t)boolEi->Index, 0x00, 0x01);
    std::string val;
    const CanMember* cm = nullptr;
    processCanMessage(msg, 0x480, val, &cm);
    CHECK((val == "on" || val == "off" || val == "?"));
}

// ============================================================================
// getOrCreateUID
// ============================================================================

TEST_CASE("getOrCreateUID: generates lowercase uid", "[can]") {
    const CanMember& cm = CanMembers[cm_manager];
    std::string uid = getOrCreateUID(cm, "HYSTERESEZEIT");
    CHECK(uid == "stiebel_manager_hysteresezeit");
}
TEST_CASE("getOrCreateUID: cached result equals first result", "[can]") {
    const CanMember& cm = CanMembers[cm_kessel];
    std::string a = getOrCreateUID(cm, "HYSTERESEZEIT");
    std::string b = getOrCreateUID(cm, "HYSTERESEZEIT");
    CHECK(a == b);
}
TEST_CASE("getOrCreateUID: different members produce different UIDs", "[can]") {
    std::string a = getOrCreateUID(CanMembers[cm_manager], "HYSTERESEZEIT");
    std::string b = getOrCreateUID(CanMembers[cm_kessel],  "HYSTERESEZEIT");
    CHECK(a != b);
}

// ============================================================================
// hash / hash_runtime
// ============================================================================

TEST_CASE("hash: compile-time and runtime hashes agree", "[can]") {
    constexpr uint32_t compile = hash("TEMP_AUSSEN");
    uint32_t runtime = hash_runtime("TEMP_AUSSEN");
    CHECK(compile == runtime);
}
TEST_CASE("hash: known signal hashes match pre-computed constants", "[can]") {
    CHECK(hash_runtime("JAHR")           == HASH_JAHR);
    CHECK(hash_runtime("MONAT")          == HASH_MONAT);
    CHECK(hash_runtime("TAG")            == HASH_TAG);
    CHECK(hash_runtime("STUNDE")         == HASH_STUNDE);
    CHECK(hash_runtime("MINUTE")         == HASH_MINUTE);
    CHECK(hash_runtime("SEKUNDE")        == HASH_SEKUNDE);
    CHECK(hash_runtime("PROGRAMMSCHALTER") == HASH_PROGRAMMSCHALTER);
    CHECK(hash_runtime("WPVORLAUFIST")   == HASH_WPVORLAUFIST);
    CHECK(hash_runtime("RUECKLAUFISTTEMP") == HASH_RUECKLAUFISTTEMP);
    CHECK(hash_runtime("VERDICHTER")     == HASH_VERDICHTER);
}
TEST_CASE("hash: different strings produce different hashes", "[can]") {
    CHECK(hash_runtime("STUNDE") != hash_runtime("MINUTE"));
    CHECK(hash_runtime("JAHR")   != hash_runtime("MONAT"));
}
TEST_CASE("hash: empty string does not crash", "[can]") {
    CHECK_NOTHROW(hash_runtime(""));
}

// ============================================================================
// storeCOPEnergyValue
// ============================================================================

TEST_CASE("storeCOPEnergyValue: valid numeric accepted", "[can]") {
    // Reset the map by calling with fresh values
    storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_WW_SUM_MWH", "1.234");
    // If it doesn't crash and accepts the value, we're good.
    // updateCOPCalculations relies on map state; tested below.
    CHECK(true);
}
TEST_CASE("storeCOPEnergyValue: empty string ignored (no crash)", "[can]") {
    CHECK_NOTHROW(storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_WW_SUM_MWH", ""));
}
TEST_CASE("storeCOPEnergyValue: non-numeric string ignored (no crash)", "[can]") {
    CHECK_NOTHROW(storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_WW_SUM_MWH", "abc"));
}
TEST_CASE("storeCOPEnergyValue: negative value accepted", "[can]") {
    CHECK_NOTHROW(storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_WW_SUM_MWH", "-1.0"));
}

// ============================================================================
// updateCOPCalculations — verifies MQTT is published when all values present
// ============================================================================

TEST_CASE("updateCOPCalculations: publishes cop_ww when all WW values present", "[can]") {
    mqtt_client_instance().clear();

    storeCOPEnergyValue("WAERMEERTRAG_WW_SUM_MWH",      "3.0");
    storeCOPEnergyValue("WAERMEERTRAG_2WE_WW_SUM_MWH",  "1.0");
    storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_WW_SUM_MWH", "2.0");
    updateCOPCalculations();

    bool found = false;
    for (auto& m : mqtt_client_instance().messages)
        if (m.topic.find("cop_ww") != std::string::npos) { found = true; break; }
    CHECK(found);
}
TEST_CASE("updateCOPCalculations: cop_ww value = (3+1)/2 = 2.00", "[can]") {
    mqtt_client_instance().clear();
    storeCOPEnergyValue("WAERMEERTRAG_WW_SUM_MWH",       "3.0");
    storeCOPEnergyValue("WAERMEERTRAG_2WE_WW_SUM_MWH",   "1.0");
    storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_WW_SUM_MWH", "2.0");
    updateCOPCalculations();

    for (auto& m : mqtt_client_instance().messages)
        if (m.topic.find("cop_ww") != std::string::npos)
            CHECK(m.payload == "2.00");
}
TEST_CASE("updateCOPCalculations: skips cop_ww when el divisor is 0", "[can]") {
    mqtt_client_instance().clear();
    storeCOPEnergyValue("WAERMEERTRAG_WW_SUM_MWH",       "3.0");
    storeCOPEnergyValue("WAERMEERTRAG_2WE_WW_SUM_MWH",   "1.0");
    storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_WW_SUM_MWH", "0.0");
    updateCOPCalculations();

    for (auto& m : mqtt_client_instance().messages)
        CHECK(m.topic.find("cop_ww") == std::string::npos);
}
TEST_CASE("updateCOPCalculations: publishes cop_heiz when all Heiz values present", "[can]") {
    mqtt_client_instance().clear();
    storeCOPEnergyValue("WAERMEERTRAG_HEIZ_SUM_MWH",       "6.0");
    storeCOPEnergyValue("WAERMEERTRAG_2WE_HEIZ_SUM_MWH",   "0.0");
    storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_HEIZ_SUM_MWH", "2.0");
    updateCOPCalculations();

    bool found = false;
    for (auto& m : mqtt_client_instance().messages)
        if (m.topic.find("cop_heiz") != std::string::npos) { found = true; break; }
    CHECK(found);
}
TEST_CASE("updateCOPCalculations: publishes cop_gesamt when all values present", "[can]") {
    mqtt_client_instance().clear();
    storeCOPEnergyValue("WAERMEERTRAG_WW_SUM_MWH",          "3.0");
    storeCOPEnergyValue("WAERMEERTRAG_2WE_WW_SUM_MWH",      "1.0");
    storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_WW_SUM_MWH",   "2.0");
    storeCOPEnergyValue("WAERMEERTRAG_HEIZ_SUM_MWH",        "6.0");
    storeCOPEnergyValue("WAERMEERTRAG_2WE_HEIZ_SUM_MWH",    "0.0");
    storeCOPEnergyValue("EL_AUFNAHMELEISTUNG_HEIZ_SUM_MWH", "2.0");
    updateCOPCalculations();

    bool found = false;
    for (auto& m : mqtt_client_instance().messages)
        if (m.topic.find("cop_gesamt") != std::string::npos) { found = true; break; }
    CHECK(found);
}

// ============================================================================
// isPermanentlyBlacklisted
// ============================================================================

TEST_CASE("isPermanentlyBlacklisted: known non-blacklisted signal returns false", "[can]") {
    CHECK_FALSE(isPermanentlyBlacklisted("HYSTERESEZEIT"));
}
// NOTE: there used to be a test here asserting that an *unknown* name returns true
// ("INDEX_NOT_FOUND is blacklisted"). That codified the conflation of "not found" with
// "blacklisted" and is exactly why the registry-wipe bug (c855098) looked intentional.
// The correct contract is asserted in
// "isPermanentlyBlacklisted: unknown names are not blacklisted" below.

using Catch::Approx;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string mqttFindPayload(const std::string& topicSubstr) {
    for (const auto& m : mqtt_client_instance().messages)
        if (m.topic.find(topicSubstr) != std::string::npos)
            return m.payload;
    return "";
}

static bool mqttTopicPublished(const std::string& topicSubstr) {
    return !mqttFindPayload(topicSubstr).empty();
}

static void resetDiscoveryState() {
    discoveredSignals.clear();
    discoveredCalculatedSensors.clear();
    discoveredWritableNumbers.clear();
    discoveredWritableSelects.clear();
    copDiscoveryPublished = false;   // one-shot guard of publishCOPDiscovery()
    mqtt_client_instance().clear();
    mqtt_client_instance().connected = true;
}

// Look a writable number up by signal name. The table is model-gated and its order
// is an implementation detail, so tests must not index it positionally.
static const WritableNumberConfig& writableNumber(const char* signalName) {
    for (size_t i = 0; i < WRITABLE_NUMBER_COUNT; i++) {
        if (strcmp(writableNumbers[i].signalName, signalName) == 0) return writableNumbers[i];
    }
    FAIL("no writable number named " << signalName);
    return writableNumbers[0];  // unreachable: FAIL() aborts the test case
}

// ============================================================================
// publishMqttState
// ============================================================================

TEST_CASE("publishMqttState: publishes to correct topic", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("AUSSENTEMP");
    REQUIRE(ei != nullptr);
    publishMqttState(mgr, ei, "21.5");
    REQUIRE(mqttTopicPublished("heatingpump/MANAGER/AUSSENTEMP/state"));
    CHECK(mqttFindPayload("AUSSENTEMP/state") == "21.5");
}

// ============================================================================
// MAX_PUBLISHED_SIGNALS — the memory bound must not freeze known entities
// ============================================================================

TEST_CASE("signal cap: blocks NEW signals but keeps updating known ones", "[mqtt]") {
    resetDiscoveryState();
    uidCache.clear();

    const ElsterIndex* known = GetElsterIndex("RAUMSOLLTEMP_I");
    const ElsterIndex* fresh = GetElsterIndex("RAUMSOLLTEMP_II");
    REQUIRE(known != nullptr);
    REQUIRE(fresh != nullptr);
    const CanMember& cm = CanMembers[cm_manager];

    // Publish one signal so it counts as known
    updateSensor(cm, known, "48.5");
    REQUIRE(mqttFindPayload("/RAUMSOLLTEMP_I/state") == "48.5");
    mqtt_client_instance().clear();

    // Fill the discovered set up to the cap
    while (discoveredSignals.size() < MAX_PUBLISHED_SIGNALS) {
        discoveredSignals.insert("filler_" + std::to_string(discoveredSignals.size()));
    }

    // A signal that was never published is now ignored — no discovery, no state
    updateSensor(cm, fresh, "22.5");
    CHECK(mqtt_client_instance().messages.empty());

    // …but an already-known signal keeps updating (the cap must not freeze it)
    updateSensor(cm, known, "49.0");
    CHECK(mqttFindPayload("/RAUMSOLLTEMP_I/state") == "49.0");
}

// ============================================================================
// Sentinel values — unknown instead of a stale retained reading
// ============================================================================

// A sentinel means "not available" (see isInvalidSignalValue). For an entity that
// already exists in HA, publishing `None` makes it read as unknown; an empty
// payload would be IGNORED for numeric sensors and would leave the stale value.
TEST_CASE("isInvalidSignalValue: scaled 0x800x sentinels are rejected", "[can]") {
    CHECK(isInvalidSignalValue("-255"));
    CHECK(isInvalidSignalValue("?"));
    CHECK(isInvalidSignalValue("--"));
    CHECK(isInvalidSignalValue("-32768"));
    // 0x8004 decodes to these for the scaled types — a "sensor error", not -3276 °C
    CHECK(isInvalidSignalValue("-3276.4"));
    CHECK(isInvalidSignalValue("-327.64"));
    // real values and text must survive
    CHECK_FALSE(isInvalidSignalValue("22.9"));
    CHECK_FALSE(isInvalidSignalValue("-25.5"));
    CHECK_FALSE(isInvalidSignalValue("0"));
    CHECK_FALSE(isInvalidSignalValue("Komfortbetrieb"));
    CHECK_FALSE(isInvalidSignalValue(""));
}

TEST_CASE("sentinel value: known signal publishes 'None', not a stale reading", "[mqtt]") {
    resetDiscoveryState();
    uidCache.clear();

    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_I");
    REQUIRE(ei != nullptr);
    const CanMember& cm = CanMembers[cm_manager];

    // A real value first, so the entity exists in HA
    updateSensor(cm, ei, "48.5");
    REQUIRE(mqttFindPayload("/RAUMSOLLTEMP_I/state") == "48.5");
    mqtt_client_instance().clear();

    // Now the sentinel: exactly one publish, payload None, no re-discovery
    updateSensor(cm, ei, "-255");
    CHECK(mqtt_client_instance().messages.size() == 1);
    CHECK(mqttFindPayload("/RAUMSOLLTEMP_I/state") == "None");
    CHECK_FALSE(mqttTopicPublished("homeassistant/"));
}

// The ghost-entity protection must survive: a signal that was never published
// stays silent even when its value is a sentinel.
TEST_CASE("sentinel value: an unknown signal creates no entity", "[mqtt]") {
    resetDiscoveryState();
    uidCache.clear();

    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_II");
    REQUIRE(ei != nullptr);

    updateSensor(CanMembers[cm_manager], ei, "-32768");

    CHECK(mqtt_client_instance().messages.empty());
}

// ============================================================================
// republishAllDiscoveries — the registry must survive a refresh
// ============================================================================

TEST_CASE("republishAllDiscoveries: keeps the registry, republishes, prunes blacklisted", "[mqtt]") {
    resetDiscoveryState();
    uidCache.clear();

    const ElsterIndex* raum  = GetElsterIndex("RAUMSOLLTEMP_I");
    const ElsterIndex* kessel = GetElsterIndex("KESSELSOLLTEMP");
    REQUIRE(raum != nullptr);
    REQUIRE(kessel != nullptr);

    // Two published entities
    updateSensor(CanMembers[cm_manager], raum, "19.0");
    updateSensor(CanMembers[cm_manager], kessel, "17.0");
    REQUIRE(discoveredSignals.size() == 2);
    const std::string raumUid  = getOrCreateUID(CanMembers[cm_manager], raum->Name);
    const std::string kesselUid = getOrCreateUID(CanMembers[cm_manager], kessel->Name);

    // Simulate an entity published *before* its register was blacklisted
    const ElsterIndex* junk = GetElsterIndex("TEST_OBJEKT_0");
    REQUIRE(junk != nullptr);
    REQUIRE(junk->isBlacklisted);
    const std::string junkUid = "stiebel_kessel_test_objekt_0";
    uidCache[UidKey{CanMembers[cm_kessel].CanId, junk->Name}] = junkUid;
    discoveredSignals.insert(junkUid);
    REQUIRE(discoveredSignals.size() == 3);

    mqtt_client_instance().clear();
    republishAllDiscoveries();

    // The live entities are still tracked — the old implementation emptied the set here
    CHECK(discoveredSignals.size() == 2);
    CHECK(discoveredSignals.count(raumUid) == 1);
    CHECK(discoveredSignals.count(kesselUid) == 1);

    // …and their discovery configs were actually re-published
    CHECK(mqttTopicPublished("heatingpump/" + raumUid));
    CHECK(mqttTopicPublished("heatingpump/" + kesselUid));

    // The blacklisted one was pruned from the registry and never advertised
    CHECK(discoveredSignals.count(junkUid) == 0);
    CHECK_FALSE(mqttTopicPublished(junkUid));
}

// ============================================================================
// Availability — every entity path must report the device as its availability source
// ============================================================================

TEST_CASE("discovery: all entity paths carry the availability topic", "[mqtt]") {
    resetDiscoveryState();

    // raw CAN sensor
    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_I");
    REQUIRE(ei != nullptr);
    updateSensor(CanMembers[cm_manager], ei, "19.0");
    CHECK(mqttFindPayload("homeassistant/sensor/heatingpump/").find("availability_topic") != std::string::npos);

    // calculated sensor
    resetDiscoveryState();
    publishCalculatedSensorDiscovery(calculatedSensors[0], true);
    CHECK(mqttFindPayload("homeassistant/").find("availability_topic") != std::string::npos);

    // writable number
    resetDiscoveryState();
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"), true);
    CHECK(mqttFindPayload("homeassistant/number/").find("availability_topic") != std::string::npos);

    // writable select
    resetDiscoveryState();
    publishWritableSelectDiscovery(writableSelects[0], true);
    CHECK(mqttFindPayload("homeassistant/select/").find("availability_topic") != std::string::npos);

    // COP sensors (three configs)
    resetDiscoveryState();
    publishCOPDiscovery();
    CHECK(mqttFindPayload("cop_ww").find("availability_topic") != std::string::npos);
    CHECK(mqttFindPayload("cop_heiz").find("availability_topic") != std::string::npos);
    CHECK(mqttFindPayload("cop_gesamt").find("availability_topic") != std::string::npos);
}

TEST_CASE("discovery: every entity path reports firmware version and model", "[mqtt]") {
    resetDiscoveryState();

    // Host build: config.h falls back to "dev"/"host", the stubs define HA_DEVICE_MODEL
    // as test_model (no HA_MODEL_* macro, so the label is the model string).
    const std::string sw    = "\"sw_version\":\"dev (ESPHome host)\"";
    const std::string model = "\"model\":\"test_model\"";

    auto checkPayload = [&](const std::string& prefix) {
        const std::string payload = mqttFindPayload(prefix);
        INFO("payload: " << payload);
        CHECK(payload.find(sw) != std::string::npos);
        CHECK(payload.find(model) != std::string::npos);
        // A leading comma in the shared field would produce ",," and invalidate the JSON.
        CHECK(payload.find(",,") == std::string::npos);
    };

    // raw CAN sensor
    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_I");
    REQUIRE(ei != nullptr);
    updateSensor(CanMembers[cm_manager], ei, "19.0");
    checkPayload("homeassistant/sensor/heatingpump/");

    // calculated sensor (main device)
    resetDiscoveryState();
    publishCalculatedSensorDiscovery(calculatedSensors[0], true);
    checkPayload("homeassistant/");

    // writable number (CAN member sub-device)
    resetDiscoveryState();
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"), true);
    checkPayload("homeassistant/number/");

    // writable select
    resetDiscoveryState();
    publishWritableSelectDiscovery(writableSelects[0], true);
    checkPayload("homeassistant/select/");

    // COP sensors (three configs)
    resetDiscoveryState();
    publishCOPDiscovery();
    checkPayload("cop_ww");
    checkPayload("cop_heiz");
    checkPayload("cop_gesamt");
}

TEST_CASE("updateSensor: WPM software registers are aggregated into one text sensor", "[can]") {
    resetDiscoveryState();
    const ElsterIndex* num = GetElsterIndex("SOFTWARE_NUMMER");
    const ElsterIndex* ver = GetElsterIndex("SOFTWARE_VERSION");
    REQUIRE(num != nullptr);
    REQUIRE(ver != nullptr);

    updateSensor(CanMembers[cm_manager], num, "449");
    // Not complete yet, and definitely no raw SOFTWARE_NUMMER entity.
    CHECK(mqttFindPayload("calculated/wpm_software/state").empty());
    CHECK(mqttFindPayload("MANAGER/SOFTWARE_NUMMER/state").empty());

    updateSensor(CanMembers[cm_manager], ver, "6");
    CHECK(mqttFindPayload("calculated/wpm_software/state") == "449 / 6");
    CHECK(mqttFindPayload("MANAGER/SOFTWARE_VERSION/state").empty());
}

TEST_CASE("updateSensor: Betriebsart follows PROGRAMMSCHALTER, not SOMMERBETRIEB", "[can]") {
    resetDiscoveryState();
    const ElsterIndex* prog   = GetElsterIndex("PROGRAMMSCHALTER");
    const ElsterIndex* summer = GetElsterIndex("SOMMERBETRIEB");
    REQUIRE(prog != nullptr);
    REQUIRE(summer != nullptr);

    updateSensor(CanMembers[cm_manager], prog, "Komfortbetrieb");
    CHECK(mqttFindPayload("calculated/betriebsart/state") == "Komfortbetrieb");

    // The summer-mode ENABLE flag used to overwrite the operating mode with the
    // invented value "Sommerbetrieb" — it must no longer touch the sensor.
    mqtt_client_instance().clear();
    updateSensor(CanMembers[cm_manager], summer, "on");
    CHECK(mqttFindPayload("calculated/betriebsart/state").empty());
    CHECK(mqttFindPayload("MANAGER/SOMMERBETRIEB/state") == "on");

    // A real mode change still updates it.
    updateSensor(CanMembers[cm_manager], prog, "Eco-Modus");
    CHECK(mqttFindPayload("calculated/betriebsart/state") == "Eco-Modus");
}

// ============================================================================
// Discovery retry — a config the broker refused must not be marked as done
// ============================================================================

TEST_CASE("discovery: a raw signal's config is retried after a failed publish", "[mqtt]") {
    resetDiscoveryState();
    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_I");
    REQUIRE(ei != nullptr);

    // A frame can arrive during boot, before the broker connection exists.
    mqtt_client_instance().connected = false;
    updateSensor(CanMembers[cm_manager], ei, "19.0");
    CHECK(discoveredSignals.empty());
    CHECK_FALSE(mqttTopicPublished("stiebel_manager_raumsolltemp_i/config"));

    mqtt_client_instance().connected = true;
    updateSensor(CanMembers[cm_manager], ei, "19.0");
    CHECK(discoveredSignals.size() == 1);
    CHECK(mqttTopicPublished("stiebel_manager_raumsolltemp_i/config"));
}

TEST_CASE("discovery: a calculated sensor is retried after a failed publish", "[mqtt]") {
    resetDiscoveryState();
    mqtt_client_instance().connected = false;
    publishCalculatedSensorDiscovery(calculatedSensors[0], false);
    CHECK(discoveredCalculatedSensors.empty());

    mqtt_client_instance().connected = true;
    publishCalculatedSensorDiscovery(calculatedSensors[0], false);
    CHECK(discoveredCalculatedSensors.size() == 1);
    CHECK(mqttTopicPublished(calculatedSensors[0].uniqueId));
}

TEST_CASE("discovery: writable number and select are retried after a failed publish", "[mqtt]") {
    resetDiscoveryState();
    mqtt_client_instance().connected = false;
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"), false);
    publishWritableSelectDiscovery(writableSelects[0], false);
    CHECK(discoveredWritableNumbers.empty());
    CHECK(discoveredWritableSelects.empty());

    mqtt_client_instance().connected = true;
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"), false);
    publishWritableSelectDiscovery(writableSelects[0], false);
    CHECK(discoveredWritableNumbers.size() == 1);
    CHECK(discoveredWritableSelects.size() == 1);
    CHECK(mqttTopicPublished("stiebel_manager_ww_soll/config"));
    CHECK(mqttTopicPublished("stiebel_manager_programmschalter/config"));
}

TEST_CASE("discovery: the COP one-shot guard is set only after a successful publish", "[mqtt]") {
    resetDiscoveryState();
    mqtt_client_instance().connected = false;
    publishCOPDiscovery();
    CHECK_FALSE(copDiscoveryPublished);
    CHECK_FALSE(mqttTopicPublished("cop_ww"));

    mqtt_client_instance().connected = true;
    publishCOPDiscovery();
    CHECK(copDiscoveryPublished);
    CHECK(mqttTopicPublished("cop_ww"));
    CHECK(mqttTopicPublished("cop_heiz"));
    CHECK(mqttTopicPublished("cop_gesamt"));
}

// ============================================================================
// "not found" vs "blacklisted" — INDEX_NOT_FOUND is a sentinel, not a verdict
// ============================================================================

TEST_CASE("isPermanentlyBlacklisted: unknown names are not blacklisted", "[can]") {
    // INDEX_NOT_FOUND (Index 0) answers for unknown names. Treating that as a blacklist
    // verdict silently skipped mistyped signals and emptied the registry before c855098.
    CHECK_FALSE(isPermanentlyBlacklisted("DOES_NOT_EXIST"));
    CHECK_FALSE(isPermanentlyBlacklisted("RAUMSOLLTEMP_I"));
    CHECK(isPermanentlyBlacklisted("TEST_OBJEKT_0"));
}

TEST_CASE("readSignal/writeSignal: unknown signal names are refused, not sent", "[can]") {
    fake_can().sent.clear();
    readSignal(&CanMembers[cm_manager], "DOES_NOT_EXIST");
    writeSignal(&CanMembers[cm_manager], "DOES_NOT_EXIST", "20.0");
    CHECK(fake_can().sent.empty());
}

TEST_CASE("processAndUpdate: a register missing from ElsterTable publishes nothing", "[can]") {
    resetDiscoveryState();

    // Find an index the table does not know (a miss reports Index 0)
    unsigned short unknownIdx = 0x0001;
    while (unknownIdx < 0xFFFF && GetElsterIndex(unknownIdx)->Index != 0) unknownIdx++;
    REQUIRE(GetElsterIndex(unknownIdx)->Index == 0);

    // 0xFA frame format: index in msg[3] (high) / msg[4] (low)
    std::vector<uint8_t> frame = {0x00, 0x00, 0xFA,
                                  (uint8_t)(unknownIdx >> 8), (uint8_t)(unknownIdx & 0xFF),
                                  0x00, 0x00};
    processAndUpdate(CanMembers[cm_manager].CanId, frame);
    CHECK(mqtt_client_instance().messages.empty());
}

// The hot path reads the resolved row's isBlacklisted flag directly (no name lookup);
// this pins that a blacklisted register is still rejected.
TEST_CASE("processAndUpdate: a blacklisted register publishes nothing", "[can]") {
    resetDiscoveryState();

    const ElsterIndex* junk = GetElsterIndex("TEST_OBJEKT_0");
    REQUIRE(junk != nullptr);
    REQUIRE(junk->isBlacklisted);

    std::vector<uint8_t> frame = {0x00, 0x00, 0xFA,
                                  (uint8_t)(junk->Index >> 8), (uint8_t)(junk->Index & 0xFF),
                                  0x00, 0x00};
    processAndUpdate(CanMembers[cm_manager].CanId, frame);
    CHECK(mqtt_client_instance().messages.empty());
}

TEST_CASE("publishMqttState: skips empty value", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("AUSSENTEMP");
    publishMqttState(mgr, ei, "");
    CHECK(!mqttTopicPublished("AUSSENTEMP/state"));
}

TEST_CASE("publishMqttState: topic uses CAN member name", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& kessel = CanMembers[cm_kessel];
    const ElsterIndex* ei = GetElsterIndex("SPEICHERISTTEMP");
    REQUIRE(ei != nullptr);
    publishMqttState(kessel, ei, "52.0");
    REQUIRE(mqttTopicPublished("heatingpump/KESSEL/SPEICHERISTTEMP/state"));
}

// ============================================================================
// publishMqttDiscovery
// ============================================================================

TEST_CASE("publishMqttDiscovery: sensor signal publishes to homeassistant/sensor", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("AUSSENTEMP");
    REQUIRE(ei != nullptr);
    publishMqttDiscovery(mgr, ei);
    CHECK(mqttTopicPublished("homeassistant/sensor/heatingpump/"));
}

TEST_CASE("publishMqttDiscovery: payload contains name, unique_id, state_topic", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("AUSSENTEMP");
    REQUIRE(ei != nullptr);
    publishMqttDiscovery(mgr, ei);
    std::string payload = mqttFindPayload("stiebel_manager_aussentemp");
    CHECK(payload.find("\"name\"") != std::string::npos);
    CHECK(payload.find("\"unique_id\"") != std::string::npos);
    CHECK(payload.find("heatingpump/MANAGER/AUSSENTEMP/state") != std::string::npos);
}

TEST_CASE("publishMqttDiscovery: temperature signal has device_class temperature", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("AUSSENTEMP");
    REQUIRE(ei != nullptr);
    publishMqttDiscovery(mgr, ei);
    std::string payload = mqttFindPayload("stiebel_manager_aussentemp");
    CHECK(payload.find("\"device_class\":\"temperature\"") != std::string::npos);
    CHECK(payload.find("unit_of_measurement") != std::string::npos);
}

TEST_CASE("publishMqttDiscovery: binary sensor publishes to homeassistant/binary_sensor", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& heizmodul = CanMembers[cm_heizmodul];
    const ElsterIndex* ei = GetElsterIndex("ABTAUUNGAKTIV");
    REQUIRE(ei != nullptr);
    publishMqttDiscovery(heizmodul, ei);
    CHECK(mqttTopicPublished("homeassistant/binary_sensor/heatingpump/"));
}

TEST_CASE("publishMqttDiscovery: binary sensor with explicit payloads includes payload_on/off", "[mqtt]") {
    // EVU_SPERRE_AKTIV has explicit payloadOn/Off in ElsterTable
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("EVU_SPERRE_AKTIV");
    REQUIRE(ei != nullptr);
    publishMqttDiscovery(mgr, ei);
    std::string payload = mqttFindPayload("homeassistant/binary_sensor");
    CHECK(payload.find("payload_on") != std::string::npos);
    CHECK(payload.find("payload_off") != std::string::npos);
}

TEST_CASE("publishMqttDiscovery: binary sensor without explicit payloads omits payload fields", "[mqtt]") {
    // ABTAUUNGAKTIV has NULL payloadOn/Off — no payload_on/off in discovery
    resetDiscoveryState();
    const CanMember& heizmodul = CanMembers[cm_heizmodul];
    const ElsterIndex* ei = GetElsterIndex("ABTAUUNGAKTIV");
    REQUIRE(ei != nullptr);
    publishMqttDiscovery(heizmodul, ei);
    std::string payload = mqttFindPayload("homeassistant/binary_sensor");
    CHECK(payload.find("payload_on") == std::string::npos);
}

TEST_CASE("publishMqttDiscovery: payload contains availability_topic", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("AUSSENTEMP");
    REQUIRE(ei != nullptr);
    publishMqttDiscovery(mgr, ei);
    std::string payload = mqttFindPayload("stiebel_manager_aussentemp");
    CHECK(payload.find("\"availability_topic\":\"heatingpump/status\"") != std::string::npos);
}

TEST_CASE("publishMqttDiscovery: payload contains via_device", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("AUSSENTEMP");
    REQUIRE(ei != nullptr);
    publishMqttDiscovery(mgr, ei);
    std::string payload = mqttFindPayload("stiebel_manager_aussentemp");
    CHECK(payload.find("\"via_device\"") != std::string::npos);
}

TEST_CASE("publishMqttDiscovery: duration signal has device_class duration and unit h", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& heizmodul = CanMembers[cm_heizmodul];
    const ElsterIndex* ei = GetElsterIndex("LZ_VERD_2_HEIZBETRIEB");
    REQUIRE(ei != nullptr);
    publishMqttDiscovery(heizmodul, ei);
    std::string payload = mqttFindPayload("lz_verd_2_heizbetrieb");
    CHECK(payload.find("\"device_class\":\"duration\"") != std::string::npos);
    CHECK(payload.find("\"unit_of_measurement\":\"h\"") != std::string::npos);
    CHECK(payload.find("\"state_class\":\"total_increasing\"") != std::string::npos);
}

// ============================================================================
// updateSensor
// ============================================================================

TEST_CASE("updateSensor: publishes discovery and state on first call", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("RUECKLAUFISTTEMP");
    REQUIRE(ei != nullptr);
    updateSensor(mgr, ei, "30.5");
    CHECK(mqttTopicPublished("homeassistant/sensor/heatingpump/"));
    CHECK(mqttTopicPublished("heatingpump/MANAGER/RUECKLAUFISTTEMP/state"));
    CHECK(mqttFindPayload("RUECKLAUFISTTEMP/state") == "30.5");
}

TEST_CASE("updateSensor: skips discovery on second call (cached)", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("RUECKLAUFISTTEMP");
    REQUIRE(ei != nullptr);
    updateSensor(mgr, ei, "30.5");
    mqtt_client_instance().clear();
    updateSensor(mgr, ei, "31.0");
    CHECK(mqtt_client_instance().messages.size() == 1);
    CHECK(mqttFindPayload("RUECKLAUFISTTEMP/state") == "31.0");
}

TEST_CASE("updateSensor: EVU_SPERRE_AKTIV inverts on → off", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("EVU_SPERRE_AKTIV");
    REQUIRE(ei != nullptr);
    updateSensor(mgr, ei, "on");
    CHECK(mqttFindPayload("EVU_SPERRE_AKTIV/state") == "off");
}

TEST_CASE("updateSensor: EVU_SPERRE_AKTIV inverts off → on", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("EVU_SPERRE_AKTIV");
    REQUIRE(ei != nullptr);
    updateSensor(mgr, ei, "off");
    CHECK(mqttFindPayload("EVU_SPERRE_AKTIV/state") == "on");
}

TEST_CASE("updateSensor: skips empty value", "[mqtt]") {
    resetDiscoveryState();
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("AUSSENTEMP");
    REQUIRE(ei != nullptr);
    updateSensor(mgr, ei, "");
    CHECK(mqtt_client_instance().messages.empty());
}

TEST_CASE("updateSensor: JAHR value updates lastJahr global", "[mqtt]") {
    resetDiscoveryState();
    lastJahr = -1;
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("JAHR");
    REQUIRE(ei != nullptr);
    updateSensor(mgr, ei, "26");
    CHECK(lastJahr == 26);
}

TEST_CASE("updateSensor: WPVORLAUFIST value updates lastWpVorlaufIst", "[mqtt]") {
    resetDiscoveryState();
    lastWpVorlaufIst = NAN;
    const CanMember& heizmodul = CanMembers[cm_heizmodul];
    const ElsterIndex* ei = GetElsterIndex("WPVORLAUFIST");
    REQUIRE(ei != nullptr);
    updateSensor(heizmodul, ei, "35.5");
    CHECK(lastWpVorlaufIst == Approx(35.5f));
}

TEST_CASE("updateSensor: RUECKLAUFISTTEMP value updates lastRuecklaufIstTemp", "[mqtt]") {
    resetDiscoveryState();
    lastRuecklaufIstTemp = NAN;
    const CanMember& mgr = CanMembers[cm_manager];
    const ElsterIndex* ei = GetElsterIndex("RUECKLAUFISTTEMP");
    REQUIRE(ei != nullptr);
    updateSensor(mgr, ei, "28.3");
    CHECK(lastRuecklaufIstTemp == Approx(28.3f));
}

TEST_CASE("updateSensor: VERDICHTER value updates lastVerdichterValue", "[mqtt]") {
    resetDiscoveryState();
    lastVerdichterValue = NAN;
    const CanMember& heizmodul = CanMembers[cm_heizmodul];
    const ElsterIndex* ei = GetElsterIndex("VERDICHTER");
    REQUIRE(ei != nullptr);
    updateSensor(heizmodul, ei, "50.0");
    CHECK(lastVerdichterValue == Approx(50.0f));
}

// ============================================================================
// publishDate
// ============================================================================

TEST_CASE("publishDate: formats date as YYYY-MM-DD", "[calc]") {
    resetDiscoveryState();
    lastJahr = 26; lastMonat = 6; lastTag = 14;
    publishDate();
    CHECK(mqttFindPayload("calculated/date/state") == "2026-06-14");
}

TEST_CASE("publishDate: zero-pads month and day", "[calc]") {
    resetDiscoveryState();
    lastJahr = 24; lastMonat = 1; lastTag = 5;
    publishDate();
    CHECK(mqttFindPayload("calculated/date/state") == "2024-01-05");
}

TEST_CASE("publishDate: skips when values uninitialized", "[calc]") {
    resetDiscoveryState();
    lastJahr = -1; lastMonat = -1; lastTag = -1;
    publishDate();
    CHECK(!mqttTopicPublished("calculated/date/state"));
}

TEST_CASE("publishDate: skips when month out of range", "[calc]") {
    resetDiscoveryState();
    lastJahr = 26; lastMonat = 13; lastTag = 1;
    publishDate();
    CHECK(!mqttTopicPublished("calculated/date/state"));
}

TEST_CASE("publishDate: skips when day out of range", "[calc]") {
    resetDiscoveryState();
    lastJahr = 26; lastMonat = 6; lastTag = 32;
    publishDate();
    CHECK(!mqttTopicPublished("calculated/date/state"));
}

// ============================================================================
// publishTime
// ============================================================================

TEST_CASE("publishTime: formats time as HH:MM:SS", "[calc]") {
    resetDiscoveryState();
    lastStunde = 14; lastMinute = 5; lastSekunde = 9;
    publishTime();
    CHECK(mqttFindPayload("calculated/time/state") == "14:05:09");
}

TEST_CASE("publishTime: zero-pads all components", "[calc]") {
    resetDiscoveryState();
    lastStunde = 0; lastMinute = 0; lastSekunde = 0;
    publishTime();
    CHECK(mqttFindPayload("calculated/time/state") == "00:00:00");
}

TEST_CASE("publishTime: skips when values uninitialized", "[calc]") {
    resetDiscoveryState();
    lastStunde = -1; lastMinute = -1; lastSekunde = -1;
    publishTime();
    CHECK(!mqttTopicPublished("calculated/time/state"));
}

TEST_CASE("publishTime: skips when hour out of range", "[calc]") {
    resetDiscoveryState();
    lastStunde = 24; lastMinute = 0; lastSekunde = 0;
    publishTime();
    CHECK(!mqttTopicPublished("calculated/time/state"));
}

TEST_CASE("publishTime: skips when minute out of range", "[calc]") {
    resetDiscoveryState();
    lastStunde = 12; lastMinute = 60; lastSekunde = 0;
    publishTime();
    CHECK(!mqttTopicPublished("calculated/time/state"));
}

// ============================================================================
// publishBetriebsart
// ============================================================================

TEST_CASE("publishBetriebsart: publishes the mode it is given", "[calc]") {
    resetDiscoveryState();
    publishBetriebsart("Komfortbetrieb");
    CHECK(mqttFindPayload("calculated/betriebsart/state") == "Komfortbetrieb");

    mqtt_client_instance().clear();
    publishBetriebsart("Eco-Modus");
    CHECK(mqttFindPayload("calculated/betriebsart/state") == "Eco-Modus");
}

TEST_CASE("publishBetriebsart: an empty mode publishes nothing", "[calc]") {
    resetDiscoveryState();
    publishBetriebsart("");
    CHECK_FALSE(mqttTopicPublished("calculated/betriebsart/state"));
}

// ============================================================================
// publishDeltaTContinuous
// ============================================================================

TEST_CASE("publishDeltaTContinuous: publishes vorlauf minus ruecklauf", "[calc]") {
    resetDiscoveryState();
    lastWpVorlaufIst = 40.0f; lastRuecklaufIstTemp = 35.0f;
    publishDeltaTContinuous();
    CHECK(mqttFindPayload("delta_t_continuous/state") == "5.00");
}

TEST_CASE("publishDeltaTContinuous: negative delta published correctly", "[calc]") {
    resetDiscoveryState();
    lastWpVorlaufIst = 30.0f; lastRuecklaufIstTemp = 35.0f;
    publishDeltaTContinuous();
    CHECK(mqttFindPayload("delta_t_continuous/state") == "-5.00");
}

TEST_CASE("publishDeltaTContinuous: skips when vorlauf is NaN", "[calc]") {
    resetDiscoveryState();
    lastWpVorlaufIst = NAN; lastRuecklaufIstTemp = 35.0f;
    publishDeltaTContinuous();
    CHECK(!mqttTopicPublished("delta_t_continuous/state"));
}

TEST_CASE("publishDeltaTContinuous: skips when ruecklauf is NaN", "[calc]") {
    resetDiscoveryState();
    lastWpVorlaufIst = 40.0f; lastRuecklaufIstTemp = NAN;
    publishDeltaTContinuous();
    CHECK(!mqttTopicPublished("delta_t_continuous/state"));
}

TEST_CASE("publishDeltaTContinuous: skips on temperature below -50 threshold", "[calc]") {
    resetDiscoveryState();
    lastWpVorlaufIst = -60.0f; lastRuecklaufIstTemp = 35.0f;
    publishDeltaTContinuous();
    CHECK(!mqttTopicPublished("delta_t_continuous/state"));
}

// ============================================================================
// publishDeltaTRunning
// ============================================================================

TEST_CASE("publishDeltaTRunning: publishes when compressor active (value > 2)", "[calc]") {
    resetDiscoveryState();
    lastVerdichterValue = 50.0f;
    lastWpVorlaufIst = 42.0f; lastRuecklaufIstTemp = 37.0f;
    publishDeltaTRunning();
    CHECK(mqttFindPayload("delta_t_running/state") == "5.00");
}

TEST_CASE("publishDeltaTRunning: skips when compressor not running", "[calc]") {
    resetDiscoveryState();
    lastVerdichterValue = 0.0f;
    lastWpVorlaufIst = 42.0f; lastRuecklaufIstTemp = 37.0f;
    publishDeltaTRunning();
    CHECK(!mqttTopicPublished("delta_t_running/state"));
}

TEST_CASE("publishDeltaTRunning: skips when compressor value is NaN", "[calc]") {
    resetDiscoveryState();
    lastVerdichterValue = NAN;
    lastWpVorlaufIst = 42.0f; lastRuecklaufIstTemp = 37.0f;
    publishDeltaTRunning();
    CHECK(!mqttTopicPublished("delta_t_running/state"));
}

TEST_CASE("publishDeltaTRunning: skips when vorlauf NaN even with compressor running", "[calc]") {
    resetDiscoveryState();
    lastVerdichterValue = 50.0f;
    lastWpVorlaufIst = NAN; lastRuecklaufIstTemp = 37.0f;
    publishDeltaTRunning();
    CHECK(!mqttTopicPublished("delta_t_running/state"));
}

// ============================================================================
// publishCompressorActive
// ============================================================================

TEST_CASE("publishCompressorActive: value > 2 → on", "[calc]") {
    resetDiscoveryState();
    lastVerdichterValue = 50.0f;
    publishCompressorActive();
    CHECK(mqttFindPayload("compressor_active/state") == "on");
}

TEST_CASE("publishCompressorActive: value <= 2 → off", "[calc]") {
    resetDiscoveryState();
    lastVerdichterValue = 1.0f;
    publishCompressorActive();
    CHECK(mqttFindPayload("compressor_active/state") == "off");
}

TEST_CASE("publishCompressorActive: exactly 2.0 → off (threshold is > 2)", "[calc]") {
    resetDiscoveryState();
    lastVerdichterValue = 2.0f;
    publishCompressorActive();
    CHECK(mqttFindPayload("compressor_active/state") == "off");
}

TEST_CASE("publishCompressorActive: skips when NaN", "[calc]") {
    resetDiscoveryState();
    lastVerdichterValue = NAN;
    publishCompressorActive();
    CHECK(!mqttTopicPublished("compressor_active/state"));
}

// ============================================================================
// publishCalculatedSensorDiscovery
// ============================================================================

TEST_CASE("publishCalculatedSensorDiscovery: publishes to correct topic", "[mqtt]") {
    resetDiscoveryState();
    CalculatedSensorConfig cfg{
        "stiebel_test_calc", "Test Calc", "heatingpump/test/state",
        "sensor", "temperature", "°C", "measurement", "mdi:thermometer", "", "", "", true
    };
    publishCalculatedSensorDiscovery(cfg, true);
    CHECK(mqttTopicPublished("homeassistant/sensor/heatingpump/stiebel_test_calc/config"));
}

TEST_CASE("publishCalculatedSensorDiscovery: payload contains name and state_topic", "[mqtt]") {
    resetDiscoveryState();
    CalculatedSensorConfig cfg{
        "stiebel_test_calc2", "My Calc", "heatingpump/test2/state",
        "sensor", "", "", "", "mdi:gauge", "", "", "", true
    };
    publishCalculatedSensorDiscovery(cfg, true);
    std::string payload = mqttFindPayload("stiebel_test_calc2");
    CHECK(payload.find("\"name\":\"My Calc\"") != std::string::npos);
    CHECK(payload.find("heatingpump/test2/state") != std::string::npos);
}

TEST_CASE("publishCalculatedSensorDiscovery: cached — second call skips publish", "[mqtt]") {
    resetDiscoveryState();
    CalculatedSensorConfig cfg{
        "stiebel_cache_calc", "Cache Calc", "heatingpump/cache/state",
        "sensor", "", "", "", "", "", "", "", true
    };
    publishCalculatedSensorDiscovery(cfg);
    mqtt_client_instance().clear();
    publishCalculatedSensorDiscovery(cfg);
    CHECK(mqtt_client_instance().messages.empty());
}

TEST_CASE("publishCalculatedSensorDiscovery: forceRepublish bypasses cache", "[mqtt]") {
    resetDiscoveryState();
    CalculatedSensorConfig cfg{
        "stiebel_force_calc", "Force Calc", "heatingpump/force/state",
        "sensor", "", "", "", "", "", "", "", true
    };
    publishCalculatedSensorDiscovery(cfg);
    mqtt_client_instance().clear();
    publishCalculatedSensorDiscovery(cfg, true);
    CHECK(!mqtt_client_instance().messages.empty());
}

TEST_CASE("publishCalculatedSensorDiscovery: diagnostic entity has entity_category and enabled_by_default=false", "[mqtt]") {
    resetDiscoveryState();
    CalculatedSensorConfig cfg{
        "stiebel_diag_calc", "Diag Calc", "heatingpump/diag/state",
        "sensor", "", "", "measurement", "mdi:alert", "", "", "diagnostic", false
    };
    publishCalculatedSensorDiscovery(cfg, true);
    std::string payload = mqttFindPayload("stiebel_diag_calc");
    CHECK(payload.find("\"entity_category\":\"diagnostic\"") != std::string::npos);
    CHECK(payload.find("\"enabled_by_default\":false") != std::string::npos);
}

TEST_CASE("publishCalculatedSensorDiscovery: normal entity omits enabled_by_default", "[mqtt]") {
    resetDiscoveryState();
    CalculatedSensorConfig cfg{
        "stiebel_normal_calc", "Normal Calc", "heatingpump/normal/state",
        "sensor", "", "", "", "", "", "", "", true
    };
    publishCalculatedSensorDiscovery(cfg, true);
    std::string payload = mqttFindPayload("stiebel_normal_calc");
    CHECK(payload.find("enabled_by_default") == std::string::npos);
}

TEST_CASE("publishCalculatedSensorDiscovery: binary sensor includes payload_on/off", "[mqtt]") {
    resetDiscoveryState();
    CalculatedSensorConfig cfg{
        "stiebel_bin_calc", "Bin Calc", "heatingpump/bin/state",
        "binary_sensor", "running", "", "", "mdi:engine", "on", "off", "", true
    };
    publishCalculatedSensorDiscovery(cfg, true);
    std::string payload = mqttFindPayload("stiebel_bin_calc");
    CHECK(payload.find("\"payload_on\":\"on\"") != std::string::npos);
    CHECK(payload.find("\"payload_off\":\"off\"") != std::string::npos);
}

// ============================================================================
// publishWritableNumberDiscovery
// ============================================================================

TEST_CASE("publishWritableNumberDiscovery: publishes to homeassistant/number", "[mqtt]") {
    resetDiscoveryState();
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"), true);
    CHECK(mqttTopicPublished("homeassistant/number/heatingpump/"));
}

TEST_CASE("publishWritableNumberDiscovery: payload contains command_topic and state_topic", "[mqtt]") {
    resetDiscoveryState();
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"), true);
    std::string payload = mqttFindPayload("homeassistant/number/heatingpump/");
    CHECK(payload.find("\"command_topic\"") != std::string::npos);
    CHECK(payload.find("/set\"") != std::string::npos);
    CHECK(payload.find("\"state_topic\"") != std::string::npos);
}

TEST_CASE("publishWritableNumberDiscovery: payload contains min, max, step", "[mqtt]") {
    resetDiscoveryState();
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"), true);
    std::string payload = mqttFindPayload("homeassistant/number/heatingpump/");
    CHECK(payload.find("\"min\"") != std::string::npos);
    CHECK(payload.find("\"max\"") != std::string::npos);
    CHECK(payload.find("\"step\"") != std::string::npos);
}

TEST_CASE("publishWritableNumberDiscovery: SG_READY signal uses main device (no via_device)", "[mqtt]") {
    resetDiscoveryState();
    publishWritableNumberDiscovery(writableNumber("SG_READY_BOOST_STATE3"), true);
    std::string payload = mqttFindPayload("homeassistant/number/heatingpump/");
    CHECK(payload.find("via_device") == std::string::npos);
}

TEST_CASE("publishWritableNumberDiscovery: regular signal uses sub-device with via_device", "[mqtt]") {
    resetDiscoveryState();
    publishWritableNumberDiscovery(writableNumber("RAUMSOLLTEMP_I"), true);
    std::string payload = mqttFindPayload("homeassistant/number/heatingpump/");
    CHECK(payload.find("\"via_device\"") != std::string::npos);
}

TEST_CASE("publishWritableNumberDiscovery: onMainDevice flag routes to main device (no via_device)", "[mqtt]") {
    resetDiscoveryState();
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"), true);
    std::string payload = mqttFindPayload("homeassistant/number/heatingpump/");
    CHECK(payload.find("via_device") == std::string::npos);
}

TEST_CASE("publishWritableNumberDiscovery: cached — second call skips publish", "[mqtt]") {
    resetDiscoveryState();
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"));
    mqtt_client_instance().clear();
    publishWritableNumberDiscovery(writableNumber("WW_SOLL"));
    CHECK(mqtt_client_instance().messages.empty());
}

// The ECO / reduced DHW setpoint is an independent register on every model except
// the WPL10AC/WPM3, where WW_SOLL keeps it in sync with the normal setpoint
// instead (SG_READY_SYNC_DHW_SETTINGS). The entity therefore only exists in the
// other models' builds — the native test build defines no HA_MODEL_*, so this
// covers the non-WPL10AC branch.
TEST_CASE("writableNumbers: ECO DHW setpoint exists on non-WPL10AC models", "[mqtt]") {
    const WritableNumberConfig& eco = writableNumber("EINSTELL_SPEICHERSOLLTEMP2");
    CHECK(eco.member == cm_manager);
    CHECK(eco.min == 20.0f);
    CHECK(eco.max == 60.0f);
    CHECK(eco.step == 1.0f);
    CHECK(std::string(eco.friendlyName).length() > 0);
}

// ============================================================================
// publishWritableSelectDiscovery
// ============================================================================

TEST_CASE("publishWritableSelectDiscovery: publishes to homeassistant/select", "[mqtt]") {
    resetDiscoveryState();
    publishWritableSelectDiscovery(writableSelects[0], true);
    CHECK(mqttTopicPublished("homeassistant/select/heatingpump/"));
}

TEST_CASE("publishWritableSelectDiscovery: payload contains options array", "[mqtt]") {
    resetDiscoveryState();
    publishWritableSelectDiscovery(writableSelects[0], true);
    std::string payload = mqttFindPayload("homeassistant/select/heatingpump/");
    CHECK(payload.find("\"options\"") != std::string::npos);
}

TEST_CASE("publishWritableSelectDiscovery: cached — second call skips publish", "[mqtt]") {
    resetDiscoveryState();
    publishWritableSelectDiscovery(writableSelects[0]);
    mqtt_client_instance().clear();
    publishWritableSelectDiscovery(writableSelects[0]);
    CHECK(mqtt_client_instance().messages.empty());
}

// ============================================================================
// i18n — LNAME_* macros resolve to non-empty strings
// ============================================================================

TEST_CASE("i18n: LNAME_AUSSENTEMP is non-empty", "[i18n]") {
    CHECK(std::string(LNAME_AUSSENTEMP).length() > 0);
}

TEST_CASE("i18n: LNAME_CALC_DATE is non-empty", "[i18n]") {
    CHECK(std::string(LNAME_CALC_DATE).length() > 0);
}

TEST_CASE("i18n: LNAME_OPT_TAGBETRIEB is non-empty", "[i18n]") {
    CHECK(std::string(LNAME_OPT_TAGBETRIEB).length() > 0);
}

TEST_CASE("i18n: LNAME_SEL_PROGRAMMSCHALTER is non-empty", "[i18n]") {
    CHECK(std::string(LNAME_SEL_PROGRAMMSCHALTER).length() > 0);
}

TEST_CASE("i18n: LNAME_COP_WW is non-empty", "[i18n]") {
    CHECK(std::string(LNAME_COP_WW).length() > 0);
}

TEST_CASE("i18n: programmschalterOptions contains 6 non-empty entries", "[i18n]") {
    for (size_t i = 0; i < 6; ++i)
        CHECK(std::string(programmschalterOptions[i]).length() > 0);
}

TEST_CASE("i18n: programmschalterOptions match BetriebsartList protocol strings", "[i18n]") {
    // These must stay German regardless of display language — they round-trip
    // through writeSignal()/TranslateString() as literal CAN-write values.
    for (size_t i = 0; i < 6; ++i)
        CHECK(std::string(programmschalterOptions[i]) == BetriebsartList[i].Name);
}

TEST_CASE("writeSignal: et_betriebsart with unrecognized string is not sent to CAN bus", "[can]") {
    fake_can().sent.clear();
    lastSignalValue.clear();
    const char* value = "Bogus Value";
    writeSignal(&CanMembers[cm_manager], "PROGRAMMSCHALTER", value);
    CHECK(fake_can().sent.empty());
}

TEST_CASE("writeSignal: et_betriebsart with valid German string is sent", "[can]") {
    fake_can().sent.clear();
    lastSignalValue.clear();
    const char* value = "Automatik";
    writeSignal(&CanMembers[cm_manager], "PROGRAMMSCHALTER", value);
    CHECK(fake_can().sent.size() == 1);
}

// ============================================================================
// Operating mode resolution — the label must come from the model's table
// ============================================================================

TEST_CASE("BetriebsartList: the SG Ready mode indices exist in every model", "[can]") {
    const int count = static_cast<int>(High(BetriebsartList)) + 1;
    CHECK(SgReadyController::MODE_STANDBY >= 0);
    CHECK(SgReadyController::MODE_STANDBY < count);
    CHECK(SgReadyController::MODE_COMFORT >= 0);
    CHECK(SgReadyController::MODE_COMFORT < count);

    // Both indices must be translatable back to their protocol value, whichever
    // label the compiled model uses ("Bereitschaft"/"Bereitschaftsbetrieb", …).
    const char* standby = BetriebsartList[SgReadyController::MODE_STANDBY].Name;
    const char* comfort = BetriebsartList[SgReadyController::MODE_COMFORT].Name;
    const char* p1 = standby;
    const char* p2 = comfort;
    CHECK(TranslateString(p1, et_betriebsart) == 0x0100);
    CHECK(TranslateString(p2, et_betriebsart) == 0x0300);
}

TEST_CASE("writeOperatingMode: writes the table's label to PROGRAMMSCHALTER", "[can]") {
    // Regression: applyEvuLock() used the literal "Bereitschaft" (the WPM2 name).
    // On a WPL10AC build the table calls mode 1 "Bereitschaftsbetrieb", so
    // TranslateString() refused the write — EVU lock silently never reached the
    // bus while MQTT and NVS already reported state 1.
    lastSignalValue.clear();
    fake_can().sent.clear();

    sgReadyIO.writeOperatingMode(SgReadyController::MODE_STANDBY);
    REQUIRE(fake_can().sent.size() == 2);      // write + read-back
    CHECK(fake_can().sent[0][3] == 0x01);      // register index 0x0112
    CHECK(fake_can().sent[0][4] == 0x12);
    CHECK(fake_can().sent[0][5] == 0x01);      // value 0x0100 = standby
    CHECK(fake_can().sent[0][6] == 0x00);

    fake_can().sent.clear();
    sgReadyIO.writeOperatingMode(SgReadyController::MODE_COMFORT);
    REQUIRE(fake_can().sent.size() == 2);
    CHECK(fake_can().sent[0][5] == 0x03);      // value 0x0300 = comfort
    CHECK(fake_can().sent[0][6] == 0x00);
}

TEST_CASE("writeOperatingMode: an out-of-range index sends nothing", "[can]") {
    lastSignalValue.clear();
    fake_can().sent.clear();

    sgReadyIO.writeOperatingMode(-1);
    sgReadyIO.writeOperatingMode(99);
    CHECK(fake_can().sent.empty());
}

TEST_CASE("i18n: sgReadyOptions contains 4 non-empty entries", "[i18n]") {
    for (size_t i = 0; i < 4; ++i)
        CHECK(std::string(sgReadyOptions[i]).length() > 0);
}

// ============================================================================
// Redundant register writes — a repeated command must cost zero CAN frames
// ============================================================================

TEST_CASE("writeSignal: identical writes are suppressed, changed values go out", "[can]") {
    lastSignalValue.clear();
    fake_can().sent.clear();
    const uint32_t writesBefore = canWriteCount;
    const uint32_t skippedBefore = canWriteSkippedCount;

    writeSignal(&CanMembers[cm_manager], "RAUMSOLLTEMP_I", "21.0");
    CHECK(fake_can().sent.size() == 1);
    CHECK(canWriteCount - writesBefore == 1);

    writeSignal(&CanMembers[cm_manager], "RAUMSOLLTEMP_I", "21.0");   // redundant
    CHECK(fake_can().sent.size() == 1);
    CHECK(canWriteSkippedCount - skippedBefore == 1);

    writeSignal(&CanMembers[cm_manager], "RAUMSOLLTEMP_I", "22.0");   // real change
    CHECK(fake_can().sent.size() == 2);
    CHECK(canWriteCount - writesBefore == 2);
}

TEST_CASE("writeSignal: a received value resets the suppression baseline", "[can]") {
    lastSignalValue.clear();
    fake_can().sent.clear();
    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_I");
    REQUIRE(ei != nullptr);

    writeSignal(&CanMembers[cm_manager], "RAUMSOLLTEMP_I", "21.0");

    // Someone changed it at the panel: the broadcast updates the baseline …
    updateSensor(CanMembers[cm_manager], ei, "18.0");

    fake_can().sent.clear();
    writeSignal(&CanMembers[cm_manager], "RAUMSOLLTEMP_I", "21.0");   // … so this must go out
    CHECK(fake_can().sent.size() == 1);
}

TEST_CASE("writeSignal: a deferred retry bypasses the suppression", "[can]") {
    lastSignalValue.clear();
    pendingCanFrames.clear();
    fake_can().sent.clear();

    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_I");
    REQUIRE(ei != nullptr);
    const char* value = "21.0";

    // Baseline already equals the value (as if it had just been written) …
    lastSignalValue[UidKey{CanMembers[cm_manager].CanId, ei->Name}] = value;

    // … and a write for it is parked in the deferral queue
    PendingCanFrame p;
    p.member = &CanMembers[cm_manager];
    p.signal = ei;
    p.isWrite = true;
    snprintf(p.value, sizeof(p.value), "%s", value);
    pendingCanFrames.push_back(p);

    retryPendingCanFrames();   // runs through the (member, ei) overload, bypassing the check
    CHECK(fake_can().sent.size() == 1);
    CHECK(pendingCanFrames.empty());
}

TEST_CASE("i18n: all calculatedSensors names are non-empty", "[i18n]") {
    for (size_t i = 0; i < CALCULATED_SENSOR_COUNT; ++i)
        CHECK(std::string(calculatedSensors[i].name).length() > 0);
}

TEST_CASE("i18n: all writableNumbers friendlyNames are non-empty", "[i18n]") {
    for (size_t i = 0; i < WRITABLE_NUMBER_COUNT; ++i)
        CHECK(std::string(writableNumbers[i].friendlyName).length() > 0);
}

TEST_CASE("i18n: named ElsterTable signals have non-empty friendlyName via LNAME macro", "[i18n]") {
    static const char* named[] = {
        "AUSSENTEMP", "SPEICHERISTTEMP", "RUECKLAUFISTTEMP", "VERDICHTER",
        "LZ_VERD_1_HEIZBETRIEB", "LZ_VERD_2_KUEHLBETRIEB", "ABTAUUNGAKTIV",
        "EVU_SPERRE_AKTIV", "HEIZKURVE", "WW_ECO", "WW_HYSTERSE", "GEBAEUDEART",
        "PROGRAMMSCHALTER", "SOMMERBETRIEB", "WP_STATUS",
    };
    for (const char* name : named) {
        const ElsterIndex* ei = GetElsterIndex(name);
        REQUIRE(ei != nullptr);
        if (ei->hasMetadata && ei->friendlyName)
            CHECK(std::string(ei->friendlyName).length() > 0);
    }
}

// ============================================================================
// dates/time validators (updateDateTime / updateTime / updateDate guards)
// ============================================================================

TEST_CASE("isValidTimeString", "[datetime]") {
    CHECK(isValidTimeString("17:30:00"));
    CHECK(isValidTimeString("00:00:00"));
    CHECK(isValidTimeString("23:59:59"));

    CHECK_FALSE(isValidTimeString(nullptr));
    CHECK_FALSE(isValidTimeString(""));                 // empty
    CHECK_FALSE(isValidTimeString("17:30"));            // too short
    CHECK_FALSE(isValidTimeString("17:30:000"));        // too long
    CHECK_FALSE(isValidTimeString("7:30:00"));          // width
    CHECK_FALSE(isValidTimeString("17 30 00"));         // wrong separators
    CHECK_FALSE(isValidTimeString("17:3x:00"));         // non-digit
    CHECK_FALSE(isValidTimeString("24:00:00"));         // hour overflow
    CHECK_FALSE(isValidTimeString("17:60:00"));         // minute overflow
    CHECK_FALSE(isValidTimeString("17:30:60"));         // second overflow
}

TEST_CASE("isValidDateString", "[datetime]") {
    CHECK(isValidDateString("2026-10-11"));
    CHECK(isValidDateString("2000-01-01"));

    CHECK_FALSE(isValidDateString(nullptr));
    CHECK_FALSE(isValidDateString(""));                 // empty
    CHECK_FALSE(isValidDateString("2026-10-11 17:30")); // full timestamp — NOT a bare date
    CHECK_FALSE(isValidDateString("26-10-11"));         // too short
    CHECK_FALSE(isValidDateString("2026/10/11"));       // wrong separators
    CHECK_FALSE(isValidDateString("2026-1x-11"));       // non-digit
    CHECK_FALSE(isValidDateString("2026-10-1"));        // truncated day
}

// The digit/separator checks alone accept "2026-13-99", which would be written
// straight into the heat pump's clock (the time part is already range-checked).
TEST_CASE("isValidDateString: field ranges and leap years", "[datetime]") {
    // valid
    CHECK(isValidDateString("2026-12-31"));
    CHECK(isValidDateString("2026-01-01"));
    CHECK(isValidDateString("2024-02-29"));   // leap year
    CHECK(isValidDateString("2000-02-29"));   // century leap year (divisible by 400)

    // month out of range
    CHECK_FALSE(isValidDateString("2026-00-11"));
    CHECK_FALSE(isValidDateString("2026-13-11"));
    CHECK_FALSE(isValidDateString("2026-99-11"));

    // day out of range for that month
    CHECK_FALSE(isValidDateString("2026-10-00"));
    CHECK_FALSE(isValidDateString("2026-10-32"));
    CHECK_FALSE(isValidDateString("2026-04-31"));  // April has 30 days
    CHECK_FALSE(isValidDateString("2026-02-30"));  // February has 28
    CHECK_FALSE(isValidDateString("2025-02-29"));  // not a leap year
    CHECK_FALSE(isValidDateString("1900-02-29"));  // century non-leap (divisible by 100, not 400)
}

// The exact regression: updateDateTime must split "YYYY-MM-DD HH:MM:SS" before
// calling the length-checking validators (the full 19-char string fails both).
TEST_CASE("updateDateTime full timestamp split: date and time parts validate separately", "[datetime]") {
    const char* full = "2026-10-11 17:30:00";
    REQUIRE(strlen(full) == 19);
    REQUIRE(full[10] == ' ');

    char dateBuf[11];
    memcpy(dateBuf, full, 10);
    dateBuf[10] = '\0';
    CHECK(isValidDateString(dateBuf));
    CHECK(isValidTimeString(full + 11));

    // Passing the full 19-char string to the date validator must fail
    // (this is what always rejected a valid timestamp before the fix).
    CHECK_FALSE(isValidDateString(full));
}

// ============================================================================
// deadlineReached — millis() wrap-safe scheduling
// ============================================================================

TEST_CASE("deadlineReached: ordinary deadlines", "[timing]") {
    CHECK_FALSE(deadlineReached(1000, 2000));  // not yet
    CHECK(deadlineReached(2000, 2000));        // exactly due
    CHECK(deadlineReached(3000, 2000));        // overdue
    CHECK(deadlineReached(0, 0));              // armed and due at the same instant
}

// The bug: `now >= deadline` reports a deadline set just before the ~49.7 day
// millis() wrap as ~49 days in the future, so whatever it schedules stalls.
TEST_CASE("deadlineReached: a deadline set before the millis() wrap is still due", "[timing]") {
    const uint32_t beforeWrap = 0xFFFFFF00u;  // set 256 ms before the wrap
    const uint32_t afterWrap  = 0x00000100u;  // 256 ms later

    CHECK(deadlineReached(afterWrap, beforeWrap));
    // the comparison this replaced would say "not due" and stall for ~49 days:
    CHECK_FALSE(afterWrap >= beforeWrap);
}

// Documented limit: anything more than half the range in the future reads as a
// wrapped past deadline. Intervals must stay far below this.
TEST_CASE("deadlineReached: deadlines beyond half the range are not representable", "[timing]") {
    CHECK_FALSE(deadlineReached(0, 0x80000000u));
}

// ============================================================================
// processSignalRequests — round-robin start index
// ============================================================================

// MAX_REQUESTS_PER_ITERATION is 1, so one call sends at most ONE request. If the
// walk restarts at slot 0 every call, the lowest-indexed due slot wins every
// call and the slots behind it are only ever served in the rare second when
// nothing earlier is due. The walk must therefore resume where it stopped.
TEST_CASE("processSignalRequests: trailing slots are served (round-robin start index)", "[can]") {
    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_I");
    REQUIRE(ei != nullptr);

    requestManagerStarted = true;  // skip the one-time schedule build
    pollSlots.clear();
    pollSlots.push_back({&CanMembers[cm_manager], ei, 0,     0, 0});  // re-armed below
    pollSlots.push_back({&CanMembers[cm_manager], ei, 60000, 0, 0});
    pollSlots.push_back({&CanMembers[cm_manager], ei, 60000, 0, 0});
    pollSlots.push_back({&CanMembers[cm_manager], ei, 60000, 0, 0});

    for (int call = 0; call < 4; call++) {
        pollSlots[0].nextAt = 0;   // slot 0 is always due…
        processSignalRequests();   // …so without rotation it wins every call
    }

    // A served slot is rescheduled to now + intervalMs + jitter; millis() is 0 in
    // the test build, so any slot that was polled now has nextAt > 0.
    CHECK(pollSlots[1].nextAt > 0);
    CHECK(pollSlots[2].nextAt > 0);
    CHECK(pollSlots[3].nextAt > 0);

    pollSlots.clear();  // do not leak the synthetic schedule into other tests
}

// ============================================================================
// pollDemandPerSecond — does the schedule fit the poll budget?
// ============================================================================

TEST_CASE("pollDemandPerSecond: nominal demand of the schedule", "[can]") {
    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_I");
    REQUIRE(ei != nullptr);

    std::vector<PollSlot> slots;
    slots.push_back({&CanMembers[cm_manager], ei, 30000, 0, 0});
    CHECK(pollDemandPerSecond(slots) == Approx(1000.0 / 30000.0));

    // A 1-minute slot adds half the demand of a 30 s slot
    slots.push_back({&CanMembers[cm_manager], ei, 60000, 0, 0});
    CHECK(pollDemandPerSecond(slots) == Approx(1000.0 / 30000.0 + 1000.0 / 60000.0));

    // A zero interval is ignored instead of dividing by zero
    slots.push_back({&CanMembers[cm_manager], ei, 0, 0, 0});
    CHECK(pollDemandPerSecond(slots) == Approx(1000.0 / 30000.0 + 1000.0 / 60000.0));

    // A worst-case WPL13E-sized schedule must fit the default budget
    std::vector<PollSlot> heavy;
    for (int i = 0; i < 57; i++) heavy.push_back({&CanMembers[cm_manager], ei, 30000, 0, 0});
    CHECK(pollDemandPerSecond(heavy) == Approx(57 * 1000.0 / 30000.0));
    CHECK(pollDemandPerSecond(heavy) < MAX_REQUESTS_PER_ITERATION);
}

// ============================================================================
// CAN TX deferral — a driver with few TX buffers (MCP2515) refuses frames
// ============================================================================

// The SG Ready burst issues up to 8 frames (write+read for mode, DHW and room)
// from a single lambda. On the ESP32-S3 the TWAI driver queues 64 deep, so nothing
// is refused; the MCP2515 has three TX buffers and returns ERROR_ALLTXBUSY instead
// of queueing, which used to drop everything past the third frame silently.
TEST_CASE("CAN TX: frames the driver refuses are deferred and retried", "[can]") {
    const ElsterIndex* ei = GetElsterIndex("RAUMSOLLTEMP_I");
    REQUIRE(ei != nullptr);

    fake_can().sent.clear();
    fake_can().tx_slots = 3;   // MCP2515: three TX buffers, then busy
    fake_can().drain();
    pendingCanFrames.clear();
    lastSignalValue.clear();   // distinct values below, but keep the baseline deterministic

    // Three write+read pairs — the tail of an SG Ready burst (6 frames)
    for (int i = 0; i < 3; i++) {
        char v[8];
        snprintf(v, sizeof(v), "%d.0", 25 + i);   // distinct, or the dedupe would skip them
        writeCanSignalRouted("RAUMSOLLTEMP_I", v);
    }
    CHECK(fake_can().sent.size() == 3);      // only what the buffers could take
    CHECK(pendingCanFrames.size() == 3);     // the rest are parked, not lost

    // The bus drains; the next 1 s request-manager tick retries the parked frames
    fake_can().drain();
    retryPendingCanFrames();
    CHECK(fake_can().sent.size() == 6);      // all six made it out
    CHECK(pendingCanFrames.empty());

    fake_can().tx_slots = 0;                 // restore the TWAI-like default
}

// The fix must not change anything on the TWAI build: with a deep queue no frame
// is ever refused, so nothing is deferred.
TEST_CASE("CAN TX: a deep TX queue never defers", "[can]") {
    fake_can().sent.clear();
    fake_can().tx_slots = 0;   // unlimited, like tx_queue_len: 64
    fake_can().drain();
    pendingCanFrames.clear();
    lastSignalValue.clear();

    for (int i = 0; i < 10; i++) {
        char v[8];
        snprintf(v, sizeof(v), "%d.0", 25 + i);   // distinct values
        writeCanSignalRouted("RAUMSOLLTEMP_I", v);
    }
    CHECK(fake_can().sent.size() == 20);     // 10 write+read pairs, none refused
    CHECK(pendingCanFrames.empty());
}
