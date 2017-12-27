/*
Copyright © 2014-2016 Justin Jacobs

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
 * GameStateConfigDesktop
 *
 * Handle game Settings Menu (desktop computer settings)
 */

#include "CommonIncludes.h"
#include "FileParser.h"
#include "GameStateConfigBase.h"
#include "GameStateConfigDesktop.h"
#include "GameStateTitle.h"
#include "MenuConfirm.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Stats.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetCheckBox.h"
#include "WidgetListBox.h"
#include "WidgetScrollBox.h"
#include "WidgetSlider.h"
#include "WidgetTabControl.h"
#include "WidgetTooltip.h"

#include <limits.h>
#include <iomanip>

GameStateConfigDesktop::GameStateConfigDesktop()
	: GameStateConfigBase(false)
	, fullscreen_cb(new WidgetCheckBox())
	, fullscreen_lb(new WidgetLabel())
	, hwsurface_cb(new WidgetCheckBox())
	, hwsurface_lb(new WidgetLabel())
	, vsync_cb(new WidgetCheckBox())
	, vsync_lb(new WidgetLabel())
	, texture_filter_cb(new WidgetCheckBox())
	, texture_filter_lb(new WidgetLabel())
	, change_gamma_cb(new WidgetCheckBox())
	, change_gamma_lb(new WidgetLabel())
	, gamma_sl(new WidgetSlider())
	, gamma_lb(new WidgetLabel())
	, music_volume_sl(new WidgetSlider())
	, music_volume_lb(new WidgetLabel())
	, sound_volume_sl(new WidgetSlider())
	, sound_volume_lb(new WidgetLabel())
	, scaling_sl(new WidgetSlider())
	, scaling_lb(new WidgetLabel())
	, hws_note_lb(new WidgetLabel())
	, dbuf_note_lb(new WidgetLabel())
	, test_note_lb(new WidgetLabel())
	, joystick_device_lstb(new WidgetListBox(10))
	, joystick_device_lb(new WidgetLabel())
	, enable_joystick_cb(new WidgetCheckBox())
	, enable_joystick_lb(new WidgetLabel())
	, mouse_move_cb(new WidgetCheckBox())
	, mouse_move_lb(new WidgetLabel())
	, mouse_aim_cb(new WidgetCheckBox())
	, mouse_aim_lb(new WidgetLabel())
	, no_mouse_cb(new WidgetCheckBox())
	, no_mouse_lb(new WidgetLabel())
	, joystick_deadzone_sl(new WidgetSlider())
	, joystick_deadzone_lb(new WidgetLabel())
	, handheld_note_lb(new WidgetLabel())
	, input_scrollbox(NULL)
	, input_confirm(new MenuConfirm(msg->get("Clear"),msg->get("Assign: ")))
	, input_confirm_ticks(0)
	, input_key(0)
	, key_count(0)
	, scrollpane_contents(0)
{
	// Allocate KeyBindings
	for (int i = 0; i < inpt->key_count; i++) {
		keybinds_lb.push_back(new WidgetLabel());
		keybinds_lb[i]->set(inpt->binding_name[i]);
		keybinds_lb[i]->setJustify(JUSTIFY_RIGHT);
	}
	for (int i = 0; i < inpt->key_count * 3; i++) {
		keybinds_btn.push_back(new WidgetButton());
	}

	key_count = static_cast<unsigned>(keybinds_btn.size()/3);

	init();
}

GameStateConfigDesktop::~GameStateConfigDesktop() {
}

void GameStateConfigDesktop::init() {
	VIDEO_TAB = 0;
	INTERFACE_TAB = 1;
	INPUT_TAB = 2;
	KEYBINDS_TAB = 3;
	MODS_TAB = 4;

	tab_control->setTabTitle(VIDEO_TAB, msg->get("Audio/Video"));
	tab_control->setTabTitle(INTERFACE_TAB, msg->get("Interface"));
	tab_control->setTabTitle(INPUT_TAB, msg->get("Input"));
	tab_control->setTabTitle(KEYBINDS_TAB, msg->get("Keybindings"));
	tab_control->setTabTitle(MODS_TAB, msg->get("Mods"));
	tab_control->updateHeader();

	readConfig();

	// Allocate KeyBindings ScrollBox
	input_scrollbox = new WidgetScrollBox(scrollpane.w, scrollpane.h);
	input_scrollbox->setBasePos(scrollpane.x, scrollpane.y);
	input_scrollbox->bg.r = scrollpane_color.r;
	input_scrollbox->bg.g = scrollpane_color.g;
	input_scrollbox->bg.b = scrollpane_color.b;
	input_scrollbox->transparent = false;
	input_scrollbox->resize(scrollpane.w, scrollpane_contents);

	// Set positions of secondary key bindings
	for (unsigned int i = key_count; i < key_count*2; i++) {
		keybinds_btn[i]->pos.x = keybinds_btn[i-key_count]->pos.x + secondary_offset.x;
		keybinds_btn[i]->pos.y = keybinds_btn[i-key_count]->pos.y + secondary_offset.y;
	}

	// Set positions of joystick bindings
	for (unsigned int i = key_count*2; i < keybinds_btn.size(); i++) {
		keybinds_btn[i]->pos.x = keybinds_btn[i-(key_count*2)]->pos.x + (secondary_offset.x*2);
		keybinds_btn[i]->pos.y = keybinds_btn[i-(key_count*2)]->pos.y + (secondary_offset.y*2);
	}

	addChildWidgets();
	addChildWidgetsDesktop();
	setupTabList();

	refreshWidgets();

	update();
}

void GameStateConfigDesktop::readConfig() {
	FileParser infile;
	if (infile.open("menus/config.txt")) {
		while (infile.next()) {
			int x1 = popFirstInt(infile.val);
			int y1 = popFirstInt(infile.val);
			int x2 = popFirstInt(infile.val);
			int y2 = popFirstInt(infile.val);

			if (parseKeyDesktop(infile, x1, y1, x2, y2))
				continue;
			else if (parseKey(infile, x1, y1, x2, y2))
				continue;
			else {
				infile.error("GameStateConfigDesktop: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
}

bool GameStateConfigDesktop::parseKeyDesktop(FileParser &infile, int &x1, int &y1, int &x2, int &y2) {
	// @CLASS GameStateConfigDesktop|Description of menus/config.txt

	int keybind_num = -1;

	if (infile.key == "listbox_scrollbar_offset") {
		// overrides same key in GameStateConfigBase
		joystick_device_lstb->scrollbar_offset = x1;
		activemods_lstb->scrollbar_offset = x1;
		inactivemods_lstb->scrollbar_offset = x1;
	}
	else if (infile.key == "fullscreen") {
		// @ATTR fullscreen|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Full Screen Mode" checkbox relative to the frame.
		placeLabeledWidget(fullscreen_lb, fullscreen_cb, x1, y1, x2, y2, msg->get("Full Screen Mode"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "mouse_move") {
		// @ATTR mouse_move|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Move hero using mouse" checkbox relative to the frame.
		placeLabeledWidget(mouse_move_lb, mouse_move_cb, x1, y1, x2, y2, msg->get("Move hero using mouse"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "hwsurface") {
		// @ATTR hwsurface|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Hardware surfaces" checkbox relative to the frame.
		placeLabeledWidget(hwsurface_lb, hwsurface_cb, x1, y1, x2, y2, msg->get("Hardware surfaces"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "vsync") {
		// @ATTR vsync|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "V-Sync" checkbox relative to the frame.
		placeLabeledWidget(vsync_lb, vsync_cb, x1, y1, x2, y2, msg->get("V-Sync"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "texture_filter") {
		// @ATTR texture_filter|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Texture Filtering" checkbox relative to the frame.
		placeLabeledWidget(texture_filter_lb, texture_filter_cb, x1, y1, x2, y2, msg->get("Texture Filtering"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "change_gamma") {
		// @ATTR change_gamma|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Allow changing gamma" checkbox relative to the frame.
		placeLabeledWidget(change_gamma_lb, change_gamma_cb, x1, y1, x2, y2, msg->get("Allow changing gamma"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "gamma") {
		// @ATTR gamma|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Gamma" slider relative to the frame.
		placeLabeledWidget(gamma_lb, gamma_sl, x1, y1, x2, y2, msg->get("Gamma"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "music_volume") {
		// @ATTR music_volume|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Music Volume" slider relative to the frame.
		placeLabeledWidget(music_volume_lb, music_volume_sl, x1, y1, x2, y2, msg->get("Music Volume"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "sound_volume") {
		// @ATTR sound_volume|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Sound Volume" slider relative to the frame.
		placeLabeledWidget(sound_volume_lb, sound_volume_sl, x1, y1, x2, y2, msg->get("Sound Volume"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "scaling") {
		// @ATTR scaling|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Scaling" slider relative to the frame.
		placeLabeledWidget(scaling_lb, scaling_sl, x1, y1, x2, y2, msg->get("Scaling"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "hws_note") {
		// @ATTR hws_note|point|Position of the "Disable for performance" label (next to Hardware surfaces) relative to the frame.
		hws_note_lb->setBasePos(x1, y1);
		hws_note_lb->set(msg->get("Disable for performance"));
	}
	else if (infile.key == "dbuf_note") {
		// @ATTR dbuf_note|point|Position of the "Disable for performance" label (next to Double buffering) relative to the frame.
		dbuf_note_lb->setBasePos(x1, y1);
		dbuf_note_lb->set(msg->get("Disable for performance"));
	}
	else if (infile.key == "test_note") {
		// @ATTR test_note|point|Position of the "Experimental" label relative to the frame.
		test_note_lb->setBasePos(x1, y1);
		test_note_lb->set(msg->get("Experimental"));
	}
	else if (infile.key == "enable_joystick") {
		// @ATTR enable_joystick|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Use joystick" checkbox relative to the frame.
		placeLabeledWidget(enable_joystick_lb, enable_joystick_cb, x1, y1, x2, y2, msg->get("Use joystick"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "joystick_device") {
		// @ATTR joystick_device|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Joystick" list box relative to the frame.
		placeLabeledWidget(joystick_device_lb, joystick_device_lstb, x1, y1, x2, y2, msg->get("Joystick"));

		for(int i = 0; i < inpt->getNumJoysticks(); i++) {
			std::string joystick_name = inpt->getJoystickName(i);
			if (joystick_name != "")
				joystick_device_lstb->append(joystick_name, joystick_name);
		}
	}
	else if (infile.key == "mouse_aim") {
		// @ATTR mouse_aim|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Mouse aim" checkbox relative to the frame.
		placeLabeledWidget(mouse_aim_lb, mouse_aim_cb, x1, y1, x2, y2, msg->get("Mouse aim"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "no_mouse") {
		// @ATTR no_mouse|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Do not use mouse" checkbox relative to the frame.
		placeLabeledWidget(no_mouse_lb, no_mouse_cb, x1, y1, x2, y2, msg->get("Do not use mouse"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "joystick_deadzone") {
		// @ATTR joystick_deadzone|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Joystick Deadzone" slider relative to the frame.
		placeLabeledWidget(joystick_deadzone_lb, joystick_deadzone_sl, x1, y1, x2, y2, msg->get("Joystick Deadzone"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "handheld_note") {
		// @ATTR handheld_note|point|Position of the "For handheld devices" label relative to the frame.
		handheld_note_lb->setBasePos(x1, y1);
		handheld_note_lb->set(msg->get("For handheld devices"));
	}
	else if (infile.key == "secondary_offset") {
		// @ATTR secondary_offset|point|Offset of the second (and third) columns of keybinds.
		secondary_offset.x = x1;
		secondary_offset.y = y1;
	}
	else if (infile.key == "keybinds_bg_color") {
		// @ATTR keybinds_bg_color|color|Background color for the keybindings scrollbox.
		scrollpane_color.r = static_cast<Uint8>(x1);
		scrollpane_color.g = static_cast<Uint8>(y1);
		scrollpane_color.b = static_cast<Uint8>(x2);
	}
	else if (infile.key == "scrollpane") {
		// @ATTR scrollpane|rectangle|Position of the keybinding scrollbox relative to the frame.
		scrollpane.x = x1;
		scrollpane.y = y1;
		scrollpane.w = x2;
		scrollpane.h = y2;
	}
	else if (infile.key == "scrollpane_contents") {
		// @ATTR scrollpane_contents|int|The vertical size of the keybinding scrollbox's contents.
		scrollpane_contents = x1;
	}

	// @ATTR cancel|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Cancel" keybind relative to the keybinding scrollbox.
	else if (infile.key == "cancel") keybind_num = CANCEL;
	// @ATTR accept|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Accept" keybind relative to the keybinding scrollbox.
	else if (infile.key == "accept") keybind_num = ACCEPT;
	// @ATTR up|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Up" keybind relative to the keybinding scrollbox.
	else if (infile.key == "up") keybind_num = UP;
	// @ATTR down|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Down" keybind relative to the keybinding scrollbox.
	else if (infile.key == "down") keybind_num = DOWN;
	// @ATTR left|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Left" keybind relative to the keybinding scrollbox.
	else if (infile.key == "left") keybind_num = LEFT;
	// @ATTR right|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Right" keybind relative to the keybinding scrollbox.
	else if (infile.key == "right") keybind_num = RIGHT;
	// @ATTR bar1|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar1" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar1") keybind_num = BAR_1;
	// @ATTR bar2|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar2" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar2") keybind_num = BAR_2;
	// @ATTR bar3|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar3" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar3") keybind_num = BAR_3;
	// @ATTR bar4|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar4" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar4") keybind_num = BAR_4;
	// @ATTR bar5|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar5" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar5") keybind_num = BAR_5;
	// @ATTR bar6|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar6" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar6") keybind_num = BAR_6;
	// @ATTR bar7|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar7" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar7") keybind_num = BAR_7;
	// @ATTR Bar8|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar8" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar8") keybind_num = BAR_8;
	// @ATTR bar9|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar9" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar9") keybind_num = BAR_9;
	// @ATTR bar0|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar0" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar0") keybind_num = BAR_0;
	// @ATTR main1|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Main1" keybind relative to the keybinding scrollbox.
	else if (infile.key == "main1") keybind_num = MAIN1;
	// @ATTR main2|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Main2" keybind relative to the keybinding scrollbox.
	else if (infile.key == "main2") keybind_num = MAIN2;
	// @ATTR character|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Character" keybind relative to the keybinding scrollbox.
	else if (infile.key == "character") keybind_num = CHARACTER;
	// @ATTR inventory|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Inventory" keybind relative to the keybinding scrollbox.
	else if (infile.key == "inventory") keybind_num = INVENTORY;
	// @ATTR powers|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Powers" keybind relative to the keybinding scrollbox.
	else if (infile.key == "powers") keybind_num = POWERS;
	// @ATTR log|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Log" keybind relative to the keybinding scrollbox.
	else if (infile.key == "log") keybind_num = LOG;
	// @ATTR ctrl|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Ctrl" keybind relative to the keybinding scrollbox.
	else if (infile.key == "ctrl") keybind_num = CTRL;
	// @ATTR shift|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Shift" keybind relative to the keybinding scrollbox.
	else if (infile.key == "shift") keybind_num = SHIFT;
	// @ATTR alt|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Alt" keybind relative to the keybinding scrollbox.
	else if (infile.key == "alt") keybind_num = ALT;
	// @ATTR delete|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Delete" keybind relative to the keybinding scrollbox.
	else if (infile.key == "delete") keybind_num = DEL;
	// @ATTR actionbar|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "ActionBar Accept" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar") keybind_num = ACTIONBAR;
	// @ATTR actionbar_back|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "ActionBar Left" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar_back") keybind_num = ACTIONBAR_BACK;
	// @ATTR actionbar_forward|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "ActionBar Right" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar_forward") keybind_num = ACTIONBAR_FORWARD;
	// @ATTR actionbar_use|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "ActionBar Use" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar_use") keybind_num = ACTIONBAR_USE;
	// @ATTR developer_menu|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Developer Menu" keybind relative to the keybinding scrollbox.
	else if (infile.key == "developer_menu") keybind_num = DEVELOPER_MENU;
	
	else return false;

	if (keybind_num > -1 && static_cast<unsigned>(keybind_num) < keybinds_lb.size() && static_cast<unsigned>(keybind_num) < keybinds_btn.size()) {
		//keybindings
		keybinds_lb[keybind_num]->setX(x1);
		keybinds_lb[keybind_num]->setY(y1);
		keybinds_btn[keybind_num]->pos.x = x2;
		keybinds_btn[keybind_num]->pos.y = y2;
	}

	return true;
}

void GameStateConfigDesktop::addChildWidgetsDesktop() {
	addChildWidget(fullscreen_cb, VIDEO_TAB);
	addChildWidget(fullscreen_lb, VIDEO_TAB);
	addChildWidget(hwsurface_cb, VIDEO_TAB);
	addChildWidget(hwsurface_lb, VIDEO_TAB);
	addChildWidget(vsync_cb, VIDEO_TAB);
	addChildWidget(vsync_lb, VIDEO_TAB);
	addChildWidget(texture_filter_cb, VIDEO_TAB);
	addChildWidget(texture_filter_lb, VIDEO_TAB);
	addChildWidget(change_gamma_cb, VIDEO_TAB);
	addChildWidget(change_gamma_lb, VIDEO_TAB);
	addChildWidget(gamma_sl, VIDEO_TAB);
	addChildWidget(gamma_lb, VIDEO_TAB);
	addChildWidget(hws_note_lb, VIDEO_TAB);
	addChildWidget(dbuf_note_lb, VIDEO_TAB);
	addChildWidget(test_note_lb, VIDEO_TAB);
	addChildWidget(music_volume_sl, VIDEO_TAB);
	addChildWidget(music_volume_lb, VIDEO_TAB);
	addChildWidget(sound_volume_sl, VIDEO_TAB);
	addChildWidget(sound_volume_lb, VIDEO_TAB);
	addChildWidget(scaling_sl, VIDEO_TAB);
	addChildWidget(scaling_lb, VIDEO_TAB);

	addChildWidget(mouse_move_cb, INPUT_TAB);
	addChildWidget(mouse_move_lb, INPUT_TAB);
	addChildWidget(enable_joystick_cb, INPUT_TAB);
	addChildWidget(enable_joystick_lb, INPUT_TAB);
	addChildWidget(mouse_aim_cb, INPUT_TAB);
	addChildWidget(mouse_aim_lb, INPUT_TAB);
	addChildWidget(no_mouse_cb, INPUT_TAB);
	addChildWidget(no_mouse_lb, INPUT_TAB);
	addChildWidget(joystick_deadzone_sl, INPUT_TAB);
	addChildWidget(joystick_deadzone_lb, INPUT_TAB);
	addChildWidget(joystick_device_lstb, INPUT_TAB);
	addChildWidget(joystick_device_lb, INPUT_TAB);
	addChildWidget(handheld_note_lb, INPUT_TAB);

	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		input_scrollbox->addChildWidget(keybinds_btn[i]);
	}
}

void GameStateConfigDesktop::setupTabList() {
	tablist.add(tab_control);
	tablist.setPrevTabList(&tablist_main);

	tablist_main.add(ok_button);
	tablist_main.add(defaults_button);
	tablist_main.add(cancel_button);
	tablist_main.setPrevTabList(&tablist);
	tablist_main.setNextTabList(&tablist);
	tablist_main.lock();

	tablist_video.add(fullscreen_cb);
	tablist_video.add(hwsurface_cb);
	tablist_video.add(vsync_cb);
	tablist_video.add(texture_filter_cb);
	tablist_video.add(change_gamma_cb);
	tablist_video.add(gamma_sl);
	tablist_video.add(music_volume_sl);
	tablist_video.add(sound_volume_sl);
	tablist_video.add(scaling_sl);
	tablist_video.setPrevTabList(&tablist);
	tablist_video.setNextTabList(&tablist_main);
	tablist_video.lock();

	tablist_interface.add(combat_text_cb);
	tablist_interface.add(show_fps_cb);
	tablist_interface.add(colorblind_cb);
	tablist_interface.add(hardware_cursor_cb);
	tablist_interface.add(dev_mode_cb);
	tablist_interface.add(loot_tooltips_cb);
	tablist_interface.add(statbar_labels_cb);
	tablist_interface.setPrevTabList(&tablist);
	tablist_interface.setNextTabList(&tablist_main);
	tablist_interface.lock();

	tablist_input.add(enable_joystick_cb);
	tablist_input.add(mouse_move_cb);
	tablist_input.add(mouse_aim_cb);
	tablist_input.add(no_mouse_cb);
	tablist_input.add(joystick_deadzone_sl);
	tablist_input.add(joystick_device_lstb);
	tablist_input.setPrevTabList(&tablist);
	tablist_input.setNextTabList(&tablist_main);
	tablist_input.lock();

	tablist_keybinds.add(input_scrollbox);
	tablist_keybinds.setPrevTabList(&tablist);
	tablist_keybinds.setNextTabList(&tablist_main);
	tablist_keybinds.lock();

	tablist_mods.add(inactivemods_lstb);
	tablist_mods.add(activemods_lstb);
	tablist_mods.add(inactivemods_activate_btn);
	tablist_mods.add(activemods_deactivate_btn);
	tablist_mods.add(activemods_shiftup_btn);
	tablist_mods.add(activemods_shiftdown_btn);
	tablist_mods.setPrevTabList(&tablist);
	tablist_mods.setNextTabList(&tablist_main);
	tablist_mods.lock();
}

void GameStateConfigDesktop::update() {
	GameStateConfigBase::update();

	updateVideo();
	updateInput();
	updateKeybinds();
}

void GameStateConfigDesktop::updateVideo() {
	if (FULLSCREEN) fullscreen_cb->Check();
	else fullscreen_cb->unCheck();
	if (HWSURFACE) hwsurface_cb->Check();
	else hwsurface_cb->unCheck();
	if (VSYNC) vsync_cb->Check();
	else vsync_cb->unCheck();
	if (TEXTURE_FILTER) texture_filter_cb->Check();
	else texture_filter_cb->unCheck();
	if (CHANGE_GAMMA) change_gamma_cb->Check();
	else {
		change_gamma_cb->unCheck();
		GAMMA = 1.0;
		gamma_sl->enabled = false;
	}
	gamma_sl->set(5, 20, static_cast<int>(GAMMA*10.0));
	render_device->setGamma(GAMMA);
	scaling_sl->set(5, 12, static_cast<int>(SCALING*10.0));
	if (AUDIO) {
		music_volume_sl->set(0,128,MUSIC_VOLUME);
		snd->setVolumeMusic(MUSIC_VOLUME);
		sound_volume_sl->set(0,128,SOUND_VOLUME);
		snd->setVolumeSFX(SOUND_VOLUME);
	}
	else {
		music_volume_sl->set(0,128,0);
		sound_volume_sl->set(0,128,0);
	}
}

void GameStateConfigDesktop::updateInput() {
	if (ENABLE_JOYSTICK) enable_joystick_cb->Check();
	else enable_joystick_cb->unCheck();
	if (MOUSE_AIM) mouse_aim_cb->Check();
	else mouse_aim_cb->unCheck();
	if (NO_MOUSE) no_mouse_cb->Check();
	else no_mouse_cb->unCheck();
	if (MOUSE_MOVE) mouse_move_cb->Check();
	else mouse_move_cb->unCheck();

	if (ENABLE_JOYSTICK && inpt->getNumJoysticks() > 0) {
		inpt->initJoystick();
		joystick_device_lstb->select(JOYSTICK_DEVICE);
	}
	joystick_device_lstb->refresh();

	joystick_deadzone_sl->set(0,32768,JOY_DEADZONE);
}

void GameStateConfigDesktop::updateKeybinds() {
	// now do labels for keybinds that are set
	for (unsigned int i = 0; i < key_count; i++) {
		keybinds_btn[i]->label = inpt->getBindingString(i);
		keybinds_btn[i]->refresh();
	}
	for (unsigned int i = key_count; i < key_count*2; i++) {
		keybinds_btn[i]->label = inpt->getBindingString(i-key_count, INPUT_BINDING_ALT);
		keybinds_btn[i]->refresh();
	}
	for (unsigned int i = key_count*2; i < keybinds_btn.size(); i++) {
		keybinds_btn[i]->label = inpt->getBindingString(i-(key_count*2), INPUT_BINDING_JOYSTICK);
		keybinds_btn[i]->refresh();
	}
	input_scrollbox->refresh();
}

void GameStateConfigDesktop::logic() {
	if (inpt->window_resized)
		refreshWidgets();

	if (defaults_confirm->visible) {
		// reset defaults confirmation
		logicDefaults();
		return;
	}
	else if (input_confirm->visible) {
		// assign a keybind
		input_confirm->logic();
		scanKey(input_key);
		input_confirm_ticks--;
		if (input_confirm_ticks == 0) input_confirm->visible = false;
		return;
	}
	else {
		if (!logicMain())
			return;
	}

	// tab contents
	active_tab = tab_control->getActiveTab();

	if (active_tab == VIDEO_TAB) {
		tablist.setNextTabList(&tablist_video);
		logicVideo();
	}
	else if (active_tab == INTERFACE_TAB) {
		tablist.setNextTabList(&tablist_interface);
		logicInterface();
	}
	else if (active_tab == INPUT_TAB) {
		tablist.setNextTabList(&tablist_input);
		logicInput();
	}
	else if (active_tab == KEYBINDS_TAB) {
		tablist.setNextTabList(&tablist_keybinds);
		logicKeybinds();
	}
	else if (active_tab == MODS_TAB) {
		tablist.setNextTabList(&tablist_mods);
		logicMods();
	}
}

bool GameStateConfigDesktop::logicMain() {
	if (GameStateConfigBase::logicMain()) {
		tablist_video.logic();
		tablist_input.logic();
		tablist_keybinds.logic();
		return true;
	}

	return false;
}

void GameStateConfigDesktop::logicVideo() {
	if (fullscreen_cb->checkClick()) {
		if (fullscreen_cb->isChecked()) FULLSCREEN=true;
		else FULLSCREEN=false;
	}
	else if (hwsurface_cb->checkClick()) {
		if (hwsurface_cb->isChecked()) HWSURFACE=true;
		else HWSURFACE=false;
	}
	else if (vsync_cb->checkClick()) {
		if (vsync_cb->isChecked()) VSYNC=true;
		else VSYNC=false;
	}
	else if (texture_filter_cb->checkClick()) {
		if (texture_filter_cb->isChecked()) TEXTURE_FILTER=true;
		else TEXTURE_FILTER=false;
	}
	else if (change_gamma_cb->checkClick()) {
		if (change_gamma_cb->isChecked()) {
			CHANGE_GAMMA = true;
			gamma_sl->enabled = true;
		}
		else {
			CHANGE_GAMMA = false;
			GAMMA = 1.0;
			gamma_sl->enabled = false;
			gamma_sl->set(5, 20, static_cast<int>(GAMMA*10.0));
			render_device->setGamma(GAMMA);
		}
	}
	else if (CHANGE_GAMMA) {
		gamma_sl->enabled = true;
		if (gamma_sl->checkClick()) {
			GAMMA = static_cast<float>(gamma_sl->getValue())*0.1f;
			render_device->setGamma(GAMMA);
		}
	}
	else if (scaling_sl->checkClick()) {
		SCALING = static_cast<float>(scaling_sl->getValue())*0.1f;
	}
	if (AUDIO) {
		if (music_volume_sl->checkClick()) {
			if (MUSIC_VOLUME == 0)
				reload_music = true;
			MUSIC_VOLUME = static_cast<short>(music_volume_sl->getValue());
			snd->setVolumeMusic(MUSIC_VOLUME);
		}
		else if (sound_volume_sl->checkClick()) {
			SOUND_VOLUME = static_cast<short>(sound_volume_sl->getValue());
			snd->setVolumeSFX(SOUND_VOLUME);
		}
	}
}

void GameStateConfigDesktop::logicInput() {
	if (inpt->joysticks_changed) {
		disableJoystickOptions();
		joystick_device_lstb->clear();
		for(int i = 0; i < inpt->getNumJoysticks(); i++) {
			std::string joystick_name = inpt->getJoystickName(i);
			if (joystick_name != "")
				joystick_device_lstb->append(joystick_name, joystick_name);
		}
		inpt->joysticks_changed = false;
	}
	if (mouse_move_cb->checkClick()) {
		if (mouse_move_cb->isChecked()) {
			MOUSE_MOVE=true;
			disableJoystickOptions();
		}
		else MOUSE_MOVE=false;
	}
	else if (mouse_aim_cb->checkClick()) {
		if (mouse_aim_cb->isChecked()) {
			MOUSE_AIM=true;
			disableJoystickOptions();
		}
		else MOUSE_AIM=false;
	}
	else if (no_mouse_cb->checkClick()) {
		if (no_mouse_cb->isChecked()) {
			NO_MOUSE=true;
			disableMouseOptions();
		}
		else NO_MOUSE=false;
	}
	else if (enable_joystick_cb->checkClick()) {
		if (enable_joystick_cb->isChecked()) {
			ENABLE_JOYSTICK=true;
			if (inpt->getNumJoysticks() > 0) {
				JOYSTICK_DEVICE = 0;
				inpt->initJoystick();
				joystick_device_lstb->select(JOYSTICK_DEVICE);
			}
			disableMouseOptions();

			if (inpt->getNumJoysticks() > 0)
				joystick_device_lstb->refresh();
		}
		else {
			disableJoystickOptions();
		}
	}
	else if (joystick_deadzone_sl->checkClick()) {
		JOY_DEADZONE = joystick_deadzone_sl->getValue();
	}
	else if (joystick_device_lstb->checkClick()) {
		JOYSTICK_DEVICE = joystick_device_lstb->getSelected();
		if (JOYSTICK_DEVICE != -1) {
			enable_joystick_cb->Check();
			ENABLE_JOYSTICK=true;
			if (inpt->getNumJoysticks() > 0) {
				inpt->initJoystick();
			}
			disableMouseOptions();
		}
		else {
			enable_joystick_cb->unCheck();
			ENABLE_JOYSTICK = false;
		}
	}
}

void GameStateConfigDesktop::logicKeybinds() {
	input_scrollbox->logic();
	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		Point mouse = input_scrollbox->input_assist(inpt->mouse);
		if (keybinds_btn[i]->checkClick(mouse.x,mouse.y)) {
			std::string confirm_msg;
			confirm_msg = msg->get("Assign: ") + inpt->binding_name[i%key_count];
			delete input_confirm;
			input_confirm = new MenuConfirm(msg->get("Clear"),confirm_msg);
			input_confirm_ticks = MAX_FRAMES_PER_SEC * 10; // 10 seconds
			input_confirm->visible = true;
			input_key = i;
			inpt->last_button = -1;
			inpt->last_key = -1;
			inpt->last_joybutton = -1;
		}
	}
}

void GameStateConfigDesktop::renderTabContents() {
	if (active_tab == KEYBINDS_TAB) {
		if (input_scrollbox->update) {
			input_scrollbox->refresh();
		}
		input_scrollbox->render();
		for (unsigned int i = 0; i < keybinds_lb.size(); i++) {
			keybinds_lb[i]->local_frame = input_scrollbox->pos;
			keybinds_lb[i]->local_offset.y = input_scrollbox->getCursor();
			keybinds_lb[i]->render();
		}
	}

	for (unsigned int i = 0; i < child_widget.size(); i++) {
		if (optiontab[i] == active_tab) child_widget[i]->render();
	}
}

void GameStateConfigDesktop::renderDialogs() {
	GameStateConfigBase::renderDialogs();

	if (input_confirm->visible)
		input_confirm->render();
}

void GameStateConfigDesktop::renderTooltips(TooltipData& tip_new) {
	GameStateConfigBase::renderTooltips(tip_new);

	if (active_tab == INPUT_TAB && tip_new.isEmpty()) tip_new = joystick_device_lstb->checkTooltip(inpt->mouse);
}

void GameStateConfigDesktop::refreshWidgets() {
	GameStateConfigBase::refreshWidgets();

	input_scrollbox->setPos(frame.x, frame.y);

	input_confirm->align();
}

void GameStateConfigDesktop::scanKey(int button) {
	// clear the keybind if the user presses CTRL+Delete
	if (input_confirm->visible && input_confirm->confirmClicked) {
		if (static_cast<unsigned>(button) < key_count) inpt->binding[button] = -1;
		else if (static_cast<unsigned>(button) < key_count*2) inpt->binding_alt[button%key_count] = -1;
		else if (static_cast<unsigned>(button) < key_count*3) inpt->binding_joy[button%key_count] = -1;

		inpt->pressing[button%key_count] = false;
		inpt->lock[button%key_count] = false;

		keybinds_btn[button]->label = msg->get("(none)");
		input_confirm->visible = false;
		input_confirm_ticks = 0;
		keybinds_btn[button]->refresh();
		return;
	}

	if (input_confirm->visible && !input_confirm->isWithinButtons) {
		// keyboard & mouse
		if (static_cast<unsigned>(button) < key_count*2) {
			if (inpt->last_button != -1) {
				if (static_cast<unsigned>(button) < key_count) inpt->binding[button] = inpt->last_button;
				else inpt->binding_alt[button-key_count] = inpt->last_button;

				inpt->pressing[button%key_count] = false;
				inpt->lock[button%key_count] = false;

				if (static_cast<unsigned>(button) < key_count)
					keybinds_btn[button]->label = inpt->getBindingString(button%key_count, INPUT_BINDING_DEFAULT);
				else if (static_cast<unsigned>(button) < key_count*2)
					keybinds_btn[button]->label = inpt->getBindingString(button%key_count, INPUT_BINDING_ALT);

				input_confirm->visible = false;
				input_confirm_ticks = 0;
				keybinds_btn[button]->refresh();
				return;
			}
			if (inpt->last_key != -1) {
				if (static_cast<unsigned>(button) < key_count) inpt->binding[button] = inpt->last_key;
				else inpt->binding_alt[button-key_count] = inpt->last_key;

				inpt->pressing[button%key_count] = false;
				inpt->lock[button%key_count] = false;

				keybinds_btn[button]->label = inpt->getKeyName(inpt->last_key);
				input_confirm->visible = false;
				input_confirm_ticks = 0;
				keybinds_btn[button]->refresh();
				return;
			}
		}
		// joystick
		else if (static_cast<unsigned>(button) >= key_count*2 && inpt->last_joybutton != -1) {
			inpt->binding_joy[button-(key_count*2)] = inpt->last_joybutton;

			keybinds_btn[button]->label = msg->get("Button %d", inpt->binding_joy[button-(key_count*2)]);
			input_confirm->visible = false;
			input_confirm_ticks = 0;
			keybinds_btn[button]->refresh();
		}
		else if (static_cast<unsigned>(button) >= key_count*2 && inpt->last_joyaxis != -1) {
			inpt->binding_joy[button-(key_count*2)] = inpt->last_joyaxis;

			keybinds_btn[button]->label = inpt->getBindingString(button%key_count, INPUT_BINDING_JOYSTICK);
			input_confirm->visible = false;
			input_confirm_ticks = 0;
			keybinds_btn[button]->refresh();
		}
	}
}

void GameStateConfigDesktop::cleanupTabContents() {
	for (std::vector<Widget*>::iterator iter = child_widget.begin(); iter != child_widget.end(); ++iter) {
		if (*iter != NULL) {
			delete (*iter);
			*iter = NULL;
		}
	}
	child_widget.clear();

	for (unsigned int i = 0; i < keybinds_lb.size(); i++) {
		if (keybinds_lb[i] != NULL) {
			delete keybinds_lb[i];
			keybinds_lb[i] = NULL;
		}
	}
	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		if (keybinds_btn[i] != NULL) {
			delete keybinds_btn[i];
			keybinds_btn[i] = NULL;
		}
	}

	if (input_scrollbox != NULL) {
		delete input_scrollbox;
		input_scrollbox = NULL;
	}
}

void GameStateConfigDesktop::cleanupDialogs() {
	if (defaults_confirm != NULL) {
		delete defaults_confirm;
		defaults_confirm = NULL;
	}
	if (input_confirm != NULL) {
		delete input_confirm;
		input_confirm = NULL;
	}
}

void GameStateConfigDesktop::disableMouseOptions() {
	mouse_aim_cb->unCheck();
	MOUSE_AIM=false;
	mouse_move_cb->unCheck();
	MOUSE_MOVE=false;

	no_mouse_cb->Check();
	NO_MOUSE = true;
}

void GameStateConfigDesktop::disableJoystickOptions() {
	enable_joystick_cb->unCheck();
	ENABLE_JOYSTICK=false;

	for (int i=0; i<joystick_device_lstb->getSize(); i++)
		joystick_device_lstb->deselect(i);

	if (inpt->getNumJoysticks() > 0)
		joystick_device_lstb->refresh();

	no_mouse_cb->unCheck();
	NO_MOUSE = false;
}
