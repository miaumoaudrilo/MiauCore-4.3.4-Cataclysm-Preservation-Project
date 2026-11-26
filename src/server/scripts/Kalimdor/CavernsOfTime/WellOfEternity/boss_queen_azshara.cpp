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
 * Reina Azshara (Pozo de la Eternidad) - 4.3.4
 * Datos: DBC 15595 + Diario de encuentros / Wowhead (Cataclysm):
 *  - Azshara no se combate (Embozo de luminosidad 102915, inmune). Sus 6 magos encantados atacan de 2 en 2:
 *    Fuego 54882 (Bola de Fuego 102265, Ola explosiva 102483, Bomba de Fuego 102482), Escarcha 54883 (Arrojar hielo
 *    102478, Hojas de hielo 102467, Llama fría 102465) y Arcano 54884 (Choque Arcano 102463).
 *  - Sirviente de la Reina (102334): controla a un jugador; matar a la Mano de la Reina (54728, sus hilos) lo libera.
 *  - Total obediencia (103241): 10 s de lanzamiento, hay que interrumpirla o controla a todo el grupo 20 s.
 *  - Al caer los 6 magos: "¡Basta!", llama a Varo'then y se marcha. Se abre el Alijo real (210025).
 */

#include "well_of_eternity.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "TemporarySummon.h"

namespace WellOfEternity::QueenAzshara
{
enum Spells
{
    SPELL_SHROUD_OF_LUMINOSITY  = 102915,
    SPELL_SERVANT_OF_THE_QUEEN  = 102334,
    SPELL_TOTAL_OBEDIENCE       = 103241,

    // Magos
    SPELL_FIREBALL              = 102265,
    SPELL_BLAST_WAVE            = 102483,
    SPELL_FIREBOMB              = 102482,
    SPELL_ICE_FLING             = 102478,
    SPELL_BLADES_OF_ICE         = 102467,
    SPELL_COLDFLAME             = 102465,
    SPELL_ARCANE_SHOCK          = 102463
};

enum Texts
{
    SAY_INTRO           = 0,
    SAY_INTERRUPTED     = 1,
    SAY_ENOUGH          = 2,
    SAY_RIDERS          = 3,
    SAY_VAROTHEN        = 4,
    SAY_SLAY            = 6,
    SAY_SERVANT_ALT     = 7,
    SAY_SERVANT         = 8,
    SAY_WAVE_3          = 9,
    SAY_WAVE_2          = 10,
    SAY_WAVE_1          = 11
};

enum Events
{
    EVENT_NEXT_WAVE = 1,
    EVENT_SERVANT,
    EVENT_TOTAL_OBEDIENCE,
    EVENT_CHECK_MAGI,
    EVENT_LEAVE,

    // Magos
    EVENT_MAGUS_1,
    EVENT_MAGUS_2,
    EVENT_MAGUS_3
};

enum Actions
{
    ACTION_MAGUS_DIED = 1
};

struct boss_queen_azshara : public BossAI
{
    boss_queen_azshara(Creature* creature) : BossAI(creature, BOSS_QUEEN_AZSHARA), _wave(0), _started(false) { }

    void Reset() override
    {
        _Reset();
        _wave = 0;
        _started = false;
        me->SetReactState(REACT_PASSIVE);
        DoCastSelf(SPELL_SHROUD_OF_LUMINOSITY, true);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        ResetMagi();
    }

    std::vector<Creature*> Magi() const
    {
        std::vector<Creature*> result;
        for (uint32 entry : { NPC_ENCHANTED_MAGUS_FIRE, NPC_ENCHANTED_MAGUS_FROST, NPC_ENCHANTED_MAGUS_ARCANE })
        {
            std::list<Creature*> list;
            me->GetCreatureListWithEntryInGrid(list, entry, 80.0f);
            for (Creature* c : list)
                result.push_back(c);
        }
        // orden estable: por distancia a la reina
        std::sort(result.begin(), result.end(), [this](Creature* a, Creature* b) { return me->GetExactDist2d(a) < me->GetExactDist2d(b); });
        return result;
    }

    void ResetMagi()
    {
        for (Creature* magus : Magi())
        {
            if (!magus->IsAlive())
                magus->Respawn(true);
            magus->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            magus->SetReactState(REACT_PASSIVE);
        }
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (_started || instance->GetBossState(BOSS_QUEEN_AZSHARA) == DONE || !who->IsPlayer() || who->ToPlayer()->IsGameMaster()
            || !me->IsWithinDistInMap(who, 40.0f))
            return;

        _started = true;
        Talk(SAY_INTRO, who);
        instance->SetBossState(BOSS_QUEEN_AZSHARA, IN_PROGRESS);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
        DoZoneInCombat();
        events.ScheduleEvent(EVENT_NEXT_WAVE, 8s);
        events.ScheduleEvent(EVENT_SERVANT, 20s);
        events.ScheduleEvent(EVENT_TOTAL_OBEDIENCE, 35s);
        events.ScheduleEvent(EVENT_CHECK_MAGI, 2s);
    }

    void ActivateWave()
    {
        // los magos aún pasivos y vivos, de 2 en 2
        uint8 activated = 0;
        for (Creature* magus : Magi())
        {
            if (activated >= 2)
                break;
            if (!magus->IsAlive() || !magus->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC))
                continue;
            magus->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            magus->SetReactState(REACT_AGGRESSIVE);
            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                magus->AI()->AttackStart(target);
            ++activated;
        }
        if (activated)
        {
            ++_wave;
            Talk(_wave == 1 ? SAY_WAVE_1 : (_wave == 2 ? SAY_WAVE_2 : SAY_WAVE_3));
        }
    }

    // Total obediencia interrumpida: "Bold of you, to strike a queen..."
    void OnSpellCastFinished(SpellInfo const* spell, SpellFinishReason reason) override
    {
        if (spell->Id == SPELL_TOTAL_OBEDIENCE && reason != SPELL_FINISHED_SUCCESSFUL_CAST)
            Talk(SAY_INTERRUPTED);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->IsPlayer())
            Talk(SAY_SLAY, victim);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (instance->GetBossState(BOSS_QUEEN_AZSHARA) == DONE)
            return;
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        summons.DespawnAll();
        for (MapReference const& ref : me->GetMap()->GetPlayers())
            if (Player* player = ref.GetSource())
                player->RemoveAurasDueToSpell(SPELL_SERVANT_OF_THE_QUEEN);
        _DespawnAtEvade(Seconds(10));
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
    }

    bool AnyPlayerAlive() const
    {
        for (MapReference const& ref : me->GetMap()->GetPlayers())
            if (Player* player = ref.GetSource())
                if (player->IsAlive() && !player->IsGameMaster() && me->IsWithinDistInMap(player, 100.0f))
                    return true;
        return false;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!_started)
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_NEXT_WAVE:
                    ActivateWave();
                    // la siguiente pareja llega aunque la anterior siga en pie
                    events.Repeat(60s);
                    break;
                case EVENT_SERVANT:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        if (target->IsPlayer() && !target->HasAura(SPELL_SERVANT_OF_THE_QUEEN))
                        {
                            Talk(urand(0, 1) ? SAY_SERVANT : SAY_SERVANT_ALT, target);
                            me->AddAura(SPELL_SERVANT_OF_THE_QUEEN, target);
                            if (Creature* hand = me->SummonCreature(NPC_HAND_OF_THE_QUEEN, target->GetPosition(), TEMPSUMMON_CORPSE_DESPAWN))
                                if (hand->IsAIEnabled())
                                    hand->AI()->SetGUID(target->GetGUID());
                        }
                    events.Repeat(25s, 30s);
                    break;
                case EVENT_TOTAL_OBEDIENCE:
                    DoCastAOE(SPELL_TOTAL_OBEDIENCE);
                    events.Repeat(40s, 45s);
                    break;
                case EVENT_CHECK_MAGI:
                {
                    if (!AnyPlayerAlive())
                    {
                        EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
                        return;
                    }

                    bool anyAlive = false;
                    bool anyActiveAlive = false;
                    for (Creature* magus : Magi())
                        if (magus->IsAlive())
                        {
                            anyAlive = true;
                            if (!magus->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC))
                                anyActiveAlive = true;
                        }

                    if (!anyAlive)
                    {
                        // todos los magos han caído: Azshara se marcha
                        events.Reset();
                        me->InterruptNonMeleeSpells(true);
                        Talk(SAY_ENOUGH);
                        events.ScheduleEvent(EVENT_LEAVE, 8s);
                        break;
                    }
                    if (!anyActiveAlive)
                    {
                        // la pareja actual ha caído: la siguiente entra ya
                        events.RescheduleEvent(EVENT_NEXT_WAVE, 3s);
                    }
                    events.Repeat(2s);
                    break;
                }
                case EVENT_LEAVE:
                    Talk(SAY_VAROTHEN);
                    for (MapReference const& ref : me->GetMap()->GetPlayers())
                        if (Player* player = ref.GetSource())
                            player->RemoveAurasDueToSpell(SPELL_SERVANT_OF_THE_QUEEN);
                    summons.DespawnAll();
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    instance->SetBossState(BOSS_QUEEN_AZSHARA, DONE);
                    me->CombatStop(true);
                    me->DespawnOrUnsummon(4s, Seconds(7 * DAY));
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    uint8 _wave;
    bool _started;
};

// 54882/54883/54884 - Enchanted Magus
struct npc_woe_enchanted_magus : public ScriptedAI
{
    npc_woe_enchanted_magus(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_MAGUS_1, 1s);
        _events.ScheduleEvent(EVENT_MAGUS_2, 8s);
        _events.ScheduleEvent(EVENT_MAGUS_3, 14s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (me->GetEntry())
            {
                case NPC_ENCHANTED_MAGUS_FIRE:
                    if (eventId == EVENT_MAGUS_1) { DoCastVictim(SPELL_FIREBALL); _events.Repeat(3s); }
                    else if (eventId == EVENT_MAGUS_2) { DoCastAOE(SPELL_BLAST_WAVE); _events.Repeat(14s, 18s); }
                    else if (eventId == EVENT_MAGUS_3) { if (Unit* t = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true)) DoCast(t, SPELL_FIREBOMB); _events.Repeat(12s, 15s); }
                    break;
                case NPC_ENCHANTED_MAGUS_FROST:
                    if (eventId == EVENT_MAGUS_1) { DoCastVictim(SPELL_ICE_FLING); _events.Repeat(3s); }
                    else if (eventId == EVENT_MAGUS_2) { if (Unit* t = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true)) DoCast(t, SPELL_BLADES_OF_ICE); _events.Repeat(15s, 18s); }
                    else if (eventId == EVENT_MAGUS_3) { if (Unit* t = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true)) DoCast(t, SPELL_COLDFLAME); _events.Repeat(12s, 15s); }
                    break;
                case NPC_ENCHANTED_MAGUS_ARCANE:
                    if (eventId == EVENT_MAGUS_1) { DoCastSelf(SPELL_ARCANE_SHOCK); _events.Repeat(10s, 12s); }
                    else if (eventId == EVENT_MAGUS_2) { DoCastVictim(SPELL_FIREBALL); _events.Repeat(4s); }
                    break;
                default:
                    break;
            }
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
};

// 54728 - Hand of the Queen: los hilos de la marioneta; al morir libera al jugador.
struct npc_woe_hand_of_the_queen : public ScriptedAI
{
    npc_woe_hand_of_the_queen(Creature* creature) : ScriptedAI(creature) { }

    void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
    {
        _puppet = guid;
        me->SetReactState(REACT_PASSIVE);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Player* puppet = ObjectAccessor::GetPlayer(*me, _puppet))
            puppet->RemoveAurasDueToSpell(SPELL_SERVANT_OF_THE_QUEEN);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        // sigue a su marioneta
        if (Player* puppet = ObjectAccessor::GetPlayer(*me, _puppet))
        {
            if (!puppet->HasAura(SPELL_SERVANT_OF_THE_QUEEN))
                me->DespawnOrUnsummon();
            else if (!me->IsWithinDist(puppet, 3.0f))
                me->GetMotionMaster()->MovePoint(0, puppet->GetPosition(), false);
        }
        else
            me->DespawnOrUnsummon();
    }

private:
    ObjectGuid _puppet;
};
}

void AddSC_boss_queen_azshara()
{
    using namespace WellOfEternity;
    using namespace WellOfEternity::QueenAzshara;
    RegisterWellOfEternityCreatureAI(boss_queen_azshara);
    RegisterWellOfEternityCreatureAI(npc_woe_enchanted_magus);
    RegisterWellOfEternityCreatureAI(npc_woe_hand_of_the_queen);
}
