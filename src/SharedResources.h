/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013-2014 Henrik Andersson
Copyright © 2013 Kurt Rinnert
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
SharedResources

"Global" sort of system resources that are used by most game classes.
Only one instance of these classes are needed by the engine.
Generic objects only. Game-specific objects don't belong here.
Created and destroyed by main.cpp
**/

#ifndef SHARED_RESOURCES_H
#define SHARED_RESOURCES_H

#include "CommonIncludes.h"
#include "AnimationManager.h"
#include "CombatText.h"
#include "CursorManager.h"
#include "FontEngine.h"
#include "IconManager.h"
#include "InputState.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "SoundManager.h"
#include "RenderDevice.h"
#include "SaveLoad.h"

extern AnimationManager *anim;
extern CombatText *comb;
extern CursorManager *curs;
extern FontEngine *font;
extern IconManager *icons;
extern InputState *inpt;
extern MessageEngine *msg;
extern ModManager *mods;
extern SoundManager *snd;
extern RenderDevice *render_device;
extern SaveLoad *save_load;

#endif
