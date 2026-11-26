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
 * Scripts for spells with SPELLFAMILY_WARRIOR and SPELLFAMILY_GENERIC spells used by warrior players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_warr_".
 */

#include "ScriptMgr.h"
#include "CellImpl.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "Random.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "GridNotifiersImpl.h"

namespace Spells::Warrior
{
enum WarriorSpells
{
    SPELL_WARRIOR_BATTLE_TRANCE_TRIGGERED           = 12964,
    SPELL_WARRIOR_BLADESTORM_PERIODIC_WHIRLWIND     = 50622,
    SPELL_WARRIOR_BLOODTHIRST                       = 23885,
    SPELL_WARRIOR_BLOODTHIRST_DAMAGE                = 23881,
    SPELL_WARRIOR_BLOODSURGE_R1                     = 46913,
    SPELL_WARRIOR_CHARGE_ENERGIZE                   = 34846,
    SPELL_WARRIOR_CHARGE_STUN                       = 7922,
    SPELL_WARRIOR_COLOSSUS_SMASH                    = 86346,
    SPELL_WARRIOR_DEADLY_CALM                       = 85730,
    SPELL_WARRIOR_DEEP_WOUNDS_PERIODIC              = 12721,
    SPELL_WARRIOR_EXECUTE                           = 5308,
    SPELL_WARRIOR_INCITE_TRIGGER                    = 86627,
    SPELL_WARRIOR_IMPROVED_HAMSTRING                = 12289,
    SPELL_WARRIOR_IMPENDING_VICTORY                 = 80128,
    SPELL_WARRIOR_INTERCEPT                         = 20252,
    SPELL_WARRIOR_JUGGERNAUT_CRIT_BONUS_BUFF        = 65156,
    SPELL_WARRIOR_JUGGERNAUT_CRIT_BONUS_TALENT      = 64976,
    SPELL_WARRIOR_LAST_STAND_TRIGGERED              = 12976,
    SPELL_WARRIOR_MORTAL_STRIKE                     = 12294,
    SPELL_WARRIOR_OPPORTUNITY_STRIKE                = 76858,
    SPELL_WARRIOR_RALLYING_CRY                      = 97463,
    SPELL_WARRIOR_RECKLESSNESS                      = 1719,
    SPELL_WARRIOR_REND                              = 94009,
    SPELL_WARRIOR_RETALIATION_DAMAGE                = 22858,
    SPELL_WARRIOR_SHIELD_OF_FURY                    = 105909,
    SPELL_WARRIOR_SHIELD_FORTRESS                   = 105914,
    SPELL_WARRIOR_T13_PROTECTION_2P                 = 105908,
    SPELL_WARRIOR_T13_PROTECTION_4P                 = 105911,
    SPELL_WARRIOR_SECOND_WIND_PROC_RANK_1           = 29834,
    SPELL_WARRIOR_SECOND_WIND_PROC_RANK_2           = 29838,
    SPELL_WARRIOR_SECOND_WIND_TRIGGER_RANK_1        = 29841,
    SPELL_WARRIOR_SECOND_WIND_TRIGGER_RANK_2        = 29842,
    SPELL_WARRIOR_SHIELD_SLAM                       = 23922,
    SPELL_WARRIOR_SLAM_MAIN_HAND                    = 50782,
    SPELL_WARRIOR_SLAM_OFF_HAND                     = 97992,
    SPELL_WARRIOR_SUDDEN_DEATH                      = 29723,
    SPELL_WARRIOR_SUNDER_ARMOR                      = 58567,
    SPELL_WARRIOR_SWEEPING_STRIKES_EXTRA_ATTACK_1   = 12723,
    SPELL_WARRIOR_SWEEPING_STRIKES_EXTRA_ATTACK_2   = 26654,
    SPELL_WARRIOR_TAUNT                             = 355,
    SPELL_WARRIOR_UNRELENTING_ASSAULT_RANK_1        = 46859,
    SPELL_WARRIOR_UNRELENTING_ASSAULT_RANK_2        = 46860,
    SPELL_WARRIOR_UNRELENTING_ASSAULT_TRIGGER_1     = 64849,
    SPELL_WARRIOR_UNRELENTING_ASSAULT_TRIGGER_2     = 64850,
    SPELL_WARRIOR_VICTORIOUS                        = 32216,
    SPELL_WARRIOR_VICTORIOUS_IMPENDING_VICTORY      = 82368,
    SPELL_WARRIOR_VIGILANCE_PROC                    = 50725,
    SPELL_WARRIOR_VENGEANCE                         = 76691
};

enum WarriorSpellIcons
{
    WARRIOR_ICON_ID_GLYPH_OF_COLOSSUS_SMASH         = 5288,
    WARRIOR_ICON_ID_BLOOD_AND_THUNDER               = 5057,
    WARRIOR_ICON_ID_SINGLE_MINDED_FURY              = 4975
};

/// Updated 4.3.4
// 23881 - Bloodthirst
class spell_warr_bloodthirst : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_BLOODTHIRST });
    }

    void CalculateDamage(Unit* /*victim*/, int32& damage, int32& /*flatMod*/, float& /*pctMod*/)
    {
        damage = CalculatePct(GetCaster()->GetTotalAttackPowerValue(BASE_ATTACK), damage);
    }

    void HandleBloodthirstHealEffect()
    {
        GetCaster()->CastSpell(nullptr, SPELL_WARRIOR_BLOODTHIRST, true);
    }

    void Register() override
    {
        CalcDamage.Register(&spell_warr_bloodthirst::CalculateDamage);
        AfterHit.Register(&spell_warr_bloodthirst::HandleBloodthirstHealEffect);
    }
};

/// Updated 4.3.4
// 23880 - Bloodthirst
class spell_warr_bloodthirst_heal : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_BLOODTHIRST_DAMAGE });
    }

    void HandleHeal(SpellEffIndex /*effIndex*/)
    {
        SpellInfo const* spellInfo = sSpellMgr->AssertSpellInfo(SPELL_WARRIOR_BLOODTHIRST_DAMAGE);
        SetEffectValue(GetCaster()->CountPctFromMaxHealth(spellInfo->Effects[EFFECT_1].CalcValue(GetCaster())) / 1000);
    }

    void Register() override
    {
        OnEffectLaunchTarget.Register(&spell_warr_bloodthirst_heal::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
    }
};

/// Updated 4.3.4
class spell_warr_charge : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_WARRIOR_JUGGERNAUT_CRIT_BONUS_TALENT,
                SPELL_WARRIOR_JUGGERNAUT_CRIT_BONUS_BUFF,
                SPELL_WARRIOR_CHARGE_ENERGIZE,
                SPELL_WARRIOR_CHARGE_STUN
            });
    }

    void HandleStun(SpellEffIndex /*effIdex*/)
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(GetHitUnit(), SPELL_WARRIOR_CHARGE_STUN, true);
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        caster->CastSpell(caster, SPELL_WARRIOR_CHARGE_ENERGIZE, { SPELLVALUE_BASE_POINT0, GetEffectValue() });

        // Juggernaut crit bonus
        if (caster->HasAura(SPELL_WARRIOR_JUGGERNAUT_CRIT_BONUS_TALENT))
            caster->CastSpell(caster, SPELL_WARRIOR_JUGGERNAUT_CRIT_BONUS_BUFF, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_warr_charge::HandleStun, EFFECT_0, SPELL_EFFECT_CHARGE);
        OnEffectHitTarget.Register(&spell_warr_charge::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};

/// Updated 4.3.4
// 12809 - Concussion Blow
class spell_warr_concussion_blow : public SpellScript
{
    void CalculateDamage(SpellEffIndex /*effIndex*/)
    {
        SetEffectValue(CalculatePct(GetCaster()->GetTotalAttackPowerValue(BASE_ATTACK), GetSpellInfo()->Effects[EFFECT_2].CalcValue(GetCaster())));
    }

    void Register() override
    {
        OnEffectLaunchTarget.Register(&spell_warr_concussion_blow::CalculateDamage, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 86346 - Colossus Smash
class spell_warr_colossus_smash : public SpellScript
{
    void HandleGlyphEffect(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            if (AuraEffect const* aurEff = caster->GetDummyAuraEffect(SPELLFAMILY_WARRIOR, WARRIOR_ICON_ID_GLYPH_OF_COLOSSUS_SMASH, EFFECT_0))
                caster->CastSpell(GetHitUnit(), SPELL_WARRIOR_SUNDER_ARMOR, aurEff);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_warr_colossus_smash::HandleGlyphEffect, EFFECT_0, SPELL_EFFECT_NORMALIZED_WEAPON_DMG);
    }
};

class spell_warr_colossus_smash_aura : public AuraScript
{
    void CalculateArmorBypass(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // Patch 4.1 halved the armor bypass against players only.
        if (GetUnitOwner()->IsPlayer())
            amount /= 2;
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_warr_colossus_smash_aura::CalculateArmorBypass, EFFECT_FIRST_FOUND, SPELL_AURA_BYPASS_ARMOR_FOR_CASTER);
    }
};

// 23922 - Shield Slam (T12 Protection 2P: 99239 -> Entrar en combustión 99240, 20% del daño como Fuego en 4 s)
class spell_warr_shield_slam_t12 : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ 99239, 99240 });
    }

    void HandleAfterHit()
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!target || GetHitDamage() <= 0)
            return;
        if (AuraEffect const* bonus = caster->GetAuraEffect(99239, EFFECT_0))
        {
            SpellInfo const* combust = sSpellMgr->AssertSpellInfo(99240);
            int32 ticks = std::max<int32>(1, combust->GetMaxDuration() / std::max<int32>(1, combust->Effects[EFFECT_0].Amplitude));
            int32 perTick = CalculatePct(GetHitDamage(), bonus->GetAmount()) / ticks;
            if (perTick > 0)
                caster->CastSpell(target, 99240, CastSpellExtraArgs(bonus).AddSpellBP0(perTick));
        }
    }

    void Register() override
    {
        AfterHit.Register(&spell_warr_shield_slam_t12::HandleAfterHit);
    }
};

// 2565 - Shield Block (T12 Protection 4P: 99242 -> Muro de llamas 99243 al expirar)
class spell_warr_shield_block_t12 : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ 99242, 99243 });
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (!GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::Expired))
            return;
        Unit* target = GetTarget();
        if (target->HasAura(99242))
            target->CastSpell(target, 99243, true);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_warr_shield_block_t12::AfterRemove, EFFECT_0, SPELL_AURA_MOD_BLOCK_PERCENT, AURA_EFFECT_HANDLE_REAL);
    }
};

// 6572 - Revenge
class spell_warr_revenge : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_T13_PROTECTION_2P, SPELL_WARRIOR_SHIELD_OF_FURY });
    }

    void ApplyShield()
    {
        if (!GetHitUnit() || GetHitUnit() != GetExplTargetUnit() || GetHitDamage() <= 0)
            return;

        Unit* caster = GetCaster();
        if (AuraEffect const* bonus = caster->GetAuraEffect(SPELL_WARRIOR_T13_PROTECTION_2P, EFFECT_0))
            if (int32 absorb = CalculatePct(GetHitDamage(), bonus->GetAmount()))
                caster->CastSpell(caster, SPELL_WARRIOR_SHIELD_OF_FURY, CastSpellExtraArgs(bonus).AddSpellBP0(absorb));
    }

    void Register() override
    {
        AfterHit.Register(&spell_warr_revenge::ApplyShield);
    }
};

// 871 - Shield Wall
class spell_warr_shield_wall : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_T13_PROTECTION_4P, SPELL_WARRIOR_SHIELD_FORTRESS });
    }

    void ApplyRaidWall()
    {
        Unit* caster = GetCaster();
        AuraEffect const* bonus = caster->GetAuraEffect(SPELL_WARRIOR_T13_PROTECTION_4P, EFFECT_0);
        AuraEffect const* wall = caster->GetAuraEffect(GetSpellInfo()->Id, EFFECT_0, caster->GetGUID());
        if (bonus && wall)
            caster->CastSpell(nullptr, SPELL_WARRIOR_SHIELD_FORTRESS, CastSpellExtraArgs(wall)
                .AddSpellBP0(CalculatePct(wall->GetAmount(), bonus->GetAmount()))
                .AddSpellMod(SPELLVALUE_DURATION, wall->GetBase()->GetDuration()));
    }

    void Register() override
    {
        AfterHit.Register(&spell_warr_shield_wall::ApplyRaidWall);
    }
};

// 32215 - Victorious State
// DBC 15595 (34428): Ataque de la victoria solo tras matar a un enemigo que aporte experiencia u honor.
class spell_warr_victorious_state : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Player* player = GetTarget()->ToPlayer();
        Unit* victim = eventInfo.GetActionTarget();
        return player && victim && player->isHonorOrXPTarget(victim);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_victorious_state::CheckProc);
    }
};

// 105914 - Shield Fortress (T13 Protección 4p)
// Muro de escudo da un 50% de su efecto al grupo o banda; el guerrero ya tiene su propio Muro de escudo y no debe acumular este.
class spell_warr_shield_fortress : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targets.remove(GetCaster());
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_warr_shield_fortress::FilterTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
    }
};

// 85730, 1719 - Deadly Calm and Recklessness
class spell_warr_deadly_calm_recklessness : public SpellScript
{
    SpellCastResult CheckCast()
    {
        uint32 conflictingSpell = GetSpellInfo()->Id == SPELL_WARRIOR_DEADLY_CALM ? SPELL_WARRIOR_RECKLESSNESS : SPELL_WARRIOR_DEADLY_CALM;
        return GetCaster()->HasAura(conflictingSpell) ? SPELL_FAILED_CASTER_AURASTATE : SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_warr_deadly_calm_recklessness::CheckCast);
    }
};

// Updated 4.3.4
// -12834 - Deep Wounds
class spell_warr_deep_wounds : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_DEEP_WOUNDS_PERIODIC });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell const* spell = eventInfo.GetProcSpell();
        if (!spell || !spell->IsTriggered())
            return true;

        SpellInfo const* info = spell->GetSpellInfo();
        if (info->SpellFamilyName != SPELLFAMILY_WARRIOR)
            return false;

        switch (info->Id)
        {
            case 20253: // Intercept
            case 52174: // Heroic Leap
            case 96103: // Raging Blow (main hand)
            case 85384: // Raging Blow (off hand)
                return true;
            default:
                return info->HasAttribute(SPELL_ATTR3_NOT_A_PROC);
        }
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        if (Player* player = GetTarget()->ToPlayer())
        {
            float minDamage = 0.0f;
            float maxDamage = 0.0f;
            player->CalculateMinMaxDamage(BASE_ATTACK, true, false, minDamage, maxDamage);
            float averageDamage = (std::max(0.0f, minDamage) + std::max(0.0f, maxDamage)) / 2.0f;
            int32 damage = CalculatePct(averageDamage, 16 * GetSpellInfo()->GetRank());

            if (damage)
                if (SpellInfo const* periodicSpell = sSpellMgr->GetSpellInfo(SPELL_WARRIOR_DEEP_WOUNDS_PERIODIC))
                    if (uint32 ticks = periodicSpell->GetMaxTicks())
                        damage = damage / ticks;

            if (Unit* target = eventInfo.GetProcTarget())
                player->CastSpell(target, SPELL_WARRIOR_DEEP_WOUNDS_PERIODIC, CastSpellExtraArgs(aurEff).AddSpellBP0(damage));
        }
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_deep_wounds::CheckProc);
        OnEffectProc.Register(&spell_warr_deep_wounds::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

/// Updated 4.3.4
// 5308 - Execute
class spell_warr_execute : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_SUDDEN_DEATH });
    }

    void CalculateDamage(Unit* /*victim*/, int32& damage, int32& /*flatMod*/, float& /*pctMod*/)
    {
        Unit* caster = GetCaster();
        int apPct = GetSpellInfo()->Effects[EFFECT_0].CalcValue(caster);

        damage = static_cast<int32>(10 + CalculatePct(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.437, apPct));

        int32 additionalRageCap = GetSpellInfo()->Effects[EFFECT_1].CalcValue(caster) * 10;
        if (!additionalRageCap)
            return;

        // Spell::TakePower has already paid the base cost before the launch phase.
        int32 additionalRage = std::clamp(caster->GetPower(POWER_RAGE), 0, additionalRageCap);

        // Based on the pct of extra rage consumed the damage will be increased
        float extraRagePct = float(additionalRage) / additionalRageCap;
        int32 bonusDamage = std::max<int32>(0, CalculatePct(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.874f, apPct) - 1);

        damage += bonusDamage * extraRagePct;

        _additionalRageConsumed = additionalRage;
    }

    void ConsumeAdditionalRage()
    {
        if (_additionalRageConsumed < 0)
            return;

        // Sudden Death bonus (keep 5/10 Rage after using Execute)
        int32 preservedRage = 0;
        if (Aura const* aura = GetCaster()->GetAuraOfRankedSpell(SPELL_WARRIOR_SUDDEN_DEATH, GetCaster()->GetGUID()))
            if (AuraEffect const* suddenDeath = aura->GetEffect(EFFECT_0))
                preservedRage = std::min<int32>(suddenDeath->GetAmount() * 10, GetSpell()->GetPowerCost() + _additionalRageConsumed);

        Unit* caster = GetCaster();
        int32 remainingRage = std::max(0, caster->GetPower(POWER_RAGE) - _additionalRageConsumed);
        int32 refund = std::max(0, preservedRage - remainingRage);
        caster->ModifyPower(POWER_RAGE, refund - _additionalRageConsumed);
    }

    void Register() override
    {
        CalcDamage.Register(&spell_warr_execute::CalculateDamage);
        AfterCast.Register(&spell_warr_execute::ConsumeAdditionalRage);
    }
private:
    int32 _additionalRageConsumed = -1; // Damage calculation has not run.
};

// 58387 - Glyph of Sunder Armor
class spell_warr_glyph_of_sunder_armor : public AuraScript
{
    void HandleEffectCalcSpellMod(AuraEffect const* aurEff, SpellModifier*& spellModifier)
    {
        if (!spellModifier)
        {
            SpellModifierByClassMask* spellMod = new SpellModifierByClassMask(aurEff->GetBase());
            spellMod->op = SpellModOp(aurEff->GetMiscValue());
            spellMod->type = SPELLMOD_FLAT;
            spellMod->spellId = GetId();
            spellMod->mask = GetSpellInfo()->Effects[aurEff->GetEffIndex()].SpellClassMask;
            spellModifier = spellMod;
        }

        static_cast<SpellModifierByClassMask*>(spellModifier)->value = aurEff->GetAmount();
    }

    void Register() override
    {
        DoEffectCalcSpellMod.Register(&spell_warr_glyph_of_sunder_armor::HandleEffectCalcSpellMod, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 1715 - Hamstring
class spell_warr_hamstring : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_IMPROVED_HAMSTRING });
    }

    void CheckReapplication()
    {
        Unit* target = GetHitUnit();
        _reapplying = target && target->HasAura(GetSpellInfo()->Id, GetCaster()->GetGUID());
    }

    void ApplyImprovedHamstring()
    {
        if (!_reapplying || !GetHitAura())
            return;

        Unit* caster = GetCaster();
        AuraEffect const* talent = caster->GetAuraEffectOfRankedSpell(SPELL_WARRIOR_IMPROVED_HAMSTRING, EFFECT_0);
        if (!talent)
            return;

        SpellHistory* history = caster->GetSpellHistory();
        if (history->HasCooldown(SPELL_WARRIOR_IMPROVED_HAMSTRING))
            return;

        uint32 root = talent->GetSpellInfo()->Effects[EFFECT_0].TriggerSpell;
        history->AddCooldown(SPELL_WARRIOR_IMPROVED_HAMSTRING, 0, std::chrono::seconds(talent->GetAmount()));
        caster->CastSpell(GetHitUnit(), root, talent);
    }

    void Register() override
    {
        BeforeHit.Register(&spell_warr_hamstring::CheckReapplication);
        AfterHit.Register(&spell_warr_hamstring::ApplyImprovedHamstring);
    }

    bool _reapplying = false;
};

// -50685 - Incite
class spell_warr_incite : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell const* spell = eventInfo.GetProcSpell();
        return spell && std::none_of(spell->m_appliedMods.begin(), spell->m_appliedMods.end(), [](Aura const* aura)
        {
            return aura->GetId() == SPELL_WARRIOR_INCITE_TRIGGER;
        });
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_incite::CheckProc);
    }
};

// 5246 - Intimidating Shout
class spell_warr_intimidating_shout : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& unitList)
    {
        unitList.remove(GetExplTargetWorldObject());
    }

    // Glyph of Intimidating Shout (63327, DBC 15595): todos los objetivos se encogen de pavor en el sitio en vez de huir.
    // Con el glifo, los objetivos secundarios no reciben el miedo (E1) ni la velocidad de huida (E2): se les aplica 20511,
    // el mismo efecto del objetivo principal.
    void FilterFearTargets(std::list<WorldObject*>& unitList)
    {
        FilterTargets(unitList);
        if (!GetCaster()->HasAura(63327))
            return;
        for (WorldObject* target : unitList)
            _cowerTargets.push_back(target->GetGUID());
        unitList.clear();
    }

    void FilterSpeedTargets(std::list<WorldObject*>& unitList)
    {
        FilterTargets(unitList);
        if (GetCaster()->HasAura(63327))
            unitList.clear();
    }

    void ApplyGlyphCower()
    {
        Unit* caster = GetCaster();
        for (ObjectGuid const& guid : _cowerTargets)
            if (Unit* target = ObjectAccessor::GetUnit(*caster, guid))
                caster->CastSpell(target, 20511, true);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_warr_intimidating_shout::FilterFearTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
        OnObjectAreaTargetSelect.Register(&spell_warr_intimidating_shout::FilterSpeedTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
        AfterCast.Register(&spell_warr_intimidating_shout::ApplyGlyphCower);
    }

    GuidVector _cowerTargets;
};

// -84583 Lambs to the Slaughter
class spell_warr_lambs_to_the_slaughter : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_MORTAL_STRIKE, SPELL_WARRIOR_REND });
    }

    void OnProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        if (Aura* rend = eventInfo.GetProcTarget()->GetAura(SPELL_WARRIOR_REND, GetTarget()->GetGUID()))
            rend->RefreshDuration(false);
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_warr_lambs_to_the_slaughter::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

/// Updated 4.3.4
// 12975 - Last Stand
class spell_warr_last_stand : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_LAST_STAND_TRIGGERED });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        int32 healthModSpellBasePoints0 = int32(caster->CountPctFromMaxHealth(GetEffectValue()));
        caster->CastSpell(caster, SPELL_WARRIOR_LAST_STAND_TRIGGERED, { SPELLVALUE_BASE_POINT0, healthModSpellBasePoints0 });
    }

    void Register() override
    {
        // add dummy effect spell handler to Last Stand
        OnEffectHit.Register(&spell_warr_last_stand::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 7384 - Overpower
class spell_warr_overpower : public SpellScript
{
    void HandleEffect(SpellEffIndex /*effIndex*/)
    {
        uint32 spellId = 0;
        if (GetCaster()->HasAura(SPELL_WARRIOR_UNRELENTING_ASSAULT_RANK_1))
            spellId = SPELL_WARRIOR_UNRELENTING_ASSAULT_TRIGGER_1;
        else if (GetCaster()->HasAura(SPELL_WARRIOR_UNRELENTING_ASSAULT_RANK_2))
            spellId = SPELL_WARRIOR_UNRELENTING_ASSAULT_TRIGGER_2;

        if (!spellId)
            return;

        if (Player* target = GetHitPlayer())
            if (target->IsNonMeleeSpellCast(false, false, true)) // UNIT_STATE_CASTING should not be used here, it's present during a tick for instant casts
                target->CastSpell(target, spellId, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_warr_overpower::HandleEffect, EFFECT_0, SPELL_EFFECT_ANY);
    }
};

// 97462 - Rallying Cry
class spell_warr_rallying_cry : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_RALLYING_CRY });
    }

    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        int32 basePoints0 = int32(GetHitUnit()->CountPctFromMaxHealth(GetEffectValue()));
        GetCaster()->CastSpell(GetHitUnit(), SPELL_WARRIOR_RALLYING_CRY, { SPELLVALUE_BASE_POINT0, basePoints0 });
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_warr_rallying_cry::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 94009 - Rend
class spell_warr_rend : public AuraScript
{
    void CalculateDamage(AuraEffect const* aurEff, Unit* /*target*/, int32& damage, int32& /*flatMod*/, float& /*pctMod*/)
    {
        if (Unit* caster = GetCaster())
        {
            // $0.25 * (($MWB + $mwb) / 2 + $AP / 14 * $MWS) bonus per tick
            float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
            int32 mws = caster->GetBaseAttackTime(BASE_ATTACK);
            float mwbMin = caster->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
            float mwbMax = caster->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);
            float mwb = ((mwbMin + mwbMax) / 2 + ap * mws / 14000) * 0.25f;
            damage += int32(caster->ApplyEffectModifiers(GetSpellInfo(), aurEff->GetEffIndex(), mwb));
        }
    }

    void KeepCalculatedAmount(AuraEffect const* /*aurEff*/, int32& /*amount*/, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
    }

    void Register() override
    {
        DoEffectCalcDamageAndHealing.Register(&spell_warr_rend::CalculateDamage, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        DoEffectCalcAmount.Register(&spell_warr_rend::KeepCalculatedAmount, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

// 20230 - Retaliation
class spell_warr_retaliation : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_RETALIATION_DAMAGE });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        // check attack comes not from behind and warrior is not stunned
        return GetTarget()->isInFront(eventInfo.GetActor(), float(M_PI)) && !GetTarget()->HasUnitState(UNIT_STATE_STUNNED);
    }

    void HandleEffectProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_WARRIOR_RETALIATION_DAMAGE, aurEff);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_retaliation::CheckProc);
        OnEffectProc.Register(&spell_warr_retaliation::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 64380, 65941 - Shattering Throw
class spell_warr_shattering_throw : public SpellScript
{
    void HandleScript(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);

        // remove shields, will still display immune to damage part
        if (Unit* target = GetHitUnit())
            target->RemoveAurasWithMechanic(1 << MECHANIC_IMMUNE_SHIELD, AuraRemoveFlags::ByEnemySpell);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_warr_shattering_throw::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

/// Updated 4.3.4
// 1464 - Slam
class spell_warr_slam : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_WARRIOR_SLAM_MAIN_HAND,
                SPELL_WARRIOR_SLAM_OFF_HAND,
                SPELL_WARRIOR_BATTLE_TRANCE_TRIGGERED
            });
    }

    void ConsumeBattleTrance()
    {
        Unit* caster = GetCaster();
        if (Aura* battleTrance = caster->GetAura(SPELL_WARRIOR_BATTLE_TRANCE_TRIGGERED))
            if (GetSpell()->m_appliedMods.contains(battleTrance))
                battleTrance->Remove();
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Unit* target = GetHitUnit();
        caster->CastSpell(target, SPELL_WARRIOR_SLAM_MAIN_HAND, true);

        if (caster->GetAuraEffect(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, SPELLFAMILY_WARRIOR, WARRIOR_ICON_ID_SINGLE_MINDED_FURY, EFFECT_0))
            caster->CastSpell(target, SPELL_WARRIOR_SLAM_OFF_HAND, true);
    }

    void Register() override
    {
        AfterCast.Register(&spell_warr_slam::ConsumeBattleTrance);
        OnEffectLaunchTarget.Register(&spell_warr_slam::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_warr_slam_triggered : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_BLOODSURGE_R1 });
    }

    void HandleBeforeCast()
    {
        _affectedByBloodsurge = GetCaster()->GetAuraEffect(SPELL_AURA_ADD_PCT_MODIFIER, SPELLFAMILY_WARRIOR, 0x0, 0x1000000, 0x0, GetCaster()->GetGUID()) != nullptr;
    }

    void CalculateDamage(Unit* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod)
    {
        // Bloodsurge damage bonus
        if (!_affectedByBloodsurge)
            return;

        if (Aura const* aura = GetCaster()->GetAuraOfRankedSpell(SPELL_WARRIOR_BLOODSURGE_R1))
            if (AuraEffect const* effect = aura->GetEffect(EFFECT_0))
                AddPct(pctMod, effect->GetAmount());
    }

    void Register() override
    {
        BeforeCast.Register(&spell_warr_slam_triggered::HandleBeforeCast);
        CalcDamage.Register(&spell_warr_slam_triggered::CalculateDamage);
    }
private:
    bool _affectedByBloodsurge = false;
};

class spell_warr_second_wind_proc : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_WARRIOR_SECOND_WIND_PROC_RANK_1,
                SPELL_WARRIOR_SECOND_WIND_PROC_RANK_2,
                SPELL_WARRIOR_SECOND_WIND_TRIGGER_RANK_1,
                SPELL_WARRIOR_SECOND_WIND_TRIGGER_RANK_2
            });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (eventInfo.GetProcTarget() == GetTarget())
            return false;

        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        return spellInfo && (spellInfo->GetAllEffectsMechanicMask() & ((1 << MECHANIC_ROOT) | (1 << MECHANIC_STUN)));
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        uint32 spellId = 0;

        if (GetSpellInfo()->Id == SPELL_WARRIOR_SECOND_WIND_PROC_RANK_1)
            spellId = SPELL_WARRIOR_SECOND_WIND_TRIGGER_RANK_1;
        else if (GetSpellInfo()->Id == SPELL_WARRIOR_SECOND_WIND_PROC_RANK_2)
            spellId = SPELL_WARRIOR_SECOND_WIND_TRIGGER_RANK_2;
        if (!spellId)
            return;

        GetTarget()->CastSpell(GetTarget(), spellId, aurEff);

    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_second_wind_proc::CheckProc);
        OnEffectProc.Register(&spell_warr_second_wind_proc::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_warr_second_wind_trigger : public AuraScript
{
    void CalculateHealing(AuraEffect const* aurEff, Unit* target, int32& amount, int32& /*flatMod*/, float& /*pctMod*/)
    {
        uint32 totalTicks = aurEff->GetTotalTicks();
        amount = totalTicks ? target->CountPctFromMaxHealth(amount) / totalTicks : 0;
    }

    void Register() override
    {
        DoEffectCalcDamageAndHealing.Register(&spell_warr_second_wind_trigger::CalculateHealing, EFFECT_1, SPELL_AURA_PERIODIC_HEAL);
    }
};

// 52437 - Sudden Death
class spell_warr_sudden_death : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_COLOSSUS_SMASH });
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Remove cooldown on Colossus Smash
        if (Player* player = GetTarget()->ToPlayer())
            player->GetSpellHistory()->ResetCooldown(SPELL_WARRIOR_COLOSSUS_SMASH, true);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_warr_sudden_death::HandleApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 12328, 18765, 35429 - Sweeping Strikes
class spell_warr_sweeping_strikes : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_WARRIOR_SWEEPING_STRIKES_EXTRA_ATTACK_1,
                SPELL_WARRIOR_SWEEPING_STRIKES_EXTRA_ATTACK_2
            });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        _procTarget = eventInfo.GetActor()->SelectNearbyTarget(eventInfo.GetProcTarget());
        return _procTarget != nullptr;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (eventInfo.GetDamageInfo())
        {
            SpellInfo const* spellInfo = eventInfo.GetDamageInfo()->GetSpellInfo();
            if (spellInfo && (spellInfo->Id == SPELL_WARRIOR_BLADESTORM_PERIODIC_WHIRLWIND || (spellInfo->Id == SPELL_WARRIOR_EXECUTE && !_procTarget->HasAuraState(AURA_STATE_HEALTHLESS_20_PERCENT))))
            {
                // If triggered by Execute (while target is not under 20% hp) or Bladestorm deals normalized weapon damage
                GetTarget()->CastSpell(_procTarget, SPELL_WARRIOR_SWEEPING_STRIKES_EXTRA_ATTACK_2, aurEff);
            }
            else
            {
                int32 damage = eventInfo.GetDamageInfo()->GetDamage();
                GetTarget()->CastSpell(_procTarget, SPELL_WARRIOR_SWEEPING_STRIKES_EXTRA_ATTACK_1, CastSpellExtraArgs(aurEff).AddSpellBP0(damage));
            }
        }
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_sweeping_strikes::CheckProc);
        OnEffectProc.Register(&spell_warr_sweeping_strikes::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
private:
    Unit* _procTarget = nullptr;
};

// -46951 - Sword and Board
class spell_warr_sword_and_board : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_SHIELD_SLAM });
    }

    void OnProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        // Remove cooldown on Shield Slam
        if (Player* player = GetTarget()->ToPlayer())
            player->GetSpellHistory()->ResetCooldown(SPELL_WARRIOR_SHIELD_SLAM, true);
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_warr_sword_and_board::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// -80128 - Impending Victory
class spell_warr_impending_victory : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* victim = eventInfo.GetActionTarget();
        return victim && victim->GetHealthPct() <= 20.0f;
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_impending_victory::CheckProc);
    }
};

// -81913 - Die by the Sword
class spell_warr_die_by_the_sword : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit const* warrior = GetTarget();
        DamageInfo const* hit = eventInfo.GetDamageInfo();
        if (!hit || !warrior->IsAlive())
            return false;

        uint32 health = warrior->GetHealth();
        uint32 threshold = warrior->CountPctFromMaxHealth(20);
        return health <= threshold && uint64(health) + hit->GetDamage() > threshold;
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_die_by_the_sword::CheckProc);
    }
};

// 34428 - Victory Rush
class spell_warr_victory_rush : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_WARRIOR_IMPENDING_VICTORY,
                SPELL_WARRIOR_VICTORIOUS,
                SPELL_WARRIOR_VICTORIOUS_IMPENDING_VICTORY
            });
    }

    // ${$AP*$m1/100}
    void CalculateDamage(Unit* /*victim*/, int32& damage, int32& /*flatMod*/, float& /*pctMod*/)
    {
        damage = CalculatePct(GetCaster()->GetTotalAttackPowerValue(BASE_ATTACK), damage);
    }

    void CalculateHeal(Unit* /*victim*/, int32& heal, int32& /*flatMod*/, float& /*pctMod*/)
    {
        Unit* caster = GetCaster();

        // Impending Victory has a chance to allow casting Victory rush but it only heals for $s1% of our health
        if (!caster->HasAura(SPELL_WARRIOR_VICTORIOUS, caster->GetGUID()) && caster->HasAura(SPELL_WARRIOR_VICTORIOUS_IMPENDING_VICTORY, caster->GetGUID()))
            if (AuraEffect const* impendingVictoryEff = caster->GetAuraEffectOfRankedSpell(SPELL_WARRIOR_IMPENDING_VICTORY, EFFECT_0, caster->GetGUID()))
                heal = caster->CountPctFromMaxHealth(impendingVictoryEff->GetAmount());
    }

    void RemoveVictorious()
    {
        GetCaster()->RemoveAurasDueToSpell(SPELL_WARRIOR_VICTORIOUS);
        GetCaster()->RemoveAurasDueToSpell(SPELL_WARRIOR_VICTORIOUS_IMPENDING_VICTORY);
    }

    void Register() override
    {
        CalcDamage.Register(&spell_warr_victory_rush::CalculateDamage);
        CalcHealing.Register(&spell_warr_victory_rush::CalculateHeal);
        AfterCast.Register(&spell_warr_victory_rush::RemoveVictorious);
    }
};

// 50720 - Vigilance
class spell_warr_vigilance : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_VENGEANCE });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        _procTarget = GetCaster();
        return _procTarget && eventInfo.GetDamageInfo();
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(_procTarget, SPELL_WARRIOR_VIGILANCE_PROC, aurEff);

        // Since patch 4.3.2, damage from other players cannot grant Vengeance.
        DamageInfo const* damageInfo = eventInfo.GetDamageInfo();
        Unit const* attacker = damageInfo->GetAttacker();
        if (!attacker || attacker->IsPlayer())
            return;

        int32 damage = std::min(CalculatePct(damageInfo->GetDamage(), aurEff->GetSpellInfo()->Effects[EFFECT_1].CalcValue()), CalculatePct(_procTarget->GetMaxHealth(), 10));
        _procTarget->CastSpell(_procTarget, SPELL_WARRIOR_VENGEANCE, CastSpellExtraArgs(aurEff).AddSpellBP0(damage).AddSpellMod(SPELLVALUE_BASE_POINT1, damage).AddSpellMod(SPELLVALUE_BASE_POINT2, damage));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_vigilance::CheckProc);
        OnEffectProc.Register(&spell_warr_vigilance::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
private:
    Unit* _procTarget = nullptr;
};

// 50725 Vigilance
class spell_warr_vigilance_trigger : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_TAUNT });
    }

    void HandleScript(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);

        // Remove Taunt cooldown
        if (Player* target = GetHitPlayer())
            target->GetSpellHistory()->ResetCooldown(SPELL_WARRIOR_TAUNT, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_warr_vigilance_trigger::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 76838 - Strikes of Opportunity
class spell_warr_strikes_of_opportunity : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_OPPORTUNITY_STRIKE });
    }

    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        if (Player const* player = GetTarget()->ToPlayer())
        {
            float chance = player->GetFloatValue(PLAYER_MASTERY) * GetSpellInfo()->Effects[EFFECT_0].BonusMultiplier;
            return roll_chance_f(chance);
        }

        return false;
    }

    void HandleEffectProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (Unit* caster = GetCaster())
            if (Unit* target = eventInfo.GetActionTarget())
                caster->CastSpell(target, SPELL_WARRIOR_OPPORTUNITY_STRIKE, aurEff);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_warr_strikes_of_opportunity::CheckProc);
        OnEffectProc.Register(&spell_warr_strikes_of_opportunity::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_warr_thunder_clap : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_REND });
    }

    bool Load() override
    {
        _allowRendSpread = false;

        return true;
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        AuraEffect const* aurEff = GetCaster()->GetDummyAuraEffect(SPELLFAMILY_WARRIOR, WARRIOR_ICON_ID_BLOOD_AND_THUNDER, EFFECT_0);
        if (!aurEff || !roll_chance_i(aurEff->GetAmount()))
            return;

        for (WorldObject const* target : targets)
        {
            if (!target->IsUnit() || !target->ToUnit()->HasAura(SPELL_WARRIOR_REND, GetCaster()->GetGUID()))
                continue;

            _allowRendSpread = true;
        }
    }

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        if (_allowRendSpread)
            GetCaster()->CastSpell(GetHitUnit(), SPELL_WARRIOR_REND, true);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_warr_thunder_clap::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnEffectHitTarget.Register(&spell_warr_thunder_clap::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
private:
    bool _allowRendSpread = false;
};

class spell_warr_shield_specialization : public AuraScript
{
    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        if (eventInfo.GetHitMask() & PROC_HIT_REFLECT)
        {
            PreventDefaultAction();
            Unit* target = GetTarget();
            if (SpellInfo const* spell = sSpellMgr->GetSpellInfo((GetSpellInfo()->Effects[aurEff->GetEffIndex()].TriggerSpell)))
            {
                int32 bp = spell->Effects[EFFECT_0].CalcValue() * 4;
                target->CastSpell(nullptr, spell->Id, CastSpellExtraArgs(aurEff).AddSpellBP0(bp));
            }
        }
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_warr_shield_specialization::HandleProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_warr_blood_craze : public AuraScript
{
    void CalculateHealing(AuraEffect const* aurEff, Unit* target, int32& amount, int32& /*flatMod*/, float& /*pctMod*/)
    {
        int32 bp = GetSpellInfo()->Effects[EFFECT_0].BasePoints / 2;
        uint32 totalTicks = aurEff->GetTotalTicks();
        amount = totalTicks ? CalculatePct(target->GetMaxHealth(), float(bp) / totalTicks) : 0;
    }

    void Register() override
    {
        DoEffectCalcDamageAndHealing.Register(&spell_warr_blood_craze::CalculateHealing, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
    }
};

// 46968 - Shockwave
class spell_warr_shockwave : public SpellScript
{
    void CalculateDamage(SpellEffIndex /*effIndex*/)
    {
        SetEffectValue(CalculatePct(GetCaster()->GetTotalAttackPowerValue(BASE_ATTACK), GetSpellInfo()->Effects[EFFECT_2].CalcValue(GetCaster())));
    }

    void Register() override
    {
        OnEffectLaunchTarget.Register(&spell_warr_shockwave::CalculateDamage, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 20243 - Devastate
class spell_warr_devastate : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_SUNDER_ARMOR });
    }

    void HandleDamageBonus(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            if (Aura* sunder = GetHitUnit()->GetAura(SPELL_WARRIOR_SUNDER_ARMOR, caster->GetGUID()))
                SetEffectValue(GetEffectValue() + GetEffectValue() * sunder->GetStackAmount());
    }

    void HandleSunderArmor(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(GetHitUnit(), SPELL_WARRIOR_SUNDER_ARMOR, CastSpellExtraArgs(true).SetCustomArg(GetSpellInfo()->Id));
    }

    void Register() override
    {
        OnEffectLaunchTarget.Register(&spell_warr_devastate::HandleDamageBonus, EFFECT_1, SPELL_EFFECT_NORMALIZED_WEAPON_DMG);
        OnEffectHitTarget.Register(&spell_warr_devastate::HandleSunderArmor, EFFECT_1, SPELL_EFFECT_NORMALIZED_WEAPON_DMG);
    }
};

class spell_warr_whirlwind : public SpellScript
{
    void CountDamagedTarget()
    {
        if (GetHitDamage() > 0)
            ++_hitTargets;
    }

    void HandleCooldownReduction()
    {
        Unit* caster = GetCaster();
        if (!caster || _hitTargets < 4)
            return;

        caster->GetSpellHistory()->ModifyCooldown(GetSpellInfo()->Id, ((GetSpellInfo()->Effects[EFFECT_2].CalcValue(caster) * IN_MILLISECONDS) * -1));
    }

    void Register() override
    {
        AfterHit.Register(&spell_warr_whirlwind::CountDamagedTarget);
        AfterCast.Register(&spell_warr_whirlwind::HandleCooldownReduction);
    }
private:
    uint32 _hitTargets = 0;
};

class spell_warr_heroic_leap : public SpellScript
{
    SpellCastResult CheckLocation()
    {
        // The client shows an area as unreachable once the target destination is 4 yards above your position
        if (!GetExplTargetDest() || GetExplTargetDest()->GetPositionZ() - GetCaster()->GetPositionZ() > 4.f)
            return SPELL_FAILED_NOPATH;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_warr_heroic_leap::CheckLocation);
    }
};

// 60970 - Heroic Fury
class spell_warr_heroic_fury : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    void HandleCooldownReset(SpellEffIndex /*effIndex*/)
    {
        GetHitUnit()->GetSpellHistory()->ResetCooldown(SPELL_WARRIOR_INTERCEPT, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_warr_heroic_fury::HandleCooldownReset, EFFECT_2, SPELL_EFFECT_APPLY_AURA);
    }
};

// 55694 - Enraged Regeneration
class spell_warr_enraged_regeneration : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_VENGEANCE });
    }

    SpellCastResult CheckCast()
    {
        // Vengeance also applies the enrage aurastate but it does not use the enrage mechanic unlike the other enrage effects. So we check for that.
        bool hasNonVengeanceEnrage = [&]()
        {
            for (auto& itr : GetCaster()->GetAppliedAuras())
            {
                SpellInfo const* spellInfo = itr.second->GetBase()->GetSpellInfo();
                if (spellInfo->Id == SPELL_WARRIOR_VENGEANCE)
                    continue;

                if (spellInfo->Mechanic && ((1 << MECHANIC_ENRAGED) & (1 << spellInfo->Mechanic)) != 0)
                    return true;

                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                    if (itr.second->HasEffect(i) && spellInfo->Effects[i].Effect && spellInfo->Effects[i].Mechanic)
                        if ((1 << MECHANIC_ENRAGED) & (1 << spellInfo->Effects[i].Mechanic))
                            return true;
            }

            return false;
        }();

        if (!hasNonVengeanceEnrage)
            return SPELL_FAILED_CASTER_AURASTATE;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_warr_enraged_regeneration::CheckCast);
    }
};

class spell_warr_enraged_regeneration_AuraScript: public AuraScript
{
    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ENRAGED, true);
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ENRAGED, false);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_warr_enraged_regeneration_AuraScript::HandleApply, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove.Register(&spell_warr_enraged_regeneration_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH, AURA_EFFECT_HANDLE_REAL);
    }
};

// 78 - Heroic Strike
// 845 - Cleave
// 4.3.4 tooltip: ${6+$ap*0.45} physical damage to the target and one nearby enemy (two with Glyph of Cleaving). The spell
// has no attack power coefficient in the DBC, so without this the core only dealt the base points.
class spell_warr_cleave : public SpellScript
{
    void CalculateDamage(Unit* /*victim*/, int32& damage, int32& /*flatMod*/, float& /*pctMod*/)
    {
        damage = 6 + int32(GetCaster()->GetTotalAttackPowerValue(BASE_ATTACK) * 0.45f);
    }

    void Register() override
    {
        CalcDamage.Register(&spell_warr_cleave::CalculateDamage);
    }
};

class spell_warr_heroic_strike : public SpellScript
{
    void CalculateDamage(Unit* /*victim*/, int32& damage, int32& /*flatMod*/, float& /*pctMod*/)
    {
        // Tooltip value:
        damage = 8 + GetCaster()->GetTotalAttackPowerValue(BASE_ATTACK) * 60 / 100;
    }

    void Register() override
    {
        CalcDamage.Register(&spell_warr_heroic_strike::CalculateDamage);
    }
};
}

void AddSC_warrior_spell_scripts()
{
    using namespace Spells::Warrior;
    RegisterSpellScript(spell_warr_blood_craze);
    RegisterSpellScript(spell_warr_bloodthirst);
    RegisterSpellScript(spell_warr_bloodthirst_heal);
    RegisterSpellScript(spell_warr_charge);
    RegisterSpellAndAuraScriptPair(spell_warr_colossus_smash, spell_warr_colossus_smash_aura);
    RegisterSpellScript(spell_warr_colossus_smash_aura);
    RegisterSpellScript(spell_warr_concussion_blow);
    RegisterSpellScript(spell_warr_deadly_calm_recklessness);
    RegisterSpellScript(spell_warr_deep_wounds);
    RegisterSpellScript(spell_warr_devastate);
    RegisterSpellScript(spell_warr_die_by_the_sword);
    RegisterSpellAndAuraScriptPair(spell_warr_enraged_regeneration, spell_warr_enraged_regeneration_AuraScript);
    RegisterSpellScript(spell_warr_execute);
    RegisterSpellScript(spell_warr_glyph_of_sunder_armor);
    RegisterSpellScript(spell_warr_hamstring);
    RegisterSpellScript(spell_warr_heroic_fury);
    RegisterSpellScript(spell_warr_heroic_leap);
    RegisterSpellScript(spell_warr_heroic_strike);
    RegisterSpellScript(spell_warr_cleave);
    RegisterSpellScript(spell_warr_impending_victory);
    RegisterSpellScript(spell_warr_incite);
    RegisterSpellScript(spell_warr_intimidating_shout);
    RegisterSpellScript(spell_warr_lambs_to_the_slaughter);
    RegisterSpellScript(spell_warr_last_stand);
    RegisterSpellScript(spell_warr_overpower);
    RegisterSpellScript(spell_warr_rallying_cry);
    RegisterSpellScript(spell_warr_rend);
    RegisterSpellScript(spell_warr_retaliation);
    RegisterSpellScript(spell_warr_revenge);
    RegisterSpellScript(spell_warr_shield_slam_t12);
    RegisterSpellScript(spell_warr_shield_block_t12);
    RegisterSpellScript(spell_warr_second_wind_proc);
    RegisterSpellScript(spell_warr_second_wind_trigger);
    RegisterSpellScript(spell_warr_shattering_throw);
    RegisterSpellScript(spell_warr_shield_specialization);
    RegisterSpellScript(spell_warr_shield_wall);
    RegisterSpellScript(spell_warr_shield_fortress);
    RegisterSpellScript(spell_warr_victorious_state);
    RegisterSpellScript(spell_warr_shockwave);
    RegisterSpellScript(spell_warr_slam);
    RegisterSpellScript(spell_warr_slam_triggered);
    RegisterSpellScript(spell_warr_strikes_of_opportunity);
    RegisterSpellScript(spell_warr_sudden_death);
    RegisterSpellScript(spell_warr_sweeping_strikes);
    RegisterSpellScript(spell_warr_sword_and_board);
    RegisterSpellScript(spell_warr_thunder_clap);
    RegisterSpellScript(spell_warr_victory_rush);
    RegisterSpellScript(spell_warr_vigilance);
    RegisterSpellScript(spell_warr_vigilance_trigger);
    RegisterSpellScript(spell_warr_whirlwind);
}
