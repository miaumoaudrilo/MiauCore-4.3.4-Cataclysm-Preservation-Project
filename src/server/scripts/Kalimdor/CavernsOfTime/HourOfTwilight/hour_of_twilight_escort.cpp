/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 * Distributed under the GNU General Public License, version 2 or later.
 */

#include "hour_of_twilight.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"
#include <iterator>

namespace HourOfTwilight
{
namespace
{
struct Checkpoint
{
    Position Location;
    float AmbushRadius;
    int8 Text;
};

Checkpoint const FinalApproach[] =
{
    { { 4927.88f, 288.89f, 96.86f }, 0.0f, 1 },
    { { 4926.84f, 260.52f, 97.09f }, 16.0f, 2 },
    { { 4901.77f, 217.12f, 99.23f }, 18.0f, 4 },
    { { 4878.83f, 180.00f, 99.18f }, 0.0f, 3 },
    { { 4864.29f, 159.25f, 97.53f }, 0.0f, 5 }
};

Checkpoint const TwilightAmbush[] =
{
    { { 4383.12f, 425.65f, 14.53f }, 18.0f, 0 },
    { { 4365.50f, 472.60f, -5.35f }, 10.0f, 1 },
    { { 4294.48f, 406.39f, -7.77f }, 28.0f, 2 },
    { { 4287.39f, 463.43f, -6.48f }, 17.0f, 0 },
    { { 4234.58f, 538.04f, 12.49f }, 16.0f, 1 },
    { { 4229.33f, 552.39f, 13.76f }, 0.0f, 5 }
};

Checkpoint const TempleApproach[] =
{
    { { 3906.71f, 252.47f, 8.30f }, 14.0f, 1 },
    { { 3813.52f, 324.78f, -32.36f }, 16.0f, 2 },
    { { 3727.03f, 266.56f, -92.37f }, 15.0f, 1 },
    { { 3563.01f, 274.59f, -115.88f }, 0.0f, 3 }
};

Position const TempleLanding = { 3943.19f, 288.46f, 13.50f, 3.9f };

enum TempleAmbushSpells
{
    SPELL_TEMPLE_AMBUSH_DORMANT = 102231,
    SPELL_TEMPLE_AMBUSH_DORMANT_EFFECT = 102230,
    SPELL_THRALL_TUNNEL_ROOT = 102550
};

uint32 const TempleAmbushFlags = UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE |
    UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC;

bool IsSpanish(Player const* player)
{
    LocaleConstant const locale = player->GetSession()->GetSessionDbLocaleIndex();
    return locale == LOCALE_esES || locale == LOCALE_esMX;
}
}

struct npc_hot_thrall_escort : ScriptedAI
{
    npc_hot_thrall_escort(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript())
    {
        switch (creature->GetEntry())
        {
            case NPC_THRALL_ENTRANCE:
                _boss = DATA_ARCURION;
                _route = FinalApproach;
                _count = std::size(FinalApproach);
                break;
            case NPC_THRALL_ASIRA:
                _boss = DATA_ASIRA_DAWNSLAYER;
                _route = TwilightAmbush;
                _count = std::size(TwilightAmbush);
                break;
            case NPC_THRALL_GAUNTLET:
                _boss = DATA_ARCHBISHOP_BENEDICTUS;
                _route = TempleApproach;
                _count = std::size(TempleApproach);
                break;
        }
        me->SetReactState(REACT_PASSIVE);
    }

    bool GossipHello(Player* player) override
    {
        ClearGossipMenuFor(player);
        if (me->IsQuestGiver())
            player->PrepareQuestMenu(me->GetGUID());
        if (CanStart(player))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                IsSpanish(player) ? "Estamos listos, Thrall. Guíanos." : "We're ready, Thrall. Lead the way.",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        if (CanTravel(player))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                IsSpanish(player) ? "Llévame a la entrada del Templo del Reposo del Dragón." : "Take me to the entrance of Wyrmrest Temple.",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
        return true;
    }

    bool GossipSelect(Player* player, uint32, uint32 gossipListId) override
    {
        uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
        CloseGossipMenuFor(player);
        if (action == GOSSIP_ACTION_INFO_DEF && CanStart(player))
        {
            _active = true;
            _next = 0;
            if (_boss == DATA_ARCHBISHOP_BENEDICTUS)
                me->RemoveAurasDueToSpell(SPELL_THRALL_TUNNEL_ROOT);
            if (_boss != DATA_ASIRA_DAWNSLAYER)
                Talk(0);
            MoveNext();
        }
        else if (action == GOSSIP_ACTION_INFO_DEF + 1 && CanTravel(player))
            player->NearTeleportTo(TempleLanding);
        return true;
    }

    void MovementInform(uint32 type, uint32 point) override
    {
        if (!_active || type != POINT_MOTION_TYPE || point != _next + 1)
            return;
        _moving = false;
        Checkpoint const& checkpoint = _route[_next];
        if (checkpoint.Text >= 0)
            Talk(checkpoint.Text);
        if (checkpoint.AmbushRadius > 0.0f)
        {
            std::list<Creature*> nearby;
            switch (_boss)
            {
                case DATA_ARCURION:
                    GetCreatureListWithEntryInGrid(nearby, me, 54555, checkpoint.AmbushRadius);
                    break;
                case DATA_ASIRA_DAWNSLAYER:
                    for (uint32 entry : { 55106u, 55107u, 55109u, 55111u, 55112u })
                        GetCreatureListWithEntryInGrid(nearby, me, entry, checkpoint.AmbushRadius);
                    break;
                case DATA_ARCHBISHOP_BENEDICTUS:
                    for (uint32 entry : { 54686u, 54632u, 54633u })
                        GetCreatureListWithEntryInGrid(nearby, me, entry, checkpoint.AmbushRadius);
                    break;
            }
            for (Creature* enemy : nearby)
                if (enemy->IsAlive())
                {
                    _ambush.push_back(enemy->GetGUID());
                    if (_boss == DATA_ARCHBISHOP_BENEDICTUS)
                    {
                        enemy->RemoveAurasDueToSpell(SPELL_TEMPLE_AMBUSH_DORMANT);
                        enemy->RemoveAurasDueToSpell(SPELL_TEMPLE_AMBUSH_DORMANT_EFFECT);
                        enemy->RemoveFlag(UNIT_FIELD_FLAGS, TempleAmbushFlags);
                        enemy->SetFaction(14);
                        enemy->SetReactState(REACT_AGGRESSIVE);
                    }
                }
        }
        _timer = 2200;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!_active)
            return;
        Player* participant = nullptr;
        for (auto const& reference : me->GetMap()->GetPlayers())
            if (Player* player = reference.GetSource())
                if (player->IsAlive() && me->GetDistance(player) < 115.0f)
                {
                    participant = player;
                    break;
                }
        if (!participant)
        {
            _abandoned += diff;
            if (_abandoned >= 25000)
            {
                _active = false;
                _moving = false;
                ResetTempleAmbush();
                _ambush.clear();
                if (_intro)
                {
                    _instance->SetData(DATA_BENEDICTUS_INTRO, 0);
                    me->SetVisible(true);
                }
                _intro = 0;
                _abandoned = 0;
                me->GetMotionMaster()->Clear();
                me->NearTeleportTo(me->GetHomePosition());
            }
            return;
        }
        _abandoned = 0;
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 1000;

        if (_moving)
        {
            // Recover a stalled path only when a living player has reached the checkpoint.
            for (auto const& reference : me->GetMap()->GetPlayers())
                if (Player* player = reference.GetSource())
                    if (player->IsAlive() && player->GetDistance(_route[_next].Location) < 12.0f)
                    {
                        me->GetMotionMaster()->Clear();
                        me->NearTeleportTo(_route[_next].Location);
                        MovementInform(POINT_MOTION_TYPE, _next + 1);
                        return;
                    }
            MoveNext();
            return;
        }

        bool ambushAlive = false;
        for (ObjectGuid const& guid : _ambush)
            if (Creature* enemy = ObjectAccessor::GetCreature(*me, guid))
                if (enemy->IsAlive())
                {
                    ambushAlive = true;
                    if (_boss == DATA_ARCHBISHOP_BENEDICTUS && !enemy->GetVictim())
                        EngageTempleEnemy(enemy);
                }
        if (ambushAlive)
            return;
        _ambush.clear();

        if (_intro)
        {
            AdvanceIntro();
            return;
        }
        if (++_next < _count)
        {
            MoveNext();
            return;
        }

        if (_boss == DATA_ARCHBISHOP_BENEDICTUS)
        {
            _intro = 1;
            AdvanceIntro();
            return;
        }
        if (_boss == DATA_ARCURION)
        {
            if (Creature* support = _instance->GetCreature(DATA_THRALL_ARCURION))
                support->NearTeleportTo({ 4775.0f, 69.0f, 66.45f, 3.8f });
        }
        else if (Creature* asira = _instance->GetCreature(DATA_ASIRA_DAWNSLAYER))
            asira->AI()->Talk(1);
        Unlock();
    }

private:
    void EngageTempleEnemy(Creature* enemy) const
    {
        Player* nearest = nullptr;
        float distance = 35.0f;
        for (auto const& reference : me->GetMap()->GetPlayers())
            if (Player* player = reference.GetSource())
                if (player->IsAlive() && !player->IsGameMaster() && enemy->IsInPhase(player))
                    if (float const current = enemy->GetDistance(player); current < distance && enemy->IsWithinLOSInMap(player))
                    {
                        nearest = player;
                        distance = current;
                    }
        if (nearest)
            if (CreatureAI* ai = enemy->AI())
                ai->AttackStart(nearest);
    }

    void ResetTempleAmbush()
    {
        if (_boss != DATA_ARCHBISHOP_BENEDICTUS)
            return;

        for (ObjectGuid const& guid : _ambush)
            if (Creature* enemy = ObjectAccessor::GetCreature(*me, guid))
                if (enemy->IsAlive())
                {
                    if (CreatureAI* ai = enemy->AI())
                        ai->EnterEvadeMode();
                    CreatureTemplate const* creatureTemplate = enemy->GetCreatureTemplate();
                    enemy->SetFaction(creatureTemplate->faction);
                    enemy->SetFlag(UNIT_FIELD_FLAGS, creatureTemplate->unit_flags & TempleAmbushFlags);
                    enemy->CastSpell(enemy, SPELL_TEMPLE_AMBUSH_DORMANT, true);
                }
    }

    bool CanStart(Player const* player) const
    {
        return _route && !_active && !player->IsInCombat() &&
            _instance->GetBossState(_boss) != DONE && !_instance->GetData(DATA_ARCURION_READY + _boss) &&
            (!_boss || _instance->GetBossState(_boss - 1) == DONE);
    }

    bool CanTravel(Player const* player) const
    {
        return (me->GetEntry() == NPC_THRALL_ASIRA || me->GetEntry() == NPC_THRALL_ENTRANCE ||
            me->GetEntry() == 55415 || me->GetEntry() == 55549) &&
            !player->IsInCombat() && _instance->GetBossState(DATA_ASIRA_DAWNSLAYER) == DONE;
    }

    void MoveNext()
    {
        _moving = true;
        _timer = 22000;
        me->SetWalk(false);
        me->GetMotionMaster()->MovePoint(_next + 1, _route[_next].Location);
    }

    void AdvanceIntro()
    {
        Creature* benedictus = _instance->GetCreature(DATA_ARCHBISHOP_BENEDICTUS);
        Creature* thrall = _instance->GetCreature(DATA_THRALL_BENEDICTUS);
        if (!benedictus || !thrall)
            return;

        switch (_intro++)
        {
            case 1:
                me->SetVisible(false);
                _instance->SetData(DATA_BENEDICTUS_INTRO, 1);
                benedictus->AI()->Talk(0);
                break;
            case 2: benedictus->AI()->Talk(1); break;
            case 3: thrall->AI()->Talk(0); break;
            case 4: benedictus->AI()->Talk(2); break;
            case 5: thrall->AI()->Talk(1); break;
            case 6: benedictus->AI()->Talk(3); break;
            case 7: benedictus->AI()->Talk(4); break;
            default:
                Unlock();
                return;
        }
        _timer = 4300;
    }

    void Unlock()
    {
        _active = false;
        _intro = 0;
        _instance->SetData(DATA_ARCURION_READY + _boss, 1);
    }

    InstanceScript* const _instance;
    Checkpoint const* _route = nullptr;
    uint32 _boss = 0;
    uint32 _count = 0;
    uint32 _next = 0;
    uint32 _timer = 0;
    uint32 _abandoned = 0;
    uint8 _intro = 0;
    bool _active = false;
    bool _moving = false;
    std::vector<ObjectGuid> _ambush;
};
}

void AddSC_hour_of_twilight_escort()
{
    using namespace HourOfTwilight;
    RegisterHourOfTwilightCreatureAI(npc_hot_thrall_escort);
}
