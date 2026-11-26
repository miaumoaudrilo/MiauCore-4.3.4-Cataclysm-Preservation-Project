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
 * Mannoroth y Varo'then (Pozo de la Eternidad) - 4.3.4
 * Datos: DBC 15595 + Diario de encuentros / Wowhead (Cataclysm):
 *  - Al llegar el grupo: Mannoroth ordena a Varo'then que lo proteja ("Varo'then, see that I am not disrupted...").
 *  - Capitán Varo'then (55419): Magigolpe (103669). Mientras vive, Mannoroth no baja del 88%.
 *    Logro That's Not Canon (6070): si con Varo'then vivo se le hace bastante daño a Mannoroth, éste lo sacrifica con
 *    Drenaje vil (104961) para curarse ("Your blood is MINE, elf!").
 *  - Mannoroth (54969): Hoja mácula (103966), Tormenta de Fuego vil (103888), Hendedura abisal (105041, Debilitadores
 *    terroríficos 55762) y ¡Inferno! (105141).
 *  - Al 5%: Malfurion hace implosionar el portal; Mannoroth es arrastrado ("No... no! This victory...") y el encuentro
 *    termina. Se abre el Alijo menor de los Aspectos (209541).
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

namespace WellOfEternity::Mannoroth
{
enum Spells
{
    // Varo'then
    SPELL_MAGISTRIKE        = 103669,
    // Mannoroth
    SPELL_FELBLADE          = 103966,
    SPELL_FEL_FIRESTORM     = 103888,
    SPELL_NETHER_TEAR       = 105041,
    SPELL_INFERNO           = 105141,
    SPELL_FEL_DRAIN         = 104961
};

enum MannorothTexts
{
    SAY_M_AGGRO         = 0,  // "Come, Stormrage, and I will show you..."
    SAY_M_DEFEAT        = 1,  // "No... no! This victory will not be ripped from my grasp!..."
    SAY_M_INTRO         = 2,  // "Varo'then, see that I am not disrupted by this rabble."
    SAY_M_SARGERAS      = 3,
    SAY_M_ROAR          = 4,
    SAY_M_SACRIFICE     = 5,  // "Your blood is MINE, elf!"
    SAY_M_CLOSE         = 6,
    SAY_M_SLAY          = 7,
    SAY_M_DEMONIC       = 8
};

enum VarothenTexts
{
    SAY_V_AGGRO = 0,
    SAY_V_DEATH = 1,
    SAY_V_SLAY  = 2
};

enum IllidanTexts
{
    SAY_I_HANDLE_VAROTHEN   = 14,
    SAY_I_SWORD             = 15,
    SAY_I_POWER             = 12
};

enum Events
{
    EVENT_MAGISTRIKE = 1,
    EVENT_FELBLADE,
    EVENT_FEL_FIRESTORM,
    EVENT_NETHER_TEAR,
    EVENT_INFERNO,
    EVENT_CHECK_WIPE,
    EVENT_OUTRO
};

enum Actions
{
    ACTION_START            = 1,
    ACTION_VAROTHEN_DIED    = 2
};

float const ThatsNotCanonHealthPct = 88.0f; // ~10-12 M de daño con Varo'then vivo (Wowhead)

static Creature* GetIllidan(Creature* me)
{
    return me->FindNearestCreature(NPC_ILLIDAN_WELL, 400.0f);
}

struct boss_mannoroth : public BossAI
{
    boss_mannoroth(Creature* creature) : BossAI(creature, BOSS_MANNOROTH_AND_VAROTHEN), _started(false), _varothenDead(false),
        _sacrificed(false), _infernoDone(false), _powerDone(false), _ending(false) { }

    void Reset() override
    {
        _Reset();
        _started = false;
        _varothenDead = false;
        _sacrificed = false;
        _infernoDone = false;
        _powerDone = false;
        _ending = false;
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (_started || instance->GetBossState(BOSS_MANNOROTH_AND_VAROTHEN) == DONE || !who->IsPlayer()
            || who->ToPlayer()->IsGameMaster() || !me->IsWithinDistInMap(who, 80.0f))
            return;
        StartEncounter();
    }

    void StartEncounter()
    {
        _started = true;
        Talk(SAY_M_INTRO);
        instance->SetBossState(BOSS_MANNOROTH_AND_VAROTHEN, IN_PROGRESS);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
        // Mannoroth es dañable desde el principio (para el logro), pero el daño se limita mientras viva Varo'then
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetFaction(14);
        if (Creature* varothen = instance->GetCreature(DATA_VAROTHEN))
            if (varothen->IsAIEnabled())
                varothen->AI()->DoAction(ACTION_START);
        if (Creature* illidan = GetIllidan(me))
            illidan->AI()->Talk(SAY_I_HANDLE_VAROTHEN);
        events.ScheduleEvent(EVENT_FEL_FIRESTORM, 20s);
        events.ScheduleEvent(EVENT_CHECK_WIPE, 3s);
    }

    void DoAction(int32 action) override
    {
        if (action == ACTION_VAROTHEN_DIED && !_varothenDead)
        {
            _varothenDead = true;
            Talk(SAY_M_AGGRO);
            if (Creature* illidan = GetIllidan(me))
                illidan->AI()->Talk(SAY_I_SWORD);
            me->SetReactState(REACT_AGGRESSIVE);
            DoZoneInCombat();
            events.ScheduleEvent(EVENT_FELBLADE, 8s);
            events.ScheduleEvent(EVENT_NETHER_TEAR, 25s);
        }
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage) override
    {
        if (_ending)
        {
            damage = 0;
            return;
        }

        // That's Not Canon: con Varo'then vivo, al recibir bastante daño Mannoroth lo sacrifica y se cura del todo
        if (!_varothenDead && !_sacrificed && me->HealthBelowPctDamaged(int32(ThatsNotCanonHealthPct), damage))
        {
            _sacrificed = true;
            if (Creature* varothen = instance->GetCreature(DATA_VAROTHEN))
                if (varothen->IsAlive())
                {
                    Talk(SAY_M_SACRIFICE);
                    instance->SetData(DATA_THATS_NOT_CANON, 1);
                    me->CastSpell(varothen, SPELL_FEL_DRAIN, true);
                    Unit::Kill(me, varothen);
                    me->SetFullHealth();
                    damage = 0;
                    return;
                }
        }

        if (!_varothenDead && me->HealthBelowPctDamaged(int32(ThatsNotCanonHealthPct), damage))
            damage = 0; // sin el sacrificio no baja de aquí hasta que caiga Varo'then

        if (!_powerDone && me->HealthBelowPctDamaged(85, damage))
        {
            _powerDone = true;
            if (Creature* illidan = GetIllidan(me))
                illidan->AI()->Talk(SAY_I_POWER);
        }

        if (!_infernoDone && me->HealthBelowPctDamaged(40, damage))
        {
            _infernoDone = true;
            Talk(SAY_M_SARGERAS);
            events.ScheduleEvent(EVENT_INFERNO, 1s);
        }

        // al 5%: Malfurion cierra el portal y Mannoroth es arrastrado
        if (me->HealthBelowPctDamaged(5, damage))
        {
            damage = 0;
            if (!_ending)
            {
                _ending = true;
                events.Reset();
                me->InterruptNonMeleeSpells(true);
                me->AttackStop();
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                Talk(SAY_M_DEFEAT);
                events.ScheduleEvent(EVENT_OUTRO, 10s);
            }
        }
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->IsPlayer())
            Talk(SAY_M_SLAY, victim);
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
            summon->AI()->AttackStart(target);
    }

    bool AnyPlayerAlive() const
    {
        for (MapReference const& ref : me->GetMap()->GetPlayers())
            if (Player* player = ref.GetSource())
                if (player->IsAlive() && !player->IsGameMaster() && me->IsWithinDistInMap(player, 120.0f))
                    return true;
        return false;
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (_ending)
            return;
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        summons.DespawnAll();
        if (Creature* varothen = instance->GetCreature(DATA_VAROTHEN))
        {
            if (varothen->isDead())
                varothen->Respawn(true);
            else if (varothen->IsAIEnabled())
                varothen->AI()->EnterEvadeMode();
        }
        _DespawnAtEvade(Seconds(15));
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
                case EVENT_CHECK_WIPE:
                    if (!AnyPlayerAlive())
                    {
                        EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
                        return;
                    }
                    events.Repeat(3s);
                    break;
                case EVENT_FEL_FIRESTORM:
                    DoCastSelf(SPELL_FEL_FIRESTORM);
                    events.Repeat(30s, 35s);
                    break;
                case EVENT_FELBLADE:
                    DoCastSelf(SPELL_FELBLADE);
                    events.Repeat(25s, 30s);
                    break;
                case EVENT_NETHER_TEAR:
                    Talk(SAY_M_DEMONIC);
                    DoCastSelf(SPELL_NETHER_TEAR);
                    for (uint8 i = 0; i < 2; ++i)
                        me->SummonCreature(NPC_DREADLORD_DEBILITATOR, me->GetNearPosition(15.0f, float(i) * float(M_PI)), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10s);
                    events.Repeat(45s, 50s);
                    break;
                case EVENT_INFERNO:
                    DoCastSelf(SPELL_INFERNO);
                    break;
                case EVENT_OUTRO:
                    summons.DespawnAll();
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    instance->SetBossState(BOSS_MANNOROTH_AND_VAROTHEN, DONE);
                    me->CombatStop(true);
                    me->DespawnOrUnsummon(2s, Seconds(7 * DAY));
                    return;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        if (_varothenDead && !_ending && UpdateVictim())
            DoMeleeAttackIfReady();
    }

private:
    bool _started;
    bool _varothenDead;
    bool _sacrificed;
    bool _infernoDone;
    bool _powerDone;
    bool _ending;
};

// 55419 - Captain Varo'then
struct npc_woe_captain_varothen : public ScriptedAI
{
    npc_woe_captain_varothen(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript()) { }

    void Reset() override
    {
        _events.Reset();
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetReactState(REACT_PASSIVE);
    }

    void DoAction(int32 action) override
    {
        if (action != ACTION_START)
            return;
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetFaction(14);
        me->SetReactState(REACT_AGGRESSIVE);
        Talk(SAY_V_AGGRO);
        DoZoneInCombat();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_MAGISTRIKE, 6s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->IsPlayer())
            Talk(SAY_V_SLAY, victim);
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_V_DEATH);
        if (_instance)
            if (Creature* mannoroth = _instance->GetCreature(DATA_MANNOROTH))
                if (mannoroth->IsAIEnabled())
                    mannoroth->AI()->DoAction(ACTION_VAROTHEN_DIED);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (_events.ExecuteEvent() == EVENT_MAGISTRIKE)
        {
            DoCastVictim(SPELL_MAGISTRIKE);
            _events.Repeat(8s, 12s);
        }

        DoMeleeAttackIfReady();
    }

private:
    InstanceScript* _instance;
    EventMap _events;
};
}

void AddSC_boss_mannoroth()
{
    using namespace WellOfEternity;
    using namespace WellOfEternity::Mannoroth;
    RegisterWellOfEternityCreatureAI(boss_mannoroth);
    RegisterWellOfEternityCreatureAI(npc_woe_captain_varothen);
}
