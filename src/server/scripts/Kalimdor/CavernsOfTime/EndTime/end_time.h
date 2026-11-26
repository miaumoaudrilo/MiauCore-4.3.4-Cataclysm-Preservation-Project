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

#ifndef DEF_ENDTIME_H
#define DEF_ENDTIME_H

#include "CreatureAIImpl.h"
#include "Define.h"

class Creature;

namespace EndTime
{
constexpr char const* DataHeader = "ET";
#define ETScriptName "instance_end_time"

uint32 const EncounterCount = 5;

enum ETDataTypes
{
    // Bosses
    DATA_ECHO_OF_BAINE      = 0,
    DATA_ECHO_OF_JAINA      = 1,
    DATA_ECHO_OF_SYLVANAS   = 2,
    DATA_ECHO_OF_TYRANDE    = 3,
    DATA_MUROZOND           = 4,

    // Additional Data
    DATA_HOURGLASS_OF_TIME,
    DATA_MUROZOND_INTRO,
    DATA_NOZDORMU_BRONZE_DRAGON_SHRINE,
    DATA_ARCANE_CIRCLE,
    DATA_COLLECTED_FRAGMENT_OF_JAINAS_STAFF,

    // Selección de ecos (2 de 4 por instancia, como en 4.3.4)
    DATA_FIRST_ECHO,
    DATA_SECOND_ECHO,
    DATA_ECHO_OF_BAINE_CREATURE,
    DATA_ECHO_OF_SYLVANAS_CREATURE,
    DATA_ECHO_OF_TYRANDE_CREATURE,
    DATA_ECHO_OF_JAINA_CREATURE,
    // Guantelete de sombras de Tyrande (5 estanques de luz de luna)
    DATA_TYRANDE_GAUNTLET,
    DATA_MOONLIGHT_POOL_DONE,
    DATA_HEALER_HIT_IN_GAUNTLET,
    // Logro Lazos cortados
    DATA_RISEN_GHOUL_KILLED_IN_CALLING,
};

uint32 const EchoBossCount = 4; // DATA_ECHO_OF_BAINE .. DATA_ECHO_OF_TYRANDE

enum ETCreatures
{
    // Bosses
    BOSS_MUROZOND               = 54432,

    // Encounter Related Creatures
    /*Murozond*/
    NPC_INFINITE_WARDEN         = 54923,
    NPC_INFINITE_SUPRESSOR      = 54920,
    NPC_NOZDORMU_DRAGON_SHRINES = 54751,

    /*Echo of Jaina*/
    NPC_ARCANE_CIRCLE           = 54639,
    NPC_FROST_BLADE             = 54494,
    NPC_BLINK_TARGET            = 54542,
    NPC_FLARECORE_EMBER         = 54446,

    /*Ecos*/
    NPC_ECHO_OF_BAINE           = 54431,
    NPC_ECHO_OF_JAINA           = 54445,
    NPC_ECHO_OF_SYLVANAS        = 54123,
    NPC_ECHO_OF_TYRANDE         = 54544,

    /*Echo of Baine*/
    NPC_BAINES_TOTEM            = 54434,

    /*Echo of Sylvanas*/
    NPC_GHOUL_SPAWNER           = 54197,
    NPC_RISEN_GHOUL             = 54191,
    NPC_BLIGHTED_ARROWS         = 54403,

    /*Echo of Tyrande*/
    NPC_POOL_OF_MOONLIGHT       = 54508,
    NPC_MOONLANCE               = 54574,
    NPC_MOONLANCE_SPLIT_1       = 54580,
    NPC_MOONLANCE_SPLIT_2       = 54581,
    NPC_MOONLANCE_SPLIT_3       = 54582,
    NPC_EYE_OF_ELUNE_1          = 54594,
    NPC_EYE_OF_ELUNE_2          = 54597,
    NPC_EYE_OF_ELUNE_3          = 54939,
    NPC_EYE_OF_ELUNE_4          = 54940,
    NPC_EYE_OF_ELUNE_5          = 54941,
    NPC_EYE_OF_ELUNE_6          = 54942,
    NPC_TIME_TWISTED_NIGHTSABER_1 = 54688,
    NPC_TIME_TWISTED_NIGHTSABER_2 = 54699,
    NPC_TIME_TWISTED_NIGHTSABER_3 = 54700,
    NPC_TIME_TWISTED_SENTINEL   = 54512,
    NPC_TIME_TWISTED_HUNTRESS   = 54701,
    NPC_IMAGE_OF_NOZDORMU       = 54867
};

enum ETAreas
{
    AREA_RUBY_DRAGONSHRINE      = 5790,
    AREA_OBSIDIAN_DRAGONSHRINE  = 5792,
    AREA_AZURE_DRAGONSHRINE     = 5793,
    AREA_EMERALD_DRAGONSHRINE   = 5794,
    AREA_BRONZE_DRAGONSHRINE    = 5795,
    AREA_ENTRYWAY_OF_TIME       = 5796
};

// Índice de cada eco en el menú del Dispositivo de tránsito temporal (gossip 13321):
// 1-4 primer eco (Rubí, Esmeralda, Azur, Obsidiana), 5-8 segundo eco, 9 Santuario de Bronce.
inline uint32 EchoGossipOffset(uint32 echoData)
{
    switch (echoData)
    {
        case DATA_ECHO_OF_SYLVANAS: return 1; // Rubí
        case DATA_ECHO_OF_TYRANDE:  return 2; // Esmeralda
        case DATA_ECHO_OF_JAINA:    return 3; // Azur
        case DATA_ECHO_OF_BAINE:    return 4; // Obsidiana
        default:                    return 0;
    }
}

enum ETGameObjectIds
{
    GO_HOURGLASS_OF_TIME = 209249,
    GO_FRAGMENT_OF_JAINAS_STAFF = 209318,
    GO_BAINE_PLATFORM_1  = 209670,
    GO_BAINE_PLATFORM_2  = 209693,
    GO_BAINE_PLATFORM_3  = 209694,
    GO_BAINE_PLATFORM_4  = 209695
};

template<class AI>
AI* GetEndTimeAI(Creature* creature)
{
    return GetInstanceAI<AI>(creature, ETScriptName);
}

#define RegisterEndTimeCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetEndTimeAI)
}

#endif // DEF_ENDTIME_H
