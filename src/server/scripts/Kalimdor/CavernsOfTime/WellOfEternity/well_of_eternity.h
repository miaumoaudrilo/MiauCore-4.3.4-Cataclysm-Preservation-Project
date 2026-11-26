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

#ifndef DEF_WELLOFETERNITY_H
#define DEF_WELLOFETERNITY_H

#include "CreatureAIImpl.h"
#include "Define.h"

namespace WellOfEternity
{
constexpr char const* DataHeader = "WOE";
#define WOEScriptName "instance_well_of_eternity"

    uint32 const EncounterCount = 3;

    enum WOEDataTypes
    {
        // Bosses
        BOSS_PEROTHARN = 0,
        BOSS_QUEEN_AZSHARA = 1,
        BOSS_MANNOROTH_AND_VAROTHEN = 2,

        // Misc
        DATA_WOE_COURTYARD_DOOR01,
        DATA_PORTALS_CLOSED,
        DATA_INVISIBLE_FIRE_WALL,
        DATA_QUEEN_AZSHARA,
        DATA_MANNOROTH,
        DATA_VAROTHEN,
        DATA_ROYAL_CACHE,
        DATA_MINOR_CACHE_OF_THE_ASPECTS,
        DATA_LAZY_EYE_FAILED,
        DATA_THATS_NOT_CANON
    };

    enum WOECreatures
    {
        NPC_PEROTHARN               = 55085,
        NPC_EYE_OF_PEROTHARN        = 55879,
        NPC_EYE_OF_PEROTHARN_2      = 55868,
        NPC_NOZDORMU                = 55624,
        NPC_ILLIDAN_COURTYARD       = 55500,

        // Patio y portales de la Legión
        NPC_LEGION_PORTAL           = 54513,
        NPC_PORTAL_CONNECTOR_1      = 55541,
        NPC_PORTAL_CONNECTOR_2      = 55542,
        NPC_PORTAL_CONNECTOR_3      = 55543,
        NPC_FEL_CRYSTAL             = 55917,
        NPC_FEL_CRYSTAL_STALKER     = 55965,
        NPC_DREADLORD_DEFENDER      = 55656,
        NPC_CORRUPTED_ARCANIST      = 55654,
        NPC_GUARDIAN_DEMON          = 54927,
        NPC_LEGION_DEMON            = 55503,

        // Reina Azshara
        NPC_QUEEN_AZSHARA           = 54853,
        NPC_ENCHANTED_MAGUS_FIRE    = 54882,
        NPC_ENCHANTED_MAGUS_FROST   = 54883,
        NPC_ENCHANTED_MAGUS_ARCANE  = 54884,
        NPC_HAND_OF_THE_QUEEN       = 54728,

        // Mannoroth y Varo'then
        NPC_MANNOROTH               = 54969,
        NPC_CAPTAIN_VAROTHEN        = 55419,
        NPC_ILLIDAN_WELL            = 55532,
        NPC_TYRANDE_WELL            = 55524,
        NPC_MALFURION_WELL          = 55570,
        NPC_DREADLORD_DEBILITATOR   = 55762,
        NPC_INFERNAL                = 56036
    };

    enum WOEGameObjectIds
    {
        GO_WOE_COURTYARD_DOOR01         = 210084,
        GO_LARGE_FIREWALL_DOOR          = 210234,
        GO_SMALL_FIREWALL_DOOR          = 210130,
        GO_INVISIBLE_FIRE_WALL          = 210097,
        GO_PORTAL_ENERGY_FOCUS_1        = 209366,
        GO_PORTAL_ENERGY_FOCUS_2        = 209447,
        GO_PORTAL_ENERGY_FOCUS_3        = 209448,
        GO_ROYAL_CACHE                  = 210025,
        GO_MINOR_CACHE_OF_THE_ASPECTS   = 209541,
        GO_TIME_TRANSIT_DEVICE_1        = 209997,
        GO_TIME_TRANSIT_DEVICE_2        = 209998,
        GO_TIME_TRANSIT_DEVICE_3        = 209999,
        GO_TIME_TRANSIT_DEVICE_4        = 210000
    };

    enum WOEAreas
    {
        AREA_WOE_COURTYARD  = 5993,
        AREA_WOE_PALACE     = 5995,
        AREA_WOE_WELL       = 5994
    };

    template<class AI>
    AI* GetWellOfEternityAI(Creature* creature)
    {
        return GetInstanceAI<AI>(creature, WOEScriptName);
    }

#define RegisterWellOfEternityCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetWellOfEternityAI)
}

#endif // DEF_WELLOFETERNITY_H
