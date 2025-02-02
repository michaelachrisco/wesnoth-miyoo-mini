/* $Id$ */
/*
   Copyright (C) 2003-5 by David White <davidnwhite@verizon.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "global.hpp"

#include "display.hpp"
#include "gettext.hpp"
#include "show_dialog.hpp"
#include "video.hpp"
#include "widgets/file_chooser.hpp"

#include <vector>
#include <string>

namespace dialogs
{

int show_file_chooser_dialog(display &disp, std::string &filename,
                             std::string const &title, int xloc, int yloc) {
	const events::event_context dialog_events_context;
	const gui::dialog_manager manager;
	const events::resize_lock prevent_resizing;

	CVideo& screen = disp.video();
	surface const scr = screen.getSurface();

	const int width = 400;
	const int height = 400;
	const int left_padding = 10;
	const int right_padding = 10;
	const int top_padding = 10;
	const int bot_padding = 10;

	// If not both locations were supplied, put the dialog in the middle
	// of the screen.
	if (yloc <= -1 || xloc <= -1) {
		xloc = scr->w / 2 - width / 2;
		yloc = scr->h / 2 - height / 2;
	}
	std::vector<gui::button*> buttons_ptr;
	gui::button ok_button_(disp.video(), _("OK"));
	gui::button cancel_button_(disp.video(), _("Cancel"));
	buttons_ptr.push_back(&ok_button_);
	buttons_ptr.push_back(&cancel_button_);
	surface_restorer restorer;
	gui::draw_dialog(xloc, yloc, width, height, disp.video(), title, NULL, &buttons_ptr, &restorer);

	gui::file_chooser fc(disp, filename);
	fc.set_location(xloc + left_padding, yloc + top_padding);
	fc.set_width(width - left_padding - right_padding);
	fc.set_height(height - top_padding - bot_padding);
	fc.set_dirty(true);

	events::raise_draw_event();
	disp.flip();
	disp.invalidate_all();

	CKey key;
	for (;;) {
		events::pump();
		events::raise_process_event();
		events::raise_draw_event();
		if (fc.choice_made()) {
			filename = fc.get_choice();
			return 0; // We know that the OK button is on index 0.
		}
		if (key[SDLK_ESCAPE]) {
			// Escape quits from the dialog.
			return -1;
		}
		for (std::vector<gui::button*>::iterator button_it = buttons_ptr.begin();
			 button_it != buttons_ptr.end(); button_it++) {
			if ((*button_it)->pressed()) {
				// Return the index of the pressed button.
				filename = fc.get_choice();
				return button_it - buttons_ptr.begin();
			}
		}
		disp.flip();
		SDL_Delay(10);
	}
}
} //end namespace dialogs
