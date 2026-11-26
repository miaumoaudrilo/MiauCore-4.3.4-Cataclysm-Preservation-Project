/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Scripts for spells with SPELLFAMILY_MAGE and SPELLFAMILY_GENERIC spells used by mage players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_mage_".
 */

#include "ScriptMgr.h"
#include "CellImpl.h"
#include "Containers.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "ThreatManager.h"

namespace Spells::Mage
{
enum MageSpells
{
    SPELL_MAGE_ARCANE_POTENCY_RANK_1             = 31571,
    SPELL_MAGE_ARCANE_POTENCY_RANK_2             = 31572,
    SPELL_MAGE_ARCANE_POTENCY_TRIGGER_RANK_1     = 57529,
    SPELL_MAGE_ARCANE_POTENCY_TRIGGER_RANK_2     = 57531,
    SPELL_MAGE_ARCANE_BLAST                      = 30451,
    SPELL_MAGE_ARCANE_MISSILES                   = 5143,
    SPELL_MAGE_ARCANE_MISSILES_AURASTATE         = 79808,
    SPELL_MAGE_GLYPH_OF_ARCANE_POWER             = 56381,
    SPELL_MAGE_GLYPH_OF_FROSTFIRE                = 61205,
    SPELL_MAGE_GLYPH_OF_INVISIBILITY             = 56366,
    SPELL_MAGE_INVISIBILITY_SPEED                = 87833,
    SPELL_MAGE_BLAZING_SPEED                     = 31643,
    SPELL_MAGE_BRAIN_FREEZE_R1                   = 44546,
    SPELL_MAGE_BRAIN_FREEZE_TRIGGERED            = 57761,
    SPELL_MAGE_BLIZZARD_DAMAGE                  = 42208,
    SPELL_MAGE_BURNOUT                           = 29077,
    SPELL_MAGE_COLD_SNAP                         = 11958,
    SPELL_MAGE_COMBUSTION_DAMAGE                 = 83853,
    SPELL_MAGE_DEEP_FREEZE_DAMAGE                = 71757,
    SPELL_MAGE_EARLY_FROST_R1                    = 83049,
    SPELL_MAGE_EARLY_FROST_R2                    = 83050,
    SPELL_MAGE_EARLY_FROST_TRIGGERED_R1          = 83162,
    SPELL_MAGE_EARLY_FROST_TRIGGERED_R2          = 83239,
    SPELL_MAGE_EARLY_FROST_VISUAL                = 94315,
    SPELL_MAGE_FIREBALL                          = 133,
    SPELL_MAGE_FIRE_BLAST                        = 2136,
    SPELL_MAGE_FROST_NOVA                        = 122,
    SPELL_MAGE_FLAME_ORB_DUMMY                   = 82731,
    SPELL_MAGE_FLAME_ORB_SUMMON                  = 84765,
    SPELL_MAGE_FLAME_ORB_AOE                     = 82734,
    SPELL_MAGE_FLAME_ORB_BEAM_DUMMY              = 86719,
    SPELL_MAGE_FLAME_ORB_DAMAGE                  = 82739,
    SPELL_MAGE_FLAME_ORB_SELF_SNARE              = 82736,
    SPELL_MAGE_FROSTFIRE_BOLT                    = 44614,
    SPELL_MAGE_FROSTFIRE_ORB_DUMMY               = 92283,
    SPELL_MAGE_FROSTFIRE_ORB_SUMMON              = 84714,
    SPELL_MAGE_FROSTFIRE_ORB_AOE                 = 84718,
    SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R1           = 95969,
    SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R2           = 84721,
    SPELL_MAGE_FROSTFIRE_ORB_RANK_R2             = 84727,
    SPELL_MAGE_FROSTFIRE_BOLT_CHILL_EFFECT       = 44614,
    SPELL_MAGE_HOT_STREAK                        = 44445,
    SPELL_MAGE_HOT_STREAK_TRIGGERED              = 48108,
    SPELL_MAGE_HYPOTHERMIA                       = 41425,
    SPELL_MAGE_IMPROVED_HOT_STREAK               = 44446,
    SPELL_MAGE_IMPROVED_POLYMORPH_RANK_1         = 11210,
    SPELL_MAGE_IMPROVED_POLYMORPH_RANK_2         = 12592,
    SPELL_MAGE_GLYPH_OF_POLYMORPH                = 56375,
    SPELL_MAGE_IMPROVED_POLYMORPH_STUN_RANK_1    = 83046,
    SPELL_MAGE_IMPROVED_POLYMORPH_STUN_RANK_2    = 83047,
    SPELL_MAGE_IMPROVED_POLYMORPH_MARKER         = 87515,
    SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED    = 44413,
    SPELL_MAGE_INCANTERS_ABSORBTION_KNOCKBACK    = 86261,
    SPELL_MAGE_INCANTERS_ABSORPTION_RANK_1       = 44394,
    SPELL_MAGE_INCANTERS_ABSORPTION_RANK_2       = 44395,
    SPELL_MAGE_IGNITE                            = 12654,
    SPELL_MAGE_LIVING_BOMB                       = 44457,
    SPELL_MAGE_LIVING_BOMB_EXPLOSION             = 44461,
    SPELL_MAGE_MASTER_OF_ELEMENTS_ENERGIZE       = 29077,
    SPELL_MAGE_MIRROR_IMAGE_TRIGGERED_FIRE       = 88092,
    SPELL_MAGE_MIRROR_IMAGE_TRIGGERED_ARCANE     = 88091,
    SPELL_MAGE_MIRROR_IMAGE_TRIGGERED_FROST      = 58832,
    SPELL_MAGE_PERMAFROST_REDUCE_HEAL            = 68391,
    SPELL_MAGE_PERMAFROST_RANK_1                 = 11175,
    SPELL_MAGE_PERMAFROST_HEAL                   = 91394,
    SPELL_MAGE_PYROBLAST                         = 11366,
    SPELL_MAGE_PYROBLAST_HOT_STREAK              = 92315,
    SPELL_MAGE_PYROMANIAC_TRIGGERED              = 83582,
    SPELL_MAGE_SCORCH                            = 2948,
    SPELL_MAGE_SLOW                              = 31589,
    SPELL_MAGE_T12_4P_BONUS                      = 99064,
    SPELL_MAGE_STOLEN_TIME                       = 105785,
    SPELL_MAGE_T13_2P_BONUS                      = 105788,
    SPELL_MAGE_GLYPH_OF_ETERNAL_WATER            = 70937,
    SPELL_MAGE_SHATTERED_BARRIER                 = 55080,
    SPELL_MAGE_SUMMON_WATER_ELEMENTAL_PERMANENT  = 70908,
    SPELL_MAGE_SUMMON_WATER_ELEMENTAL_TEMPORARY  = 70907,
    SPELL_MAGE_GLYPH_OF_BLAST_WAVE               = 62126,

    SPELL_MAGE_FLAMESTRIKE                       = 2120,

    SPELL_MAGE_CHILLED_R1                        = 12484,
    SPELL_MAGE_CHILLED_R2                        = 12485,

    SPELL_MAGE_CONE_OF_COLD_AURA_R1              = 11190,
    SPELL_MAGE_CONE_OF_COLD_AURA_R2              = 12489,
    SPELL_MAGE_CONE_OF_COLD_TRIGGER_R1           = 83301,
    SPELL_MAGE_CONE_OF_COLD_TRIGGER_R2           = 83302,

    SPELL_MAGE_SHATTERED_BARRIER_R1              = 44745,
    SPELL_MAGE_SHATTERED_BARRIER_R2              = 54787,
    SPELL_MAGE_SHATTERED_BARRIER_FREEZE_R1       = 55080,
    SPELL_MAGE_SHATTERED_BARRIER_FREEZE_R2       = 83073,

    SPELL_MAGE_IMPROVED_MANA_GEM_TRIGGERED       = 83098,

    SPELL_MAGE_RING_OF_FROST_SUMMON              = 82676,
    SPELL_MAGE_RING_OF_FROST_FREEZE              = 82691,
    SPELL_MAGE_RING_OF_FROST_DUMMY               = 91264,

    SPELL_MAGE_FINGERS_OF_FROST                  = 44544,
    SPELL_MAGE_TEMPORAL_DISPLACEMENT             = 80354,
};

enum MageIcons
{
    ICON_MAGE_IMPROVED_FLAMESTRIKE               = 37,
    ICON_MAGE_IMPROVED_FREEZE                    = 94,
    ICON_MAGE_IMPROVED_MANA_GEM                  = 1036,
    ICON_MAGE_GLYPH_OF_ICE_BLOCK                 = 14,
    ICON_MAGE_GLYPH_OF_ICY_VEINS                 = 2162,
    ICON_MAGE_GLYPH_OF_MIRROR_IMAGE              = 331,
    ICON_MAGE_GLYPH_OF_POLYMORPH                 = 82,
    ICON_MAGE_HOT_STREAK                         = 2999

};

enum MiscSpells
{
    SPELL_HUNTER_INSANITY                        = 95809,
    SPELL_PRIEST_SHADOW_WORD_DEATH               = 32409,
    SPELL_SHAMAN_EXHAUSTION                      = 57723,
    SPELL_SHAMAN_SATED                           = 57724
};

// -31571 - Arcane Potency
class spell_mage_arcane_potency : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_ARCANE_POTENCY_RANK_1,
                SPELL_MAGE_ARCANE_POTENCY_RANK_2,
                SPELL_MAGE_ARCANE_POTENCY_TRIGGER_RANK_1,
                SPELL_MAGE_ARCANE_POTENCY_TRIGGER_RANK_2
            });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        uint32 spellId = 0;

        if (GetSpellInfo()->Id == SPELL_MAGE_ARCANE_POTENCY_RANK_1)
            spellId = SPELL_MAGE_ARCANE_POTENCY_TRIGGER_RANK_1;
        else if (GetSpellInfo()->Id == SPELL_MAGE_ARCANE_POTENCY_RANK_2)
            spellId = SPELL_MAGE_ARCANE_POTENCY_TRIGGER_RANK_2;
        if (!spellId)
            return;

        GetTarget()->CastSpell(GetTarget(), spellId, aurEff);

    }

    void Register() override
    {
        OnEffectProc.Register(&spell_mage_arcane_potency::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

enum MageSpellIcons
{
    SPELL_ICON_MAGE_SHATTERED_BARRIER = 2945
};

// 11113 - Blast Wave
class spell_mage_blast_wave : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_FLAMESTRIKE });
    }

    void HandleImprovedFlamestrike(SpellEffIndex /*effIndex*/)
    {
        ++_hitTargetsCount;
        if (_hitTargetsCount != 2)
            return;

        AuraEffect const* talent = GetCaster()->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_MAGE, ICON_MAGE_IMPROVED_FLAMESTRIKE, EFFECT_0);
        if (!talent || !roll_chance_i(talent->GetAmount()))
            return;

        if (WorldLocation const* targetDest = GetExplTargetDest())
            GetCaster()->CastSpell(targetDest->GetPosition(), SPELL_MAGE_FLAMESTRIKE, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_mage_blast_wave::HandleImprovedFlamestrike, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
private:
    uint8 _hitTargetsCount = 0;
};

// 43987 - Ritual of Refreshment
// Efecto dummy: inicia el ritual del nivel correspondiente (DBC 15595): 74650 (<80), 92824 (80-84), 92827 (85).
class spell_mage_ritual_of_refreshment : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ 74650, 92824, 92827 });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        uint8 level = caster->getLevel();
        uint32 spellId = level >= 85 ? 92827 : (level >= 80 ? 92824 : 74650);
        caster->CastSpell(caster, spellId, false);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_mage_ritual_of_refreshment::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// -86303 - Reactive Barrier
// Al recibir daño que deja al mago por debajo del 50% de vida, lanza Barrera de hielo (11426) sin coste de maná.
// Respeta el tiempo de reutilización de Barrera de hielo y lo inicia al activarse (DBC 15595). Probabilidad/ProcFlags de la DBC.
class spell_mage_reactive_barrier : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ 11426 });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        Unit* target = GetTarget();
        if (!damageInfo || !damageInfo->GetDamage() || !target->IsPlayer())
            return false;
        if (!target->ToPlayer()->HasSpell(11426) || target->GetSpellHistory()->HasCooldown(11426))
            return false;
        // Damage procs are dispatched after health has already been reduced.
        return target->IsAlive() && target->HealthBelowPct(50);
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        Unit* target = GetTarget();
        // sin coste ni lanzamiento, pero sin ignorar el tiempo de reutilización (lo inicia)
        target->CastSpell(target, 11426, CastSpellExtraArgs(TriggerCastFlags(TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_GCD
            | TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_CAST_DIRECTLY | TRIGGERED_IGNORE_SET_FACING)));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_reactive_barrier::CheckProc);
        OnEffectProc.Register(&spell_mage_reactive_barrier::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// -31641 - Blazing Speed
class spell_mage_blazing_speed : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_BLAZING_SPEED });
    }

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        Unit* target = GetTarget();
        if (target->CastSpell(target, SPELL_MAGE_BLAZING_SPEED, aurEff) != SPELL_CAST_OK)
            return;

        target->RemoveAurasWithMechanic((1u << MECHANIC_ROOT) | (1u << MECHANIC_SNARE), AuraRemoveFlags::ByDefault, 0, true);
        target->RemoveAurasByType(SPELL_AURA_MOD_ROOT, ObjectGuid::Empty, nullptr, true, false);
        target->RemoveAurasByType(SPELL_AURA_MOD_DECREASE_SPEED, ObjectGuid::Empty, nullptr, true, false);
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_mage_blazing_speed::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 42208 - Blizzard
/// Updated 4.3.4
class spell_mage_blizzard : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_CHILLED_R1,
                SPELL_MAGE_CHILLED_R2
            });
    }

    void AddChillEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (Unit* unitTarget = GetHitUnit())
        {
            if (caster->IsScriptOverriden(GetSpellInfo(), 836))
                caster->CastSpell(unitTarget, SPELL_MAGE_CHILLED_R1, true);
            else if (caster->IsScriptOverriden(GetSpellInfo(), 988))
                caster->CastSpell(unitTarget, SPELL_MAGE_CHILLED_R2, true);
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_mage_blizzard::AddChillEffect, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 11958 - Cold Snap
class spell_mage_cold_snap : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->GetSpellHistory()->ResetCooldowns([](SpellHistory::CooldownStorageType::iterator itr) -> bool
        {
            SpellInfo const* spellInfo = sSpellMgr->AssertSpellInfo(itr->first);
        return spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && (spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_FROST) &&
            spellInfo->Id != SPELL_MAGE_COLD_SNAP && spellInfo->GetRecoveryTime() > 0;
        }, true);
    }

    void Register() override
    {
        OnEffectHit.Register(&spell_mage_cold_snap::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 120 - Cone of Cold
/// Updated 4.3.4
static std::array<uint32, 2> const ImprovedConeOfColdSpellIds = { SPELL_MAGE_CONE_OF_COLD_TRIGGER_R1, SPELL_MAGE_CONE_OF_COLD_TRIGGER_R2 };

class spell_mage_cone_of_cold : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(ImprovedConeOfColdSpellIds);
    }

    void ApplyFreeze()
    {
        Unit* target = GetHitUnit();
        Aura* chill = GetHitAura();
        if (!target || !target->IsAlive() || !chill)
            return;

        AuraApplication const* application = chill->GetApplicationOfTarget(target->GetGUID());
        if (!application || !application->HasEffect(EFFECT_0))
            return;

        Unit* caster = GetCaster();
        uint32 freeze = 0;
        if (caster->HasAura(SPELL_MAGE_CONE_OF_COLD_AURA_R2))
            freeze = SPELL_MAGE_CONE_OF_COLD_TRIGGER_R2;
        else if (caster->HasAura(SPELL_MAGE_CONE_OF_COLD_AURA_R1))
            freeze = SPELL_MAGE_CONE_OF_COLD_TRIGGER_R1;

        if (freeze)
            caster->CastSpell(target, freeze, true);
    }

    void Register() override
    {
        AfterHit.Register(&spell_mage_cone_of_cold::ApplyFreeze);
    }
};

// 42955 Conjure Refreshment
/// Updated 4.3.4
struct ConjureRefreshmentData
{
    uint32 minLevel;
    uint32 maxLevel;
    uint32 spellId;
};

ConjureRefreshmentData const _conjureData[] =
{
    { 33, 43, 92739 },
    { 44, 53, 92799 },
    { 54, 63, 92802 },
    { 64, 73, 92805 },
    { 74, 79, 74625 },
    { 80, 84, 92822 },
    { 85, 85, 92727 }
};
uint8 const MAX_CONJURE_REFRESHMENT_SPELLS = std::extent<decltype(_conjureData)>::value;

// 42955 - Conjure Refreshment
class spell_mage_conjure_refreshment : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        for (uint8 i = 0; i < MAX_CONJURE_REFRESHMENT_SPELLS; ++i)
            if (!ValidateSpellInfo({ _conjureData[i].spellId }))
                return false;
        return true;
    }

    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        uint8 level = GetHitUnit()->getLevel();
        for (uint8 i = 0; i < MAX_CONJURE_REFRESHMENT_SPELLS; ++i)
        {
            ConjureRefreshmentData const& spellData = _conjureData[i];
            if (level < spellData.minLevel || level > spellData.maxLevel)
                continue;
            GetHitUnit()->CastSpell(GetHitUnit(), spellData.spellId);
            break;
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_mage_conjure_refreshment::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// -31661 - Dragon's Breath
class spell_mage_dragon_breath : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        DamageInfo const* damage = eventInfo.GetDamageInfo();
        if (!damage || !damage->GetDamage() || damage->GetDamageType() == DOT)
            return false;

        // Living Bomb's explosion is also exempt from breaking this disorient.
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        return !spellInfo || spellInfo->Id != SPELL_MAGE_LIVING_BOMB_EXPLOSION;
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_dragon_breath::CheckProc);
    }
};

// 54646 - Focus Magic
class spell_mage_focus_magic : public AuraScript
{
    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        if (Unit* caster = GetCaster())
            if (caster->IsAlive())
                caster->CastSpell(caster, GetSpellInfo()->Effects[EFFECT_1].TriggerSpell, aurEff);
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_mage_focus_magic::HandleProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 116 - Frostbolt
/// Updated 4.3.4
class spell_mage_frostbolt : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                 SPELL_MAGE_EARLY_FROST_R1,
                 SPELL_MAGE_EARLY_FROST_R2,
                 SPELL_MAGE_EARLY_FROST_TRIGGERED_R1,
                 SPELL_MAGE_EARLY_FROST_TRIGGERED_R2,
                 SPELL_MAGE_EARLY_FROST_VISUAL
            });
    }

    void HandleEarlyFrost()
    {
        Unit* caster = GetCaster();
        if (caster->HasAura(SPELL_MAGE_EARLY_FROST_TRIGGERED_R1) || caster->HasAura(SPELL_MAGE_EARLY_FROST_TRIGGERED_R2))
            return;

        uint32 cooldown = 0;
        if (caster->HasAura(SPELL_MAGE_EARLY_FROST_R2))
            cooldown = SPELL_MAGE_EARLY_FROST_TRIGGERED_R2;
        else if (caster->HasAura(SPELL_MAGE_EARLY_FROST_R1))
            cooldown = SPELL_MAGE_EARLY_FROST_TRIGGERED_R1;

        if (cooldown)
        {
            caster->CastSpell(caster, cooldown, true);
            caster->RemoveAurasDueToSpell(SPELL_MAGE_EARLY_FROST_VISUAL);
        }
    }

    void Register() override
    {
        OnSpellStart.Register(&spell_mage_frostbolt::HandleEarlyFrost);
    }
};

// 45438 - Ice Block
class spell_mage_ice_block : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_HYPOTHERMIA, SPELL_MAGE_FROST_NOVA });
    }

    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Hypothermia debuff
        GetTarget()->CastSpell(nullptr, SPELL_MAGE_HYPOTHERMIA, true);

        // Glyph of Ice Block
        if (GetTarget()->GetDummyAuraEffect(SPELLFAMILY_MAGE, ICON_MAGE_GLYPH_OF_ICE_BLOCK, EFFECT_0))
            GetTarget()->GetSpellHistory()->ResetCooldown(SPELL_MAGE_FROST_NOVA, true);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_mage_ice_block::AfterApply, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

// 12472 - Icy Veins
class spell_mage_icy_veins : public AuraScript
{
    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTarget()->GetDummyAuraEffect(SPELLFAMILY_MAGE, ICON_MAGE_GLYPH_OF_ICY_VEINS, EFFECT_0))
        {
            GetTarget()->RemoveAurasByType(SPELL_AURA_HASTE_SPELLS, ObjectGuid::Empty, 0, true, false);
            GetTarget()->RemoveAurasByType(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, ObjectGuid::Empty, nullptr, true, false);
            GetTarget()->RemoveAurasByType(SPELL_AURA_MOD_DECREASE_SPEED, ObjectGuid::Empty, nullptr, true, false);
        }
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::Expired))
            GetTarget()->RemoveAurasDueToSpell(SPELL_MAGE_STOLEN_TIME);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_mage_icy_veins::AfterApply, EFFECT_0, SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        AfterEffectRemove.Register(&spell_mage_icy_veins::AfterRemove, EFFECT_0, SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, AURA_EFFECT_HANDLE_REAL);
    }
};

// 44457 - Living Bomb
class spell_mage_living_bomb : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ uint32(spellInfo->Effects[EFFECT_1].CalcValue()) });
    }

    void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        // DBC 15595 (44461): explota al expirar "o si el hechizo se disipa" (ByEnemySpell = disipar en un aura negativa)
        EnumFlag<AuraRemoveFlags> const removeMode = GetTargetApplication()->GetRemoveMode();
        if (!removeMode.HasFlag(AuraRemoveFlags::Expired) && !removeMode.HasFlag(AuraRemoveFlags::ByEnemySpell))
            return;

        if (Unit* caster = GetCaster())
            caster->CastSpell(GetTarget(), uint32(aurEff->GetAmount()), aurEff);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_mage_living_bomb::AfterRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 11426 - Ice Barrier
/// Updated 4.3.4
class spell_mage_ice_barrier : public AuraScript
{
    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        if (Unit* caster = GetCaster())
        {
            amount += int32(0.87f * caster->SpellBaseHealingBonusDone(GetSpellInfo()->GetSchoolMask()));

            // Glyph of Ice Barrier: absorb increased by 30%
            if (AuraEffect const* glyph = caster->GetAuraEffect(63095, EFFECT_0))
                AddPct(amount, glyph->GetAmount());
        }
    }

    void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        // Barrera destrozada: solo cuando el escudo se agota. ByEnemySpell también lo usan disipar y Robar hechizo,
        // y entonces el escudo aún tiene absorción restante (no debe congelar).
        if (!GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::ByEnemySpell) || aurEff->GetAmount() > 0)
            return;

        if (GetTarget()->HasAura(SPELL_MAGE_SHATTERED_BARRIER_R1))
            GetTarget()->CastSpell(GetTarget(), SPELL_MAGE_SHATTERED_BARRIER_FREEZE_R1, true);
        else if (GetTarget()->HasAura(SPELL_MAGE_SHATTERED_BARRIER_R2))
            GetTarget()->CastSpell(GetTarget(), SPELL_MAGE_SHATTERED_BARRIER_FREEZE_R2, true);
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_mage_ice_barrier::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        AfterEffectRemove.Register(&spell_mage_ice_barrier::AfterRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
    }
};

// -11119 - Ignite
class spell_mage_ignite : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_IGNITE,
                SPELL_MAGE_FLAME_ORB_DAMAGE,
                SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R1,
                SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R2
            });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (!eventInfo.GetSpellInfo() || !eventInfo.GetProcTarget())
            return false;

        switch (eventInfo.GetSpellInfo()->Id)
        {
            case SPELL_MAGE_FLAME_ORB_DAMAGE:
            case SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R1:
            case SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R2:
                return false;
        }

        return true;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* target = GetTarget();

        DamageInfo* damage = eventInfo.GetDamageInfo();

        int32 bp = CalculatePct(damage->GetDamage(), aurEff->GetAmount()) * 0.5f;
        if (bp > 0)
            target->CastSpell(eventInfo.GetProcTarget(), SPELL_MAGE_IGNITE, CastSpellExtraArgs(aurEff).AddSpellBP0(bp));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_ignite::CheckProc);
        OnEffectProc.Register(&spell_mage_ignite::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 12654 - Ignite
class spell_mage_ignite_periodic : public AuraScript
{
    void CalculateRefreshedDot(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        _critDamageValues.emplace_back(std::make_pair(2, amount));

        amount = 0;
        for (auto const& damageValuePair : _critDamageValues)
            amount += damageValuePair.second;
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        int32 newDotValue = 0;
        bool changed = false;
        for (std::vector<std::pair<uint8, int32>>::iterator itr = _critDamageValues.begin(); itr != _critDamageValues.end();)
        {
            --itr->first;
            newDotValue += itr->second;
            if (itr->first == 0)
            {
                itr = _critDamageValues.erase(itr);
                changed = true;
            }
            else
                ++itr;
        }

        if (changed)
            const_cast<AuraEffect*>(aurEff)->SetAmount(newDotValue);
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_mage_ignite_periodic::CalculateRefreshedDot, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        OnEffectPeriodic.Register(&spell_mage_ignite_periodic::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
private:
    std::vector<std::pair<uint8, int32>> _critDamageValues;
};

static AuraEffect const* GetIncantersAbsorptionTalent(Unit const* target)
{
    if (AuraEffect const* talent = target->GetAuraEffect(SPELL_MAGE_INCANTERS_ABSORPTION_RANK_2, EFFECT_0))
        return talent;

    return target->GetAuraEffect(SPELL_MAGE_INCANTERS_ABSORPTION_RANK_1, EFFECT_0);
}

static void GrantIncantersAbsorption(Unit* target, uint32 absorbAmount)
{
    if (AuraEffect const* talent = GetIncantersAbsorptionTalent(target))
        if (int32 bonus = CalculatePct(absorbAmount, talent->GetAmount()))
            target->CastSpell(target, SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED,
                CastSpellExtraArgs(talent).AddSpellBP0(bonus));
}

// 543 - Mage Ward
/// Updated 4.3.4
class spell_mage_mage_ward : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED });
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        if (Unit* caster = GetCaster())
        {
            // ${$m1+0.807*$SPA}
            float bonus = 0.807f * caster->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ARCANE);
            amount += int32(bonus);
        }
    }

    void HandleAbsorb(AuraEffect* /*aurEff*/, DamageInfo& /*dmgInfo*/, uint32& absorbAmount)
    {
        GrantIncantersAbsorption(GetTarget(), absorbAmount);
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_mage_mage_ward::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        AfterEffectAbsorb.Register(&spell_mage_mage_ward::HandleAbsorb, EFFECT_0);
    }
};

// 1463 - Mana Shield
/// Updated 4.3.4
class spell_mage_mana_shield : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED,
                SPELL_MAGE_INCANTERS_ABSORBTION_KNOCKBACK
            });
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        if (Unit* caster = GetCaster())
        {
            // 80.7% of spell power.
            float bonus = 0.807f * caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask());
            amount += int32(bonus);
        }
    }

    void HandleAbsorb(AuraEffect* /*aurEff*/, DamageInfo& /*dmgInfo*/, uint32& absorbAmount)
    {
        GrantIncantersAbsorption(GetTarget(), absorbAmount);
    }

    void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        // Solo al agotarse el escudo: disipar o robar también usan ByEnemySpell pero dejan absorción restante.
        if (GetIncantersAbsorptionTalent(GetTarget()))
            if (GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::ByEnemySpell) && aurEff->GetAmount() <= 0)
                GetTarget()->CastSpell(GetTarget(), SPELL_MAGE_INCANTERS_ABSORBTION_KNOCKBACK, true);
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_mage_mana_shield::CalculateAmount, EFFECT_0, SPELL_AURA_MANA_SHIELD);
        AfterEffectManaShield.Register(&spell_mage_mana_shield::HandleAbsorb, EFFECT_0);
        AfterEffectRemove.Register(&spell_mage_mana_shield::AfterRemove, EFFECT_0, SPELL_AURA_MANA_SHIELD, AURA_EFFECT_HANDLE_REAL);
    }
};

// -29074 - Master of Elements
class spell_mage_master_of_elements : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_MASTER_OF_ELEMENTS_ENERGIZE });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spell = eventInfo.GetSpellInfo();
        return eventInfo.GetDamageInfo() && spell && spell->PowerType == POWER_MANA && spell->SpellPowerId;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        SpellInfo const* spell = eventInfo.GetSpellInfo();
        int32 baseCost = int32(spell->ManaCost);
        if (spell->ManaCostPercentage)
        {
            float percent = spell->ManaCostPercentage2 != 0.0f ? spell->ManaCostPercentage2 : float(spell->ManaCostPercentage);
            baseCost += int32(CalculatePct(float(GetTarget()->GetCreateMana()), percent));
        }
        if (spell->SpellScalingId)
            baseCost = int32(std::ceil(spell->GetSpellScalingMultiplier(GetTarget()->getLevel(), true) * baseCost));
        else
            baseCost += int32(spell->ManaCostPerlevel * spell->SpellLevel);

        if (int32 mana = CalculatePct(std::max(0, baseCost), aurEff->GetAmount()))
            GetTarget()->CastSpell(GetTarget(), SPELL_MAGE_MASTER_OF_ELEMENTS_ENERGIZE, CastSpellExtraArgs(aurEff).AddSpellBP0(mana));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_master_of_elements::CheckProc);
        OnEffectProc.Register(&spell_mage_master_of_elements::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// -86181 - Nether Vortex
class spell_mage_nether_vortex : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_SLOW });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spell = eventInfo.GetSpellInfo();
        DamageInfo const* damage = eventInfo.GetDamageInfo();
        return spell && spell->Id == SPELL_MAGE_ARCANE_BLAST && damage && damage->GetDamage() && eventInfo.GetProcTarget()
            && !GetTarget()->HasSingleCastAuraOfSpell(SPELL_MAGE_SLOW);
    }

    void HandleEffectProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_MAGE_SLOW, aurEff);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_nether_vortex::CheckProc);
        OnEffectProc.Register(&spell_mage_nether_vortex::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// -11175 - Permafrost
class spell_mage_permafrost : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_PERMAFROST_HEAL
            });
    }

    bool DoCheck(ProcEventInfo& eventInfo)
    {
        DamageInfo const* damage = eventInfo.GetDamageInfo();
        return eventInfo.GetSpellInfo() && damage && damage->GetDamage() && GetTarget()->GetGuardianPet();
    }

    void HandleEffectProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* target = GetTarget();
        int32 heal = int32(CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), aurEff->GetAmount()));
        if (heal > 0)
            target->CastSpell(nullptr, SPELL_MAGE_PERMAFROST_HEAL, CastSpellExtraArgs(aurEff).AddSpellBP0(heal));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_permafrost::DoCheck);
        OnEffectProc.Register(&spell_mage_permafrost::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 118 - Polymorph
class spell_mage_polymorph : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_IMPROVED_POLYMORPH_RANK_1,
                SPELL_MAGE_IMPROVED_POLYMORPH_RANK_2,
                SPELL_MAGE_IMPROVED_POLYMORPH_STUN_RANK_1,
                SPELL_MAGE_IMPROVED_POLYMORPH_STUN_RANK_2,
                SPELL_MAGE_IMPROVED_POLYMORPH_MARKER
            });
    }

    void HandleGlyphEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Glyph of Polymorph (56375): "Your Polymorph spell also removes all damage over time effects from the target."
        // Improved Polymorph and the glyph share the same dummy icon, so looking the aura up by icon could return the
        // talent first and skip the glyph for mages that have both (#206 comment). Check the glyph itself.
        if (Unit* caster = GetCaster())
        {
            if (caster->HasAura(SPELL_MAGE_GLYPH_OF_POLYMORPH))
            {
                GetTarget()->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                GetTarget()->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                GetTarget()->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH);
            }
        }
    }

    void HandleEffectProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        Unit* caster = GetCaster();
        DamageInfo const* damage = eventInfo.GetDamageInfo();
        if (!caster || caster->HasAura(SPELL_MAGE_IMPROVED_POLYMORPH_MARKER) || !GetTarget()->IsAlive()
            || !damage || !damage->GetDamage() || int64(damage->GetDamage()) < aurEff->GetAmount())
            return;

        uint32 stun = 0;
        if (caster->HasAura(SPELL_MAGE_IMPROVED_POLYMORPH_RANK_2))
            stun = SPELL_MAGE_IMPROVED_POLYMORPH_STUN_RANK_2;
        else if (caster->HasAura(SPELL_MAGE_IMPROVED_POLYMORPH_RANK_1))
            stun = SPELL_MAGE_IMPROVED_POLYMORPH_STUN_RANK_1;

        // Let the core remove Polymorph for the same damage event.
        if (stun)
        {
            caster->CastSpell(GetTarget(), stun, aurEff);
            caster->CastSpell(nullptr, SPELL_MAGE_IMPROVED_POLYMORPH_MARKER, aurEff);
        }
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_mage_polymorph::HandleGlyphEffect, EFFECT_0, SPELL_AURA_MOD_CONFUSE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        OnEffectProc.Register(&spell_mage_polymorph::HandleEffectProc, EFFECT_0, SPELL_AURA_MOD_CONFUSE);
    }
};

// 5405  - Replenish Mana (Mana Gem)
/// Updated 4.3.4
class spell_mage_replenish_mana : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_IMPROVED_MANA_GEM_TRIGGERED });
    }

    void HandleImprovedManaGem()
    {
        if (AuraEffect* aurEff = GetCaster()->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_MAGE, ICON_MAGE_IMPROVED_MANA_GEM, EFFECT_0))
        {
            int32 bp = CalculatePct(GetCaster()->GetMaxPower(POWER_MANA), aurEff->GetAmount());
            GetCaster()->CastSpell(GetCaster(), SPELL_MAGE_IMPROVED_MANA_GEM_TRIGGERED, CastSpellExtraArgs(true).AddSpellBP0(bp).AddSpellMod(SPELLVALUE_BASE_POINT1, bp));
        }
    }

    void Register() override
    {
        AfterCast.Register(&spell_mage_replenish_mana::HandleImprovedManaGem);
    }
};

// 82676 - Ring of Frost
/// Updated 4.3.4
class spell_mage_ring_of_frost : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_RING_OF_FROST_SUMMON,
                SPELL_MAGE_RING_OF_FROST_FREEZE,
                SPELL_MAGE_RING_OF_FROST_DUMMY
            });
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        PreventDefaultAction();
        Creature* creature = ObjectAccessor::GetCreature(*GetTarget(), _ringGuid);
        TempSummon* ring = creature ? creature->ToTempSummon() : nullptr;
        if (ring && ring->IsAlive() && GetMaxDuration() - int32(ring->GetTimer()) >= sSpellMgr->AssertSpellInfo(SPELL_MAGE_RING_OF_FROST_DUMMY)->GetDuration())
            GetTarget()->CastSpell(ring->GetPosition(), SPELL_MAGE_RING_OF_FROST_FREEZE, true);
    }

    void Apply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        std::list<Creature*> minions;
        GetTarget()->GetAllMinionsByEntry(minions, GetSpellInfo()->Effects[EFFECT_0].MiscValue);
        TempSummon* newest = nullptr;
        for (Creature* minion : minions)
        {
            TempSummon* ring = minion->ToTempSummon();
            if (ring && (!newest || ring->GetGUID().GetCounter() > newest->GetGUID().GetCounter()))
                newest = ring;
        }

        _ringGuid = newest ? newest->GetGUID() : ObjectGuid::Empty;
        for (Creature* minion : minions)
            if (TempSummon* ring = minion->ToTempSummon(); ring && ring != newest)
                ring->DespawnOrUnsummon();
    }

    ObjectGuid _ringGuid;

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_mage_ring_of_frost::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        OnEffectApply.Register(&spell_mage_ring_of_frost::Apply, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

// 82691 - Ring of Frost (freeze efect)
/// Updated 4.3.4
class spell_mage_ring_of_frost_freeze : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_RING_OF_FROST_SUMMON, SPELL_MAGE_RING_OF_FROST_FREEZE, SPELL_MAGE_RING_OF_FROST_DUMMY });
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        WorldLocation const* center = GetExplTargetDest();
        if (!center)
        {
            targets.clear();
            return;
        }

        float const outerRadius = sSpellMgr->AssertSpellInfo(SPELL_MAGE_RING_OF_FROST_SUMMON)->Effects[EFFECT_0].CalcRadius();
        targets.remove_if([center, outerRadius](WorldObject const* object)
        {
            Unit const* unit = object->ToUnit();
            return !unit || unit->HasAura(SPELL_MAGE_RING_OF_FROST_DUMMY) || unit->HasAura(SPELL_MAGE_RING_OF_FROST_FREEZE)
                || unit->GetExactDist(center) > outerRadius || unit->GetExactDist(center) < 4.7f;
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_mage_ring_of_frost_freeze::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
    }
};

class spell_mage_ring_of_frost_freeze_AuraScript : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_RING_OF_FROST_DUMMY });
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (!GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::Expired))
            if (GetCaster())
                GetCaster()->CastSpell(GetTarget(), SPELL_MAGE_RING_OF_FROST_DUMMY, true);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_mage_ring_of_frost_freeze_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
    }
};

// 66 - Invisibility fade
class spell_mage_invisibility_fade : public AuraScript
{
    void ReduceThreat(AuraEffect const* /*effect*/)
    {
        PreventDefaultAction();
        for (auto const& entry : GetTarget()->GetThreatManager().GetThreatenedByMeList())
            entry.second->ModifyThreatByPercent(-10);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_mage_invisibility_fade::ReduceThreat, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

// 32612 - Full invisibility
class spell_mage_invisibility : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_GLYPH_OF_INVISIBILITY, SPELL_MAGE_INVISIBILITY_SPEED });
    }

    void ApplyGlyph(AuraEffect const* effect, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (target->HasAura(SPELL_MAGE_GLYPH_OF_INVISIBILITY))
            target->CastSpell(target, SPELL_MAGE_INVISIBILITY_SPEED, CastSpellExtraArgs(effect)
                .AddSpellMod(SPELLVALUE_DURATION, GetDuration()));
    }

    void RemoveGlyph(AuraEffect const* /*effect*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_MAGE_INVISIBILITY_SPEED);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_mage_invisibility::ApplyGlyph, EFFECT_1, SPELL_AURA_MOD_INVISIBILITY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        AfterEffectRemove.Register(&spell_mage_invisibility::RemoveGlyph, EFFECT_1, SPELL_AURA_MOD_INVISIBILITY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 80353 - Time Warp
class spell_mage_time_warp : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_TEMPORAL_DISPLACEMENT,
                SPELL_HUNTER_INSANITY,
                SPELL_SHAMAN_EXHAUSTION,
                SPELL_SHAMAN_SATED
            });
    }

    void RemoveInvalidTargets(std::list<WorldObject*>& targets)
    {
        targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_MAGE_TEMPORAL_DISPLACEMENT));
        targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_HUNTER_INSANITY));
        targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_SHAMAN_EXHAUSTION));
        targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_SHAMAN_SATED));
    }

    void ApplyDebuff()
    {
        if (Unit* target = GetHitUnit())
            if (target->HasAura(GetSpellInfo()->Id, GetCaster()->GetGUID()))
                target->CastSpell(target, SPELL_MAGE_TEMPORAL_DISPLACEMENT, true);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_mage_time_warp::RemoveInvalidTargets, EFFECT_ALL, TARGET_UNIT_CASTER_AREA_RAID);
        AfterHit.Register(&spell_mage_time_warp::ApplyDebuff);
    }
};

// 105788 - Mage T13 two-piece bonus
class spell_mage_stolen_time : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spell = eventInfo.GetSpellInfo();
        if (!spell)
            return false;

        switch (spell->Id)
        {
            case SPELL_MAGE_ARCANE_BLAST:
                return true;
            case SPELL_MAGE_FIREBALL:
            case SPELL_MAGE_FROSTFIRE_BOLT:
            case SPELL_MAGE_PYROBLAST:
            case SPELL_MAGE_PYROBLAST_HOT_STREAK:
            case 116: // Frostbolt
                return roll_chance_i(GetEffect(EFFECT_0)->GetAmount());
            default:
                return false;
        }
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_MAGE_STOLEN_TIME);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_stolen_time::CheckProc);
        AfterEffectRemove.Register(&spell_mage_stolen_time::AfterRemove, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

// 12042 - Arcane Power
class spell_mage_arcane_power : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_GLYPH_OF_ARCANE_POWER });
    }

    void CalculateGlobalCooldownModifier(AuraEffect const* /*effect*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        if (AuraEffect const* glyph = GetUnitOwner()->GetAuraEffect(SPELL_MAGE_GLYPH_OF_ARCANE_POWER, EFFECT_0))
            amount = glyph->GetAmount();
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::Expired))
            GetTarget()->RemoveAurasDueToSpell(SPELL_MAGE_STOLEN_TIME);
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_mage_arcane_power::CalculateGlobalCooldownModifier, EFFECT_2, SPELL_AURA_ADD_FLAT_MODIFIER);
        AfterEffectRemove.Register(&spell_mage_arcane_power::AfterRemove, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
    }
};

// 33395 Water Elemental's Freeze
/// Updated 4.3.4
class spell_mage_water_elemental_freeze : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_FINGERS_OF_FROST });
    }

    void HandleImprovedFreeze(SpellEffIndex /*effIndex*/)
    {
        if (_rolled)
            return;

        _rolled = true;
        Unit* owner = GetCaster()->GetOwner();
        if (!owner)
            return;

        if (AuraEffect* aurEff = owner->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_MAGE, ICON_MAGE_IMPROVED_FREEZE, EFFECT_0))
        {
            if (roll_chance_i(aurEff->GetAmount()))
                owner->CastSpell(owner, SPELL_MAGE_FINGERS_OF_FROST, CastSpellExtraArgs(true).AddSpellMod(SPELLVALUE_AURA_STACK, 2));
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_mage_water_elemental_freeze::HandleImprovedFreeze, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }

private:
    bool _rolled = false;
};

// 44546, 44548, 44549 - Brain Freeze
class spell_mage_brain_freeze : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spell = eventInfo.GetSpellInfo();
        if (!spell || spell->Id == SPELL_MAGE_FROSTFIRE_BOLT)
            return false;

        if (spell->Id != SPELL_MAGE_BLIZZARD_DAMAGE)
            return true;

        Unit const* victim = eventInfo.GetProcTarget();
        ObjectGuid caster = GetTarget()->GetGUID();
        return victim && (victim->HasAura(SPELL_MAGE_CHILLED_R1, caster) || victim->HasAura(SPELL_MAGE_CHILLED_R2, caster));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_brain_freeze::CheckProc);
    }
};

// 44544 - Fingers of Frost
class spell_mage_fingers_of_frost : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spell = eventInfo.GetSpellInfo();
        return spell && (spell->Id != SPELL_MAGE_FROSTFIRE_BOLT || GetTarget()->HasAura(SPELL_MAGE_BRAIN_FREEZE_TRIGGERED));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_fingers_of_frost::CheckProc);
    }
};

// 79683 Arcane Missiles!
class spell_mage_arcane_missiles_trigger : public AuraScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_ARCANE_MISSILES_AURASTATE });
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
    }

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_MAGE_ARCANE_MISSILES_AURASTATE, true);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAura(SPELL_MAGE_ARCANE_MISSILES_AURASTATE);
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_mage_arcane_missiles_trigger::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        AfterEffectApply.Register(&spell_mage_arcane_missiles_trigger::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove.Register(&spell_mage_arcane_missiles_trigger::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 79684 Offensive State (DND)
class spell_mage_offensive_state_dnd : public AuraScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_ARCANE_MISSILES,
                SPELL_MAGE_HOT_STREAK,
                SPELL_MAGE_BRAIN_FREEZE_R1
            });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Player* player = GetTarget()->ToPlayer();
        SpellInfo const* spell = eventInfo.GetSpellInfo();
        if (!player || !spell || spell->Id == SPELL_MAGE_ARCANE_MISSILES)
            return false;

        // Don't proc when caster does not know Arcane Missiles
        if (!player->HasSpell(SPELL_MAGE_ARCANE_MISSILES))
            return false;

        // Hot Streak will no longer allow Arcane Missiles to proc
        if (player->HasAura(SPELL_MAGE_HOT_STREAK))
            return false;

        // Brain Freeze will no longer allow Arcane Missiles to proc
        if (player->GetAuraOfRankedSpell(SPELL_MAGE_BRAIN_FREEZE_R1))
            return false;

        return true;
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_offensive_state_dnd::CheckProc);
    }
};

class spell_mage_early_frost : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_EARLY_FROST_VISUAL });
    }

    void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (!GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::Expired))
            return;

        Unit* target = GetTarget();
        if (target->IsAlive() && (target->HasAura(SPELL_MAGE_EARLY_FROST_R1) || target->HasAura(SPELL_MAGE_EARLY_FROST_R2)))
            target->CastSpell(target, SPELL_MAGE_EARLY_FROST_VISUAL, aurEff);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_mage_early_frost::AfterRemove, EFFECT_0, SPELL_AURA_ADD_FLAT_MODIFIER, AURA_EFFECT_HANDLE_REAL);
    }
};

// 82731 - Flame Orb
class spell_mage_flame_orb : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_FLAME_ORB_DUMMY,
                SPELL_MAGE_FROSTFIRE_ORB_DUMMY,
                SPELL_MAGE_FLAME_ORB_SUMMON,
                SPELL_MAGE_FROSTFIRE_ORB_SUMMON
            });
    }

    bool Load() override
    {
        dummySpellId = GetSpellInfo()->Id;
        return true;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
        {
            switch (dummySpellId)
            {
                case SPELL_MAGE_FLAME_ORB_DUMMY:
                    caster->CastSpell(caster, SPELL_MAGE_FLAME_ORB_SUMMON, true);
                    break;
                case SPELL_MAGE_FROSTFIRE_ORB_DUMMY:
                    caster->CastSpell(caster, SPELL_MAGE_FROSTFIRE_ORB_SUMMON, true);
                    break;
                default:
                    break;
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_mage_flame_orb::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }

private:
    uint32 dummySpellId;
};

// 82690, 84717 - Orb pulse auras
class spell_mage_orb_pulse : public AuraScript
{
    void CalculatePeriod(AuraEffect const* /*effect*/, bool& /*isPeriodic*/, int32& period)
    {
        period = IN_MILLISECONDS;
    }

    void Register() override
    {
        DoEffectCalcPeriodic.Register(&spell_mage_orb_pulse::CalculatePeriod, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

// 82734 - Flame Orb dummy AOE
class spell_mage_flame_orb_aoe_dummy : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_FLAME_ORB_AOE,
                SPELL_MAGE_FROSTFIRE_ORB_AOE,
                SPELL_MAGE_FLAME_ORB_BEAM_DUMMY,
                SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R1,
                SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R2,
                SPELL_MAGE_FLAME_ORB_DAMAGE,
                SPELL_MAGE_FLAME_ORB_SELF_SNARE,
                SPELL_MAGE_FROSTFIRE_ORB_RANK_R2,
            });
    }

    bool Load() override
    {
        dummySpellId = GetSpellInfo()->Id;
        return true;
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targets.remove_if([](WorldObject const* object)
        {
            Unit const* unit = object->ToUnit();
            return !unit || unit->IsCritter();
        });

        if (targets.empty())
            return;

        targets.sort(Trinity::ObjectDistanceOrderPred(GetCaster(), true));
        targets.resize(1);
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (TempSummon* caster = GetCaster()->ToTempSummon())
            if (Unit* summoner = caster->GetSummoner())
                if (Unit* target = GetHitUnit())
                {
                    switch (dummySpellId)
                    {
                        case SPELL_MAGE_FLAME_ORB_AOE:
                            caster->CastSpell(caster, SPELL_MAGE_FLAME_ORB_SELF_SNARE, true);
                            caster->CastSpell(target, SPELL_MAGE_FLAME_ORB_BEAM_DUMMY, true);
                            summoner->CastSpell(target, SPELL_MAGE_FLAME_ORB_DAMAGE, true);
                            break;
                        case SPELL_MAGE_FROSTFIRE_ORB_AOE:
                            caster->CastSpell(caster, SPELL_MAGE_FLAME_ORB_SELF_SNARE, true);
                            caster->CastSpell(target, SPELL_MAGE_FLAME_ORB_BEAM_DUMMY, true);

                            if (summoner->HasAura(SPELL_MAGE_FROSTFIRE_ORB_RANK_R2))
                                summoner->CastSpell(target, SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R2, true);
                            else
                                summoner->CastSpell(target, SPELL_MAGE_FROSTFIRE_ORB_DAMAGE_R1, true);
                            break;
                        default:
                            break;
                    }
                }
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_mage_flame_orb_aoe_dummy::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
        OnEffectHitTarget.Register(&spell_mage_flame_orb_aoe_dummy::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }

private:
    uint32 dummySpellId;
};

static bool IsHotStreakSpell(ProcEventInfo& eventInfo)
{
    SpellInfo const* spell = eventInfo.GetSpellInfo();
    DamageInfo const* damage = eventInfo.GetDamageInfo();
    if (!spell || !damage || damage->GetDamageType() != SPELL_DIRECT_DAMAGE)
        return false;

    switch (spell->Id)
    {
        case SPELL_MAGE_FIREBALL:
        case SPELL_MAGE_FIRE_BLAST:
        case SPELL_MAGE_FROSTFIRE_BOLT:
        case SPELL_MAGE_PYROBLAST:
        case SPELL_MAGE_PYROBLAST_HOT_STREAK:
        case SPELL_MAGE_SCORCH:
            return true;
        default:
            return false;
    }
}

// 44445 - Hot Streak
class spell_mage_hot_streak : public AuraScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_HOT_STREAK_TRIGGERED,
                SPELL_MAGE_T12_4P_BONUS,
                SPELL_MAGE_IMPROVED_HOT_STREAK
            });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (!IsHotStreakSpell(eventInfo))
            return false;

        if (AuraEffect const* aurEff = GetTarget()->GetDummyAuraEffect(SPELLFAMILY_MAGE, ICON_MAGE_HOT_STREAK, EFFECT_0))
            if (aurEff->GetSpellInfo()->Id == SPELL_MAGE_IMPROVED_HOT_STREAK || aurEff->GetSpellInfo()->GetRank() > 1)
                return false;

        int32 procChance = GetSpellInfo()->Effects[EFFECT_0].CalcValue();
        // T12 4P bonus
        if (AuraEffect const* aurEff = GetTarget()->GetAuraEffect(SPELL_MAGE_T12_4P_BONUS, EFFECT_1))
            procChance += aurEff->GetAmount();

        return roll_chance_i(procChance);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        GetCaster()->CastSpell(GetCaster(), SPELL_MAGE_HOT_STREAK_TRIGGERED, aurEff);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_hot_streak::CheckProc);
        OnEffectProc.Register(&spell_mage_hot_streak::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// -44446 - Improved Hot Streak
class spell_mage_improved_hot_streak : public AuraScript
{
    bool Load() override
    {
        _critCount = 0;
        return GetCaster()->IsPlayer();
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_HOT_STREAK_TRIGGERED });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (!IsHotStreakSpell(eventInfo))
            return false;

        if (eventInfo.GetDamageInfo()->GetHitMask() & PROC_HIT_CRITICAL)
            _critCount++;
        else
            _critCount = 0;

        if (_critCount == 2)
        {
            _critCount = 0;
            return roll_chance_i(GetSpellInfo()->Effects[EFFECT_0].CalcValue());
        }

        return false;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        GetCaster()->CastSpell(GetCaster(), SPELL_MAGE_HOT_STREAK_TRIGGERED, aurEff);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_improved_hot_streak::CheckProc);
        OnEffectProc.Register(&spell_mage_improved_hot_streak::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }

private:
    uint8 _critCount = 0;
};

// 55342 - Mirror Image
class spell_mage_mirror_image : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_MAGE_MIRROR_IMAGE_TRIGGERED_FIRE,
                SPELL_MAGE_MIRROR_IMAGE_TRIGGERED_ARCANE,
                SPELL_MAGE_MIRROR_IMAGE_TRIGGERED_FROST
            });
    }

    void HandleDummyEffect(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetHitPlayer();
        if (!player)
            return;

        uint32 spellId = SPELL_MAGE_MIRROR_IMAGE_TRIGGERED_FROST;
        if (player->GetDummyAuraEffect(SPELLFAMILY_MAGE, ICON_MAGE_GLYPH_OF_MIRROR_IMAGE, EFFECT_0))
        {
            if (player->GetPrimaryTalentTree(player->GetActiveSpec()) == TALENT_TREE_MAGE_FIRE)
                spellId = SPELL_MAGE_MIRROR_IMAGE_TRIGGERED_FIRE;
            else if (player->GetPrimaryTalentTree(player->GetActiveSpec()) == TALENT_TREE_MAGE_ARCANE)
                spellId = SPELL_MAGE_MIRROR_IMAGE_TRIGGERED_ARCANE;
        }

        player->CastSpell(player, spellId, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_mage_mirror_image::HandleDummyEffect, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};

class spell_mage_mirror_image_AurasScript : public AuraScript
{
    void HandleEffectPeriodic(AuraEffect const* aurEff)
    {
        if (aurEff->GetTickNumber() == 1)
            GetTarget()->CastSpell(GetTarget(), GetSpellInfo()->Effects[aurEff->GetEffIndex()].TriggerSpell, true);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_mage_mirror_image_AurasScript::HandleEffectPeriodic, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY);
    }
};

class spell_mage_mirror_image_attack : public SpellScript
{
    void FilterTarget(WorldObject*& target)
    {
        if (target)
            if (Unit const* unit = target->ToUnit(); unit && unit->HasBreakableByDamageCrowdControlAura())
                target = nullptr;
    }

    void Register() override
    {
        OnObjectTargetSelect.Register(&spell_mage_mirror_image_attack::FilterTarget, EFFECT_ALL, TARGET_UNIT_TARGET_ENEMY);
    }
};

class spell_mage_initialize_images : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        ObjectGuid casterGuid = GetCaster()->GetGUID();
        targets.remove_if([casterGuid](WorldObject* object)
        {
            Unit* unit = object->ToUnit();
            TempSummon* summon = unit ? unit->ToTempSummon() : nullptr;
            return !summon || summon->GetSummonerGUID() != casterGuid;
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_mage_initialize_images::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};

// 71761 - Deep Freeze Immunity State
// El proc nativo (spell_proc: máscara de Congelación profunda, HitMask IMMUNE) dispara 71757 ante cualquier inmunidad.
// DBC 15595: el daño solo se aplica a objetivos "permanentemente inmunes a aturdimientos" (criaturas con inmunidad de plantilla),
// no a jugadores con inmunidad temporal (rendimientos decrecientes, Escudo divino, Bloque de hielo...). Antes, además, el script
// de 44572 lanzaba 71757 por su cuenta y las criaturas inmunes recibían el daño dos veces.
class spell_mage_deep_freeze_immunity_state : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Creature const* creature = eventInfo.GetActionTarget() ? eventInfo.GetActionTarget()->ToCreature() : nullptr;
        return creature && (creature->GetCreatureTemplate()->MechanicImmuneMask & (1u << (MECHANIC_STUN - 1)));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_deep_freeze_immunity_state::CheckProc);
    }
};

// -34293 - Pyromaniac
class spell_mage_pyromaniac : public AuraScript
{
    bool Load() override
    {
        _buffActive = false;
        return true;
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_PYROMANIAC_TRIGGERED });
    }

    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        isPeriodic = true;
        amplitude = 1 * IN_MILLISECONDS;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* victim = eventInfo.GetProcTarget();
        if (!victim || !IsValidDotTarget(GetTarget(), victim->GetGUID()))
            return;

        _dotTargetGuids.insert(victim->GetGUID());

        // Pyomaniac is active already so there is no need to iterate through potential targets
        if (_buffActive)
            return;

        Unit* target = GetTarget();
        CleanDotTargets(target);

        if (_dotTargetGuids.size() >= 3)
        {
            target->CastSpell(target, SPELL_MAGE_PYROMANIAC_TRIGGERED, CastSpellExtraArgs(aurEff).AddSpellBP0(aurEff->GetAmount()));
            _buffActive = true;
        }
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        // Buff is not active right now so there is no need to check targets
        if (!_buffActive)
            return;

        Unit* target = GetTarget();
        CleanDotTargets(target);

        if (_dotTargetGuids.size() < 3)
        {
            target->RemoveAurasDueToSpell(SPELL_MAGE_PYROMANIAC_TRIGGERED);
            _buffActive = false;
        }
    }

    void Register() override
    {
        DoEffectCalcPeriodic.Register(&spell_mage_pyromaniac::CalcPeriodic, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc.Register(&spell_mage_pyromaniac::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectPeriodic.Register(&spell_mage_pyromaniac::HandlePeriodic, EFFECT_0, SPELL_AURA_DUMMY);
        AfterEffectRemove.Register(&spell_mage_pyromaniac::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
private:
    GuidSet _dotTargetGuids;
    bool _buffActive;

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_MAGE_PYROMANIAC_TRIGGERED);
    }

    void CleanDotTargets(Unit* caster)
    {
        GuidSet guids = _dotTargetGuids;
        for (ObjectGuid guid : guids)
            if (!IsValidDotTarget(caster, guid))
                _dotTargetGuids.erase(guid);
    }

    bool IsValidDotTarget(Unit* caster, ObjectGuid guid) const
    {
        Unit* target = ObjectAccessor::GetUnit(*caster, guid);
        if (!target || !target->IsAlive())
            return false;

        Unit::AuraEffectList const& dotAuraEffects = target->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
        if (dotAuraEffects.empty())
            return false;

        for (AuraEffect const* effect : dotAuraEffects)
            if (effect->GetCasterGUID() == caster->GetGUID() && effect->GetAmount() > 0
                && effect->GetSpellInfo()->SpellFamilyName == SPELLFAMILY_MAGE
                && (effect->GetSpellInfo()->SpellFamilyFlags[2] & 0x00000008))
                return true;

        return false;
    }
};

// -11103 - Impact
class spell_mage_impact : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_FIRE_BLAST });
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        GetTarget()->GetSpellHistory()->ResetCooldown(SPELL_MAGE_FIRE_BLAST, true);
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_mage_impact::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 12355 - Impact
class spell_mage_impact_triggered : public SpellScript
{
    struct FireDot
    {
        uint32 SpellId;
        int32 Duration;
        std::array<std::optional<int32>, MAX_SPELL_EFFECTS> Amounts;
    };

    void RegisterFireDots(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();
        Unit* launchTarget = GetHitUnit();
        if (!target || !caster || target != launchTarget)
            return;

        Unit::AuraEffectList const& dotAuraEffects = target->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
        if (dotAuraEffects.empty())
            return;

        for (AuraEffect const* effect : dotAuraEffects)
        {
            if (effect->GetCasterGUID() != caster->GetGUID()
                || effect->GetSpellInfo()->SpellFamilyName != SPELLFAMILY_MAGE
                || !(effect->GetSpellInfo()->SpellFamilyFlags[2] & 0x00000008))
                continue;

            Aura const* aura = effect->GetBase();
            if (std::any_of(_fireDots.begin(), _fireDots.end(), [aura](FireDot const& dot) { return dot.SpellId == aura->GetId(); }))
                continue;

            FireDot dot{ aura->GetId(), aura->GetDuration(), {} };
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (AuraEffect const* original = aura->GetEffect(i))
                    dot.Amounts[i] = original->GetAmount();
            _fireDots.push_back(dot);
        }
    }

    void SpreadFireDots(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (GetExplTargetUnit() == GetHitUnit())
            return;

        for (FireDot const& dot : _fireDots)
        {
            if (Aura* addAura = caster->AddAura(dot.SpellId, GetHitUnit()))
            {
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
                    if (dot.Amounts[i])
                        if (AuraEffect* effect = addAura->GetEffect(i))
                            effect->SetAmount(*dot.Amounts[i]);

                addAura->SetDuration(dot.Duration);
            }
        }
    }

    void Register() override
    {
        OnEffectLaunchTarget.Register(&spell_mage_impact_triggered::RegisterFireDots, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
        OnEffectHitTarget.Register(&spell_mage_impact_triggered::SpreadFireDots, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
private:
    std::vector<FireDot> _fireDots;
};

// 11129 - Combustion
class spell_mage_combustion : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_COMBUSTION_DAMAGE });
    }

    void HandleScriptEffect(SpellEffIndex effIndex)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Unit* target = GetHitUnit();

        float basePoints = 0.0f;
        float scalingFactor = static_cast<float>(GetEffectValue());
        float tickPeriod = sSpellMgr->AssertSpellInfo(SPELL_MAGE_COMBUSTION_DAMAGE)->Effects[EFFECT_0].Amplitude;

        for (AuraEffect const* aurEff : target->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE))
        {
            if (aurEff->GetCasterGUID() != caster->GetGUID()
                || !(aurEff->GetSpellInfo()->GetSchoolMask() & SPELL_SCHOOL_MASK_FIRE)
                || aurEff->GetSpellInfo()->SpellFamilyName != SPELLFAMILY_MAGE)
                continue;

            // Only combine auras specified in the script effect's class mask
            if (!aurEff->GetSpellInfo()->IsAffected(SPELLFAMILY_MAGE, GetSpellInfo()->Effects[effIndex].SpellClassMask))
                continue;

            // Match damage over time before haste changes the tick rate of Combustion itself.
            if (uint32 sourcePeriod = aurEff->GetSpellInfo()->Effects[aurEff->GetEffIndex()].Amplitude)
                basePoints += CalculatePct(static_cast<float>(aurEff->GetAmount()), scalingFactor) * tickPeriod / sourcePeriod;
        }

        if (basePoints <= 0 || caster->CastSpell(target, SPELL_MAGE_COMBUSTION_DAMAGE,
            CastSpellExtraArgs(true).AddSpellBP0(int32(basePoints))) != SPELL_CAST_OK)
            return;

        if (!caster->HasAura(SPELL_MAGE_T13_2P_BONUS))
            return;

        // Use the original cast's duration, so copies made by Impact never restart this timer.
        if (Aura const* combustion = target->GetAura(SPELL_MAGE_COMBUSTION_DAMAGE, caster->GetGUID()))
            if (combustion->GetDuration() > 0)
                caster->m_Events.AddEventAtOffset([caster]() { caster->RemoveAurasDueToSpell(SPELL_MAGE_STOLEN_TIME); },
                    Milliseconds(combustion->GetDuration()));
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_mage_combustion::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 44614 - Frostfire Bolt
class spell_mage_frostfire_bolt : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_GLYPH_OF_FROSTFIRE, SPELL_MAGE_PERMAFROST_RANK_1, SPELL_MAGE_PERMAFROST_REDUCE_HEAL });
    }

    void SelectSlowTarget(WorldObject*& target)
    {
        if (GetCaster()->HasAura(SPELL_MAGE_GLYPH_OF_FROSTFIRE))
            target = nullptr;
    }

    void SelectPeriodicDamageTarget(WorldObject*& target)
    {
        if (!GetCaster()->HasAura(SPELL_MAGE_GLYPH_OF_FROSTFIRE))
            target = nullptr;
    }

    void ApplyPermafrost()
    {
        Unit* target = GetHitUnit();
        Aura* chill = GetHitAura();
        if (!target || !chill)
            return;

        AuraApplication const* application = chill->GetApplicationOfTarget(target->GetGUID());
        if (!application || !application->HasEffect(EFFECT_0))
            return;

        if (AuraEffect const* talent = GetCaster()->GetAuraEffectOfRankedSpell(SPELL_MAGE_PERMAFROST_RANK_1, EFFECT_2))
            GetCaster()->CastSpell(target, SPELL_MAGE_PERMAFROST_REDUCE_HEAL, CastSpellExtraArgs(talent)
                .AddSpellBP0(talent->GetAmount()).AddSpellMod(SPELLVALUE_DURATION, chill->GetDuration()));
    }

    void Register() override
    {
        OnObjectTargetSelect.Register(&spell_mage_frostfire_bolt::SelectSlowTarget, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
        OnObjectTargetSelect.Register(&spell_mage_frostfire_bolt::SelectPeriodicDamageTarget, EFFECT_2, TARGET_UNIT_TARGET_ENEMY);
        AfterHit.Register(&spell_mage_frostfire_bolt::ApplyPermafrost);
    }
};
}

namespace Spells::Mage
{
// 44614 - Frostfire Bolt's glyphed periodic damage.
class spell_mage_frostfire_bolt_periodic : public AuraScript
{
    void CalculateAmount(AuraEffect const* effect, int32& amount, bool& /*canBeRecalculated*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        AuraEffect const* glyphDamage = caster->GetAuraEffect(SPELL_MAGE_GLYPH_OF_FROSTFIRE, EFFECT_2);
        AuraEffect const* glyphDuration = caster->GetAuraEffect(SPELL_MAGE_GLYPH_OF_FROSTFIRE, EFFECT_1);
        if (!glyphDamage || !glyphDuration)
            return;

        SpellInfo const* bolt = GetSpellInfo();
        uint32 period = bolt->Effects[EFFECT_2].Amplitude;
        int32 duration = bolt->GetDuration() + glyphDuration->GetAmount();
        if (!period || duration <= 0)
            return;

        Unit* target = GetUnitOwner();
        // The periodic effect already has its own spell-power coefficient in the DBC.
        float totalBaseDamage = CalculatePct(float(bolt->Effects[EFFECT_1].CalcBaseValue(caster, target)), glyphDamage->GetAmount());
        int32 basePerTick = int32(std::round(totalBaseDamage * period / duration));
        amount = caster->SpellDamageBonusDone(target, bolt, basePerTick, DOT, EFFECT_2, 1, nullptr, effect);
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_mage_frostfire_bolt_periodic::CalculateAmount, EFFECT_2, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

// -83156 - Piercing Chill: "Your Frostbolt criticals apply the chill effect to 1 / 2 additional nearby targets"
class spell_mage_piercing_chill : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        return eventInfo.GetSpellInfo() && eventInfo.GetSpellInfo()->Id == 116 && eventInfo.GetProcTarget();
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* caster = GetTarget();
        Unit* target = eventInfo.GetProcTarget();

        std::list<Unit*> units;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(target, caster, 10.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(target, units, check);
        Cell::VisitAllObjects(target, searcher, 10.0f);

        units.remove(target);
        units.remove_if([caster](Unit* unit) { return !caster->IsWithinLOSInMap(unit) || unit->HasAura(116, caster->GetGUID()); });
        Trinity::Containers::RandomResize(units, std::max(aurEff->GetAmount(), 1));

        // Only the chill (Frostbolt's slow) is applied, not its damage
        for (Unit* unit : units)
            caster->AddAura(116, unit);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_mage_piercing_chill::CheckProc);
        OnEffectProc.Register(&spell_mage_piercing_chill::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};
}

namespace Spells::Mage
{
// -86948 - Cauterize: 50% / 100% chance that an attack which would kill you instead leaves you at 40% health, but you
// burn for 12% of your maximum health every 1.5 sec for 6 sec (87023). Cannot occur more than once per minute.
class spell_mage_cauterize : public AuraScript
{
    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = -1; // unlimited, the absorb handler decides
    }

    void Absorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount)
    {
        Unit* target = GetTarget();
        if (dmgInfo.GetDamage() < target->GetHealth() || target->GetSpellHistory()->HasCooldown(87023))
            return;

        if (!roll_chance_i(GetSpellInfo()->Effects[EFFECT_0].CalcValue()))
            return;

        uint32 health = target->CountPctFromMaxHealth(GetSpellInfo()->Effects[EFFECT_1].CalcValue());
        absorbAmount = dmgInfo.GetDamage();
        target->SetHealth(std::max<uint32>(health, 1));
        target->GetSpellHistory()->AddCooldown(87023, 0, std::chrono::minutes(1));
        target->CastSpell(target, 87023, true);
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_mage_cauterize::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb.Register(&spell_mage_cauterize::Absorb, EFFECT_0);
    }
};
}

namespace Spells::Mage
{
// 48108 Hot Streak, 57761 Brain Freeze, 79683 Arcane Missiles!
// Item - Mage T10 2P Bonus (70752): "Your Hot Streak, Missile Barrage and Brain Freeze talents also grant you 12% haste
// for 5 sec when the talent effect is consumed" (70753 Pushing the Limit). Not when the buff simply expires.
class spell_mage_t10_2p_bonus_consumer : public AuraScript
{
    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::Expired))
            return;

        Unit* target = GetTarget();
        if (target->HasAura(70752))
            target->CastSpell(target, 70753, true);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_mage_t10_2p_bonus_consumer::HandleRemove, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};
}

void AddSC_mage_spell_scripts()
{
    using namespace Spells::Mage;
    RegisterSpellScript(spell_mage_t10_2p_bonus_consumer);
    RegisterSpellScript(spell_mage_cauterize);
    RegisterSpellScript(spell_mage_piercing_chill);
    RegisterSpellScript(spell_mage_arcane_missiles_trigger);
    RegisterSpellScript(spell_mage_arcane_potency);
    RegisterSpellScript(spell_mage_arcane_power);
    RegisterSpellScript(spell_mage_blast_wave);
    RegisterSpellScript(spell_mage_brain_freeze);
    RegisterSpellScript(spell_mage_blazing_speed);
    RegisterSpellScript(spell_mage_reactive_barrier);
    RegisterSpellScript(spell_mage_ritual_of_refreshment);
    RegisterSpellScript(spell_mage_blizzard);
    RegisterSpellScript(spell_mage_cold_snap);
    RegisterSpellScript(spell_mage_combustion);
    RegisterSpellScript(spell_mage_cone_of_cold);
    RegisterSpellScript(spell_mage_conjure_refreshment);
    RegisterSpellScript(spell_mage_deep_freeze_immunity_state);
    RegisterSpellScript(spell_mage_dragon_breath);
    RegisterSpellScript(spell_mage_early_frost);
    RegisterSpellScript(spell_mage_flame_orb);
    RegisterSpellScript(spell_mage_fingers_of_frost);
    RegisterSpellScript(spell_mage_flame_orb_aoe_dummy);
    RegisterSpellScript(spell_mage_orb_pulse);
    RegisterSpellScript(spell_mage_focus_magic);
    RegisterSpellScript(spell_mage_frostbolt);
    RegisterSpellAndAuraScriptPair(spell_mage_frostfire_bolt, spell_mage_frostfire_bolt_periodic);
    RegisterSpellScript(spell_mage_hot_streak);
    RegisterSpellScript(spell_mage_ice_barrier);
    RegisterSpellScript(spell_mage_ice_block);
    RegisterSpellScript(spell_mage_icy_veins);
    RegisterSpellScript(spell_mage_ignite);
    RegisterSpellScript(spell_mage_ignite_periodic);
    RegisterSpellScript(spell_mage_impact);
    RegisterSpellScript(spell_mage_impact_triggered);
    RegisterSpellScript(spell_mage_improved_hot_streak);
    RegisterSpellScript(spell_mage_invisibility);
    RegisterSpellScript(spell_mage_invisibility_fade);
    RegisterSpellScript(spell_mage_initialize_images);
    RegisterSpellScript(spell_mage_living_bomb);
    RegisterSpellScript(spell_mage_mage_ward);
    RegisterSpellScript(spell_mage_mana_shield);
    RegisterSpellScript(spell_mage_master_of_elements);
    RegisterSpellAndAuraScriptPair(spell_mage_mirror_image, spell_mage_mirror_image_AurasScript);
    RegisterSpellScript(spell_mage_mirror_image_attack);
    RegisterSpellScript(spell_mage_nether_vortex);
    RegisterSpellScript(spell_mage_offensive_state_dnd);
    RegisterSpellScript(spell_mage_permafrost);
    RegisterSpellScript(spell_mage_polymorph);
    RegisterSpellScript(spell_mage_pyromaniac);
    RegisterSpellScript(spell_mage_replenish_mana);
    RegisterSpellScript(spell_mage_ring_of_frost);
    RegisterSpellScript(spell_mage_stolen_time);
    RegisterSpellAndAuraScriptPair(spell_mage_ring_of_frost_freeze, spell_mage_ring_of_frost_freeze_AuraScript);
    RegisterSpellScript(spell_mage_time_warp);
    RegisterSpellScript(spell_mage_water_elemental_freeze);
}
