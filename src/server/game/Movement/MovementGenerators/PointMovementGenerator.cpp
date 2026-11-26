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

#include "PointMovementGenerator.h"
#include "CreatureAI.h"
#include "Creature.h"
#include "Player.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "G3DPosition.hpp"
#include "ObjectAccessor.h"
#include "PathGenerator.h"
#include "World.h"

//----- Point Movement Generator

template<class T>
void PointMovementGenerator<T>::DoInitialize(T* owner)
{
    if (_movementId == EVENT_CHARGE_PREPATH)
    {
        owner->AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
        return;
    }

    owner->AddUnitState(UNIT_STATE_ROAMING);

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
    {
        _interrupt = true;
        owner->StopMoving();
        return;
    }

    owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);

    Movement::MoveSplineInit init(owner);
    init.MoveTo(_x, _y, _z, _generatePath);
    if (_speed > 0.0f)
        init.SetVelocity(_speed);
    init.Launch();

    // Call for creature group update
    if (Creature* creature = owner->ToCreature())
        creature->SignalFormationMovement();
}

template<class T>
void PointMovementGenerator<T>::UpdateChargeTarget(T* owner, uint32 diff)
{
    static constexpr uint32 CHARGE_CHECK_INTERVAL = 100;
    static constexpr uint8 CHARGE_MAX_REPATHS = 5;

    if (_chargeTarget.IsEmpty() || _chargeRepaths >= CHARGE_MAX_REPATHS || owner->movespline->Finalized())
        return;

    if (_chargeCheckTimer > diff)
    {
        _chargeCheckTimer -= diff;
        return;
    }
    _chargeCheckTimer = CHARGE_CHECK_INTERVAL;

    Unit* target = ObjectAccessor::GetUnit(*owner, _chargeTarget);
    if (!target || !target->IsInWorld() || !target->IsAlive() || !target->IsInMap(owner))
    {
        _chargeTarget.Clear();
        return;
    }

    // still close to the destination we are running to: nothing to do
    float const reach = target->GetCombatReach();
    if (target->GetExactDist(_x, _y, _z) <= reach + 2.0f)
        return;

    PathGenerator path(owner);
    if (_chargeMaxPathLength > 0.0f)
        path.SetPathLengthLimit(_chargeMaxPathLength);

    bool const result = path.CalculatePath(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), false);
    if (!result || (path.GetPathType() & (PATHFIND_NOPATH | PATHFIND_INCOMPLETE | PATHFIND_SHORT)) || path.IsInvalidDestinationZ(target))
    {
        // the target got out of reach (blink behind a wall, too far...): finish the charge where it was going
        _chargeTarget.Clear();
        return;
    }

    path.ShortenPathUntilDist(PositionToVector3(target), reach);
    G3D::Vector3 const dest = path.GetActualEndPosition();
    _x = dest.x;
    _y = dest.y;
    _z = dest.z;
    ++_chargeRepaths;

    Movement::MoveSplineInit init(owner);
    init.MovebyPath(path.GetPath());
    if (_speed > 0.0f)
        init.SetVelocity(_speed);
    init.Launch();
}

template<class T>
bool PointMovementGenerator<T>::DoUpdate(T* owner, uint32 diff)
{
    if (!owner)
        return false;

    if (_movementId == EVENT_CHARGE_PREPATH)
    {
        UpdateChargeTarget(owner, diff);
        return !owner->movespline->Finalized();
    }

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || (owner->IsMovementPreventedByCasting() && _movementId != EVENT_CHARGE))
    {
        _interrupt = true;
        owner->StopMoving();
        return true;
    }

    if ((_interrupt && owner->movespline->Finalized()) || (_recalculateSpeed && !owner->movespline->Finalized()))
    {
        _recalculateSpeed = false;
        _interrupt = false;

        owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);

        Movement::MoveSplineInit init(owner);
        init.MoveTo(_x, _y, _z, _generatePath);
        if (_speed > 0.0f) // Default value for point motion type is 0.0, if 0.0 spline will use GetSpeed on unit
            init.SetVelocity(_speed);
        init.Launch();

        // Call for creature group update
        if (Creature* creature = owner->ToCreature())
            creature->SignalFormationMovement();
    }

    return !owner->movespline->Finalized();
}

template<class T>
void PointMovementGenerator<T>::DoFinalize(T* owner)
{
    owner->ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);

    if (owner->movespline->Finalized())
        MovementInform(owner);
}

template<class T>
void PointMovementGenerator<T>::DoReset(T* owner)
{
    owner->StopMoving();
    DoInitialize(owner);
}

template<class T>
void PointMovementGenerator<T>::MovementInform(T*) { }

template <>
void PointMovementGenerator<Creature>::MovementInform(Creature* owner)
{
    if (owner->AI())
        owner->AI()->MovementInform(POINT_MOTION_TYPE, _movementId);
}

template void PointMovementGenerator<Player>::DoInitialize(Player*);
template void PointMovementGenerator<Creature>::DoInitialize(Creature*);
template void PointMovementGenerator<Player>::DoFinalize(Player*);
template void PointMovementGenerator<Creature>::DoFinalize(Creature*);
template void PointMovementGenerator<Player>::DoReset(Player*);
template void PointMovementGenerator<Creature>::DoReset(Creature*);
template bool PointMovementGenerator<Player>::DoUpdate(Player*, uint32);
template bool PointMovementGenerator<Creature>::DoUpdate(Creature*, uint32);

//---- AssistanceMovementGenerator

void AssistanceMovementGenerator::Finalize(Unit* owner)
{
    owner->ClearUnitState(UNIT_STATE_ROAMING);
    owner->StopMoving();
    owner->ToCreature()->SetNoCallAssistance(false);
    owner->ToCreature()->CallAssistance();
    if (owner->IsAlive())
        owner->GetMotionMaster()->MoveSeekAssistanceDistract(sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY));
}
