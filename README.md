<div align="center">

# 🐉 MiauCore 4.3.4
### ⚔️ Cataclysm Preservation Project

[![WoW](https://img.shields.io/badge/WoW-4.3.4%20Cataclysm-F8B700?style=for-the-badge&logo=battledotnet&logoColor=white)]()
[![Build](https://img.shields.io/badge/build-15595-00A2FF?style=for-the-badge)]()
[![TrinityCore](https://img.shields.io/badge/based%20on-TrinityCore-E74C3C?style=for-the-badge)](https://www.trinitycore.org/)
[![License](https://img.shields.io/badge/license-GPL--2.0-2ECC71?style=for-the-badge)](COPYING)
[![Status](https://img.shields.io/badge/estado-compila%20%26%20arranca-27AE60?style=for-the-badge)]()

**🌍 Core de servidor de World of Warcraft 4.3.4 · A WoW 4.3.4 server core**

*Proyecto de preservación basado en TrinityCore · A preservation project based on TrinityCore*

</div>

---

<div align="center">

## 📊 El trabajo en números · The work in numbers

![Clases](https://img.shields.io/badge/Clases%20auditadas-10%2F10-success?style=flat-square)
![Mago](https://img.shields.io/badge/Hechizos%20de%20mago-~288-blue?style=flat-square)
![Scripts](https://img.shields.io/badge/Scripts%20de%20hechizo-35%2B-blueviolet?style=flat-square)
![Seguridad](https://img.shields.io/badge/Correcciones%20de%20seguridad-12-critical?style=flat-square)
![Hardening](https://img.shields.io/badge/Anti--DoS%20%7C%20anti--cheat-4-orange?style=flat-square)
![NPC](https://img.shields.io/badge/NPC%20nuevo-1-ff69b4?style=flat-square)
![Zona](https://img.shields.io/badge/Zona%20reparada-Deathknell-yellowgreen?style=flat-square)

</div>

| 🧩 Área · Area | 🔢 Cantidad · Count |
|:---|:---:|
| Clases auditadas vs DBC 15595 · Classes audited | **10 / 10** |
| Hechizos de mago revisados uno a uno · Mage spells reviewed | **~288** |
| Scripts de hechizo añadidos/corregidos · Spell scripts added/fixed | **35+** |
| Correcciones de seguridad · Security fixes | **12** |
| Endurecimientos anti-DoS / anti-cheat · Hardening | **4** |
| NPC de bienvenida nuevo · New welcome NPC | **1** |
| Zona de inicio reparada · Starting zone repaired | **1** (Deathknell) |

---

## 🧭 Estado de scripting por clase · Per-class scripting status

> 🇪🇸 Verificación **hechizo por hechizo** contra el DBC del cliente (build 15595):
> comprobando el efecto/aura e índice de cada script. 🇬🇧 **Spell-by-spell** verification
> against the client DBC (build 15595): effect/aura and index checked for every script.

| Clase · Class | Estado · Status |
|:---|:---|
| 🗡️ Pícaro · Rogue | ✅ Completamente scripteada · Fully scripted |
| 🔮 Brujo · Warlock | ✅ Completamente scripteada · Fully scripted |
| ⚔️ Guerrero · Warrior | ✅ Completamente scripteada · Fully scripted |
| 🛡️ Paladín · Paladin | 🚧 En progreso · In progress |

![Pícaro](https://img.shields.io/badge/P%C3%ADcaro-verificado-success?style=flat-square)
![Brujo](https://img.shields.io/badge/Brujo-verificado-success?style=flat-square)
![Guerrero](https://img.shields.io/badge/Guerrero-verificado-success?style=flat-square)
![Paladín](https://img.shields.io/badge/Palad%C3%ADn-en%20progreso-orange?style=flat-square)

---

## 🪄 Hechizos · Spells

> 🇪🇸 Las **10 clases** auditadas contra el DBC del cliente (build 15595). El **Mago**
> recibió una auditoría hechizo por hechizo (**~288**). Se añadieron o corrigieron
> **más de 35 scripts** en Mago, Guerrero, Paladín, Sacerdote, Pícaro, Druida,
> Cazador, Chamán, Brujo y Caballero de la Muerte (incluyendo muchos glifos que
> faltaban). `DBErrors` se queda en **0** tras cada reinicio.
>
> 🇬🇧 All **10 classes** audited against the client DBC (build 15595). The **Mage**
> got a full spell-by-spell audit (**~288**). **35+ spell scripts** were added or
> corrected across all classes (including many missing glyphs). `DBErrors` stays
> at **0** after every restart.

## 🗺️ Zonas y NPCs · Zones & NPCs

> 🇪🇸 Un **NPC de bienvenida** (apariencia de Alexstrasza) que sube al jugador a 85,
> le da oro, lo teletransporta a la capital y autocompleta las cadenas de inicio de
> CM / Huargen / Goblin. La **zona de inicio de los No-Muertos (Deathknell)** fue
> realineada: posiciones de NPCs, Agatha/Val'kyr y cadenas de misión ajustadas a 4.3.4.
>
> 🇬🇧 A **welcome NPC** (Alexstrasza's look) that levels the player to 85, grants
> gold, teleports to the faction capital and auto-completes the DK/Worgen/Goblin
> starting chains. The **Undead starting zone (Deathknell)** was realigned to match 4.3.4.

## 🛡️ Seguridad · Security

> 🇪🇸 **12 correcciones** — toma de cuentas por clave de sesión vacía (auth + world),
> lectura fuera de límites en el token TOTP, **8 vectores de caída** por reserva de
> memoria sin cota (addon, consulta masiva, talentos, quién, pesos de hechizo,
> transmog, LFG, tickets) y 2 fallos de caída por IA.
>
> 🇬🇧 **12 fixes** — account takeover via empty session key, a TOTP out-of-bounds
> read, **8 unbounded-allocation crash vectors** and 2 AI crash fixes.

**➕ Endurecimiento · Hardening:**

| 🔒 | Medida · Measure |
|:---:|:---|
| 🚦 | Límite de conexiones por IP · Per-IP connection throttle |
| 🌊 | Anti-inundación de paquetes por sesión · Per-session packet flood limit |
| 🏃 | Anti-cheat de movimiento · Movement anti-cheat |
| 👁️ | Visibilidad ajustada para menos lag · Tuned visibility for less lag |

---

## ✅ Estado · Status

> [!NOTE]
> 🇪🇸 `game`, `authserver` y `worldserver` **compilan sin errores**; el servidor
> **arranca** (`World initialized`) con **0 errores de base de datos**. Verificado:
> compila, arranca y carga la BD.
>
> 🇬🇧 `game`, `authserver` and `worldserver` **compile cleanly**; the server
> **boots** (`World initialized`) with **0 DB errors**. Verified: builds, boots,
> database loads.

> [!WARNING]
> 🇪🇸 Algunos arreglos de hechizos aún merecen **prueba real en juego** con jugadores.
> 🇬🇧 Some gameplay spell fixes still deserve **live in-game testing** with players.

---

## 📦 Contenido · Contents

> [!TIP]
> 🇪🇸 **Incluye:** el código base del core (`authserver`, `worldserver`, librerías
> compartidas y dependencias) con todas las correcciones de MiauCore.
> **No incluye:** SQL, herramientas, binarios compilados ni `.conf` con credenciales reales.
>
> 🇬🇧 **Includes:** the base source of the core with all MiauCore fixes.
> **Excludes:** SQL, tools, compiled binaries and `.conf` files with real credentials.

---

## 📜 Créditos y licencia · Credits & license

<div align="center">

[![TrinityCore](https://img.shields.io/badge/Built%20on-TrinityCore-E74C3C?style=for-the-badge)](https://www.trinitycore.org/)
[![GPL-2.0](https://img.shields.io/badge/License-GPL--2.0-2ECC71?style=for-the-badge)](COPYING)

</div>

🇪🇸 Este proyecto se basa en **TrinityCore**, bajo la **licencia GNU GPL v2** (ver
[`COPYING`](COPYING) y [`AUTHORS`](AUTHORS)). Se conservan todos los derechos de
autor y la licencia. MiauCore es un esfuerzo de preservación **independiente y sin
ánimo de lucro**, no afiliado a Blizzard Entertainment ni al proyecto TrinityCore.

🇬🇧 This project is built on **TrinityCore**, released under the **GNU GPL v2**
(see [`COPYING`](COPYING) and [`AUTHORS`](AUTHORS)). All original copyrights and the
license are preserved. MiauCore is an **independent, non-commercial** preservation
effort, not affiliated with Blizzard Entertainment or the TrinityCore project.

<div align="center">

**🐉 MiauCore · 4.3.4 Cataclysm 🐉**

</div>
