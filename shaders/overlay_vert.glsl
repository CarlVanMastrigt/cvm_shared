/**
Copyright 2020 Carl van Mastrigt

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

layout (location = 0) in vec2 vertex;
layout (location = 1) in ivec4 data1;
layout (location = 2) in ivec4 data2;
layout (location = 3) in ivec4 data3;

uniform vec2 inv_window_size;

flat out ivec4 data2_;
flat out ivec4 data3_;

void main()
{
    vec4 ps=vec4(data1);
    gl_Position=vec4( ((ps.xy+ps.zw*vertex)*inv_window_size-0.5)*vec2(2.0,-2.0) ,0.0,1.0);

    data2_=data2;
    data3_=data3;
}
