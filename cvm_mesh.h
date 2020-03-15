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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_MESH_H
#define CVM_MESH_H

int generate_mesh_from_objs(const char * name,float scale);
int generate_mesh_with_adjacent_from_objs(const char * name,float scale);

typedef enum
{
    SHADOW_MESH_GROUP=0,
    COLOUR_MESH_GROUP,
    TRANSPARENT_SHADOW_MESH_GROUP,
    TRANSPARENT_COLOUR_MESH_GROUP,
    STICKER_BASIC_MESH_GROUP,
    STICKER_CIRCLE_MESH_GROUP,
    STICKER_CONSTRAINED_CIRCLE_MESH_GROUP,
    NUM_MESH_GROUPS
}
mesh_group;

typedef struct mesh
{
    uint32_t index;
    uint32_t colour_offset;
}
mesh;

typedef struct draw_elements_indirect_command_data///Should probably be put in another file, but here is fine for now
{
    GLuint  count;
    GLuint  instance_count;
    GLuint  first_index;
    GLuint  base_vertex;
    GLuint  base_instance;
}
draw_elements_indirect_command_data;

typedef struct mesh_group_
{
    GLuint dcbo;
    GLuint ibo;
    GLuint vbo;
    GLuint cbo;

    uint32_t indexed_colour:1;
    uint32_t adjacents_present:1;
    /// need to also include texture coordinate lookup and

    draw_elements_indirect_command_data * draw_command_buffer;
    uint32_t draw_command_count;
    uint32_t draw_command_space;

    GLshort * index_buffer;
    uint32_t index_count;
    uint32_t index_space;

    GLfloat * vertex_buffer;
    uint32_t vertex_count;
    uint32_t vertex_space;

    GLshort * colour_buffer;
    uint32_t colour_count;
    uint32_t colour_space;
}
mesh_group_;

uint32_t get_mesh_group_count(mesh_group group);

void initialise_mesh_buffers(void);
void transfer_mesh_draw_data(gl_functions * glf);

mesh load_mesh_file(char * filename,mesh_group group);
mesh load_mesh_adjacency_file(char * filename);
mesh load_mesh_transparent_adjacency_file(char * filename);

draw_elements_indirect_command_data * map_mesh_dcbo_for_writing(gl_functions * glf,mesh_group group);
void unmap_mesh_dcbo(gl_functions * glf);

void render_meshes(gl_functions * glf,mesh_group group_to_render);
void bind_mesh_buffers_for_vao(gl_functions * glf);




void initialise_assorted_meshes(gl_functions * glf);
void bind_assorted_for_vao_vec3(gl_functions * glf);
void bind_assorted_for_vao_vec2(gl_functions * glf);

void render_octahedron_colour(gl_functions * glf,uint32_t count);
void render_octahedron_shadow(gl_functions * glf,uint32_t count);

void render_triangular_antiprism_colour(gl_functions * glf,uint32_t count);
void render_triangular_antiprism_shadow(gl_functions * glf,uint32_t count);

void render_square(gl_functions * glf,uint32_t count);

void render_circle_lines(gl_functions * glf,uint32_t count);
void render_cross_lines(gl_functions * glf,uint32_t count);
void render_square_lines(gl_functions * glf,uint32_t count);







#endif




