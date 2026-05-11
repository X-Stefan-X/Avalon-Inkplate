---
repo: https://github.com/X-Stefan-X/Avalon-Inkplate
hardware: Inkplate 6 (ESP32-basiertes E-Paper Display)
source-file: src/Avalon Display.cpp
signalk-host: openplotter-test.local:3000
---

# Interface: Inkplate 6 (Avalon Display)

Reines Anzeigegerät — kein SensESP, keine GPIO-Aktoren. Verbindet sich direkt per WebSocket und HTTP REST mit dem SignalK Server.

## Architektur

- **FreeRTOS** mit 3 Tasks: `WebSocketTask` (Core 0), `DisplayTask` (Core 0), `SlowDataTask` (Core 1)
- **WebSocket** für Echtzeit-Navigationsdaten (Subscribe/Delta)
- **HTTP REST** für Barometer-Trend (polling alle 10 s)
- Zwei Refresh-Klassen: Navigation (alle 500 ms), Umgebung (alle 3 s)

## Subscribes (SignalK → Anzeige)

### Schnelle Daten — WebSocket, Periode 200 ms

| SK-Pfad | Einheit SK | Anzeige | Konvertierung |
|---|---|---|---|
| `environment.depth.belowSurface` | m | Tiefe in m | — |
| `navigation.courseOverGroundTrue` | rad | COG in ° | × RAD_TO_DEG |
| `navigation.speedThroughWater` | m/s | STW in kn | × 1.94384 |
| `navigation.speedOverGround` | m/s | SOG in kn | × 1.94384 |
| `environment.wind.angleTrueGround` | rad | TWA in ° | × RAD_TO_DEG |
| `environment.wind.angleApparent` | rad | AWA in ° | × RAD_TO_DEG |
| `environment.wind.speedOverGround` | m/s | TWS in kn | × 1.94384 |
| `environment.wind.speedApparent` | m/s | AWS in kn | × 1.94384 |

### Langsame Daten — WebSocket, Periode 10.000 ms

| SK-Pfad | Einheit SK | Anzeige | Konvertierung |
|---|---|---|---|
| `environment.outside.pressure` | Pa | Luftdruck in hPa | ÷ 100 |
| `navigation.position` | lat/lon (°) | Position Lat/Lon | — |

### Langsame Daten — HTTP REST, alle 10 s

| Endpunkt | Felder | Anzeige |
|---|---|---|
| `/signalk/v1/api/vessels/self/environment/barometer/trend` | `severity`, `tendency`, `changerate` | Drucktendenz unter Luftdruck |

## Display-Layout (Inkplate 6, 600×800 px, 1-Bit)

```
┌─────────────────┬─────────────────┐
│  Tiefe in m     │  COG in Grad    │
│  (env.depth)    │  (nav.COG)      │
├─────────────────┼─────────────────┤
│  STW in kn      │  SOG in kn      │
├─────────────────┼─────────────────┤
│  Luftdruck hPa  │  Position       │
│  + Trend        │  Lat / Lon      │
├─────────────────┼─────────────────┤
│  TWA in Grad    │  AWA in Grad    │
├─────────────────┼─────────────────┤
│  TWS in kn      │  AWS in kn      │
└─────────────────┴─────────────────┘
         (Inkplate / Avalon 2026)
```

## Hardware Interface

| Typ | Detail |
|---|---|
| Display | Inkplate 6 — E-Paper 1-Bit, 600×800 px |
| WiFi | WebSocket + HTTP → SignalK Server |
| Kein GPIO-Ausgang | Nur Anzeige, keine Aktoren |

## Offene Punkte / Hinweise

- ⚠️ `environment.wind.speedOverGround` als TWS ist ungewöhnlich — SignalK-Standard für True Wind Speed wäre `environment.wind.speedTrue`
- ⚠️ Bug in `displayNavigationData()`: Zeile für TWS-Feld (Zeile ~695) rendert `navData.tws` statt `navData.twa` — TWA wird in dieser Zeile nicht korrekt angezeigt
- SignalK-Host ist hardcodiert: `openplotter-test.local` — kein mDNS-Fallback
- Drucktendenz kommt parallel über HTTP REST, nicht über WebSocket-Subscribe