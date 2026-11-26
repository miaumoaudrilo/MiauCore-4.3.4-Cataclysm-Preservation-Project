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
 * Peroth'arn (Pozo de la Eternidad) - 4.3.4
 * Datos: DBC 15595 + Diario de encuentros / Wowhead (Cataclysm):
 *  - Toque corruptor (104939): sus golpes aumentan un 10% el daño recibido por el objetivo (acumulable).
 *  - Llamas viles (108141) sobre un jugador aleatorio; Descomposición vil (105544) sobre un jugador aleatorio.
 *  - Al 60%: Drenar esencia (104905, aturde a todos 4 s) y se oculta (Camuflaje 105341). Los Ojos de Peroth'arn (55879)
 *    buscan al grupo: si uno detecta a alguien, Presa fácil (105493) y Peroth'arn ataca a ese jugador. Si nadie es
 *    detectado, vuelve con "¡BASTA!" y Aceleración vil (105526).
 *  - Al 20%: Frenesí infinito (105521).
 *  - Antes de cerrar los 3 portales es inmune; su aparición en la entrada del patio solo da órdenes y se marcha.
 *  - Logro Lazy Eye (6127): derrotarlo sin que ningún ojo detecte al grupo.
 */

#include "well_of_eternity.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"

namespace WellOfEternity::Perotharn
{
enum Spells
{
    SPELL_CORRUPTING_TOUCH  = 104939,
    SPELL_FEL_FLAMES        = 108141,
    SPELL_FEL_DECAY         = 105544,
    SPELL_DRAIN_ESSENCE     = 104905,
    SPELL_CAMOUFLAGE        = 105341,
    SPELL_EASY_PREY         = 105493,
    SPELL_FEL_QUICKENING    = 105526,
    SPELL_ENDLESS_FRENZY    = 105521
};

enum Texts
{
    SAY_AGGRO           = 0,
    SAY_DEATH           = 1,
    SAY_ENOUGH          = 2,
    SAY_HIDE_START      = 3,
    SAY_INTRO_FELGUARD  = 4,
    SAY_INTRO_COURTYARD = 5,
    SAY_FOUND           = 6,
    SAY_HIDE_TAUNT_1    = 7,
    SAY_HIDE_TAUNT_2    = 8,
    SAY_PORTALS_CLOSED  = 9,
    SAY_SLAY            = 10,
    SAY_DRAIN_ESSENCE   = 11,
    SAY_SHADOWS         = 12
};

enum Events
{
    EVENT_FEL_FLAMES = 1,
    EVENT_FEL_DECAY,
    EVENT_HIDE,
    EVENT_EYE_CHECK,
    EVENT_HIDE_TAUNT,
    EVENT_HIDE_TIMEOUT,
    EVENT_INTRO_2,
    EVENT_INTRO_LEAVE
};

enum Phases
{
    PHASE_NORMAL = 1,
    PHASE_HIDDEN = 2
};

enum Actions
{
    ACTION_PORTALS_CLOSED = 1
};

Position const PalaceDoors = { 3464.8f, -5094.6f, 214.0f, 0.0f };

struct boss_perotharn : public BossAI
{
    boss_perotharn(Creature* creature) : BossAI(creature, BOSS_PEROTHARN), _isIntro(false), _introDone(false), _activated(false), _drained(false), _frenzy(false) { }

    void InitializeAI() override
    {
        // la aparición junto a la entrada del patio es la del recorrido: da órdenes y se marcha
        _isIntro = me->GetExactDist2d(&PalaceDoors) > 100.0f;
        BossAI::InitializeAI();
    }

    void Reset() override
    {
        _Reset();
        _drained = false;
        _frenzy = false;
        me->RemoveAurasDueToSpell(SPELL_CAMOUFLAGE);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        if (_isIntro || instance->GetData(DATA_PORTALS_CLOSED) < 3)
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        else
        {
            _activated = true;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        }
    }

    void DoAction(int32 action) override
    {
        if (action == ACTION_PORTALS_CLOSED && !_isIntro && !_activated)
        {
            _activated = true;
            Talk(SAY_PORTALS_CLOSED);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        }
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (_isIntro)
        {
            if (!_introDone && who->IsPlayer() && !who->ToPlayer()->IsGameMaster() && me->IsWithinDistInMap(who, 40.0f))
            {
                _introDone = true;
                Talk(SAY_INTRO_FELGUARD);
                events.ScheduleEvent(EVENT_INTRO_2, 5s);
            }
            return;
        }
        BossAI::MoveInLineOfSight(who);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO, who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
        DoCastSelf(SPELL_CORRUPTING_TOUCH, true);
        events.SetPhase(PHASE_NORMAL);
        events.ScheduleEvent(EVENT_FEL_FLAMES, 10s, 0, PHASE_NORMAL);
        events.ScheduleEvent(EVENT_FEL_DECAY, 15s, 0, PHASE_NORMAL);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage) override
    {
        if (!_drained && me->HealthBelowPctDamaged(60, damage))
        {
            _drained = true;
            Talk(SAY_DRAIN_ESSENCE);
            DoCastAOE(SPELL_DRAIN_ESSENCE, true);
            events.ScheduleEvent(EVENT_HIDE, 4500ms, 0, PHASE_NORMAL);
        }
        if (!_frenzy && me->HealthBelowPctDamaged(20, damage))
        {
            _frenzy = true;
            DoCastSelf(SPELL_ENDLESS_FRENZY, true);
        }
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->IsPlayer())
            Talk(SAY_SLAY, victim);
    }

    void JustDied(Unit* killer) override
    {
        BossAI::JustDied(killer);
        Talk(SAY_DEATH, killer);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        summons.DespawnAll();
        BossAI::EnterEvadeMode(why);
    }

    void StartHide()
    {
        Talk(SAY_SHADOWS);
        events.SetPhase(PHASE_HIDDEN);
        me->AttackStop();
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        DoCastSelf(SPELL_CAMOUFLAGE, true);

        // Ojos de Peroth'arn repartidos alrededor del grupo
        for (uint8 i = 0; i < 6; ++i)
        {
            float angle = float(i) * float(M_PI) / 3.0f;
            Position pos = me->GetNearPosition(15.0f + float(urand(0, 10)), angle);
            if (Creature* eye = me->SummonCreature(NPC_EYE_OF_PEROTHARN, pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 45s))
            {
                eye->SetReactState(REACT_PASSIVE);
                eye->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);
                eye->GetMotionMaster()->MoveRandom(10.0f);
            }
        }

        Talk(SAY_HIDE_START);
        events.ScheduleEvent(EVENT_EYE_CHECK, 500ms, 0, PHASE_HIDDEN);
        events.ScheduleEvent(EVENT_HIDE_TAUNT, 10s, 0, PHASE_HIDDEN);
        events.ScheduleEvent(EVENT_HIDE_TIMEOUT, 40s, 0, PHASE_HIDDEN);
    }

    void EndHide(Unit* target, bool found)
    {
        summons.DespawnEntry(NPC_EYE_OF_PEROTHARN);
        me->RemoveAurasDueToSpell(SPELL_CAMOUFLAGE);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        me->SetReactState(REACT_AGGRESSIVE);
        events.SetPhase(PHASE_NORMAL);
        events.CancelEvent(EVENT_EYE_CHECK);
        events.CancelEvent(EVENT_HIDE_TAUNT);
        events.CancelEvent(EVENT_HIDE_TIMEOUT);

        if (found && target)
        {
            Talk(SAY_FOUND, target);
            AddThreat(target, 1000000.0f);
            AttackStart(target);
        }
        else
        {
            Talk(SAY_ENOUGH);
            DoCastSelf(SPELL_FEL_QUICKENING, true);
            if (Unit* victim = SelectTarget(SELECT_TARGET_MAXTHREAT, 0))
                AttackStart(victim);
        }
        events.ScheduleEvent(EVENT_FEL_FLAMES, 6s, 0, PHASE_NORMAL);
        events.ScheduleEvent(EVENT_FEL_DECAY, 12s, 0, PHASE_NORMAL);
    }

    // ¿algún ojo ve a un jugador? (a 5 m)
    Player* CheckEyes()
    {
        for (ObjectGuid const& guid : summons)
            if (Creature* eye = ObjectAccessor::GetCreature(*me, guid))
                if (eye->GetEntry() == NPC_EYE_OF_PEROTHARN && eye->IsAlive())
                    for (MapReference const& ref : me->GetMap()->GetPlayers())
                        if (Player* player = ref.GetSource())
                            if (player->IsAlive() && !player->IsGameMaster() && eye->IsWithinDist(player, 5.0f))
                            {
                                eye->CastSpell(player, SPELL_EASY_PREY, true);
                                return player;
                            }
        return nullptr;
    }

    void UpdateAI(uint32 diff) override
    {
        if (_isIntro)
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_INTRO_2)
                {
                    Talk(SAY_INTRO_COURTYARD);
                    events.ScheduleEvent(EVENT_INTRO_LEAVE, 6s);
                }
                else if (eventId == EVENT_INTRO_LEAVE)
                    me->DespawnOrUnsummon(0s, Seconds(7 * DAY));
            }
            return;
        }

        // hay dos Peroth'arn (recorrido y palacio): el del palacio comprueba él mismo si ya se cerraron los portales
        if (!_activated && !me->IsInCombat() && instance->GetData(DATA_PORTALS_CLOSED) >= 3)
            DoAction(ACTION_PORTALS_CLOSED);

        if (!UpdateVictim() && !events.IsInPhase(PHASE_HIDDEN))
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_FEL_FLAMES:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        DoCast(target, SPELL_FEL_FLAMES);
                    events.Repeat(12s, 16s);
                    break;
                case EVENT_FEL_DECAY:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        DoCast(target, SPELL_FEL_DECAY);
                    events.Repeat(18s, 22s);
                    break;
                case EVENT_HIDE:
                    StartHide();
                    break;
                case EVENT_EYE_CHECK:
                    if (Player* found = CheckEyes())
                    {
                        instance->SetData(DATA_LAZY_EYE_FAILED, 1);
                        EndHide(found, true);
                    }
                    else
                        events.Repeat(500ms);
                    break;
                case EVENT_HIDE_TAUNT:
                    Talk(urand(0, 1) ? SAY_HIDE_TAUNT_1 : SAY_HIDE_TAUNT_2);
                    events.Repeat(10s);
                    break;
                case EVENT_HIDE_TIMEOUT:
                    EndHide(nullptr, false);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        if (events.IsInPhase(PHASE_NORMAL))
            DoMeleeAttackIfReady();
    }

private:
    bool _isIntro;
    bool _introDone;
    bool _activated;
    bool _drained;
    bool _frenzy;
};
}

void AddSC_boss_perotharn()
{
    using namespace WellOfEternity;
    using namespace WellOfEternity::Perotharn;
    RegisterWellOfEternityCreatureAI(boss_perotharn);
}
