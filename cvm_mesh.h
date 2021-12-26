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
    ///reserved 12 bits

    ///precalculate following for speed of access/use
    uint32_t index_offset;
    uint32_t adjacency_offset;
    uint32_t position_offset;
    uint32_t material_offset;
    ///uint32_t normal_offset;
    ///uint32_t texture_coordinate_offset;

    cvm_vk_dynamic_buffer_allocation * allocation;///store so that this can be deleted quickly, will be null if static allocation
}
cvm_mesh;


#define CVM_MESH_FAIL_STAGING_INSUFFICIENT          1
#define CVM_MESH_FAIL_STAGING_TOTAL_INSUFFICIENT    2
#define CVM_MESH_FAIL_DESTINATION_INSUFFICIENT      3
#define CVM_MESH_FAIL_FLAGS_MISMATCH                4
#define CVM_MESH_FAIL_FILE_INVALID                  5
#define CVM_MESH_FAIL_VERSION_MISMATCH              6
#define CVM_MESH_FAIL_FILE_MISSING                  7

int cvm_mesh_load_file_header(FILE * f,cvm_mesh * mesh);
int cvm_mesh_load_file_body(FILE * f,cvm_mesh * mesh,uint16_t * indices,uint16_t * adjacency,uint16_t * materials,float * positions/*,float * normals,uint16_t * texture_uvs*/);

int cvm_mesh_load_file_to_buffer(cvm_mesh * mesh,char * filename,uint16_t flags,bool dynamic,cvm_vk_managed_buffer * mb,cvm_vk_staging_buffer * sb,VkCommandBuffer transfer_cb);






#endif




