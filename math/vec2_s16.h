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

typedef struct vec2_s16
{
    int16_t x;
    int16_t y;
}
vec2_s16;

///rectangle
static inline vec2_s16 vec2_s16_set(int16_t x,int16_t y)
{
    return (vec2_s16){.x=x, .y=y};
}
static inline vec2_s16 vec2_s16_add(vec2_s16 lhs, vec2_s16 rhs)
{
    return (vec2_s16){.x=lhs.x+rhs.x, .y=lhs.y+rhs.y};
}
static inline vec2_s16 vec2_s16_sub(vec2_s16 lhs, vec2_s16 rhs)
{
    return (vec2_s16){.x=lhs.x-rhs.x, .y=lhs.y-rhs.y};
}


