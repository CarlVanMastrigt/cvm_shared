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

/** these are the enumerators used to describe various things to do with the GUI
 * incompatible options are enumerators (list of choices) compatible options are defines (bit-flags)*/

// layout of anything GUI related, e.g. contents of a box, text orientation, scrollable bar, the dynamic part of a grid (hopefully these are self explanitory)
enum sol_gui_layout
{
    SOL_GUI_LAYOUT_HORIZONTAL,
    SOL_GUI_LAYOUT_VERTICAL,
};

// how space in a container will be shared/distributed
enum sol_gui_space_distribution
{
	// dont use the remaining space, leave it empty at the start of the container
    SOL_GUI_SPACE_DISTRIBUTION_START,
    // dont use the remaining space, leave it empty at the end of the container
    SOL_GUI_SPACE_DISTRIBUTION_END,
	// first object in container gets all remaining space
    SOL_GUI_SPACE_DISTRIBUTION_FIRST,
    // last object in container gets all remaining space
    SOL_GUI_SPACE_DISTRIBUTION_LAST,
    // distribute remaining space evenly amonst all objects
    SOL_GUI_SPACE_DISTRIBUTION_EVEN,
    // distribute space in an attempt to make all objects in the container the same size, starting with the smallest first
    SOL_GUI_SPACE_DISTRIBUTION_SAME,
};

// used for things like text, specifically avoiding left/right because alignment may be rlative to horizontal OR vertical directions
enum sol_gui_alignment
{
    SOL_GUI_ALIGNMENT_START,
    SOL_GUI_ALIGNMENT_END,
    SOL_GUI_ALIGNMENT_CENTRE,
};
// also difficuly because we could need horizontal AND vertical alignment

// SOL_GUI_REFERENCE_BIT_COUNT + SOL_GUI_OBJECT_STATUS_BIT_COUNT + SOL_GUI_POS_BIT_CCOUNT <= 32
#define SOL_GUI_REFERENCE_BIT_COUNT 8


#define SOL_GUI_OBJECT_STATUS_BIT_COUNT 16
#define SOL_GUI_OBJECT_STATUS_ACTIVE     0x00000001 /* inactive objects are not rendered or selected, used to quickly "remove" objects without actually altering structure */
#define SOL_GUI_OBJECT_STATUS_IS_ROOT    0x00000002 /** used for validation in various places*/
#define SOL_GUI_OBJECT_STATUS_REGISTERED 0x00008000 /** used for validation, ensures the object base is only registered and constructed once */
// #define SOL_GUI_OBJECT_MINIMIZE_SPACE     0x00000004 /* always uses minimum space, forces it's parent to ignore this object when distributing space; everything that this does can probably be done better */
// #define SOL_GUI_OBJECT_CLOSE_POPUP_TREE   0x00000008 /** does not collapse parent popup hierarchy upon interaction (e.g. toggle buttons, popup triggering buttons, enterboxes, scrollbars and slider_bars)  */

/// placement used to communicate which edge of the screen (if any) a gui object is touching
#define SOL_GUI_PLACEMENT_BIT_COUNT 4
#define SOL_GUI_PLACEMENT_FIRST_X 0x00000001
#define SOL_GUI_PLACEMENT_LAST_X  0x00000002
#define SOL_GUI_PLACEMENT_FIRST_Y 0x00000004
#define SOL_GUI_PLACEMENT_LAST_Y  0x00000008
// position will be combined with theme flags below to render parts of the theme correctly

// apply blank space surrounding the widget, used for all functions that have spatial considerations
#define SOL_GUI_THEME_FLAG_BORDER 0x80000000
