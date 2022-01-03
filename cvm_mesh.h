/**
Copyright 2020,2021 Carl van Mastrigt

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



#define CVM_MESH_SIMPLE             0x0000
#define CVM_MESH_ADGACENCY          0x0001
#define CVM_MESH_PER_FACE_MATERIAL  0x0002
#define CVM_MESH_VERTEX_NORMALS     0x0004
#define CVM_MESH_TEXTURE_COORDS     0x0008

int cvm_mesh_generate_file_from_objs(const char * name,uint16_t flags);

typedef struct cvm_mesh
{
    uint16_t flags;
    uint16_t vertex_count;
    uint32_t face_count:20;///implicitly triangles
    uint32_t dynamic:1;
    ///stages of creation
    uint32_t started:1;///must be set false earlier
    uint32_t allocated:1;
    uint32_t ready:1;

    ///precalculate following for speed of access/use
    uint32_t index_offset;
    uint32_t adjacency_offset;
    uint32_t material_offset;
    uint32_t vertex_offset;

    union
    {
        cvm_vk_dynamic_buffer_allocation * dynamic_allocation;///store so that this can be deleted quickly, will be null if static allocation
        uint64_t static_offset;
    };
}
cvm_mesh;

void cvm_mesh_load_file_header(FILE * f,cvm_mesh * mesh);
void cvm_mesh_load_file_body(FILE * f,cvm_mesh * mesh,uint16_t * indices,uint16_t * adjacency,uint16_t * materials,void * vertex_data);

size_t cvm_mesh_get_vertex_data_size(cvm_mesh * mesh);

bool cvm_mesh_load_file(cvm_mesh * mesh,char * filename,uint16_t flags,bool dynamic,cvm_vk_managed_buffer * mb);

void cvm_mesh_relinquish(cvm_mesh * mesh,cvm_vk_managed_buffer * mb);

void cvm_mesh_render(cvm_mesh * mesh,VkCommandBuffer graphics_cb,uint32_t instance_count,uint32_t instance_offset);///assumes managed buffer used in creation was bound to appropriate points




#endif




