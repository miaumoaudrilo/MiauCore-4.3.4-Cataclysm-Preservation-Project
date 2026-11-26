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
 * Scripts for spells with SPELLFAMILY_ROGUE and SPELLFAMILY_GENERIC spells used by rogue players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_rog_".
 */

#include "ScriptMgr.h"
#include "CellImpl.h"
#include "Containers.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "GridNotifiersImpl.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"

namespace Spells::Rogue
{
enum RogueSpells
{
    SPELL_ROGUE_ADRENALIN_RUSH                      = 13750,
    SPELL_ROGUE_BANDITS_GUILE                       = 84748,
    SPELL_ROGUE_BLADE_FLURRY                        = 13877,
    SPELL_ROGUE_BLADE_FLURRY_EXTRA_ATTACK           = 22482,
    SPELL_ROGUE_CHEAT_DEATH_COOLDOWN                = 31231,
    SPELL_ROGUE_CHEAT_DEATH_DAMAGE_REDUCTION         = 45182,
    SPELL_ROGUE_CRIPPLING_POISON                    = 3409,
    SPELL_ROGUE_DEEP_INSIGHT                        = 84747,
    SPELL_ROGUE_ENERGETIC_RECOVERY                  = 4893,
    SPELL_ROGUE_EVISCERATE_AND_ENVENOM_BONUS_DAMAGE = 37169,
    SPELL_ROGUE_EXPOSE_ARMOR                        = 8647,
    SPELL_ROGUE_GLYPH_OF_PREPARATION                = 56819,
    SPELL_ROGUE_GLYPH_HEMORRHAGE_TRIGGERED          = 89775,
    SPELL_ROGUE_IMPROVED_EXPOSE_ARMOR               = 79128,
    SPELL_ROGUE_KILLING_SPREE                       = 51690,
    SPELL_ROGUE_KILLING_SPREE_TELEPORT              = 57840,
    SPELL_ROGUE_KILLING_SPREE_WEAPON_DMG            = 57841,
    SPELL_ROGUE_KILLING_SPREE_DMG_BUFF              = 61851,
    SPELL_ROGUE_MAIN_GAUCHE                         = 86392,
    SPELL_ROGUE_MASTER_OF_SUBTLETY_DAMAGE_PERCENT   = 31665,
    SPELL_ROGUE_MASTER_OF_SUBTLETY_PASSIVE          = 31223,
    SPELL_ROGUE_MASTER_OF_SUBTLETY_PERIODIC         = 31666,
    SPELL_ROGUE_MASTER_POISONER                     = 58410,
    SPELL_ROGUE_MODERATE_INSIGHT                    = 84746,
    SPELL_ROGUE_MURDEROUS_INTENT                    = 79132,
    SPELL_ROGUE_OVERKILL_TALENT                     = 58426,
    SPELL_ROGUE_OVERKILL_PERIODIC                   = 58428,
    SPELL_ROGUE_OVERKILL_POWER_REGEN                = 58427,
    SPELL_ROGUE_PREY_ON_THE_WEAK                    = 58670,
    SPELL_ROGUE_REVEALING_STRIKE                    = 84617,
    SPELL_ROGUE_REDIRECT                            = 73981,
    SPELL_ROGUE_BANDITS_GUILE_R1                    = 84652,
    SPELL_ROGUE_GLYPH_OF_BLIND                      = 91299,
    SPELL_ROGUE_SMOKE_BOMB_INTERFERE                = 88611,
    SPELL_ROGUE_RELENTLESS_STRIKES_ENERGIZE         = 98440,
    SPELL_ROGUE_SHALLOW_INSIGHT                     = 84745,
    SPELL_ROGUE_SHIV_TRIGGERED                      = 5940,
    SPELL_ROGUE_SILCE_AND_DICE                      = 5171,
    SPELL_ROGUE_SPRINT                              = 2983,
    SPELL_ROGUE_TRICKS_OF_THE_TRADE                 = 57934,
    SPELL_ROGUE_TRICKS_OF_THE_TRADE_DMG_BOOST       = 57933,
    SPELL_ROGUE_TRICKS_OF_THE_TRADE_PROC            = 59628,
    SPELL_ROGUE_SERRATED_BLADES_R1                  = 14171,
    SPELL_ROGUE_VENOMOUS_VIM                        = 51637,
    SPELL_ROGUE_VENOMOUS_WOUND                      = 79136,
    SPELL_ROGUE_RUPTURE                             = 1943,
    SPELL_ROGUE_HONOR_AMONG_THIEVES_TRIGGERED       = 51699,
    SPELL_ROGUE_BLACKJACK_R1                        = 79123,
    SPELL_RACIAL_ELUSIVENESS                        = 21009,
    SPELL_ROGUE_STEALTH                             =  1784,
    SPELL_ROGUE_VANISH_STEALTH                      = 11327
};

enum RogueSpellIcons
{
    ICON_ROGUE_IMPROVED_RECUPERATE                  = 4819,
    ROGUE_ICON_ID_SERRATED_BLADES                   = 2004,
    ROGUE_ICON_ID_SANGUINARY_VEIN                   = 4821,
    ROGUE_ICON_ID_VENOMOUS_WOUNDS                   = 4888
};

// 13877, 33735, (check 51211, 65956) - Blade Flurry
class spell_rog_blade_flurry : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_BLADE_FLURRY_EXTRA_ATTACK });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        DamageInfo const* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetVictim() || !damageInfo->GetDamage())
            return;

        Unit* rogue = GetTarget();
        Unit* target = nullptr;
        if (GetId() == SPELL_ROGUE_BLADE_FLURRY)
        {
            SpellInfo const* cleave = sSpellMgr->AssertSpellInfo(SPELL_ROGUE_BLADE_FLURRY_EXTRA_ATTACK);
            float const searchRange = std::max(float(NOMINAL_MELEE_RANGE), rogue->GetMeleeRange(rogue));
            std::list<Unit*> targets;
            Trinity::AnyAoETargetUnitInObjectRangeCheck check(rogue, rogue, searchRange, cleave);
            Trinity::UnitListSearcher<Trinity::AnyAoETargetUnitInObjectRangeCheck> searcher(rogue, targets, check);
            Cell::VisitAllObjects(rogue, searcher, searchRange);
            targets.remove_if([&](Unit const* candidate)
            {
                return candidate == damageInfo->GetVictim() || !rogue->IsWithinMeleeRange(candidate) ||
                    !rogue->IsWithinLOSInMap(candidate) || candidate->IsSpiritService() || candidate->IsCritter();
            });

            if (!targets.empty())
                target = Trinity::Containers::SelectRandomContainerElement(targets);
        }
        else
            target = rogue->SelectNearbyTarget(damageInfo->GetVictim(), rogue->GetCombatReach());

        if (target)
            rogue->CastSpell(target, SPELL_ROGUE_BLADE_FLURRY_EXTRA_ATTACK, CastSpellExtraArgs(aurEff).AddSpellBP0(damageInfo->GetDamage()));
    }

    void Register() override
    {
        if (m_scriptSpellId == SPELL_ROGUE_BLADE_FLURRY)
            OnEffectProc.Register(&spell_rog_blade_flurry::HandleProc, EFFECT_0, SPELL_AURA_MOD_POWER_REGEN_PERCENT);
        else
            OnEffectProc.Register(&spell_rog_blade_flurry::HandleProc, EFFECT_0, SPELL_AURA_MOD_MELEE_HASTE);
    }
};

// 31228 - Cheat Death
class spell_rog_cheat_death : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_CHEAT_DEATH_COOLDOWN, SPELL_ROGUE_CHEAT_DEATH_DAMAGE_REDUCTION });
    }

    bool Load() override
    {
        _absorbChance = GetSpellInfo()->Effects[EFFECT_0].CalcValue();
        return GetUnitOwner()->GetTypeId() == TYPEID_PLAYER;
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // Set absorbtion amount to unlimited
        amount = -1;
    }

    void Absorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount)
    {
        Player* target = GetTarget()->ToPlayer();
        if (dmgInfo.GetDamage() < target->GetHealth() || target->GetSpellHistory()->HasCooldown(SPELL_ROGUE_CHEAT_DEATH_COOLDOWN) || !roll_chance_i(_absorbChance))
            return;

        target->CastSpell(target, SPELL_ROGUE_CHEAT_DEATH_COOLDOWN, true);
        target->GetSpellHistory()->AddCooldown(SPELL_ROGUE_CHEAT_DEATH_COOLDOWN, 0, std::chrono::seconds(90));
        target->CastSpell(target, SPELL_ROGUE_CHEAT_DEATH_DAMAGE_REDUCTION, true);

        uint32 health10 = std::max<uint32>(1, target->CountPctFromMaxHealth(10));

        // hp > 10% - absorb hp till 10%
        if (target->GetHealth() > health10)
            absorbAmount = dmgInfo.GetDamage() - target->GetHealth() + health10;
        // hp lower than 10% - absorb everything
        else
            absorbAmount = dmgInfo.GetDamage();
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_rog_cheat_death::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb.Register(&spell_rog_cheat_death::Absorb, EFFECT_0);
    }
private:
    int32 _absorbChance = 0;
};

// -51625 - Deadly Brew
class spell_rog_crippling_poison : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_CRIPPLING_POISON });
    }

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_ROGUE_CRIPPLING_POISON, aurEff);
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_rog_crippling_poison::OnProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// -51664 - Cut to the Chase
class spell_rog_cut_to_the_chase : public AuraScript
{
    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        if (Aura* aur = GetTarget()->GetAura(SPELL_ROGUE_SILCE_AND_DICE))
            aur->SetDuration(aur->GetSpellInfo()->GetMaxDuration(), true);
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_rog_cut_to_the_chase::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 2818 - Deadly Poison
class spell_rog_deadly_poison : public SpellScript
{
    bool Load() override
    {
        // at this point CastItem must already be initialized
        return GetCaster()->GetTypeId() == TYPEID_PLAYER && GetCastItem();
    }

    void HandleBeforeHit()
    {
        if (Unit* target = GetHitUnit())
            // Deadly Poison
            if (AuraEffect const* aurEff = target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x10000, 0x80000, 0, GetCaster()->GetGUID()))
                _stackAmount = aurEff->GetBase()->GetStackAmount();
    }

    void HandleAfterHit()
    {
        if (_stackAmount < 5)
            return;

        Player* player = GetCaster()->ToPlayer();

        if (Unit* target = GetHitUnit())
        {

            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

            if (item == GetCastItem())
                item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

            if (!item)
                return;

            // item combat enchantments
            for (uint8 slot = 0; slot < MAX_ENCHANTMENT_SLOT; ++slot)
            {
                if (slot > PRISMATIC_ENCHANTMENT_SLOT && slot < PROP_ENCHANTMENT_SLOT_0)    // not holding enchantment id
                    continue;

                SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(item->GetEnchantmentId(EnchantmentSlot(slot)));
                if (!enchant)
                    continue;

                for (uint8 s = 0; s < 3; ++s)
                {
                    if (enchant->Effect[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                        continue;

                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(enchant->EffectArg[s]);
                    if (!spellInfo)
                        continue;

                    // Proc only rogue poisons
                    if (spellInfo->SpellFamilyName != SPELLFAMILY_ROGUE || spellInfo->Dispel != DISPEL_POISON)
                        continue;

                    // Do not reproc deadly
                    if (spellInfo->SpellFamilyFlags.IsEqual(0x10000, 0x80000, 0))
                        continue;

                    if (spellInfo->IsPositive())
                        player->CastSpell(player, enchant->EffectArg[s], item);
                    else
                        player->CastSpell(target, enchant->EffectArg[s], item);
                }
            }
        }
    }

    void Register() override
    {
        BeforeHit.Register(&spell_rog_deadly_poison::HandleBeforeHit);
        AfterHit.Register(&spell_rog_deadly_poison::HandleAfterHit);
    }
private:
    uint8 _stackAmount = 0;
};

// 51690 - Killing Spree
class spell_rog_killing_spree : public SpellScriptLoader
{
    public:
        static char constexpr const ScriptName[] = "spell_rog_killing_spree";

        spell_rog_killing_spree() : SpellScriptLoader(ScriptName) { }

        class spell_rog_killing_spree_SpellScript : public SpellScript
        {
            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if([this](WorldObject const* object)
                {
                    Unit const* target = object->ToUnit();
                    return !target || !target->IsAlive() || target->IsInFlight() ||
                        target->HasAuraType(SPELL_AURA_MOD_STEALTH) || target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY) ||
                        !GetCaster()->IsWithinLOSInMap(target);
                });

                if (targets.empty() || GetCaster()->GetVehicleBase())
                    FinishCast(SPELL_FAILED_OUT_OF_RANGE);
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Aura* aura = GetCaster()->GetAura(SPELL_ROGUE_KILLING_SPREE))
                    if (spell_rog_killing_spree_AuraScript* script = aura->GetScript<spell_rog_killing_spree_AuraScript>(ScriptName))
                        script->AddTarget(GetHitUnit());
            }

            void Register() override
            {
                OnObjectAreaTargetSelect.Register(&spell_rog_killing_spree_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
                OnEffectHitTarget.Register(&spell_rog_killing_spree_SpellScript::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rog_killing_spree_SpellScript();
        }

        class spell_rog_killing_spree_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_ROGUE_KILLING_SPREE_TELEPORT,
                    SPELL_ROGUE_KILLING_SPREE_WEAPON_DMG,
                    SPELL_ROGUE_KILLING_SPREE_DMG_BUFF
                });
            }

            void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetTarget()->CastSpell(GetTarget(), SPELL_ROGUE_KILLING_SPREE_DMG_BUFF, true);
            }

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                Unit* rogue = GetTarget();
                float const range = GetSpellInfo()->GetMaxRange(false, rogue);
                GuidList candidates = _targets;
                while (!candidates.empty())
                {
                    ObjectGuid guid = Trinity::Containers::SelectRandomContainerElement(candidates);
                    candidates.remove(guid);
                    Unit* target = ObjectAccessor::GetUnit(*rogue, guid);
                    if (!target || !target->IsAlive() || target->IsInFlight() ||
                        target->HasAuraType(SPELL_AURA_MOD_STEALTH) || target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY) ||
                        !rogue->IsValidAttackTarget(target, GetSpellInfo()) || !rogue->IsWithinDistInMap(target, range) ||
                        !rogue->IsWithinLOSInMap(target))
                        continue;

                    if (rogue->CastSpell(target, SPELL_ROGUE_KILLING_SPREE_TELEPORT, true) != SPELL_CAST_OK)
                        continue;

                    rogue->CastSpell(target, SPELL_ROGUE_KILLING_SPREE_WEAPON_DMG, true);
                    break;
                }
            }

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetTarget()->RemoveAurasDueToSpell(SPELL_ROGUE_KILLING_SPREE_DMG_BUFF);
            }

            void Register() override
            {
                AfterEffectApply.Register(&spell_rog_killing_spree_AuraScript::HandleApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic.Register(&spell_rog_killing_spree_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
                AfterEffectRemove.Register(&spell_rog_killing_spree_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }

        public:
            void AddTarget(Unit* target)
            {
                _targets.push_back(target->GetGUID());
            }

        private:
            GuidList _targets;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_rog_killing_spree_AuraScript();
        }
};
char constexpr const spell_rog_killing_spree::ScriptName[];

// 31666 - Master of Subtlety
class spell_rog_master_of_subtlety : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_MASTER_OF_SUBTLETY_DAMAGE_PERCENT });
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();

        if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
            target->RemoveAurasDueToSpell(SPELL_ROGUE_MASTER_OF_SUBTLETY_DAMAGE_PERCENT);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_rog_master_of_subtlety::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 58428 - Overkill
class spell_rog_overkill : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_OVERKILL_POWER_REGEN });
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();

        if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
            target->RemoveAurasDueToSpell(SPELL_ROGUE_OVERKILL_POWER_REGEN);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_rog_overkill::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 14185 - Preparation
class spell_rog_preparation : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_GLYPH_OF_PREPARATION });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        caster->GetSpellHistory()->ResetCooldowns([caster](SpellHistory::CooldownStorageType::iterator itr) -> bool
        {
            SpellInfo const* spellInfo = sSpellMgr->AssertSpellInfo(itr->first);
        if (spellInfo->SpellFamilyName != SPELLFAMILY_ROGUE)
            return false;

        return (spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG0_ROGUE_VAN_SPRINT || spellInfo->SpellFamilyFlags[1] & SPELLFAMILYFLAG1_ROGUE_SHADOWSTEP) ||              // Vanish, Sprint, Shadowstep
            // Glyph of Preparation
            (caster->HasAura(SPELL_ROGUE_GLYPH_OF_PREPARATION) &&
            (spellInfo->SpellFamilyFlags[1] & SPELLFAMILYFLAG1_ROGUE_DISMANTLE_SMOKE_BOMB ||    // Dismantle, Smoke Bomb
            spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG0_ROGUE_KICK));
        }, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_rog_preparation::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 51685 - Prey on the Weak
class spell_rog_prey_on_the_weak : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_PREY_ON_THE_WEAK });
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();
        Unit* victim = target->GetVictim();
        if (victim && (target->GetHealthPct() > victim->GetHealthPct()))
        {
            if (!target->HasAura(SPELL_ROGUE_PREY_ON_THE_WEAK))
            {
                int32 bp = GetSpellInfo()->Effects[EFFECT_0].CalcValue();
                target->CastSpell(target, SPELL_ROGUE_PREY_ON_THE_WEAK, CastSpellExtraArgs(true).AddSpellBP0(bp));
            }
        }
        else
            target->RemoveAurasDueToSpell(SPELL_ROGUE_PREY_ON_THE_WEAK);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_rog_prey_on_the_weak::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 73651 - Recuperate
class spell_rog_recuperate : public AuraScript
{
    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void OnPeriodic(AuraEffect const* /*aurEff*/)
    {
        if (Unit* caster = GetCaster())
            if (AuraEffect* effect = GetAura()->GetEffect(EFFECT_0))
                effect->RecalculateAmount(caster);
    }

    void CalculateBonus(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        if (Unit* caster = GetCaster())
        {
            int32 baseAmount = GetSpellInfo()->Effects[EFFECT_0].CalcValue(caster) * 1000;
            // Improved Recuperate
            if (AuraEffect const* auraEffect = caster->GetDummyAuraEffect(SPELLFAMILY_ROGUE, ICON_ROGUE_IMPROVED_RECUPERATE, EFFECT_0))
                baseAmount += auraEffect->GetAmount();

            amount = CalculatePct(caster->GetMaxHealth(), float(baseAmount) / 1000.0f);
        }
    }

    void HandleEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        Duration = aurEff->GetBase()->GetDuration();
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_rog_recuperate::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
        DoEffectCalcAmount.Register(&spell_rog_recuperate::CalculateBonus, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
        AfterEffectApply.Register(&spell_rog_recuperate::HandleEffectApply, EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
public:
    int32 Duration = 0;
};

// -1943 - Rupture
class spell_rog_rupture : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_REVEALING_STRIKE, SPELL_ROGUE_VENOMOUS_VIM });
    }

    bool Load() override
    {
        Unit* caster = GetCaster();
        return caster && caster->GetTypeId() == TYPEID_PLAYER;
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        if (Unit* caster = GetCaster())
        {
            canBeRecalculated = false;

            float const attackpowerPerCombo[6] =
            {
                0.0f,
                0.015f,         // 1 point:  ${($m1 + $b1*1 + 0.015 * $AP) * 5} damage over 8 secs
                0.024f,         // 2 points: ${($m1 + $b1*2 + 0.024 * $AP) * 5} damage over 10 secs
                0.03f,          // 3 points: ${($m1 + $b1*3 + 0.03 * $AP) * 5} damage over 12 secs
                0.03428571f,    // 4 points: ${($m1 + $b1*4 + 0.03428571 * $AP) * 5} damage over 14 secs
                0.0375f         // 5 points: ${($m1 + $b1*5 + 0.0375 * $AP) * 5} damage over 16 secs
            };

            uint8 cp = std::min(caster->ToPlayer()->GetComboPoints(), uint8(5));
            amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * attackpowerPerCombo[cp]);

            if (AuraEffect* const revealingStrike = GetUnitOwner()->GetAuraEffect(SPELL_ROGUE_REVEALING_STRIKE, EFFECT_2, caster->GetGUID()))
            {
                amount += CalculatePct(amount, revealingStrike->GetAmount());
                revealingStrike->GetBase()->Remove();
            }
        }
    }

    void HandleVenomousWounds(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (!GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::ByDeath))
            return;

        Unit* caster = GetCaster();
        if (!caster || !caster->IsAlive() ||
            !caster->GetDummyAuraEffect(SPELLFAMILY_ROGUE, ROGUE_ICON_ID_VENOMOUS_WOUNDS, EFFECT_0))
            return;

        int32 const energy = GetDuration() * 5 / IN_MILLISECONDS;
        if (energy > 0)
            caster->CastSpell(caster, SPELL_ROGUE_VENOMOUS_VIM, CastSpellExtraArgs(true).AddSpellBP0(energy));
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_rog_rupture::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        AfterEffectRemove.Register(&spell_rog_rupture::HandleVenomousWounds, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};

// 5938 - Shiv
class spell_rog_shiv : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_SHIV_TRIGGERED });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (Unit* unitTarget = GetHitUnit())
            caster->CastSpell(unitTarget, SPELL_ROGUE_SHIV_TRIGGERED, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_rog_shiv::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 1784 - Stealth, 11327 - Vanish
class spell_rog_stealth : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_ROGUE_MASTER_OF_SUBTLETY_PASSIVE,
                SPELL_ROGUE_MASTER_OF_SUBTLETY_DAMAGE_PERCENT,
                SPELL_ROGUE_MASTER_OF_SUBTLETY_PERIODIC,
                SPELL_ROGUE_OVERKILL_TALENT,
                SPELL_ROGUE_OVERKILL_POWER_REGEN,
                SPELL_ROGUE_OVERKILL_PERIODIC,
                SPELL_RACIAL_ELUSIVENESS
            });
    }

    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();

        // Master of Subtlety
        if (AuraEffect const* aurEff = target->GetAuraEffect(SPELL_ROGUE_MASTER_OF_SUBTLETY_PASSIVE, EFFECT_0))
        {
            int32 basepoints0 = aurEff->GetAmount();
            target->CastSpell(target, SPELL_ROGUE_MASTER_OF_SUBTLETY_DAMAGE_PERCENT, CastSpellExtraArgs(true).AddSpellBP0(basepoints0));
        }

        // Overkill
        if (target->HasAura(SPELL_ROGUE_OVERKILL_TALENT))
            target->CastSpell(target, SPELL_ROGUE_OVERKILL_POWER_REGEN, true);
    }

    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (GetId() == SPELL_ROGUE_VANISH_STEALTH && GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::Expired))
            return;

        // Master of subtlety
        if (target->HasAura(SPELL_ROGUE_MASTER_OF_SUBTLETY_PASSIVE))
            target->CastSpell(target, SPELL_ROGUE_MASTER_OF_SUBTLETY_PERIODIC, true);

        // Overkill
        if (target->HasAura(SPELL_ROGUE_OVERKILL_TALENT))
            target->CastSpell(target, SPELL_ROGUE_OVERKILL_PERIODIC, true);
    }

    void HandleSpeedIncrease(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // Elusiveness (Racial)
        if (AuraEffect const* aurEff = GetUnitOwner()->GetAuraEffect(SPELL_RACIAL_ELUSIVENESS, EFFECT_0))
            amount += aurEff->GetAmount();
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_rog_stealth::HandleEffectApply, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        AfterEffectRemove.Register(&spell_rog_stealth::HandleEffectRemove, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        DoEffectCalcAmount.Register(&spell_rog_stealth::HandleSpeedIncrease, EFFECT_2, SPELL_AURA_MOD_SPEED_ALWAYS);
    }
};

// 57934 - Tricks of the Trade
class spell_rog_tricks_of_the_trade_aura : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_ROGUE_TRICKS_OF_THE_TRADE_DMG_BOOST,
                SPELL_ROGUE_TRICKS_OF_THE_TRADE_PROC
            });
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (!GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::ByDefault) || !GetTarget()->HasAura(SPELL_ROGUE_TRICKS_OF_THE_TRADE_PROC))
            GetTarget()->GetThreatManager().UnregisterRedirectThreat(SPELL_ROGUE_TRICKS_OF_THE_TRADE);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* rogue = GetTarget();
        Unit* target = ObjectAccessor::GetUnit(*rogue, _redirectTarget);
        if (target && target->IsAlive() && rogue->IsInRaidWith(target))
        {
            rogue->CastSpell(target, SPELL_ROGUE_TRICKS_OF_THE_TRADE_DMG_BOOST, aurEff);
            rogue->CastSpell(rogue, SPELL_ROGUE_TRICKS_OF_THE_TRADE_PROC, aurEff);
        }
        // The proc aura now owns the redirect until its six-second duration ends.
        Remove(AuraRemoveFlags::ByDefault);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_rog_tricks_of_the_trade_aura::OnRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectProc.Register(&spell_rog_tricks_of_the_trade_aura::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }

    ObjectGuid _redirectTarget;
public:
    void SetRedirectTarget(ObjectGuid const& guid) { _redirectTarget = guid; }
};

class spell_rog_tricks_of_the_trade : public SpellScript
{
    void DoAfterHit()
    {
        if (Aura* aura = GetHitAura())
            if (auto* script = aura->GetScript<spell_rog_tricks_of_the_trade_aura>("spell_rog_tricks_of_the_trade"))
            {
                if (Unit* explTarget = GetExplTargetUnit())
                    script->SetRedirectTarget(explTarget->GetGUID());
                else
                    script->SetRedirectTarget(ObjectGuid::Empty);
            }
    }

    void Register() override
    {
        AfterHit.Register(&spell_rog_tricks_of_the_trade::DoAfterHit);
    }
};

// 59628 - Tricks of the Trade (Proc)
class spell_rog_tricks_of_the_trade_proc : public AuraScript
{
    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->GetThreatManager().UnregisterRedirectThreat(SPELL_ROGUE_TRICKS_OF_THE_TRADE);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_rog_tricks_of_the_trade_proc::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// -51698 - Honor Among Thieves
class spell_rog_honor_among_thieves : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_HONOR_AMONG_THIEVES_TRIGGERED });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* rogue = GetCaster();
        Unit* actor = eventInfo.GetActor();
        if (!rogue || !rogue->IsPlayer() || !actor || !actor->IsPlayer())
            return false;

        Unit* target = ObjectAccessor::GetUnit(*rogue, rogue->GetTarget());
        return target && rogue->IsValidAttackTarget(target, sSpellMgr->AssertSpellInfo(SPELL_ROGUE_HONOR_AMONG_THIEVES_TRIGGERED));
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (Player* player = caster->ToPlayer())
            if (Unit* target = ObjectAccessor::GetUnit(*player, player->GetTarget()))
                caster->CastSpell(target, SPELL_ROGUE_HONOR_AMONG_THIEVES_TRIGGERED, aurEff);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rog_honor_among_thieves::CheckProc);
        OnEffectProc.Register(&spell_rog_honor_among_thieves::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 76806 - Main Gauche
class spell_rog_main_gauche : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_MAIN_GAUCHE });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Player* player = GetTarget()->ToPlayer();
        if (!player || !eventInfo.GetProcTarget() || !eventInfo.GetProcTarget()->IsAlive() ||
            !player->GetWeaponForAttack(BASE_ATTACK, true))
            return false;

        if (SpellInfo const* spell = eventInfo.GetSpellInfo(); spell && spell->Id == SPELL_ROGUE_MAIN_GAUCHE)
            return false;

        if (!(eventInfo.GetTypeMask() & PROC_FLAG_MAIN_HAND_WEAPON_SWING))
            return false;

        return roll_chance_i(GetEffect(EFFECT_0)->GetAmount());
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Player* caster = GetTarget()->ToPlayer();
        Unit* target = eventInfo.GetProcTarget();
        Item* item = caster->GetWeaponForAttack(BASE_ATTACK, true);
        if (!item || !target || !target->IsAlive())
            return;

        caster->CastSpell(target, SPELL_ROGUE_MAIN_GAUCHE, CastSpellExtraArgs(item).SetTriggeringAura(aurEff));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rog_main_gauche::CheckProc);
        OnEffectProc.Register(&spell_rog_main_gauche::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_rog_glyph_of_hemorrhage : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_GLYPH_HEMORRHAGE_TRIGGERED });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        DamageInfo* damage = eventInfo.GetDamageInfo();
        if (!damage)
            return;

        int32 bp = CalculatePct(damage->GetDamage(), aurEff->GetAmount());
        SpellInfo const* spell = sSpellMgr->AssertSpellInfo(SPELL_ROGUE_GLYPH_HEMORRHAGE_TRIGGERED);
        if (uint8 ticks = spell->GetMaxTicks())
        {
            bp /= ticks;
            GetTarget()->CastSpell(damage->GetVictim(), SPELL_ROGUE_GLYPH_HEMORRHAGE_TRIGGERED, CastSpellExtraArgs(aurEff).AddSpellBP0(bp));
        }
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_rog_glyph_of_hemorrhage::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 2098 - Eviscerate
class spell_rog_eviscerate : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_ROGUE_EVISCERATE_AND_ENVENOM_BONUS_DAMAGE,
                SPELL_ROGUE_REVEALING_STRIKE
            });
    }

    bool Load() override
    {
        if (GetCaster()->GetTypeId() != TYPEID_PLAYER)
            return false;

        if (GetCaster()->ToPlayer()->getClass() != CLASS_ROGUE)
            return false;

        return true;
    }

    void CalculateDamage(Unit* victim, int32& /*damage*/, int32& flatMod, float& pctMod)
    {
        Player const* playerCaster = GetCaster()->ToPlayer();
        uint8 comboPoints = GetSpell()->GetComboPointsUsed();
        flatMod += static_cast<int32>(comboPoints * playerCaster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.091f);

        // Eviscerate and Envenom Bonus Damage (item set effect)
        if (AuraEffect* aurEff = playerCaster->GetAuraEffect(SPELL_ROGUE_EVISCERATE_AND_ENVENOM_BONUS_DAMAGE, EFFECT_0))
            flatMod += comboPoints * aurEff->GetAmount();

        if (AuraEffect* const revealingStrike = victim->GetAuraEffect(SPELL_ROGUE_REVEALING_STRIKE, EFFECT_2, playerCaster->GetGUID()))
            AddPct(pctMod, revealingStrike->GetAmount());
    }

    void HandleFinishers()
    {
        Unit* target = GetHitUnit();
        if (!target || !(GetSpell()->GetHitMask() & (PROC_HIT_NORMAL | PROC_HIT_CRITICAL | PROC_HIT_ABSORB)))
            return;

        ObjectGuid const casterGuid = GetCaster()->GetGUID();
        target->RemoveAurasDueToSpell(SPELL_ROGUE_REVEALING_STRIKE, casterGuid);

        AuraEffect const* blades = GetCaster()->GetDummyAuraEffect(SPELLFAMILY_ROGUE, ROGUE_ICON_ID_SERRATED_BLADES, EFFECT_0);
        if (blades && roll_chance_i(blades->GetAmount() * GetSpell()->GetComboPointsUsed()))
            if (Aura* rupture = target->GetAura(SPELL_ROGUE_RUPTURE, casterGuid))
                rupture->SetDuration(rupture->GetMaxDuration());
    }

    void Register() override
    {
        CalcDamage.Register(&spell_rog_eviscerate::CalculateDamage);
        AfterHit.Register(&spell_rog_eviscerate::HandleFinishers);
    }
};

// 32645 - Envenom
class spell_rog_envenom : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_ROGUE_EVISCERATE_AND_ENVENOM_BONUS_DAMAGE,
                SPELL_ROGUE_REVEALING_STRIKE,
                SPELL_ROGUE_MASTER_POISONER
            });
    }

    bool Load() override
    {
        return (GetCaster()->IsPlayer() && GetCaster()->ToPlayer()->getClass() == CLASS_ROGUE);
    }

    void ChangeDamage(SpellEffIndex /*effIndex*/)
    {
        Unit const* caster = GetCaster();
        Unit const* target = GetHitUnit();
        if (!target)
            return;

        Player const* player = caster->ToPlayer();
        uint8 const comboPoints = GetSpell()->GetComboPointsUsed();
        if (!comboPoints)
            return;

        int32 damage = GetEffectValue();

        // Consume one stack of Deadly Poison per spent combo point to increase the damage dealt by Envenom
        if (AuraEffect const* aurEff = target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x00010000, 0, 0, caster->GetGUID()))
        {
            damage *= std::min<uint8>(comboPoints, aurEff->GetBase()->GetStackAmount());
            damage += int32(player->GetTotalAttackPowerValue(BASE_ATTACK) * 0.09f * comboPoints);
        }

        // Eviscerate and Envenom Bonus Damage (item set effect)
        if (AuraEffect* aurEff = caster->GetAuraEffect(SPELL_ROGUE_EVISCERATE_AND_ENVENOM_BONUS_DAMAGE, EFFECT_0))
            damage += comboPoints * aurEff->GetAmount();

        if (AuraEffect const* revealingStrike = target->GetAuraEffect(SPELL_ROGUE_REVEALING_STRIKE, EFFECT_2, caster->GetGUID()))
            damage += CalculatePct(damage, revealingStrike->GetAmount());

        SetEffectValue(damage);
    }

    void ConsumeOnHit(SpellEffIndex /*effIndex*/)
    {
        Player* caster = GetCaster()->ToPlayer();
        Unit* target = GetHitUnit();
        uint8 const comboPoints = GetSpell()->GetComboPointsUsed();
        if (!target || !target->IsAlive() || !comboPoints || target->IsImmunedToDamage(GetSpellInfo()))
            return;

        if (!caster->HasSpell(SPELL_ROGUE_MASTER_POISONER))
            if (AuraEffect const* poison = target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x00010000, 0, 0, caster->GetGUID()))
                poison->GetBase()->ModStackAmount(-std::min<uint8>(comboPoints, poison->GetBase()->GetStackAmount()));

        target->RemoveAurasDueToSpell(SPELL_ROGUE_REVEALING_STRIKE, caster->GetGUID());
    }

    void Register() override
    {
        OnEffectLaunchTarget.Register(&spell_rog_envenom::ChangeDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        OnEffectHitTarget.Register(&spell_rog_envenom::ConsumeOnHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 5171 - Slice and Dice
class spell_rog_slice_and_dice : public AuraScript
{
    void HandleEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        Duration = aurEff->GetBase()->GetDuration();
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_rog_slice_and_dice::HandleEffectApply, EFFECT_0, SPELL_AURA_MOD_MELEE_HASTE_3, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
public:
    int32 Duration = 0;
};

// -79121 - Deadly Momentum
class spell_rog_deadly_momentum : public AuraScript
{
    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        Unit* target = GetTarget();

        // Slice and Dice duration refreshing
        if (AuraEffect* slice = target->GetAuraEffect(SPELL_AURA_MOD_MELEE_HASTE_3, SPELLFAMILY_ROGUE, 0x00040000, 0, 0))
            if (spell_rog_slice_and_dice* script = slice->GetBase()->GetScript<spell_rog_slice_and_dice>("spell_rog_slice_and_dice"))
                slice->GetBase()->SetDuration(script->Duration);

        // Recuperate duration refreshing
        if (AuraEffect* recuperate = target->GetAuraEffect(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_ROGUE, 0, 0x08000000, 0))
        {
            if (spell_rog_recuperate* script = recuperate->GetBase()->GetScript<spell_rog_recuperate>("spell_rog_recuperate"))
            {
                recuperate->GetBase()->SetDuration(script->Duration);
                recuperate->ResetTicks();
            }
        }
    }

    // Solo al matar a un objetivo que aporte experiencia u honor (4.3.4)
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Player* player = GetTarget()->ToPlayer();
        Unit* victim = eventInfo.GetActionTarget();
        return player && victim && player->isHonorOrXPTarget(victim);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rog_deadly_momentum::CheckProc);
        OnEffectProc.Register(&spell_rog_deadly_momentum::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE);
    }
};

// 6770 - Sap
class spell_rog_sap: public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_BLACKJACK_R1 });
    }

    void HandleBlackJack(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (Aura const* aura = caster->GetAuraOfRankedSpell(SPELL_ROGUE_BLACKJACK_R1))
            caster->CastSpell(GetTarget(), aura->GetSpellInfo()->Id + 1, true);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_rog_sap::HandleBlackJack, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
    }
};

// 408 - Kidney Shot, 8647 - Expose Armor
class spell_rog_revealing_strike_duration : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_REVEALING_STRIKE });
    }

    void ExtendDuration(SpellEffIndex /*effIndex*/)
    {
        Aura* finisher = GetHitAura();
        if (!finisher || finisher->GetDuration() <= 0)
            return;

        AuraEffect const* revealingStrike = GetHitUnit()->GetAuraEffect(SPELL_ROGUE_REVEALING_STRIKE, EFFECT_2, GetCaster()->GetGUID());
        if (!revealingStrike)
            return;

        int32 duration = finisher->GetDuration();
        AddPct(duration, revealingStrike->GetAmount());
        finisher->SetMaxDuration(duration);
        finisher->SetDuration(duration);
        revealingStrike->GetBase()->Remove();
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_rog_revealing_strike_duration::ExtendDuration, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
    }
};

// -14168 - Improved Expose Armor
class spell_rog_improved_expose_armor : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_ROGUE_IMPROVED_EXPOSE_ARMOR,
                SPELL_ROGUE_EXPOSE_ARMOR
            });
    }

    bool Load() override
    {
        return GetUnitOwner()->IsPlayer();
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        // Expose Armor shares its family mask with all other finishing moves so there is no way arround an ID check
        if (!eventInfo.GetSpellInfo() || eventInfo.GetSpellInfo()->Id != SPELL_ROGUE_EXPOSE_ARMOR)
            return false;

        return eventInfo.GetProcTarget();
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Player* player = GetTarget()->ToPlayer();
        player->CastSpell(eventInfo.GetProcTarget(), SPELL_ROGUE_IMPROVED_EXPOSE_ARMOR, CastSpellExtraArgs(true).AddSpellBP0(player->GetComboPoints()));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rog_improved_expose_armor::CheckProc);
        OnEffectProc.Register(&spell_rog_improved_expose_armor::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// -14158 - Murderous Intent
class spell_rog_murderous_intent : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_MURDEROUS_INTENT });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        return eventInfo.GetProcTarget() && eventInfo.GetProcTarget()->GetHealthPct() <= GetSpellInfo()->Effects[EFFECT_1].CalcValue(GetTarget());
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(GetTarget(), SPELL_ROGUE_MURDEROUS_INTENT, CastSpellExtraArgs(aurEff).AddSpellBP0(aurEff->GetAmount()));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rog_murderous_intent::CheckProc);
        OnEffectProc.Register(&spell_rog_murderous_intent::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// -35541 - Combat Potency
class spell_rog_combat_potency : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (SpellInfo const* spell = eventInfo.GetSpellInfo())
            return spell->Id == SPELL_ROGUE_MAIN_GAUCHE;
        if (DamageInfo const* damage = eventInfo.GetDamageInfo())
            return damage->GetAttackType() == OFF_ATTACK;

        return false;
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rog_combat_potency::CheckProc);
    }
};

// -79095 - Restless Blades
class spell_rog_restless_blades : public AuraScript
{
    bool Load() override
    {
        return GetUnitOwner()->IsPlayer();
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_ROGUE_ADRENALIN_RUSH,
                SPELL_ROGUE_KILLING_SPREE,
                SPELL_ROGUE_REDIRECT,
                SPELL_ROGUE_SPRINT
            });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell const* spell = eventInfo.GetProcSpell();
        if (!spell || !spell->GetComboPointsUsed())
            return false;

        switch (spell->GetSpellInfo()->Id)
        {
            case 1943:  // Rupture
            case 2098:  // Eviscerate
            case 26679: // Deadly Throw
            case 32645: // Envenom
                return true;
            default:
                return false;
        }
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Player* player = GetTarget()->ToPlayer();
        int32 const cooldownReduction = -eventInfo.GetProcSpell()->GetComboPointsUsed() * aurEff->GetAmount();
        player->GetSpellHistory()->ModifyCooldown(SPELL_ROGUE_ADRENALIN_RUSH, cooldownReduction);
        player->GetSpellHistory()->ModifyCooldown(SPELL_ROGUE_KILLING_SPREE, cooldownReduction);
        player->GetSpellHistory()->ModifyCooldown(SPELL_ROGUE_REDIRECT, cooldownReduction);
        player->GetSpellHistory()->ModifyCooldown(SPELL_ROGUE_SPRINT, cooldownReduction);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rog_restless_blades::CheckProc);
        OnEffectProc.Register(&spell_rog_restless_blades::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// -84652 - Bandit's Guile
class spell_rog_bandits_guile : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_ROGUE_SHALLOW_INSIGHT,
                SPELL_ROGUE_MODERATE_INSIGHT,
                SPELL_ROGUE_DEEP_INSIGHT,
                SPELL_ROGUE_BANDITS_GUILE
            });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (GetTarget()->HasAura(SPELL_ROGUE_DEEP_INSIGHT, GetTarget()->GetGUID()))
            return false;

        return eventInfo.GetProcTarget();
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* target = GetTarget();
        Unit* procTarget = eventInfo.GetProcTarget();
        uint32 spellId = SPELL_ROGUE_SHALLOW_INSIGHT;
        int32 basepoints = 10;

        // Todo: validate if the required number of successful procs itself changes or if the proc chance of lower ranks is taking care of it already.
        uint8 const neededProcs = 4;

        // We are striking a new opponent, reset progress
        if (_recentTargetGUID != procTarget->GetGUID())
        {
            target->RemoveAurasDueToSpell(SPELL_ROGUE_SHALLOW_INSIGHT, target->GetGUID());
            target->RemoveAurasDueToSpell(SPELL_ROGUE_MODERATE_INSIGHT, target->GetGUID());
            target->RemoveAurasDueToSpell(SPELL_ROGUE_DEEP_INSIGHT, target->GetGUID());
            _recentTargetGUID = procTarget->GetGUID();
            _procStrikes = 0;
        }

        _procStrikes = std::min<uint8>(_procStrikes + 1, neededProcs * 3);

        if (_procStrikes < neededProcs)
            return;

        // We are increasing our insight on the opponent
        if (_procStrikes >= neededProcs * 3)
        {
            spellId = SPELL_ROGUE_DEEP_INSIGHT;
            basepoints = 30;
            _procStrikes = 0;
        }
        else if (_procStrikes >= neededProcs * 2)
        {
            spellId = SPELL_ROGUE_MODERATE_INSIGHT;
            basepoints = 20;
        }

        if (spellId == SPELL_ROGUE_DEEP_INSIGHT && target->HasAura(SPELL_ROGUE_MODERATE_INSIGHT, target->GetGUID()))
            target->RemoveAurasDueToSpell(SPELL_ROGUE_MODERATE_INSIGHT, target->GetGUID());
        else if (spellId == SPELL_ROGUE_MODERATE_INSIGHT && target->HasAura(SPELL_ROGUE_SHALLOW_INSIGHT, target->GetGUID()))
            target->RemoveAurasDueToSpell(SPELL_ROGUE_SHALLOW_INSIGHT, target->GetGUID());

        target->CastSpell(target, spellId);
        target->CastSpell(procTarget, SPELL_ROGUE_BANDITS_GUILE, CastSpellExtraArgs().AddSpellBP0(basepoints).AddSpellMod(SPELLVALUE_BASE_POINT1, basepoints));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rog_bandits_guile::CheckProc);
        OnEffectProc.Register(&spell_rog_bandits_guile::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }

public:
    // Redirect (73981) tooltip: "...as well as any insight gained from Bandit's Guile".
    // Keep the progress and move the Bandit's Guile debuff from the previous opponent to the new one.
    void RedirectTo(Unit* rogue, Unit* newTarget)
    {
        if (!_recentTargetGUID || _recentTargetGUID == newTarget->GetGUID())
            return;

        if (Unit* oldTarget = ObjectAccessor::GetUnit(*rogue, _recentTargetGUID))
            oldTarget->RemoveAurasDueToSpell(SPELL_ROGUE_BANDITS_GUILE, rogue->GetGUID());

        _recentTargetGUID = newTarget->GetGUID();

        static constexpr std::array<std::pair<uint32, int32>, 3> insights = {{
            { SPELL_ROGUE_DEEP_INSIGHT, 30 },
            { SPELL_ROGUE_MODERATE_INSIGHT, 20 },
            { SPELL_ROGUE_SHALLOW_INSIGHT, 10 }
        }};

        for (auto const& [spellId, amount] : insights)
        {
            Aura const* insight = rogue->GetAura(spellId, rogue->GetGUID());
            if (!insight || insight->GetDuration() <= 0)
                continue;

            rogue->CastSpell(newTarget, SPELL_ROGUE_BANDITS_GUILE, CastSpellExtraArgs(true)
                .AddSpellBP0(amount)
                .AddSpellMod(SPELLVALUE_BASE_POINT1, amount)
                .AddSpellMod(SPELLVALUE_DURATION, insight->GetDuration()));
            break;
        }
    }

private:
    ObjectGuid _recentTargetGUID;
    uint8 _procStrikes = 0;
};

// 73981 - Redirect
// "Transfers any existing combo points to the current enemy target, as well as any insight gained from
// Bandit's Guile. Requires active combo points." The spell's only effect adds 0 combo points, so without
// this script it consumed the combo points and did nothing.
class spell_rog_redirect : public SpellScript
{
    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster->GetComboPoints())
            return SPELL_FAILED_NO_COMBO_POINTS;

        if (Unit* target = GetExplTargetUnit())
            if (target->GetGUID() == caster->GetComboTarget())
                return SPELL_FAILED_BAD_TARGETS;

        return SPELL_CAST_OK;
    }

    void StoreComboPoints()
    {
        // The core clears the caster's combo points at the end of target processing (the spell requires
        // combo points), so remember them before that happens.
        _comboPoints = GetCaster()->GetComboPoints();
    }

    void HandleHit(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
        if (Unit* target = GetHitUnit())
            _newTargetGUID = target->GetGUID();
    }

    void TransferComboPoints()
    {
        Unit* caster = GetCaster();
        Unit* target = ObjectAccessor::GetUnit(*caster, _newTargetGUID);
        if (!target || !_comboPoints)
            return;

        caster->AddComboPoints(target, _comboPoints);

        if (Aura* guile = caster->GetAuraOfRankedSpell(SPELL_ROGUE_BANDITS_GUILE_R1))
            if (spell_rog_bandits_guile* script = guile->GetScript<spell_rog_bandits_guile>("spell_rog_bandits_guile"))
                script->RedirectTo(caster, target);
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_rog_redirect::CheckCast);
        BeforeCast.Register(&spell_rog_redirect::StoreComboPoints);
        OnEffectHitTarget.Register(&spell_rog_redirect::HandleHit, EFFECT_0, SPELL_EFFECT_ADD_COMBO_POINTS);
        AfterCast.Register(&spell_rog_redirect::TransferComboPoints);
    }

    int8 _comboPoints = 0;
    ObjectGuid _newTargetGUID;
};

// 76577 - Smoke Bomb
// "Creates a cloud of thick smoke in a 8 yard radius around the Rogue for 5 sec. Enemies are unable to target
// into or out of the smoke cloud." The cloud (persistent area aura, dummy) covers every unit inside it; each of
// them carries 88611 (SPELL_AURA_INTERFERE_TARGETTING) from the rogue while inside, which is what the targeting
// checks look at (Unit::IsTargetingInterferedWith).
class spell_rog_smoke_bomb : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_SMOKE_BOMB_INTERFERE });
    }

    void HandleEnterCloud(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* rogue = GetCaster();
        if (!rogue)
            return;

        if (Aura* interference = rogue->AddAura(SPELL_ROGUE_SMOKE_BOMB_INTERFERE, GetTarget()))
        {
            // lasts exactly as long as the unit stays in the cloud
            interference->SetMaxDuration(-1);
            interference->SetDuration(-1);
        }
    }

    void HandleLeaveCloud(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_ROGUE_SMOKE_BOMB_INTERFERE, GetCasterGUID());
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_rog_smoke_bomb::HandleEnterCloud, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove.Register(&spell_rog_smoke_bomb::HandleLeaveCloud, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 2094 - Blind
// Glyph of Blind (91299): "Your Blind ability also removes all damage over time effects from the target."
class spell_rog_blind : public AuraScript
{
    void HandleGlyph(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->HasAura(SPELL_ROGUE_GLYPH_OF_BLIND))
            return;

        Unit* target = GetTarget();
        target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
        target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
        target->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_rog_blind::HandleGlyph, EFFECT_1, SPELL_AURA_MOD_CONFUSE, AURA_EFFECT_HANDLE_REAL);
    }
};

// 14183 - Premeditation
class spell_rog_premeditation : public AuraScript
{
    void CountTemporaryPoints(AuraEffect const* /*effect*/, int32& amount, bool& canBeRecalculated)
    {
        // The combo point gain is finalized after effects have been processed.
        // The holder still has only its previous points when this aura is built.
        amount = std::clamp(5 - int32(GetUnitOwner()->GetComboPoints()), 0, amount);
        canBeRecalculated = false;
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_rog_premeditation::CountTemporaryPoints, EFFECT_1, SPELL_AURA_RETAIN_COMBO_POINTS);
    }
};

// 14181 - Relentless Strikes
class spell_rog_relentless_strikes : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_RELENTLESS_STRIKES_ENERGIZE });
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(caster, SPELL_ROGUE_RELENTLESS_STRIKES_ENERGIZE);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_rog_relentless_strikes::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 1776 - Gouge
class spell_rog_gouge : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* actor = eventInfo.GetActor();
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (!actor || actor != GetCaster() || !spellInfo || !(eventInfo.GetTypeMask() & PROC_FLAG_TAKE_PERIODIC))
            return true;

        // The Glyph of Hemorrhage places the bleed mechanic on its aura effect.
        if (!(spellInfo->GetAllEffectsMechanicMask() & (1 << MECHANIC_BLEED)))
            return true;

        AuraEffect const* talent = actor->GetDummyAuraEffect(SPELLFAMILY_ROGUE, ROGUE_ICON_ID_SANGUINARY_VEIN, EFFECT_1);
        return !talent || !roll_chance_i(talent->GetAmount());
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rog_gouge::CheckProc);
    }
};

// 16511 - Hemorrhage
class spell_rog_hemorrhage : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    void CalculateDamage(Unit* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod)
    {
        for (WeaponAttackType attType : { BASE_ATTACK, OFF_ATTACK })
        {
            // Dealing additional 45% damage when a dagger is equipped
            if (Item* item = GetCaster()->ToPlayer()->GetWeaponForAttack(attType, true))
            {
                if (item->GetTemplate()->GetSubClass() == ITEM_SUBCLASS_WEAPON_DAGGER)
                {
                    pctMod *= 1.45f;
                    return;
                }
            }
        }
    }

    void Register() override
    {
        CalcDamage.Register(&spell_rog_hemorrhage::CalculateDamage);
    }
};


// 1766 - Kick
// Glyph of Kick (56805): si Patada interrumpe con éxito un hechizo, su tiempo de reutilización se reduce 6 s (4.3.4).
class spell_rog_kick : public SpellScript
{
    void HandleGlyph()
    {
        if (!(GetSpell()->GetHitMask() & PROC_HIT_INTERRUPT))
            return;

        Unit* caster = GetCaster();
        if (AuraEffect const* glyph = caster->GetAuraEffect(56805, EFFECT_1))
            caster->GetSpellHistory()->ModifyCooldown(GetSpellInfo()->Id, -glyph->GetAmount());
    }

    void Register() override
    {
        AfterHit.Register(&spell_rog_kick::HandleGlyph);
    }
};

// 74001 - Combat Readiness
class spell_rog_combat_readiness : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ spellInfo->Effects[EFFECT_0].TriggerSpell });
    }

    void Start(AuraEffect const* effect, AuraEffectHandleModes /*mode*/)
    {
        _inactivity = sSpellMgr->AssertSpellInfo(effect->GetSpellInfo()->Effects[EFFECT_0].TriggerSpell)->GetDuration();
        _expiresAt = GameTime::GetGameTimeSteadyPoint() + Milliseconds(GetAura()->GetMaxDuration());
        RefreshAfterHit();
    }

    void RefreshAfterHit()
    {
        auto remaining = std::chrono::duration_cast<Milliseconds>(_expiresAt - GameTime::GetGameTimeSteadyPoint());
        GetAura()->SetDuration(std::clamp(static_cast<int32>(remaining.count()), 0, _inactivity));
    }

    void HandleWeaponHit(AuraEffect const* /*effect*/, ProcEventInfo& /*eventInfo*/)
    {
        RefreshAfterHit();
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_rog_combat_readiness::Start, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        AfterEffectProc.Register(&spell_rog_combat_readiness::HandleWeaponHit, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }

    int32 _inactivity = 0;
    std::chrono::steady_clock::time_point _expiresAt;
};

// 11327 - Vanish
class spell_rog_vanish : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_STEALTH });
    }

    void ClearTracking(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasByType(SPELL_AURA_MOD_STALKED);
    }

    void EnterStealth(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Applying ordinary Stealth earlier would replace Vanish's shapeshift aura.
        if (!GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::Expired))
            return;

        Unit* unitTarget = GetTarget();
        if (!unitTarget->IsAlive() || unitTarget->HasAura(SPELL_ROGUE_STEALTH))
            return;

        // Reset cooldown on stealth if needed
        if (unitTarget->GetSpellHistory()->HasCooldown(SPELL_ROGUE_STEALTH))
            unitTarget->GetSpellHistory()->ResetCooldown(SPELL_ROGUE_STEALTH);

        unitTarget->CastSpell(nullptr, SPELL_ROGUE_STEALTH, true);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_rog_vanish::ClearTracking, EFFECT_1, SPELL_AURA_MOD_STEALTH, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        AfterEffectRemove.Register(&spell_rog_vanish::EnterStealth, EFFECT_1, SPELL_AURA_MOD_STEALTH, AURA_EFFECT_HANDLE_REAL);
    }
};

// 921 - Pick Pocket
class spell_rog_pickpocket : public SpellScript
{
    SpellCastResult CheckCast()
    {
        if (!GetExplTargetUnit() || !GetCaster()->IsValidAttackTarget(GetExplTargetUnit(), GetSpellInfo()))
            return SPELL_FAILED_BAD_TARGETS;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_rog_pickpocket::CheckCast);
    }
};

// -79133 - Venomous Wounds
class spell_rogue_venomous_wounds : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_VENOMOUS_WOUND, SPELL_ROGUE_VENOMOUS_VIM });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* victim = eventInfo.GetProcTarget();
        if (!victim || !eventInfo.GetDamageInfo() || !eventInfo.GetDamageInfo()->GetDamage())
            return false;

        // Deadly Poison
        if (victim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x0, 0x80000, 0x0, GetTarget()->GetGUID()))
            return true;

        // Wound Poison
        if (victim->GetAuraEffect(SPELL_AURA_MOD_HEALING_PCT, SPELLFAMILY_ROGUE, 0x0, 0x80000, 0x0, GetTarget()->GetGUID()))
            return true;

        // Crippling Poison
        if (victim->GetAuraEffect(SPELL_AURA_MOD_DECREASE_SPEED, SPELLFAMILY_ROGUE, 0x0, 0x80000, 0x0, GetTarget()->GetGUID()))
            return true;

        // Mind-Numbing Poison
        if (victim->GetAuraEffect(SPELL_AURA_HASTE_SPELLS, SPELLFAMILY_ROGUE, 0x0, 0x80000, 0x0, GetTarget()->GetGUID()))
            return true;

        return false;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_ROGUE_VENOMOUS_WOUND, aurEff);
        if (AuraEffect const* energizeEffect = GetEffect(EFFECT_1))
            GetTarget()->CastSpell(GetTarget(), SPELL_ROGUE_VENOMOUS_VIM, CastSpellExtraArgs(aurEff).AddSpellBP0(energizeEffect->GetAmount()));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_rogue_venomous_wounds::CheckProc);
        OnEffectProc.Register(&spell_rogue_venomous_wounds::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};
}

namespace Spells::Rogue
{
// 51723 - Fan of Knives. Vile Poisons (16513-16515, effect 2): "your Fan of Knives has a 33% / 67% / 100% of the normal
// chance to apply poisons from your equipped melee weapons". Fan of Knives is a thrown attack, so the core only rolls the
// ranged slot's enchantments: roll the main hand and off hand poisons here.
class spell_rog_fan_of_knives : public SpellScript
{
    void HandleVilePoisons(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster()->ToPlayer();
        Unit* target = GetHitUnit();
        if (!player || !target || !target->IsAlive())
            return;

        AuraEffect const* vilePoisons = player->GetDummyAuraEffect(SPELLFAMILY_ROGUE, 857, EFFECT_2);
        if (!vilePoisons)
            return;

        for (WeaponAttackType attack : { BASE_ATTACK, OFF_ATTACK })
        {
            Item* item = player->GetWeaponForAttack(attack, true);
            if (!item)
                continue;

            SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT));
            if (!enchant)
                continue;

            for (uint8 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
            {
                if (enchant->Effect[i] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                    continue;

                SpellInfo const* poison = sSpellMgr->GetSpellInfo(enchant->EffectArg[i]);
                if (!poison || poison->SpellFamilyName != SPELLFAMILY_ROGUE || poison->Dispel != DISPEL_POISON)
                    continue;

                float chance = enchant->EffectPointsMin[i] != 0 ? float(enchant->EffectPointsMin[i]) : player->GetWeaponProcChance();
                if (SpellEnchantProcEntry const* entry = sSpellMgr->GetSpellEnchantProcEvent(enchant->ID))
                {
                    if (entry->ProcsPerMinute != 0.f)
                        chance = player->GetPPMProcChance(item->GetTemplate()->GetDelay(), entry->ProcsPerMinute, poison);
                    else if (entry->Chance != 0.f)
                        chance = entry->Chance;
                }

                player->ApplySpellMod(poison, SpellModOp::ProcChance, chance);
                chance = chance * vilePoisons->GetAmount() / 100.0f;

                if (roll_chance_f(chance))
                    player->CastSpell(poison->IsPositive() ? player : target, poison->Id, CastSpellExtraArgs(item));
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_rog_fan_of_knives::HandleVilePoisons, EFFECT_0, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
    }
};
}

void AddSC_rogue_spell_scripts()
{
    using namespace Spells::Rogue;
    RegisterSpellScript(spell_rog_fan_of_knives);
    RegisterSpellScript(spell_rog_bandits_guile);
    RegisterSpellScript(spell_rog_redirect);
    RegisterSpellScript(spell_rog_blind);
    RegisterSpellScript(spell_rog_smoke_bomb);
    RegisterSpellScript(spell_rog_blade_flurry);
    RegisterSpellScript(spell_rog_cheat_death);
    RegisterSpellScript(spell_rog_crippling_poison);
    RegisterSpellScript(spell_rog_cut_to_the_chase);
    RegisterSpellScript(spell_rog_combat_potency);
    RegisterSpellScript(spell_rog_combat_readiness);
    RegisterSpellScript(spell_rog_deadly_momentum);
    RegisterSpellScript(spell_rog_deadly_poison);
    RegisterSpellScript(spell_rog_envenom);
    RegisterSpellScript(spell_rog_eviscerate);
    RegisterSpellScript(spell_rog_glyph_of_hemorrhage);
    RegisterSpellScript(spell_rog_gouge);
    RegisterSpellScript(spell_rog_hemorrhage);
    RegisterSpellScript(spell_rog_improved_expose_armor);
    new spell_rog_killing_spree();
    RegisterSpellScript(spell_rog_main_gauche);
    RegisterSpellScript(spell_rog_master_of_subtlety);
    RegisterSpellScript(spell_rog_murderous_intent);
    RegisterSpellScript(spell_rog_overkill);
    RegisterSpellScript(spell_rog_pickpocket);
    RegisterSpellScript(spell_rog_preparation);
    RegisterSpellScript(spell_rog_prey_on_the_weak);
    RegisterSpellScript(spell_rog_premeditation);
    RegisterSpellScript(spell_rog_recuperate);
    RegisterSpellScript(spell_rog_relentless_strikes);
    RegisterSpellScript(spell_rog_restless_blades);
    RegisterSpellScript(spell_rog_revealing_strike_duration);
    RegisterSpellScript(spell_rog_rupture);
    RegisterSpellScript(spell_rog_sap);
    RegisterSpellScript(spell_rog_shiv);
    RegisterSpellScript(spell_rog_slice_and_dice);
    RegisterSpellScript(spell_rog_stealth);
    RegisterSpellAndAuraScriptPair(spell_rog_tricks_of_the_trade, spell_rog_tricks_of_the_trade_aura);
    RegisterSpellScript(spell_rog_tricks_of_the_trade_proc);
    RegisterSpellScript(spell_rog_honor_among_thieves);
    RegisterSpellScript(spell_rog_vanish);
    RegisterSpellScript(spell_rog_kick);
    RegisterSpellScript(spell_rogue_venomous_wounds);
}
