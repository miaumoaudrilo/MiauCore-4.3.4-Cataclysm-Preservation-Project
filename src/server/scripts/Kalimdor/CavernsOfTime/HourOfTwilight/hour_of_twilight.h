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

#ifndef DEF_HOUROFTWILIGHT_H
#define DEF_HOUROFTWILIGHT_H

#include "CreatureAIImpl.h"
#include "Define.h"

namespace HourOfTwilight
{
constexpr char const* DataHeader = "HOT";
#define HOTScriptName "instance_hour_of_twilight"

uint32 const EncounterCount = 3;

enum HOTDataTypes
{
    // Bosses
    DATA_ARCURION               = 0,
    DATA_ASIRA_DAWNSLAYER       = 1,
    DATA_ARCHBISHOP_BENEDICTUS  = 2,
    DATA_THRALL_ARCURION,
    DATA_THRALL_ASIRA,
    DATA_THRALL_BENEDICTUS,
    DATA_ARCURION_READY,
    DATA_ASIRA_READY,
    DATA_BENEDICTUS_READY,
    DATA_BENEDICTUS_INTRO
};

enum HOTCreatures
{
    NPC_ARCURION = 54590,
    NPC_ASIRA = 54968,
    NPC_BENEDICTUS = 54938,
    NPC_THRALL_ENTRANCE = 54548,
    NPC_THRALL_ARCURION = 55779,
    NPC_THRALL_ASIRA = 54972,
    NPC_THRALL_GAUNTLET = 54634,
    NPC_THRALL_BENEDICTUS = 54971
};

enum HOTGameObjectIds
{
    GO_ARCURION_EXIT = 210048,
    GO_ARCURION_ENTRANCE = 210049
};

enum HOTActions
{
    ACTION_UNLOCK_ENCOUNTER = 1
};

template<class AI>
AI* GetHourOfTwilightAI(Creature* creature)
{
    return GetInstanceAI<AI>(creature, HOTScriptName);
}

#define RegisterHourOfTwilightCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetHourOfTwilightAI)
}

#endif // DEF_HOUROFTWILIGHT_H
