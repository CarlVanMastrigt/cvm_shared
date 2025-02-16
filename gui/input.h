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

#pragma once

#include <inttypes.h>

#include "math/vec2_s16.h"


#warning wait, fuck, how do we know if THE SAME THING was clicked and released (could we just... not?)
#warning how do we handle tab bars that allow dragging to move? would they have to be the same object to allow this?

enum sol_gui_input_type
{
	// usually deactivates active widget, should only do something if there's an active widget (b button, escape key, click away)
	SOL_GUI_INPUT_CANCEL,

	/// basically ui click, then actions happening after something was selected
	/// may need variants based on button that was pressed (r button different to l-button)
	SOL_GUI_INPUT_SELECT_BEGIN,
	SOL_GUI_INPUT_SELECT_END,

	/// should know if somthing can be dropped (needn't be a GUI object, can be something independent, e.g. game object)
	SOL_GUI_INPUT_DROP_QUERY,// can this item be dropped into with the currently active object (NOT NECESSARILY GUI)
	SOL_GUI_INPUT_DROP_PERFORM,// actually drop the current active object into the GUI element below it

	SOL_GUI_INPUT_TEXT_EDIT,
	SOL_GUI_INPUT_TEXT_PASTE,
	SOL_GUI_INPUT_TEXT_COPY,
	SOL_GUI_INPUT_TEXT_DELETE,// back/forwards directions

	SOL_GUI_INPUT_MOVE_DIRECTION,// arrow keys and controller joystick (probably float vec2, possibly vec2_s16)
	SOL_GUI_INPUT_MOVE_POSITION,// acts on selected/active object? (drag &c.)
};

// includes required null terminator (text[SOL_GUI_INPUT_TEXT_EDIT_LENGTH-1] will be set to '\0')
#define SOL_GUI_INPUT_TEXT_EDIT_LENGTH 32

struct sol_gui_input
{
	enum sol_gui_input_type type;

	// time since app start
	uint32_t time_rel_msec;
	// unix time
	uint32_t time_abs_nsec;
	uint64_t time_abs_sec;


	union
	{
		struct
		{
			vec2_s16 global_pixel_location;
			// how to handle double click/double select? :
			// int repeats; // ?
			// int priority; // ? (higher if multiple?)
		}
		select;// begin, end

		struct
		{
			// need broad, registerable data type that handles drag drop operations, *ideally* registered through a higher level data type/struct than UI
		}
		drop;//query and perform

		struct
		{
			char text[SOL_GUI_INPUT_TEXT_EDIT_LENGTH];
		}
		text_edit;

		// some types may not have backing data...
	};
};