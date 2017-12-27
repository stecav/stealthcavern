/*
Copyright © 2011-2012 kitano
Copyright © 2014-2015 Justin Jacobs

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

#include "GameState.h"

GameState::GameState()
	: hasMusic(false)
	, has_background(true)
	, reload_music(false)
	, reload_backgrounds(false)
	, save_settings_on_exit(true)
	, load_counter(0)
	, requestedGameState(NULL)
	, exitRequested(false) {
}

GameState* GameState::getRequestedGameState() {
	return requestedGameState;
}

void GameState::logic() {
}

void GameState::render() {
}

void GameState::refreshWidgets() {
}

void GameState::setLoadingFrame() {
	load_counter = 2;
}

bool GameState::isPaused() {
	return false;
}

GameState::~GameState() {
}
