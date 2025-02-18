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

struct sol_gui_input;
struct sol_gui_object;

/** context
 * outside ofgui setup/creation usually want to pass around context for rendering, input management &c.
*/

struct sol_gui_context
{
    // should be position and size of containing system window
    vec2_s16 window_offset;
    vec2_s16 window_size;
    vec2_s16 window_min_size;

	// shouldnt be able to modify the theme's data from
    const struct sol_gui_theme* theme;

    /// put settings here?? make it a pointer!?

    uint32_t registered_object_count;//for debug
    bool content_fit;

    // uint32_t double_click_time;//move to settings

    struct sol_gui_object* prominent_object;// highlighted when using directional (arrows/joystick) navigation, can/should probably change with mouse motion?
    struct sol_gui_object* focused_object;// limit interaction to only this object and its children, click away/cancel called when defocused?

    /// used for double click detection
    struct sol_gui_object* previously_clicked_object;
    uint32_t previously_clicked_time;

    struct sol_gui_object* root_container;// this should not change
};

// also creates and returns the root object
struct sol_gui_object* sol_gui_context_initialise(struct sol_gui_context* context, const struct sol_gui_theme* theme, vec2_s16 window_offset, vec2_s16 window_size);
void                   sol_gui_context_terminate (struct sol_gui_context* context);

// actually not sure how to handle these, they WILL retain objects though
void sol_gui_context_set_prominent_object(struct sol_gui_context* context, struct sol_gui_object* obj);
void sol_gui_context_set_focused_object  (struct sol_gui_context* context, struct sol_gui_object* obj);


/**
 * on failure should use context's window_min_size to ensure window is big enough before calling next
 * otherwise if min_size is null will supress errors and FORCE it to work
 *  this will result in content escaping the screen range, and will set content_fits to false
 * */
bool sol_gui_context_update_window(struct sol_gui_context* context, vec2_s16 window_offset, vec2_s16 window_size);
// call this when contents of all widgets may have changed, e.g. at crteation time, after theme change, if a single widget in root has changed, instead try to be more precise
bool sol_gui_context_reorganise_root(struct sol_gui_context* context);

bool sol_gui_context_handle_input(struct sol_gui_context* context, const struct sol_gui_input* input);


