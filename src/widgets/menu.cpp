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

#include "../global.hpp"

#include "menu.hpp"
#include "cursor.hpp"

#include "../font.hpp"
#include "../image.hpp"
#include "../sdl_utils.hpp"
#include "../util.hpp"
#include "../video.hpp"
#include "../wml_separators.hpp"
#include "serialization/string_utils.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

namespace {
const size_t menu_font_size = font::SIZE_NORMAL;
const size_t menu_cell_padding = font::SIZE_NORMAL * 3/5;
}

namespace gui {

menu::basic_sorter::basic_sorter()
{
	set_id_sort(-1);
}

menu::basic_sorter& menu::basic_sorter::set_alpha_sort(int column)
{
	alpha_sort_.insert(column);
	return *this;
}

menu::basic_sorter& menu::basic_sorter::set_numeric_sort(int column)
{
	numeric_sort_.insert(column);
	return *this;
}

menu::basic_sorter& menu::basic_sorter::set_id_sort(int column)
{
	id_sort_.insert(column);
	return *this;
}

menu::basic_sorter& menu::basic_sorter::set_redirect_sort(int column, int to)
{
	if(column != to) {
		redirect_sort_.insert(std::pair<int,int>(column,to));
	}

	return *this;
}

menu::basic_sorter& menu::basic_sorter::set_position_sort(int column, const std::vector<int>& pos)
{
	pos_sort_[column] = pos;
	return *this;
}

bool menu::basic_sorter::column_sortable(int column) const
{
	const std::map<int,int>::const_iterator redirect = redirect_sort_.find(column);
	if(redirect != redirect_sort_.end()) {
		return column_sortable(redirect->second);
	}

	return alpha_sort_.count(column) == 1 || numeric_sort_.count(column) == 1 ||
		   pos_sort_.count(column) == 1 || id_sort_.count(column) == 1;
}

bool menu::basic_sorter::less(int column, const item& row1, const item& row2) const
{
	const std::map<int,int>::const_iterator redirect = redirect_sort_.find(column);
	if(redirect != redirect_sort_.end()) {
		return less(redirect->second,row1,row2);
	}

	if(id_sort_.count(column) == 1) {
		return row1.id < row2.id;
	}

	if(column < 0 || column >= int(row2.fields.size())) {
		return false;
	}

	if(column >= int(row1.fields.size())) {
		return true;
	}

	const std::string& item1 = row1.fields[column];
	const std::string& item2 = row2.fields[column];
	if(alpha_sort_.count(column) == 1) {
		std::string::const_iterator begin1 = item1.begin(), end1 = item1.end(),
		                            begin2 = item2.begin(), end2 = item2.end();
		while(begin1 != end1 && (font::is_format_char(*begin1) || is_wml_separator(*begin1))) {
			++begin1;
		}

		while(begin2 != end2 && (font::is_format_char(*begin2) || is_wml_separator(*begin2))) {
			++begin2;
		}

		return std::lexicographical_compare(begin1,end1,begin2,end2,chars_less_insensitive);
	} else if(numeric_sort_.count(column) == 1) {
		const char* a = item1.c_str();
		const char* b = item2.c_str();

		while(*a != 0 && !isdigit(*a)) {
			++a;
		}

		while(*b != 0 && !isdigit(*b)) {
			++b;
		}

		return atoi(a) > atoi(b);
	}

	const std::map<int,std::vector<int> >::const_iterator itor = pos_sort_.find(column);
	if(itor != pos_sort_.end()) {
		const std::vector<int>& pos = itor->second;
		if(row1.id >= pos.size()) {
			return false;
		}

		if(row2.id >= pos.size()) {
			return true;
		}

		return pos[row1.id] < pos[row2.id];
	}

	return false;
}

menu::menu(CVideo& video, const std::vector<std::string>& items,
           bool click_selects, int max_height, int max_width,
		   const sorter* sorter_obj)
        : scrollarea(video),
          max_height_(max_height), max_width_(max_width), max_items_(-1), item_height_(-1),
		  heading_height_(-1),
	  cur_help_(-1,-1), help_string_(-1),
	  selected_(0), click_selects_(click_selects),
	  previous_button_(true), show_result_(false),
	  double_clicked_(false),
	  num_selects_(true),
	  ignore_next_doubleclick_(false),
	  last_was_doubleclick_(false),
	  sorter_(sorter_obj), sortby_(-1), sortreversed_(false), highlight_heading_(-1)
{
	fill_items(items, true);
}

void menu::fill_items(const std::vector<std::string>& items, bool strip_spaces)
{
	for(std::vector<std::string>::const_iterator itor = items.begin();
	    itor != items.end(); ++itor) {

		if(itor->empty() == false && (*itor)[0] == HEADING_PREFIX) {
			heading_ = utils::quoted_split(itor->substr(1),COLUMN_SEPARATOR, !strip_spaces);
			continue;
		}

		const size_t id = items_.size();
		item_pos_.push_back(id);
		const item new_item(utils::quoted_split(*itor, COLUMN_SEPARATOR, !strip_spaces),id);
		items_.push_back(new_item);

		//make sure there is always at least one item
		if(items_.back().fields.empty()) {
			items_.back().fields.push_back(" ");
		}

		//if the first character in an item is an asterisk,
		//it means this item should be selected by default
		std::string& first_item = items_.back().fields.front();
		if(first_item.empty() == false && first_item[0] == DEFAULT_ITEM) {
			selected_ = id;
			first_item.erase(first_item.begin());
		}
	}

	create_help_strings();

	if(sortby_ >= 0) {
		do_sort();
	}
	update_size();
}

namespace {

class sort_func
{
public:
	sort_func(const menu::sorter& pred, int column) : pred_(&pred), column_(column)
	{}

	bool operator()(const menu::item& a, const menu::item& b) const
	{
		return pred_->less(column_,a,b);
	}

private:
	const menu::sorter* pred_;
	int column_;
};

}

void menu::do_sort()
{
	if(sorter_ == NULL || sorter_->column_sortable(sortby_) == false) {
		return;
	}

	const int selectid = selection();

	std::stable_sort(items_.begin(), items_.end(), sort_func(*sorter_, sortby_));
	if (sortreversed_)
		std::reverse(items_.begin(), items_.end());

	recalculate_pos();

	if(selectid >= 0 && selectid < int(item_pos_.size())) {
		move_selection(selectid,NO_MOVE_VIEWPORT);
	}

	set_dirty();
}

void menu::recalculate_pos()
{
	size_t sz = items_.size();
	item_pos_.resize(sz);
	for(size_t i = 0; i != sz; ++i)
		item_pos_[items_[i].id] = i;
	assert_pos();
}

void menu::assert_pos()
{
	size_t sz = items_.size();
	assert(item_pos_.size() == sz);
	for(size_t n = 0; n != sz; ++n) {
		size_t i = item_pos_[n];
		assert(i < sz && n == items_[i].id);
	}
}

void menu::create_help_strings()
{
	for(std::vector<item>::iterator i = items_.begin(); i != items_.end(); ++i) {
		i->help.clear();
		for(std::vector<std::string>::iterator j = i->fields.begin(); j != i->fields.end(); ++j) {
			if(std::find(j->begin(),j->end(),static_cast<char>(HELP_STRING_SEPARATOR)) == j->end()) {
				i->help.push_back("");
			} else {
				const std::vector<std::string>& items = utils::split(*j, HELP_STRING_SEPARATOR, 0);
				if(items.size() >= 2) {
					*j = items.front();
					i->help.push_back(items.back());
				} else {
					i->help.push_back("");
				}
			}
		}
	}
}

void menu::update_scrollbar_grip_height()
{
	set_full_size(items_.size());
	set_shown_size(max_items_onscreen());
}

void menu::update_size()
{
	unsigned int h = heading_height();
	for(size_t i = get_position(),
	    i_end = minimum(items_.size(), i + max_items_onscreen());
	    i != i_end; ++i)
		h += get_item_rect(i).h;
	h = maximum(h, height());
	if (max_height_ > 0 && h > max_height_)
		h = max_height_;

	std::vector<int> const &widths = column_widths();
	unsigned w = std::accumulate(widths.begin(), widths.end(), 0);
	if (items_.size() > max_items_onscreen())
		w += scrollbar_width();
	w = maximum(w, width());
	if (max_width_ > 0 && w > max_width_)
		w = max_width_;

	update_scrollbar_grip_height();
	set_measurements(w, h);
}

int menu::selection() const
{
	if (selected_ >= items_.size()) {
		return -1;
	}

	return items_[selected_].id;
}

void menu::set_inner_location(SDL_Rect const &rect)
{
	itemRects_.clear();
	update_scrollbar_grip_height();
	bg_register(rect);
}

void menu::change_item(int pos1, int pos2,const std::string& str)
{
	if(pos1 < 0 || pos1 >= int(item_pos_.size()) ||
		pos2 < 0 || pos2 >= int(items_[item_pos_[pos1]].fields.size())) {
		return;
	}

	items_[item_pos_[pos1]].fields[pos2] = str;
	set_dirty();
}

void menu::erase_item(size_t index)
{
	size_t nb_items = items_.size();
	if (index >= nb_items)
		return;
	--nb_items;

	clear_item(nb_items);

	// fix ordered positions of items
	size_t pos = item_pos_[index];
	item_pos_.erase(item_pos_.begin() + index);
	items_.erase(items_.begin() + pos);
	for(size_t i = 0; i != nb_items; ++i) {
		size_t &n1 = item_pos_[i], &n2 = items_[i].id;
		if (n1 > pos) --n1;
		if (n2 > index) --n2;
	}
	assert_pos();

	if (selected_ >= nb_items)
		selected_ = nb_items - 1;

	update_scrollbar_grip_height();
	adjust_viewport_to_selection();
	itemRects_.clear();
	set_dirty();
}

void menu::set_heading(const std::vector<std::string>& heading)
{
	itemRects_.clear();
	column_widths_.clear();

	heading_ = heading;
	max_items_ = -1;

	set_dirty();
}

void menu::set_items(const std::vector<std::string>& items, bool strip_spaces, bool keep_viewport)
{
	items_.clear();
	item_pos_.clear();
	itemRects_.clear();
	column_widths_.clear();
	//undrawn_items_.clear();
	max_items_ = -1; // Force recalculation of the max items.
	item_height_ = -1; // Force recalculation of the item height.

	if (!keep_viewport || selected_ >= items.size()) {
		selected_ = 0;
	}

	fill_items(items, strip_spaces);
	if(!keep_viewport) {
		set_position(0);
	}

	update_scrollbar_grip_height();

	if(!keep_viewport) {
		adjust_viewport_to_selection();
	}
	set_dirty();
}

void menu::set_max_height(const int new_max_height)
{
	max_height_ = new_max_height;
	itemRects_.clear();
	max_items_ = -1;
	update_size();
}

void menu::set_max_width(const int new_max_width)
{
	max_width_ = new_max_width;
}

size_t menu::max_items_onscreen() const
{
	if(max_items_ != -1) {
		return size_t(max_items_);
	}

	const size_t max_height = (max_height_ == -1 ? (video().gety()*66)/100 : max_height_) - heading_height();
	std::vector<int> heights;
	size_t n;
	for(n = 0; n != items_.size(); ++n) {
		heights.push_back(get_item_height(n));
	}

	std::sort(heights.begin(),heights.end(),std::greater<int>());
	size_t sum = 0;
	for(n = 0; n != items_.size() && sum < max_height; ++n) {
		sum += heights[n];
	}

	if(sum > max_height && n > 1)
		--n;

	return max_items_ = n;
}

void menu::adjust_viewport_to_selection()
{
	if(click_selects_)
		return;
	adjust_position(selected_);
}

void menu::move_selection_up(size_t dep)
{
	set_selection_pos(selected_ > dep ? selected_ - dep : 0);
}

void menu::reset_selection()
{
	set_selection_pos(0);
}

void menu::move_selection_down(size_t dep)
{
	size_t nb_items = items_.size();
	set_selection_pos(selected_ + dep >= nb_items ? nb_items - 1 : selected_ + dep);
}

void menu::set_selection_pos(size_t new_selected, SELECTION_MOVE_VIEWPORT move_viewport)
{
	if (new_selected == selected_ || new_selected >= items_.size())
		return;

	invalidate_row_pos(selected_);
	invalidate_row_pos(new_selected);
	selected_ = new_selected;

	if(move_viewport == MOVE_VIEWPORT) {
		adjust_viewport_to_selection();
	}
}

void menu::move_selection(size_t id, SELECTION_MOVE_VIEWPORT move_viewport)
{
	if(id < item_pos_.size()) {
		set_selection_pos(item_pos_[id],move_viewport);
	}
}

void menu::key_press(SDLKey key)
{
	if (!click_selects_) {
		switch(key) {
		case SDLK_UP:
			if(cursor::is_emulated() == false)
				move_selection_up(1);
			break;
		case SDLK_DOWN:
			if(cursor::is_emulated() == false)
				move_selection_down(1);
			break;
		case SDLK_PAGEUP:
			move_selection_up(max_items_onscreen());
			break;
		case SDLK_PAGEDOWN:
			move_selection_down(max_items_onscreen());
			break;
		case SDLK_HOME:
			set_selection_pos(0);
			break;
		case SDLK_END:
			set_selection_pos(items_.size() - 1);
			break;
		//case SDLK_RETURN:
		//	double_clicked_ = true;
		//	break;
		default:
			break;
		}
	}

	if (num_selects_ && key >= SDLK_1 && key <= SDLK_9)
		set_selection_pos(key - SDLK_1);
}

void menu::handle_event(const SDL_Event& event)
{
	scrollarea::handle_event(event);
	if(cursor::is_emulated() == false && event.type == SDL_KEYDOWN) {
		// Only pass key events if we have the focus
		if (focus())
			key_press(event.key.keysym.sym);
	} else if(event.type == SDL_MOUSEBUTTONDOWN &&
	          event.button.button == SDL_BUTTON_LEFT ||
		  event.type == DOUBLE_CLICK_EVENT) {

		int x = 0;
		int y = 0;
		if(event.type == SDL_MOUSEBUTTONDOWN) {
			x = event.button.x;
			y = event.button.y;
		} else {
			x = (long)event.user.data1;
			y = (long)event.user.data2;
		}

		const int item = hit(x,y);
		if(item != -1) {
			set_focus(true);
			move_selection(item);

			if(click_selects_) {
				show_result_ = true;
			}

			if(event.type == DOUBLE_CLICK_EVENT) {
				if (ignore_next_doubleclick_) {
					ignore_next_doubleclick_ = false;
				} else {
					double_clicked_ = true;
					last_was_doubleclick_ = true;
				}
			} else if (last_was_doubleclick_) {
				// If we have a double click as the next event, it means
				// this double click was generated from a click that
				// already has helped in generating a double click.
				SDL_Event ev;
				SDL_PeepEvents(&ev, 1, SDL_PEEKEVENT,
							   SDL_EVENTMASK(DOUBLE_CLICK_EVENT));
				if (ev.type == DOUBLE_CLICK_EVENT) {
					ignore_next_doubleclick_ = true;
				}
				last_was_doubleclick_ = false;
			}
		}


		if(sorter_ != NULL) {
			const int heading = hit_heading(x,y);
			if(heading >= 0 && sorter_->column_sortable(heading)) {
				sort_by(heading);
			}
		}
	} else if(event.type == SDL_MOUSEMOTION) {
		if(click_selects_) {
			const int item = hit(event.motion.x,event.motion.y);
			if (item != -1)
				move_selection(item);
		}

		const int heading_item = hit_heading(event.motion.x,event.motion.y);
		if(heading_item != highlight_heading_) {
			highlight_heading_ = heading_item;
			invalidate_heading();
		}
	}
}

int menu::process()
{
	if(show_result_) {
		show_result_ = false;
		return selected_;
	} else {
		return -1;
	}
}

bool menu::double_clicked()
{
	bool old = double_clicked_;
	double_clicked_ = false;
	return old;
}

void menu::set_click_selects(bool value)
{
	click_selects_ = value;
}

void menu::set_numeric_keypress_selection(bool value)
{
	num_selects_ = value;
}

void menu::scroll(int)
{
	itemRects_.clear();
	set_dirty();
}

void menu::sort_by(int column)
{
	const bool already_sorted = (column == sortby_);

	if(already_sorted) {
		if(sortreversed_ == false) {
			sortreversed_ = true;
		} else {
			sortreversed_ = false;
			sortby_ = -1;
		}
	} else {
		sortby_ = column;
		sortreversed_ = false;
	}

	do_sort();
	itemRects_.clear();
	set_dirty();
}

namespace {
	SDL_Rect item_size(const std::string& item) {
		SDL_Rect res = {0,0,0,0};
		std::vector<std::string> img_text_items = utils::split(item, IMG_TEXT_SEPARATOR);
		for (std::vector<std::string>::const_iterator it = img_text_items.begin();
			 it != img_text_items.end(); it++) {
			if (res.w > 0 || res.h > 0) {
				// Not the first item, add the spacing.
				res.w += 5;
			}
			const std::string str = *it;
			if (!str.empty() && str[0] == IMAGE_PREFIX) {
				const std::string image_name(str.begin()+1,str.end());
				surface const img = image::get_image(image_name,image::UNSCALED);
				if(img != NULL) {
					res.w += img->w;
					res.h = maximum<int>(img->h, res.h);
				}
			} else {
				const SDL_Rect area = {0,0,10000,10000};
				const SDL_Rect font_size =
					font::draw_text(NULL,area,menu_font_size,font::NORMAL_COLOUR,str,0,0);
				res.w += font_size.w;
				res.h = maximum<int>(font_size.h, res.h);
			}
		}
		return res;
	}
}

void menu::column_widths_item(const std::vector<std::string>& row, std::vector<int>& widths) const
{
	for(size_t col = 0; col != row.size(); ++col) {
		const SDL_Rect res = item_size(row[col]);

		if(col == widths.size()) {
			widths.push_back(res.w + menu_cell_padding);
		} else if(res.w > widths[col] - menu_cell_padding) {
			widths[col] = res.w + menu_cell_padding;
		}
	}
}

const std::vector<int>& menu::column_widths() const
{
	if(column_widths_.empty()) {
		column_widths_item(heading_,column_widths_);
		for(size_t row = 0; row != items_.size(); ++row) {
			column_widths_item(items_[row].fields,column_widths_);
		}
	}

	return column_widths_;
}

void menu::clear_item(int item)
{
	SDL_Rect rect = get_item_rect(item);
	if (rect.w == 0)
		return;
	bg_restore(rect);
}

void menu::draw_row(const std::vector<std::string>& row, const SDL_Rect& rect, ROW_TYPE type)
{
	if(rect.w == 0 || rect.h == 0) {
		return;
	}

	bg_restore(rect);

	int rgb = 0;
	double alpha = 0.0;

	switch(type) {
	case NORMAL_ROW:
		rgb = 0x000000;
		alpha = 0.2;
		break;
	case SELECTED_ROW:
		rgb = 0x990000;
		alpha = 0.6;
		break;
	case HEADING_ROW:
		rgb = 0x333333;
		alpha = 0.3;
		break;
	}

	draw_solid_tinted_rectangle(rect.x, rect.y, rect.w, rect.h,
				    (rgb&0xff0000) >> 16,(rgb&0xff00) >> 8,rgb&0xff,alpha,
				    video().getSurface());

	SDL_Rect const &area = screen_area();
	SDL_Rect const &loc = inner_location();

	const std::vector<int>& widths = column_widths();

	int xpos = rect.x;
	for(size_t i = 0; i != row.size(); ++i) {

		if(type == HEADING_ROW && highlight_heading_ == int(i)) {
			draw_solid_tinted_rectangle(xpos,rect.y,widths[i],rect.h,255,255,255,0.3,video().getSurface());
		}

		const int last_x = xpos;
		std::string str = row[i];
		std::vector<std::string> img_text_items = utils::split(str, IMG_TEXT_SEPARATOR);
		for (std::vector<std::string>::const_iterator it = img_text_items.begin();
			 it != img_text_items.end(); it++) {
			str = *it;
			if (!str.empty() && str[0] == IMAGE_PREFIX) {
				const std::string image_name(str.begin()+1,str.end());
				const surface img = image::get_image(image_name,image::UNSCALED);
				const int max_width = max_width_ < 0 ? area.w :
					minimum<int>(max_width_, area.w - xpos);
				if(img != NULL && (xpos - rect.x) + img->w < max_width
				   && rect.y + img->h < area.h) {
					const size_t y = rect.y + (rect.h - img->h)/2;
					video().blit_surface(xpos,y,img);
					xpos += img->w + 5;
				}
			} else {
				const std::string to_show = max_width_ > -1 ?
					font::make_text_ellipsis(str, menu_font_size, loc.w - (xpos - rect.x)) : str;
				const SDL_Rect& text_size = font::text_area(str,menu_font_size);
				const size_t y = rect.y + (rect.h - text_size.h)/2;
				font::draw_text(&video(),area,menu_font_size,font::NORMAL_COLOUR,to_show,xpos,y);

				if(type == HEADING_ROW && sortby_ == int(i)) {
					const surface sort_img = image::get_image(sortreversed_ ? "misc/sort-arrow.png" :
					                                   "misc/sort-arrow-reverse.png", image::UNSCALED);
					if(sort_img != NULL && sort_img->w <= widths[i] && sort_img->h <= rect.h) {
						const size_t sort_x = xpos + widths[i] - sort_img->w;
						const size_t sort_y = rect.y + rect.h/2 - sort_img->h/2;
						video().blit_surface(sort_x,sort_y,sort_img);
					}
				}

				xpos += text_size.w + 5;
			}
		}
		xpos = last_x + widths[i];
	}
}

void menu::draw_contents()
{
	SDL_Rect heading_rect = inner_location();
	heading_rect.h = heading_height();
	draw_row(heading_,heading_rect,HEADING_ROW);

	for(size_t i = 0; i != item_pos_.size(); ++i) {
		draw_row(items_[item_pos_[i]].fields,get_item_rect(i),item_pos_[i] == selected_ ? SELECTED_ROW : NORMAL_ROW);
	}
}

void menu::draw()
{
	if(hidden()) {
		return;
	}

	if(!dirty()) {

		for(std::set<int>::const_iterator i = invalid_.begin(); i != invalid_.end(); ++i) {
			if(*i == -1) {
				SDL_Rect heading_rect = inner_location();
				heading_rect.h = heading_height();
				bg_restore(heading_rect);
				draw_row(heading_,heading_rect,HEADING_ROW);
				update_rect(heading_rect);
			} else if(*i >= 0 && *i < int(item_pos_.size())) {
				const int pos = item_pos_[*i];
				const SDL_Rect& rect = get_item_rect(*i);
				bg_restore(rect);
				draw_row(items_[pos].fields,rect,pos == selected_ ? SELECTED_ROW : NORMAL_ROW);
				update_rect(rect);
			}
		}

		invalid_.clear();
		return;
	}

	invalid_.clear();

	bg_restore();

	util::scoped_ptr<clip_rect_setter> clipper(NULL);
	if(clip_rect())
		clipper.assign(new clip_rect_setter(video().getSurface(), *clip_rect()));

	draw_contents();

	update_rect(location());
	set_dirty(false);
}

int menu::hit(int x, int y) const
{
	SDL_Rect const &loc = inner_location();
	if (x >= loc.x  && x < loc.x + loc.w && y >= loc.y && y < loc.y + loc.h) {
		for(size_t i = 0; i != items_.size(); ++i) {
			const SDL_Rect& rect = get_item_rect(i);
			if (y >= rect.y && y < rect.y + rect.h)
				return i;
		}
	}

	return -1;
}

int menu::hit_column(int x, int y) const
{
	std::vector<int> const &widths = column_widths();
	x -= location().x;
	for(int j = 0, j_end = widths.size(); j != j_end; ++j) {
		x -= widths[j];
		if (x < 0) {
			return j;
		}
	}

	return -1;
}

std::pair<int,int> menu::hit_cell(int x, int y) const
{
	const int row = hit(x, y);
	if(row < 0) {
		return std::pair<int,int>(-1, -1);
	}

	const int col = hit_column(x,y);
	if(col < 0) {
		return std::pair<int,int>(-1, -1);
	}

	return std::pair<int,int>(x,y);
}

int menu::hit_heading(int x, int y) const
{
	const size_t height = heading_height();
	const SDL_Rect& loc = inner_location();
	if(y >= loc.y && y < loc.y + height) {
		return hit_column(x,y);
	} else {
		return -1;
	}
}

SDL_Rect menu::get_item_rect(int item) const
{
	return get_item_rect_internal(item_pos_[item]);
}

SDL_Rect menu::get_item_rect_internal(size_t item) const
{
	const SDL_Rect empty_rect = {0,0,0,0};
	int first_item_on_screen = get_position();
	if (item < first_item_on_screen ||
	    size_t(item) >= first_item_on_screen + max_items_onscreen()) {
		return empty_rect;
	}

	const std::map<int,SDL_Rect>::const_iterator i = itemRects_.find(item);
	if(i != itemRects_.end())
		return i->second;

	SDL_Rect const &loc = inner_location();

	int y = loc.y + heading_height();
	if (item != first_item_on_screen) {
		const SDL_Rect& prev = get_item_rect_internal(item-1);
		y = prev.y + prev.h;
	}

	SDL_Rect res = { loc.x, y, loc.w, get_item_height(item) };

	SDL_Rect const &screen_area = ::screen_area();

	if(res.x > screen_area.w) {
		return empty_rect;
	} else if(res.x + res.w > screen_area.w) {
		res.w = screen_area.w - res.x;
	}

	if(res.y > screen_area.h) {
		return empty_rect;
	} else if(res.y + res.h > screen_area.h) {
		res.h = screen_area.h - res.y;
	}

	//only insert into the cache if the menu's co-ordinates have
	//been initialized
	if (loc.x > 0 && loc.y > 0)
		itemRects_.insert(std::pair<int,SDL_Rect>(item,res));

	return res;
}

size_t menu::get_item_height_internal(const std::vector<std::string>& item) const
{
	size_t res = 0;
	for(std::vector<std::string>::const_iterator i = item.begin(); i != item.end(); ++i) {
		SDL_Rect rect = item_size(*i);
		res = maximum<int>(rect.h,res);
	}

	return res;
}

size_t menu::heading_height() const
{
	if(heading_height_ == -1) {
		heading_height_ = int(get_item_height_internal(heading_));
	}

	return minimum<unsigned int>(heading_height_,max_height_);
}

size_t menu::get_item_height(int) const
{
	if(item_height_ != -1)
		return size_t(item_height_);

	size_t max_height = 0;
	for(size_t n = 0; n != items_.size(); ++n) {
		max_height = maximum<int>(max_height,get_item_height_internal(items_[n].fields));
	}

	return item_height_ = max_height;
}

void menu::process_help_string(int mousex, int mousey)
{
	const std::pair<int,int> loc = hit_cell(mousex,mousey);
	if(loc == cur_help_) {
		return;
	} else if(loc.first == -1) {
		video().clear_help_string(help_string_);
		help_string_ = -1;
	} else {
		if(help_string_ != -1) {
			video().clear_help_string(help_string_);
			help_string_ = -1;
		}

		if(size_t(loc.first) < items_.size()) {
			const std::vector<std::string>& row = items_[loc.first].help;
			if(size_t(loc.second) < row.size()) {
				const std::string& help = row[loc.second];
				if(help.empty() == false) {
					//std::cerr << "setting help string from menu to '" << help << "'\n";
					help_string_ = video().set_help_string(help);
				}
			}
		}
	}

	cur_help_ = loc;
}

void menu::invalidate_row(size_t id)
{
	if(id >= items_.size()) {
		return;
	}

	invalid_.insert(int(id));
}

void menu::invalidate_row_pos(size_t pos)
{
	if(pos >= items_.size()) {
		return;
	}

	invalidate_row(items_[pos].id);
}

void menu::invalidate_heading()
{
	invalid_.insert(-1);
}

}
