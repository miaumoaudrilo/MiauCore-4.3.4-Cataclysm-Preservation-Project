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

#include "end_time.h"
#include "AchievementMgr.h"
#include "EventMap.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <array>
#include <sstream>

namespace EndTime
{
ObjectData const creatureData[] =
{
    { BOSS_MUROZOND,        DATA_MUROZOND                   },
    { NPC_ARCANE_CIRCLE,    DATA_ARCANE_CIRCLE              },
    { NPC_ECHO_OF_BAINE,    DATA_ECHO_OF_BAINE_CREATURE     },
    { NPC_ECHO_OF_SYLVANAS, DATA_ECHO_OF_SYLVANAS_CREATURE  },
    { NPC_ECHO_OF_TYRANDE,  DATA_ECHO_OF_TYRANDE_CREATURE   },
    { NPC_ECHO_OF_JAINA,    DATA_ECHO_OF_JAINA_CREATURE     },
    { 0,                    0                               } // END
};

ObjectData const gameobjectData[] =
{
    { GO_HOURGLASS_OF_TIME, DATA_HOURGLASS_OF_TIME },
    { 0,                    0                      } // END
};

DoorData const doorData[] =
{
    { 0, 0, DOOR_TYPE_ROOM } // END
};

enum ETEvents
{
    EVENT_RESPAWN_MUROZOND = 1
};

enum ETSpawnGroups
{
    SPAWN_GROUP_ID_MUROZOND_CHEST   = 437,
    SPAWN_GROUP_ID_ECHO_OF_JAINA    = 460
};

enum ETWorldStates
{
    WORLD_STATE_ID_SHOW_COLLECTED_STAVE_FRAGMENTS   = 6046,
    WORLD_STATE_ID_COLLECTED_STAVE_FRAGMENTS        = 6025
};

enum ETMapEvents
{
    MAP_EVENT_AZURE_DRAGONSHRINE_ENTERED = 29225
};

enum ETSpells
{
    SPELL_SUMMON_PHANTOM            = 102200,
    SPELL_TYRANDE_ACHIEVEMENT       = 102542  // crédito del logro Guardia lunar (criterio 18138, BE_SPELL_TARGET)
};

// Crédito de encuentro (instance_encounters 1269 "Primer eco" / 1268 "Segundo eco"): el eco cambia en cada instancia,
// así que el script da el crédito con una entrada fija (hechizos de premio que no se lanzan en esta mazmorra).
enum ETEncounterCredit
{
    SPELL_CREDIT_FIRST_ECHO  = 101620,
    SPELL_CREDIT_SECOND_ECHO = 101621
};

enum ETAchievementCriteria
{
    CRITERIA_SEVERED_TIES = 18499
};

std::array<Position const, 2> MurozondSpawnPositions =
{
    Position(4288.125f, -456.40277f, 160.4989f,  2.98451f), // Initial spawn position
    Position(4181.117f, -420.21933f, 138.38057f, 3.10668f)  // Respawn position
};

// Datos de los ecos: entrada del PNJ por DATA_ECHO_OF_*
static uint32 EchoEntry(uint32 echoData)
{
    switch (echoData)
    {
        case DATA_ECHO_OF_BAINE:    return NPC_ECHO_OF_BAINE;
        case DATA_ECHO_OF_JAINA:    return NPC_ECHO_OF_JAINA;
        case DATA_ECHO_OF_SYLVANAS: return NPC_ECHO_OF_SYLVANAS;
        case DATA_ECHO_OF_TYRANDE:  return NPC_ECHO_OF_TYRANDE;
        default:                    return 0;
    }
}

class instance_end_time : public InstanceMapScript
{
public:
    instance_end_time() : InstanceMapScript(ETScriptName, 938) { }

    struct instance_end_time_InstanceMapScript : public InstanceScript
    {
        instance_end_time_InstanceMapScript(InstanceMap* map) : InstanceScript(map),
            _killedInfiniteDragonkins(0), _collectedStaffFragments(0), _firstEcho(EchoBossCount), _secondEcho(EchoBossCount),
            _gauntletProgress(0), _healerHitInGauntlet(false), _risenGhoulsKilledInCalling(0)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            LoadDoorData(doorData);
            LoadObjectData(creatureData, gameobjectData);
        }

        // En 4.3.4 cada instancia de Fin de los Días tiene 2 de los 4 ecos, elegidos al azar al crearla.
        void SelectEchoes()
        {
            if (_firstEcho < EchoBossCount && _secondEcho < EchoBossCount && _firstEcho != _secondEcho)
                return;

            std::vector<uint32> echoes = { DATA_ECHO_OF_BAINE, DATA_ECHO_OF_JAINA, DATA_ECHO_OF_SYLVANAS, DATA_ECHO_OF_TYRANDE };
            Trinity::Containers::RandomShuffle(echoes);
            _firstEcho = echoes[0];
            _secondEcho = echoes[1];
            SaveToDB();
        }

        bool IsEchoSelected(uint32 echoData) const
        {
            return echoData == _firstEcho || echoData == _secondEcho;
        }

        void ProcessEvent(WorldObject* /*obj*/, uint32 eventId, WorldObject* /*invoker*/) override
        {
            if (_executedMapEvents.find(eventId) != _executedMapEvents.end())
                return;

            switch (eventId)
            {
                case MAP_EVENT_AZURE_DRAGONSHRINE_ENTERED:
                    if (GetBossState(DATA_ECHO_OF_JAINA) == DONE || !IsEchoSelected(DATA_ECHO_OF_JAINA))
                        break;
                    DoUpdateWorldState(WORLD_STATE_ID_SHOW_COLLECTED_STAVE_FRAGMENTS, 1);
                    break;
                default:
                    break;
            }

            _executedMapEvents.insert(eventId);
        }

        void Create() override
        {
            InstanceScript::Create();
            SelectEchoes();

            // The instance has been created without save data, just spawn Murozond at his initial position.
            instance->SummonCreature(BOSS_MUROZOND, MurozondSpawnPositions[0]);
        }

        void Load(char const* data) override
        {
            InstanceScript::Load(data);
            SelectEchoes(); // guardados antiguos sin selección de ecos

            // The instance has been created from existing save data, but Murozond has not been defeated yet. Spawn him at his initial position.
            if (GetBossState(DATA_MUROZOND) != DONE)
                instance->SummonCreature(BOSS_MUROZOND, MurozondSpawnPositions[0]);
        }

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            data << _firstEcho << ' ' << _secondEcho << ' ' << uint32(_gauntletProgress) << ' ';
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            uint32 first = EchoBossCount, second = EchoBossCount, gauntlet = 0;
            data >> first >> second >> gauntlet;
            _firstEcho = first < EchoBossCount ? first : EchoBossCount;
            _secondEcho = second < EchoBossCount ? second : EchoBossCount;
            _gauntletProgress = uint8(gauntlet & 0x1F); // máscara de estanques superados
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (!unit->IsCreature())
                return;

            switch (unit->GetEntry())
            {
                case NPC_INFINITE_SUPRESSOR:
                case NPC_INFINITE_WARDEN:
                    ++_killedInfiniteDragonkins;
                    if (_killedInfiniteDragonkins == 4 || _killedInfiniteDragonkins == 8)
                        if (Creature* murozond = GetCreature(DATA_MUROZOND))
                            if (murozond->IsAIEnabled())
                                murozond->AI()->SetData(DATA_MUROZOND_INTRO, _killedInfiniteDragonkins == 4 ? IN_PROGRESS : DONE);
                    break;
                default:
                    break;
            }
        }

        void OnCreatureCreate(Creature* creature) override
        {
            InstanceScript::OnCreatureCreate(creature);

            switch (creature->GetEntry())
            {
                case NPC_NOZDORMU_DRAGON_SHRINES:
                    if (creature->GetAreaId() == AREA_BRONZE_DRAGONSHRINE)
                        AddObject(creature, DATA_NOZDORMU_BRONZE_DRAGON_SHRINE, true);
                    break;
                case NPC_ECHO_OF_BAINE:
                case NPC_ECHO_OF_JAINA:
                case NPC_ECHO_OF_SYLVANAS:
                case NPC_ECHO_OF_TYRANDE:
                {
                    uint32 echoData = creature->GetEntry() == NPC_ECHO_OF_BAINE ? DATA_ECHO_OF_BAINE :
                        creature->GetEntry() == NPC_ECHO_OF_JAINA ? DATA_ECHO_OF_JAINA :
                        creature->GetEntry() == NPC_ECHO_OF_SYLVANAS ? DATA_ECHO_OF_SYLVANAS : DATA_ECHO_OF_TYRANDE;
                    // Los ecos que no tocan en esta instancia no aparecen (ni se puede viajar a su santuario).
                    if (!IsEchoSelected(echoData))
                        creature->DespawnOrUnsummon(0s, Seconds(7 * DAY));
                    break;
                }
                case NPC_ARCANE_CIRCLE:
                    if (!IsEchoSelected(DATA_ECHO_OF_JAINA))
                        creature->DespawnOrUnsummon(0s, Seconds(7 * DAY));
                    break;
                default:
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            InstanceScript::OnGameObjectCreate(go);

            if (go->GetEntry() == GO_FRAGMENT_OF_JAINAS_STAFF && (!IsEchoSelected(DATA_ECHO_OF_JAINA) || GetBossState(DATA_ECHO_OF_JAINA) == DONE))
                go->DespawnOrUnsummon(0s, Seconds(7 * DAY));
        }

        void OnCreatureRemove(Creature* creature) override
        {
            InstanceScript::OnCreatureRemove(creature);

            switch (creature->GetEntry())
            {
                case NPC_NOZDORMU_DRAGON_SHRINES:
                    if (creature->GetAreaId() == AREA_BRONZE_DRAGONSHRINE)
                        AddObject(creature, DATA_NOZDORMU_BRONZE_DRAGON_SHRINE, false);
                    break;
                default:
                    break;
            }
        }

        bool SetBossState(uint32 type, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            if (state == DONE && type < EchoBossCount && IsEchoSelected(type))
            {
                uint32 echoesDone = (GetBossState(_firstEcho) == DONE ? 1 : 0) + (GetBossState(_secondEcho) == DONE ? 1 : 0);
                UpdateEncounterStateForSpellCast(echoesDone >= 2 ? SPELL_CREDIT_SECOND_ECHO : SPELL_CREDIT_FIRST_ECHO, nullptr);
            }

            switch (type)
            {
                case DATA_MUROZOND:
                    if (state == FAIL)
                        _events.ScheduleEvent(EVENT_RESPAWN_MUROZOND, 30s);
                    else if (state == DONE)
                        instance->SpawnGroupSpawn(SPAWN_GROUP_ID_MUROZOND_CHEST);
                    break;
                case DATA_ECHO_OF_JAINA:
                    if (state == IN_PROGRESS)
                    {
                        DoUpdateWorldState(WORLD_STATE_ID_SHOW_COLLECTED_STAVE_FRAGMENTS, 0);
                        if (Creature* circle = GetCreature(DATA_ARCANE_CIRCLE))
                            circle->DespawnOrUnsummon();
                    }
                    break;
                case DATA_ECHO_OF_SYLVANAS:
                    if (state == IN_PROGRESS || state == NOT_STARTED || state == FAIL)
                        _risenGhoulsKilledInCalling = 0;
                    break;
                case DATA_ECHO_OF_TYRANDE:
                    if (state == IN_PROGRESS && !_healerHitInGauntlet)
                        DoUpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, SPELL_TYRANDE_ACHIEVEMENT);
                    break;
                default:
                    break;
            }

            return true;
        }

        void SetData(uint32 type, uint32 value) override
        {
            switch (type)
            {
                case DATA_COLLECTED_FRAGMENT_OF_JAINAS_STAFF:
                    if (GetBossState(DATA_ECHO_OF_JAINA) == DONE)
                        break;

                    ++_collectedStaffFragments;
                    DoUpdateWorldState(WORLD_STATE_ID_COLLECTED_STAVE_FRAGMENTS, _collectedStaffFragments);

                    if (_collectedStaffFragments < 16)
                    {
                        if (Creature* circle = GetCreature(DATA_ARCANE_CIRCLE))
                            circle->CastSpell(nullptr, SPELL_SUMMON_PHANTOM);
                    }
                    else
                        instance->SpawnGroupSpawn(SPAWN_GROUP_ID_ECHO_OF_JAINA);
                    break;
                case DATA_MOONLIGHT_POOL_DONE: // value = bit del estanque superado
                    if (!(_gauntletProgress & value))
                    {
                        _gauntletProgress |= uint8(value & 0x1F);
                        SaveToDB();
                    }
                    break;
                case DATA_TYRANDE_GAUNTLET:
                    _gauntletProgress = uint8(value & 0x1F);
                    if (!_gauntletProgress)
                        _healerHitInGauntlet = false;
                    SaveToDB();
                    break;
                case DATA_HEALER_HIT_IN_GAUNTLET:
                    _healerHitInGauntlet = true;
                    break;
                case DATA_RISEN_GHOUL_KILLED_IN_CALLING:
                    ++_risenGhoulsKilledInCalling;
                    break;
                default:
                    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_FIRST_ECHO:
                    return _firstEcho;
                case DATA_SECOND_ECHO:
                    return _secondEcho;
                case DATA_TYRANDE_GAUNTLET:
                    return _gauntletProgress;
                case DATA_HEALER_HIT_IN_GAUNTLET:
                    return _healerHitInGauntlet ? 1 : 0;
                case DATA_RISEN_GHOUL_KILLED_IN_CALLING:
                    return _risenGhoulsKilledInCalling;
                default:
                    return 0;
            }
        }

        bool CheckAchievementCriteriaMeet(uint32 criteriaId, Player const* source, Unit const* target, uint32 miscValue1) override
        {
            switch (criteriaId)
            {
                case CRITERIA_SEVERED_TIES: // Lazos cortados: 2 necrófagos alzados muertos durante La llamada de los Altonato
                    return _risenGhoulsKilledInCalling >= 2;
                default:
                    return InstanceScript::CheckAchievementCriteriaMeet(criteriaId, source, target, miscValue1);
            }
        }

        void Update(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_RESPAWN_MUROZOND:
                        instance->SummonCreature(BOSS_MUROZOND, MurozondSpawnPositions[1]);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
        uint8 _killedInfiniteDragonkins;
        uint8 _collectedStaffFragments;
        uint32 _firstEcho;
        uint32 _secondEcho;
        uint8 _gauntletProgress;
        bool _healerHitInGauntlet;
        uint8 _risenGhoulsKilledInCalling;

        std::unordered_set<uint32> _executedMapEvents;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_end_time_InstanceMapScript(map);
    }
};
}

void AddSC_instance_end_time()
{
    using namespace EndTime;
    new instance_end_time();
}
