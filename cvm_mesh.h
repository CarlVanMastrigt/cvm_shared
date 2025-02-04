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
    uint16_t vertex_count;///use u16 indexing so this MUST be enough
    uint32_t face_count;///implicitly triangles
}
cvm_mesh;

void cvm_mesh_load_file_header(FILE * f,cvm_mesh * mesh);///separate out metadata contents relevant to this operation such that complete mesh, tied to every other part of system, isn't necessary
void cvm_mesh_load_file_body(FILE * f,cvm_mesh * mesh,uint16_t * indices,uint16_t * adjacency,uint16_t * materials,void * vertex_data);

///need efficient way to handle this data when multithreaded

#define CVM_MESH_STATE_INITIAL 0
#define CVM_MESH_STATE_LOADED  1
#define CVM_MESH_STATE_READY   2


///wont bother supporting other/custom mesh structures, can just custom mesh type should that become desirable
typedef struct cvm_mesh_data
{
    uint64_t buffer_offset;///used for both permanent and temporary allocations

    uint32_t face_count;///implicitly triangles, worth having to multiply by 3 every time? (does allow per face data)
    ///following necessary for processing and rendering
    uint16_t vertex_count;///use u16 indexing so this MUST be enough

    uint16_t state:2;/// ini -> load -> ready -> init(upon deletion/release)
    uint16_t is_temporary_allocation:1;/// can be released and re-acquired
    uint16_t is_custom_loader:1;///if faulse use cvm loading defaults and treat load_data as simply the filename pointer (no need to free data)

    uint16_t type:9;///effectively layout of data contained ??

    uint32_t temporary_allocation_index;

    const char * filename;
    cvm_vk_managed_buffer * mb;///could this be put in the payload? is not unreasonable to assume a module has only 1 managed buffer (it REALLY should)
    atomic_uint_fast16_t lock;///to support multithreading, should only become relevant
}
cvm_mesh_data;

/// merge mesh and managed mesh?

typedef struct cvm_managed_mesh
{
    cvm_vk_managed_buffer * mb;///will also need for creation AND deletion, would have to batch or associate with a module (or somehow implicitly know parent buffer) to avoid this
    char * filename;///could clear/free once loaded in retail

    uint64_t buffer_offset;///used for both permanent and temporary allocations
    uint32_t temporary_allocation_index;///not necessarily used

    uint16_t is_temporary_allocation:1;
    ///stages of creation
    uint16_t loaded:1;///data was loaded to staging buffer in preparation for copy
    uint16_t ready:1;
    uint16_t freeing:1;///backing memory was freed, wait on availability token
    uint16_t availability_token;///only becomes relevant after moving data to the GPU

    ///precalculate following for speed of access/use
    uint32_t index_offset;
    uint32_t adjacency_offset;
    uint32_t material_offset;
    uint32_t vertex_offset;

    cvm_mesh data;
}
cvm_managed_mesh;

void cvm_managed_mesh_create(cvm_managed_mesh * mm,cvm_vk_managed_buffer * mb,char * filename,uint16_t flags,bool temporary);
void cvm_managed_mesh_destroy(cvm_managed_mesh * mm);

bool cvm_managed_mesh_load(cvm_managed_mesh * mm,cvm_vk_module_work_payload * work_payload);

void cvm_managed_mesh_dismiss(cvm_managed_mesh * mm,cvm_vk_module_work_payload * work_payload);

///following require buffer to already be bound
void cvm_managed_mesh_render(cvm_managed_mesh * mm,cvm_vk_module_work_payload * work_payload,uint32_t instance_count,uint32_t instance_offset);
void cvm_managed_mesh_adjacency_render(cvm_managed_mesh * mm,cvm_vk_module_work_payload * work_payload,uint32_t instance_count,uint32_t instance_offset);

typedef struct cvm_mesh_data_pos
{
    float pos[3];
}
cvm_mesh_data_pos;

static inline VkFormat cvm_mesh_get_pos_format(void){return VK_FORMAT_R32G32B32_SFLOAT;}


#endif




