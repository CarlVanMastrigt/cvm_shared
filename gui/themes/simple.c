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

#include "gui/theme.h"

#include "cvm_overlay_text.h"






void sol_gui_theme_simple_initialise(struct sol_gui_theme* theme, const FT_Library * freetype_library)
{
	*theme = (struct sol_gui_theme)
	{
		#warning replace cvm_overlay_font, and its use here
		.font = malloc(sizeof(struct cvm_overlay_font)),
	};



	cvm_overlay_create_font(theme->font, freetype_library,"resources/Comfortaa-Regular.ttf",16);
}

void sol_gui_theme_simple_terminate(struct sol_gui_theme* theme)
{
	#warning need to destroy font, but this can be solved later
}

struct sol_gui_theme* sol_gui_theme_simple_create(const FT_Library * freetype_library)
{
	struct sol_gui_theme* theme = malloc(sizeof(struct sol_gui_theme));
	sol_gui_theme_simple_initialise(theme, freetype_library);
	return theme;
}

void sol_gui_theme_simple_destroy(struct sol_gui_theme* theme)
{
	sol_gui_theme_simple_terminate(theme);
	free(theme);
}