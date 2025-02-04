/**
Copyright 2020,2021,2022 Carl van Mastrigt

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

#ifndef solipsix_H
#include "solipsix.h"
#endif

#ifndef WIDGET_POPUP_H
#define WIDGET_POPUP_H


typedef struct widget_popup
{
    widget_base base;

    widget * contents;

    widget * internal_alignment_widget;

    widget * external_alignment_widget;

    widget * trigger_widget;

    widget_relative_positioning positioning;
}
widget_popup;

widget * create_popup(struct widget_context* context, widget_relative_positioning positioning,bool auto_close);

void set_popup_alignment_widgets(widget * popup,widget * internal_alignment_widget,widget * external_alignment_widget);

void set_popup_trigger_widget(widget * popup,widget * trigger_widget);

void toggle_exclusive_popup(widget * popup);
void toggle_auto_close_popup(widget * popup);

bool close_auto_close_popup_tree(widget * interacted);


#endif

