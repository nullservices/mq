/*
 * MacroQuest: The extension platform for EverQuest
 * Copyright (C) 2002-present MacroQuest Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include "Common.h"
#include "EQClasses.h"
#include "Containers.h"
#include "Requirements.h"

namespace eqlib {

//============================================================================

struct AltAbilityEffectData
{
	int effectType;
	int baseEffect1;
	int baseEffect2;
	int slot;
};

// @sizeof(CAltAbilityData) == 0xe0 :: 2024-07-22 (live) @ 0x1402087fd
constexpr size_t CAltAbilityData_size = 0xe0;

class [[offsetcomments]] CAltAbilityData
{
public:
	int GetMaxRank() const { return MaxRank; }

	EQLIB_OBJECT const char* GetNameString() const;
	EQLIB_OBJECT const char* GetDescriptionString() const;
	EQLIB_OBJECT const char* GetCategoryString() const;
	EQLIB_OBJECT const char* GetExpansionString() const;

	// These two together define the lines of text displayed in a hotbutton.
	EQLIB_OBJECT const char* GetShortName1() const;
	EQLIB_OBJECT const char* GetShortName2() const;

/*0x00*/ int                 Index;
/*0x04*/ bool                bShowInAbilityWindow;               // [0] = enabled flag? everything 1
/*0x08*/ int                 nShortName;
/*0x0c*/ int                 nShortName2;
/*0x10*/ int                 nName;
/*0x14*/ int                 nDesc;
/*0x18*/ int                 MinLevel;                           // LevelNeeded
/*0x1c*/ int                 Cost;                               // Initial Cost or cost the last time you bought a level of it
/*0x20*/ int                 GroupID;                            // ID of the AA group (/alt activate id)
/*0x24*/ int                 CurrentRank;                        // GroupLevel the current rank of this AA first rank is 1 etc
/*0x28*/ ArrayClass<int>     RequiredGroups;                     // array of required groups
/*0x40*/ ArrayClass<int>     RequiredGroupLevels;                // array of required ranks for those groups
/*0x58*/ int                 Type;                               // 1 General 2 Archetype 3 Class 4 special 5 focus
/*0x5c*/ int                 SpellID;                            // -1 for no Spell
/*0x60*/ bool                bRefund;
/*0x68*/ ArrayClass<int>     TimerIds;                           // list of timer ids
/*0x80*/ int                 ReuseTimer;                         // in seconds
/*0x84*/ int                 Classes;                            // Classes/2 is the actual value we want.
/*0x88*/ int                 MaxRank;                            // so like x/25, this is the 25
/*0x8c*/ int                 TotalPoints;
/*0x90*/ int                 PreviousGroupAbilityId;             // -1 if none, although sometimes it can be -1 when there is ...
/*0x94*/ int                 NextGroupAbilityId;                 // ID of the next rank
/*0x98*/ bool                QuestOnly;                          // vet AA, quest AA, item, etc
/*0x9c*/ int                 Charges;                            // charges on expendable AAs (limited use aa's)
/*0xa0*/ bool                bIgnoreDeLevel;                     // still usable after de-leveling?
/*0xa4*/ int                 Expansion;
/*0xa8*/ int                 DisplayCategory;                    // 7 is expendable, -1 none
/*0xac*/ bool                bShroud;                            // shrouds are also called "templates"
/*0xad*/ bool                bBetaOnlyAbility;
/*0xae*/ bool                bResetOnDeath;                      // true for lay on hands for example.
/*0xaf*/ bool                bAutogrant;                         // true if its an autogrant ability? or if auto grant is enabled? need to check cause this could be useful
/*0xb0*/ int                 AutoGrantAssociationId;             // Usually the same as normal expansion if enabled
/*0xb4*/ int                 Unknown0x98;
/*0xb8*/ int                 Unknown0x9C;
/*0xc0*/ EQList<AltAbilityEffectData> PersistentEffects;
/*0xe0*/

	ALT_MEMBER_GETTER(int, GroupID, ID);
	ALT_MEMBER_GETTER(int, nShortName2, nShorterName);
};
using ALTABILITY = CAltAbilityData;
using PALTABILITY = CAltAbilityData*;

SIZE_CHECK(CAltAbilityData, CAltAbilityData_size);

//============================================================================

class [[offsetcomments]] AltAdvManager
{
public:
	EQLIB_OBJECT CAltAbilityData* GetAAById(int index, int level = -1);
	EQLIB_OBJECT bool IsAbilityReady(PcClient*, CAltAbilityData*, int* Refresh/*out*/ = nullptr, int* Timer/*out*/ = nullptr);
	EQLIB_OBJECT uint32_t GetCalculatedTimer(PcClient*, CAltAbilityData*);
	EQLIB_OBJECT bool CanSeeAbility(PcClient*, CAltAbilityData*, bool = true);
	EQLIB_OBJECT bool CanTrainAbility(PcClient* pWho, CAltAbilityData* pAbility, bool = false, bool = false, bool = false);

	EQLIB_OBJECT CAltAbilityData* GetOwnedAbilityFromGroupID(PcZoneClient* pc, int groupId);

/*0x000*/ RequirementAssociationManager reqAssocManager;
/*0x248*/ HashTable<CAltAbilityData*>* abilities;
/*0x250*/
};

//============================================================================

// Mercenary AltAbilities


class [[offsetcomments]] MercenaryAbilityReq
{
public:
/*0x00*/ int           ReqGroupID;
/*0x04*/ int           ReqGroupRank;
/*0x08*/
};

class [[offsetcomments]] MercenaryAbilitiesData
{
public:
	EQLIB_OBJECT const char* GetNameString() const;
	EQLIB_OBJECT const char* GetDescriptionString() const;
	EQLIB_OBJECT const char* GetTypeString() const;

	EQLIB_OBJECT bool IsRequirementsMet() const;
	EQLIB_OBJECT bool IsRequirementMet(const MercenaryAbilityReq& req) const;

/*0x00*/ int           AbilityID;
/*0x04*/ int           nName;                    // eMercenaryAbilityName
/*0x08*/ int           nDesc;                    // eMercenaryAbilityDescription
/*0x0c*/ int           Cost;
/*0x10*/ int           GroupID;
/*0x14*/ int           GroupRank;
/*0x18*/ int           Type;
/*0x1c*/ int           MinPlayerLevel;           // min level to purchase...
/*0x20*/ int           RequirementAssociationID;
/*0x24*/ int           Refund;
/*0x28*/ ArrayClass<MercenaryAbilityReq> AbilityReqs;
/*0x40*/
};

constexpr int MercenaryAbilityGroupsSize = 16;

using MercenaryAbilityGroup = HashList<int, MercenaryAbilityGroupsSize>;

class [[offsetcomments]] MercenaryAlternateAdvancementManagerClient
{
	enum {
		TypesSize = 5,
		AbilitiesSize = 64,
	};
public:
	EQLIB_OBJECT static MercenaryAlternateAdvancementManagerClient& Instance();

	EQLIB_OBJECT bool CanTrainAbility(int abilityId) const;
	EQLIB_OBJECT void BuyAbility(int abilityId, bool trainAll = false, bool unk = false);

	// Look up an ability by its id
	EQLIB_OBJECT const MercenaryAbilitiesData* GetMercenaryAbility(int abilityId) const;

	// Look up the first ability id that matches the given group id
	EQLIB_OBJECT const int* GetFirstMercenaryAbilityIdByGroupId(int groupId) const;
	// look up the next ability id in the given group
	EQLIB_OBJECT const int* GetNextMercenaryAbilityIdByGroupId(const int* groupId) const;

	EQLIB_OBJECT int GetMercenaryMaxGroupRank(int abilityId) const;
	EQLIB_OBJECT int GetMercenaryMaxOwnedGroupRank(int abilityId) const;

	EQLIB_OBJECT int GetMercenaryNextGroupAbility(int abilityId) const;
	EQLIB_OBJECT const MercenaryAbilitiesData* GetFirstUnownedAbilityByGroupId(int groupId) const;

/*0x000*/ HashList<int, TypesSize>                              MercenaryTypes;
/*0x048*/ HashList<MercenaryAbilitiesData, AbilitiesSize>       MercenaryAbilities;
/*0x268*/ HashList<int, AbilitiesSize>                          MercenaryAbilitiesByGroupID;
/*0x488*/ RequirementAssociationManager                         ReqAssocManager;
/*0x6d0*/ HashList<int, AbilitiesSize>                          MercenaryAbilitiesOwnedByGroupID;
/*0x8f0*/ HashList<MercenaryAbilityGroup, AbilitiesSize>        MercenaryAbilityGroups;
/*0xb10*/
};

} // namespace eqlib
