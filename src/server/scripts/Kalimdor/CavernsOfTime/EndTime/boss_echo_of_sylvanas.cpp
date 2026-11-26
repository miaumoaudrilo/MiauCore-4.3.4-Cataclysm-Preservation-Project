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
 * Eco de Sylvanas (Fin de los Días, Santuario de Dragones Rubí) - 4.3.4
 * Datos: DBC 15595 + Diario de encuentros / Wowhead (Cataclysm):
 *  - Disparo profano (101411), Chillido de los Altonato (101412), Flecha negra (101404): sobre jugadores aleatorios.
 *  - Flechas contagiadas: marca el suelo bajo un jugador (PNJ 54403, 101552 -> 103171 cada 1 s, -50% velocidad),
 *    salta y al caer las flechas derriban a quien siga dentro (100763).
 *  - La llamada de los Altonato: atrae al grupo (101397), se vuelve inmune (100686) e invoca 8 necrófagos alzados
 *    (100894/100917-100924 -> 54197 -> 101200 -> 54191) en círculo a 30 m. Avanzan hacia ella unidos por Dolor atroz
 *    (101221 a quien cruce la cadena); reciben más daño cuanto más heridos están. Al llegar al centro: Sacrificio (101348).
 *  - Logro Lazos cortados (6130): matar 2 necrófagos alzados durante La llamada y derrotar al eco.
 */

#include "end_time.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "TemporarySummon.h"

namespace EndTime::EchoOfSylvanas
{
enum Spells
{
    SPELL_UNHOLY_SHOT               = 101411,
    SPELL_SHRIEK_OF_THE_HIGHBORNE   = 101412,
    SPELL_BLACK_ARROW               = 101404,
    SPELL_BLIGHTED_ARROWS_PERIODIC  = 101552,
    SPELL_BLIGHTED_ARROWS_KNOCKBACK = 100763,
    SPELL_DEATH_GRIP                = 101987, // salto del jugador hacia Sylvanas (101397)
    SPELL_CALLING_IMMUNE            = 100686,
    SPELL_SUMMON_GHOUL_N            = 100894,
    SPELL_SUMMON_GHOUL_NE           = 100917,
    SPELL_SUMMON_GHOUL_NW           = 100918,
    SPELL_SUMMON_GHOUL_E            = 100919,
    SPELL_SUMMON_GHOUL_W            = 100920,
    SPELL_SUMMON_GHOUL_S            = 100921,
    SPELL_SUMMON_GHOUL_SE           = 100923,
    SPELL_SUMMON_GHOUL_SW           = 100924,
    SPELL_SUMMON_RISEN_GHOUL        = 101200,
    SPELL_WRACKING_PAIN_DAMAGE      = 101221,
    SPELL_SACRIFICE                 = 101348
};

enum Events
{
    EVENT_UNHOLY_SHOT = 1,
    EVENT_SHRIEK,
    EVENT_BLACK_ARROW,
    EVENT_BLIGHTED_ARROWS,
    EVENT_CALLING,
    EVENT_CALLING_SUMMON,
    EVENT_WRACKING_PAIN,
    EVENT_CALLING_END,

    // Flechas contagiadas
    EVENT_ARROWS_LAND
};

enum Texts
{
    SAY_INTRO   = 0,
    SAY_AGGRO   = 1,
    SAY_SLAY    = 2,
    SAY_CALLING = 3,
    SAY_DEATH   = 4
};

enum Phases
{
    PHASE_COMBAT  = 1,
    PHASE_CALLING = 2
};

enum Actions
{
    ACTION_GHOUL_REACHED_CENTER = 1
};

static std::array<uint32, 8> const SummonGhoulSpells =
{
    SPELL_SUMMON_GHOUL_N, SPELL_SUMMON_GHOUL_NE, SPELL_SUMMON_GHOUL_E, SPELL_SUMMON_GHOUL_SE,
    SPELL_SUMMON_GHOUL_S, SPELL_SUMMON_GHOUL_SW, SPELL_SUMMON_GHOUL_W, SPELL_SUMMON_GHOUL_NW
};

// Distancia de un punto al segmento AB (plano XY)
static float DistToSegment2d(Position const& p, Position const& a, Position const& b)
{
    float dx = b.GetPositionX() - a.GetPositionX();
    float dy = b.GetPositionY() - a.GetPositionY();
    float len2 = dx * dx + dy * dy;
    float t = len2 > 0.0f ? ((p.GetPositionX() - a.GetPositionX()) * dx + (p.GetPositionY() - a.GetPositionY()) * dy) / len2 : 0.0f;
    t = std::max(0.0f, std::min(1.0f, t));
    float px = a.GetPositionX() + t * dx - p.GetPositionX();
    float py = a.GetPositionY() + t * dy - p.GetPositionY();
    return std::sqrt(px * px + py * py);
}

struct boss_echo_of_sylvanas : public BossAI
{
    boss_echo_of_sylvanas(Creature* creature) : BossAI(creature, DATA_ECHO_OF_SYLVANAS), _introDone(false), _sacrificeDone(false) { }

    void Reset() override
    {
        _Reset();
        me->RemoveAurasDueToSpell(SPELL_CALLING_IMMUNE);
        me->SetReactState(REACT_AGGRESSIVE);
        _ghouls.clear();
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!_introDone && who->IsPlayer() && me->IsWithinDistInMap(who, 50.0f) && !who->ToPlayer()->IsGameMaster())
        {
            _introDone = true;
            Talk(SAY_INTRO);
        }
        BossAI::MoveInLineOfSight(who);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO, who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
        events.SetPhase(PHASE_COMBAT);
        events.ScheduleEvent(EVENT_UNHOLY_SHOT, 8s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_SHRIEK, 13s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_BLACK_ARROW, 18s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_BLIGHTED_ARROWS, 22s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_CALLING, 40s, 0, PHASE_COMBAT);
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

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
        if (summon->GetEntry() == NPC_RISEN_GHOUL)
        {
            _ghouls.push_back(summon->GetGUID());
            summon->SetReactState(REACT_PASSIVE);
            summon->SetWalk(true);
            summon->SetSpeedRate(MOVE_WALK, 0.8f); // ~15 s desde el círculo (30 m) hasta Sylvanas
            summon->GetMotionMaster()->MovePoint(1, me->GetPosition(), true);
        }
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (summon->GetEntry() != NPC_RISEN_GHOUL || !events.IsInPhase(PHASE_CALLING) || _sacrificeDone)
            return;

        instance->SetData(DATA_RISEN_GHOUL_KILLED_IN_CALLING, 1);

        // si el grupo acaba con todos los necrófagos, La llamada termina sin Sacrificio
        bool anyAlive = false;
        for (ObjectGuid const& guid : _ghouls)
            if (Creature* ghoul = ObjectAccessor::GetCreature(*me, guid))
                if (ghoul->IsAlive() && ghoul != summon)
                    anyAlive = true;
        if (!anyAlive)
        {
            _sacrificeDone = true;
            events.ScheduleEvent(EVENT_CALLING_END, 1s, 0, PHASE_CALLING);
        }
    }

    void DoAction(int32 action) override
    {
        if (action == ACTION_GHOUL_REACHED_CENTER && events.IsInPhase(PHASE_CALLING) && !_sacrificeDone)
        {
            _sacrificeDone = true;
            for (ObjectGuid const& guid : _ghouls)
                if (Creature* ghoul = ObjectAccessor::GetCreature(*me, guid))
                    if (ghoul->IsAlive())
                    {
                        ghoul->CastSpell(ghoul, SPELL_SACRIFICE, true);
                        ghoul->KillSelf();
                    }
            events.ScheduleEvent(EVENT_CALLING_END, 1s, 0, PHASE_CALLING);
        }
    }

    // Dolor atroz: la cadena entre necrófagos vecinos daña a quien la cruce.
    void HandleWrackingPain()
    {
        std::vector<Creature*> alive;
        for (ObjectGuid const& guid : _ghouls)
            if (Creature* ghoul = ObjectAccessor::GetCreature(*me, guid))
                if (ghoul->IsAlive())
                    alive.push_back(ghoul);

        if (alive.size() < 2)
            return;

        // ordenar por ángulo alrededor de Sylvanas para unir solo a los vecinos
        std::sort(alive.begin(), alive.end(), [this](Creature* a, Creature* b) { return me->GetAngle(a) < me->GetAngle(b); });

        for (MapReference const& ref : me->GetMap()->GetPlayers())
        {
            Player* player = ref.GetSource();
            if (!player || !player->IsAlive() || player->IsGameMaster() || !me->IsWithinDistInMap(player, 50.0f))
                continue;

            for (size_t i = 0; i < alive.size(); ++i)
            {
                Creature* a = alive[i];
                Creature* b = alive[(i + 1) % alive.size()];
                // si falta el necrófago de en medio queda un hueco: no hay cadena entre vecinos separados por más de 90º
                if (Position::NormalizeOrientation(me->GetAngle(b) - me->GetAngle(a)) > float(M_PI) / 3.0f && alive.size() > 2)
                    continue;
                if (DistToSegment2d(player->GetPosition(), a->GetPosition(), b->GetPosition()) < 1.5f)
                {
                    player->CastSpell(player, SPELL_WRACKING_PAIN_DAMAGE, CastSpellExtraArgs(TRIGGERED_FULL_MASK).SetOriginalCaster(a->GetGUID()));
                    break;
                }
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_UNHOLY_SHOT:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        DoCast(target, SPELL_UNHOLY_SHOT);
                    events.Repeat(14s, 18s);
                    break;
                case EVENT_SHRIEK:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        DoCast(target, SPELL_SHRIEK_OF_THE_HIGHBORNE);
                    events.Repeat(20s, 24s);
                    break;
                case EVENT_BLACK_ARROW:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        DoCast(target, SPELL_BLACK_ARROW);
                    events.Repeat(18s, 22s);
                    break;
                case EVENT_BLIGHTED_ARROWS:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        me->SummonCreature(NPC_BLIGHTED_ARROWS, target->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 6s);
                    me->GetMotionMaster()->MoveJump(me->GetPosition(), 1.0f, 15.0f, 0);
                    events.Repeat(25s, 30s);
                    break;
                case EVENT_CALLING:
                {
                    Talk(SAY_CALLING);
                    events.SetPhase(PHASE_CALLING);
                    _sacrificeDone = false;
                    _ghouls.clear();
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->StopMoving();
                    DoCastSelf(SPELL_CALLING_IMMUNE, true);
                    // Atracción letal: todo el grupo salta hacia ella
                    for (MapReference const& ref : me->GetMap()->GetPlayers())
                        if (Player* player = ref.GetSource())
                            if (player->IsAlive() && !player->IsGameMaster() && me->IsWithinDistInMap(player, 60.0f))
                                player->CastSpell(me, SPELL_DEATH_GRIP, true);
                    events.ScheduleEvent(EVENT_CALLING_SUMMON, 2s, 0, PHASE_CALLING);
                    break;
                }
                case EVENT_CALLING_SUMMON:
                    for (uint32 spell : SummonGhoulSpells)
                        DoCastSelf(spell, true);
                    events.ScheduleEvent(EVENT_WRACKING_PAIN, 2s, 0, PHASE_CALLING);
                    break;
                case EVENT_WRACKING_PAIN:
                    HandleWrackingPain();
                    events.Repeat(1s);
                    break;
                case EVENT_CALLING_END:
                    _ghouls.clear();
                    me->RemoveAurasDueToSpell(SPELL_CALLING_IMMUNE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    events.SetPhase(PHASE_COMBAT);
                    events.ScheduleEvent(EVENT_UNHOLY_SHOT, 5s, 0, PHASE_COMBAT);
                    events.ScheduleEvent(EVENT_SHRIEK, 9s, 0, PHASE_COMBAT);
                    events.ScheduleEvent(EVENT_BLACK_ARROW, 14s, 0, PHASE_COMBAT);
                    events.ScheduleEvent(EVENT_BLIGHTED_ARROWS, 18s, 0, PHASE_COMBAT);
                    events.ScheduleEvent(EVENT_CALLING, 45s, 0, PHASE_COMBAT);
                    if (Unit* victim = me->GetVictim())
                        AttackStart(victim);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        if (events.IsInPhase(PHASE_COMBAT))
            DoMeleeAttackIfReady();
    }

private:
    bool _introDone;
    bool _sacrificeDone;
    std::vector<ObjectGuid> _ghouls;
};

// 54197 - Ghoul: marcador del círculo; crea al Necrófago alzado en su sitio y desaparece.
struct npc_echo_of_sylvanas_ghoul_spawner : public ScriptedAI
{
    npc_echo_of_sylvanas_ghoul_spawner(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(Unit* summoner) override
    {
        me->SetReactState(REACT_PASSIVE);
        // el necrófago lo invoca Sylvanas en este punto (101200 en la DBC); así queda entre sus invocaciones
        if (summoner)
            summoner->SummonCreature(NPC_RISEN_GHOUL, me->GetPosition(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5s);
        me->DespawnOrUnsummon(500ms);
    }

    void UpdateAI(uint32 /*diff*/) override { }
};

// 54191 - Risen Ghoul: camina hacia Sylvanas; recibe más daño cuanto más herido está; Sacrificio al llegar.
struct npc_echo_of_sylvanas_risen_ghoul : public ScriptedAI
{
    npc_echo_of_sylvanas_risen_ghoul(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage) override
    {
        // +10% de daño recibido por cada 5% de salud que le falta
        uint32 missingSteps = uint32((100.0f - me->GetHealthPct()) / 5.0f);
        damage = uint32(damage * (1.0f + 0.10f * missingSteps));
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || id != 1)
            return;

        if (TempSummon* summon = me->ToTempSummon())
            if (Creature* sylvanas = summon->GetSummonerCreatureBase())
                if (sylvanas->IsAIEnabled())
                    sylvanas->AI()->DoAction(ACTION_GHOUL_REACHED_CENTER);
    }

    void UpdateAI(uint32 /*diff*/) override { }
};

// 54403 - Blighted Arrows: suelo contagiado bajo el jugador; a los 3 s caen las flechas (derribo).
struct npc_echo_of_sylvanas_blighted_arrows : public ScriptedAI
{
    npc_echo_of_sylvanas_blighted_arrows(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(Unit* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        DoCastSelf(SPELL_BLIGHTED_ARROWS_PERIODIC, true);
        _events.ScheduleEvent(EVENT_ARROWS_LAND, 3s);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        if (_events.ExecuteEvent() == EVENT_ARROWS_LAND)
        {
            // las flechas caen sobre el suelo contagiado (el área sale de este punto; el daño es de Sylvanas)
            ObjectGuid sylvanasGuid;
            if (TempSummon* summon = me->ToTempSummon())
                sylvanasGuid = summon->GetSummonerGUID();
            me->CastSpell(me, SPELL_BLIGHTED_ARROWS_KNOCKBACK, CastSpellExtraArgs(TRIGGERED_FULL_MASK).SetOriginalCaster(sylvanasGuid));
            me->RemoveAurasDueToSpell(SPELL_BLIGHTED_ARROWS_PERIODIC);
            me->DespawnOrUnsummon(1s);
        }
    }

private:
    EventMap _events;
};

// 103171 / 100763 / 101348 - áreas de radio corto alrededor del lanzador
class spell_echo_of_sylvanas_short_radius : public SpellScript
{
public:
    spell_echo_of_sylvanas_short_radius(float radius) : _radius(radius) { }

private:
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        WorldObject* center = GetCaster();
        Position const* dest = GetExplTargetDest();
        targets.remove_if([&](WorldObject* target)
        {
            return dest ? target->GetExactDist2d(dest) > _radius : target->GetExactDist2d(center) > _radius;
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_echo_of_sylvanas_short_radius::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENEMY);
    }

    float _radius;
};
}

void AddSC_boss_echo_of_sylvanas()
{
    using namespace EndTime;
    using namespace EndTime::EchoOfSylvanas;
    RegisterEndTimeCreatureAI(boss_echo_of_sylvanas);
    RegisterEndTimeCreatureAI(npc_echo_of_sylvanas_ghoul_spawner);
    RegisterEndTimeCreatureAI(npc_echo_of_sylvanas_risen_ghoul);
    RegisterEndTimeCreatureAI(npc_echo_of_sylvanas_blighted_arrows);
    RegisterSpellScriptWithArgs(spell_echo_of_sylvanas_short_radius, "spell_echo_of_sylvanas_blighted_ground", 3.0f);
    RegisterSpellScriptWithArgs(spell_echo_of_sylvanas_short_radius, "spell_echo_of_sylvanas_blighted_arrows_knockback", 3.0f);
    RegisterSpellScriptWithArgs(spell_echo_of_sylvanas_short_radius, "spell_echo_of_sylvanas_sacrifice", 8.0f);
}
