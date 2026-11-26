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
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "npc_pet_pri_".
 */

#include "ScriptMgr.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "MotionMaster.h"
#include "ScriptedCreature.h"
#include "PassiveAI.h"
#include "PetAI.h"

namespace Pets::Priest
{
enum PriestSpells
{
    SPELL_PRIEST_GLYPH_OF_SHADOWFIEND       = 58228,
    SPELL_PRIEST_SHADOWFIEND_DEATH          = 57989,
    SPELL_PRIEST_LIGHTWELL_CHARGES          = 59907,

    SPELL_PRIEST_SHADOW_WORD_PAIN           = 589,
    SPELL_PRIEST_SHADOWY_APPARITION_CLONE   = 87213, // Summon Shadowy Apparition Spawn Effect (clone caster)
    SPELL_PRIEST_SHADOWY_APPARITION_DAMAGE  = 87532
};

// 46954 - Shadowy Apparition (summoned by 87426, see spell_pri_shadow_word_pain)
// "...a shadow version of yourself that will slowly move towards a target which is afflicted by your Shadow Word: Pain.
// Once reaching the target, it will instantly deal shadow damage." (#53)
struct npc_pet_pri_shadowy_apparition : public ScriptedAI
{
    npc_pet_pri_shadowy_apparition(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void IsSummonedBy(Unit* summoner) override
    {
        _ownerGUID = summoner->GetGUID();
        summoner->CastSpell(me, SPELL_PRIEST_SHADOWY_APPARITION_CLONE, true); // looks like the priest
        me->SetWalk(true);                                                     // "slowly moves"
        SelectNewTarget();
    }

    void AttackStart(Unit* /*who*/) override { }
    void MoveInLineOfSight(Unit* /*who*/) override { }

    void UpdateAI(uint32 diff) override
    {
        _checkTimer += diff;
        if (_checkTimer < 200)
            return;
        _checkTimer = 0;

        Unit* owner = ObjectAccessor::GetUnit(*me, _ownerGUID);
        if (!owner)
        {
            me->DespawnOrUnsummon();
            return;
        }

        Unit* target = ObjectAccessor::GetUnit(*me, _targetGUID);
        if (!IsValidTarget(target, owner))
        {
            if (!SelectNewTarget())
                me->DespawnOrUnsummon();
            return;
        }

        if (me->IsWithinDistInMap(target, 2.0f))
        {
            // Damage belongs to the priest (spell power, combat log), dealt from the apparition's position
            me->CastSpell(target, SPELL_PRIEST_SHADOWY_APPARITION_DAMAGE, CastSpellExtraArgs(true).SetOriginalCaster(_ownerGUID));
            _targetGUID.Clear();
            me->DespawnOrUnsummon(500ms);
        }
    }

private:
    bool IsValidTarget(Unit* target, Unit* owner) const
    {
        return target && target->IsAlive() && target->IsInMap(me) && target->HasAura(SPELL_PRIEST_SHADOW_WORD_PAIN, owner->GetGUID());
    }

    bool SelectNewTarget()
    {
        Unit* owner = ObjectAccessor::GetUnit(*me, _ownerGUID);
        if (!owner)
            return false;

        std::list<Unit*> targets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(me, owner, 40.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(me, targets, check);
        Cell::VisitAllObjects(me, searcher, 40.0f);
        targets.remove_if([&](Unit* unit) { return !IsValidTarget(unit, owner); });
        if (targets.empty())
            return false;

        targets.sort(Trinity::ObjectDistanceOrderPred(me));
        Unit* target = targets.front();
        _targetGUID = target->GetGUID();
        me->GetMotionMaster()->MoveFollow(target, 0.0f, 0.0f);
        return true;
    }

    ObjectGuid _ownerGUID;
    ObjectGuid _targetGUID;
    uint32 _checkTimer = 0;
};

class npc_pet_pri_lightwell : public CreatureScript
{
    public:
        npc_pet_pri_lightwell() : CreatureScript("npc_pet_pri_lightwell") { }

        struct npc_pet_pri_lightwellAI : public PassiveAI
        {
            npc_pet_pri_lightwellAI(Creature* creature) : PassiveAI(creature)
            {
                DoCast(me, SPELL_PRIEST_LIGHTWELL_CHARGES, false);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                if (!me->IsAlive())
                    return;

                me->CombatStop(true);
                EngagementOver();
                me->ResetPlayerDamageReq();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_pri_lightwellAI(creature);
        }
};

class npc_pet_pri_shadowfiend : public CreatureScript
{
    public:
        npc_pet_pri_shadowfiend() : CreatureScript("npc_pet_pri_shadowfiend") { }

        struct npc_pet_pri_shadowfiendAI : public PetAI
        {
            npc_pet_pri_shadowfiendAI(Creature* creature) : PetAI(creature) { }

            void IsSummonedBy(Unit* summoner) override
            {
                if (summoner->HasAura(SPELL_PRIEST_GLYPH_OF_SHADOWFIEND))
                    DoCastAOE(SPELL_PRIEST_SHADOWFIEND_DEATH);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_pri_shadowfiendAI(creature);
        }
};
}

void AddSC_priest_pet_scripts()
{
    using namespace Pets::Priest;
    new npc_pet_pri_lightwell();
    new npc_pet_pri_shadowfiend();
    RegisterCreatureAI(npc_pet_pri_shadowy_apparition);
}
