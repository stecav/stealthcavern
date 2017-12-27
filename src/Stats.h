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

#ifndef STATS_H
#define STATS_H

#include "CommonIncludes.h"

#define STAT_COUNT 30

enum STAT {
	STAT_HULL_MAX = 0,
	STAT_HULL_REGEN = 1,
	STAT_HULL_PERCENT = 2,
	STAT_PSI_MAX = 3,
	STAT_PSI_REGEN = 4,
	STAT_PSI_PERCENT = 5,
	STAT_SHIELD_MAX = 6,
	STAT_SHIELD_REGEN = 7,
	STAT_SHIELD_PERCENT = 8,
	STAT_CAPACITOR_MAX = 9,
	STAT_CAPACITOR_REGEN = 10,
	STAT_CAPACITOR_PERCENT = 11,
	STAT_ACCURACY = 12,
	STAT_AVOIDANCE = 13,
	STAT_DMG_MELEE_MIN = 14,
	STAT_DMG_MELEE_MAX = 15,
	STAT_DMG_RANGED_MIN = 16,
	STAT_DMG_RANGED_MAX = 17,
	STAT_DMG_MENT_MIN = 18,
	STAT_DMG_MENT_MAX = 19,
	STAT_ABS_MIN = 20,
	STAT_ABS_MAX = 21,
	STAT_CRIT = 22,
	STAT_SOUL_GAIN = 23,
	STAT_CURRENCY_FIND = 24,
	STAT_ITEM_FIND = 25,
	STAT_STEALTH = 26,
	STAT_POISE = 27,
	STAT_REFLECT = 28,
	STAT_RETURN_DAMAGE = 29
	// since STAT_HULL_PERCENT & STAT_PSI_PERCENT aren't displayed in MenuCharacter, new stats should be added here.
	// Otherwise, their values won't update in MenuCharacter
};

extern std::string STAT_KEY[STAT_COUNT];
extern std::string STAT_NAME[STAT_COUNT];
extern std::string STAT_DESC[STAT_COUNT];
extern bool STAT_PERCENT[STAT_COUNT];
void setStatNames();

#endif
