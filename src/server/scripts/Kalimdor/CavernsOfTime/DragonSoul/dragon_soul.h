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

#ifndef DEF_DRAGONSOUL_H
#define DEF_DRAGONSOUL_H

#include "Define.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

namespace DragonSoul
{
constexpr char const* DataHeader = "DS";
#define DSScriptName "instance_dragon_soul"

uint32 const EncounterCount = 8;

enum DSDataTypes
{
    // Bosses
    DATA_MORCHOK                = 0,
    DATA_WARLORD_ZONOZZ         = 1,
    DATA_YORSAHJ_THE_UNSLEEPING = 2,
    DATA_HAGARA_THE_STORMBINDER = 3,
    DATA_ULTRAXION              = 4,
    DATA_WARMASTER_BLACKHORN    = 5,
    DATA_SPINE_OF_DEATHWING     = 6,
    DATA_MADNESS_OF_DEATHWING   = 7,

    // Additional Data
    DATA_DEATHWING_MADNESS_OF_DEATHWING,
    DATA_THRALL_MADNESS_OF_DEATHWING,
    DATA_YSERA_MADNESS_OF_DEATHWING,
    DATA_ALEXSTRASZA_MADNESS_OF_DEATHWING,
    DATA_NOZDORMU_MADNESS_OF_DEATHWING,
    DATA_KALECGOS_MADNESS_OF_DEATHWING,
    DATA_TAIL_TENTACLE_MADNESS_OF_DEATHWING
};

enum DSCreatures
{
    // Bosses
    BOSS_MADNESS_OF_DEATHWING                   = 56173,

    /*Madness of Deathwing*/
    NPC_DEATHWING_MADNESS_OF_DEATHWING          = 57962,
    NPC_ARM_TENTACLE_1                          = 56167,
    NPC_ARM_TENTACLE_2                          = 56846,
    NPC_WING_TENTACLE                           = 56168,
    NPC_TAIL_TENTACLE                           = 56844,
    NPC_MUTATED_CORRUPTION                      = 56471,
    NPC_CRUSH_TARGET                            = 56581,
    NPC_PLATFORM                                = 56307,
    NPC_HEMORRHAGE_TARGET                       = 56359,
    NPC_ELEMENTIUM_BOLT                         = 56262,
    NPC_BLISTERING_TENTACLE                     = 56188,
    NPC_TIME_ZONE_TARGET                        = 56332,
    NPC_TIME_ZONE                               = 56311,
    NPC_COSMETIC_TENTACLE                       = 57693,
    NPC_ELEMENTIUM_FRAGMENT                     = 56724,
    NPC_ELEMENTIUM_TERROR                       = 56710,
    NPC_JUMP_PAD                                = 56699,
    NPC_THRALL_MADNESS_OF_DEATHWING             = 56103,
    NPC_YSERA_MADNESS_OF_DEATHWING              = 56100,
    NPC_ALEXSTRASZA_MADNESS_OF_DEATHWING        = 56099,
    NPC_NOZDORMU_MADNESS_OF_DEATHWING           = 56102,
    NPC_KALECGOS_MADNESS_OF_DEATHWING           = 56101
};

enum DSGameObjectIds
{
    GO_ELEMENTIUM_FRAGMENT_10_NORMAL = 210079,
    GO_ELEMENTIUM_FRAGMENT_25_NORMAL = 210218,
    GO_ELEMENTIUM_FRAGMENT_25_LFR    = 210220,
    GO_ELEMENTIUM_FRAGMENT_10_HEROIC = 210219,
    GO_ELEMENTIUM_FRAGMENT_25_HEROIC = 210217
};

enum DSSpells
{
    SPELL_CALM_MAELSTROM_SKYBOX = 109480
};

enum DSMapObjIds
{
    /*
    * Data Values:
    * 1, 0, 60,  0, 0, 0  -- Talk
    * 1, 0, 213, 0, 0, 0  -- Scream in Agony
    */
    MAP_OBJ_ID_SPINE_OF_DEATHWING_HEAD  = 6574436,
    /*
    * Data Values:
    * 1, 1, [(0 - 100)], 0, 0, 0 -- Skybox cloud speed
    */
    MAP_OBJ_ID_SKYFIRE_SKYBOX           = 6858573
};

enum DragonSoulEncounterData
{
    DATA_RAID_MODE = 100,
    DATA_RESTORED_MANA = 17,
    DATA_DW_SPINE_BACKPLATE_1 = 21,
    DATA_DW_SPINE_BACKPLATE_CNT = 24,
    DATA_DW_SPINE_ROLL_INFO = 25,
    DATA_SPINE_CHEST = 28,
    DATA_SPINE_CHEST_LFR = 29,
    NPC_MORCHOK = 55265,
    NPC_RESONATING_CRYSTAL = 55346,
    NPC_WORLD_TRIGGER_VISUAL = 12999,
    NPC_KOHCROM = 57773,
    BLUE_GLOBULE = 55866,
    RED_GLOBULE = 55865,
    GREEN_GLOBULE = 55862,
    YELLOW_GLOBULE = 55864,
    VIOLETT_GLOBULE = 55863,
    BLACK_GLOBULE = 55867,
    NPC_FORGOTTEN_ONE = 56265,
    NPC_CORRUPTION_VEH_L = 56161,
    NPC_CORRUPTION_VEH_R = 56162,
    NPC_CORRUPTION = 53891,
    NPC_DW_SPINE_SPAWNER = 53888,
    NPC_CORRUPTED_BLOOD = 53889,
    NPC_HIDEOUS_AMALGAMATION = 53890,
    NPC_BURNING_TENDONS_L = 56575,
    NPC_BURNING_TENDONS_R = 56341,
    NPC_TRAVEL_TO_WYRMREST_TEMPLE = 57328,
    SPELL_PRESENCE_OF_THE_DRAGON_SOUL = 109247,
    LOOT_MODE_LFR = 2,
    RAID_MODE_NORMAL = 1,
    RAID_MODE_LFR = 2,
    GO_FALLING_FRAGMENT = 209596,
    WS_BLACK_YELLOW = 6221,
    WS_RED_GREEN = 6222,
    WS_BLACK_BLUE = 6223,
    WS_PURPLE_YELLOW = 6224,
};

template<class AI>
AI* GetDragonSoulAI(Creature* creature)
{
    return GetInstanceAI<AI>(creature, DSScriptName);
}

#define RegisterDragonSoulCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetDragonSoulAI)
}

#endif // DEF_DRAGONSOUL_H
