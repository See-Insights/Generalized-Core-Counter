# Generalized Core Counter - Project Status

## Project Purpose

The Generalized Core Counter is the next-generation firmware platform for See Insights occupancy and visitation counters.

The goal is to create a highly reliable, low-power, field-deployable IoT platform that can support multiple sensing technologies, multiple communications methods, and multiple deployment scenarios using a common software architecture.

This firmware is intended to operate unattended for years in remote locations with unreliable cellular connectivity, limited power availability, and minimal maintenance opportunities.

---

# Current Project Status

## Overall Health

Status: Stable and Functional

Current focus areas:

1. Cellular connectivity resiliency
2. Device OS 6.4.x migration
3. Power consumption optimization
4. Codebase simplification and modularization
5. LoRa gateway/node architecture improvements
6. Fleet observability and diagnostics

The platform is currently deployed in production and supports active customer installations.

---

# Core Architectural Principles

## Principle 1 - Reliability Above All Else

The primary objective is not feature velocity.

The primary objective is continuous field operation.

A device that runs for years without intervention is more valuable than a device with additional features.

When evaluating changes:

Reliability > Maintainability > Features

---

## Principle 2 - Battery Is a Critical Resource

All design decisions must consider power consumption.

The system should aggressively sleep whenever useful work is not being performed.

Connectivity attempts should be adaptive and battery-aware.

The firmware should preserve operational life during poor connectivity conditions.

---

## Principle 3 - Cellular Connectivity Is Unreliable

Assume cellular service may be:

- unavailable
- intermittent
- slow
- degraded for extended periods

The device must continue performing its primary sensing function even when cloud connectivity is unavailable.

Connectivity failures must never prevent data collection.

---

## Principle 4 - Non-Blocking Architecture

The main loop should remain responsive.

Long blocking delays are discouraged.

State-machine based operation is preferred.

Target:

Main loop execution time < 100 ms under normal conditions.

---

## Principle 5 - Configuration Comes From The Cloud

The cloud ledger is the source of truth.

Device configuration should be:

- centrally managed
- remotely adjustable
- persistently stored

Local configuration should only exist to support offline operation and startup.

---

# Current Technical Priorities

## Priority 1 - Device OS 6.4.x Migration

Objectives:

- leverage modern sleep architecture
- evaluate AM18x5 / AB1805 integration
- evaluate POWER_OFF sleep
- improve recovery after connectivity failures
- reduce battery consumption

Success criteria:

- no regression in field reliability
- measurable power improvement
- simpler sleep logic

---

## Priority 2 - Cellular Resiliency

Objectives:

- adaptive connection budgets
- battery-aware retry strategies
- reduced wasted connection attempts
- improved diagnostics

Success criteria:

- higher long-term reporting success
- lower battery depletion during outages

---

## Priority 3 - Codebase Simplification

Objectives:

- reduce coupling
- reduce file size
- improve readability
- improve AI-assisted maintainability

Current refactor candidates:

- Cloud.cpp
- Connectivity management
- Configuration management
- Reporting pipeline

---

# Known Constraints

## Hardware Constraints

Primary platforms:

- Particle Boron BRN404X
- Particle Boron BRN402X
- Photon 2

Memory and power limitations must be respected.

---

## Deployment Constraints

Devices may be deployed:

- in parks
- on trails
- in remote parking areas
- in areas without utility power

Devices may experience:

- extreme temperatures
- poor signal conditions
- seasonal accessibility limitations

---

# Success Metrics

The project is successful when:

- Devices reliably count visitors.
- Devices survive prolonged connectivity outages.
- Battery life supports intended deployment duration.
- Remote configuration works reliably.
- Firmware updates can be performed safely.
- Field support effort decreases over time.

---

# Design Rules For AI Assistants

When proposing changes:

1. Do not sacrifice reliability for elegance.
2. Do not increase power consumption without justification.
3. Do not introduce blocking delays.
4. Assume poor cellular conditions.
5. Preserve backward compatibility whenever practical.
6. Prefer simple and observable designs.
7. Favor maintainability over cleverness.
8. The cloud ledger is the source of truth.
9. Data collection must continue during cloud outages.
10. Explain tradeoffs before recommending major architectural changes.

---

# Next Major Milestone

Create a fully modular architecture separating:

- Sensor Management
- Connectivity Management
- Cloud Services
- Configuration Services
- Power Management
- Reporting Pipeline
- Diagnostics

while preserving field-proven reliability characteristics.