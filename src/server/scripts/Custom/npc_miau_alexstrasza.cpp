#include "ScriptMgr.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Trainer.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuestDef.h"
#include "SpellMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"
#include <array>

namespace
{
    enum MiauAlexstrasza
    {
        NPC_TEXT_WELCOME = 190000,
        NPC_TEXT_LOCKED = 190003,
        NPC_TEXT_QUARTERMASTER = 190010,
        NPC_TEXT_QUARTERMASTER_COMPLETE = 190011,
        NPC_TEXT_QUARTERMASTER_INELIGIBLE = 190012,
        ACTION_TRAVEL = 100,
        ACTION_SUPPLIES = 101,
        ACTION_OPEN_TRAVEL = 102,
        ACTION_CANCEL_TRAVEL = 103,
        MENU_ALEXSTRASZA = 190000,
        MENU_CONFIRM_TRAVEL = 190001,
        MENU_ALEXSTRASZA_STORY = 190002,
        MENU_PIPA = 190010,
        MENU_PIPA_KIT = 190011,
        MENU_PIPA_HONOR = 190012,
        NPC_TEXT_TRAVEL = 190013,
        NPC_TEXT_ALEXSTRASZA_STORY = 190014,
        NPC_TEXT_PIPA_KIT = 190015,
        NPC_TEXT_PIPA_HONOR = 190016,
        ACTION_ALEXSTRASZA_STORY = 104,
        ACTION_PIPA_KIT = 105,
        ACTION_PIPA_HONOR = 106,
        ACTION_BACK = 107,
        SPELL_LEVEL_UP_VISUAL = 47292,
        SPELL_TELEPORT_VISUAL = 52096,
        SPELL_RIGOR_MORTIS = 73523,
        BAG_ITEM = 10050,
        TARGET_LEVEL = 85,
        GOLD_REWARD = 10000 * 10000
    };

    struct Capital
    {
        char const* Name;
        uint32 Map;
        uint32 Zone;
        float X, Y, Z, O;
    };

    Capital const Stormwind{ "Ventormenta", 0, 1519, -8833.07f, 622.778f, 93.9317f, 0.67710f };
    Capital const Orgrimmar{ "Orgrimmar", 1, 1637, 1569.97f, -4397.41f, 16.0472f, 0.54303f };

    Capital const& GetCapital(Player const* player)
    {
        return player->GetTeam() == ALLIANCE ? Stormwind : Orgrimmar;
    }

    struct Adventure
    {
        uint32 Character = 0;
        bool Boosted = false;
        uint64 SupplyMask = 0;
        bool Completed = false;
    };

    uint32 AccountId(Player const* player) { return player->GetSession()->GetAccountId(); }
    uint32 CharacterId(Player const* player) { return uint32(player->GetGUID().GetCounter()); }

    Adventure ReadAdventure(Player const* player)
    {
        if (QueryResult result = CharacterDatabase.PQuery(
            "SELECT character_guid, boosted, supply_mask, completed FROM miau_adventure WHERE account_id = %u", AccountId(player)))
        {
            Field* row = result->Fetch();
            return { row[0].GetUInt32(), row[1].GetBool(), row[2].GetUInt64(), row[3].GetBool() };
        }
        return {};
    }

    bool SaveAdventure(Player* player, uint64 supplies, bool complete)
    {
        CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
        player->SaveToDB(transaction);
        transaction->PAppend("UPDATE miau_adventure SET boosted = 1, supply_mask = %llu, completed = %u WHERE account_id = %u AND character_guid = %u",
            static_cast<unsigned long long>(supplies), uint32(complete), AccountId(player), CharacterId(player));
        // Player::SaveToDB appends statements prepared on asynchronous connections.
        return CharacterDatabase.AsyncCommitTransaction(transaction).m_future.get();
    }

    // Hechizos que normalmente se obtienen con las misiones de las zonas de inicio (DBC 15595)
    std::array<uint32, 5> const DeathKnightQuestSpells = { 53428, 50977, 33388, 33391, 48778 }; // Forja de runas, Portón de la muerte, equitación, destrero
    std::array<uint32, 6> const WorgenQuestSpells = { 68975, 68976, 68978, 68992, 68996, 94293 }; // Saña, Aberración, Desollador, Vuelo oscuro, Dos formas, forma alterada
    std::array<uint32, 1> const GoblinQuestSpells = { 69046 };                                    // Duende de carga
    std::array<uint32, 6> const RidingSpells = { 33388, 33391, 34090, 34091, 90267, 54197 };        // Aprendiz..Artesano, licencia de vuelo, vuelo en clima frío

    // QuestSortID / zona de las cadenas de inicio que se dan por completadas
    constexpr int32 SORT_DEATH_KNIGHT = -372;
    constexpr int32 ZONE_GILNEAS = 4714;
    constexpr int32 ZONE_GILNEAS_CITY = 4755;
    constexpr int32 ZONE_KEZAN = 4737;
    constexpr int32 ZONE_LOST_ISLES = 4720;

    template<std::size_t N>
    void LearnAll(Player* player, std::array<uint32, N> const& spells)
    {
        for (uint32 spellId : spells)
            if (!player->HasSpell(spellId) && sSpellMgr->GetSpellInfo(spellId))
                player->LearnSpell(spellId, false);
    }

    // Da por completada una cadena de inicio: la quita del registro y la marca como recompensada
    uint32 CompleteStartingChain(Player* player, std::initializer_list<int32> zonesOrSorts)
    {
        uint32 completed = 0;
        for (auto const& [questId, quest] : sObjectMgr->GetQuestTemplates())
        {
            if (std::find(zonesOrSorts.begin(), zonesOrSorts.end(), quest.GetZoneOrSort()) == zonesOrSorts.end())
                continue;
            if (!player->SatisfyQuestRace(&quest, false) || !player->SatisfyQuestClass(&quest, false))
                continue;
            if (player->GetQuestRewardStatus(questId))
                continue;

            uint16 slot = player->FindQuestSlot(questId);
            if (slot < MAX_QUEST_LOG_SIZE)
                player->SetQuestSlot(slot, 0);
            player->RemoveActiveQuest(questId, false);
            player->SetRewardedQuest(questId);
            ++completed;
        }
        return completed;
    }

    void PrepareStartingZoneCharacter(Player* player)
    {
        uint32 completed = 0;
        if (player->getClass() == CLASS_DEATH_KNIGHT)
        {
            LearnAll(player, DeathKnightQuestSpells);
            completed += CompleteStartingChain(player, { SORT_DEATH_KNIGHT });
        }

        if (player->getRace() == RACE_WORGEN)
        {
            if (player->getClass() != CLASS_DEATH_KNIGHT)
                LearnAll(player, WorgenQuestSpells);
            completed += CompleteStartingChain(player, { ZONE_GILNEAS, ZONE_GILNEAS_CITY });
        }

        if (player->getRace() == RACE_GOBLIN)
        {
            if (player->getClass() != CLASS_DEATH_KNIGHT)
                LearnAll(player, GoblinQuestSpells);
            completed += CompleteStartingChain(player, { ZONE_KEZAN, ZONE_LOST_ISLES });
        }

        if (completed)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffff8000Alexstrasza|r ha completado tu historia de inicio (%u misiones) y te ha enseñado sus facultades.", completed);
            PhasingHandler::OnConditionChange(player);
        }
    }

    void TeleportToCapital(Player* player)
    {
        Capital const& capital = GetCapital(player);
        player->ExitVehicle();
        player->RemoveAurasByType(SPELL_AURA_MOUNTED);
        player->CombatStop(true);
        if (!player->TeleportTo(capital.Map, capital.X, capital.Y, capital.Z, capital.O))
            ChatHandler(player->GetSession()).PSendSysMessage("El portal a %s no se ha abierto. Puedes volver a intentarlo con este personaje.", capital.Name);
    }

    using Armor = std::array<uint32, 8>;
    Armor const Plate{ 75126, 75119, 75135, 75124, 75122, 75123, 75136, 75120 };
    Armor const MailAgility{ 75102, 75116, 75115, 75114, 75081, 75100, 75108, 75110 };
    Armor const MailIntellect{ 75090, 75061, 75084, 75094, 75085, 75083, 75097, 75092 };
    Armor const Leather{ 75105, 75113, 75103, 75131, 75104, 75127, 75112, 75130 };
    Armor const Cloth{ 75062, 75091, 75088, 75089, 75063, 75086, 75082, 75087 };
    using Accessories = std::array<uint32, 5>;
    Accessories const Strength{ 75077, 75078, 75071, 56415, 56458 };
    Accessories const Agility{ 75076, 75074, 75067, 56445, 56440 };
    Accessories const Intellect{ 75065, 75075, 75068, 56333, 56320 };

    struct Kit
    {
        Armor const& ArmorItems;
        Accessories const& AccessoryItems;
        uint32 MainHand;
        uint32 OffHand;
        uint32 Ranged;
        uint32 Trainer;
    };

    Kit const* GetKit(uint8 playerClass)
    {
        static Kit const warrior{ Plate, Strength, 56342, 0, 56376, 16 };
        static Kit const paladin{ Plate, Strength, 56342, 0, 75069, 164 };
        static Kit const hunter{ MailAgility, Agility, 56382, 0, 56317, 40 };
        static Kit const rogue{ Leather, Agility, 56326, 56335, 56420, 33 };
        static Kit const priest{ Cloth, Intellect, 56424, 0, 56446, 3 };
        static Kit const deathKnight{ Plate, Strength, 56342, 0, 75069, 55 };
        static Kit const shaman{ MailIntellect, Intellect, 55065, 56402, 75066, 124 };
        static Kit const mage{ Cloth, Intellect, 56271, 0, 56354, 135 };
        static Kit const warlock{ Cloth, Intellect, 56271, 0, 56354, 154 };
        static Kit const druid{ Leather, Agility, 56325, 0, 75079, 39 };
        switch (playerClass)
        {
            case CLASS_WARRIOR: return &warrior;
            case CLASS_PALADIN: return &paladin;
            case CLASS_HUNTER: return &hunter;
            case CLASS_ROGUE: return &rogue;
            case CLASS_PRIEST: return &priest;
            case CLASS_DEATH_KNIGHT: return &deathKnight;
            case CLASS_SHAMAN: return &shaman;
            case CLASS_MAGE: return &mage;
            case CLASS_WARLOCK: return &warlock;
            case CLASS_DRUID: return &druid;
            default: return nullptr;
        }
    }

    struct Supply
    {
        uint32 Item;
        uint8 Slot;
    };

    // Keep this order stable. The character database stores one bit per delivered entry.
    std::array<Supply, 21> MakeSupplies(Player const* player, Kit const& kit)
    {
        Armor const& a = kit.ArmorItems;
        Accessories const& x = kit.AccessoryItems;
        return {{
            { BAG_ITEM, INVENTORY_SLOT_BAG_START }, { BAG_ITEM, INVENTORY_SLOT_BAG_START + 1 },
            { BAG_ITEM, INVENTORY_SLOT_BAG_START + 2 }, { BAG_ITEM, INVENTORY_SLOT_BAG_START + 3 },
            { a[0], EQUIPMENT_SLOT_HEAD }, { a[1], EQUIPMENT_SLOT_SHOULDERS }, { a[2], EQUIPMENT_SLOT_CHEST },
            { a[3], EQUIPMENT_SLOT_WRISTS }, { a[4], EQUIPMENT_SLOT_HANDS }, { a[5], EQUIPMENT_SLOT_WAIST },
            { a[6], EQUIPMENT_SLOT_LEGS }, { a[7], EQUIPMENT_SLOT_FEET },
            { x[0], EQUIPMENT_SLOT_BACK }, { x[1], EQUIPMENT_SLOT_NECK }, { x[2], EQUIPMENT_SLOT_FINGER1 },
            { x[3], EQUIPMENT_SLOT_FINGER2 }, { x[4], EQUIPMENT_SLOT_TRINKET1 },
            { player->GetTeam() == ALLIANCE ? 42123u : 42122u, EQUIPMENT_SLOT_TRINKET2 },
            { kit.MainHand, EQUIPMENT_SLOT_MAINHAND }, { kit.OffHand, EQUIPMENT_SLOT_OFFHAND },
            { kit.Ranged, EQUIPMENT_SLOT_RANGED }
        }};
    }

    bool DeliverItem(Player* player, Supply const& supply)
    {
        if (!supply.Item)
            return true;

        Item* equipped = player->GetItemByPos(INVENTORY_SLOT_BAG_0, supply.Slot);
        uint16 destination;
        if (!equipped &&
            player->CanEquipNewItem(supply.Slot, destination, supply.Item, false) == EQUIP_ERR_OK)
        {
            if (Item* item = player->EquipNewItem(destination, supply.Item, true))
            {
                player->SendNewItem(item, 1, true, false);
                return true;
            }
        }

        ItemPosCountVec storage;
        if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, storage, supply.Item, 1) != EQUIP_ERR_OK)
            return false;

        if (Item* item = player->StoreNewItem(storage, supply.Item, true))
        {
            if (supply.Item == BAG_ITEM && equipped && item->GetBagSlot() != supply.Slot
                && equipped->GetTemplate()->GetContainerSlots() < item->GetTemplate()->GetContainerSlots())
                player->SwapItem(item->GetPos(), equipped->GetPos());

            player->SendNewItem(item, 1, true, false);
            return true;
        }
        return false;
    }
}

struct npc_miau_alexstrasza : public ScriptedAI
{
    npc_miau_alexstrasza(Creature* creature) : ScriptedAI(creature) { }

    bool GossipHello(Player* player) override
    {
        player->RemoveAurasDueToSpell(SPELL_RIGOR_MORTIS);
        ClearGossipMenuFor(player);
        InitGossipMenuFor(player, MENU_ALEXSTRASZA);
        Adventure state = ReadAdventure(player);
        if (state.Completed || (state.Character && state.Character != CharacterId(player)))
        {
            SendGossipMenuFor(player, NPC_TEXT_LOCKED, me->GetGUID());
            return true;
        }

        AddGossipItemFor(player, GOSSIP_ICON_TAXI, std::string("Quiero viajar a ") + GetCapital(player).Name + ".", GOSSIP_SENDER_MAIN, ACTION_OPEN_TRAVEL);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "¿Por qué me has llamado, Protectora de la Vida?", GOSSIP_SENDER_MAIN, ACTION_ALEXSTRASZA_STORY);
        SendGossipMenuFor(player, NPC_TEXT_WELCOME, me->GetGUID());
        return true;
    }

    bool GossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != player->PlayerTalkClass->GetGossipMenu().GetMenuId())
            return true;

        uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
        if (menuId == MENU_ALEXSTRASZA_STORY && action == ACTION_BACK)
            return GossipHello(player);

        if (menuId == MENU_ALEXSTRASZA && action == ACTION_ALEXSTRASZA_STORY)
        {
            ClearGossipMenuFor(player);
            InitGossipMenuFor(player, MENU_ALEXSTRASZA_STORY);
            me->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "¿Y adónde me llevará tu portal?", GOSSIP_SENDER_MAIN, ACTION_OPEN_TRAVEL);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Volver.", GOSSIP_SENDER_MAIN, ACTION_BACK);
            SendGossipMenuFor(player, NPC_TEXT_ALEXSTRASZA_STORY, me->GetGUID());
            return true;
        }

        if ((menuId == MENU_ALEXSTRASZA || menuId == MENU_ALEXSTRASZA_STORY) && action == ACTION_OPEN_TRAVEL)
        {
            Adventure state = ReadAdventure(player);
            if (state.Completed || (state.Character && state.Character != CharacterId(player)))
                return GossipHello(player);

            ClearGossipMenuFor(player);
            InitGossipMenuFor(player, MENU_CONFIRM_TRAVEL);
            std::string accept = std::string("Confirmar viaje a ") + GetCapital(player).Name;
            if (!state.Boosted && player->getLevel() < TARGET_LEVEL)
                accept += " (nivel 85 y 10.000 monedas de oro)";
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, accept, GOSSIP_SENDER_MAIN, ACTION_TRAVEL);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Cancelar y volver.", GOSSIP_SENDER_MAIN, ACTION_CANCEL_TRAVEL);
            me->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
            SendGossipMenuFor(player, NPC_TEXT_TRAVEL, me->GetGUID());
            return true;
        }

        if (menuId == MENU_CONFIRM_TRAVEL && action == ACTION_CANCEL_TRAVEL)
            return GossipHello(player);

        CloseGossipMenuFor(player);
        if (menuId != MENU_CONFIRM_TRAVEL || action != ACTION_TRAVEL || !player->IsAlive() || player->IsBeingTeleported())
            return true;

        CharacterDatabase.DirectPExecute("INSERT IGNORE INTO miau_adventure (account_id, character_guid) VALUES (%u, %u)",
            AccountId(player), CharacterId(player));
        Adventure state = ReadAdventure(player);
        if (state.Character != CharacterId(player) || state.Completed)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("Este viaje no está disponible para este personaje.");
            return true;
        }

        if (!state.Boosted)
        {
            if (player->getLevel() < TARGET_LEVEL)
            {
                player->GiveLevel(TARGET_LEVEL);
                player->InitTalentForLevel();
                player->SetUInt32Value(PLAYER_XP, 0);
                player->ModifyMoney(GOLD_REWARD);
                player->CastSpell(player, SPELL_LEVEL_UP_VISUAL, true);
            }
            PrepareStartingZoneCharacter(player);
            Capital const& capital = GetCapital(player);
            player->SetHomebind(WorldLocation(capital.Map, capital.X, capital.Y, capital.Z, capital.O), capital.Zone);
            if (!SaveAdventure(player, 0, false))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("La bendición no se pudo guardar. El portal sigue cerrado.");
                return true;
            }
        }

        me->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST_OMNI);
        me->Say("Que mi llama te guíe, campeón. Al otro lado te espera Pipa. Si te pide que firmes con sangre, dile que la tinta sirve.", LANG_UNIVERSAL, player);
        player->CastSpell(player, SPELL_TELEPORT_VISUAL, true);
        TeleportToCapital(player);
        return true;
    }
};

struct npc_miau_pipa_bolsilleno : public ScriptedAI
{
    npc_miau_pipa_bolsilleno(Creature* creature) : ScriptedAI(creature) { }

    bool GossipHello(Player* player) override
    {
        Adventure state = ReadAdventure(player);
        ClearGossipMenuFor(player);
        InitGossipMenuFor(player, MENU_PIPA);
        uint32 text = NPC_TEXT_QUARTERMASTER_INELIGIBLE;
        if (state.Character == CharacterId(player) && state.Boosted && player->getLevel() >= TARGET_LEVEL)
        {
            text = state.Completed ? NPC_TEXT_QUARTERMASTER_COMPLETE : NPC_TEXT_QUARTERMASTER;
            if (!state.Completed)
            {
                me->SetFacingToObject(player);
                me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE_NO_SHEATH);
                AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Alexstrasza me envía. ¿Qué hay en el cargamento?", GOSSIP_SENDER_MAIN, ACTION_PIPA_KIT);
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "¿Cómo empiezo a ganar honor?", GOSSIP_SENDER_MAIN, ACTION_PIPA_HONOR);
        }
        SendGossipMenuFor(player, text, me->GetGUID());
        return true;
    }

    bool GossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != player->PlayerTalkClass->GetGossipMenu().GetMenuId())
            return true;

        uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
        if ((menuId == MENU_PIPA_KIT || menuId == MENU_PIPA_HONOR) && action == ACTION_BACK)
            return GossipHello(player);

        if (menuId == MENU_PIPA && (action == ACTION_PIPA_KIT || action == ACTION_PIPA_HONOR))
        {
            bool const reviewKit = action == ACTION_PIPA_KIT;
            ClearGossipMenuFor(player);
            InitGossipMenuFor(player, reviewKit ? MENU_PIPA_KIT : MENU_PIPA_HONOR);
            me->HandleEmoteCommand(reviewKit ? EMOTE_ONESHOT_LAUGH : EMOTE_ONESHOT_POINT);
            if (reviewKit)
                AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Va por Azeroth. ¡Abre el cargamento!", GOSSIP_SENDER_MAIN, ACTION_SUPPLIES);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Volver.", GOSSIP_SENDER_MAIN, ACTION_BACK);
            SendGossipMenuFor(player, reviewKit ? NPC_TEXT_PIPA_KIT : NPC_TEXT_PIPA_HONOR, me->GetGUID());
            return true;
        }

        CloseGossipMenuFor(player);
        if (menuId != MENU_PIPA_KIT || action != ACTION_SUPPLIES || !player->IsAlive() || player->IsBeingTeleported())
            return true;

        Adventure state = ReadAdventure(player);
        if (state.Character != CharacterId(player) || !state.Boosted || state.Completed || player->getLevel() < TARGET_LEVEL)
            return true;

        Kit const* kit = GetKit(player->getClass());
        Trainer::Trainer const* trainer = kit ? sObjectMgr->GetTrainer(kit->Trainer) : nullptr;
        if (!trainer || trainer->GetTrainerType() != Trainer::Type::Class || !trainer->IsTrainerValidForPlayer(player))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("Falta el instructor de tu clase. El equipo sigue reservado.");
            return true;
        }

        LearnAll(player, RidingSpells);
        LearnAll(player, player->GetTeam() == ALLIANCE ? std::array<uint32, 2>{ 23227, 32235 } : std::array<uint32, 2>{ 23250, 32243 });
        trainer->TeachAvailableClassSpells(player);
        if (player->getClass() == CLASS_MAGE)
            if (Trainer::Trainer const* portals = sObjectMgr->GetTrainer(player->GetTeam() == ALLIANCE ? 114 : 12))
                portals->TeachAvailableClassSpells(player);

        auto supplies = MakeSupplies(player, *kit);
        uint64 mask = state.SupplyMask;
        for (std::size_t i = 0; i < supplies.size(); ++i)
            if (!(mask & (uint64(1) << i)) && DeliverItem(player, supplies[i]))
                mask |= uint64(1) << i;

        bool const complete = mask == (uint64(1) << supplies.size()) - 1;
        if (!SaveAdventure(player, mask, complete))
        {
            player->GetSession()->SendNotification("No se pudo guardar la entrega del equipo.");
            return true;
        }
        if (!complete)
        {
            me->HandleEmoteCommand(EMOTE_ONESHOT_TALK_NO_SHEATHE);
            me->Say("¡Alto! El reglamento prohíbe guardar un yelmo dentro de otro yelmo. Haz hueco y vuelve por lo que falta.", LANG_UNIVERSAL, player);
            return true;
        }

        me->HandleEmoteCommand(EMOTE_ONESHOT_CHEER_NO_SHEATHE);
        me->SendPlaySpellVisualKit(179, 0, 0);
        me->Say("¡Listo! Cuatro bolsas, dos monturas y ni un solo recibo. Ve al campo de batalla y trae honor. Los recibos los he perdido yo.", LANG_UNIVERSAL, player);
        player->GetSession()->SendNotification("Equipo de iniciación entregado. ¡Nos vemos en el campo de batalla!");
        return true;
    }
};

void AddSC_npc_miau_alexstrasza()
{
    RegisterCreatureAI(npc_miau_alexstrasza);
    RegisterCreatureAI(npc_miau_pipa_bolsilleno);
}
