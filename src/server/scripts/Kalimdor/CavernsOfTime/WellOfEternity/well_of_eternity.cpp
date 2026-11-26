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
 * Pozo de la Eternidad - eventos del recorrido (4.3.4):
 *  - Nozdormu (55624): recibe al grupo en la entrada.
 *  - Illidan (55500): acompaña en el patio; dice su frase al llegar el grupo a cada uno de sus puntos.
 *  - Foco de energía del portal (209366/209447/209448): cierra el portal de la Legión si sus guardianes han caído.
 *  - Dispositivo de tránsito temporal (209997-210000): viaje entre zonas desbloqueadas.
 */

#include "well_of_eternity.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

namespace WellOfEternity
{
// 55624 - Nozdormu: bienvenida en la entrada
struct npc_woe_nozdormu : public ScriptedAI
{
    npc_woe_nozdormu(Creature* creature) : ScriptedAI(creature), _done(false) { }

    void MoveInLineOfSight(Unit* who) override
    {
        if (_done || !who->IsPlayer() || who->ToPlayer()->IsGameMaster() || !me->IsWithinDistInMap(who, 30.0f))
            return;

        _done = true;
        Talk(0, who); // "I see you've arrived. This is the eve of the sundering..."
        _events.ScheduleEvent(1, 12s);
        _events.ScheduleEvent(2, 22s);
        _events.ScheduleEvent(3, 32s);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        while (uint32 id = _events.ExecuteEvent())
            Talk(uint8(id)); // 1: Dragon Soul, 2: wards of Azshara, 3: Good luck
    }

private:
    EventMap _events;
    bool _done;
};

// 55500 - Illidan (patio): cada aparición dice su frase cuando llega el grupo
struct npc_woe_illidan_courtyard : public ScriptedAI
{
    npc_woe_illidan_courtyard(Creature* creature) : ScriptedAI(creature), _done(false) { }

    void Reset() override
    {
        me->SetReactState(REACT_PASSIVE);
    }

    uint8 PointText() const
    {
        struct Point { float x, y; uint8 text; };
        static Point const points[] =
        {
            { 3173.7f, -4875.5f, 13 }, // "Come with me if you'd like to live long enough..."
            { 3294.2f, -4982.0f, 18 }, // "The stench of sulfur and brimstone..."
            { 3445.0f, -4886.3f, 34 }, // "Destroy the portal energy focus!"
            { 3471.1f, -4839.8f, 20 }  // "Let us shut down this final portal and finish this."
        };
        uint8 text = 13;
        float best = 1000.0f;
        for (Point const& p : points)
        {
            float d = me->GetExactDist2d(p.x, p.y);
            if (d < best)
            {
                best = d;
                text = p.text;
            }
        }
        return text;
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (_done || !who->IsPlayer() || who->ToPlayer()->IsGameMaster() || !me->IsWithinDistInMap(who, 25.0f))
            return;

        _done = true;
        InstanceScript* instance = me->GetInstanceScript();
        if (instance && instance->GetData(DATA_PORTALS_CLOSED) >= 3)
            Talk(22, who); // "The demons are no longer pouring from the palace. We can move ahead."
        else
            Talk(PointText(), who);
    }

    void UpdateAI(uint32 /*diff*/) override { }

private:
    bool _done;
};

// 209366/209447/209448 - Portal Energy Focus
struct go_woe_portal_energy_focus : public GameObjectAI
{
    go_woe_portal_energy_focus(GameObject* go) : GameObjectAI(go) { }

    bool GossipHello(Player* player) override
    {
        InstanceScript* instance = me->GetInstanceScript();
        if (!instance)
            return true;

        // primero hay que acabar con los demonios que protegen el portal
        static uint32 const guards[] = { NPC_DREADLORD_DEFENDER, NPC_CORRUPTED_ARCANIST, NPC_GUARDIAN_DEMON, NPC_LEGION_DEMON };
        for (uint32 entry : guards)
            if (Creature* guard = me->FindNearestCreature(entry, 30.0f, true))
                if (guard->IsAlive())
                {
                    player->GetSession()->SendNotification("Primero acaba con los demonios que protegen este portal.");
                    return true;
                }

        // cerrar el portal: desaparecen el portal y sus conductos/cristales
        static uint32 const portalParts[] = { NPC_LEGION_PORTAL, NPC_PORTAL_CONNECTOR_1, NPC_PORTAL_CONNECTOR_2, NPC_PORTAL_CONNECTOR_3, NPC_FEL_CRYSTAL, NPC_FEL_CRYSTAL_STALKER };
        for (uint32 entry : portalParts)
        {
            std::list<Creature*> parts;
            me->GetCreatureListWithEntryInGrid(parts, entry, 45.0f);
            for (Creature* part : parts)
                part->DespawnOrUnsummon(0s, Seconds(7 * DAY));
        }

        me->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        me->SetGoState(GO_STATE_ACTIVE);
        instance->SetData(DATA_PORTALS_CLOSED, me->GetEntry());

        if (Creature* illidan = me->FindNearestCreature(NPC_ILLIDAN_COURTYARD, 80.0f))
            illidan->AI()->Talk(instance->GetData(DATA_PORTALS_CLOSED) >= 3 ? 21 : 23, player); // "The demons should all be leaving..." / "Too easy."
        return true;
    }
};

// 209997-210000 - Time Transit Device (Pozo de la Eternidad)
struct go_woe_time_transit_device : public GameObjectAI
{
    go_woe_time_transit_device(GameObject* go) : GameObjectAI(go) { }

    struct Destination
    {
        char const* name;
        WorldLocation loc;
        int32 requiredBoss;   // jefe que hay que derrotar para desbloquear (-1: siempre)
        int32 statusBoss;     // jefe cuyo estado se muestra (-1: ninguno)
    };

    static std::array<Destination, 4> const& Destinations()
    {
        static std::array<Destination, 4> const dest =
        {{
            { "Entrada",                                   WorldLocation(939, 3221.7f, -5003.7f, 194.1f, 0.0f), -1,             -1                          },
            { "Puertas del palacio - Peroth'arn",          WorldLocation(939, 3495.4f, -5009.1f, 197.6f, 0.0f), -1,             BOSS_PEROTHARN              },
            { "Palacio de Azshara - Reina Azshara",        WorldLocation(939, 3492.2f, -5200.0f, 229.9f, 0.0f), BOSS_PEROTHARN, BOSS_QUEEN_AZSHARA          },
            { "Pozo de la Eternidad - Mannoroth y Varo'then", WorldLocation(939, 3056.9f, -5564.7f, 18.1f, 0.0f), BOSS_QUEEN_AZSHARA, BOSS_MANNOROTH_AND_VAROTHEN }
        }};
        return dest;
    }

    bool GossipHello(Player* player) override
    {
        InstanceScript* instance = me->GetInstanceScript();
        if (!instance || player->IsInCombat())
            return true;

        ClearGossipMenuFor(player);
        auto const& dest = Destinations();
        for (uint32 i = 0; i < dest.size(); ++i)
        {
            Destination const& d = dest[i];
            if (me->GetExactDist(&d.loc) < 30.0f)
                continue; // no ofrecer el sitio donde ya estás
            if (d.requiredBoss >= 0 && instance->GetBossState(uint32(d.requiredBoss)) != DONE)
                continue;
            // las puertas del palacio solo tras cerrar los 3 portales
            if (i == 1 && instance->GetData(DATA_PORTALS_CLOSED) < 3)
                continue;

            std::string text = d.name;
            if (d.statusBoss >= 0)
                text += instance->GetBossState(uint32(d.statusBoss)) == DONE ? " |cff006400(Derrotado)|r" : " |cff8b0000(Sin derrotar)|r";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, text, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + i);
        }
        SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
        return true;
    }

    bool GossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        uint32 index = player->PlayerTalkClass->GetGossipOptionAction(gossipListId) - GOSSIP_ACTION_INFO_DEF;
        CloseGossipMenuFor(player);
        auto const& dest = Destinations();
        if (index < dest.size())
            player->NearTeleportTo(dest[index].loc);
        return true;
    }
};
}

void AddSC_well_of_eternity()
{
    using namespace WellOfEternity;
    RegisterWellOfEternityCreatureAI(npc_woe_nozdormu);
    RegisterWellOfEternityCreatureAI(npc_woe_illidan_courtyard);
    RegisterGameObjectAI(go_woe_portal_energy_focus);
    RegisterGameObjectAI(go_woe_time_transit_device);
}
