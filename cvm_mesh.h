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



#define CVM_MESH_SIMPLE             0x00000000
#define CVM_MESH_ADGACENCY          0x00000001
#define CVM_MESH_PER_FACE_COLOUR    0x00000002
//#define MESH_GROUP_VERTEX_NORMS     0x00000004
//#define MESH_GROUP_UV               0x00000008

int cvm_mesh_generate_file_from_objs(const char * name,uint32_t flags);


typedef struct cvm_mesh
{
//    uint32_t index;
//    uint32_t colour_offset;
}
cvm_mesh;




cvm_mesh cvm_mesh_load_file(char * filename);


//void initialise_assorted_meshes(gl_functions * glf);
//
//
//
//
//
//
//
//void initialise_mesh_group(gl_functions * glf,mesh_group_ * mg,uint32_t flags);
//mesh load_mesh_file_to_group(mesh_group_ * mg,const char * filename);///type specifier in mesh file (for adjacency &c.)?
//
//void bind_mesh_group_vertex_buffer(gl_functions * glf,mesh_group_ * mg,GLuint attribute_index);
//void bind_mesh_group_adjacent_vertex_buffer(gl_functions * glf,mesh_group_ * mg,GLuint attribute_index);
//void bind_mesh_group_normal_buffer(gl_functions * glf,mesh_group_ * mg,GLuint attribute_index);
//void bind_mesh_group_uv_buffer(gl_functions * glf,mesh_group_ * mg,GLuint attribute_index);
//
//void transfer_mesh_group_buffer_data(gl_functions * glf,mesh_group_ * mg);
//void transfer_mesh_group_draw_commands(gl_functions * glf,mesh_group_ * mg);
//void delete_mesh_group(gl_functions * glf,mesh_group_ * mg);




#endif




