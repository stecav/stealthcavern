/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2014 Henrik Andersson
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
 * GameStateLoad
 */

#include "Avatar.h"
#include "FileParser.h"
#include "GameStateLoad.h"
#include "GameStateTitle.h"
#include "GameStatePlay.h"
#include "GameStateNew.h"
#include "ItemManager.h"
#include "MenuConfirm.h"
#include "SaveLoad.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "WidgetScrollBar.h"

bool compareSaveDirs(const std::string& dir1, const std::string& dir2) {
	int first = toInt(dir1);
	int second = toInt(dir2);

	return first < second;
}

GameSlot::GameSlot()
	: id(0) {
}

GameSlot::~GameSlot() {
	for (size_t i=0; i<sprites.size(); ++i) {
		delete sprites[i];
	}
	sprites.clear();
}

GameStateLoad::GameStateLoad() : GameState()
	, background(NULL)
	, selection(NULL)
	, portrait_border(NULL)
	, portrait(NULL)
	, loading_requested(false)
	, loading(false)
	, loaded(false)
	, delete_items(true)
	, current_frame(0)
	, frame_ticker(0)
	, stance_frames(1)
	, stance_ticks_per_frame(1)
	, stance_duration(1)
	, stance_type(PLAY_ONCE)
	, selected_slot(-1)
	, visible_slots(0)
	, scroll_offset(0)
	, has_scroll_bar(false)
	, game_slot_max(4)
	, text_trim_boundary(0) {

	if (items == NULL)
		items = new ItemManager();

	label_loading = new WidgetLabel();

	// Confirmation box to confirm deleting
	confirm = new MenuConfirm(msg->get("Erase"), msg->get("Delete this avatar?"));
	button_exit = new WidgetButton();
	button_exit->label = msg->get("<- Title");
	button_exit->setBasePos(0, 0, ALIGN_BOTTOM);
	button_exit->refresh();

	button_new = new WidgetButton();
	button_new->label = msg->get("New Avatar");
	button_new->enabled = true;

	button_load = new WidgetButton();
	button_load->label = msg->get("Select Avatar");
	button_load->enabled = false;

	button_delete = new WidgetButton();
	button_delete->label = msg->get("Purge Avatar");
	button_delete->enabled = false;

	scrollbar = new WidgetScrollBar();

	// Set up tab list
	tablist = TabList(HORIZONTAL);
	tablist.add(button_exit);
	tablist.add(button_new);

	// Read positions from config file
	FileParser infile;

	// @CLASS GameStateLoad|Description of menus/gameload.txt
	if (infile.open("menus/gameload.txt")) {
		while (infile.next()) {
			// @ATTR button_new|point|Position of the "New Game" button.
			if (infile.key == "button_new") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				button_new->setBasePos(x, y);
			}
			// @ATTR button_load|point|Position of the "Load Game" button.
			else if (infile.key == "button_load") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				button_load->setBasePos(x, y);
			}
			// @ATTR button_delete|point|Position of the "Delete Save" button.
			else if (infile.key == "button_delete") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				button_delete->setBasePos(x, y);
			}
			// @ATTR portrait|rectangle|Position and dimensions of the portrait image.
			else if (infile.key == "portrait") {
				portrait_dest = toRect(infile.val);
			}
			// @ATTR gameslot|rectangle|Position and dimensions of the first game slot.
			else if (infile.key == "gameslot") {
				gameslot_pos = toRect(infile.val);
			}
			// @ATTR name|label|The label for the hero's name. Position is relative to game slot position.
			else if (infile.key == "name") {
				name_pos = eatLabelInfo(infile.val);
			}
			// @ATTR level|label|The label for the hero's level. Position is relative to game slot position.
			else if (infile.key == "level") {
				level_pos = eatLabelInfo(infile.val);
			}
			// @ATTR class|label|The label for the hero's class. Position is relative to game slot position.
			else if (infile.key == "class") {
				class_pos = eatLabelInfo(infile.val);
			}
			// @ATTR map|label|The label for the hero's current location. Position is relative to game slot position.
			else if (infile.key == "map") {
				map_pos = eatLabelInfo(infile.val);
			}
			// @ATTR loading_label|label|The label for the "Entering game world..."/"Loading saved game..." text.
			else if (infile.key == "loading_label") {
				loading_pos = eatLabelInfo(infile.val);
			}
			// @ATTR sprite|point|Position for the avatar preview image in each slot
			else if (infile.key == "sprite") {
				sprites_pos = toPoint(infile.val);
			}
			// @ATTR visible_slots|int|The maximum numbers of visible save slots.
			else if (infile.key == "visible_slots") {
				game_slot_max = toInt(infile.val);

				// can't have less than 1 game slot visible
				game_slot_max = std::max(game_slot_max, 1);
			}
			// @ATTR text_trim_boundary|int|The position of the right-side boundary where text will be shortened with an ellipsis. Position is relative to game slot position.
			else if (infile.key == "text_trim_boundary") {
				text_trim_boundary = toInt(infile.val);
			}
			else {
				infile.error("GameStateLoad: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
	// prevent text from overflowing on the right edge of game slots
	if (text_trim_boundary == 0 || text_trim_boundary > gameslot_pos.w)
		text_trim_boundary = gameslot_pos.w;
		
	// get displayable types list
	bool found_layer = false;
	if (infile.open("engine/hero_layers.txt")) {
		while(infile.next()) {
			if (infile.key == "layer") {
				unsigned dir = popFirstInt(infile.val);
				if (dir != 6) continue;
				else found_layer = true;

				std::string layer = popFirstString(infile.val);
				while (layer != "") {
					preview_layer.push_back(layer);
					layer = popFirstString(infile.val);
				}
			}
		}
		infile.close();
	}
	if (!found_layer) logError("GameStateLoad: Could not find layers for direction 6");

	button_new->refresh();
	button_load->refresh();
	button_delete->refresh();

	// animation data
	if (infile.open("animations/hero.txt")) {
		while (infile.next()) {
			if (infile.section == "stance") {
				if (infile.key == "frames") {
					stance_frames = toInt(infile.val);
				}
				else if (infile.key == "duration") {
					stance_duration = parse_duration(infile.val);
				}
				else if (infile.key == "type") {
					if (infile.val == "play_once")
						stance_type = PLAY_ONCE;
					else if (infile.val == "looped")
						stance_type = LOOPED;
					else if (infile.val == "back_forth")
						stance_type = BACK_FORTH;
				}
				else {
					infile.error("GameStateLoad: '%s' is not a valid key.", infile.key.c_str());
				}
			}
		}
		infile.close();
	}

	stance_frames = std::max(1, stance_frames);
	stance_ticks_per_frame = std::max(1, (stance_duration / stance_frames));

	loadGraphics();
	readGameSlots();

	color_normal = font->getColor("menu_normal");

	refreshWidgets();
	updateButtons();
}

void GameStateLoad::loadGraphics() {
	Image *graphics;

	graphics = render_device->loadImage("images/menus/game_slots.png");
	if (graphics) {
		background = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/game_slot_select.png",
			   "Couldn't load image", false);
	if (graphics) {
		selection = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/portrait_border.png",
			   "Couldn't load image", false);
	if (graphics) {
		portrait_border = graphics->createSprite();
		graphics->unref();
	}
}

void GameStateLoad::loadPortrait(int slot) {

	Image *graphics;

	if (portrait) {
		delete portrait;
		portrait = NULL;
	}

	if (slot < 0) return;

	if (game_slots[slot]->stats.name == "") return;

	graphics = render_device->loadImage(game_slots[slot]->stats.gfx_portrait);
	if (graphics) {
		portrait = graphics->createSprite();
		portrait->setClipW(portrait_dest.w);
		portrait->setClipH(portrait_dest.h);
		graphics->unref();
	}

}

void GameStateLoad::readGameSlots() {
	FileParser infile;
	std::stringstream filename;
	std::vector<std::string> save_dirs;

	getDirList(PATH_USER + "saves/" + SAVE_PREFIX, save_dirs);
	std::sort(save_dirs.begin(), save_dirs.end(), compareSaveDirs);
	game_slots.resize(save_dirs.size(), NULL);

	visible_slots = (game_slot_max > static_cast<int>(game_slots.size()) ? static_cast<int>(game_slots.size()) : game_slot_max);

	for (size_t i=0; i<save_dirs.size(); ++i){
		// save data is stored in slot#/avatar.txt
		filename.str("");
		filename << PATH_USER << "saves/" << SAVE_PREFIX << "/" << save_dirs[i] << "/avatar.txt";

		if (!infile.open(filename.str(),false)) continue;

		game_slots[i] = new GameSlot();
		game_slots[i]->id = toInt(save_dirs[i]);

		while (infile.next()) {

			// load (key=value) pairs
			if (infile.key == "name") {
				game_slots[i]->stats.name = infile.val;
			}
			else if (infile.key == "class") {
				game_slots[i]->stats.character_class = popFirstString(infile.val);
				game_slots[i]->stats.character_subclass = popFirstString(infile.val);
			}
			else if (infile.key == "soul")  {
				game_slots[i]->stats.soul = toInt(infile.val);
			}
			else if (infile.key == "build") {
				for (size_t j = 0; j < PRIMARY_STATS.size(); ++j) {
					game_slots[i]->stats.primary[j] = popFirstInt(infile.val);
				}
			}
			else if (infile.key == "equipped") {
				std::string repeat_val = popFirstString(infile.val);
				while (repeat_val != "") {
					game_slots[i]->equipped.push_back(toInt(repeat_val));
					repeat_val = popFirstString(infile.val);
				}
			}
			else if (infile.key == "option") {
				game_slots[i]->stats.gfx_base = popFirstString(infile.val);
				game_slots[i]->stats.gfx_head = popFirstString(infile.val);
				game_slots[i]->stats.gfx_portrait = popFirstString(infile.val);
			}
			else if (infile.key == "spawn") {
				game_slots[i]->current_map = getMapName(popFirstString(infile.val));
			}
			else if (infile.key == "permadeath") {
				game_slots[i]->stats.permadeath = toBool(infile.val);
			}
		}
		infile.close();

		game_slots[i]->stats.recalc();
		loadPreview(game_slots[i]);
	}
}

std::string GameStateLoad::getMapName(const std::string& map_filename) {
	FileParser infile;
	if (!infile.open(map_filename, true, "")) return "";
	std::string map_name = "";

	while (map_name == "" && infile.next()) {
		if (infile.key == "title")
			map_name = msg->get(infile.val);
	}

	infile.close();
	return map_name;
}

void GameStateLoad::loadPreview(GameSlot* slot) {
	if (!slot) return;

	Image *graphics;
	std::vector<std::string> img_gfx;

	for (unsigned int i=0; i<slot->sprites.size(); i++) {
		if (slot->sprites[i])
			delete slot->sprites[i];
	}
	slot->sprites.clear();

	// fall back to default if it exists
	for (unsigned int i=0; i<preview_layer.size(); i++) {
		bool exists = fileExists(mods->locate("animations/avatar/" + slot->stats.gfx_base + "/default_" + preview_layer[i] + ".txt"));
		if (exists) {
			img_gfx.push_back("default_" + preview_layer[i]);
		}
		else if (preview_layer[i] == "head") {
			img_gfx.push_back(slot->stats.gfx_head);
		}
		else {
			img_gfx.push_back("");
		}
	}

	for (unsigned int i=0; i<slot->equipped.size(); i++) {
		if (static_cast<unsigned>(slot->equipped[i]) > items->items.size()-1) {
			logError("GameStateLoad: Item in save slot %d with id=%d is out of bounds 1-%d. Your savegame is broken or you might be using an incompatible savegame/mod", slot->id, slot->equipped[i], static_cast<int>(items->items.size())-1);
			continue;
		}

		if (slot->equipped[i] > 0 && !preview_layer.empty() && static_cast<unsigned>(slot->equipped[i]) < items->items.size()) {
			std::vector<std::string>::iterator found = find(preview_layer.begin(), preview_layer.end(), items->items[slot->equipped[i]].type);
			if (found != preview_layer.end())
				img_gfx[distance(preview_layer.begin(), found)] = items->items[slot->equipped[i]].gfx;
		}
	}

	// composite the hero graphic
	slot->sprites.resize(img_gfx.size());
	for (unsigned int i=0; i<img_gfx.size(); i++) {
		if (img_gfx[i] == "")
			continue;

		graphics = render_device->loadImage("images/avatar/" + slot->stats.gfx_base + "/preview/" + img_gfx[i] + ".png");
		slot->sprites[i] = NULL;
		if (graphics) {
			slot->sprites[i] = graphics->createSprite();
			int frame_width = slot->sprites[i]->getGraphicsWidth() / stance_frames;
			slot->sprites[i]->setClip(0, 0, frame_width, slot->sprites[i]->getGraphicsHeight());
			graphics->unref();
		}
	}

}


void GameStateLoad::logic() {

	if (inpt->window_resized)
		refreshWidgets();

	// animate the avatar preview images
	if (stance_type == PLAY_ONCE && frame_ticker < stance_duration) {
		current_frame = frame_ticker / stance_ticks_per_frame;
		frame_ticker++;
	}
	else {
		if (stance_type == LOOPED) {
			if (frame_ticker == stance_duration) frame_ticker = 0;
			current_frame = frame_ticker / stance_ticks_per_frame;
		}
		else if (stance_type == BACK_FORTH) {
			if (frame_ticker == stance_duration*2) frame_ticker = 0;
			if (frame_ticker < stance_duration)
				current_frame = frame_ticker / stance_ticks_per_frame;
			else
				current_frame = ((stance_duration*2) - frame_ticker -1) / stance_ticks_per_frame;
		}
		frame_ticker++;
	}

	current_frame = std::min(current_frame, stance_frames-1);

	if (!confirm->visible) {
		tablist.logic(true);
		if (button_exit->checkClick() || (inpt->pressing[CANCEL] && !inpt->lock[CANCEL])) {
			inpt->lock[CANCEL] = true;
			delete requestedGameState;
			requestedGameState = new GameStateTitle();
		}

		if (loading_requested) {
			loading = true;
			loading_requested = false;
			logicLoading();
		}

		bool outside_scrollbar = true;

		if (button_new->checkClick()) {
			// create a new game
			GameStateNew* newgame = new GameStateNew();
			newgame->game_slot = (game_slots.empty() ? 1 : game_slots.back()->id+1);
			requestedGameState = newgame;
			delete_items = false;
		}
		else if (button_load->checkClick()) {
			loading_requested = true;
		}
		else if (button_delete->checkClick()) {
			// Display pop-up to make sure save should be deleted
			confirm->visible = true;
			confirm->render();
		}
		else if (game_slots.size() > 0) {
			Rect scroll_area = slot_pos[0];
			scroll_area.h = slot_pos[0].h * game_slot_max;

			if (isWithinRect(scroll_area, inpt->mouse)) {
				if (inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
					for (int i=0; i<visible_slots; ++i) {
						if (isWithinRect(slot_pos[i], inpt->mouse)) {
							inpt->lock[MAIN1] = true;
							selected_slot = i + scroll_offset;
							updateButtons();
							break;
						}
					}
				}
				else if (inpt->scroll_up) {
					scrollUp();
				}
				else if (inpt->scroll_down) {
					scrollDown();
				}
			}
			else if (has_scroll_bar) {
				switch (scrollbar->checkClick(inpt->mouse.x, inpt->mouse.y)) {
					case 1:
						scrollUp();
						outside_scrollbar = false;
						break;
					case 2:
						scrollDown();
						outside_scrollbar = false;
						break;
					case 3:
						scroll_offset = scrollbar->getValue();
						if (scroll_offset >= static_cast<int>(game_slots.size()) - visible_slots) {
							scroll_offset = static_cast<int>(game_slots.size()) - visible_slots;
						}
						outside_scrollbar = false;
						break;
					default:
						break;
				}
			}

			if (outside_scrollbar && inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
				inpt->lock[MAIN1] = true;
				selected_slot = -1;
				updateButtons();
			}

			// Allow characters to be navigateable via up/down keys
			if (inpt->pressing[UP] && !inpt->lock[UP]) {
				inpt->lock[UP] = true;
				selected_slot = (--selected_slot < 0) ? static_cast<int>(game_slots.size()) - 1 : selected_slot;
				scroll_offset = std::min(static_cast<int>(game_slots.size()) - visible_slots, selected_slot);
				updateButtons();
			}
			else if (inpt->pressing[DOWN] && !inpt->lock[DOWN]) {
				inpt->lock[DOWN] = true;
				selected_slot = (++selected_slot == static_cast<int>(game_slots.size())) ? 0 : selected_slot;
				scroll_offset = std::max(0, selected_slot-visible_slots+1);
				updateButtons();
			}
		}
	}
	else if (confirm->visible) {
		confirm->logic();
		if (confirm->confirmClicked) {
			removeSaveDir(game_slots[selected_slot]->id);

			delete game_slots[selected_slot];
			game_slots[selected_slot] = NULL;
			game_slots.erase(game_slots.begin()+selected_slot);

			visible_slots = (game_slot_max > static_cast<int>(game_slots.size()) ? static_cast<int>(game_slots.size()) : game_slot_max);
			selected_slot = -1;

			while (scroll_offset + visible_slots > static_cast<int>(game_slots.size())) {
				scroll_offset--;
			}

			updateButtons();

			confirm->visible = false;
			confirm->confirmClicked = false;
		}
	}
}

void GameStateLoad::logicLoading() {
	// load an existing game
	inpt->lock_all = true;
	delete_items = false;
	GameStatePlay* play = new GameStatePlay();
	play->resetGame();
	save_load->setGameSlot(game_slots[selected_slot]->id);
	save_load->loadGame();
	requestedGameState = play;
	loaded = true;
	loading = false;
}

void GameStateLoad::updateButtons() {
	loadPortrait(selected_slot);

	// check status of New Game button
	if (!fileExists(mods->locate("maps/spawn.txt"))) {
		button_new->enabled = false;
		tablist.remove(button_new);
		button_new->tooltip = msg->get("Enable a story mod to continue");
	}

	if (selected_slot >= 0) {
		// slot selected: we can load/delete
		if (button_load->enabled == false) {
			button_load->enabled = true;
			tablist.add(button_load);
		}
		button_load->tooltip = "";

		if (button_delete->enabled == false) {
			button_delete->enabled = true;
			tablist.add(button_delete);
		}

		button_load->label = msg->get("Load Avatar");
		if (game_slots[selected_slot]->current_map == "") {
			if (!fileExists(mods->locate("maps/spawn.txt"))) {
				button_load->enabled = false;
				tablist.remove(button_load);
				button_load->tooltip = msg->get("Enable a story mod to continue");
			}
		}
	}
	else {
		// no slot selected: can't load/delete
		button_load->label = msg->get("Choose Avatar");
		button_load->enabled = false;
		tablist.remove(button_load);

		button_delete->enabled = false;
		tablist.remove(button_delete);
	}

	button_new->refresh();
	button_load->refresh();
	button_delete->refresh();

	refreshWidgets();
}

void GameStateLoad::refreshWidgets() {
	button_exit->setPos();
	button_new->setPos((VIEW_W-FRAME_W)/2, (VIEW_H-FRAME_H)/2);
	button_load->setPos((VIEW_W-FRAME_W)/2, (VIEW_H-FRAME_H)/2);
	button_delete->setPos((VIEW_W-FRAME_W)/2, (VIEW_H-FRAME_H)/2);

	label_loading->setPos();

	if (portrait) {
		portrait->setDestX(portrait_dest.x + ((VIEW_W-FRAME_W)/2));
		portrait->setDestY(portrait_dest.y + ((VIEW_H-FRAME_H)/2));
	}

	slot_pos.resize(visible_slots);
	for (size_t i=0; i<slot_pos.size(); i++) {
		slot_pos[i].x = gameslot_pos.x + (VIEW_W - FRAME_W)/2;
		slot_pos[i].h = gameslot_pos.h;
		slot_pos[i].y = gameslot_pos.y + (VIEW_H - FRAME_H)/2 + (static_cast<int>(i) * gameslot_pos.h);
		slot_pos[i].w = gameslot_pos.w;
	}

	refreshScrollBar();
	confirm->align();
}

void GameStateLoad::scrollUp() {
	if (scroll_offset > 0)
		scroll_offset--;

	refreshScrollBar();
}

void GameStateLoad::scrollDown() {
	if (scroll_offset < static_cast<int>(game_slots.size()) - visible_slots)
		scroll_offset++;

	refreshScrollBar();
}

void GameStateLoad::refreshScrollBar() {
	has_scroll_bar = (static_cast<int>(game_slots.size()) > game_slot_max);

	if (has_scroll_bar) {
		Rect scroll_pos;
		scroll_pos.x = slot_pos[0].x + slot_pos[0].w;
		scroll_pos.y = slot_pos[0].y;
		scroll_pos.w = scrollbar->pos_up.w;
		scroll_pos.h = (slot_pos[0].h * game_slot_max) - scrollbar->pos_down.h;
		scrollbar->refresh(scroll_pos.x, scroll_pos.y, scroll_pos.h, scroll_offset, static_cast<int>(game_slots.size()) - visible_slots);
	}
}

void GameStateLoad::render() {

	Rect src;
	Rect dest;


	// portrait
	if (selected_slot >= 0 && portrait != NULL && portrait_border != NULL) {
		render_device->render(portrait);
		dest.x = int(portrait->getDest().x);
		dest.y = int(portrait->getDest().y);
		portrait_border->setDest(dest);
		render_device->render(portrait_border);
	}

	Point label;
	std::stringstream ss;

	if (loading_requested || loading || loaded) {
		label.x = loading_pos.x + (VIEW_W - FRAME_W)/2;
		label.y = loading_pos.y + (VIEW_H - FRAME_H)/2;

		if ( loaded) {
			label_loading->set(msg->get("Deployment established..."));
		}
		else {
			label_loading->set(msg->get("Deploying assets..."));
		}

		label_loading->set(label.x, label.y, loading_pos.justify, loading_pos.valign, label_loading->get(), color_normal, loading_pos.font_style);
		label_loading->render();
	}

	Color color_permadeath_enabled = font->getColor("hardcore_color_name");

	// display text
	for (int slot=0; slot<visible_slots; slot++) {
		int off_slot = slot+scroll_offset;

		// slot background
		if (background) {
			src.x = 0;
			src.y = (off_slot % 4) * gameslot_pos.h;
			dest.w = dest.h = 0;

			src.w = gameslot_pos.w;
			src.h = gameslot_pos.h;
			dest.x = slot_pos[slot].x;
			dest.y = slot_pos[slot].y;

			background->setClip(src);
			background->setDest(dest);
			render_device->render(background);
		}
		Point slot_dest = FPointToPoint(background->getDest());

		Color color_used = game_slots[off_slot]->stats.permadeath ? color_permadeath_enabled : color_normal;

		// name
		label.x = slot_pos[slot].x + name_pos.x;
		label.y = slot_pos[slot].y + name_pos.y;
		game_slots[off_slot]->label_name.set(label.x, label.y, name_pos.justify, name_pos.valign, game_slots[off_slot]->stats.name, color_used, name_pos.font_style);
		if (text_trim_boundary > 0 && game_slots[off_slot]->label_name.bounds.x + game_slots[off_slot]->label_name.bounds.w >= text_trim_boundary + slot_dest.x)
			game_slots[off_slot]->label_name.setMaxWidth(text_trim_boundary - (game_slots[off_slot]->label_name.bounds.x - slot_dest.x));
		game_slots[off_slot]->label_name.render();

		// generation
		ss.str("");
		label.x = slot_pos[slot].x + level_pos.x;
		label.y = slot_pos[slot].y + level_pos.y;
		ss << msg->get("Generation %d", game_slots[off_slot]->stats.generation);
		if (game_slots[off_slot]->stats.permadeath)
			ss << ", " + msg->get("Permadeath");
		game_slots[off_slot]->label_level.set(label.x, label.y, level_pos.justify, level_pos.valign, ss.str(), color_normal, level_pos.font_style);
		if (text_trim_boundary > 0 && game_slots[off_slot]->label_level.bounds.x + game_slots[off_slot]->label_level.bounds.w >= text_trim_boundary + slot_dest.x)
			game_slots[off_slot]->label_level.setMaxWidth(text_trim_boundary - (game_slots[off_slot]->label_level.bounds.x - slot_dest.x));
		game_slots[off_slot]->label_level.render();

		// class
		label.x = slot_pos[slot].x + class_pos.x;
		label.y = slot_pos[slot].y + class_pos.y;
		game_slots[off_slot]->label_class.set(label.x, label.y, class_pos.justify, class_pos.valign, game_slots[off_slot]->stats.getLongClass(), color_normal, class_pos.font_style);
		if (text_trim_boundary > 0 && game_slots[off_slot]->label_class.bounds.x + game_slots[off_slot]->label_class.bounds.w >= text_trim_boundary + slot_dest.x)
			game_slots[off_slot]->label_class.setMaxWidth(text_trim_boundary - (game_slots[off_slot]->label_class.bounds.x - slot_dest.x));
		game_slots[off_slot]->label_class.render();

		// map
		label.x = slot_pos[slot].x + map_pos.x;
		label.y = slot_pos[slot].y + map_pos.y;
		game_slots[off_slot]->label_map.set(label.x, label.y, map_pos.justify, map_pos.valign, game_slots[off_slot]->current_map, color_normal, map_pos.font_style);
		if (text_trim_boundary > 0 && game_slots[off_slot]->label_map.bounds.x + game_slots[off_slot]->label_map.bounds.w >= text_trim_boundary + slot_dest.x)
			game_slots[off_slot]->label_map.setMaxWidth(text_trim_boundary - (game_slots[off_slot]->label_map.bounds.x - slot_dest.x));
		game_slots[off_slot]->label_map.render();

		// render character preview
		dest.x = slot_pos[slot].x + sprites_pos.x;
		dest.y = slot_pos[slot].y + sprites_pos.y;

		for (size_t i=0; i<game_slots[off_slot]->sprites.size(); i++) {
			if (game_slots[off_slot]->sprites[i] == NULL) continue;

			src = game_slots[off_slot]->sprites[i]->getClip();
			src.x = current_frame * src.w;

			game_slots[off_slot]->sprites[i]->setClip(src);
			game_slots[off_slot]->sprites[i]->setDest(dest);
			render_device->render(game_slots[off_slot]->sprites[i]);
		}
	}

	// display selection
	if (selected_slot >= scroll_offset && selected_slot < visible_slots+scroll_offset && selection != NULL) {
		selection->setDest(slot_pos[selected_slot-scroll_offset]);
		render_device->render(selection);
	}

	if (has_scroll_bar)
		scrollbar->render();

	// display warnings
	if (confirm->visible) confirm->render();

	// display buttons
	button_exit->render();
	button_new->render();
	button_load->render();
	button_delete->render();
}

GameStateLoad::~GameStateLoad() {
	if (background)
		delete background;
	if (selection)
		delete selection;
	if (portrait_border)
		delete portrait_border;
	if (portrait)
		delete portrait;

	delete button_exit;
	delete button_new;
	delete button_load;
	delete button_delete;

	if (delete_items) {
		delete items;
		items = NULL;
	}

	for (size_t i = 0; i < game_slots.size(); ++i) {
		delete game_slots[i];
	}
	game_slots.clear();

	delete label_loading;
	delete scrollbar;
	delete confirm;
}
