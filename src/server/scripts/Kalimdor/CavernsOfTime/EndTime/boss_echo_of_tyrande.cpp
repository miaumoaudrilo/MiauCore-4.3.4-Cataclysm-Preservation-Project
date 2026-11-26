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
 * Eco de Tyrande (Fin de los Días, Santuario de Dragones Esmeralda) - 4.3.4
 * Datos: DBC 15595 + Diario de encuentros / Wowhead (Cataclysm):
 *  Guantelete de sombras: Tyrande está envuelta en sombras (101841, -90% daño recibido, no seleccionable). Hay que superar
 *  5 estanques de luz de luna (54508): al acercarse se activa la luz (Luz lunar 101842 quita "En sombras" a los enemigos
 *  dentro) y atacan los guerreros del tiempo de alrededor. Al limpiar el 5.º, Tyrande se vuelve atacable.
 *  Combate:
 *   - Luz de luna oscura (102414): -50% velocidad de lanzamiento en 15 m alrededor de ella.
 *   - Descarga lunar (102193) al objetivo; Polvo estelar (102173, interrumpible) a todo el grupo.
 *   - Picaluna (102151): lanza (54574) que avanza y se divide en tres (102152), cada parte 102149 (daño + aturdir 5 s, 2 m).
 *   - Ojos de la diosa (102181 / 102605 / 102606): dos ojos (54594...) orbitan y lanzan Mirada penetrante (102183, 6 m, silencio).
 *   - Guía de la Luna (102472) al 80% y al 55%: +25% de celeridad por acumulación.
 *   - Lágrimas de Elune (102241) al 30%: llueven lágrimas (102243 -> 102244, 4 m) el resto del combate.
 *  Logro Guardia lunar (5995): llegar a Tyrande sin que el sanador reciba daño durante el guantelete.
 */

#include "end_time.h"
#include "Group.h"
#include "InstanceScript.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "TemporarySummon.h"

namespace EndTime::EchoOfTyrande
{
enum Spells
{
    SPELL_IN_SHADOWS            = 101841,
    SPELL_MOONLIT               = 101842,
    SPELL_DARK_MOONLIGHT        = 102414,
    SPELL_MOONBOLT              = 102193,
    SPELL_STARDUST              = 102173,
    SPELL_MOONLANCE             = 102151,
    SPELL_MOONLANCE_PERIODIC    = 102150,
    SPELL_MOONLANCE_SPLIT       = 102152,
    SPELL_EYES_OF_THE_GODDESS_1 = 102181,
    SPELL_EYES_OF_THE_GODDESS_2 = 102605,
    SPELL_EYES_OF_THE_GODDESS_3 = 102606,
    SPELL_PIERCING_GAZE         = 102182,
    SPELL_LUNAR_GUIDANCE        = 102472,
    SPELL_TEARS_OF_ELUNE        = 102241,
    SPELL_TEARS_OF_ELUNE_MISSILE = 102243
};

enum Events
{
    EVENT_GAUNTLET_CHECK = 1,
    EVENT_MOONBOLT,
    EVENT_STARDUST,
    EVENT_MOONLANCE,
    EVENT_EYES,

    // Moonlance
    EVENT_MOONLANCE_SPLIT,
    // Eye of Elune
    EVENT_EYE_ORBIT
};

enum Texts
{
    SAY_INTRO           = 0,
    SAY_POOL_1          = 1, // 1..5: un estanque por grupo
    SAY_GAUNTLET_DONE   = 6,
    SAY_EYES            = 7,
    SAY_MOONLANCE       = 8,
    SAY_LUNAR_GUIDANCE  = 9,
    EMOTE_TEARS         = 10,
    SAY_TEARS           = 11,
    SAY_DEATH           = 12
};

enum Phases
{
    PHASE_GAUNTLET = 1,
    PHASE_COMBAT   = 2
};

uint32 const GauntletTrash[] = { NPC_TIME_TWISTED_NIGHTSABER_1, NPC_TIME_TWISTED_NIGHTSABER_2, NPC_TIME_TWISTED_NIGHTSABER_3, NPC_TIME_TWISTED_SENTINEL, NPC_TIME_TWISTED_HUNTRESS };

float const PoolActivationRange = 25.0f; // jugador cerca del estanque para encender la luz
float const PoolTrashRange      = 25.0f; // guerreros del tiempo que defienden ese estanque
float const MoonlightRadius     = 12.0f; // radio de la luz del estanque

static bool IsGauntletTrash(uint32 entry)
{
    return std::find(std::begin(GauntletTrash), std::end(GauntletTrash), entry) != std::end(GauntletTrash);
}

// Sanador del grupo: rol de búsqueda de mazmorras o árbol de talentos de sanación (4.3.4).
static bool IsHealer(Player* player)
{
    if (Group* group = player->GetGroup())
        if (group->isLFGGroup() && (group->GetLfgRoles(player->GetGUID()) & 8 /*PLAYER_ROLE_HEALER*/))
            return true;

    switch (player->GetPrimaryTalentTree(player->GetActiveSpec()))
    {
        case 760: // Sacerdote Disciplina
        case 813: // Sacerdote Sagrado
        case 831: // Paladín Sagrado
        case 748: // Druida Restauración
        case 262: // Chamán Restauración
            return true;
        default:
            return false;
    }
}

struct boss_echo_of_tyrande : public BossAI
{
    boss_echo_of_tyrande(Creature* creature) : BossAI(creature, DATA_ECHO_OF_TYRANDE), _introDone(false), _guidanceStacks(0), _tearsDone(false) { }

    void InitializeAI() override
    {
        BossAI::InitializeAI();
        SortPools();
        UpdateShadowState();
    }

    void Reset() override
    {
        _Reset();
        _guidanceStacks = 0;
        _tearsDone = false;
        UpdateShadowState();
        events.SetPhase(PHASE_GAUNTLET);
        events.ScheduleEvent(EVENT_GAUNTLET_CHECK, 1s, 0, PHASE_GAUNTLET);
    }

    // Estanques en el orden del camino desde la Imagen de Nozdormu.
    void SortPools()
    {
        _pools.clear();
        std::list<Creature*> pools;
        me->GetCreatureListWithEntryInGrid(pools, NPC_POOL_OF_MOONLIGHT, 200.0f);
        Position start = me->GetPosition();
        if (Creature* image = me->FindNearestCreature(NPC_IMAGE_OF_NOZDORMU, 200.0f))
            start = image->GetPosition();

        std::vector<Creature*> ordered(pools.begin(), pools.end());
        // ordenar por recorrido: siempre el estanque más cercano al anterior
        Position current = start;
        while (!ordered.empty())
        {
            auto it = std::min_element(ordered.begin(), ordered.end(), [&](Creature* a, Creature* b) { return a->GetExactDist2d(current) < b->GetExactDist2d(current); });
            _pools.push_back((*it)->GetGUID());
            current = (*it)->GetPosition();
            ordered.erase(it);
        }
    }

    // Progreso del guantelete: máscara de estanques superados (bit = índice en _pools). 5 estanques -> 0x1F.
    uint32 DoneMask() const { return instance->GetData(DATA_TYRANDE_GAUNTLET) & 0x1F; }
    static uint32 PoolsDone(uint32 mask) { uint32 n = 0; for (; mask; mask &= mask - 1) ++n; return n; }

    bool GauntletDone() const
    {
        return PoolsDone(DoneMask()) >= 5;
    }

    void UpdateShadowState()
    {
        if (GauntletDone())
        {
            me->RemoveAurasDueToSpell(SPELL_IN_SHADOWS);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        }
        else
        {
            if (!me->HasAura(SPELL_IN_SHADOWS))
                me->AddAura(SPELL_IN_SHADOWS, me);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        }
    }

    void CompletePool(Creature* pool, uint32 index)
    {
        if (pool)
            pool->RemoveAurasDueToSpell(SPELL_MOONLIT);
        instance->SetData(DATA_MOONLIGHT_POOL_DONE, 1u << index);
        if (GauntletDone())
        {
            Talk(SAY_GAUNTLET_DONE);
            UpdateShadowState();
        }
    }

    // Cada estanque se enciende cuando un jugador se acerca (en cualquier orden) y se supera al morir sus defensores.
    void GauntletUpdate()
    {
        if (GauntletDone())
            return;

        if (_pools.size() < 5)
            SortPools();

        // seguridad: si ya no queda ningún guerrero del tiempo vivo en el santuario, el guantelete está superado
        bool anyTrashAlive = false;
        for (uint32 entry : GauntletTrash)
            if (Creature* trash = me->FindNearestCreature(entry, 200.0f, true))
                if (trash->IsAlive())
                    anyTrashAlive = true;

        for (uint32 i = 0; i < _pools.size() && i < 5; ++i)
        {
            if (DoneMask() & (1u << i))
                continue;

            Creature* pool = ObjectAccessor::GetCreature(*me, _pools[i]);
            if (!pool)
                continue;

            if (!anyTrashAlive)
            {
                CompletePool(pool, i);
                continue;
            }

            Player* nearPlayer = nullptr;
            for (MapReference const& ref : me->GetMap()->GetPlayers())
                if (Player* player = ref.GetSource())
                    if (player->IsAlive() && !player->IsGameMaster() && pool->IsWithinDist2d(player, PoolActivationRange))
                        nearPlayer = player;

            bool active = pool->HasAura(SPELL_MOONLIT);
            if (!active && nearPlayer)
            {
                if (!_introDone)
                {
                    _introDone = true;
                    Talk(SAY_INTRO);
                }
                pool->AddAura(SPELL_MOONLIT, pool);
                Talk(SAY_POOL_1 + std::min<uint32>(PoolsDone(DoneMask()), 4));
                for (Creature* trash : PoolDefenders(pool))
                    if (trash->IsAIEnabled() && !trash->IsInCombat())
                        trash->AI()->AttackStart(nearPlayer);
                active = true;
            }

            if (!active)
                continue;

            // jugador más cercano al estanque (para que los defensores vayan a por él)
            Player* closest = nearPlayer;
            if (!closest)
                for (MapReference const& ref : me->GetMap()->GetPlayers())
                    if (Player* player = ref.GetSource())
                        if (player->IsAlive() && !player->IsGameMaster() && pool->IsWithinDist2d(player, 60.0f)
                            && (!closest || pool->GetExactDist2d(player) < pool->GetExactDist2d(closest)))
                            closest = player;

            // luz de luna: quita "En sombras" a los defensores dentro de la luz; fuera de ella la recuperan.
            // Los defensores que no estén luchando acuden al estanque a por el jugador.
            std::vector<Creature*> defenders = PoolDefenders(pool);
            for (Creature* trash : defenders)
            {
                if (trash->IsWithinDist2d(pool, MoonlightRadius))
                    trash->RemoveAurasDueToSpell(SPELL_IN_SHADOWS);
                else if (!trash->HasAura(SPELL_IN_SHADOWS))
                    trash->AddAura(SPELL_IN_SHADOWS, trash);

                if (closest && !trash->IsInCombat() && trash->IsAIEnabled() && !trash->IsInEvadeMode())
                    trash->AI()->AttackStart(closest);
            }

            if (defenders.empty())
                CompletePool(pool, i);
            else if (++_gauntletLogTimer >= 10)
            {
                _gauntletLogTimer = 0;
                for (Creature* trash : defenders)
                    TC_LOG_INFO("scripts", "Fin de los Días - guantelete de Tyrande: estanque %u (%.0f, %.0f) espera a %s (GUID %u) en %.1f %.1f %.1f, en combate: %s",
                        i + 1, pool->GetPositionX(), pool->GetPositionY(), trash->GetName().c_str(), trash->GetSpawnId(),
                        trash->GetPositionX(), trash->GetPositionY(), trash->GetPositionZ(), trash->IsInCombat() ? "sí" : "no");
            }
        }
    }

    std::vector<Creature*> PoolDefenders(Creature* pool) const
    {
        std::vector<Creature*> result;
        for (uint32 entry : GauntletTrash)
        {
            std::list<Creature*> list;
            pool->GetCreatureListWithEntryInGrid(list, entry, PoolTrashRange);
            for (Creature* c : list)
                if (c->IsAlive())
                    result.push_back(c);
        }
        return result;
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
        DoCastSelf(SPELL_DARK_MOONLIGHT, true);
        events.SetPhase(PHASE_COMBAT);
        events.ScheduleEvent(EVENT_MOONBOLT, 1s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_MOONLANCE, 8s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_STARDUST, 12s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_EYES, 20s, 0, PHASE_COMBAT);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage) override
    {
        if (!events.IsInPhase(PHASE_COMBAT))
            return;

        float pct = me->HealthBelowPctDamaged(80, damage) ? 80.f : 100.f;
        if (_guidanceStacks == 0 && pct <= 80.f)
        {
            ++_guidanceStacks;
            Talk(SAY_LUNAR_GUIDANCE);
            DoCastSelf(SPELL_LUNAR_GUIDANCE);
        }
        else if (_guidanceStacks == 1 && me->HealthBelowPctDamaged(55, damage))
        {
            ++_guidanceStacks;
            Talk(SAY_LUNAR_GUIDANCE);
            DoCastSelf(SPELL_LUNAR_GUIDANCE);
        }
        else if (!_tearsDone && me->HealthBelowPctDamaged(30, damage))
        {
            _tearsDone = true;
            Talk(SAY_TEARS);
            Talk(EMOTE_TEARS);
            DoCastSelf(SPELL_TEARS_OF_ELUNE);
        }
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
        switch (summon->GetEntry())
        {
            case NPC_EYE_OF_ELUNE_1:
            case NPC_EYE_OF_ELUNE_2:
            case NPC_EYE_OF_ELUNE_3:
            case NPC_EYE_OF_ELUNE_4:
            case NPC_EYE_OF_ELUNE_5:
            case NPC_EYE_OF_ELUNE_6:
                summon->SetReactState(REACT_PASSIVE);
                summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                summon->CastSpell(summon, SPELL_PIERCING_GAZE, true);
                break;
            default:
                break;
        }
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

    void UpdateAI(uint32 diff) override
    {
        if (events.IsInPhase(PHASE_GAUNTLET) || !me->IsInCombat())
        {
            events.Update(diff);
            if (events.ExecuteEvent() == EVENT_GAUNTLET_CHECK)
            {
                GauntletUpdate();
                events.ScheduleEvent(EVENT_GAUNTLET_CHECK, 1s, 0, PHASE_GAUNTLET);
            }
            if (!me->IsInCombat())
                return;
        }

        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_MOONBOLT:
                    DoCastVictim(SPELL_MOONBOLT);
                    events.Repeat(3s);
                    break;
                case EVENT_STARDUST:
                    DoCastAOE(SPELL_STARDUST);
                    events.Repeat(15s, 18s);
                    break;
                case EVENT_MOONLANCE:
                    // Picaluna siempre va al jugador más alejado
                    if (Unit* target = SelectTarget(SELECT_TARGET_MAXDISTANCE, 0, 100.0f, true))
                    {
                        Talk(SAY_MOONLANCE);
                        DoCast(target, SPELL_MOONLANCE);
                    }
                    events.Repeat(8s, 10s);
                    break;
                case EVENT_EYES:
                    Talk(SAY_EYES);
                    DoCastSelf(_guidanceStacks == 0 ? SPELL_EYES_OF_THE_GODDESS_1 : (_guidanceStacks == 1 ? SPELL_EYES_OF_THE_GODDESS_2 : SPELL_EYES_OF_THE_GODDESS_3));
                    events.Repeat(30s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    bool _introDone;
    uint8 _guidanceStacks;
    bool _tearsDone;
    std::vector<ObjectGuid> _pools;
    uint32 _gauntletLogTimer = 0;
};

// 54574 - Moonlance: avanza en línea recta alejándose de Tyrande y a los 3 s se divide en tres (102152).
// 54580/54581/54582 - partes de la Picaluna: salen en abanico (-30º, 0º, +30º).
struct npc_echo_of_tyrande_moonlance : public ScriptedAI
{
    npc_echo_of_tyrande_moonlance(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(Unit* summoner) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        DoCastSelf(SPELL_MOONLANCE_PERIODIC, true);

        float angle = summoner ? summoner->GetAngle(me) : me->GetOrientation();
        switch (me->GetEntry())
        {
            case NPC_MOONLANCE_SPLIT_1: angle -= float(M_PI) / 6.0f; break;
            case NPC_MOONLANCE_SPLIT_3: angle += float(M_PI) / 6.0f; break;
            default: break;
        }
        // las partes usan la dirección de la lanza original (su invocador)
        if (me->GetEntry() != NPC_MOONLANCE && summoner && summoner->GetEntry() == NPC_MOONLANCE)
            if (TempSummon const* parent = summoner->ToTempSummon())
                if (Unit* tyrande = parent->GetSummoner())
                {
                    angle = tyrande->GetAngle(summoner);
                    if (me->GetEntry() == NPC_MOONLANCE_SPLIT_1) angle -= float(M_PI) / 6.0f;
                    if (me->GetEntry() == NPC_MOONLANCE_SPLIT_3) angle += float(M_PI) / 6.0f;
                }

        float dist = me->GetEntry() == NPC_MOONLANCE ? 15.0f : 25.0f;
        Position dest(me->GetPositionX() + std::cos(angle) * dist, me->GetPositionY() + std::sin(angle) * dist, me->GetPositionZ());
        me->UpdateGroundPositionZ(dest.m_positionX, dest.m_positionY, dest.m_positionZ);
        me->SetSpeedRate(MOVE_RUN, 0.6f);
        me->GetMotionMaster()->MovePoint(0, dest, false);

        if (me->GetEntry() == NPC_MOONLANCE)
            _events.ScheduleEvent(EVENT_MOONLANCE_SPLIT, 3s);
        else
            me->DespawnOrUnsummon(8s);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        if (_events.ExecuteEvent() == EVENT_MOONLANCE_SPLIT)
        {
            DoCastSelf(SPELL_MOONLANCE_SPLIT, true);
            me->DespawnOrUnsummon(200ms);
        }
    }

private:
    EventMap _events;
};

// 54594/54597/54939-54942 - Eye of Elune: orbitan alrededor de Tyrande (no atacables).
struct npc_echo_of_tyrande_eye_of_elune : public ScriptedAI
{
    npc_echo_of_tyrande_eye_of_elune(Creature* creature) : ScriptedAI(creature), _angle(0.0f) { }

    void IsSummonedBy(Unit* summoner) override
    {
        _summoner = summoner ? summoner->GetGUID() : ObjectGuid::Empty;
        bool second = me->GetEntry() == NPC_EYE_OF_ELUNE_2 || me->GetEntry() == NPC_EYE_OF_ELUNE_4 || me->GetEntry() == NPC_EYE_OF_ELUNE_6;
        _angle = second ? float(M_PI) : 0.0f;
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        me->SetCanFly(true);
        me->SetDisableGravity(true);
        _events.ScheduleEvent(EVENT_EYE_ORBIT, 100ms);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        if (_events.ExecuteEvent() != EVENT_EYE_ORBIT)
            return;

        if (Unit* tyrande = ObjectAccessor::GetUnit(*me, _summoner))
        {
            _angle = Position::NormalizeOrientation(_angle + 0.35f);
            float radius = 25.0f;
            Position dest(tyrande->GetPositionX() + std::cos(_angle) * radius, tyrande->GetPositionY() + std::sin(_angle) * radius, tyrande->GetPositionZ() + 1.0f);
            me->GetMotionMaster()->MovePoint(0, dest, false);
        }
        _events.ScheduleEvent(EVENT_EYE_ORBIT, 1s);
    }

private:
    EventMap _events;
    ObjectGuid _summoner;
    float _angle;
};

// Hechizos de área con objetivo "entrada" (TARGET_UNIT_SRC_AREA_ENTRY) que en realidad golpean a jugadores en un radio.
class spell_echo_of_tyrande_players_in_radius : public SpellScript
{
public:
    spell_echo_of_tyrande_players_in_radius(float radius) : _radius(radius) { }

private:
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Unit* caster = GetCaster();
        targets.remove_if([&](WorldObject* target)
        {
            return !target->IsPlayer() || target->GetExactDist2d(caster) > _radius;
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_echo_of_tyrande_players_in_radius::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENTRY);
    }

    float _radius;
};

// 102242 - Tears of Elune: cada lágrima cae sobre un jugador al azar (102243 -> 102244).
class spell_echo_of_tyrande_tears_of_elune : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targets.remove_if([](WorldObject* target) { return !target->IsPlayer(); });
        if (targets.size() > 1)
            Trinity::Containers::RandomResize(targets, 1);
    }

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
            GetCaster()->CastSpell(target, SPELL_TEARS_OF_ELUNE_MISSILE, true);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_echo_of_tyrande_tears_of_elune::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        OnEffectHitTarget.Register(&spell_echo_of_tyrande_tears_of_elune::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};
}

namespace EndTime::EchoOfTyrande
{
// Logro Guardia lunar (5995): cualquier daño al sanador durante el guantelete de Tyrande lo invalida.
class end_time_moon_guard_tracker : public UnitScript
{
public:
    end_time_moon_guard_tracker() : UnitScript("end_time_moon_guard_tracker") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!damage || !victim || !victim->IsPlayer() || victim->GetMapId() != 938 || attacker == victim)
            return;

        InstanceScript* instance = victim->GetInstanceScript();
        if (!instance || instance->GetBossState(DATA_ECHO_OF_TYRANDE) != NOT_STARTED)
            return;

        if (victim->GetAreaId() != AREA_EMERALD_DRAGONSHRINE || (instance->GetData(DATA_TYRANDE_GAUNTLET) & 0x1F) == 0x1F)
            return;

        if (IsHealer(victim->ToPlayer()))
            instance->SetData(DATA_HEALER_HIT_IN_GAUNTLET, 1);
    }
};
}

void AddSC_boss_echo_of_tyrande()
{
    using namespace EndTime;
    using namespace EndTime::EchoOfTyrande;
    RegisterEndTimeCreatureAI(boss_echo_of_tyrande);
    RegisterEndTimeCreatureAI(npc_echo_of_tyrande_moonlance);
    RegisterEndTimeCreatureAI(npc_echo_of_tyrande_eye_of_elune);
    RegisterSpellScriptWithArgs(spell_echo_of_tyrande_players_in_radius, "spell_echo_of_tyrande_moonlance_damage", 2.0f);
    RegisterSpellScriptWithArgs(spell_echo_of_tyrande_players_in_radius, "spell_echo_of_tyrande_piercing_gaze", 6.0f);
    RegisterSpellScriptWithArgs(spell_echo_of_tyrande_players_in_radius, "spell_echo_of_tyrande_stardust", 100.0f);
    RegisterSpellScript(spell_echo_of_tyrande_tears_of_elune);
    new end_time_moon_guard_tracker();
}
