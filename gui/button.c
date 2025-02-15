/**
Copyright 2025 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>

#include "gui/button.h"


static const struct sol_gui_object_structure_functions sol_gui_button_structure_functions =
{
	#warning can/should probably remove this (structure dependent on type)
};

#warning possibly do need return type? to handle setting widget as active &c.

#warning does this need to travese up the stack? (call same with parent) OR do that at top level (as that would avoid stack use)
bool sol_gui_button_default_input_action(struct sol_gui_object* obj, const struct sol_gui_input* input)
{
	struct sol_gui_button* button = (struct sol_gui_button*)obj;

	switch(input->type)
	{
		case SOL_GUI_INPUT_SELECT_BEGIN:
			button->select_action(button->data);
			return true;

		default:
			return false;
	}
}

void sol_gui_button_construct(struct sol_gui_button* button, struct sol_gui_context* context, void(*select_action)(void*), void* data)
{
	sol_gui_object_construct(&button->base, context);

	button->base.structure_functions = &sol_gui_button_structure_functions;
	button->base.input_action = &sol_gui_button_default_input_action;

	button->select_action = select_action;
	button->data = data;
}








// specific buttons

// get the buffer malloc'd after the button,
//actually preferable here NOT to have pointer as it communicates that its not an allocated buffer and should NOT be freed

static inline void* sol_gui_button_get_buffer(struct sol_gui_button* button)
{
	return (button+1);
}





static void sol_gui_text_button_render(struct sol_gui_object* obj, vec2_s16 offset, rect_s16 bounds, struct cvm_overlay_render_batch* batch)
{
	struct sol_gui_button* button = (struct sol_gui_button*)obj;
	char* text = sol_gui_button_get_buffer(button);
	exit('!');
}

static struct sol_gui_object* sol_gui_text_button_select(struct sol_gui_object* obj, vec2_s16 location)
{
	exit('!');
	return obj;
}

static void sol_gui_text_button_set_min_size(struct sol_gui_object* obj)
{
	exit('!');
}

static const struct sol_gui_object_structure_functions sol_gui_text_button_structure_functions =
{
	.render        = &sol_gui_text_button_render,
	.select        = &sol_gui_text_button_select,
	.set_min_size  = &sol_gui_text_button_set_min_size,
};

struct sol_gui_object* sol_gui_text_button_create(struct sol_gui_context* context, void(*select_action)(void*), void* data, char* text)
{
	size_t text_len = strlen(text) + 1;
	struct sol_gui_button* button = malloc(sizeof(struct sol_gui_button) + text_len);
	void* text_buf = sol_gui_button_get_buffer(button);

	sol_gui_button_construct(button, context, select_action, data);

	button->base.structure_functions = &sol_gui_text_button_structure_functions;

	memcpy(text_buf, text, text_len);

	return &button->base;
}









static void sol_gui_utf8_icon_button_render(struct sol_gui_object* obj, vec2_s16 offset, rect_s16 bounds, struct cvm_overlay_render_batch* batch)
{
	struct sol_gui_button* button = (struct sol_gui_button*)obj;
	char* utf8_icon = sol_gui_button_get_buffer(button);
	exit('!');
}

static struct sol_gui_object* sol_gui_utf8_icon_button_select(struct sol_gui_object* obj, vec2_s16 location)
{
	exit('!');
	return obj;
}

static void sol_gui_utf8_icon_button_set_min_size(struct sol_gui_object* obj)
{
	exit('!');
}

static const struct sol_gui_object_structure_functions sol_gui_utf8_icon_button_structure_functions =
{
	.render        = &sol_gui_utf8_icon_button_render,
	.select        = &sol_gui_utf8_icon_button_select,
	.set_min_size  = &sol_gui_utf8_icon_button_set_min_size,
};

struct sol_gui_object* sol_gui_utf8_icon_button_create(struct sol_gui_context* context, void(*select_action)(void*), void* data, char* utf8_icon)
{
	size_t utf8_icon_len = strlen(utf8_icon) + 1;
	struct sol_gui_button* button = malloc(sizeof(struct sol_gui_button) + utf8_icon_len);
	void* utf8_icon_buf = sol_gui_button_get_buffer(button);

	sol_gui_button_construct(button, context, select_action, data);

	button->base.structure_functions = &sol_gui_utf8_icon_button_structure_functions;

	memcpy(utf8_icon_buf, utf8_icon, utf8_icon_len);

	return &button->base;
}





