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
    uint32_t face_count;///implicitly triangles
}
cvm_mesh;

void cvm_mesh_load_file_header(FILE * f,cvm_mesh * mesh);///separate out metadata contents relevant to this operation such that complete mesh, tied to every other part of system, isn't necessary
void cvm_mesh_load_file_body(FILE * f,cvm_mesh * mesh,uint16_t * indices,uint16_t * adjacency,uint16_t * materials,void * vertex_data);

size_t cvm_mesh_get_vertex_data_size(cvm_mesh * mesh);

typedef struct cvm_managed_mesh
{
    cvm_vk_managed_buffer * mb;
    char * filename;

    union
    {
        cvm_vk_dynamic_buffer_allocation * dynamic_allocation;///store so that this can be deleted quickly, will be null if static allocation
        uint64_t static_offset;
    };

    uint16_t dynamic:1;
    ///stages of creation
    uint16_t allocated:1;
    uint16_t loaded:1;
    uint16_t ready:1;
    cvm_vk_availability_token availability_token;///only becomes relevant after moving data to the GPU

    ///precalculate following for speed of access/use
    uint32_t index_offset;
    uint32_t adjacency_offset;
    uint32_t material_offset;
    uint32_t vertex_offset;

    cvm_mesh data;
}
cvm_managed_mesh;

void cvm_managed_mesh_create(cvm_managed_mesh * mm,cvm_vk_managed_buffer * mb,char * filename,uint16_t flags,bool dynamic);
void cvm_managed_mesh_destroy(cvm_managed_mesh * mm);

bool cvm_managed_mesh_load(cvm_managed_mesh * mm);

void cvm_managed_mesh_relinquish(cvm_managed_mesh * mm);

void cvm_managed_mesh_render(cvm_managed_mesh * mm,VkCommandBuffer graphics_cb,uint32_t instance_count,uint32_t instance_offset);///assumes managed buffer used in creation was bound to appropriate points




#endif




