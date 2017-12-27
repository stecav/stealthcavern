/*
Copyright © 2012-2016 Justin Jacobs
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

/**
 * class EffectManager
 *
 * Holds the collection of hazards (active attacks, spells, etc) and handles group operations
 */

#ifndef EFFECT_MANAGER_H
#define EFFECT_MANAGER_H

#include "CommonIncludes.h"
#include "Hazard.h"
#include "SharedResources.h"
#include "Stats.h"
#include "Utils.h"

class Animation;
class Hazard;

#define EFFECT_COUNT 38

enum EFFECT_TYPE {
	EFFECT_NONE = 0,
	EFFECT_DAMAGE = 1,
	EFFECT_DAMAGE_PERCENT = 2,
	EFFECT_HPOT = 3,
	EFFECT_HPOT_PERCENT = 4,
	EFFECT_MPOT = 5,
	EFFECT_MPOT_PERCENT = 6,
	EFFECT_SPOT = 7,
	EFFECT_SPOT_PERCENT = 8,
	EFFECT_SPEED = 9,
	EFFECT_IMMUNITY = 10,
	EFFECT_IMMUNITY_DAMAGE = 11,
	EFFECT_IMMUNITY_SLOW = 12,
	EFFECT_IMMUNITY_STUN = 13,
	EFFECT_IMMUNITY_HULL_STEAL = 14,
	EFFECT_IMMUNITY_PSI_STEAL = 15,
	EFFECT_IMMUNITY_CAPACITOR_STEAL = 16,
	EFFECT_IMMUNITY_KNOCKBACK = 17,
	EFFECT_IMMUNITY_DAMAGE_REFLECT = 18,
	EFFECT_STUN = 19,
	EFFECT_REVIVE = 20,
	EFFECT_CONVERT = 21,
	EFFECT_FEAR = 22,
	EFFECT_INFLUENCE = 23,
	EFFECT_LUCK = 24,
	EFFECT_BRAWN = 25,
	EFFECT_RESOLVE = 26,
	EFFECT_RESILIENCE = 27,
	EFFECT_CONJURE = 28,
	EFFECT_NIMBLE = 29,
	EFFECT_CONCENTRATION = 30,
	EFFECT_DEATH_SENTENCE = 31,
	EFFECT_SHIELD = 32,
	EFFECT_HEAL = 33,
	EFFECT_KNOCKBACK = 34,
	EFFECT_IMMUNITY_STAT_DEBUFF = 35,
	EFFECT_ATTACK_SPEED = 36,
	EFFECT_FEALTY = 37
};

class Effect {
public:
	std::string id;
	std::string name;
	int icon;
	int ticks;
	int duration;
	int type;
	int magnitude;
	int magnitude_max;
	std::string animation_name;
	Animation* animation;
	bool item;
	int trigger;
	bool render_above;
	int passive_id;
	int source_type;
	bool group_stack;
	Color color_mod;
	uint8_t alpha_mod;
	std::string attack_speed_anim;

	Effect()
		: id("")
		, name("")
		, icon(-1)
		, ticks(0)
		, duration(-1)
		, type(EFFECT_NONE)
		, magnitude(0)
		, magnitude_max(0)
		, animation_name("")
		, animation(NULL)
		, item(false)
		, trigger(-1)
		, render_above(false)
		, passive_id(0)
		, source_type(SOURCE_TYPE_HERO) 
		, group_stack(false)
		, color_mod(255, 255, 255)
		, alpha_mod(255)
		, attack_speed_anim("") {
	}

	~Effect() {
	}

};

class EffectManager {
private:
	Animation* loadAnimation(const std::string &s);
	void removeEffect(size_t id);
	void removeAnimation(size_t id);
	void clearStatus();
	int getType(const std::string& type);

public:
	EffectManager();
	~EffectManager();
	EffectManager& operator= (const EffectManager &emSource);
	void logic();
	void addEffect(EffectDef &effect, int duration, int magnitude, bool item, int trigger, int passive_id, int source_type);
	void removeEffectType(const int type);
	void removeEffectPassive(int id);
	void removeEffectID(const std::vector< std::pair<std::string, int> >& remove_effects);
	void clearEffects();
	void clearNegativeEffects(int type = -1);
	void clearItemEffects();
	void clearTriggerEffects(int trigger);
	int damageShields(int dmg);
	bool isDebuffed();
	void getCurrentColor(Color& color_mod);
	void getCurrentAlpha(uint8_t& alpha_mod);
	bool hasEffect(const std::string& id, int req_count);
	float getAttackSpeed(const std::string& anim_name);

	std::vector<Effect> effect_list;

	int damage;
	int damage_percent;
	int hpot;
	int hpot_percent;
	int mpot;
	int mpot_percent;
	int spot;
	int spot_percent;
	float speed;
	bool immunity_damage;
	bool immunity_slow;
	bool immunity_stun;
	bool immunity_hull_steal;
	bool immunity_psi_steal;
	bool immunity_capacitor_steal;
	bool immunity_knockback;
	bool immunity_damage_reflect;
	bool immunity_stat_debuff;
	bool stun;
	bool revive;
	bool convert;
	bool death_sentence;
	bool fear;
	float knockback_speed;

	std::vector<int> bonus;
	std::vector<int> bonus_resist;
	std::vector<int> bonus_primary;
	std::vector<int> bonus_secondary;
	std::vector<int> bonus_fealty;

	bool triggered_others;
	bool triggered_block;
	bool triggered_hit;
	bool triggered_halfdeath;
	bool triggered_joincombat;
	bool triggered_death;
	bool refresh_stats;
};

#endif
