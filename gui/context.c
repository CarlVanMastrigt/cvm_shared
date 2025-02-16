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

#include <assert.h>

#include "context.h"
#include "container.h"


struct sol_gui_object* sol_gui_context_initialise(struct sol_gui_context* context, const struct sol_gui_theme* theme)
{
	struct sol_gui_object* root_container;
	*context = (struct sol_gui_context)
	{
		.theme = theme,
		.registered_object_count = 0,
		.selected_object = NULL,
		.active_object = NULL,
		.interactable_object = NULL,
		.previously_clicked_object = NULL,
	};

	root_container = sol_gui_container_create(context);

	context->root_container = root_container;

	return root_container;
}

void sol_gui_context_terminate(struct sol_gui_context* context)
{
	if(context->previously_clicked_object)
	{
		sol_gui_object_release(context->previously_clicked_object);
	}
	sol_gui_context_set_selected_object(context, NULL);
	sol_gui_context_set_active_object(context, NULL);
	sol_gui_context_set_interactable_object(context, NULL);

	sol_gui_object_recursive_destroy(context->root_container);
	assert(context->registered_object_count == 0);
}

void sol_gui_context_update_range(struct sol_gui_context* context, rect_s16 range)
{
}

void sol_gui_context_set_selected_object(struct sol_gui_context* context, struct sol_gui_object* obj)
{
	assert(obj->context == context);
	if(context->selected_object)
	{
		sol_gui_object_release(context->selected_object);
		#warning need more here? (some action when not selected anymore?)
	}
	if(obj)
	{
		sol_gui_object_retain(obj);
		context->selected_object = obj;
	}
}
void sol_gui_context_set_active_object(struct sol_gui_context* context, struct sol_gui_object* obj)
{
	assert(obj->context == context);
	if(context->active_object)
	{
		sol_gui_object_release(context->active_object);
		#warning need more here? (some action when not selected anymore?)
	}
	if(obj)
	{
		sol_gui_object_retain(obj);
		context->active_object = obj;
	}
}
void sol_gui_context_set_interactable_object(struct sol_gui_context* context, struct sol_gui_object* obj)
{
	assert(obj->context == context);
	if(context->interactable_object)
	{
		sol_gui_object_release(context->interactable_object);
		#warning need more here? (some action when not selected anymore?)
	}
	if(obj)
	{
		sol_gui_object_retain(obj);
		context->interactable_object = obj;
	}
}