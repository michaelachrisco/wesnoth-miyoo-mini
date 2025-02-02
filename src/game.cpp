/* $Id$ */
/*
   Copyright (C) 2003 by David White <davidnwhite@verizon.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "global.hpp"

#include "SDL.h"

#include "about.hpp"
#include "actions.hpp"
#include "ai_interface.hpp"
#include "config.hpp"
#include "cursor.hpp"
#include "dialogs.hpp"
#include "display.hpp"
#include "events.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "game_config.hpp"
#include "game_errors.hpp"
#include "game_events.hpp"
#include "gamestatus.hpp"
#include "gettext.hpp"
#include "hotkeys.hpp"
#include "intro.hpp"
#include "key.hpp"
#include "language.hpp"
#include "log.hpp"
#include "mapgen.hpp"
#include "multiplayer.hpp"
#include "network.hpp"
#include "pathfind.hpp"
#include "playcampaign.hpp"
#include "preferences.hpp"
#include "publish_campaign.hpp"
#include "replay.hpp"
#include "show_dialog.hpp"
#include "sound.hpp"
#include "statistics.hpp"
#include "team.hpp"
#include "thread.hpp"
#include "titlescreen.hpp"
#include "util.hpp"
#include "unit_types.hpp"
#include "unit.hpp"
#include "video.hpp"
#include "wassert.hpp"
#include "wml_separators.hpp"
#include "serialization/binary_or_text.hpp"
#include "serialization/binary_wml.hpp"
#include "serialization/parser.hpp"
#include "serialization/preprocessor.hpp"
#include "serialization/string_utils.hpp"
#include "widgets/button.hpp"
#include "widgets/menu.hpp"

#include "wesconfig.h"

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

namespace {


bool less_campaigns_rank(const config* a, const config* b) {
	return lexical_cast_default<int>((*a)["rank"],1000) <
	       lexical_cast_default<int>((*b)["rank"],1000);
}

class game_controller
{
public:
	game_controller(int argc, char** argv);

	display& disp();

	bool init_video();
	bool init_config();
	bool init_language();
	bool play_test();
	bool play_multiplayer_mode();

	void reset_game_cfg();

	bool is_loading() const;
	bool load_game();
	void set_tutorial();

	bool new_campaign();
	bool play_multiplayer();
	bool change_language();

	void show_preferences();

	enum RELOAD_GAME_DATA { RELOAD_DATA, NO_RELOAD_DATA };
	void play_game(RELOAD_GAME_DATA reload=RELOAD_DATA);

private:
	game_controller(const game_controller&);
	void operator=(const game_controller&);

	void read_game_cfg(const preproc_map& defines, config& cfg, bool use_cache);
	void refresh_game_cfg(bool reset_translations=false);

	void download_campaigns();
	void upload_campaign(const std::string& campaign, network::connection sock);
	void delete_campaign(const std::string& campaign, network::connection sock);
	void remove_campaign(const std::string& campaign);

	const int argc_;
	int arg_;
	const char* const * const argv_;

	//this should get destroyed *after* the video, since we want to clean up threads
	//after the display disappears.
	const threading::manager thread_manager;

	CVideo video_;

	const font::manager font_manager_;
	const preferences::manager prefs_manager_;
	const sound::manager sound_manager_;
	const image::manager image_manager_;
	const events::event_context main_event_context_;
	const hotkey::manager hotkey_manager_;
	binary_paths_manager paths_manager_;

	bool test_mode_, multiplayer_mode_, no_gui_;
	bool use_caching_;
	int force_bpp_;

	config game_config_;
	game_data units_data_;

	util::scoped_ptr<display> disp_;

	game_state state_;

	std::string loaded_game_;
	bool loaded_game_show_replay_;

	preproc_map defines_map_, old_defines_map_;
};

game_controller::game_controller(int argc, char** argv)
   : argc_(argc), arg_(1), argv_(argv), thread_manager(),
     test_mode_(false), multiplayer_mode_(false),
     no_gui_(false), use_caching_(true), force_bpp_(-1), disp_(NULL),
     loaded_game_show_replay_(false)
{
	for(arg_ = 1; arg_ != argc_; ++arg_) {
		const std::string val(argv_[arg_]);
		if(val.empty()) {
			continue;
		}

		if(val == "--fps") {
			preferences::set_show_fps(true);
		} else if(val == "--nocache") {
			use_caching_ = false;
		} else if(val == "--resolution" || val == "-r") {
			if(arg_+1 != argc_) {
				++arg_;
				const std::string val(argv_[arg_]);
				const std::vector<std::string> res = utils::split(val, 'x');
				if(res.size() == 2) {
					const int xres = lexical_cast_default<int>(res.front());
					const int yres = lexical_cast_default<int>(res.back());
					if(xres > 0 && yres > 0) {
						const std::pair<int,int> resolution(xres,yres);
						preferences::set_resolution(resolution);
					}
				}
			}
		} else if(val == "--bpp") {
			if(arg_+1 != argc_) {
				++arg_;
				force_bpp_ = lexical_cast_default<int>(argv_[arg_],-1);
			}
		} else if(val == "--nogui") {
			no_gui_ = true;
		} else if(val == "--windowed" || val == "-w") {
			preferences::set_fullscreen(false);
		} else if(val == "--fullscreen" || val == "-f") {
			preferences::set_fullscreen(true);
		} else if(val == "--multiplayer") {
			multiplayer_mode_ = true;
			break; //parse the rest of the arguments when we set up the game
		} else if(val == "--test" || val == "-t") {
			test_mode_ = true;
		} else if(val == "--debug" || val == "-d") {
			game_config::debug = true;
		} else if (val.substr(0, 6) == "--log-") {
		} else if(val == "--nosound") {
			preferences::set_sound(false);
			preferences::set_music(false);
		} else if(val[0] == '-') {
			std::cerr << "unknown option: " << val << std::endl;
			throw config::error("unknown option");
		} else {

		  std::cerr << "Setting path using " << val << std::endl;
			if(val[0] == '/') {
				game_config::path = val;
			} else {
				game_config::path = get_cwd() + '/' + val;
			}

			if(!is_directory(game_config::path)) {
				std::cerr << "Could not find directory '" << game_config::path << "'\n";
				throw config::error("directory not found");
			}

		}
	}

	if (preferences::sound_on() || preferences::music_on()) {
		if(!sound::init_sound()) {
			preferences::set_sound(false);
			preferences::set_music(false);
		}
	}
}

display& game_controller::disp()
{
	if(disp_.get() == NULL) {

		if(get_video_surface() == NULL) {
			throw CVideo::error();
		}

		static display::unit_map dummy_umap;
		static config dummy_cfg;
		static gamemap dummy_map(dummy_cfg, "1");
		static gamestatus dummy_status(dummy_cfg, 0);
		static std::vector<team> dummy_teams;
		disp_.assign(new display(dummy_umap, video_, dummy_map, dummy_status,
			dummy_teams, dummy_cfg, dummy_cfg, dummy_cfg));
	}

	return *disp_.get();
}

bool game_controller::init_video()
{
	if(no_gui_) {
		if(!multiplayer_mode_) {
			std::cerr << "--nogui flag is only valid with --multiplayer flag\n";
			return false;
		}
		video_.make_fake();
		return true;
	}

	image::set_wm_icon();

	int video_flags = preferences::fullscreen() ? FULL_SCREEN : 0;

	std::pair<int,int> resolution = preferences::resolution();

	int DefaultBPP = 24;
	const SDL_VideoInfo* const video_info = SDL_GetVideoInfo();
	if(video_info != NULL && video_info->vfmt != NULL) {
		DefaultBPP = video_info->vfmt->BitsPerPixel;
	}

	std::cerr << "Checking video mode: " << resolution.first
		  << "x" << resolution.second << "x" << DefaultBPP << "...\n";
	int bpp = video_.modePossible(resolution.first,resolution.second,DefaultBPP,video_flags);

	std::cerr << bpp << "\n";

	if(bpp == 0) {
		//Video mode not supported, maybe from bad prefs.
		std::cerr << "Video mode " << resolution.first
		          << "x" << resolution.second << "x" << DefaultBPP << " "
		          << "is not supported - attempting 1024x768x" << DefaultBPP << "...\n";

		// Miyoo doesn't support 1024x768 - Force 640x480 (much to the disagreement of the UI)
		resolution.first = 640;
		resolution.second = 480;

		bpp = video_.modePossible(resolution.first,resolution.second,DefaultBPP,video_flags);

		if(bpp == 0) {
			std::cerr << "1024x768x" << DefaultBPP << " not available - attempting 800x600x" << DefaultBPP << "...\n";

			resolution.first = 800;
			resolution.second = 600;

			bpp = video_.modePossible(resolution.first,resolution.second,DefaultBPP,video_flags);
		}

#ifdef USE_TINY_GUI // don't use this, it causes problems.
		if(bpp == 0) {
			std::cerr << "800x600x" << DefaultBPP << " not available - attempting 640x480x" << DefaultBPP << "...\n";

			resolution.first = 640;
			resolution.second = 480;

			bpp = video_.modePossible(resolution.first,resolution.second,DefaultBPP,video_flags);
		}

		if(bpp == 0) {
			std::cerr << "640x480x" << DefaultBPP << " not available - attempting 320x240x" << DefaultBPP << "...\n";

			resolution.first = 320;
			resolution.second = 240;

			bpp = video_.modePossible(resolution.first,resolution.second,DefaultBPP,video_flags);
		}
#endif

		if(bpp == 0) {
			//couldn't do 1024x768 or 800x600

			std::cerr << "The required video mode, " << resolution.first
			          << "x" << resolution.second << "x" << DefaultBPP << " "
			          << "is not supported\n";

			if((video_flags&FULL_SCREEN) != 0) {
				std::cerr << "Try running the program with the --windowed option "
				          << "using a " << DefaultBPP << "bpp X windows setting\n";
			}

			if((video_flags&FULL_SCREEN) == 0) {
				std::cerr << "Try running with the --fullscreen option\n";
			}

			return false;
		}
	}

	if(force_bpp_ > 0) {
		bpp = force_bpp_;
	}

	std::cerr << "setting mode to " << resolution.first << "x" << resolution.second << "x" << bpp << "\n";
	const int res = video_.setMode(resolution.first,resolution.second,bpp,video_flags);
	video_.setBpp(bpp);
	if(res == 0) {
		std::cerr << "required video mode, " << resolution.first << "x"
		          << resolution.second << "x" << bpp << " is not supported\n";
		return false;
	}

	cursor::set(cursor::NORMAL);

	return true;
}

bool game_controller::init_config()
{
	units_data_.clear();
	//Resets old_defines_map_, to force refresh_game_cfg to reload
	//everything.
	old_defines_map_.clear();

	reset_game_cfg();

	game_config::load_config(game_config_.child("game_config"));

	hotkey::load_hotkeys(game_config_);
	paths_manager_.set_paths(game_config_);
	::init_textdomains(game_config_);

	return true;
}

bool game_controller::init_language()
{
	const bool lang_res = ::set_language(get_locale());
	if(!lang_res) {
		std::cerr << "No translation for locale '" << get_locale().language
		          << "', default to system locale\n";

		const bool lang_res = ::set_language(known_languages[0]);
		if(!lang_res) {
			std::cerr << "Language data not found\n";
		}
	}

	if(!no_gui_) {
		SDL_WM_SetCaption(_("The Battle for Wesnoth"), NULL);
	}

	hotkey::load_descriptions();

	return true;
}

bool game_controller::play_test()
{
	static bool first_time = true;

	if(test_mode_ == false) {
		return true;
	}
	if(!first_time)
		return false;

	first_time = false;

	state_.campaign_type = "test";
	state_.scenario = "test";

	try {
		::play_game(disp(),state_,game_config_,units_data_,video_);
	} catch(game::load_game_exception& e) {
		loaded_game_ = e.game;
		loaded_game_show_replay_ = e.show_replay;

		return true;
	}

	return false;
}

bool game_controller::play_multiplayer_mode()
{
	state_ = game_state();

	if(!multiplayer_mode_) {
		return true;
	}

	std::string era = "era_default";
	std::string scenario = "multiplayer_test";
	std::map<int,std::string> side_types, side_controllers, side_algorithms;
	std::map<int,string_map> side_parameters;

	size_t sides_counted = 0;

	for(++arg_; arg_ < argc_; ++arg_) {
		const std::string val(argv_[arg_]);
		if(val.empty()) {
			continue;
		}

		std::vector<std::string> name_value = utils::split(val, '=');
		if(name_value.size() > 2) {
			std::cerr << "invalid argument '" << val << "'\n";
			return false;
		} else if(name_value.size() == 2) {
			const std::string name = name_value.front();
			const std::string value = name_value.back();

			const std::string name_head = name.substr(0,name.size()-1);
			const char name_tail = name[name.size()-1];
			const bool last_digit = isdigit(name_tail) ? true:false;
			const size_t side = name_tail - '0';

			if(last_digit && side > sides_counted) {
				std::cerr << "counted sides: " << side << "\n";
				sides_counted = side;
			}

			if(name == "--scenario") {
				scenario = value;
			} else if(name == "--era") {
				era = value;
			} else if(last_digit && name_head == "--controller") {
				side_controllers[side] = value;
			} else if(last_digit && name_head == "--algorithm") {
				side_algorithms[side] = value;
			} else if(last_digit && name_head == "--side") {
				side_types[side] = value;
			} else if(last_digit && name_head == "--parm") {
				const std::vector<std::string> name_value = utils::split(value, ':');
				if(name_value.size() != 2) {
					std::cerr << "argument to '" << name << "' must be in the format name:value\n";
					return false;
				}

				side_parameters[side][name_value.front()] = name_value.back();
			} else {
				std::cerr << "unrecognized option: '" << name << "'\n";
				return false;
			}
		}
	}

	const config* const lvl = game_config_.find_child("multiplayer","id",scenario);
	if(lvl == NULL) {
		std::cerr << "Could not find scenario '" << scenario << "'\n";
		return false;
	}

	state_.campaign_type = "multiplayer";
	state_.scenario = "";
	state_.snapshot = config();

	config level = *lvl;
	std::vector<config*> story;

	const config* const era_cfg = game_config_.find_child("era","id",era);
	if(era_cfg == NULL) {
		std::cerr << "Could not find era '" << era << "'\n";
		return false;
	}

	const config* const side = era_cfg->child("multiplayer_side");
	if(side == NULL) {
		std::cerr << "Could not find multiplayer side\n";
		return false;
	}

	while(level.get_children("side").size() < sides_counted) {
		std::cerr << "now adding side...\n";
		level.add_child("side");
	}

	int side_num = 1;
	for(config::child_itors itors = level.child_range("side"); itors.first != itors.second; ++itors.first, ++side_num) {
		std::map<int,std::string>::const_iterator type = side_types.find(side_num),
		                                          controller = side_controllers.find(side_num),
		                                          algorithm = side_algorithms.find(side_num);

		const config* side = type == side_types.end() ?
			era_cfg->find_child("multiplayer_side", "random_faction", "yes") :
			era_cfg->find_child("multiplayer_side", "id", type->second);

		if (side == NULL) {
			unknown_side_id:
			std::string side_name = (type == side_types.end() ? "default" : type->second);
			std::cerr << "Could not find side '" << side_name << "' for side " << side_num << "\n";
			return false;
		}

		if ((*side)["random_faction"] == "yes") {
			const config::child_list& eras = era_cfg->get_children("multiplayer_side");
			for(int i = 0; i != 100 && (*side)["random_faction"] == "yes"; ++i) {
				side = eras[rand()%eras.size()];
			}
			if ((*side)["random_faction"] == "yes")
				goto unknown_side_id;
		}

		char buf[20];
		snprintf(buf,sizeof(buf),"%d",side_num);
		(*itors.first)->values["side"] = buf;

		(*itors.first)->values["canrecruit"] = "1";

		for(string_map::const_iterator i = side->values.begin(); i != side->values.end(); ++i) {
			(*itors.first)->values[i->first] = i->second;
		}

		if(controller != side_controllers.end()) {
			(*itors.first)->values["controller"] = controller->second;
		}

		if(algorithm != side_algorithms.end()) {
			(*itors.first)->values["ai_algorithm"] = algorithm->second;
		}

		config& ai_params = (*itors.first)->add_child("ai");

		//now add in any arbitrary parameters given to the side
		for(string_map::const_iterator j = side_parameters[side_num].begin(); j != side_parameters[side_num].end(); ++j) {
			(*itors.first)->values[j->first] = j->second;
			ai_params[j->first] = j->second;
		}
	}

	try {
		state_.snapshot = level;
		::play_game(disp(),state_,game_config_,units_data_,video_);
		//play_level(units_data_,game_config_,&level,video_,state_,story);
	} catch(game::error& e) {
		std::cerr << "caught error: '" << e.message << "'\n";
	} catch(game::load_game_exception& e) {
		//the user's trying to load a game, so go into the normal title screen loop and load one
		loaded_game_ = e.game;
		loaded_game_show_replay_ = e.show_replay;
		return true;
	} catch(...) {
		std::cerr << "caught unknown error playing level...\n";
	}

	return false;
}

bool game_controller::is_loading() const
{
	return loaded_game_.empty() == false;
}

bool game_controller::load_game()
{
	state_ = game_state();

	bool show_replay = loaded_game_show_replay_;

	const std::string game = loaded_game_.empty() ? dialogs::load_game_dialog(disp(),game_config_,units_data_,&show_replay) : loaded_game_;

	loaded_game_ = "";

	if(game == "") {
		return false;
	}

	try {
		//to load a save file, we first load the file in, then we re-parse game
		//data with the save's #defines, and then we finally parse the save file,
		//with the game data ready to go.

		config cfg;
		std::string error_log;
		read_save_file(game,cfg,&error_log);
		if(!error_log.empty()) {
			gui::show_error_message(disp(),
					_("Warning: The file you have tried to load is corrupt. Loading anyway.\n") +
					error_log);
		}

		defines_map_.clear();
		defines_map_[cfg["difficulty"]] = preproc_define();

		if(defines_map_.count("NORMAL")) {
			defines_map_["MEDIUM"] = preproc_define();
		}

		const std::string& campaign_define = cfg["campaign_define"];
		if(campaign_define.empty() == false) {
			defines_map_[campaign_define] = preproc_define();
		}

		refresh_game_cfg();

		state_ = read_game(units_data_,&cfg);

		if(state_.version != game_config::version) {
			const int res = gui::show_dialog(disp(),NULL,"",
			                      _("This save is from a different version of the game. Do you want to try to load it?"),
			                      gui::YES_NO);
			if(res == 1) {
				return false;
			}
		}

	} catch(game::error& e) {
		gui::show_error_message(disp(), _("The file you have tried to load is corrupt: '") + e.message + '\'');
		return false;
	} catch(config::error& e) {
		gui::show_error_message(disp(), _("The file you have tried to load is corrupt: '") + e.message + '\'');
		return false;
	} catch(io_exception&) {
		gui::show_error_message(disp(), _("File I/O Error while reading the game"));
		return false;
	}
	recorder = replay(state_.replay_data);
	recorder.start_replay();
	recorder.set_skip(0);

	std::cerr << "has snapshot: " << (state_.snapshot.child("side") ? "yes" : "no") << "\n";

	if(state_.snapshot.child("side") == NULL) {
		// No snapshot; this is a start-of-scenario
		if (show_replay) {
			// There won't be any turns to replay, but the
			// user gets to watch the intro sequence again ...
			std::cerr << "replaying (start of scenario)\n";
		} else {
			std::cerr << "skipping...\n";
			recorder.set_skip(-1);
		}
	} else {
		// We have a snapshot. But does the user want to see a replay?
		if(show_replay) {
			statistics::clear_current_scenario();
			std::cerr << "replaying (snapshot)\n";
		} else {
			std::cerr << "setting replay to end...\n";
			recorder.set_to_end();
			if(!recorder.at_end()) {
				std::cerr << "recorder is not at the end!!!\n";
			}
		}
	}

	if(state_.campaign_type == "tutorial") {
		defines_map_["TUTORIAL"] = preproc_define();
	} else if(state_.campaign_type == "multiplayer") {
		for(config::child_itors sides = state_.snapshot.child_range("side");
		    sides.first != sides.second; ++sides.first) {
			if((**sides.first)["controller"] == "network")
				(**sides.first)["controller"] = "human";
		}
	}

	return true;
}

void game_controller::set_tutorial()
{
	state_ = game_state();
	state_.campaign_type = "tutorial";
	state_.scenario = "tutorial";
	state_.campaign_define = "TUTORIAL";
	defines_map_.clear();
	defines_map_["TUTORIAL"] = preproc_define();
}

bool game_controller::new_campaign()
{
	state_ = game_state();
	state_.campaign_type = "scenario";

	config::child_list campaigns = game_config_.get_children("campaign");
	std::sort(campaigns.begin(),campaigns.end(),less_campaigns_rank);

	std::vector<std::string> campaign_names;
	std::vector<std::pair<std::string,std::string> > campaign_desc;

	for(config::child_list::const_iterator i = campaigns.begin(); i != campaigns.end(); ++i) {
		std::stringstream str;
		const std::string& icon = (**i)["icon"];
		const std::string desc = (**i)["description"];
		const std::string image = (**i)["image"];
		if(icon.empty()) {
			str << COLUMN_SEPARATOR;
		} else {
			str << IMAGE_PREFIX << icon << COLUMN_SEPARATOR;
		}

		str << (**i)["name"];

		campaign_names.push_back(str.str());
		campaign_desc.push_back(std::pair<std::string,std::string>(desc,image));
	}

	campaign_names.push_back(std::string(1, COLUMN_SEPARATOR) + _("Get More Campaigns..."));
	campaign_desc.push_back(std::pair<std::string,std::string>(_("Download more campaigns from an Internet server."),game_config::download_campaign_image));

	int res = 0;

	dialogs::campaign_preview_pane campaign_preview(disp().video(),&campaign_desc);
	std::vector<gui::preview_pane*> preview_panes;
	preview_panes.push_back(&campaign_preview);

	wassert(campaign_names.size() > 0);
	res = gui::show_dialog(disp(),NULL,_("Campaign"),
			_("Choose the campaign you want to play:"),
			gui::OK_CANCEL,&campaign_names,&preview_panes);

	if(res == -1) {
		return false;
	}

	//get more campaigns from server
	if(res == int(campaign_names.size()-1)) {
		download_campaigns();
		return new_campaign();
	}

	const config& campaign = *campaigns[res];

	state_.scenario = campaign["first_scenario"];

	const std::string difficulty_descriptions = campaign["difficulty_descriptions"];
	std::vector<std::string> difficulty_options = utils::split(difficulty_descriptions, ';');

	const std::vector<std::string> difficulties = utils::split(campaign["difficulties"]);

	if(difficulties.empty() == false) {
		if(difficulty_options.size() != difficulties.size()) {
			difficulty_options.resize(difficulties.size());
			std::copy(difficulties.begin(),difficulties.end(),difficulty_options.begin());
		}

		const int res = gui::show_dialog(disp(),NULL,_("Difficulty"),
		                            _("Select difficulty level:"),
		                            gui::OK_CANCEL,&difficulty_options);
		if(res == -1) {
			return false;
		}

		state_.difficulty = difficulties[res];
		defines_map_.clear();
		defines_map_[difficulties[res]] = preproc_define();
	}

	state_.campaign_define = campaign["define"];

	return true;
}

namespace
{

std::string format_file_size(const std::string& size_str)
{
	double size = lexical_cast_default<double>(size_str,0.0);

	const double k = 1024;
	if(size > 0.0) {
		std::string size_postfix = _("B");
		if(size > k) {
			size /= k;
			size_postfix = _("KB");
			if(size > k) {
				size /= k;
				size_postfix = _("MB");
			}
		}

		std::ostringstream stream;
#ifdef _MSC_VER
		//Visual C++ makes 'precision' set the number of decimal places. Other platforms
		//make it set the number of significant figures
		stream.precision(1);
		stream << std::fixed << size << size_postfix;
#else
		if (size < 100) stream.precision(3);
		else size = (int)size;
		stream << size << size_postfix;
#endif
		return stream.str();
	} else {
		return "";
	}
}

}

void game_controller::download_campaigns()
{
	std::string host = preferences::campaign_server();

	const int res = gui::show_dialog(disp(),NULL,_("Connect to Server"),
	        _("You will now connect to a campaign server to download campaigns."),
	        gui::OK_CANCEL,NULL,NULL,_("Server: "),&host);
	if(res != 0) {
		return;
	}

	const std::vector<std::string> items = utils::split(host, ':');
	host = items.front();
	preferences::set_campaign_server(host);

	try {
		const network::manager net_manager;
		const network::connection sock = network::connect(items.front(),lexical_cast_default<int>(items.back(),15002));
		if(!sock) {
			gui::show_error_message(disp(), _("Could not connect to host."));
			return;
		}

		config cfg;
		cfg.add_child("request_campaign_list");
		network::send_data(cfg,sock);

		network::connection res = gui::network_data_dialog(disp(),_("Awaiting response from server"),cfg,sock);
		if(!res) {
			return;
		}

		const config* const error = cfg.child("error");
		if(error != NULL) {
			gui::show_error_message(disp(), (*error)["message"]);
			return;
		}

		const config* const campaigns_cfg = cfg.child("campaigns");
		if(campaigns_cfg == NULL) {
			gui::show_error_message(disp(), _("Error communicating with the server."));
			return;
		}

		std::vector<std::string> campaigns, options;

		std::string sep(1, COLUMN_SEPARATOR);

		std::stringstream heading;
		heading << HEADING_PREFIX << sep << _("Name") << sep << _("Version") << sep
				<< _("Author") << sep << _("Downloads") << sep << _("Size");

		const config::child_list& cmps = campaigns_cfg->get_children("campaign");
		const std::vector<std::string>& publish_options = available_campaigns();

		std::vector<std::string> delete_options;

		std::vector<int> sizes;

		for(config::child_list::const_iterator i = cmps.begin(); i != cmps.end(); ++i) {
			const std::string& name = (**i)["name"];
			campaigns.push_back(name);

			if(std::count(publish_options.begin(),publish_options.end(),name) != 0) {
				delete_options.push_back(name);
			}

			std::string title = (**i)["title"];
			if(title == "") {
				title = name;
				std::replace(title.begin(),title.end(),'_',' ');
			}

			std::string version   = (**i)["version"],
			            author    = (**i)["author"];

			if(title.size() > 20) {
				title.resize(20);
			}

			if(version.size() > 12) {
				version.resize(12);
			}

			if(author.size() > 16) {
				author.resize(16);
			}

			//add negative sizes to reverse the sort order
			sizes.push_back(-atoi((**i)["size"].c_str()));

			const int max_icon_dim = 80;

			//make sure the icon isn't too big
			std::string icon = (**i)["icon"];
			const surface icon_img = image::get_image(icon,image::UNSCALED);
			if(icon_img.null() == false && icon_img->w > max_icon_dim && icon_img->h > max_icon_dim) {
				icon = "";
			}

			options.push_back(IMAGE_PREFIX + icon + COLUMN_SEPARATOR +
			                  title + COLUMN_SEPARATOR +
			                  version + COLUMN_SEPARATOR +
			                  author + COLUMN_SEPARATOR +
			                  (**i)["downloads"].str() + COLUMN_SEPARATOR +
			                  format_file_size((**i)["size"]));
		}

		options.push_back(heading.str());

		for(std::vector<std::string>::const_iterator j = publish_options.begin(); j != publish_options.end(); ++j) {
			options.push_back(sep + _("Publish campaign: ") + *j);
		}

		for(std::vector<std::string>::const_iterator d = delete_options.begin(); d != delete_options.end(); ++d) {
			options.push_back(sep + _("Delete campaign: ") + *d);
		}

		if(campaigns.empty() && publish_options.empty()) {
			gui::show_error_message(disp(), _("There are no campaigns available for download from this server."));
			return;
		}

		gui::menu::basic_sorter sorter;
		sorter.set_alpha_sort(1).set_alpha_sort(2).set_alpha_sort(3).set_numeric_sort(4).set_position_sort(5,sizes);

		const int index = gui::show_dialog(disp(),NULL,_("Get Campaign"),_("Choose the campaign to download."),gui::OK_CANCEL,&options,
			                               NULL,"",NULL,0,NULL,NULL,-1,-1,NULL,NULL,"",&sorter);
		if(index < 0) {
			return;
		}

		if(index >= int(campaigns.size() + publish_options.size())) {
			delete_campaign(delete_options[index - int(campaigns.size() + publish_options.size())],sock);
			return;
		}

		if(index >= int(campaigns.size())) {
			upload_campaign(publish_options[index - int(campaigns.size())],sock);
			return;
		}

		config request;
		request.add_child("request_campaign")["name"] = campaigns[index];
		network::send_data(request,sock);

		res = gui::network_data_dialog(disp(),_("Downloading campaign..."),cfg,sock);
		if(!res) {
			return;
		}

		if(cfg.child("error") != NULL) {
			gui::show_error_message(disp(), (*cfg.child("error"))["message"]);
			return;
		}

		if(!check_names_legal(cfg)) {
			gui::show_error_message(disp(), "The campaign has an invalid file or directory name and can not be installed.");
			return;
		}

		//remove any existing versions of the just downloaded campaign
		//assuming it consists of a dir and a cfg file
		remove_campaign(campaigns[index]);

		//put a break at line below to see that it really works.
		unarchive_campaign(cfg);

		// when using zipios, force a reread zip and directory indices
		if (!filesystem_init()) {
			gui::show_error_message(disp(), _("Cannot rescan the filesystem"));
			return;
		}

		//force a reload of configuration information
		const bool old_cache = use_caching_;
		use_caching_ = false;
		old_defines_map_.clear();
		refresh_game_cfg();
		use_caching_ = old_cache;
		::init_textdomains(game_config_);
		paths_manager_.set_paths(game_config_);

		clear_binary_paths_cache();

		gui::show_dialog(disp(),NULL,_("Campaign Installed"),_("The campaign has been installed."),gui::OK_ONLY);
	} catch(config::error&) {
		gui::show_error_message(disp(), _("Network communication error."));
	} catch(network::error&) {
		gui::show_error_message(disp(), _("Remote host disconnected."));
	} catch(io_exception&) {
		gui::show_error_message(disp(), _("There was a problem creating the files necessary to install this campaign."));
	}
}

void game_controller::upload_campaign(const std::string& campaign, network::connection sock)
{
	config request_terms;
	request_terms.add_child("request_terms");
	network::send_data(request_terms,sock);
	config data;
	sock = network::receive_data(data,sock,5000);
	if(!sock) {
		gui::show_error_message(disp(), _("Connection timed out"));
		return;
	} else if(data.child("error")) {
		gui::show_error_message(disp(), _("The server responded with an error: \"") +
		                        (*data.child("error"))["message"].str() + '"');
		return;
	} else if(data.child("message")) {
		const int res = gui::show_dialog(disp(),NULL,_("Terms"),(*data.child("message"))["message"],gui::OK_CANCEL);
		if(res != 0) {
			return;
		}
	}

	config cfg;
	get_campaign_info(campaign,cfg);

	std::string passphrase = cfg["passphrase"];
	if(passphrase.empty()) {
		passphrase.resize(8);
		for(size_t n = 0; n != 8; ++n) {
			passphrase[n] = 'a' + (rand()%26);
		}
		cfg["passphrase"] = passphrase;
		set_campaign_info(campaign,cfg);
	}

	cfg["name"] = campaign;

	config campaign_data;
	archive_campaign(campaign,campaign_data);

	data.clear();
	data.add_child("upload",cfg).add_child("data",campaign_data);

	std::cerr << "uploading campaign...\n";
	network::send_data(data,sock);

	sock = gui::network_data_dialog(disp(),_("Awaiting response from server"),data,sock);
	if(!sock) {
		gui::show_error_message(disp(), _("Connection timed out"));
	} else if(data.child("error")) {
		gui::show_error_message(disp(), _("The server responded with an error: \"") +
		                        (*data.child("error"))["message"].str() + '"');
	} else if(data.child("message")) {
		gui::show_dialog(disp(),NULL,_("Response"),(*data.child("message"))["message"],gui::OK_ONLY);
	}
}

void game_controller::delete_campaign(const std::string& campaign, network::connection sock)
{
	config cfg;
	get_campaign_info(campaign,cfg);

	config msg;
	msg["name"] = campaign;
	msg["passphrase"] = cfg["passphrase"];

	config data;
	data.add_child("delete",msg);

	network::send_data(data,sock);

	sock = network::receive_data(data,sock,5000);
	if(!sock) {
		gui::show_error_message(disp(), _("Connection timed out"));
	} else if(data.child("error")) {
		gui::show_error_message(disp(), _("The server responded with an error: \"") +
		                        (*data.child("error"))["message"].str() + '"');
	} else if(data.child("message")) {
		gui::show_dialog(disp(),NULL,_("Response"),(*data.child("message"))["message"],gui::OK_ONLY);
	}
}

void game_controller::remove_campaign(const std::string& campaign)
{
	const std::string campaign_dir = get_user_data_dir() + "/data/campaigns/" + campaign;
	delete_directory(campaign_dir);
	delete_directory(campaign_dir + ".cfg");
}

bool game_controller::play_multiplayer()
{
	state_ = game_state();
	state_.campaign_type = "multiplayer";
	state_.campaign_define = "MULTIPLAYER";

	std::vector<std::string> host_or_join;
	std::string const pre = IMAGE_PREFIX + std::string("icons/icon-");
	char const sep1 = COLUMN_SEPARATOR, sep2 = HELP_STRING_SEPARATOR;

	host_or_join.push_back(pre + "server.png"
		+ sep1 + _("Join Official Server")
		+ sep2 + _("Log on to the official Wesnoth multiplayer server"));
	host_or_join.push_back(pre + "serverother.png"
		+ sep1 + _("Join Game")
		+ sep2 + _("Join a server or hosted game"));
	host_or_join.push_back(pre + "hostgame.png"
		+ sep1 + _("Host Networked Game")
		+ sep2 + _("Host a game without using a server"));
	host_or_join.push_back(pre + "hotseat.png"
		+ sep1 + _("Hotseat Game")
		+ sep2 + _("Play a multiplayer game sharing the same machine"));
	host_or_join.push_back(pre + "ai.png"
		+ sep1 + _("Human vs AI")
		+ sep2 + _("Play a game against AI opponents"));

	std::string login = preferences::login();

	int res = gui::show_dialog(disp(), NULL, _("Multiplayer"), "",
	                           gui::OK_CANCEL, &host_or_join, NULL,
	                           _("Login: "), &login);
	if (res < 0)
		return false;

	preferences::set_login(login);

	try {
		defines_map_.clear();
		defines_map_[state_.campaign_define] = preproc_define();
		refresh_game_cfg();

		if(res >= 2) {
			std::vector<std::string> chat;
			config game_data;

			const mp::controller cntr = (res == 2 ?
					mp::CNTR_NETWORK :
					(res == 3 ? mp::CNTR_LOCAL : mp::CNTR_COMPUTER ));
			const bool is_server = res == 2;

			mp::start_server(disp(), game_config_, units_data_, cntr, is_server);

		} else if(res == 0 || res == 1) {
			std::string host;
			if(res == 0) {
				host = preferences::official_network_host();
			}

			mp::start_client(disp(), game_config_, units_data_, host);
		}
	} catch(game::load_game_failed& e) {
		gui::show_error_message(disp(), _("The game could not be loaded: ") + e.message);
	} catch(game::game_error& e) {
		gui::show_error_message(disp(), _("Error while playing the game: ") + e.message);
	} catch(network::error& e) {
		std::cerr << "caught network error...\n";
		if(e.message != "") {
			gui::show_dialog(disp(),NULL,"",e.message,gui::OK_ONLY);
		}
	} catch(config::error& e) {
		std::cerr << "caught config::error...\n";
		if(e.message != "") {
			gui::show_dialog(disp(),NULL,"",e.message,gui::OK_ONLY);
		}
	} catch(gamemap::incorrect_format_exception& e) {
		gui::show_error_message(disp(), std::string(_("The game map could not be loaded: ")) + e.msg_);
	} catch(game::load_game_exception& e) {
		//this will make it so next time through the title screen loop, this game is loaded
		loaded_game_ = e.game;
		loaded_game_show_replay_ = e.show_replay;
	}

	return false;
}

bool game_controller::change_language()
{
	std::vector<language_def> langdefs = get_languages();

	// this only works because get_languages() returns a fresh vector at each calls
	// unless show_gui cleans the "*" flag
	const std::vector<language_def>::iterator current = std::find(langdefs.begin(),langdefs.end(),get_language());
	if(current != langdefs.end()) {
		(*current).language = "*" + (*current).language;
	}

	// prepare a copy with just the labels for the list to be displayed
	std::vector<std::string> langs;
	langs.reserve(langdefs.size());
	std::transform(langdefs.begin(),langdefs.end(),std::back_inserter(langs),languagedef_name);

	const int res = gui::show_dialog(disp(),NULL,_("Language"),
	                         _("Choose your preferred language:"),
	                         gui::OK_CANCEL,&langs);
	if(size_t(res) < langs.size()) {
		::set_language(known_languages[res]);
		preferences::set_language(known_languages[res].localename);

		refresh_game_cfg(true);
	}

	font::load_font_config();
	hotkey::load_descriptions();

	return false;
}

void game_controller::show_preferences()
{
	const preferences::display_manager disp_manager(&disp());
	preferences::show_preferences_dialog(disp(),game_config_);

	disp().redraw_everything();
}

//this function reads the game configuration, searching for valid cached copies first
void game_controller::read_game_cfg(const preproc_map& defines, config& cfg, bool use_cache)
{
	log_scope("read_game_cfg");

	if(defines.size() < 4) {
		bool is_valid = true;
		std::stringstream str;
		str << "-v" << game_config::version;
		for(preproc_map::const_iterator i = defines.begin(); i != defines.end(); ++i) {
			if(i->second.value != "" || i->second.arguments.empty() == false) {
				is_valid = false;
				break;
			}

			str << "-" << i->first;
		}
		//std::string localename = get_locale().localename;
		//str << "-lang_" << (localename.empty() ? "default" : localename);

		if(is_valid) {
			const std::string& cache = get_cache_dir();
			if(cache != "") {
				const std::string fname = cache + "/game.cfg-cache" + str.str();
				const std::string fname_checksum = fname + ".checksum";

				file_tree_checksum dir_checksum;

				if(use_cache) {
					try {
						if(file_exists(fname_checksum)) {
							config checksum_cfg;
							scoped_istream stream = istream_file(fname_checksum);
							read(checksum_cfg, *stream);
							dir_checksum = file_tree_checksum(checksum_cfg);
						}
					} catch(config::error&) {
						std::cerr << "cache checksum is corrupt\n";
					} catch(io_exception&) {
						std::cerr << "error reading cache checksum\n";
					}
				}

				if(use_cache && file_exists(fname) && file_create_time(fname) > data_tree_checksum().modified && dir_checksum == data_tree_checksum()) {
					std::cerr << "found valid cache at '" << fname << "' using it\n";
					log_scope("read cache");
					try {
						scoped_istream stream = istream_file(fname);
						read_compressed(cfg, *stream);
						return;
					} catch(config::error&) {
						std::cerr << "cache is corrupt. Loading from files\n";
					} catch(io_exception&) {
						std::cerr << "error reading cache. Loading from files\n";
					}
				}

				std::cerr << "no valid cache found. Writing cache to '" << fname << "'\n";

				preproc_map defines_map(defines);

				//read the file and then write to the cache
				scoped_istream stream = preprocess_file("data/game.cfg", &defines_map);

				std::string error_log, user_error_log;

				read(cfg, *stream, &error_log);

				//load user campaigns
				const std::string user_campaign_dir = get_user_data_dir() + "/data/campaigns/";
				std::vector<std::string> user_campaigns, error_campaigns;
				get_files_in_dir(user_campaign_dir,&user_campaigns,NULL,ENTIRE_FILE_PATH);
				for(std::vector<std::string>::const_iterator uc = user_campaigns.begin(); uc != user_campaigns.end(); ++uc) {
					static const std::string extension = ".cfg";
					if(uc->size() < extension.size() || std::equal(uc->end() - extension.size(),uc->end(),extension.begin()) == false) {
						continue;
					}

					try {
						preproc_map user_defines_map(defines_map);
						scoped_istream stream = preprocess_file(*uc,&user_defines_map);

						std::string campaign_error_log;

						config user_campaign_cfg;
						read(user_campaign_cfg,*stream,&campaign_error_log);

						if(campaign_error_log.empty()) {
							cfg.append(user_campaign_cfg);
						} else {
							user_error_log += campaign_error_log;
							error_campaigns.push_back(*uc);
						}
					} catch(config::error& err) {
						std::cerr << "error reading user campaign '" << *uc << "'\n";
						error_campaigns.push_back(*uc);

						user_error_log += err.message + "\n";
					} catch(io_exception&) {
						std::cerr << "error reading user campaign '" << *uc << "'\n";
						error_campaigns.push_back(*uc);
					}
				}

				if(error_campaigns.empty() == false) {
					std::stringstream msg;
					msg << _("The following add-on campaign(s) had errors and could not be loaded:");
					for(std::vector<std::string>::const_iterator i = error_campaigns.begin(); i != error_campaigns.end(); ++i) {
						msg << "\n" << *i;
					}

					msg << "\n" << _("ERROR DETAILS:") << "\n" << user_error_log;

					gui::show_error_message(disp(),msg.str());
				}

				cfg.merge_children("units");

				if(!error_log.empty()) {
					gui::show_error_message(disp(),
							_("Warning: Errors occurred while loading game configuration files: '") +
							error_log);

				} else {
					try {
						scoped_ostream cache = ostream_file(fname);
						write_compressed(*cache, cfg);
						config checksum_cfg;
						data_tree_checksum().write(checksum_cfg);
						scoped_ostream checksum = ostream_file(fname_checksum);
						write(*checksum, checksum_cfg);
					} catch(io_exception&) {
						std::cerr << "could not write to cache '" << fname << "'\n";
					}
				}

				return;
			}
		}
	}

	std::cerr << "caching cannot be done. Reading file\n";
	preproc_map defines_map(defines);
	scoped_istream stream = preprocess_file("data/game.cfg", &defines_map);
	read(cfg, *stream);
}

void game_controller::refresh_game_cfg(bool reset_translations)
{
	try {
		if(old_defines_map_.empty() || defines_map_ != old_defines_map_ || reset_translations) {

			units_data_.clear();

			if(!reset_translations) {
				game_config_.clear();
				read_game_cfg(defines_map_, game_config_, use_caching_);
			} else {
				game_config_.reset_translation();
			}

			const config* const units = game_config_.child("units");
			if(units != NULL) {
				const bool allow_advancefrom = defines_map_.find("MULTIPLAYER") == defines_map_.end();
				units_data_.set_config(*units,allow_advancefrom);
			}

			old_defines_map_ = defines_map_;
		}
	} catch(config::error& e) {
		std::cerr << "Error loading game configuration files\n";
		gui::show_error_message(disp(), _("Error loading game configuration files: '") +
		                        e.message + _("' (The game will now exit)"));
		throw e;
	}
}

void game_controller::reset_game_cfg()
{
	defines_map_.clear();

	//load in the game's configuration files
#if defined(__APPLE__)
	defines_map_["APPLE"] = preproc_define();
#endif

	defines_map_["NORMAL"] = preproc_define();
	defines_map_["MEDIUM"] = preproc_define();

	if(multiplayer_mode_) {
		defines_map_["MULTIPLAYER"] = preproc_define();
	}

	refresh_game_cfg();
}

void game_controller::play_game(RELOAD_GAME_DATA reload)
{
	if(reload == RELOAD_DATA) {
		if(state_.campaign_define.empty() == false) {
			defines_map_[state_.campaign_define] = preproc_define();
		}

		if(defines_map_.count("NORMAL")) {
			defines_map_["MEDIUM"] = preproc_define();
		}

		refresh_game_cfg();
	}

	const binary_paths_manager bin_paths_manager(game_config_);

	try {
		const LEVEL_RESULT result = ::play_game(disp(),state_,game_config_,units_data_,video_);
		// don't show The End for multiplayer scenario
		// change this if MP campaigns are implemented
		if(result == VICTORY && (state_.campaign_type.empty() || state_.campaign_type != "multiplayer")) {
			the_end(disp());
			about::show_about(disp());
		}
	} catch(game::load_game_exception& e) {

		//this will make it so next time through the title screen loop, this game is loaded
		loaded_game_ = e.game;
		loaded_game_show_replay_ = e.show_replay;
	}
}

} //end anon namespace

int play_game(int argc, char** argv)
{
	const int start_ticks = SDL_GetTicks();

	//parse arguments that shouldn't require a display device
	int arg;
	for(arg = 1; arg != argc; ++arg) {
		const std::string val(argv[arg]);
		if(val.empty()) {
			continue;
		}

		if(val == "--help" || val == "-h") {
			std::cout << "usage: " << argv[0]
		    << " [options] [data-directory]\n"
			<< "  -d, --debug       Shows debugging information in-game\n"
			<< "  -f, --fullscreen  Runs the game in full-screen\n"
			<< "  -h, --help        Prints this message and exits\n"
			<< "  --path            Prints the name of the game data directory and exits\n"
			<< "  -t, --test        Runs the game in a small example scenario\n"
			<< "  -w, --windowed    Runs the game in windowed mode\n"
			<< "  -v, --version     Prints the game's version number and exits\n"
			<< "  --log-error=\"domain1,domain2,...\", --log-warning=..., --log-info=...\n"
			<< "                    Set the severity level of the debug domains\n"
			<< "                    \"all\" can be used to match any debug domain\n"
			<< "  --nocache         Disables caching of game data\n"
			<< "  --nosound         Disables sounds\n"
			<< "  --compress file1 file2 Compresses the text-WML file file1 into the\n"
			<< "                    binary-WML file file2\n"
			<< "  --decompress file1 file2 Uncompresses the binary-WML file file2 into\n"
			<< "                    the text-WML file file2\n"
			;
			return 0;
		} else if(val == "--version" || val == "-v") {
			std::cout << _("Battle for Wesnoth") << " " << game_config::version
			          << "\n";
			return 0;
		} else if(val == "--path") {
			std::cout <<  game_config::path
			          << "\n";
			return 0;
		} else if (val.substr(0, 6) == "--log-") {
			size_t p = val.find('=');
			if (p == std::string::npos) {
				std::cerr << "unknown option: " << val << '\n';
				return 0;
			}
			std::string s = val.substr(6, p - 6);
			int severity;
			if (s == "error") severity = 0;
			else if (s == "warning") severity = 1;
			else if (s == "info") severity = 2;
			else {
				std::cerr << "unknown debug level: " << s << '\n';
				return 0;
			}
			while (p != std::string::npos) {
				size_t q = val.find(',', p + 1);
				s = val.substr(p + 1, q == std::string::npos ? q : q - (p + 1));
				if (!lg::set_log_domain_severity(s, severity)) {
					std::cerr << "unknown debug domain: " << s << '\n';
					return 0;
				}
				p = q;
			}
		} else if(val == "--compress" || val == "--decompress") {
			if(argc != arg+3) {
				std::cerr << "format of " << val << " command: " << val << " <input file> <output file>\n";
				return 0;
			}

			const std::string input(argv[arg+1]);
			const std::string output(argv[arg+2]);

			scoped_istream stream = istream_file(input);
			if (stream->fail()) {
				std::cerr << "could not read file '" << input << "'\n";
				return 0;
			}

			config cfg;

			const bool compress = val == "--compress";
			try {
				const bool is_compressed = detect_format_and_read(cfg, *stream);
				if(is_compressed && compress) {
					std::cerr << input << " is already compressed\n";
					return 0;
				} else if(!is_compressed && !compress) {
					std::cerr << input << " is already decompressed\n";
					return 0;
				}

				scoped_ostream output_stream = ostream_file(output);
				write_possibly_compressed(*output_stream, cfg, compress);
			} catch(config::error& e) {
				std::cerr << input << " is not a valid Wesnoth file: " << e.message << "\n";
			} catch(io_exception& e) {
				std::cerr << "IO error: " << e.what() << "\n";
			}

			return 0;
		}
	}

	srand(time(NULL));

	game_controller game(argc,argv);

	if (!filesystem_init()) {
		std::cerr << "cannot init filesystem code\n";
		return 1;
	}

	// I would prefer to setup locale first so that early error
	// messages can get localized, but we need the game_controller
	// initialized to have get_intl_dir() to work.  Note: this
	// setlocale() but this does not take GUI language setting
	// into account.
	setlocale(LC_ALL, "C");
	setlocale(LC_MESSAGES, "");
	const std::string& intl_dir = get_intl_dir();
	bindtextdomain (PACKAGE, intl_dir.c_str());
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	bindtextdomain (PACKAGE "-lib", intl_dir.c_str());
	bind_textdomain_codeset (PACKAGE "-lib", "UTF-8");
	textdomain (PACKAGE);

	bool res;

	// do initialize fonts before reading the game config, to have game
	// config error messages displayed. fonts will be re-initialized later
	// when the language is read from the game config.
	res = font::load_font_config();
	if(res == false) {
		std::cerr << "could not initialize fonts\n";
		return 0;
	}

	res = game.init_video();
	if(res == false) {
		std::cerr << "could not initialize display\n";
		return 0;
	}

#ifdef WIN32
	res = game.init_config();
	if(res == false) {
		std::cerr << "could not initialize game config\n";
		return 0;
	}
#endif

	res = game.init_language();
	if(res == false) {
		std::cerr << "could not initialize the language\n";
		return 0;
	}

	res = font::load_font_config();
	if(res == false) {
		std::cerr << "could not re-initialize fonts for the current language\n";
		return 0;
	}

#ifndef WIN32
	// it is better for gettext-native platforms to read the config
	// files after having pre-initialized the language, maybe...
	res = game.init_config();
	if(res == false) {
		std::cerr << "could not initialize game config\n";
		return 0;
	}
#endif

	const cursor::manager cursor_manager;
#if defined(_X11) && !defined(__APPLE__)
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
#endif

	int ntip = -1;
	config tips_of_day;

	for(;;) {
		//make sure the game config is always set to how it should be at the title screen
		game.reset_game_cfg();

		statistics::fresh_stats();

		sound::play_music(game_config::title_music);

		std::cerr << "started music\n";
		std::cerr << (SDL_GetTicks() - start_ticks) << "\n";

		if(game.play_test() == false) {
			return 0;
		}

		if(game.play_multiplayer_mode() == false) {
			return 0;
		}

		recorder.clear();

		std::cerr << "showing title screen...\n";
		std::cerr << (SDL_GetTicks() - start_ticks) << "\n";
		gui::TITLE_RESULT res = game.is_loading() ? gui::LOAD_GAME : gui::TITLE_CONTINUE;

		while(res == gui::TITLE_CONTINUE) {
			res = gui::show_title(game.disp(),tips_of_day,&ntip);
		}

		game_controller::RELOAD_GAME_DATA should_reload = game_controller::RELOAD_DATA;
		std::cerr << "title screen returned result\n";
		if(res == gui::QUIT_GAME) {
			std::cerr << "quitting game...\n";
			return 0;
		} else if(res == gui::LOAD_GAME) {
			if(game.load_game() == false) {
				continue;
			}

			should_reload = game_controller::NO_RELOAD_DATA;
		} else if(res == gui::TUTORIAL) {
			game.set_tutorial();
		} else if(res == gui::NEW_CAMPAIGN) {
			if(game.new_campaign() == false) {
				continue;
			}
		} else if(res == gui::MULTIPLAYER) {
			if(game.play_multiplayer() == false) {
				continue;
			}
		} else if(res == gui::CHANGE_LANGUAGE) {
			if(game.change_language() == false) {
				tips_of_day.clear();
				continue;
			}
			tips_of_day.clear();
		} else if(res == gui::EDIT_PREFERENCES) {
			game.show_preferences();
			continue;
		} else if(res == gui::SHOW_ABOUT) {
			about::show_about(game.disp());
			continue;
		}

		game.play_game(should_reload);
		ntip = -1; // Change tip when a game is played
	}

	return 0;
}

int main(int argc, char** argv)
{
	try {
		std::cerr << "Battle for Wesnoth v" << VERSION << "\n";
		time_t t = time(NULL);
		std::cerr << "Started on " << ctime(&t) << "\n";

		std::cerr << "started game: " << SDL_GetTicks() << "\n";
		const int res = play_game(argc,argv);
		std::cerr << "exiting with code " << res << "\n";
		return res;
	} catch(CVideo::error&) {
		std::cerr << "Could not initialize video. Exiting.\n";
	} catch(font::manager::error&) {
		std::cerr << "Could not initialize fonts. Exiting.\n";
	} catch(config::error& e) {
		std::cerr << e.message << "\n";
	} catch(gui::button::error&) {
		std::cerr << "Could not create button: Image could not be found\n";
	} catch(CVideo::quit&) {
		//just means the game should quit
	} catch(end_level_exception&) {
		std::cerr << "caught end_level_exception (quitting)\n";
	} catch(std::bad_alloc&) {
		std::cerr << "Ran out of memory. Aborted.\n";
	} /*catch(...) {
		std::cerr << "Unhandled exception. Exiting\n";
	}*/

	return 0;
}
