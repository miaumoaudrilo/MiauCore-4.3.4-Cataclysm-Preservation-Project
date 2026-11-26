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
 * Scriptnames of files in this file should be prefixed with "npc_pet_pal_".
 */

#include "ScriptMgr.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "MotionMaster.h"
#include "PetAI.h"
#include "ScriptedCreature.h"

namespace Pets::Paladin
{
enum GuardianOfAncientKings
{
    SPELL_ANCIENT_GUARDIAN                      = 86657,
    NPC_GUARDIAN_OF_ANCIENT_KINGS_PROTECTION    = 46490
};

struct npc_pet_pal_guardian_of_ancient_kings : public PetAI
{
    npc_pet_pal_guardian_of_ancient_kings(Creature* creature) : PetAI(creature)
    {
        Initialize();
    }

    void Initialize()
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void IsSummonedBy(Unit* summoner) override
    {
        if (me->GetEntry() == NPC_GUARDIAN_OF_ANCIENT_KINGS_PROTECTION)
        {
            me->AddUnitState(UNIT_STATE_ROOT);
            DoCast(summoner, SPELL_ANCIENT_GUARDIAN);
        }
    }
};

// 46506 - Guardian of Ancient Kings (Retribution, summoned by 86698)
// Tooltip: "The Guardian of Ancient Kings attacks your current enemy." It had no script and used the default guardian AI,
// which attacks whatever is nearby (#69). It now attacks the paladin's current target and follows target switches.
struct npc_pet_pal_guardian_of_ancient_kings_retribution : public ScriptedAI
{
    npc_pet_pal_guardian_of_ancient_kings_retribution(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE); // never pick targets on its own
    }

    Unit* GetOwnerTarget() const
    {
        Unit* owner = me->GetOwner();
        if (!owner)
            return nullptr;

        Unit* target = owner->GetVictim();
        if (!target)
            if (Player* player = owner->ToPlayer())
                target = player->GetSelectedUnit();

        if (target && target->IsAlive() && me->IsValidAttackTarget(target))
            return target;
        return nullptr;
    }

    void IsSummonedBy(Unit* /*summoner*/) override
    {
        if (Unit* target = GetOwnerTarget())
            AttackStart(target);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        // Do not run back to a home position: go back to the paladin and wait for the next target
        me->CombatStop(true);
        if (Unit* owner = me->GetOwner())
            me->GetMotionMaster()->MoveFollow(owner, 0.0f, 0.0f);
    }

    void UpdateAI(uint32 diff) override
    {
        _checkTimer += diff;
        if (_checkTimer >= 500)
        {
            _checkTimer = 0;
            Unit* target = GetOwnerTarget();
            if (target && target != me->GetVictim())
                AttackStart(target); // the paladin switched targets
            else if (!target && !me->GetVictim())
                if (Unit* owner = me->GetOwner())
                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
                        me->GetMotionMaster()->MoveFollow(owner, 0.0f, 0.0f);
        }

        if (!me->GetVictim() || !me->GetVictim()->IsAlive())
            return;

        DoMeleeAttackIfReady();
    }

private:
    uint32 _checkTimer = 0;
};
}

void AddSC_paladin_pet_script()
{
    using namespace Pets::Paladin;
    RegisterCreatureAI(npc_pet_pal_guardian_of_ancient_kings);
    RegisterCreatureAI(npc_pet_pal_guardian_of_ancient_kings_retribution);
}
