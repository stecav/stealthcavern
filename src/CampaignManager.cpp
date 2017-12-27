/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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
 * class CampaignManager
 *
 * Contains data for story mode
 */

#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "Menu.h"
#include "MenuManager.h"
#include "MenuInventory.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"

CampaignManager::CampaignManager()
	: status()
	, bonus_soul(0.0) {
}

/**
 * Take the savefile campaign= and convert to status array
 */
void CampaignManager::setAll(const std::string& s) {
	std::string str = s + ',';
	std::string token;
	while (str != "") {
		token = popFirstString(str);
		if (token != "") this->setStatus(token);
	}
}

/**
 * Convert status array to savefile campaign= (status csv)
 */
std::string CampaignManager::getAll() {
	std::stringstream ss;
	ss.str("");
	for (unsigned int i=0; i < status.size(); i++) {
		ss << status[i];
		if (i < status.size()-1) ss << ',';
	}
	return ss.str();
}

bool CampaignManager::checkStatus(const std::string& s) {

	// avoid searching empty statuses
	if (s == "") return false;

	for (unsigned int i=0; i < status.size(); i++) {
		if (status[i] == s) return true;
	}
	return false;
}

void CampaignManager::setStatus(const std::string& s) {

	// avoid adding empty statuses
	if (s == "") return;

	// if it's already set, don't add it again
	if (checkStatus(s)) return;

	status.push_back(s);
	pc->stats.check_title = true;
}

void CampaignManager::unsetStatus(const std::string& s) {

	// avoid searching empty statuses
	if (s == "") return;

	std::vector<std::string>::iterator it;
	// see http://stackoverflow.com/a/223405
	for (it = status.end(); it != status.begin();) {
		--it;
		if ((*it) == s) {
			it = status.erase(it);
			return;
		}
		pc->stats.check_title = true;
	}
}

bool CampaignManager::checkCurrency(int quantity) {
	return menu->inv->inventory[CARRIED].contain(CURRENCY_ID, quantity);
}

bool CampaignManager::checkItem(int item_id) {
	if (menu->inv->inventory[CARRIED].contain(item_id))
		return true;
	else
		return menu->inv->inventory[EQUIPMENT].contain(item_id);
}

void CampaignManager::removeCurrency(int quantity) {
	int max_amount = std::min(quantity, menu->inv->currency);

	if (max_amount > 0) {
		menu->inv->removeCurrency(max_amount);
		pc->logMsg(msg->get("%d %s removed.", max_amount, CURRENCY), false);
		items->playSound(CURRENCY_ID);
	}
}

void CampaignManager::removeItem(int item_id) {
	if (item_id < 0 || static_cast<unsigned>(item_id) >= items->items.size()) return;

	if (menu->inv->remove(item_id)) {
		pc->logMsg(msg->get("%s removed.", items->getItemName(item_id)), false);
		items->playSound(item_id);
	}
}

void CampaignManager::rewardItem(ItemStack istack) {
	if (istack.empty())
		return;

	menu->inv->add(istack, CARRIED, -1, true, true);

	if (istack.item != CURRENCY_ID) {
		if (istack.quantity <= 1)
			pc->logMsg(msg->get("You receive %s.", items->getItemName(istack.item)), false);
		if (istack.quantity > 1)
			pc->logMsg(msg->get("You receive %s x%d.", istack.quantity, items->getItemName(istack.item)), false);
	}
}

void CampaignManager::rewardCurrency(int amount) {
	ItemStack stack;
	stack.item = CURRENCY_ID;
	stack.quantity = amount;

	pc->logMsg(msg->get("You receive %d %s.", amount, CURRENCY), false);
	rewardItem(stack);
}

void CampaignManager::rewardSoul(int amount, bool show_message) {
	bonus_soul += (static_cast<float>(amount) * (100.0f + static_cast<float>(pc->stats.get(STAT_SOUL_GAIN)))) / 100.0f;
	pc->stats.addSoul(static_cast<int>(bonus_soul));
	bonus_soul -= static_cast<float>(static_cast<int>(bonus_soul));
	pc->stats.refresh_stats = true;
	if (show_message) pc->logMsg(msg->get("You receive %d Soul.", amount), false);
}

void CampaignManager::rewardStat(const std::string& s, int stat) {
	pc->logMsg(msg->get("Your %s increased by %d.", stat, s), false);
	for(int q = 0; q < STAT_COUNT;q++){
		if(s == STAT_KEY[q]) {
			pc->stats.primary[q] += stat;
			break;
		}
	}
	pc->stats.refresh_stats = true;
}

void CampaignManager::rewardTalent(const std::string& s, int rTalent) {
	for(unsigned q = 0; q < TALENTS.size();q++){
		if(s == TALENTS[q].id) {
			pc->logMsg(msg->get("Your %s increased by %d.", rTalent, s), false);
			pc->stats.talent[q] += rTalent;
			break;
		}
	}
	pc->stats.refresh_stats = true;
}

void CampaignManager::restoreHullCapacitor(const std::string& s) {
	if (s == "hull") {
		pc->stats.hull = pc->stats.get(STAT_HULL_MAX);
		pc->logMsg(msg->get("Hull restored."), false);
	}
	else if (s == "capacitor") {
		pc->stats.capacitor = pc->stats.get(STAT_CAPACITOR_MAX);
		pc->logMsg(msg->get("Capacitor restored."), false);
	}
	else if (s == "psi") {
		pc->stats.psi = pc->stats.get(STAT_PSI_MAX);
		pc->logMsg(msg->get("PSI restored."), false);
	}
	else if (s == "hullcapacitor") {
		pc->stats.hull = pc->stats.get(STAT_HULL_MAX);
		pc->stats.capacitor = pc->stats.get(STAT_CAPACITOR_MAX);
		pc->logMsg(msg->get("Hull and Capacitor restored."), false);
	}
	else if (s == "hullpsi") {
		pc->stats.hull = pc->stats.get(STAT_HULL_MAX);
		pc->stats.psi = pc->stats.get(STAT_PSI_MAX);
		pc->logMsg(msg->get("Hull and PSI restored."), false);
	}
	else if (s == "status") {
		pc->stats.effects.clearNegativeEffects();
		pc->logMsg(msg->get("Negative effects removed."), false);
	}
	else if (s == "all") {
		pc->stats.hull = pc->stats.get(STAT_HULL_MAX);
		pc->stats.psi = pc->stats.get(STAT_PSI_MAX);
		pc->stats.capacitor = pc->stats.get(STAT_CAPACITOR_MAX);
		pc->stats.shield = pc->stats.get(STAT_SHIELD_MAX);
		pc->stats.effects.clearNegativeEffects();
		pc->logMsg(msg->get("HULL and PSI restored, negative effects removed"), false);
	}
}

bool CampaignManager::checkAllRequirements(const Event_Component& ec) {
	if (ec.type == EC_REQUIRES_STATUS) {
		if (checkStatus(ec.s))
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_STATUS) {
		if (!checkStatus(ec.s))
			return true;
	}
	else if (ec.type == EC_REQUIRES_CURRENCY) {
		if (checkCurrency(ec.x))
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_CURRENCY) {
		if (!checkCurrency(ec.x))
			return true;
	}
	else if (ec.type == EC_REQUIRES_ITEM) {
		if (checkItem(ec.x))
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_ITEM) {
		if (!checkItem(ec.x))
			return true;
	}
	else if (ec.type == EC_REQUIRES_GENERATION) {
		if (pc->stats.generation >= ec.x)
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_GENERATION) {
		if (pc->stats.generation < ec.x)
			return true;
	}
	else if (ec.type == EC_REQUIRES_CLASS) {
		if (pc->stats.character_class == ec.s)
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_CLASS) {
		if (pc->stats.character_class != ec.s)
			return true;
	}
	else if (ec.type == EC_REQUIRES_TALENT) {
		for(unsigned int u=0; u < TALENTS.size(); u++){
			if (ec.s == TALENTS[u].id){
				if(pc->stats.talent[u] >= ec.x){ return true; }
			}
		}
	}
	else if (ec.type == EC_CHALLENGE_STAT) {
		for(unsigned int y=0; y < pc->stats.primary.size();y++){
			if(ec.s == PRIMARY_STATS[y].id){
				if((pc->stats.primary[y] > 5) || checkCurrency(ec.x)){ 
					return true;
				}
			}
		}
	}
	else {
		// Event component is not a requirement check
		// treat it as if the "requirement" was met
		return true;
	}

	// requirement check failed
	return false;
}

CampaignManager::~CampaignManager() {
}
