/*
Copyright © 2011-2012 Clint Bellanger and kitano
Copyright © 2012-2015 Justin Jacobs

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
 * class Entity
 *
 * An Entity represents any character in the game - the player, allies, enemies
 * This base class handles logic common to all of these child classes
 */


#ifndef ENTITY_H
#define ENTITY_H

#include "CommonIncludes.h"
#include "SoundManager.h"
#include "StatBlock.h"

class Animation;
class AnimationSet;

class Entity {
protected:
	Image *sprites;

	void move_from_offending_tile();
	virtual void resetActiveAnimation();

public:
	Entity();
	Entity(const Entity&);
	virtual ~Entity();

	void loadSounds(StatBlock *src_stats = NULL);
	void unloadSounds();
	void playAttackSound(const std::string& attack_name);
	bool move();
	bool takeHit(Hazard &h);
	virtual void doRewards(int) {}

	// sound effects
	std::vector<std::pair<std::string, SoundManager::SoundID> > sound_attack;
	SoundManager::SoundID sound_hit;
	SoundManager::SoundID sound_die;
	SoundManager::SoundID sound_critdie;
	SoundManager::SoundID sound_block;
	SoundManager::SoundID sound_generationup;

	bool setAnimation(const std::string& animation);
	Animation *activeAnimation;
	AnimationSet *animationSet;

	StatBlock stats;
};

extern const int directionDeltaX[];
extern const int directionDeltaY[];
extern const float speedMultiplyer[];

#endif

