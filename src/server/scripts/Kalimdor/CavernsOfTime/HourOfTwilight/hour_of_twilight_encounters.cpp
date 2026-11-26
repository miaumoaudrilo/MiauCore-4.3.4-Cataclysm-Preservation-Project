/*
 * Hour of Twilight encounter IDs verified against the Cataclysm 15595 client.
 * Licensed under the GNU General Public License, version 2 or later.
 */

#include "Creature.h"
#include "DBCStores.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "hour_of_twilight.h"
#include <array>

namespace HourOfTwilight
{
using namespace std::chrono_literals;

enum Spells
{
    SPELL_HAND_OF_FROST = 102593,
    SPELL_CHAINS_OF_FROST = 102582,
    SPELL_ICY_TOMB = 103252,
    SPELL_ICY_TOMB_STUN = 103251,
    SPELL_ICY_TOMB_SUMMON_AURA = 103250,
    SPELL_ICY_BOULDER = 102198,
    SPELL_TORRENT_OF_FROST = 104050,
    SPELL_MARK_OF_SILENCE = 102726,
    SPELL_THROW_KNIFE = 103597,
    SPELL_KNIFE_SILENCE = 103587,
    SPELL_CHOKING_SMOKE = 103558,
    SPELL_SMOKE_DAMAGE = 103790,
    SPELL_BLADE_BARRIER = 103419,
    SPELL_LESSER_BLADE_BARRIER = 103562,
    SPELL_LAVA_BURST = 103923,
    SPELL_RISING_FIRE_TOTEM = 103819,
    SPELL_HEALING_WAVE = 103641,
    SPELL_RIGHTEOUS_SHEAR = 103151,
    SPELL_PURIFYING_BLAST = 103651,
    SPELL_PURIFYING_ORB = 103578,
    SPELL_WAVE_OF_VIRTUE = 103678,
    SPELL_WATER_SHELL = 103686,
    SPELL_CLEANSE_SPIRIT = 103550,
    SPELL_TWILIGHT_EPIPHANY = 103754,
    SPELL_TWILIGHT_TRANSFORM = 103765,
    SPELL_TWILIGHT_SHEAR = 103363,
    SPELL_TWILIGHT_ORB = 103769,
    SPELL_TWILIGHT_ORB_IMPACT = 103777,
    SPELL_WAVE_OF_TWILIGHT = 103780,
    SPELL_SEEPING_LIGHT = 104528,
    SPELL_SEEPING_TWILIGHT = 104537,
    SPELL_THRALL_LAVA_BURST_BENEDICTUS = 108442,
    SPELL_SMITE = 104503,
    SPELL_TWILIGHT_BLAST = 104504
};

enum EncounterCreatures
{
    NPC_FROZEN_SERVITOR = 54555,
    NPC_SERVITOR_POSITION = 54598,
    NPC_TWILIGHT_SPARK = 55466,
    NPC_PURIFYING_LIGHT = 55377,
    NPC_CORRUPTING_TWILIGHT = 55467,
    NPC_WAVE_OF_VIRTUE = 55441,
    NPC_WAVE_OF_TWILIGHT = 55469
};

void FreeThrall(Creature* thrall)
{
    thrall->RemoveAurasDueToSpell(SPELL_ICY_TOMB);
    thrall->RemoveAurasDueToSpell(SPELL_ICY_TOMB_STUN);
    thrall->RemoveAurasDueToSpell(SPELL_ICY_TOMB_SUMMON_AURA);
}

enum CombatEvents
{
    EVENT_PRIMARY = 1,
    EVENT_SECONDARY,
    EVENT_THIRD,
    EVENT_FOURTH,
    EVENT_THRALL_SUPPORT,
    EVENT_PHASE_CHANGE,
    EVENT_PLATFORM_BOUNDARY
};

struct HourOfTwilightBoss : BossAI
{
    HourOfTwilightBoss(Creature* creature, uint32 boss, uint32 ready) : BossAI(creature, boss), _ready(ready) { }

    void Reset() override
    {
        _Reset();
        RefreshAttackable();
    }

    void DoAction(int32 action) override
    {
        if (action == ACTION_UNLOCK_ENCOUNTER)
            RefreshAttackable();
    }

protected:
    void RefreshAttackable()
    {
        bool const ready = instance->GetData(_ready);
        uint32 const lockedFlags = UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE;
        if (ready)
            me->RemoveFlag(UNIT_FIELD_FLAGS, lockedFlags);
        else
            me->SetFlag(UNIT_FIELD_FLAGS, lockedFlags);
        me->SetReactState(ready ? REACT_AGGRESSIVE : REACT_PASSIVE);
    }

    void Engage(Unit* who)
    {
        BossAI::JustEngagedWith(who);
        if (me->IsInCombat())
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
    }

    void Complete()
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        _JustDied();
    }

    void EnterEvadeMode(EvadeReason reason) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        BossAI::EnterEvadeMode(reason);
    }

    Creature* LivingThrall(uint32 entry)
    {
        if (Creature* thrall = instance->GetCreature(entry))
            if (thrall->IsAlive())
                return thrall;
        return nullptr;
    }

private:
    uint32 const _ready;
};

struct boss_hot_arcurion : HourOfTwilightBoss
{
    boss_hot_arcurion(Creature* creature) : HourOfTwilightBoss(creature, DATA_ARCURION, DATA_ARCURION_READY) { }

    void Reset() override
    {
        HourOfTwilightBoss::Reset();
        _torrent = false;
        if (Creature* thrall = LivingThrall(DATA_THRALL_ARCURION))
            FreeThrall(thrall);
    }

    void JustEngagedWith(Unit* who) override
    {
        Engage(who);
        if (!me->IsInCombat())
            return;

        Talk(0);
        if (Creature* thrall = LivingThrall(DATA_THRALL_ARCURION))
            thrall->AI()->Talk(1);
        events.ScheduleEvent(EVENT_PRIMARY, 3s);
        events.ScheduleEvent(EVENT_SECONDARY, 10s);
        events.ScheduleEvent(EVENT_THIRD, 18s);
        events.ScheduleEvent(EVENT_FOURTH, 6s);
        events.ScheduleEvent(EVENT_THRALL_SUPPORT, 4s);
    }

    void JustDied(Unit*) override
    {
        Talk(5);
        if (Creature* thrall = LivingThrall(DATA_THRALL_ARCURION))
        {
            FreeThrall(thrall);
            thrall->AI()->Talk(3);
        }
        Complete();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (!_torrent && me->HealthBelowPct(30))
        {
            _torrent = true;
            Talk(3);
            events.CancelEvent(EVENT_PRIMARY);
            events.CancelEvent(EVENT_SECONDARY);
            events.CancelEvent(EVENT_THIRD);
            events.CancelEvent(EVENT_FOURTH);
            DoCastSelf(SPELL_TORRENT_OF_FROST);
            return;
        }

        switch (events.ExecuteEvent())
        {
            case EVENT_PRIMARY:
                DoCastVictim(SPELL_HAND_OF_FROST);
                events.Repeat(3s, 5s);
                break;
            case EVENT_SECONDARY:
                DoCastAOE(SPELL_CHAINS_OF_FROST);
                events.Repeat(11s, 14s);
                break;
            case EVENT_THIRD:
                if (Creature* thrall = LivingThrall(DATA_THRALL_ARCURION))
                    if (!thrall->HasAura(SPELL_ICY_TOMB))
                    {
                        Talk(4);
                        DoCast(thrall, SPELL_ICY_TOMB, true);
                    }
                events.Repeat(28s);
                break;
            case EVENT_FOURTH:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 90.0f, true))
                    if (Creature* perch = me->FindNearestCreature(NPC_SERVITOR_POSITION, 100.0f))
                        perch->CastSpell(target, SPELL_ICY_BOULDER, true);
                events.Repeat(5s, 7s);
                break;
            case EVENT_THRALL_SUPPORT:
                if (Creature* thrall = LivingThrall(DATA_THRALL_ARCURION))
                    if (!thrall->HasAura(SPELL_ICY_TOMB))
                        thrall->CastSpell(me, SPELL_LAVA_BURST, true);
                events.Repeat(7s);
                break;
        }
        DoMeleeAttackIfReady();
    }

private:
    bool _torrent = false;
};

struct npc_hot_icy_tomb : ScriptedAI
{
    npc_hot_icy_tomb(Creature* creature) : ScriptedAI(creature) { me->SetReactState(REACT_PASSIVE); }

    void JustDied(Unit*) override
    {
        if (InstanceScript* instance = me->GetInstanceScript())
            if (Creature* thrall = instance->GetCreature(DATA_THRALL_ARCURION))
                FreeThrall(thrall);
    }
};

struct boss_hot_asira : HourOfTwilightBoss
{
    boss_hot_asira(Creature* creature) : HourOfTwilightBoss(creature, DATA_ASIRA_DAWNSLAYER, DATA_ASIRA_READY) { }

    void Reset() override
    {
        HourOfTwilightBoss::Reset();
        _barrier = false;
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_MARK_OF_SILENCE);
    }

    void JustEngagedWith(Unit* who) override
    {
        Engage(who);
        if (!me->IsInCombat())
            return;
        Talk(2);
        DoCastAOE(SPELL_MARK_OF_SILENCE, true);
        events.ScheduleEvent(EVENT_PRIMARY, 11s);
        events.ScheduleEvent(EVENT_THRALL_SUPPORT, 5s);
    }

    void JustDied(Unit*) override
    {
        Talk(5);
        if (Creature* thrall = LivingThrall(DATA_THRALL_ASIRA))
            thrall->AI()->Talk(7);
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_MARK_OF_SILENCE);
        Complete();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (!_barrier && me->HealthBelowPct(30))
        {
            _barrier = true;
            DoCastSelf(SPELL_BLADE_BARRIER, true);
        }

        switch (events.ExecuteEvent())
        {
            case EVENT_PRIMARY:
                DoCastSelf(SPELL_CHOKING_SMOKE);
                events.Repeat(17s, 22s);
                break;
            case EVENT_THRALL_SUPPORT:
                if (Creature* thrall = LivingThrall(DATA_THRALL_ASIRA))
                {
                    thrall->CastSpell(thrall, SPELL_RISING_FIRE_TOTEM, true);
                    if (thrall->HealthBelowPct(60))
                        thrall->CastSpell(thrall, SPELL_HEALING_WAVE);
                    else
                        thrall->CastSpell(me, SPELL_LAVA_BURST);
                }
                events.Repeat(20s);
                break;
        }
        DoMeleeAttackIfReady();
    }

private:
    bool _barrier = false;
};

class spell_hot_blade_barrier : public AuraScript
{
    void SetAbsorbAmount(AuraEffect const*, int32& amount, bool&) { amount = -1; }

    void Absorb(AuraEffect* effect, DamageInfo& damage, uint32& absorbed)
    {
        uint32 const incoming = damage.GetDamage();
        absorbed = 0;
        if (!incoming)
            return;
        if (incoming < uint32(effect->GetBaseAmount()))
        {
            absorbed = incoming - 1;
            return;
        }

        Unit* owner = GetTarget();
        uint32 const id = GetId();
        GetAura()->Remove();
        if (id == SPELL_BLADE_BARRIER && owner->IsAlive())
            owner->CastSpell(owner, SPELL_LESSER_BLADE_BARRIER, true);
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_hot_blade_barrier::SetAbsorbAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb.Register(&spell_hot_blade_barrier::Absorb, EFFECT_0);
    }
};

class spell_hot_throw_knife : public SpellScript
{
    void SelectInterceptor(std::list<WorldObject*>& targets)
    {
        Unit* marked = GetExplTargetUnit();
        if (!marked)
            return;

        WorldObject* intercept = marked;
        for (WorldObject* target : targets)
            if (target->ToPlayer() && target->IsInBetween(GetCaster(), marked, 1.0f)
                && GetCaster()->GetDistance(target) < GetCaster()->GetDistance(intercept))
                intercept = target;
        targets.clear();
        targets.push_back(intercept);
    }

    void Silence(SpellEffIndex)
    {
        if (Unit* target = GetHitUnit())
            if (target == GetExplTargetUnit())
                GetCaster()->CastSpell(target, SPELL_KNIFE_SILENCE, true);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_hot_throw_knife::SelectInterceptor, EFFECT_1, TARGET_UNIT_CONE_ENEMY_24);
        OnEffectHitTarget.Register(&spell_hot_throw_knife::Silence, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

class spell_hot_choking_smoke : public AuraScript
{
    Position _center;

    void SaveCenter(AuraEffect const*, AuraEffectHandleModes)
    {
        _center = GetTarget()->GetPosition();
    }

    void Tick(AuraEffect const*)
    {
        GetTarget()->CastSpell(_center, SPELL_SMOKE_DAMAGE, true);
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_hot_choking_smoke::SaveCenter, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic.Register(&spell_hot_choking_smoke::Tick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
    }
};

class player_hot_mark_of_silence : public PlayerScript
{
public:
    player_hot_mark_of_silence() : PlayerScript("player_hot_mark_of_silence") { }

    void OnSpellCast(Player* player, Spell* spell, bool) override
    {
        if (player->GetMapId() != 940 || spell->IsTriggered() || spell->GetCastTime() <= 0)
            return;
        if (Aura* mark = player->GetAura(SPELL_MARK_OF_SILENCE))
            if (Unit* asira = mark->GetCaster())
                if (asira->GetEntry() == NPC_ASIRA && asira->IsAlive() && asira->IsInCombat())
                    asira->CastSpell(player, SPELL_THROW_KNIFE, true);
    }
};

struct boss_hot_benedictus : HourOfTwilightBoss
{
    boss_hot_benedictus(Creature* creature) : HourOfTwilightBoss(creature, DATA_ARCHBISHOP_BENEDICTUS, DATA_BENEDICTUS_READY) { }

    void Reset() override
    {
        _shadow = false;
        _changing = false;
        _sparksKilled = 0;
        _reverseWave = false;
        me->RemoveAurasDueToSpell(SPELL_TWILIGHT_TRANSFORM);
        me->SetFaction(instance->GetData(DATA_BENEDICTUS_READY) ? 14 : 35);
        HourOfTwilightBoss::Reset();
        RemoveEncounterAuras();
    }

    void DoAction(int32 action) override
    {
        if (action == ACTION_UNLOCK_ENCOUNTER)
            me->SetFaction(14);
        HourOfTwilightBoss::DoAction(action);
    }

    void JustEngagedWith(Unit* who) override
    {
        Engage(who);
        if (!me->IsInCombat())
            return;
        Talk(5);
        SchedulePhase();
        events.ScheduleEvent(EVENT_PLATFORM_BOUNDARY, 1s);
        events.ScheduleEvent(EVENT_THRALL_SUPPORT, 3s);
    }

    void JustDied(Unit*) override
    {
        Talk(12);
        RemoveEncounterAuras();
        if (_sparksKilled >= 10)
            if (AchievementEntry const* eclipse = sAchievementStore.LookupEntry(6132))
                for (auto const& reference : me->GetMap()->GetPlayers())
                    if (Player* player = reference.GetSource())
                        if (player->GetDistance(me) <= 100.0f)
                            player->CompletedAchievement(eclipse);
        Complete();
        if (Creature* thrall = LivingThrall(DATA_THRALL_BENEDICTUS))
            thrall->AI()->Talk(3);
    }

    void SummonedCreatureDies(Creature* summon, Unit* killer) override
    {
        if (summon->GetEntry() == NPC_TWILIGHT_SPARK && killer && killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            ++_sparksKilled;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (!_shadow && !_changing && me->HealthBelowPct(60))
        {
            _changing = true;
            Talk(8);
            events.Reset();
            DoCastSelf(SPELL_TWILIGHT_EPIPHANY);
            events.ScheduleEvent(EVENT_PHASE_CHANGE, 6s);
            return;
        }

        switch (events.ExecuteEvent())
        {
            case EVENT_PRIMARY:
                DoCastVictim(_shadow ? SPELL_TWILIGHT_BLAST : SPELL_SMITE);
                events.Repeat(6s, 8s);
                break;
            case EVENT_SECONDARY:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 80.0f, true))
                {
                    DoCast(target, _shadow ? SPELL_TWILIGHT_SHEAR : SPELL_RIGHTEOUS_SHEAR);
                    if (!_shadow)
                        if (Creature* thrall = LivingThrall(DATA_THRALL_BENEDICTUS))
                            thrall->CastSpell(target, SPELL_CLEANSE_SPIRIT, true);
                }
                events.Repeat(18s, 22s);
                break;
            case EVENT_THIRD:
                LaunchOrbs();
                events.Repeat(21s);
                break;
            case EVENT_FOURTH:
                LaunchWave();
                events.Repeat(32s);
                break;
            case EVENT_PHASE_CHANGE:
                _shadow = true;
                _changing = false;
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEEPING_LIGHT);
                Talk(9);
                DoCastSelf(SPELL_TWILIGHT_TRANSFORM, true);
                if (IsHeroic())
                    SpawnTwilightSparks();
                SchedulePhase();
                events.ScheduleEvent(EVENT_PLATFORM_BOUNDARY, 1s);
                break;
            case EVENT_PLATFORM_BOUNDARY:
                DoCastSelf(_shadow ? SPELL_SEEPING_TWILIGHT : SPELL_SEEPING_LIGHT, true);
                events.Repeat(1s);
                break;
            case EVENT_THRALL_SUPPORT:
                if (Creature* thrall = LivingThrall(DATA_THRALL_BENEDICTUS))
                    thrall->CastSpell(me, SPELL_THRALL_LAVA_BURST_BENEDICTUS, true);
                events.Repeat(3s);
                break;
        }
        DoMeleeAttackIfReady();
    }

private:
    void SchedulePhase()
    {
        events.ScheduleEvent(EVENT_PRIMARY, 3s);
        events.ScheduleEvent(EVENT_SECONDARY, 9s);
        events.ScheduleEvent(EVENT_THIRD, 14s);
        events.ScheduleEvent(EVENT_FOURTH, 24s);
    }

    void RemoveEncounterAuras()
    {
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_RIGHTEOUS_SHEAR);
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_SHEAR);
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEEPING_LIGHT);
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEEPING_TWILIGHT);
    }

    void SpawnTwilightSparks()
    {
        Position const perimeter[] =
        {
            { 3548.88f, 329.05f, -120.06f }, { 3559.38f, 310.17f, -112.84f },
            { 3526.16f, 307.20f, -112.54f }, { 3594.22f, 232.02f, -119.97f },
            { 3537.41f, 235.60f, -112.54f }, { 3504.15f, 269.36f, -115.23f },
            { 3513.35f, 251.13f, -112.54f }, { 3587.46f, 276.43f, -115.63f },
            { 3586.22f, 309.80f, -120.06f }, { 3503.89f, 315.76f, -120.06f },
            { 3558.39f, 218.52f, -119.97f }, { 3512.05f, 224.64f, -120.06f },
            { 3488.35f, 266.62f, -120.06f }, { 3617.41f, 245.96f, -101.92f }
        };
        for (Position const& position : perimeter)
            if (TempSummon* spark = me->SummonCreature(NPC_TWILIGHT_SPARK, position, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 2s))
            {
                spark->RemoveAurasDueToSpell(103766);
                spark->SetReactState(REACT_PASSIVE);
            }
    }

    void LaunchOrbs()
    {
        std::list<Unit*> targets;
        SelectTargetList(targets, 3, SELECT_TARGET_RANDOM, 0, 85.0f, true);
        if (!_shadow && LivingThrall(DATA_THRALL_BENEDICTUS) && targets.size() > 1)
            targets.pop_front();
        for (Unit* target : targets)
        {
            Position origin = me->GetPosition();
            origin.m_positionZ += 6.0f;
            if (TempSummon* orb = me->SummonCreature(_shadow ? NPC_CORRUPTING_TWILIGHT : NPC_PURIFYING_LIGHT, origin, TEMPSUMMON_TIMED_DESPAWN, 9s))
            {
                orb->SetReactState(REACT_PASSIVE);
                orb->SetCanFly(true);
                orb->SetDisableGravity(true);
                orb->CastSpell(orb, _shadow ? SPELL_TWILIGHT_ORB : SPELL_PURIFYING_ORB, true);
                orb->GetMotionMaster()->MovePoint(1, target->GetPosition(), false, 8.0f);
                orb->AI()->SetData(0, _shadow ? SPELL_TWILIGHT_ORB_IMPACT : SPELL_PURIFYING_BLAST);
            }
        }
    }

    void LaunchWave()
    {
        Talk(_shadow ? 10 : 6);
        if (!_shadow)
            if (Creature* thrall = LivingThrall(DATA_THRALL_BENEDICTUS))
                thrall->CastSpell(thrall, SPELL_WATER_SHELL, true);

        // The platform lies on the X axis. Alternate the two ends.
        bool const reverse = _reverseWave;
        _reverseWave = !_reverseWave;
        Position const begin = { reverse ? 3656.0f : 3547.0f, 273.0f, -115.9f, reverse ? 3.14159f : 0.0f };
        Position const end = { reverse ? 3547.0f : 3656.0f, 273.0f, -115.9f, begin.GetOrientation() };
        if (TempSummon* wave = me->SummonCreature(_shadow ? NPC_WAVE_OF_TWILIGHT : NPC_WAVE_OF_VIRTUE, begin, TEMPSUMMON_TIMED_DESPAWN, 20s))
        {
            wave->SetReactState(REACT_PASSIVE);
            wave->CastSpell(wave, _shadow ? SPELL_WAVE_OF_TWILIGHT : SPELL_WAVE_OF_VIRTUE, true);
            wave->GetMotionMaster()->MovePoint(1, end, false, 8.0f);
        }
    }

    bool _shadow = false;
    bool _changing = false;
    bool _reverseWave = false;
    uint8 _sparksKilled = 0;
};

class spell_hot_benedictus_platform_boundary : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        // The stock caster is at the center of the platform. Only the outer ring is hazardous.
        Position const center = { 3546.42f, 272.74f, -115.88f };
        targets.remove_if([&center](WorldObject const* target)
        {
            float const radius = target->GetExactDist2d(&center);
            return !target->ToPlayer() || radius < 36.0f || radius > 72.0f ||
                std::abs(target->GetPositionZ() - center.GetPositionZ()) > 10.0f;
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_hot_benedictus_platform_boundary::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnObjectAreaTargetSelect.Register(&spell_hot_benedictus_platform_boundary::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

struct npc_hot_benedictus_orb : ScriptedAI
{
    npc_hot_benedictus_orb(Creature* creature) : ScriptedAI(creature) { }

    void SetData(uint32, uint32 spell) override { _impact = spell; }

    void MovementInform(uint32 type, uint32 point) override
    {
        if (type == POINT_MOTION_TYPE && point == 1 && _impact)
        {
            me->CastSpell(me->GetPosition(), _impact, true);
            me->DespawnOrUnsummon(1s);
        }
    }

private:
    uint32 _impact = 0;
};
}

void AddSC_hour_of_twilight_encounters()
{
    using namespace HourOfTwilight;
    RegisterHourOfTwilightCreatureAI(boss_hot_arcurion);
    RegisterHourOfTwilightCreatureAI(boss_hot_asira);
    RegisterHourOfTwilightCreatureAI(boss_hot_benedictus);
    RegisterHourOfTwilightCreatureAI(npc_hot_icy_tomb);
    RegisterHourOfTwilightCreatureAI(npc_hot_benedictus_orb);
    RegisterSpellScript(spell_hot_blade_barrier);
    RegisterSpellScript(spell_hot_throw_knife);
    RegisterSpellScript(spell_hot_choking_smoke);
    RegisterSpellScript(spell_hot_benedictus_platform_boundary);
    new player_hot_mark_of_silence();
}
