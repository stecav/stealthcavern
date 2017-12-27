/*
Copyright © 2013-2015 Justin Jacobs
Copyright © 2017 Bryan Mcdowell

This file is part of Stealth Cavern.

Stealth Cavern is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Stealth Cavern is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Stealth Cavern.  If not, see http://www.gnu.org/licenses/
*/

#include "Settings.h"
#include "SharedResources.h"
#include "Stats.h"

std::string STAT_KEY[STAT_COUNT];
std::string STAT_NAME[STAT_COUNT];
std::string STAT_DESC[STAT_COUNT];
bool STAT_PERCENT[STAT_COUNT];

// STAT_KEY values aren't visible in-game, but they are used for parsing config files like engine/stats.txt
// STAT_NAME values are the translated strings visible in the Character menu and item tooltips
// STAT_DESC values are the translated descriptions of stats visible in Character menu tooltips
// STAT_PERCENT is used to determine if we should treat the value as a percentage when displaying it (i.e. use %)
void setStatNames() {
	// @CLASS Stats|Description of the base stats, aka "stat name" or ${STATNAME}

	// @TYPE hull|Hit points
	STAT_KEY[STAT_HULL_MAX] = "hull";
	STAT_NAME[STAT_HULL_MAX] = msg->get("Max hull");
	STAT_DESC[STAT_HULL_MAX] = "";
	STAT_PERCENT[STAT_HULL_MAX] = false;
	// @TYPE hull_regen|HULL restored per minute
	STAT_KEY[STAT_HULL_REGEN] = "hull_regen";
	STAT_NAME[STAT_HULL_REGEN] = msg->get("Hull regen");
	STAT_DESC[STAT_HULL_REGEN] = msg->get("Ticks of Hull regen per minute.");
	STAT_PERCENT[STAT_HULL_REGEN] = false;
	// @TYPE hull_percent|Base HULL altered by percentage
	STAT_KEY[STAT_HULL_PERCENT] = "hull_percent";
	STAT_NAME[STAT_HULL_PERCENT] = msg->get("Base hull");
	STAT_DESC[STAT_HULL_PERCENT] = "";
	STAT_PERCENT[STAT_HULL_PERCENT] = true;
	// @TYPE psi|PSI points
	STAT_KEY[STAT_PSI_MAX] = "psi";
	STAT_NAME[STAT_PSI_MAX] = msg->get("Max PSI");
	STAT_DESC[STAT_PSI_MAX] = "";
	STAT_PERCENT[STAT_PSI_MAX] = false;
	// @TYPE psi_regen|PSI restored per minute
	STAT_KEY[STAT_PSI_REGEN] = "psi_regen";
	STAT_NAME[STAT_PSI_REGEN] = msg->get("PSI regen");
	STAT_DESC[STAT_PSI_REGEN] = msg->get("Ticks of PSI regen per minute.");
	STAT_PERCENT[STAT_PSI_REGEN] = false;
	// @TYPE psi_percent|Base PSI altered by percentage
	STAT_KEY[STAT_PSI_PERCENT] = "psi_percent";
	STAT_NAME[STAT_PSI_PERCENT] = msg->get("Base PSI");
	STAT_DESC[STAT_PSI_PERCENT] = "";
	STAT_PERCENT[STAT_PSI_PERCENT] = true;
	STAT_KEY[STAT_SHIELD_MAX] = "shield";
	STAT_NAME[STAT_SHIELD_MAX] = "Max shield";
	STAT_KEY[STAT_SHIELD_REGEN] = "shield_regen";
	STAT_NAME[STAT_SHIELD_REGEN] = "Shield regen";
	STAT_KEY[STAT_SHIELD_PERCENT] = "shield_percent";
	STAT_NAME[STAT_SHIELD_PERCENT] = msg->get("Base shield");
	STAT_DESC[STAT_SHIELD_PERCENT] = "";
	STAT_PERCENT[STAT_SHIELD_PERCENT] = true;
	STAT_KEY[STAT_CAPACITOR_MAX] = "capacitor";
	STAT_NAME[STAT_CAPACITOR_MAX] = "Max capacitor";
	STAT_KEY[STAT_CAPACITOR_REGEN] = "capacitor_regen";
	STAT_NAME[STAT_CAPACITOR_REGEN] = "Capacitor regen";
	STAT_KEY[STAT_CAPACITOR_PERCENT] = msg->get("Base capacitor");
	STAT_PERCENT[STAT_CAPACITOR_PERCENT] = true;
	STAT_DESC[STAT_CAPACITOR_PERCENT] = "";
	// @TYPE accuracy|Accuracy %. Higher values mean less likely to miss.
	STAT_KEY[STAT_ACCURACY] = "accuracy";
	STAT_NAME[STAT_ACCURACY] = msg->get("Accuracy");
	STAT_DESC[STAT_ACCURACY] = "";
	STAT_PERCENT[STAT_ACCURACY] = true;
	// @TYPE avoidance|Avoidance %. Higher values means more likely to not get hit.
	STAT_KEY[STAT_AVOIDANCE] = "avoidance";
	STAT_NAME[STAT_AVOIDANCE] = msg->get("Avoidance");
	STAT_DESC[STAT_AVOIDANCE] = "";
	STAT_PERCENT[STAT_AVOIDANCE] = true;
	// @TYPE dmg_melee_min|Minimum melee damage
	STAT_KEY[STAT_DMG_MELEE_MIN] = "dmg_melee_min";
	STAT_NAME[STAT_DMG_MELEE_MIN] = msg->get("Melee damage min");
	STAT_DESC[STAT_DMG_MELEE_MIN] = "";
	STAT_PERCENT[STAT_DMG_MELEE_MIN] = false;
	// @TYPE dmg_melee_max|Maximum melee damage
	STAT_KEY[STAT_DMG_MELEE_MAX] = "dmg_melee_max";
	STAT_NAME[STAT_DMG_MELEE_MAX] = msg->get("Melee damage max");
	STAT_DESC[STAT_DMG_MELEE_MAX] = "";
	STAT_PERCENT[STAT_DMG_MELEE_MAX] = false;
	// @TYPE dmg_ranged_min|Minimum ranged damage
	STAT_KEY[STAT_DMG_RANGED_MIN] = "dmg_ranged_min";
	STAT_NAME[STAT_DMG_RANGED_MIN] = msg->get("Ranged damage min");
	STAT_DESC[STAT_DMG_RANGED_MIN] = "";
	STAT_PERCENT[STAT_DMG_RANGED_MIN] = false;
	// @TYPE dmg_ranged_max|Maximum ranged damage
	STAT_KEY[STAT_DMG_RANGED_MAX] = "dmg_ranged_max";
	STAT_NAME[STAT_DMG_RANGED_MAX] = msg->get("Ranged damage max");
	STAT_DESC[STAT_DMG_RANGED_MAX] = "";
	STAT_PERCENT[STAT_DMG_RANGED_MAX] = false;
	// @TYPE dmg_ment_min|Minimum mental damage
	STAT_KEY[STAT_DMG_MENT_MIN] = "dmg_ment_min";
	STAT_NAME[STAT_DMG_MENT_MIN] = msg->get("Mental damage min");
	STAT_DESC[STAT_DMG_MENT_MIN] = "";
	STAT_PERCENT[STAT_DMG_MENT_MIN] = false;
	// @TYPE dmg_ment_max|Maximum mental damage
	STAT_KEY[STAT_DMG_MENT_MAX] = "dmg_ment_max";
	STAT_NAME[STAT_DMG_MENT_MAX] = msg->get("Mental damage max");
	STAT_DESC[STAT_DMG_MENT_MAX] = "";
	STAT_PERCENT[STAT_DMG_MENT_MAX] = false;
	// @TYPE absorb_min|Minimum damage absorption
	STAT_KEY[STAT_ABS_MIN] = "absorb_min";
	STAT_NAME[STAT_ABS_MIN] = msg->get("Absorb min");
	STAT_DESC[STAT_ABS_MIN] = "";
	STAT_PERCENT[STAT_ABS_MIN] = false;
	// @TYPE absorb_max|Maximum damage absorption
	STAT_KEY[STAT_ABS_MAX] = "absorb_max";
	STAT_NAME[STAT_ABS_MAX] = msg->get("Absorb max");
	STAT_DESC[STAT_ABS_MAX] = "";
	STAT_PERCENT[STAT_ABS_MAX] = false;
	// @TYPE crit|Critical hit chance %
	STAT_KEY[STAT_CRIT] = "crit";
	STAT_NAME[STAT_CRIT] = msg->get("Critical hit chance");
	STAT_DESC[STAT_CRIT] = "";
	STAT_PERCENT[STAT_CRIT] = true;
	// @TYPE soul_gain|Percentage boost to the amount of soul points gained per kill.
	STAT_KEY[STAT_SOUL_GAIN] = "soul_gain";
	STAT_NAME[STAT_SOUL_GAIN] = msg->get("Bonus soul");
	STAT_DESC[STAT_SOUL_GAIN] = msg->get("Increases the soul gained per kill.");
	STAT_PERCENT[STAT_SOUL_GAIN] = true;
	// @TYPE currency_find|Percentage boost to the amount of credits dropped per loot event.
	STAT_KEY[STAT_CURRENCY_FIND] = "credit_find";
	STAT_NAME[STAT_CURRENCY_FIND] = msg->get("Bonus %s", CURRENCY);
	STAT_DESC[STAT_CURRENCY_FIND] = msg->get("Increases the %s found per drop.", CURRENCY);
	STAT_PERCENT[STAT_CURRENCY_FIND] = true;
	// @TYPE item_find|Increases the chance of finding items in loot.
	STAT_KEY[STAT_ITEM_FIND] = "item_find";
	STAT_NAME[STAT_ITEM_FIND] = msg->get("Item find chance");
	STAT_DESC[STAT_ITEM_FIND] = msg->get("Increases the chance that an enemy will drop an item.");
	STAT_PERCENT[STAT_ITEM_FIND] = true;
	// @TYPE stealth|Decrease the distance required to alert enemies by %
	STAT_KEY[STAT_STEALTH] = "stealth";
	STAT_NAME[STAT_STEALTH] = msg->get("Stealth");
	STAT_DESC[STAT_STEALTH] = msg->get("Increases your ability to move undetected.");
	STAT_PERCENT[STAT_STEALTH] = true;
	// @TYPE poise|Reduced % chance of entering "hit" animation when damaged
	STAT_KEY[STAT_POISE] = "poise";
	STAT_NAME[STAT_POISE] = msg->get("Poise");
	STAT_DESC[STAT_POISE] = msg->get("Reduces your chance of stumbling when hit.");
	STAT_PERCENT[STAT_POISE] = true;
	// @TYPE reflect_chance|Percentage chance to reflect missiles
	STAT_KEY[STAT_REFLECT] = "reflect_chance";
	STAT_NAME[STAT_REFLECT] = msg->get("Missile reflect chance");
	STAT_DESC[STAT_REFLECT] = msg->get("Increases your chance of reflecting missiles back at enemies.");
	STAT_PERCENT[STAT_REFLECT] = true;
	// @TYPE return_damage|Deals a percentage of the damage taken back to the attacker
	STAT_KEY[STAT_RETURN_DAMAGE] = "return_damage";
	STAT_NAME[STAT_RETURN_DAMAGE] = msg->get("Damage reflection");
	STAT_DESC[STAT_RETURN_DAMAGE] = msg->get("Deals a percentage of damage taken back to the attacker.");
	STAT_PERCENT[STAT_RETURN_DAMAGE] = true;
}

