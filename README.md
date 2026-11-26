# MiauCore 4.3.4 — Cataclysm Preservation Project

> **English** · [Español](#español)

A preservation project for a **World of Warcraft 4.3.4 (build 15595, Cataclysm)**
server core, based on [TrinityCore](https://www.trinitycore.org/). The goal is to
keep a stable, playable and well-maintained Cataclysm core alive, with bug fixes
and quality improvements on top of the original framework.

### What this repository contains

- The **base source code** of the core (`authserver`, `worldserver`, shared
  libraries and bundled dependencies).
- Fixes and additions made for MiauCore: spell corrections across all classes,
  starting-zone scripting, a welcome NPC, security hardening (packet bounds
  checks, per-IP connection limiting, anti-flood) and movement anti-cheat.

### What it does **not** contain

- **SQL** (database structure and world data) — not published here.
- **Tools** and helper scripts.
- **Compiled binaries** and local configuration (`*.conf` with real credentials).

### Credits & license

This project is built on **TrinityCore**, released under the **GNU GPL v2**
(see the [`COPYING`](COPYING) and [`AUTHORS`](AUTHORS) files). All original
copyrights and the license are preserved. MiauCore is an independent,
non-commercial preservation effort and is **not** affiliated with Blizzard
Entertainment or the TrinityCore project.

---

## Español

Proyecto de preservación de un core de servidor de **World of Warcraft 4.3.4
(build 15595, Cataclysm)**, basado en [TrinityCore](https://www.trinitycore.org/).
El objetivo es mantener vivo un core de Cataclysm estable, jugable y bien
cuidado, con correcciones de errores y mejoras de calidad sobre el framework
original.

### Qué contiene este repositorio

- El **código base** del core (`authserver`, `worldserver`, librerías
  compartidas y dependencias incluidas).
- Correcciones y añadidos de MiauCore: arreglos de hechizos de todas las clases,
  scripting de zonas de inicio, un NPC de bienvenida, refuerzos de seguridad
  (cotas en el parseo de paquetes, límite de conexiones por IP, anti-inundación)
  y anti-cheat de movimiento.

### Qué **no** contiene

- **SQL** (estructura de la base de datos y datos del mundo) — no se publica aquí.
- **Herramientas** y scripts auxiliares.
- **Binarios compilados** y configuración local (`*.conf` con credenciales reales).

### Créditos y licencia

Este proyecto se basa en **TrinityCore**, publicado bajo la **licencia GNU GPL v2**
(ver los archivos [`COPYING`](COPYING) y [`AUTHORS`](AUTHORS)). Se conservan todos
los derechos de autor originales y la licencia. MiauCore es un esfuerzo de
preservación independiente y sin ánimo de lucro, y **no** está afiliado a Blizzard
Entertainment ni al proyecto TrinityCore.
