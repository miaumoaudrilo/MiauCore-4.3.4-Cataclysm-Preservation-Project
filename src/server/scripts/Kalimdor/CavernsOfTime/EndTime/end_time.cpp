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
#include "GameObject.h"
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "PassiveAI.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"

namespace EndTime
{
enum Events
{
    // Nozdormu
    EVENT_TALK_ENCOUNTER_INTRO = 1,
    EVENT_TALK_ENCOUNTER_OUTRO_1,
    EVENT_TALK_ENCOUNTER_OUTRO_2,
    EVENT_TALK_ENCOUNTER_OUTRO_3,
    EVENT_TALK_ENCOUNTER_OUTRO_4
};

enum Actions
{
    // Nozdormu
    ACTION_ENCOUNTER_INTRO = 1,
    ACTION_ENCOUNTER_OUTRO = 2
};

enum Texts
{
    // Nozdormu
    SAY_ENCOUNTER_INTRO     = 0,
    SAY_ENCOUNTER_OUTRO_1   = 1,
    SAY_ENCOUNTER_OUTRO_2   = 2,
    SAY_ENCOUNTER_OUTRO_3   = 3,
    SAY_ENCOUNTER_OUTRO_4   = 4,
    SAY_OBSIDIAN_SHRINE     = 5
};

enum GossipMenuIds
{
    GOSSIP_MENU_ID_NOZDORMU                 = 13360,
    GOSSIP_MENU_OPTION_ID_WELL_OF_ETERNITY  = 0
};

Position const NozdormuEncounterTeleportPosition    = { 4033.37f, -294.457f, 181.613f, 5.8119464f };
Position const NozdormuDefeatTeleportPosition       = { 4138.48f, -429.436f, 122.259f, 5.8119464f };
WorldLocation const WellOfEternityEntrance(939, 3238.55f, -4998.39f, 194.093f, 2.35619f); // areatrigger_teleport 7151

struct npc_end_time_nozdormu : public NullCreatureAI
{
    npc_end_time_nozdormu(Creature* creature) : NullCreatureAI(creature),  _instance(nullptr), _introDone(false), _obsidianIntroDone(false) { }

    void InitializeAI() override
    {
        _instance = me->GetInstanceScript();
    }

    void MoveInLineOfSight(Unit* who) override
    {
        // Santuario Obsidiana: aviso al llegar (solo si el Eco de Baine está en esta instancia)
        if (!_obsidianIntroDone && _instance && who->IsPlayer() && !who->ToPlayer()->IsGameMaster() && me->GetAreaId() == AREA_OBSIDIAN_DRAGONSHRINE
            && me->IsWithinDistInMap(who, 40.0f) && (_instance->GetData(DATA_FIRST_ECHO) == DATA_ECHO_OF_BAINE || _instance->GetData(DATA_SECOND_ECHO) == DATA_ECHO_OF_BAINE))
        {
            _obsidianIntroDone = true;
            Talk(SAY_OBSIDIAN_SHRINE, who);
        }
    }

    bool GossipHello(Player* player) override
    {
        if (!_instance)
            return true;

        if (_instance->GetBossState(DATA_MUROZOND) == DONE)
            AddGossipItemFor(player, GOSSIP_MENU_ID_NOZDORMU, GOSSIP_MENU_OPTION_ID_WELL_OF_ETERNITY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

        SendGossipMenuFor(player, player->GetGossipTextId(GOSSIP_MENU_ID_NOZDORMU, me), me->GetGUID());

        return true;
    }

    // "Estoy listo para viajar al pasado, al Pozo de la Eternidad": lleva al jugador a la entrada del Pozo de la Eternidad
    // (areatrigger_teleport 7151, mapa 939). TeleportTo aplica los requisitos de acceso (logro 6117 en heroico) y el bloqueo de instancia.
    bool GossipSelect(Player* player, uint32 /*menuId*/, uint32 /*gossipListId*/) override
    {
        CloseGossipMenuFor(player);
        if (_instance && _instance->GetBossState(DATA_MUROZOND) == DONE)
            player->TeleportTo(WellOfEternityEntrance);
        return true;
    }

    void DoAction(int32 action) override
    {
        switch (action)
        {
            case ACTION_ENCOUNTER_INTRO:
                if (!_introDone)
                {
                    _events.ScheduleEvent(EVENT_TALK_ENCOUNTER_INTRO, 8s + 400ms);
                    _introDone = true;
                }
                break;
            case ACTION_ENCOUNTER_OUTRO:
                me->setActive(true);
                me->NearTeleportTo(NozdormuDefeatTeleportPosition);
                _events.ScheduleEvent(EVENT_TALK_ENCOUNTER_OUTRO_1, 8s + 700ms);
                me->setActive(false);
                break;
            default:
                break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TALK_ENCOUNTER_INTRO:
                    Talk(SAY_ENCOUNTER_INTRO);
                    me->setActive(true);
                    me->NearTeleportTo(NozdormuEncounterTeleportPosition);
                    me->setActive(false);
                    break;
                case EVENT_TALK_ENCOUNTER_OUTRO_1:
                    Talk(SAY_ENCOUNTER_OUTRO_1);
                    _events.ScheduleEvent(EVENT_TALK_ENCOUNTER_OUTRO_2, 17s);
                    break;
                case EVENT_TALK_ENCOUNTER_OUTRO_2:
                    Talk(SAY_ENCOUNTER_OUTRO_2);
                    _events.ScheduleEvent(EVENT_TALK_ENCOUNTER_OUTRO_3, 17s);
                    break;
                case EVENT_TALK_ENCOUNTER_OUTRO_3:
                    Talk(SAY_ENCOUNTER_OUTRO_3);
                    _events.ScheduleEvent(EVENT_TALK_ENCOUNTER_OUTRO_4, 14s);
                    break;
                case EVENT_TALK_ENCOUNTER_OUTRO_4:
                    Talk(SAY_ENCOUNTER_OUTRO_4);
                    me->SetFacingTo(3.1765f);
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap _events;
    InstanceScript* _instance;
    bool _introDone;
    bool _obsidianIntroDone;
};

// 54476 Nozdormu (Santuario Rubí / Entrada del Tiempo) y 54867 Imagen de Nozdormu (Santuarios Azur y Esmeralda):
// avisan al grupo al llegar a cada santuario (solo si su eco está en esta instancia).
enum ShrineTexts
{
    SAY_SHRINE_FIRST  = 0, // 54476: Rubí        | 54867: Azur
    SAY_SHRINE_SECOND = 1  // 54476: Entrada     | 54867: Esmeralda
};

struct npc_end_time_shrine_nozdormu : public NullCreatureAI
{
    npc_end_time_shrine_nozdormu(Creature* creature) : NullCreatureAI(creature), _done(false) { }

    void MoveInLineOfSight(Unit* who) override
    {
        if (_done || !who->IsPlayer() || who->ToPlayer()->IsGameMaster() || !me->IsWithinDistInMap(who, 40.0f))
            return;

        InstanceScript* instance = me->GetInstanceScript();
        if (!instance)
            return;

        auto selected = [instance](uint32 echo) { return instance->GetData(DATA_FIRST_ECHO) == echo || instance->GetData(DATA_SECOND_ECHO) == echo; };

        int32 group = -1;
        switch (me->GetAreaId())
        {
            case AREA_RUBY_DRAGONSHRINE:    if (selected(DATA_ECHO_OF_SYLVANAS)) group = SAY_SHRINE_FIRST; break;
            case AREA_ENTRYWAY_OF_TIME:     if (instance->GetBossState(DATA_MUROZOND) != DONE) group = SAY_SHRINE_SECOND; break;
            case AREA_AZURE_DRAGONSHRINE:   if (selected(DATA_ECHO_OF_JAINA)) group = SAY_SHRINE_FIRST; break;
            case AREA_EMERALD_DRAGONSHRINE: if (selected(DATA_ECHO_OF_TYRANDE)) group = SAY_SHRINE_SECOND; break;
            default: break;
        }

        _done = true;
        if (group >= 0)
            Talk(uint8(group), who);
    }

private:
    bool _done;
};

enum TimeTransitDeviceSpells
{
    SPELL_TELEPORT_TO_ENTRANCE              = 102564,
    SPELL_TELEPORT_TO_RUBY_DRAGONSHRINE     = 102579,
    SPELL_TELEPORT_TO_EMERALD_DRAGONSHRINE  = 104761,
    SPELL_TELEPORT_TO_BLUE_DRAGONSHRINE     = 102126,
    SPELL_TELEPORT_TO_BLACK_DRAGONSHRINE    = 103868,
    SPELL_TELEPORT_TO_BRONZE_DRAGONSHRINE   = 104764
};

enum TimeTransitGossipMenuIds
{
    GOSSIP_MENU_ID_SELECT_YOUR_DESTINATION = 13321
};

enum TimeTransitGossipIndexes
{
    GOSSIP_INDEX_TELEPORT_TO_ENTRYWAY_OF_TIME   = 0,
    GOSSIP_INDEX_SECOND_ECHO_OFFSET             = 4,
    GOSSIP_INDEX_TELEPORT_TO_BRONZE_DRAGONSHRINE = 9
};

// Opciones del menú 13321 (DB): 0 entrada, 1-4 primer eco, 5-8 segundo eco (Rubí, Esmeralda, Azur, Obsidiana), 9 Bronce.
static uint32 TransitDeviceTeleportSpell(uint32 index)
{
    switch (index)
    {
        case 0:         return SPELL_TELEPORT_TO_ENTRANCE;
        case 1: case 5: return SPELL_TELEPORT_TO_RUBY_DRAGONSHRINE;
        case 2: case 6: return SPELL_TELEPORT_TO_EMERALD_DRAGONSHRINE;
        case 3: case 7: return SPELL_TELEPORT_TO_BLUE_DRAGONSHRINE;
        case 4: case 8: return SPELL_TELEPORT_TO_BLACK_DRAGONSHRINE;
        case 9:         return SPELL_TELEPORT_TO_BRONZE_DRAGONSHRINE;
        default:        return 0;
    }
}

struct go_end_time_time_transit_device : public GameObjectAI
{
    go_end_time_time_transit_device(GameObject* gameObject) : GameObjectAI(gameObject), _instance(nullptr) { }

    void InitializeAI() override
    {
        _instance = me->GetInstanceScript();
    }

    // Texto de la opción con el estado del jefe de ese destino (verde: derrotado, rojo: sin derrotar).
    void AddOption(Player* player, uint32 index, std::string const& destination, int32 bossData)
    {
        std::string text = destination;
        if (bossData >= 0)
            text += _instance->GetBossState(uint32(bossData)) == DONE ? " |cff006400(Derrotado)|r" : " |cff8b0000(Sin derrotar)|r";
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, text, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + index);
    }

    static char const* EchoDestination(uint32 echoData)
    {
        switch (echoData)
        {
            case DATA_ECHO_OF_SYLVANAS: return "Santuario de Dragones Rubí - Eco de Sylvanas";
            case DATA_ECHO_OF_TYRANDE:  return "Santuario de Dragones Esmeralda - Eco de Tyrande";
            case DATA_ECHO_OF_JAINA:    return "Santuario de Dragones Azur - Eco de Jaina";
            case DATA_ECHO_OF_BAINE:    return "Santuario de Dragones Obsidiana - Eco de Baine";
            default:                    return "";
        }
    }

    bool GossipHello(Player* player) override
    {
        if (!_instance)
            return false;

        if (player->IsInCombat())
            return true;

        ClearGossipMenuFor(player);

        if (player->GetAreaId() != AREA_ENTRYWAY_OF_TIME)
            AddOption(player, GOSSIP_INDEX_TELEPORT_TO_ENTRYWAY_OF_TIME, "Umbral del Tiempo", -1);

        uint32 firstEcho = _instance->GetData(DATA_FIRST_ECHO);
        uint32 secondEcho = _instance->GetData(DATA_SECOND_ECHO);

        // Paso a paso: primer eco -> (al vencerlo) segundo eco -> (al vencer ambos) Santuario de Bronce
        if (uint32 offset = EchoGossipOffset(firstEcho))
            AddOption(player, offset, EchoDestination(firstEcho), int32(firstEcho));

        if (_instance->GetBossState(firstEcho) == DONE)
            if (uint32 offset = EchoGossipOffset(secondEcho))
                AddOption(player, offset + GOSSIP_INDEX_SECOND_ECHO_OFFSET, EchoDestination(secondEcho), int32(secondEcho));

        if (_instance->GetBossState(firstEcho) == DONE && _instance->GetBossState(secondEcho) == DONE)
            AddOption(player, GOSSIP_INDEX_TELEPORT_TO_BRONZE_DRAGONSHRINE, "Santuario de Dragones Bronce - Murozond", DATA_MUROZOND);

        SendGossipMenuFor(player, player->GetGossipTextId(GOSSIP_MENU_ID_SELECT_YOUR_DESTINATION, me), me->GetGUID());

        return true;
    }

    bool GossipSelect(Player* player, uint32 /*gossipMenuId*/, uint32 gossipListId) override
    {
        uint32 index = player->PlayerTalkClass->GetGossipOptionAction(gossipListId) - GOSSIP_ACTION_INFO_DEF;
        CloseGossipMenuFor(player);
        if (uint32 spell = TransitDeviceTeleportSpell(index))
            player->CastSpell(player, spell);
        return true;
    }

private:
    InstanceScript* _instance;
};

struct go_end_time_fragment_of_jainas_staff : public GameObjectAI
{
    go_end_time_fragment_of_jainas_staff(GameObject* gameObject) : GameObjectAI(gameObject), _instance(nullptr) { }

    void InitializeAI() override
    {
        _instance = me->GetInstanceScript();
    }

    bool GossipHello(Player* /*player*/) override
    {
        if (!_instance)
            return false;

        _instance->SetData(DATA_COLLECTED_FRAGMENT_OF_JAINAS_STAFF, 0);
        me->DespawnOrUnsummon();

        return true;
    }

private:
    InstanceScript* _instance;
};
}

void AddSC_end_time()
{
    using namespace EndTime;
    RegisterEndTimeCreatureAI(npc_end_time_nozdormu);
    RegisterEndTimeCreatureAI(npc_end_time_shrine_nozdormu);
    RegisterGameObjectAI(go_end_time_time_transit_device);
    RegisterGameObjectAI(go_end_time_fragment_of_jainas_staff);
}
