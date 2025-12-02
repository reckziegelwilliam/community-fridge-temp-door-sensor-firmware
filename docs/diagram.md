# Hardware & System Diagrams

This document illustrates how the **Community Fridge Probe Firmware** fits into a real refrigerator, how the electronics are wired, and how the node can integrate with a backend/dashboard.

- **Board:** Raspberry Pi Pico (RP2040)
- **Sensors:** TMP36 (or thermistor) for temperature, magnetic reed switch for door
- **Firmware:** `fridge_probe.uf2` from this repository

---

## 1. Physical Fridge Layout & Probe Placement

This diagram shows a typical top-freezer household fridge and where the **temperature probe**, **door sensor**, and **controller box** are placed.

### 1.1 Side View of Fridge + Probe Hardware

```text
                         FRONT (doors)
                   ┌───────────────────────┐
                   │      FREEZER DOOR     │
                   ├───────────────────────┤
                   │      FRIDGE DOOR      │
                   └───────────────────────┘
                         ▲           ▲
                         │           │
                         │           │
        ┌────────────────┘           │
        │                            │
        │   ┌──────────────────────┐ │
 TOP    │   │      FREEZER        │ │
        │   │  (evaporator coil   │ │
        │   │   & fan hidden)     │ │
        │   └──────────────────────┘ │
        │   ┌──────────────────────┐ │
        │   │   FRESH-FOOD        │ │
        │   │   COMPARTMENT       │ │
        │   │                      │ │
        │   │   [A] TEMP PROBE    │ │
        │   │       (sensor tip)  │◄─── Stainless probe or TMP36/thermistor
        │   │                      │     mounted mid-height, away from walls
        │   │                      │
        │   └──────────────────────┘
        │
        │   ┌──────────────────────┐
 REAR   │   │   INSULATION +       │
        │   │   CABINET WALL       │
        │   └──────────────────────┘
        │
        │   ┌──────────────────────┐
        │   │ MECH. COMPARTMENT    │
        │   │                      │
        │   │  [C] COMPRESSOR      │
        │   │  [D] CONDENSER COILS │
        │   │  [E] DRAIN PAN       │
        │   └──────────────────────┘
        └───────────────────────────────────────────── FLOOR

Outside hardware on or near fridge
────────────────────────────────────────────────────────────────

                 (magnet on door edge)
                    ┌────────┐
      [B] DOOR MAGN.│  MAG   │
                    └────────┘
                        ║   ← closes to…
                        ║  (when door shut)
                        ║
   DOOR FRAME EDGE  ┌───────────┐
                    │  REED     │  [B] DOOR REED SENSOR
                    │  SWITCH   │  - One lead to GPIO15
                    └───────────┘  - Other lead to GND
                        │
                        │ cable (thin low-voltage) out of
                        │ gasket edge / hinge side, zip-tied
                        ▼
          ┌────────────────────────────┐
          │ [F] PROBE CONTROLLER BOX   │
          │  (Pico + PSU + wiring)     │
          │                            │
          │  - Raspberry Pi Pico       │
          │  - Temp sensor cable in    │◄── from [A]
          │  - Door reed cable in      │◄── from [B]
          │  - USB cable (data+power)  │◄── to host/logging
          └────────────────────────────┘
                        │
                        │ USB / power
                        ▼
                Wall outlet / nearby host
````

**Caption:**

* **[A] Temp probe** sits inside the fresh-food compartment at mid-height, not touching walls or vents.
* **[B] Reed switch + magnet** detect whether the door is open or closed.
* **[F] Controller box** (with the Pico) lives outside the fridge, connecting to sensors via low-voltage cables and to a host/logging device via USB.

---

## 2. Electronics Block Diagram (Pico + Sensors + USB)

This diagram shows how the **Raspberry Pi Pico**, **temperature sensor**, **door sensor**, and **status LED** are wired, matching the pin assignments in the README.

### 2.1 Node Hardware Overview

```text
                           COMMUNITY FRIDGE PROBE NODE
                           ───────────────────────────

           INSIDE FRIDGE                          CONTROLLER BOX / PICO
──────────────────────────────────┐        ┌─────────────────────────────────┐
                                  │        │ Raspberry Pi Pico (RP2040)     │
      [A] TEMP SENSOR             │        │                                 │
      (TMP36 / thermistor)        │        │       ┌───────────────────┐     │
      ┌───────────────┐           │  ADC0  │  GPIO26│  ADC + TEMP       │     │
      │ Stainless     │────────────────────▶        │  CONVERSION       │     │
      │ probe / TO-92 │ signal     │        │       └───────────────────┘     │
      │               │            │        │               ▲                 │
      └───────────────┘            │        │               │                 │
         │      │      │           │        │       ┌───────────────────┐     │
        VCC    GND   SHIELD        │  3V3   │  3V3  │ Power for sensors │     │
         │      │                  │  GND   │  GND  └───────────────────┘     │
         ▼      ▼                  │        │                                 │
                                  │        │       ┌───────────────────┐     │
      [B] DOOR REED SWITCH        │        │  GPIO15│ DOOR SENSOR      │     │
                                  │────────▶        │ (debounce logic) │     │
 Door               Frame         │        │   GND  └───────────────────┘     │
 ┌──────┐        ┌───────────┐    │        │               ▲                 │
 │ MAG  │◄══════▶│ REED SW   │────┘        │               │                 │
 └──────┘        └───────────┘             │       ┌───────────────────┐     │
                                           │       │   APP_LOGIC       │     │
                                           │       │ - History buffer  │     │
                                           │       │ - Rolling avg     │     │
                                           │       │ - Status decision │     │
                                           │       └───────────────────┘     │
                                           │         ▲           ▲           │
                                           │         │           │           │
                                           │   ┌──────────┐   ┌──────────┐   │
                                           │   │ LED      │   │ SERIAL   │   │
                                           │   │ STATUS   │   │ TELEMETRY│   │
                                           │   └──────────┘   └──────────┘   │
                                           │       │              │          │
                                           │  GPIO25│              │ USB     │
                                           │  (on-  │              │ (CDC)   │
                                           │  board)│              │         │
                                           └─────▲──┴──────────────┴─────────┘
                                                 │
                      [C] STATUS LED (GPIO25)    │
                      - Solid / blink patterns   │
                                                 │
                                    USB CABLE ───┘
                              (power + 115200 baud serial)
                              to laptop / Pi / logging box
```

**Caption:**

* Temperature sensor output feeds **GPIO26 (ADC0)**.
* Door reed switch bridges **GPIO15 ↔ GND**, using the Pico’s internal pull-up and software debouncing.
* **GPIO25** drives the on-board LED for local status patterns.
* USB provides both power and **serial telemetry** (e.g., `t=4.3C, avg=4.1C, door=closed, status=OK`).

---

## 3. System Context Diagram (Probe + Backend / Dashboard)

This diagram shows how a deployed node can fit into a larger monitoring system (optional for v1 firmware, but useful for future extensions).

### 3.1 Node in a Monitoring System

```text
                   COMMUNITY FRIDGE MONITORING SYSTEM
                   ──────────────────────────────────

                             INTERNET / LAN
                     ┌────────────────────────┐
                     │  Backend / Ingest API │
                     │  (Go / Python / etc.) │
                     └──────────▲────────────┘
                                │
                                │ JSON / line protocol
                                │ (HTTP, MQTT, etc.)
                                │
                 ┌──────────────┴──────────────┐
                 │                             │
         ┌───────┴────────┐             ┌──────┴─────────┐
         │  Time-series   │             │  Web / Mobile  │
         │  DB / Storage  │             │  Dashboard     │
         │ (Influx, TSDB, │             │ (wefridge UI)  │
         │  Postgres)     │             │                │
         └───────▲────────┘             └──────▲─────────┘
                 │                                │
                 │                                │
           Metrics / alerts                Charts, alerts,
           (temp, door, status)            fridge health, logs

─────────────────────────────────────────────────────────────────────

            FRIDGE SITE / PHYSICAL LOCATION

                  ┌──────────────────────────┐
                  │  PROBE CONTROLLER BOX   │
                  │  (Raspberry Pi Pico)    │
                  │                          │
                  │  - Reads temp + door    │
                  │  - LED status patterns  │
                  │  - Serial telemetry     │
                  └───────────▲─────────────┘
                              │ USB (serial)
                              │
                  ┌───────────┴─────────────┐
                  │  Local Host Device      │
                  │  (Raspberry Pi, NUC,    │
                  │   or laptop)            │
                  │                         │
                  │ - Reads serial stream   │
                  │ - Converts to JSON      │
                  │ - Sends to backend      │
                  └─────────────────────────┘
```

**Caption:**

* The **Pico node** is responsible only for **sensing, local status indication, and serial telemetry**.
* A nearby **host device** (Pi, mini-PC, or laptop) reads the probe’s serial output, then forwards metrics to a **backend API**.
* The backend persists data in a **time-series DB** and powers dashboards and alerts for fridge health and food safety.
