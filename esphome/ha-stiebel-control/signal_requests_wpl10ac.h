/*
 * Signal Request Table — WPL10 AC Heat Pump
 *
 * IMPORTANT — bus and bitrate:
 *   On the WPL 10 AC + WPM3 the full data set (DHW/boiler temperature, setpoints)
 *   is only available on the WPM3's CAN B / display connection at 50 kBit/s. On
 *   CAN A (20 kBit/s) the Boiler does not answer read requests. Set can_bit_rate
 *   to 50kbps and tap the CAN B / display connector.
 *
 * NOTE — sender:
 *   The Boiler (0x180) originally only answered DHW/setpoint reads sent as
 *   FES_COMFORT (0x100), but impersonating the WPM3 display destabilised the bus
 *   (bus-off after prolonged operation). The 0x100 sender override was removed and
 *   all reads use the normal ComfortSoft sender (0x680), which the Boiler answers
 *   on the CAN B / display connection.
 *
 * Member layout (verified via capture on CAN B / display):
 *   - KESSEL (0x180):  outside temp, DHW actual/setpoint, room setpoints
 *   - MANAGER (0x480): clock, operating mode, EVU lock
 *   - HEIZMODUL (0x500): compressor/defrost state, energy counters
 *   - RAUMFERNFUEHLER_2 (0x401): room actual temperature (0x4ec7), humidity (0x4ec8)
 *
 * See signal_requests_base.h for the base macro definition.
 */

#ifndef SIGNAL_REQUESTS_WPL10AC_H
#define SIGNAL_REQUESTS_WPL10AC_H

#include "config.h"
#include "signal_requests_base.h"

extern const SignalRequest signalRequests[] = {
    SIGNAL_REQUESTS_BASE   // date/time, EVU lock, operating mode, energy counters

    // ========================================================================
    // BOILER / KESSEL (0x180)
    // ========================================================================
    {"AUSSENTEMP",                 FREQ_30S,   cm_kessel},
    {"RUECKLAUFISTTEMP",           FREQ_30S,   cm_kessel},
    {"HILFSKESSELSOLL",            FREQ_30S,   cm_kessel},
    {"KESSELSOLLTEMP",             FREQ_30S,   cm_kessel},
    {"SPEICHERISTTEMP",            FREQ_30S,   cm_kessel},   // DHW actual temperature
    {"SPEICHERSOLLTEMP",           FREQ_10MIN, cm_kessel},
    {"EINSTELL_SPEICHERSOLLTEMP",  FREQ_10MIN, cm_kessel},
    {"EINSTELL_SPEICHERSOLLTEMP2", FREQ_10MIN, cm_kessel},

    // ========================================================================
    // HEATING MODULE (0x500) — compressor/defrost state, verified
    // ========================================================================
    {"VERDAMPFERTEMP",             FREQ_30S,   cm_heizmodul},
    {"ABTAUUNGAKTIV",              FREQ_1MIN,  cm_heizmodul},

    // ========================================================================
    // ROOM SETPOINTS — single member (MANAGER) to avoid duplicate/-255 entities
    // ========================================================================
    {"RAUMSOLLTEMP_I",             FREQ_30S,   cm_manager},
    {"RAUMSOLLTEMP_NACHT",         FREQ_30S,   cm_manager},

    // ========================================================================
    // ENERGY COUNTERS (HEIZMODUL 0x500) — verified
    // ========================================================================
    {"EL_AUFNAHMELEISTUNG_HEIZ_SUM_KWH", FREQ_10MIN, cm_heizmodul},
    {"EL_AUFNAHMELEISTUNG_HEIZ_TAG_WH",  FREQ_10MIN, cm_heizmodul},
    {"EL_AUFNAHMELEISTUNG_HEIZ_TAG_KWH", FREQ_10MIN, cm_heizmodul},
    {"EL_AUFNAHMELEISTUNG_WW_SUM_KWH",   FREQ_10MIN, cm_heizmodul},
    {"EL_AUFNAHMELEISTUNG_WW_TAG_WH",    FREQ_10MIN, cm_heizmodul},
    {"EL_AUFNAHMELEISTUNG_WW_TAG_KWH",   FREQ_10MIN, cm_heizmodul},
    {"WAERMEERTRAG_HEIZ_SUM_KWH",        FREQ_10MIN, cm_heizmodul},
    {"WAERMEERTRAG_HEIZ_TAG_WH",         FREQ_10MIN, cm_heizmodul},
    {"WAERMEERTRAG_HEIZ_TAG_KWH",        FREQ_10MIN, cm_heizmodul},
    {"WAERMEERTRAG_WW_SUM_KWH",          FREQ_10MIN, cm_heizmodul},
    {"WAERMEERTRAG_WW_TAG_WH",           FREQ_10MIN, cm_heizmodul},
    {"WAERMEERTRAG_WW_TAG_KWH",          FREQ_10MIN, cm_heizmodul},

    // ========================================================================
    // DIAGNOSTICS — formerly blacklisted registers, verified readable on the
    // WPM3 on 2026-10-25 (see the note in ElsterTable.h).
    // ========================================================================
    {"SAMMEL_RELAISSTATUS",     FREQ_30S,   cm_manager},
    {"FATAL_ERROR",             FREQ_1MIN,  cm_manager},
    // Aggregated into the calculated "WPM-Software" text sensor (no raw entities).
    // Static values: read once shortly after boot (see the immediate-poll list), then hourly.
    {"SOFTWARE_NUMMER",         FREQ_60MIN, cm_manager},
    {"SOFTWARE_VERSION",        FREQ_60MIN, cm_manager},

    // ========================================================================
    // FLOW / DELTA-T
    // Evaluated on 2026-10-29 with a diagnostic poll of all plausible registers
    // (DURCHFLUSS_CH/WW, VOLUMENSTROM ×2 members, VOLUMENSTROM_PUMPE_HZK,
    // WAERMEMENGE, IMPULSRATE, WMZ_MESSUNG_IMPULS, VOLUMENSTROMMESSUNG,
    // WASSERDRUCK ×3 members, VORLAUFISTTEMP ×3 members, WPVORLAUFIST ×3 members).
    // Only these two answered; everything else is 0x8000 ("not available") or an
    // error code (VORLAUFISTTEMP on KESSEL → 0x8004). Do not re-add the rest
    // without evidence — the WPM3 does not expose pressure or a second flow.
    // ========================================================================
    // The only volume-flow register the KESSEL answers. Value is 0 while the
    // compressor is off; scale/unit still to be determined from Q = 1.163·V̇·ΔT.
    {"VOLUMENSTROM",            FREQ_30S,   cm_kessel},
    // Heat-pump flow temperature (the only member that answers); with
    // RUECKLAUFISTTEMP it feeds the delta_t_continuous/running sensors.
    {"WPVORLAUFIST",            FREQ_30S,   cm_heizmodul},

    // ========================================================================
    // MONITORING — refrigerant pressures
    // These are the only maintenance values this model actually reports.
    // Pruned on 2026-10-18 after logging every poll for 12 minutes:
    // WASSERDRUCK, DURCHFLUSS_CH/WW and all five LAUFZEIT_* counters answer
    // 0x8000 ("not available" → -255) on *every* read, so polling them only
    // consumed request-schedule slots. Re-add a line if a future WPM3 firmware
    // starts reporting one.
    // ========================================================================
    {"ANZEIGE_HOCHDRUCK",                FREQ_1MIN,  cm_heizmodul}, // refrigerant high pressure
    {"ANZEIGE_NIEDERDRUCK",              FREQ_1MIN,  cm_heizmodul}, // refrigerant low pressure
};

extern const size_t SIGNAL_REQUEST_COUNT_VALUE = sizeof(signalRequests) / sizeof(SignalRequest);

#endif // SIGNAL_REQUESTS_WPL10AC_H