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

#include "gui/object.h"

void sol_gui_object_construct(struct sol_gui_object* obj, struct sol_gui_context* context)
{
	*obj = (struct sol_gui_object)
	{
		.context = context,
		.reference_count = 1,// existing is a reference, MUST call delete
		.status_flags = SOL_GUI_OBJECT_STATUS_REGISTERED | SOL_GUI_OBJECT_STATUS_ACTIVE,
	};

	context->registered_object_count++;
}






