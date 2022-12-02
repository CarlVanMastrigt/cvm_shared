/**
Copyright 2020,2021,2022 Carl van Mastrigt

This file is part of cvm_shared.

cvm_shared is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

cvm_shared is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with cvm_shared.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef WIDGET_TEXT_BAR_H
#define WIDGET_TEXT_BAR_H


typedef struct widget_text_bar
{
    widget_base base;

	char * text;

	char * selection_begin;
    char * selection_end;

	uint32_t free_text:1;
	uint32_t allow_selection:1;
	uint32_t recalculate_text_size:1;

	int32_t visible_offset;///usually based on text_alignment, alternatively can be based on selection
	int32_t max_visible_offset;///usually based on text_alignment, alternatively can be based on selection
    int32_t text_pixel_length;

	int32_t min_glyph_render_count;///should definitely make use of this (and/or max render count) in static text bar as well!
	int32_t max_glyph_render_count;
	///uint32_t max_strlen;needed if handling text internally (compositing internally, not sure this is desirable functionality though)
	widget_text_alignment text_alignment;///when selection not active?
}
widget_text_bar;

/// min_glyph_render_count==0  ->  render all text (doesnt really work with dynamic text though...
widget * create_static_text_bar(char * text);
widget * create_dynamic_text_bar(int min_glyph_render_count,widget_text_alignment text_alignment,bool allow_selection);

void text_bar_widget_set_text_pointer(widget * w,char * text_pointer);///also marks as having been updated
void text_bar_widget_set_text(widget * w,char * text_to_copy);

#endif

