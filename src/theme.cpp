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

#include "config.hpp"
#include "font.hpp"
#include "language.hpp"
#include "log.hpp"
#include "sdl_utils.hpp"
#include "theme.hpp"
#include "util.hpp"
#include "wassert.hpp"
#include "serialization/string_utils.hpp"

#include <cstdlib>
#include <sstream>

#define LOG_DP LOG_STREAM(info, display)

namespace {
	const SDL_Rect empty_rect = {0,0,0,0};
	const int XDim = 1024;
	const int YDim = 768;

	const size_t DefaultFontSize = font::SIZE_NORMAL;

	typedef struct { size_t x1,y1,x2,y2; } _rect;
	_rect ref_rect = { 0, 0, 0, 0 };

	size_t compute(std::string expr, size_t ref1, size_t ref2=0 ) {
		size_t ref = 0;
		if (expr[0] == '=') {
		  ref = ref1;
		  expr = expr.substr(1);
		} else if ((expr[0] == '+') || (expr[0] == '-')) {
		  ref = ref2;
		}

		return ref + atoi(expr.c_str());
	}

	_rect read_rect(const config& cfg) {
		_rect rect = { 0, 0, 0, 0 };
		const std::vector<std::string> items = utils::split(cfg["rect"].c_str());
		if(items.size() >= 1)
			rect.x1 = atoi(items[0].c_str());

		if(items.size() >= 2)
			rect.y1 = atoi(items[1].c_str());

		if(items.size() >= 3)
			rect.x2 = atoi(items[2].c_str());

		if(items.size() >= 4)
			rect.y2 = atoi(items[3].c_str());

		return rect;
	}

	SDL_Rect read_sdl_rect(const config& cfg) {
		SDL_Rect sdlrect;
		const _rect rect = read_rect(cfg);
		sdlrect.x = rect.x1;
		sdlrect.y = rect.y1;
		sdlrect.w = rect.x2 - rect.x1;
		sdlrect.h = rect.y2 - rect.y1;

		return sdlrect;
	}

	std::string resolve_rect(const std::string& rect_str) {
		_rect rect = { 0, 0, 0, 0 };
		std::stringstream resolved;
		const std::vector<std::string> items = utils::split(rect_str.c_str());
		if(items.size() >= 1) {
			rect.x1 = compute(items[0], ref_rect.x1, ref_rect.x2);
			resolved << rect.x1;
		}
		if(items.size() >= 2) {
			rect.y1 = compute(items[1], ref_rect.y1, ref_rect.y2);
			resolved << "," << rect.y1;
		}
		if(items.size() >= 3) {
			rect.x2 = compute(items[2], ref_rect.x2, rect.x1);
			resolved << "," << rect.x2;
		}
		if(items.size() >= 4) {
			rect.y2 = compute(items[3], ref_rect.y2, rect.y1);
			resolved << "," << rect.y2;
		}

		// std::cerr << "Rect " << rect_str << "\t: " << resolved.str() << "\n";

		ref_rect = rect;
		return resolved.str();
	}

	config empty_config = config();

	config& find_ref(const std::string& id, config& cfg, bool remove = false) {
		for(config::child_map::const_iterator i = cfg.all_children().begin();
		    i != cfg.all_children().end(); i++) {
			for (config::child_list::const_iterator j = i->second.begin();
			     j != i->second.end(); j++) {
				if ((**j)["id"] == id) {
					//std::cerr << "Found a " << *(*i).first << "\n";
					if (remove) {
						const config* const res = cfg.find_child((*i).first,"id",id);
						const size_t index = std::find((*i).second.begin(), (*i).second.end(),
									       res) - (*i).second.begin();
						cfg.remove_child((*i).first,index);
						return empty_config;
					} else {
						return **j;
					}
				}

				// recursively look in children
				config& c = find_ref(id, **j, remove);
				if (!c["id"].empty()) {
					return c;
				}
			}
		}
		// not found
		return empty_config;
	}

#ifdef DEBUG
	// to be called from gdb
	config& find_ref(const char* id, config& cfg) {
		return find_ref(std::string(id),cfg);
	}
#endif

	void expand_partialresolution(config& dst_cfg, const config& top_cfg)
	{
		std::vector<config> res_cfgs_;
		// resolve all the partialresolutions
		const config::child_list& parts_list = top_cfg.get_children("partialresolution");
		for(config::child_list::const_iterator i = parts_list.begin(); i != parts_list.end(); ++i) {
			// follow the inheritance hierarchy and push all the nodes on the stack
			std::vector<const config*> parent_stack(1, (*i));
			const config* parent;
			const t_string* parent_id = &((**i)["inherits"]);
			while((parent = top_cfg.find_child("resolution", "id", (*parent_id))) == NULL) {
				parent = top_cfg.find_child("partialresolution", "id", (*parent_id));
				if(parent == NULL)
					throw config::error("[partialresolution] refers to non-existant [resolution] " + (*parent_id).str());
				parent_stack.push_back(parent);
				parent_id = &((*parent)["inherits"]);
			}

			// add the parent resolution and apply all the modifications of its children
			res_cfgs_.push_back(*parent);
			while(!parent_stack.empty()) {
				//override attributes
				for(string_map::const_iterator j = parent_stack.back()->values.begin(); j != parent_stack.back()->values.end(); ++j) {
					res_cfgs_.back().values[j->first] = j->second;
				}

				{
					const config::child_list& c = parent_stack.back()->get_children("remove");
					for(config::child_list::const_iterator j = c.begin(); j != c.end(); ++j) {
						find_ref ((**j)["id"], res_cfgs_.back(), true);
					}
				}
				{
					const config::child_list& c = parent_stack.back()->get_children("change");
					for(config::child_list::const_iterator j = c.begin(); j != c.end(); ++j) {
						config& target = find_ref ((**j)["id"], res_cfgs_.back());
						for(string_map::iterator k = (**j).values.begin();
								k != (**j).values.end(); ++k) {
							target.values[k->first] = k->second;
						}
					}
				}
				{
					// cannot add [status] sub-elements, but who cares
					const config* c = parent_stack.back()->child("add");
					if (c != NULL) {
						const config::child_map m = c->all_children();
						for(config::child_map::const_iterator j = m.begin(); j != m.end(); ++j) {
							for(config::child_list::const_iterator k = j->second.begin();
									k != j->second.end(); ++k) {
								res_cfgs_.back().add_child(j->first, **k);
							}
						}
					}
				}
				parent_stack.pop_back();
			}
		}
		// add all the resolutions
		const config::child_list& res_list = top_cfg.get_children("resolution");
		{
			for(config::child_list::const_iterator i = res_list.begin(); i != res_list.end(); ++i) {
				dst_cfg.add_child("resolution", (**i));
			}
		}
		// add all the resolved resolutions
		{
			for(std::vector<config>::const_iterator i = res_cfgs_.begin(); i != res_cfgs_.end(); ++i) {
				dst_cfg.add_child("resolution", (*i));
			}
		}
		return;
	}

	void do_resolve_rects(const config& cfg, config& resolved_config, config* resol_cfg = NULL) {

		// recursively resolve children
		for(config::all_children_iterator i = cfg.ordered_begin(); i != cfg.ordered_end(); ++i) {
			const std::pair<const std::string*,const config*>& value = *i;
			config& childcfg = resolved_config.add_child(*value.first);
			do_resolve_rects(*value.second, childcfg, (*value.first =="resolution") ? &childcfg : resol_cfg);
		}

		// copy all key/values
		for(string_map::const_iterator j = cfg.values.begin(); j != cfg.values.end(); ++j) {
			resolved_config.values[j->first] = j->second;
		}

		// override default reference rect with "ref" parameter if any
		if (!cfg["ref"].empty()) {
			if (resol_cfg == NULL) {
				std::cerr << "Use of ref= outside a [resolution] block\n";
			} else {
				//std::cerr << ">> Looking for " << cfg["ref"] << "\n";
				const config ref = find_ref (cfg["ref"], *resol_cfg);

				if (ref["id"].empty()) {
					std::cerr << "Reference to non-existent rect id \"" << cfg["ref"] << "\"\n";
				} else if (ref["rect"].empty()) {
					std::cerr << "Reference to id \"" << cfg["ref"] <<
						"\" which does not have a \"rect\"\n";
				} else {
					ref_rect = read_rect(ref);
				}
			}
		}
		// resolve the rect value to absolute coordinates
		if (!cfg["rect"].empty()) {
			resolved_config.values["rect"] = resolve_rect(cfg["rect"]);
		}
	}
}

theme::object::object() : loc_(empty_rect), relative_loc_(empty_rect),
                          last_screen_(empty_rect), xanchor_(object::FIXED), yanchor_(object::FIXED)
{}

theme::object::object(const config& cfg)
                   : loc_(read_sdl_rect(cfg)), relative_loc_(empty_rect),
				     last_screen_(empty_rect),
                     xanchor_(read_anchor(cfg["xanchor"])),
					 yanchor_(read_anchor(cfg["yanchor"]))
{}

SDL_Rect& theme::object::location(const SDL_Rect& screen) const
{
	if(last_screen_ == screen)
		return relative_loc_;

	last_screen_ = screen;

	switch(xanchor_) {
	case FIXED:
		relative_loc_.x = loc_.x;
		relative_loc_.w = loc_.w;
		break;
	case TOP_ANCHORED:
		relative_loc_.x = loc_.x;
		relative_loc_.w = screen.w - minimum<size_t>(XDim - loc_.w,screen.w);
		break;
	case BOTTOM_ANCHORED:
		relative_loc_.x = screen.w - minimum<size_t>(XDim - loc_.x,screen.w);
		relative_loc_.w = loc_.w;
		break;
	case PROPORTIONAL:
		relative_loc_.x = (loc_.x*screen.w)/XDim;
		relative_loc_.w = (loc_.w*screen.w)/XDim;
		break;
	default:
		wassert(false);
	}

	switch(yanchor_) {
	case FIXED:
		relative_loc_.y = loc_.y;
		relative_loc_.h = loc_.h;
		break;
	case TOP_ANCHORED:
		relative_loc_.y = loc_.y;
		relative_loc_.h = screen.h - minimum<size_t>(YDim - loc_.h,screen.h);
		break;
	case BOTTOM_ANCHORED:
		relative_loc_.y = screen.h - minimum<size_t>(YDim - loc_.y,screen.h);
		relative_loc_.h = loc_.h;
		break;
	case PROPORTIONAL:
		relative_loc_.y = (loc_.y*screen.h)/YDim;
		relative_loc_.h = (loc_.h*screen.h)/YDim;
		break;
	default:
		wassert(false);
	}

	relative_loc_.x = minimum<int>(relative_loc_.x,screen.w);
	relative_loc_.w = minimum<int>(relative_loc_.w,screen.w - relative_loc_.x);
	relative_loc_.y = minimum<int>(relative_loc_.y,screen.h);
	relative_loc_.h = minimum<int>(relative_loc_.h,screen.h - relative_loc_.y);

	return relative_loc_;
}

const SDL_Rect& theme::object::get_location(void) const
{
	return loc_;
}

theme::object::ANCHORING theme::object::read_anchor(const std::string& str)
{
	static const std::string top_anchor = "top", left_anchor = "left",
	                         bot_anchor = "bottom", right_anchor = "right",
							 fixed_anchor = "fixed", proportional_anchor = "proportional";
	if(str == top_anchor || str == left_anchor)
		return TOP_ANCHORED;
	else if(str == bot_anchor || str == right_anchor)
		return BOTTOM_ANCHORED;
	else if(str == proportional_anchor)
		return PROPORTIONAL;
	else
		return FIXED;
}

theme::label::label()
{}

theme::label::label(const config& cfg)
      : object(cfg), text_(cfg["prefix"].str() + cfg["text"].str() + cfg["postfix"].str()),
	    icon_(cfg["icon"]), font_(atoi(cfg["font_size"].c_str()))
{
	if(font_ == 0)
		font_ = DefaultFontSize;
}

const std::string& theme::label::text() const
{
	return text_;
}

const std::string& theme::label::icon() const
{
	return icon_;
}

bool theme::label::empty() const
{
	return text_.empty() && icon_.empty();
}

size_t theme::label::font_size() const
{
	return font_;
}

theme::status_item::status_item(const config& cfg)
        : object(cfg),
		  prefix_(cfg["prefix"].str() + cfg["prefix_literal"].str()),
		  postfix_(cfg["postfix_literal"].str() + cfg["postfix"].str()),
          font_(atoi(cfg["font_size"].c_str()))
{
	if(font_ == 0)
		font_ = DefaultFontSize;

	const config* const label_child = cfg.child("label");
	if(label_child != NULL) {
		label_ = label(*label_child);
	}
}

const std::string& theme::status_item::prefix() const
{
	return prefix_;
}

const std::string& theme::status_item::postfix() const
{
	return postfix_;
}

const theme::label* theme::status_item::get_label() const
{
	return label_.empty() ? NULL : &label_;
}

size_t theme::status_item::font_size() const
{
	return font_;
}

theme::panel::panel(const config& cfg) : object(cfg), image_(cfg["image"])
{}

const std::string& theme::panel::image() const
{
	return image_;
}

theme::menu::menu() : context_(false)
{}

theme::menu::menu(const config& cfg) : object(cfg), context_(cfg["is_context_menu"] == "true"),
                                       title_(cfg["title"].str() + cfg["title_literal"].str()),
				       image_(cfg["image"]),
				       items_(utils::split(cfg["items"]))
{}

bool theme::menu::is_context() const { return context_; }

const std::string& theme::menu::title() const { return title_; }

const std::string& theme::menu::image() const { return image_; }

const std::vector<std::string>& theme::menu::items() const { return items_; }

theme::theme(const config& cfg, const SDL_Rect& screen) {
	config tmp;
	expand_partialresolution(tmp, cfg);
	do_resolve_rects(tmp, cfg_);
	set_resolution(screen);
}

bool theme::set_resolution(const SDL_Rect& screen)
{
	bool result = false;

	const config::child_list& resolutions = cfg_.get_children("resolution");
	int current_rating = 1000000;
	config::child_list::const_iterator i;
	config::child_list::const_iterator current = resolutions.end();
	for(i = resolutions.begin(); i != resolutions.end(); ++i) {
		const int width = atoi((**i)["width"].c_str());
		const int height = atoi((**i)["height"].c_str());
		LOG_DP << "comparing resolution " << screen.w << "," << screen.h << " to " << width << "," << height << "\n";
		if(screen.w >= width && screen.h >= height) {
			LOG_DP << "loading theme: " << width << "," << height << "\n";
			current = i;
			result = true;
			break;
		}

		const int rating = width*height;
		if(rating < current_rating) {
			current = i;
			current_rating = rating;
		}
	}

	if(current == resolutions.end()) {
		if(!resolutions.empty())
			LOG_STREAM(err, display) << "No valid resolution found\n";
		return false;
	}

	panels_.clear();
	labels_.clear();
	status_.clear();
	menus_.clear();

	const config& cfg = **current;

	const config* const main_map_cfg = cfg.child("main_map");
	if(main_map_cfg != NULL) {
		main_map_ = object(*main_map_cfg);
	} else {
		main_map_ = object();
	}

	const config* const mini_map_cfg = cfg.child("mini_map");
	if(mini_map_cfg != NULL) {
		mini_map_ = object(*mini_map_cfg);
	} else {
		mini_map_ = object();
	}

	const config* const status_cfg = cfg.child("status");
	if(status_cfg != NULL) {
		for(config::child_map::const_iterator i = status_cfg->all_children().begin(); i != status_cfg->all_children().end(); ++i) {
			for(config::child_list::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
				status_.insert(std::pair<std::string,status_item>(i->first,status_item(**j)));
			}
		}
	}

	const config::child_list& panel_list = cfg.get_children("panel");
	for(config::child_list::const_iterator p = panel_list.begin(); p != panel_list.end(); ++p) {
		panels_.push_back(panel(**p));
	}

	const config::child_list& label_list = cfg.get_children("label");
	for(config::child_list::const_iterator lb = label_list.begin(); lb != label_list.end(); ++lb) {
		labels_.push_back(label(**lb));
	}

	const config::child_list& menu_list = cfg.get_children("menu");
	for(config::child_list::const_iterator m = menu_list.begin(); m != menu_list.end(); ++m) {
		const menu new_menu(**m);
		LOG_DP << "adding menu: " << (new_menu.is_context() ? "is context" : "not context") << "\n";
		if(new_menu.is_context())
			context_ = new_menu;
		else
			menus_.push_back(new_menu);

		LOG_DP << "done adding menu...\n";
	}

	return result;
}

const std::vector<theme::panel>& theme::panels() const
{
	return panels_;
}

const std::vector<theme::label>& theme::labels() const
{
	return labels_;
}

const std::vector<theme::menu>& theme::menus() const
{
	return menus_;
}

const theme::menu* theme::context_menu() const
{
	return context_.is_context() ? &context_ : NULL;
}

const theme::status_item* theme::get_status_item(const std::string& key) const
{
	const std::map<std::string,status_item>::const_iterator i = status_.find(key);
	if(i != status_.end())
		return &i->second;
	else
		return NULL;
}

const SDL_Rect& theme::main_map_location(const SDL_Rect& screen) const
{
	return main_map_.location(screen);
}

const SDL_Rect& theme::mini_map_location(const SDL_Rect& screen) const
{
	return mini_map_.location(screen);
}

std::map<std::string, config> theme::known_themes;
void theme::set_known_themes(const config* cfg){
        known_themes.clear();
        if(cfg == NULL)
	       return;
	const config& v = *cfg;
	const config::child_list& known_themes_cfg = v.get_children("theme");

	for(config::child_list::const_iterator thm = known_themes_cfg.begin(); thm != known_themes_cfg.end(); ++thm) {
	       std::string thm_name=(**thm)["name"];
	       if(thm_name!="null" && thm_name!="editor"){
	              known_themes[thm_name]=(**thm);
	       }
	}
}

std::vector<std::string> theme::get_known_themes(){
        std::vector<std::string> names;


        for(std::map<std::string, config>::iterator p_thm=known_themes.begin();p_thm!=known_themes.end();p_thm++){
	  names.push_back(p_thm->first);
	}
        return(names);
}

