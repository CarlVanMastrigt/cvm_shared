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

#include "math/rect_s16.h"

struct sol_gui_object;

/** context
 * outside ofgui setup/creation usually want to pass around context for rendering, input management &c.
*/

struct sol_gui_context
{
    // should be position and size of containing system window
    rect_s16 range;

	// shouldnt be able to modify the theme's data from
    const struct sol_gui_theme* theme;

    /// put settings here?? make it a pointer!?

    uint32_t registered_object_count;//for debug

    // uint32_t double_click_time;//move to settings


    // not sure if a separation of "currently clicked"(selected) and "clicked but held onto"(active) is desirable
    struct sol_gui_object* selected_object;// highlighted when using directional (arrows/joystick) navigation
    struct sol_gui_object* active_object;
    struct sol_gui_object* interactable_object;// limit interaction to only this object and its children

    /// used for double click detection
    struct sol_gui_object* previously_clicked_object;
    uint32_t previously_clicked_time;

    struct sol_gui_object* root_container;// this should not change
};

// also creates and returns the root object
struct sol_gui_object* sol_gui_context_initialise(struct sol_gui_context* context, const struct sol_gui_theme* theme);
void                   sol_gui_context_terminate (struct sol_gui_context* context);

void sol_gui_context_update_range(struct sol_gui_context* context, rect_s16 range);

// actually not sure how to handle these, they WILL retain objects though
void sol_gui_context_set_selected_object    (struct sol_gui_context* context, struct sol_gui_object* obj);
void sol_gui_context_set_active_object      (struct sol_gui_context* context, struct sol_gui_object* obj);
void sol_gui_context_set_interactable_object(struct sol_gui_context* context, struct sol_gui_object* obj);

