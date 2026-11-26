# 📜 Changelog · MiauCore 4.3.4

Registro de cambios notables del core. · Notable changes to the core.

---

## 🗡️ Pícaro · Rogue — 2026-11-26

Revisión amplia de la clase Pícaro y del sistema de amenaza que la soporta.
· Broad pass over the Rogue class and the threat system that backs it.

### ⚙️ Motor · Engine

- **`Spell` — `GetComboPointsUsed()`:** se capturan los puntos de combo **en el
  momento del lanzamiento** (`m_comboPointsAtCast`), para que los finishers calculen
  daño y efectos con los puntos realmente gastados, no con el valor ya reseteado.
  · Combo points are captured **at cast time**, so finishers compute from the points
  actually spent instead of the post-reset value.
- **`ThreatManager` — amenaza que expira · expiring threat:** la amenaza
  transferida por **Tretas del Oficio** (Tricks of the Trade, `57934`) ahora se
  **desvanece a los 30 s**, como en retail. Nuevo `AddExpiringThreat` / `ExpireThreat`,
  `_redirectInfo` pasa a struct con `SpellId`, y `AddThreatInternal(..., expires)`.
- **`SpellMgr`:** corrección de **Cheat Death** (`45182`) → aura de reducción de
  daño recibido (`SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN`).
- **`cs_debug`:** el comando `.debug threat` adaptado a la nueva estructura `RedirectInfo`.

### 🪄 Hechizos de pícaro · Rogue spells

- **Cheat Death · Burlar a la Muerte:** lanza la reducción de daño (`45182`),
  cooldown a **90 s** y la curación mínima protegida (≥ 1).
- **Tricks of the Trade · Tretas del Oficio:** el objetivo debe estar **vivo y en tu
  banda**; el redirect se mantiene durante los 6 s del proc en vez de cancelarse al instante.
- **Honor Among Thieves · Honor entre Ladrones:** nuevo `CheckProc` — solo procea con
  un objetivo de ataque válido.
- **Main Gauche:** usa `GetTarget` en vez de `GetActor`, evita auto-procearse y valida el arma.
- **Eviscerate · Eviscerar:** usa `GetComboPointsUsed()`; finisher rehecho
  (Serrated Blades refresca Rupture y consume Revealing Strike).
- **Envenom · Envenenar:** usa `GetComboPointsUsed()`; consume cargas de Veneno Mortal
  al impactar y gestiona Revealing Strike.
- **Combat Potency · Potencia de Combate:** procea desde Main Gauche.
- **Restless Blades · Cuchillas Inquietas:** nuevo `CheckProc` — solo con finishers que
  gasten puntos de combo.
- **Premeditation · Premeditación (`14183`):** **script nuevo** — otorga puntos de combo
  hasta un máximo de 5.

> [!NOTE]
> Verificado: `game` y `scripts` **compilan sin errores**. Para que el servidor en
> marcha use estos cambios hace falta recompilar el `worldserver` y reiniciarlo.
> · Verified: `game` and `scripts` compile cleanly. A `worldserver` rebuild + restart
> is required for a running server to pick these up.
