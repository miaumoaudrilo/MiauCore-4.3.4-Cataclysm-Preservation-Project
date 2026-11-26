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
 * Eco de Baine (Fin de los Días, Santuario de Dragones Obsidiana) - 4.3.4
 * Datos: DBC 15595 + Diario de encuentros / Wowhead (Cataclysm):
 *  - Lanzar tótem (101615 -> 101614): daño + derribo al jugador e invoca el Tótem de Baine (54434) 20 s en su posición.
 *    El jugador puede recoger el tótem (clic: 107837 -> 101601) y devolvérselo (101603 -> 101602): Baine queda aturdido
 *    y recibe un 50% más de daño durante 6 s.
 *  - Pulverizar (101625 -> 101626 salto -> 101627): salta sobre un jugador, 30000 de daño en 15 m y destruye la plataforma (101815).
 *  - Hacha de arrabio (101834 en Baine): al tocar la lava el hacha se prende (101836, 10 s) y sus golpes cuerpo a cuerpo
 *    aplican Explosión de arrabio (101840, disipable).
 *  - Magma (101619): los jugadores en la lava reciben daño de Fuego que se acumula (101819).
 */

#include "end_time.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"

namespace EndTime::EchoOfBaine
{
enum Spells
{
    SPELL_BAINE_VISUALS         = 101624,
    SPELL_MOLTEN_AXE_PERIODIC   = 101834,
    SPELL_MOLTEN_AXE            = 101836,
    SPELL_THROW_TOTEM           = 101615,
    SPELL_PULVERIZE             = 101625,
    SPELL_PULVERIZE_JUMP        = 101626,
    SPELL_PULVERIZE_DAMAGE      = 101627,
    SPELL_PULVERIZE_DESTROY     = 101815,
    SPELL_MAGMA                 = 101619,
    SPELL_THROW_TOTEM_BACK_HIT  = 101602
};

enum Events
{
    EVENT_THROW_TOTEM = 1,
    EVENT_PULVERIZE,
    EVENT_CHECK_LAVA
};

enum Texts
{
    SAY_INTRO           = 0,
    SAY_AGGRO           = 1,
    SAY_SLAY            = 2,
    SAY_THROW_TOTEM     = 4,
    SAY_PULVERIZE       = 5,
    SAY_MOLTEN_AXE      = 6,
    SAY_TOTEM_RETURNED  = 7,
    SAY_DEATH           = 8
};

static std::array<uint32, 4> const PlatformEntries = { GO_BAINE_PLATFORM_1, GO_BAINE_PLATFORM_2, GO_BAINE_PLATFORM_3, GO_BAINE_PLATFORM_4 };

static bool IsInLava(Unit const* unit)
{
    LiquidData liquid;
    ZLiquidStatus status = unit->GetMap()->GetLiquidStatus(unit->GetPhaseShift(), unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), map_liquidHeaderTypeFlags::AllLiquids, &liquid);
    return (status & MAP_LIQUID_STATUS_SWIMMING) && liquid.type_flags.HasFlag(map_liquidHeaderTypeFlags::Magma);
}

struct boss_echo_of_baine : public BossAI
{
    boss_echo_of_baine(Creature* creature) : BossAI(creature, DATA_ECHO_OF_BAINE), _introDone(false) { }

    void Reset() override
    {
        _Reset();
        RebuildPlatforms();
        me->RemoveAurasDueToSpell(SPELL_MOLTEN_AXE);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!_introDone && who->IsPlayer() && me->IsWithinDistInMap(who, 60.0f) && !who->ToPlayer()->IsGameMaster())
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
        events.ScheduleEvent(EVENT_THROW_TOTEM, 10s);
        events.ScheduleEvent(EVENT_PULVERIZE, 30s);
        events.ScheduleEvent(EVENT_CHECK_LAVA, 1s);
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
        RemoveMagmaFromPlayers();
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        RemoveMagmaFromPlayers();
        summons.DespawnAll();
        BossAI::EnterEvadeMode(why);
    }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_THROW_TOTEM_BACK_HIT)
            Talk(SAY_TOTEM_RETURNED);
        else if (spellInfo->Id == SPELL_MOLTEN_AXE)
            Talk(SAY_MOLTEN_AXE);
    }

    // 101626 (salto) no lanza solo su efecto de aterrizaje: al llegar, 101627 (daño en área + destrucción de la plataforma).
    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == EFFECT_MOTION_TYPE && id == EVENT_JUMP && me->IsInCombat())
            DoCastSelf(SPELL_PULVERIZE_DAMAGE, true);
    }

    void RebuildPlatforms()
    {
        for (uint32 entry : PlatformEntries)
        {
            std::list<GameObject*> platforms;
            me->GetGameObjectListWithEntryInGrid(platforms, entry, 200.0f);
            for (GameObject* platform : platforms)
                platform->SetDestructibleState(GO_DESTRUCTIBLE_REBUILDING, nullptr, true);
        }
    }

    void RemoveMagmaFromPlayers()
    {
        for (MapReference const& ref : me->GetMap()->GetPlayers())
            if (Player* player = ref.GetSource())
                player->RemoveAurasDueToSpell(SPELL_MAGMA);
    }

    // Jugadores dentro de la lava del santuario: Magma (daño de Fuego que se acumula). Fuera de ella se retira.
    void UpdateMagma()
    {
        for (MapReference const& ref : me->GetMap()->GetPlayers())
        {
            Player* player = ref.GetSource();
            if (!player || !player->IsAlive() || player->IsGameMaster() || !me->IsWithinDistInMap(player, 120.0f))
                continue;

            bool inLava = IsInLava(player);
            if (inLava && !player->HasAura(SPELL_MAGMA))
                player->CastSpell(player, SPELL_MAGMA, true);
            else if (!inLava)
                player->RemoveAurasDueToSpell(SPELL_MAGMA);
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
                case EVENT_THROW_TOTEM:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                    {
                        Talk(SAY_THROW_TOTEM, target);
                        DoCast(target, SPELL_THROW_TOTEM);
                    }
                    else if (Unit* victim = me->GetVictim())
                    {
                        Talk(SAY_THROW_TOTEM, victim);
                        DoCast(victim, SPELL_THROW_TOTEM);
                    }
                    events.Repeat(25s);
                    break;
                case EVENT_PULVERIZE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                    {
                        Talk(SAY_PULVERIZE, target);
                        DoCast(target, SPELL_PULVERIZE_JUMP);
                    }
                    else if (Unit* victim = me->GetVictim())
                    {
                        Talk(SAY_PULVERIZE, victim);
                        DoCast(victim, SPELL_PULVERIZE_JUMP);
                    }
                    events.Repeat(40s);
                    break;
                case EVENT_CHECK_LAVA:
                    UpdateMagma();
                    events.Repeat(1s);
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
    bool _introDone;
};

// 101834 - Molten Axe (periódico en Baine): si está en la lava se prende el hacha (101836).
class spell_echo_of_baine_molten_axe_periodic : public AuraScript
{
    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();
        if (target->IsInCombat() && IsInLava(target) && !target->HasAura(SPELL_MOLTEN_AXE))
            target->CastSpell(target, SPELL_MOLTEN_AXE, true);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_echo_of_baine_molten_axe_periodic::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 101815 - Pulverize (destrucción): la plataforma más cercana al punto de impacto queda destruida.
class spell_echo_of_baine_pulverize_destroy : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targets.clear();
        Unit* caster = GetCaster();
        GameObject* nearest = nullptr;
        float best = 30.0f;
        for (uint32 entry : PlatformEntries)
            if (GameObject* platform = caster->FindNearestGameObject(entry, 30.0f))
            {
                float dist = caster->GetExactDist2d(platform);
                if (dist < best)
                {
                    best = dist;
                    nearest = platform;
                }
            }

        if (nearest)
            targets.push_back(nearest);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_echo_of_baine_pulverize_destroy::FilterTargets, EFFECT_0, TARGET_GAMEOBJECT_DEST_AREA);
    }
};

}

void AddSC_boss_echo_of_baine()
{
    using namespace EndTime;
    using namespace EndTime::EchoOfBaine;
    RegisterEndTimeCreatureAI(boss_echo_of_baine);
    RegisterSpellScript(spell_echo_of_baine_molten_axe_periodic);
    RegisterSpellScript(spell_echo_of_baine_pulverize_destroy);
}
