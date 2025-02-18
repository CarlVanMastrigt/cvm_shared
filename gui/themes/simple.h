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

#include <ft2build.h>
#include <freetype/freetype.h>

#warning massively dislike having to include freetype here, have custom wrapper if possible?
/// ^ sol_gui_text_library ???

struct sol_gui_theme;

void sol_gui_theme_simple_initialise(struct sol_gui_theme* theme, const FT_Library * freetype_library);
void sol_gui_theme_simple_terminate(struct sol_gui_theme* theme);

struct sol_gui_theme* sol_gui_theme_simple_create(const FT_Library * freetype_library);
void sol_gui_theme_simple_destroy(struct sol_gui_theme* theme);
