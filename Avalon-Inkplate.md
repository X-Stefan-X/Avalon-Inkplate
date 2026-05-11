---
repo: https://github.com/X-Stefan-X/Avalon-Inkplate
hardware: Inkplate 6 (ESP32-basiertes E-Paper Display)
source-file: src/Avalon Display.cpp
signalk-host: automatisch per mDNS, Fallback openplotter-test.local:3000
---

# Interface: Inkplate 6 (Avalon Display)

Reines Anzeigegerät — kein SensESP, keine GPIO-Aktoren. Verbindet sich direkt per WebSocket und HTTP REST mit dem SignalK Server.

## Architektur

- **FreeRTOS** mit 3 Tasks: `WebSocketTask` (Core 0, Prio 3), `DisplayTask` (Core 0, Prio 2), `SlowDataTask` (Core 1, Prio 1)
- **WebSocket** für Echtzeit-Navigationsdaten (Subscribe/Delta, Policy `fixed`)
- **HTTP REST** für Barometer-Trend (polling alle 10 s)
- Zwei Refresh-Klassen: Navigation (alle 500 ms), Umgebung (alle 3 s)
- Statischer RX-Puffer: 4096 Byte (kein heap-Alloc im ISR-Pfad)
- Mutex schützt Display-Zugriff zwischen Tasks (500 ms Timeout)
- WiFi-Reconnect über `onWiFiEvent()` — event-getrieben, kein Polling

## WebSocket-Konfiguration

| Parameter | Wert |
|---|---|
| URL | `/signalk/v1/stream?subscribe=none` |
| Reconnect-Intervall | 5000 ms |
| Heartbeat Ping | 15000 ms |
| Heartbeat Timeout | 3000 ms / 2 Versuche |
| Disconnect-Timeout | 30 s |

## SignalK-Host-Discovery

Host und Port werden beim Start per mDNS (`_signalk-ws._tcp`) automatisch ermittelt. Nach jedem WiFi-Reconnect (`STA_GOT_IP`) wird `discoverSignalK()` erneut aufgerufen. Fallback: `openplotter-test.local:3000`.

## Subscribes (SignalK → Anzeige)

### Schnelle Daten — WebSocket, Periode 200 ms

| SK-Pfad | Einheit SK | Anzeige | Konvertierung |
|---|---|---|---|
| `environment.depth.belowSurface` | m | Tiefe in m | — |
| `navigation.courseOverGroundTrue` | rad | COG in ° | × RAD_TO_DEG |
| `navigation.speedThroughWater` | m/s | STW in kn | × 1.94384 |
| `navigation.speedOverGround` | m/s | SOG in kn | × 1.94384 |
| `environment.wind.angleTrue` | rad | TWA in ° | × RAD_TO_DEG |
| `environment.wind.angleApparent` | rad | AWA in ° | × RAD_TO_DEG |
| `environment.wind.speedTrue` | m/s | TWS in kn | × 1.94384 |
| `environment.wind.speedApparent` | m/s | AWS in kn | × 1.94384 |

### Langsame Daten — WebSocket, Periode 10.000 ms

| SK-Pfad | Einheit SK | Anzeige | Konvertierung |
|---|---|---|---|
| `environment.outside.pressure` | Pa | Luftdruck in hPa | ÷ 100 |
| `navigation.position` | lat/lon (°) | Position Lat/Lon | — (eigener String-Parser, key-sensitiv) |

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
```

Mittelspalte: schwarzer Balken bei x=280, beschriftet mit „Inkplate 4 Avalon 2026" (vertikal).

## Startup-Sequenz

1. WiFi-Scan (bis zu 10 Netzwerke auf Display)
2. WiFi-Event-Handler registrieren (`WiFi.onEvent`)
3. WiFi-Verbindung (SSID/PASS aus build flags)
4. `discoverSignalK()` — mDNS-Suche nach `_signalk-ws._tcp`
5. Splash-Screen: Bild `IMG_8906` + „Willkommen auf Avalon!" (3 s)
6. WebSocket-Verbindung zu SignalK
7. Bei `WStype_CONNECTED`: `initDisplayText()` → statisches Layout aufbauen
8. Subscribe aller Pfade

## Hardware Interface

| Typ | Detail |
|---|---|
| Display | Inkplate 6 — E-Paper 1-Bit, 600×800 px |
| WiFi | WebSocket + HTTP → SignalK Server |
| Kein GPIO-Ausgang | Nur Anzeige, keine Aktoren |

## Hinweise

- `navigation.position` verwendet `safeParsePos(ptr, key)` — key-Parameter stellt korrekte Offset-Berechnung für `"latitude":` (11 Zeichen) und `"longitude":` (12 Zeichen) sicher
- `DisplayError()` wird ausschließlich durch `displayUpdateTask` unter Mutex aufgerufen — kein direkter Display-Zugriff aus dem WebSocket-Callback

