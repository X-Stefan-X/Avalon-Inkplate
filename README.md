# Avalon-Inkplate

Navigationsdisplay für eine Segelyacht auf Basis eines **Inkplate 6** (ESP32, E-Paper 600×800 px, 1-Bit).  
Verbindet sich direkt per WebSocket und HTTP REST mit einem **SignalK-Server** — kein SensESP, keine GPIO-Aktoren.

## Features

- Echtzeit-Navigationsdaten über WebSocket-Subscribe (SignalK Delta)
- Barometer-Trend über HTTP REST
- Automatische SignalK-Host-Erkennung via **mDNS** (`_signalk-ws._tcp`)
- FreeRTOS-Multitasking: Datenempfang, Anzeige und HTTP entkoppelt
- Zwei Refresh-Klassen: Navigation (500 ms), Umgebung (3 s)
- Einheitenumrechnung: rad → °, m/s → kn, Pa → hPa
- Verbindungsverlust-Erkennung mit Fehleranzeige auf dem Display
- Splash-Screen beim Start mit Foto des Bootes

## Angezeigte Daten

| Anzeige | SignalK-Pfad |
|---|---|
| Tiefe (m) | `environment.depth.belowSurface` |
| COG (°) | `navigation.courseOverGroundTrue` |
| STW (kn) | `navigation.speedThroughWater` |
| SOG (kn) | `navigation.speedOverGround` |
| TWA (°) | `environment.wind.angleTrue` |
| AWA (°) | `environment.wind.angleApparent` |
| TWS (kn) | `environment.wind.speedTrue` |
| AWS (kn) | `environment.wind.speedApparent` |
| Luftdruck + Trend | `environment.outside.pressure` + HTTP barometer/trend |
| Position Lat/Lon | `navigation.position` |

## Hardware

- **Inkplate 6** — E-Paper Display, ESP32, 600×800 px
- WiFi-Verbindung zum SignalK-Server (WebSocket + HTTP)

## Konfiguration

SSID, Passwort und (optional) SignalK-Host werden als Build-Flags in `platformio.ini` gesetzt:

```ini
build_flags =
    '-D WIFI_SSID="MeinNetz"'
    '-D WIFI_PASS="MeinPasswort"'
```

Der SignalK-Host wird automatisch per mDNS gefunden. Wird kein Server entdeckt, greift der Fallback `openplotter-test.local:3000`.

## Architektur

```
Core 0                        Core 1
┌─────────────────────┐       ┌─────────────────────┐
│  WebSocketTask      │       │  SlowDataTask        │
│  - WS loop          │       │  - HTTP barometer    │
│  - JSON-Parsing     │       │    alle 10 s         │
│  - navData/envData  │       └─────────────────────┘
├─────────────────────┤
│  DisplayTask        │
│  - navData 500 ms   │
│  - envData 3 s      │
│  - Mutex auf        │
│    display.*        │
└─────────────────────┘
```

## Entwicklungsgeschichte

1. **V1** — Einfache Polling-Schleife, kein FreeRTOS
2. **V2** — SensESP-basiert
3. **V3 (aktuell)** — Direkter WebSocket + FreeRTOS, Datenempfang und Anzeige vollständig entkoppelt
