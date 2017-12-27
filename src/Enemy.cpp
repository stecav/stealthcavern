/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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

#include "Animation.h"
#include "Avatar.h"
#include "BehaviorAlly.h"
#include "BehaviorStandard.h"
#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "EnemyBehavior.h"
#include "Enemy.h"
#include "Hazard.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsMath.h"
#include <math.h>

Enemy::Enemy() : Entity() {

	stats.cur_state = ENEMY_STANCE;
	stats.turn_ticks = MAX_FRAMES_PER_SEC;
	stats.cooldown = 0;
	stats.in_combat = false;
	stats.join_combat = false;

	haz = NULL;

	reward_soul = false;
	instant_power = false;
	kill_source_type = SOURCE_TYPE_NEUTRAL;
	eb = NULL;
}

Enemy::Enemy(const Enemy& e)
	: Entity(e)
	, type(e.type)
	, haz(NULL) // do not copy hazard. This constructor is used during mapload, so no hazard should be active.
	, reward_soul(e.reward_soul)
	, instant_power(e.instant_power)
	, kill_source_type(e.kill_source_type) {
	eb = new BehaviorStandard(this); // Putting a 'this' into the init list will make MSVS complain, hence it's in the body of the ctor
	assert(e.haz == NULL);
}

/**
 * The current direction leads to a wall.  Try the next best direction, if one is available.
 */
unsigned char Enemy::faceNextBest(float mapx, float mapy) {
	float dx = static_cast<float>(fabs(mapx - stats.pos.x));
	float dy = static_cast<float>(fabs(mapy - stats.pos.y));
	switch (stats.direction) {
		case 0:
			if (dy > dx) return 7;
			else return 1;
		case 1:
			if (mapy > stats.pos.y) return 0;
			else return 2;
		case 2:
			if (dx > dy) return 1;
			else return 3;
		case 3:
			if (mapx < stats.pos.x) return 2;
			else return 4;
		case 4:
			if (dy > dx) return 3;
			else return 5;
		case 5:
			if (mapy < stats.pos.y) return 4;
			else return 6;
		case 6:
			if (dx > dy) return 5;
			else return 7;
		case 7:
			if (mapx > stats.pos.x) return 6;
			else return 0;
	}
	return 0;
}

/**
 * logic()
 * Handle a single frame.  This includes:
 * - move the enemy based on AI % chances
 * - calculate the next frame of animation
 */
void Enemy::logic() {

	eb->logic();

	//need to check whether the enemy was converted here
	//cant do it in behaviour because the behaviour object would be replaced by this
	if(stats.effects.convert != stats.converted) {
		delete eb;
		eb = stats.hero_ally ? new BehaviorStandard(this) : new BehaviorAlly(this);
		stats.converted = !stats.converted;
		stats.hero_ally = !stats.hero_ally;
		if (stats.convert_status != "") {
			camp->setStatus(stats.convert_status);
		}
	}

	return;
}

/**
 * Upon enemy death, handle rewards (currency, soul, loot)
 */
void Enemy::doRewards(int source_type) {

	if(stats.hero_ally && !stats.converted)
		return;

	reward_soul = true;
	kill_source_type = source_type;

	// some creatures create special loot if we're on a mission
	if (stats.mission_loot_requires_status != "") {

		// the loot manager will check mission_loot_id
		// if set (not zero), the loot manager will 100% generate that loot.
		if (!(camp->checkStatus(stats.mission_loot_requires_status) && !camp->checkStatus(stats.mission_loot_requires_not_status))) {
			stats.mission_loot_id = 0;
		}
	}

	// some creatures drop special loot the first time they are defeated
	// this must be done in conjunction with defeat status
	if (stats.first_defeat_loot > 0) {
		if (!camp->checkStatus(stats.defeat_status)) {
			stats.mission_loot_id = stats.first_defeat_loot;
		}
	}

	// defeating some creatures (e.g. bosses) affects the story
	if (stats.defeat_status != "") {
		camp->setStatus(stats.defeat_status);
	}

	loot->addEnemyLoot(this);
}

/**
 * getRender()
 * Map objects need to be drawn in Z order, so we allow a parent object (GameEngine)
 * to collect all mobile sprites each frame.
 */
Renderable Enemy::getRender() {
	Renderable r = activeAnimation->getCurrentFrame(stats.direction);
	r.map_pos.x = stats.pos.x;
	r.map_pos.y = stats.pos.y;
	return r;
}

Enemy::~Enemy() {
	delete haz;
	delete eb;
}

