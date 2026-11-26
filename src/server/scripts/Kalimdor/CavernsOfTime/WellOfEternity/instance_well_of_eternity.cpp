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
 * Pozo de la Eternidad (mapa 939) - 4.3.4
 *  1. Patio: hay que cerrar los 3 portales de la Legión (Foco de energía del portal 209366/209447/209448) tras acabar con
 *     sus guardianes. Al cerrar el 3.º cae el Muro de fuego invisible (210097) y Peroth'arn sale a defender el palacio.
 *  2. Reina Azshara: no se combate a ella sino a sus 6 magos encantados (de 2 en 2). Al caer todos se marcha.
 *  3. Mannoroth y Varo'then: el encuentro termina cuando Mannoroth baja al 5% (Malfurion cierra el portal).
 *  Créditos (DungeonEncounter): 1272 Peroth'arn, 1273 Azshara, 1274 Mannoroth (LFG 437).
 *  Logros: 6127 Lazy Eye (Peroth'arn sin que ningún ojo detecte al grupo), 6070 That's Not Canon (Mannoroth sacrifica a Varo'then).
 */

#include "ScriptMgr.h"
#include "AchievementMgr.h"
#include "Creature.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "well_of_eternity.h"

#include <sstream>

namespace WellOfEternity
{
ObjectData const creatureData[] =
{
    { NPC_PEROTHARN,        BOSS_PEROTHARN      },
    { NPC_QUEEN_AZSHARA,    DATA_QUEEN_AZSHARA  },
    { NPC_MANNOROTH,        DATA_MANNOROTH      },
    { NPC_CAPTAIN_VAROTHEN, DATA_VAROTHEN       },
    { 0,                    0                   } // END
};

ObjectData const gameobjectData[] =
{
    { GO_WOE_COURTYARD_DOOR01,          DATA_WOE_COURTYARD_DOOR01       },
    { GO_INVISIBLE_FIRE_WALL,           DATA_INVISIBLE_FIRE_WALL        },
    { GO_ROYAL_CACHE,                   DATA_ROYAL_CACHE                },
    { GO_MINOR_CACHE_OF_THE_ASPECTS,    DATA_MINOR_CACHE_OF_THE_ASPECTS },
    { 0,                                0                               } // END
};

DoorData const doorData[] =
{
    { GO_LARGE_FIREWALL_DOOR,   BOSS_PEROTHARN, DOOR_TYPE_ROOM },
    { GO_SMALL_FIREWALL_DOOR,   BOSS_PEROTHARN, DOOR_TYPE_ROOM },
    { 0,                        0,              DOOR_TYPE_ROOM } // END
};

// Crédito de encuentro para los jefes que no mueren (instance_encounters 1273/1274, hechizos de premio sin uso en el mapa)
enum WOEEncounterCredit
{
    SPELL_CREDIT_QUEEN_AZSHARA  = 101622,
    SPELL_CREDIT_MANNOROTH      = 101623,
    SPELL_THATS_NOT_CANON       = 105576  // criterio 18363 (BE_SPELL_TARGET)
};

enum WOEAchievementCriteria
{
    CRITERIA_LAZY_EYE = 18618
};

class instance_well_of_eternity : public InstanceMapScript
{
public:
    instance_well_of_eternity() : InstanceMapScript(WOEScriptName, 939) { }

    struct instance_well_of_eternity_InstanceMapScript : public InstanceScript
    {
        instance_well_of_eternity_InstanceMapScript(InstanceMap* map) : InstanceScript(map), _portalsClosed(0), _lazyEyeFailed(false), _thatsNotCanon(false)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            LoadDoorData(doorData);
            LoadObjectData(creatureData, gameobjectData);
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            InstanceScript::OnGameObjectCreate(go);

            switch (go->GetEntry())
            {
                case GO_WOE_COURTYARD_DOOR01:
                    // la entrada al patio está abierta: el bloqueo es el muro de fuego hacia el palacio
                    go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_INVISIBLE_FIRE_WALL:
                    go->SetGoState(_portalsClosed >= 3 ? GO_STATE_ACTIVE : GO_STATE_READY);
                    break;
                case GO_ROYAL_CACHE:
                    UpdateCache(go, BOSS_QUEEN_AZSHARA);
                    break;
                case GO_MINOR_CACHE_OF_THE_ASPECTS:
                    UpdateCache(go, BOSS_MANNOROTH_AND_VAROTHEN);
                    break;
                case GO_PORTAL_ENERGY_FOCUS_1:
                case GO_PORTAL_ENERGY_FOCUS_2:
                case GO_PORTAL_ENERGY_FOCUS_3:
                    if (_closedFoci.count(go->GetEntry()))
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                default:
                    break;
            }
        }

        // Los cofres solo se pueden abrir tras derrotar a su jefe.
        void UpdateCache(GameObject* go, uint32 boss)
        {
            if (GetBossState(boss) == DONE)
                go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_LOCKED);
            else
                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_LOCKED);
        }

        bool SetBossState(uint32 type, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch (type)
            {
                case BOSS_PEROTHARN:
                    if (state == IN_PROGRESS)
                        _lazyEyeFailed = false;
                    break;
                case BOSS_QUEEN_AZSHARA:
                    if (state == DONE)
                    {
                        UpdateEncounterStateForSpellCast(SPELL_CREDIT_QUEEN_AZSHARA, nullptr);
                        if (GameObject* cache = GetGameObject(DATA_ROYAL_CACHE))
                            UpdateCache(cache, BOSS_QUEEN_AZSHARA);
                    }
                    break;
                case BOSS_MANNOROTH_AND_VAROTHEN:
                    if (state == IN_PROGRESS)
                        _thatsNotCanon = false;
                    else if (state == DONE)
                    {
                        if (_thatsNotCanon)
                            DoUpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, SPELL_THATS_NOT_CANON);
                        UpdateEncounterStateForSpellCast(SPELL_CREDIT_MANNOROTH, nullptr);
                        if (GameObject* cache = GetGameObject(DATA_MINOR_CACHE_OF_THE_ASPECTS))
                            UpdateCache(cache, BOSS_MANNOROTH_AND_VAROTHEN);
                    }
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
                case DATA_PORTALS_CLOSED:
                    if (_closedFoci.insert(value).second)
                    {
                        _portalsClosed = uint8(_closedFoci.size());
                        if (_portalsClosed >= 3)
                        {
                            if (GameObject* wall = GetGameObject(DATA_INVISIBLE_FIRE_WALL))
                                wall->SetGoState(GO_STATE_ACTIVE);
                            if (Creature* perotharn = GetCreature(BOSS_PEROTHARN))
                                if (perotharn->IsAIEnabled())
                                    perotharn->AI()->DoAction(1); // ACTION_PORTALS_CLOSED
                        }
                        SaveToDB();
                    }
                    break;
                case DATA_LAZY_EYE_FAILED:
                    _lazyEyeFailed = true;
                    break;
                case DATA_THATS_NOT_CANON:
                    _thatsNotCanon = true;
                    break;
                default:
                    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_PORTALS_CLOSED:
                    return _portalsClosed;
                case DATA_LAZY_EYE_FAILED:
                    return _lazyEyeFailed ? 1 : 0;
                default:
                    return 0;
            }
        }

        bool CheckAchievementCriteriaMeet(uint32 criteriaId, Player const* source, Unit const* target, uint32 miscValue1) override
        {
            if (criteriaId == CRITERIA_LAZY_EYE)
                return !_lazyEyeFailed;
            return InstanceScript::CheckAchievementCriteriaMeet(criteriaId, source, target, miscValue1);
        }

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            uint32 mask = 0;
            if (_closedFoci.count(GO_PORTAL_ENERGY_FOCUS_1)) mask |= 1;
            if (_closedFoci.count(GO_PORTAL_ENERGY_FOCUS_2)) mask |= 2;
            if (_closedFoci.count(GO_PORTAL_ENERGY_FOCUS_3)) mask |= 4;
            data << mask << ' ';
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            uint32 mask = 0;
            data >> mask;
            if (mask & 1) _closedFoci.insert(GO_PORTAL_ENERGY_FOCUS_1);
            if (mask & 2) _closedFoci.insert(GO_PORTAL_ENERGY_FOCUS_2);
            if (mask & 4) _closedFoci.insert(GO_PORTAL_ENERGY_FOCUS_3);
            _portalsClosed = uint8(_closedFoci.size());
        }

    private:
        uint8 _portalsClosed;
        std::set<uint32> _closedFoci;
        bool _lazyEyeFailed;
        bool _thatsNotCanon;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_well_of_eternity_InstanceMapScript(map);
    }
};
}

void AddSC_instance_well_of_eternity()
{
    using namespace WellOfEternity;
    new instance_well_of_eternity();
}
