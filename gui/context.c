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

#include <stdio.h>
#warning remove above
#include <assert.h>

#include "gui/context.h"
#include "gui/input.h"
#include "gui/container.h"


struct sol_gui_object* sol_gui_context_initialise(struct sol_gui_context* context, const struct sol_gui_theme* theme, vec2_s16 window_offset, vec2_s16 window_size)
{
	struct sol_gui_object* root_container;
	*context = (struct sol_gui_context)
	{
		.window_size = window_size,
		.window_offset = window_offset,
		.theme = theme,
		.registered_object_count = 0,
		.content_fit = true,
		.prominent_object = NULL,
		.focused_object = NULL,
		.previously_clicked_object = NULL,
	};

	root_container = sol_gui_container_create(context);

	context->root_container = root_container;
	root_container->status_flags |= SOL_GUI_OBJECT_STATUS_IS_ROOT;

	return root_container;
}

void sol_gui_context_terminate(struct sol_gui_context* context)
{
	if(context->previously_clicked_object)
	{
		assert(context->previously_clicked_object->context == context);
		sol_gui_object_release(context->previously_clicked_object);
	}
	sol_gui_context_set_prominent_object(context, NULL);
	sol_gui_context_set_focused_object(context, NULL);

	// this will effectively recursively release the objects in the heirarchy
	sol_gui_object_release(context->root_container);
	assert(context->registered_object_count == 0);
}


#warning need to differentiate select input and selected object ??
#warning how to handle arrow selection/highlighting and mouse hover highlighting? (mouse overrides conditionally) ( conditional highlight variance when active perhaps? )

// chosen? elected? highlighted? focused?
void sol_gui_context_set_prominent_object(struct sol_gui_context* context, struct sol_gui_object* obj)
{
	if(context->prominent_object != obj)
	{
		if(context->prominent_object)
		{
			// need deselect action?
			sol_gui_object_release(context->prominent_object);
		}
		if(obj)
		{
			assert(obj->context == context);
			sol_gui_object_retain(obj);
			context->prominent_object = obj;
		}
	}
}

void sol_gui_context_set_focused_object(struct sol_gui_context* context, struct sol_gui_object* obj)
{
	// crap, this may require time of event?, is there an easy/accurate way to get that based on provoking cause of this??
	struct sol_gui_input cancel_input =
	{
		.type = SOL_GUI_INPUT_CANCEL,
	};

	if(context->focused_object != obj)
	{
		if(context->focused_object)
		{
			sol_gui_object_handle_input(context, &cancel_input);
			sol_gui_object_release(context->focused_object);
		}
		if(obj)
		{
			assert(obj->context == context);
			sol_gui_object_retain(obj);
			context->focused_object = obj;
		}
	}
}




/// this is different than reorganise root, this assumes min_sizes havent changed
bool sol_gui_context_update_window(struct sol_gui_context* context, vec2_s16 window_offset, vec2_s16 window_size)
{
	// assume min_size setting isn't required
	if(vec2_s16_cmp_all_eq(window_size, context->window_size))
	{
		// nothing internal has changed, no need to place content again
		context->window_offset = window_offset;
	}
	else
	{
		context->content_fit = vec2_s16_cmp_all_lte(context->window_min_size, window_size);
		rect_s16 content_rect = {.start={0,0}, .end = vec2_s16_max(context->window_min_size, context->window_size)};
		sol_gui_object_place_content(context->root_container, content_rect);
	}

	return context->content_fit;
}

// call this when contents of all widgets may have changed, e.g. at crteation time, after theme change, if a single "toplevel" object in root has changed, instead try to be more precise
bool sol_gui_context_reorganise_root(struct sol_gui_context* context)
{
	context->window_min_size = sol_gui_object_min_size(context->root_container);

	context->content_fit = vec2_s16_cmp_all_lte(context->window_min_size, context->window_size);

	rect_s16 content_rect = {.start={0,0}, .end = vec2_s16_max(context->window_min_size, context->window_size)};
	sol_gui_object_place_content(context->root_container, content_rect);

	return context->content_fit;
}



bool sol_gui_context_handle_input(struct sol_gui_context* context, const struct sol_gui_input* input)
{
	#warning need to query for prominent object? (under mouse OR resulting from directional input)

	/// FOCUS consumes all input (?)
	/// need to remove FOCUS to change this (some input that sets focus to NULL, usually cancel but NOT always)
	/// 	^ for this reason probably desirable to distinguish click away from cancel

	// move ; mouse or direction can change prominent object (let object handle this internally? -- this or "can be prominent" flag) but probably want movement away to remove prominence ?
	// ^ prominence important for keyboard/controller navigation! so need many widgets to allow prominence, but not all...
	///		^ probably want them to handle directional input internally, with common default values to allow scanning for a new prominent object

	// select should at least be run for "click away" purposes, assuming that can function the same as cancel,
	// perhaps NOT canceling is an indicator to ignore that input? can/should select only run if it does overlap with FOCUS object




	switch(input->type)
	{
		// fallthrough is intentional here
	case SOL_GUI_INPUT_CANCEL:
		// probably an error not to handle TEXT functions (as otherwise what is accepting text?)
	case SOL_GUI_INPUT_TEXT_EDIT:
	case SOL_GUI_INPUT_TEXT_PASTE:
	case SOL_GUI_INPUT_TEXT_COPY:
	case SOL_GUI_INPUT_TEXT_DELETE:
		// ^ only acceptable by focused object
	case SOL_GUI_INPUT_MOVE_DIRECTION:// (arrow keys) this can probably act on focused AND prominent
	case SOL_GUI_INPUT_MOVE_POSITION:// (mouse move) act on prominent (if focused?) and generic?
		// only acceptable by focused object
		break;
	case SOL_GUI_INPUT_PRIMARY_SELECT_BEGIN:
	case SOL_GUI_INPUT_PRIMARY_SELECT_END:
	case SOL_GUI_INPUT_CONTEXT_SELECT_BEGIN:
	case SOL_GUI_INPUT_CONTEXT_SELECT_END:
		// for drop, need something in held slot, not sure how to store/communicate it though
		break;
	case SOL_GUI_INPUT_DROP_QUERY:// moving when something held to be dropped, need
	case SOL_GUI_INPUT_DROP_PERFORM:
	}
	return false;
}
