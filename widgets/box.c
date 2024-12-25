/**
Copyright 2020,2021,2022 Carl van Mastrigt

This file is part of cvm_shared.

cvm_shared is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

cvm_shared is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with cvm_shared.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "cvm_shared.h"




static int count_affecting_box_contents(widget * container)
{
    int count=0;

    widget * current=container->container.first;

    while(current)
    {
        if(widget_active(current))
        {
            count++;
        }

        current=current->base.next;
    }

    return count;
}





// note: we're sneaky here in avoiding 2 loops of double execution by operating on `prev` widget


static void horizontal_box_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=0;

    widget * current=w->container.first;
    widget * prev=NULL;

    uint32_t pos_flags=w->base.status&WIDGET_POS_FLAGS_H;

    while(current)
    {
        if(widget_active(current))
        {
            if(prev)
            {
                w->base.min_w+=set_widget_minimum_width(prev, theme, pos_flags&(~WIDGET_H_LAST));
                pos_flags&=(~WIDGET_H_FIRST);
            }
            prev=current;
        }
        current=current->base.next;
    }

    if(prev)
    {
        w->base.min_w += set_widget_minimum_width(prev, theme, pos_flags);
    }
}

static void horizontal_box_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=0;

    widget * current=w->container.first;

    uint32_t pos_flags=w->base.status&WIDGET_POS_FLAGS_V;

    while(current)
    {
        if(widget_active(current))
        {
            set_widget_minimum_height(current, theme, pos_flags);

            if(w->base.min_h<current->base.min_h)w->base.min_h=current->base.min_h;
        }

        current=current->base.next;
    }
}

static void horizontal_all_same_distributed_box_widget_min_w(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;
    widget * prev=NULL;

    uint32_t pos_flags=w->base.status&WIDGET_POS_FLAGS_H;

    int num_elements=0;
    int max_width=0;

    while(current)
    {
        if(widget_active(current))
        {
            if(prev)
            {
                set_widget_minimum_width(prev, theme, pos_flags&(~WIDGET_H_LAST));
                if(prev->base.min_w>max_width)
                {
                    max_width=prev->base.min_w;
                }
                pos_flags&=(~WIDGET_H_FIRST);
            }
            prev=current;

            num_elements++;
        }
        current=current->base.next;
    }

    if(prev)
    {
        set_widget_minimum_width(prev, theme, pos_flags);
        if(prev->base.min_w>max_width)max_width=prev->base.min_w;
    }

    w->base.min_w=max_width*num_elements;
}

static void horizontal_first_distributed_box_widget_set_w(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    int offset=0;
    int difference = w->base.r.x2 - w->base.r.x1 - w->base.min_w;

    while(current)
    {
        if(widget_active(current))
        {
            offset+=organise_widget_horizontally(current, theme, offset, current->base.min_w + difference);
            difference=0;
        }

        current=current->base.next;
    }
}

static void horizontal_last_distributed_box_widget_set_w(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;
    widget * prev=NULL;

    int offset=0;
    int difference = w->base.r.x2 - w->base.r.x1 - w->base.min_w;

    while(current)
    {
        if(widget_active(current))
        {
            if(prev)offset+=organise_widget_horizontally(prev, theme, offset, prev->base.min_w);
            prev=current;
        }

        current=current->base.next;
    }

    if(prev)
    {
        organise_widget_horizontally(prev, theme, offset, prev->base.min_w + difference);
    }
}

static void horizontal_evenly_distributed_box_widget_set_w(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    int offset=0;
    int element_number=0;
    int difference=w->base.r.x2-w->base.r.x1-w->base.min_w;
    int num_elements=count_affecting_box_contents(w);

    while(current)
    {
        if(widget_active(current))
        {
            offset+=organise_widget_horizontally(current, theme, offset, current->base.min_w+( (difference/num_elements)+(element_number<(difference%num_elements)) ));
            element_number++;
        }

        current=current->base.next;
    }
}

static void horizontal_normally_distributed_box_widget_set_w(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    int offset=0;

    while(current)
    {
        if(widget_active(current))
        {
            offset+=organise_widget_horizontally(current, theme, offset, current->base.min_w);
        }

        current=current->base.next;
    }
}

static void horizontal_all_same_distributed_box_widget_set_w(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    int offset=0;
    int element_number=0;
    int difference=w->base.r.x2-w->base.r.x1;
    int num_elements=count_affecting_box_contents(w);

    while(current)
    {
        if(widget_active(current))
        {
            offset+=organise_widget_horizontally(current, theme, offset, (difference/num_elements)+(element_number<(difference%num_elements)) );
            element_number++;
        }

        current=current->base.next;
    }
}



static void horizontal_box_widget_set_h(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    while(current)
    {
        if(widget_active(current))
        {
            organise_widget_vertically(current, theme, 0, w->base.r.y2 - w->base.r.y1);
        }

        current=current->base.next;
    }
}






static void vertical_box_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=0;

    widget * current=w->container.first;

    uint32_t pos_flags=w->base.status&WIDGET_POS_FLAGS_H;

    while(current)
    {
        if(widget_active(current))
        {
            set_widget_minimum_width(current, theme, pos_flags);

            if(w->base.min_w<current->base.min_w)w->base.min_w=current->base.min_w;
        }

        current=current->base.next;
    }
}

static void vertical_box_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=0;

    widget * current=w->container.first;
    widget * prev=NULL;

    uint32_t pos_flags=w->base.status&WIDGET_POS_FLAGS_V;

    while(current)
    {
        if(widget_active(current))
        {
            if(prev)
            {
                w->base.min_h+=set_widget_minimum_height(prev, theme, pos_flags&(~WIDGET_V_LAST));
                pos_flags&=(~WIDGET_V_FIRST);
            }
            prev=current;
        }

        current=current->base.next;
    }

    if(prev) 
    {
        w->base.min_h+=set_widget_minimum_height(prev, theme, pos_flags);
    }
}

static void vertical_all_same_distributed_box_widget_min_h(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;
    widget * prev=NULL;

    uint32_t pos_flags=w->base.status&WIDGET_POS_FLAGS_V;

    int num_elements=0;
    int max_height=0;

    while(current)
    {
        if(widget_active(current))
        {
            if(prev)
            {
                w->base.min_h+=set_widget_minimum_height(prev, theme, pos_flags&(~WIDGET_V_LAST));
                if(prev->base.min_h>max_height)max_height=prev->base.min_h;
                pos_flags&=(~WIDGET_V_FIRST);
            }
            prev=current;

            num_elements++;
        }

        current=current->base.next;
    }

    if(prev)
    {
        w->base.min_h+=set_widget_minimum_height(prev, theme, pos_flags&(~WIDGET_V_LAST));
        if(prev->base.min_h>max_height)max_height=prev->base.min_h;
    }

    w->base.min_h=max_height*num_elements;
}

static void vertical_box_widget_set_w(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    while(current)
    {
        if(widget_active(current))
        {
            organise_widget_horizontally(current, theme, 0, w->base.r.x2 - w->base.r.x1);
        }

        current=current->base.next;
    }
}

static void vertical_first_distributed_box_widget_set_h(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    int offset=0;
    int difference = w->base.r.y2 - w->base.r.y1 - w->base.min_h;

    while(current)
    {
        if(widget_active(current))
        {
            offset+=organise_widget_vertically(current, theme, offset, current->base.min_h + difference);

            difference=0;
        }

        current = current->base.next;
    }
}

static void vertical_last_distributed_box_widget_set_h(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;
    widget * prev=NULL;

    int offset=0;
    int difference = w->base.r.y2 - w->base.r.y1 - w->base.min_h;

    while(current)
    {
        if(widget_active(current))
        {
            if(prev)
            {
                offset+=organise_widget_vertically(prev, theme, offset, prev->base.min_h);
            }
            prev=current;
        }

        current=current->base.next;
    }

    if(prev)
    {
        organise_widget_vertically(prev, theme, offset, prev->base.min_h + difference);
    }
}

static void vertical_evenly_distributed_box_widget_set_h(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    int offset=0;
    int element_number=0;
    int difference = w->base.r.y2 - w->base.r.y1 - w->base.min_h;
    int num_elements=count_affecting_box_contents(w);

    while(current)
    {
        if(widget_active(current))
        {
            offset+=organise_widget_vertically(current, theme, offset, current->base.min_h+( (difference/num_elements)+(element_number<(difference%num_elements)) ));

            element_number++;
        }

        current=current->base.next;
    }
}

static void vertical_normally_distributed_box_widget_set_h(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    int offset=0;

    while(current)
    {
        if(widget_active(current))
        {
            offset+=organise_widget_vertically(current, theme, offset, current->base.min_h);
        }

        current=current->base.next;
    }
}

static void vertical_all_same_distributed_box_widget_set_h(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    int offset=0;
    int element_number=0;
    int difference=w->base.r.y2-w->base.r.y1;
    int num_elements=count_affecting_box_contents(w);

    while(current)
    {
        if(widget_active(current))
        {
            offset+=organise_widget_vertically(current, theme, offset, (difference/num_elements)+(element_number<(difference%num_elements)) );

            element_number++;
        }

        current=current->base.next;
    }
}









static widget_appearence_function_set vertical_last_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   vertical_box_widget_min_w,
    .min_h  =   vertical_box_widget_min_h,
    .set_w  =   vertical_box_widget_set_w,
    .set_h  =   vertical_last_distributed_box_widget_set_h
};

static widget_appearence_function_set vertical_first_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   vertical_box_widget_min_w,
    .min_h  =   vertical_box_widget_min_h,
    .set_w  =   vertical_box_widget_set_w,
    .set_h  =   vertical_first_distributed_box_widget_set_h
};

static widget_appearence_function_set vertical_evenly_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   vertical_box_widget_min_w,
    .min_h  =   vertical_box_widget_min_h,
    .set_w  =   vertical_box_widget_set_w,
    .set_h  =   vertical_evenly_distributed_box_widget_set_h
};

static widget_appearence_function_set vertical_normally_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   vertical_box_widget_min_w,
    .min_h  =   vertical_box_widget_min_h,
    .set_w  =   vertical_box_widget_set_w,
    .set_h  =   vertical_normally_distributed_box_widget_set_h
};

static widget_appearence_function_set vertical_all_same_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   vertical_box_widget_min_w,
    .min_h  =   vertical_all_same_distributed_box_widget_min_h,
    .set_w  =   vertical_box_widget_set_w,
    .set_h  =   vertical_all_same_distributed_box_widget_set_h
};





static widget_appearence_function_set horizontal_last_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   horizontal_box_widget_min_w,
    .min_h  =   horizontal_box_widget_min_h,
    .set_w  =   horizontal_last_distributed_box_widget_set_w,
    .set_h  =   horizontal_box_widget_set_h
};

static widget_appearence_function_set horizontal_first_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   horizontal_box_widget_min_w,
    .min_h  =   horizontal_box_widget_min_h,
    .set_w  =   horizontal_first_distributed_box_widget_set_w,
    .set_h  =   horizontal_box_widget_set_h
};

static widget_appearence_function_set horizontal_evenly_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   horizontal_box_widget_min_w,
    .min_h  =   horizontal_box_widget_min_h,
    .set_w  =   horizontal_evenly_distributed_box_widget_set_w,
    .set_h  =   horizontal_box_widget_set_h
};

static widget_appearence_function_set horizontal_normally_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   horizontal_box_widget_min_w,
    .min_h  =   horizontal_box_widget_min_h,
    .set_w  =   horizontal_normally_distributed_box_widget_set_w,
    .set_h  =   horizontal_box_widget_set_h
};

static widget_appearence_function_set horizontal_all_same_distributed_box_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   horizontal_all_same_distributed_box_widget_min_w,
    .min_h  =   horizontal_box_widget_min_h,
    .set_w  =   horizontal_all_same_distributed_box_widget_set_w,
    .set_h  =   horizontal_box_widget_set_h
};

















widget * create_box(struct widget_context* context, widget_layout layout,widget_distribution distribution)
{
    widget * w=create_container(context, sizeof(widget_container));///actually is just pure container

    if(layout==WIDGET_VERTICAL)
    {
        if(distribution==WIDGET_FIRST_DISTRIBUTED)          w->base.appearence_functions=&vertical_first_distributed_box_functions;
        else if(distribution==WIDGET_LAST_DISTRIBUTED)      w->base.appearence_functions=&vertical_last_distributed_box_functions;
        else if(distribution==WIDGET_EVENLY_DISTRIBUTED)    w->base.appearence_functions=&vertical_evenly_distributed_box_functions;
        else if(distribution==WIDGET_NORMALLY_DISTRIBUTED)  w->base.appearence_functions=&vertical_normally_distributed_box_functions;
        else if(distribution==WIDGET_ALL_SAME_DISTRIBUTED)  w->base.appearence_functions=&vertical_all_same_distributed_box_functions;
    }
    if(layout==WIDGET_HORIZONTAL)
    {
        if(distribution==WIDGET_FIRST_DISTRIBUTED)          w->base.appearence_functions=&horizontal_first_distributed_box_functions;
        else if(distribution==WIDGET_LAST_DISTRIBUTED)      w->base.appearence_functions=&horizontal_last_distributed_box_functions;
        else if(distribution==WIDGET_EVENLY_DISTRIBUTED)    w->base.appearence_functions=&horizontal_evenly_distributed_box_functions;
        else if(distribution==WIDGET_NORMALLY_DISTRIBUTED)  w->base.appearence_functions=&horizontal_normally_distributed_box_functions;
        else if(distribution==WIDGET_ALL_SAME_DISTRIBUTED)  w->base.appearence_functions=&horizontal_all_same_distributed_box_functions;
    }

    return w;
}




