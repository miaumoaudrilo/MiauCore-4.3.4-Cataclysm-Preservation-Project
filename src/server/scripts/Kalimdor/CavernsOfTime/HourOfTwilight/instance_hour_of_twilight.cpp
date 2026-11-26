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

#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "hour_of_twilight.h"

namespace HourOfTwilight
{
ObjectData const creatureData[] =
{
    { NPC_ARCURION, DATA_ARCURION },
    { NPC_ASIRA, DATA_ASIRA_DAWNSLAYER },
    { NPC_BENEDICTUS, DATA_ARCHBISHOP_BENEDICTUS },
    { NPC_THRALL_ARCURION, DATA_THRALL_ARCURION },
    { NPC_THRALL_ASIRA, DATA_THRALL_ASIRA },
    { NPC_THRALL_BENEDICTUS, DATA_THRALL_BENEDICTUS },
    { 0, 0  } // END
};

ObjectData const gameobjectData[] =
{
    { GO_ARCURION_ENTRANCE, GO_ARCURION_ENTRANCE },
    { 0, 0 } // END
};

DoorData const doorData[] =
{
    { GO_ARCURION_EXIT, DATA_ARCURION, DOOR_TYPE_PASSAGE },
    { 0, 0, DOOR_TYPE_ROOM } // END
};

class instance_hour_of_twilight : public InstanceMapScript
{
public:
    instance_hour_of_twilight() : InstanceMapScript(HOTScriptName, 940) { }

    struct instance_hour_of_twilight_InstanceMapScript : public InstanceScript
    {
        instance_hour_of_twilight_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            LoadDoorData(doorData);
            LoadObjectData(creatureData, gameobjectData);
        }

        void OnGameObjectCreate(GameObject* gameObject) override
        {
            InstanceScript::OnGameObjectCreate(gameObject);
            if (gameObject->GetEntry() == GO_ARCURION_ENTRANCE)
                HandleGameObject(gameObject->GetGUID(), _ready[DATA_ARCURION] || GetBossState(DATA_ARCURION) == DONE, gameObject);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            InstanceScript::OnCreatureCreate(creature);
            if (creature->GetEntry() == NPC_THRALL_BENEDICTUS)
                creature->SetVisible(_benedictusIntro || GetBossState(DATA_ARCHBISHOP_BENEDICTUS) == DONE);
        }

        uint32 GetData(uint32 type) const override
        {
            if (type >= DATA_ARCURION_READY && type <= DATA_BENEDICTUS_READY)
                return _ready[type - DATA_ARCURION_READY];
            return InstanceScript::GetData(type);
        }

        void SetData(uint32 type, uint32 value) override
        {
            if (type == DATA_BENEDICTUS_INTRO)
            {
                _benedictusIntro = value != 0;
                if (Creature* thrall = GetCreature(DATA_THRALL_BENEDICTUS))
                    thrall->SetVisible(_benedictusIntro);
                return;
            }
            if (type < DATA_ARCURION_READY || type > DATA_BENEDICTUS_READY)
                return;

            uint32 const boss = type - DATA_ARCURION_READY;
            _ready[boss] = value != 0;
            if (boss == DATA_ARCURION)
                HandleGameObject(GetGuidData(GO_ARCURION_ENTRANCE), _ready[boss] || GetBossState(boss) == DONE);
            if (_ready[boss])
                if (Creature* creature = GetCreature(boss))
                    if (CreatureAI* ai = creature->AI())
                        ai->DoAction(ACTION_UNLOCK_ENCOUNTER);
        }

        bool CheckRequiredBosses(uint32 boss, Player const* player) const override
        {
            return _SkipCheckRequiredBosses(player) ||
                (boss < EncounterCount && _ready[boss] && (!boss || GetBossState(boss - 1) == DONE));
        }

    private:
        bool _ready[EncounterCount] = {};
        bool _benedictusIntro = false;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_hour_of_twilight_InstanceMapScript(map);
    }
};
}

void AddSC_instance_hour_of_twilight()
{
    using namespace HourOfTwilight;
    new instance_hour_of_twilight();
}
