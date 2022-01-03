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

#include "cvm_shared.h"

static uint16_t cvm_mesh_file_signiature=0x53FF;
static uint16_t cvm_mesh_version_number=0x0001;

int cvm_mesh_generate_file_from_objs(const char * name,uint16_t flags)
{
    uint16_t num_verts,num_positions,num_normals,num_tex_coords,current_matreial,k,pm,pa,n,adj_count;
    uint32_t num_faces,i,j;
    size_t interleaved_vertex_data_size;

    bool has_normals,has_tex_coords;

    uint16_t * v_indices;///indices_0
    //uint16_t * normal_indices;
    //uint16_t * tex_coord_indices;
    uint16_t * indices;///final/resultant
    uint16_t * adjacency;

    uint16_t * materials;

    float * positions;
    //float * norms;
    //float * tex_coords;//male uint16_t (unorm coordinates) ??

    void * interleaved_vertex_data;

    FILE * f;

    char buffer[1024];
    uint16_t vb[3];///support for up to 12
    uint16_t vnb[3];///support for up to 12
    uint16_t vtb[3];///support for up to 12


    if(flags&CVM_MESH_VERTEX_NORMALS)
    {
        fprintf(stderr,"MESH NORMAL SUPPORT NYI\n");
        return -1;
    }
    if(flags&CVM_MESH_TEXTURE_COORDS)
    {
        fprintf(stderr,"MESH TEXTURE COORDINATE SUPPORT NYI\n");
        return -1;
    }

    strcpy(buffer,name);
    strcat(buffer,".obj");

    printf("converting to mesh file: %s\n",buffer);

    num_tex_coords=num_normals=num_positions=0;
    num_faces=0;

    f=fopen(buffer,"r");

    if(!f)
    {
        fprintf(stderr,"UNABLE TO LOAD OBJ FILE: %s\n",buffer);
        return -1;
    }

    while(fgets(buffer,1024,f))
    {
        if(buffer[0]=='f' && buffer[1]==' ' && num_faces++ ==0x000FFFFF)
        {
            fprintf(stderr,"TOO MANY FACES IN OBJ: %s\n",name);
            return -1;
        }
        else if(buffer[0]=='v')
        {
            if(buffer[1]==' ' && num_positions++ ==0xFFFF)
            {
                fprintf(stderr,"TOO MANY VERTS IN OBJ: %s\n",name);
                return -1;
            }
            else if(buffer[1]=='n' && buffer[2]==' ' && num_normals++ ==0xFFFF)
            {
                fprintf(stderr,"TOO MANY VERT NORMALS IN OBJ: %s\n",name);
                return -1;
            }
            else if(buffer[1]=='t' && buffer[2]==' ' && num_tex_coords++ ==0xFFFF)
            {
                fprintf(stderr,"TOO MANY TEXTURE COORDINATES IN OBJ: %s\n",name);
                return -1;
            }
        }
    }

    fclose(f);

    if(!num_faces)
    {
        fprintf(stderr,"NO FACES FOUND IN OBJ %s\n",name);
        return -1;
    }
    if(!num_positions)
    {
        fprintf(stderr,"NO VERTS FOUND IN OBJ %s\n",name);
        return -1;
    }
    if(flags&CVM_MESH_VERTEX_NORMALS && !num_normals)
    {
        fprintf(stderr,"NO NORMALS FOUND IN OBJ %s\n",name);
        return -1;
    }
    if(flags&CVM_MESH_TEXTURE_COORDS && !num_tex_coords)
    {
        fprintf(stderr,"NO TEXTURE COORDINATES FOUND IN OBJ %s\n",name);
        return -1;
    }

    printf("num_faces: %u\nnum_positions: %hu\nnum_normals: %hu\nnum_texture_coordinates: %hu\n",num_faces,num_positions,num_normals,num_tex_coords);

    has_normals=num_normals>0;
    has_tex_coords=num_tex_coords>0;


    v_indices=malloc(sizeof(uint16_t)*num_faces*3);

    positions=malloc(sizeof(float)*num_positions*3);

    if(flags&CVM_MESH_ADGACENCY)
    {
        adjacency=malloc(sizeof(uint16_t)*num_faces*6);
    }
    else adjacency=NULL;

    if(flags&CVM_MESH_PER_FACE_MATERIAL)
    {
        materials=malloc(sizeof(uint16_t)*num_faces);
    }
    else materials=NULL;


    num_tex_coords=num_normals=num_positions=num_faces=0;
    current_matreial=0;

    strcpy(buffer,name);
    strcat(buffer,".obj");

    f=fopen(buffer,"r");

    while(fgets(buffer,1024,f))
    {
        if(buffer[0]=='f' && buffer[1]==' ')
        {
            if(has_normals && has_tex_coords)
            {
                if(sscanf(buffer+2,"%hu/%hu/%hu %hu/%hu/%hu %hu/%hu/%hu",vb+0,vtb+0,vnb+0,vb+1,vtb+1,vnb+1,vb+2,vtb+2,vnb+2) != 9)
                {
                    fprintf(stderr,"FAILED LOADING INDICES VTN: %s : %s\n",name,buffer);
                    return -1;
                }
            }
            else if(has_tex_coords)
            {
                if(sscanf(buffer+2,"%hu/%hu %hu/%hu %hu/%hu",vb+0,vtb+0,vb+1,vtb+1,vb+2,vtb+2) != 6)
                {
                    fprintf(stderr,"FAILED LOADING INDICES VT: %s : %s\n",name,buffer);
                    return -1;
                }
            }
            else if(has_normals)
            {
                if(sscanf(buffer+2,"%hu//%hu %hu//%hu %hu//%hu",vb+0,vnb+0,vb+1,vnb+1,vb+2,vnb+2) != 6)
                {
                    fprintf(stderr,"FAILED LOADING INDICES VN: %s : %s\n",name,buffer);
                    return -1;
                }
            }
            else if(sscanf(buffer+2,"%hu %hu %hu",vb+0,vb+1,vb+2) != 3)
            {
                fprintf(stderr,"FAILED LOADING INDICES V: %s : %s\n",name,buffer);
                return -1;
            }

            v_indices[num_faces*3+0]=vb[0];
            v_indices[num_faces*3+1]=vb[1];
            v_indices[num_faces*3+2]=vb[2];

            if(flags&CVM_MESH_PER_FACE_MATERIAL) materials[num_faces]=current_matreial;
            num_faces++;
        }
        else if(buffer[0]=='v')
        {
            if(buffer[1]==' ')
            {
                if(sscanf(buffer+2,"%f %f %f",positions+num_positions*3,positions+num_positions*3+1,positions+num_positions*3+2) != 3)
                {
                    fprintf(stderr,"FAILED LOADING VERTICES : %s : %s\n",name,buffer);
                    return -1;
                }
                num_positions++;
            }
            else if(buffer[1]=='t' && buffer[2]==' ')
            {
                /// texture_coords
            }
            else if(buffer[1]=='n' && buffer[2]==' ')
            {
                /// vertex_normals
            }
        }
        ///check material here!
    }

    ///if handling tex_coords and normals will need to duplicate resources here so that verts match up (NYI)
    ///must be handled before touching adjacency

    ///need to free vertex, normal and texture coordinate indices here

    ///following are hack for now as above is NYI (which would decide size of this buffer)
    indices=v_indices;
    num_verts=num_positions;
    interleaved_vertex_data_size=sizeof(float)*3;
    interleaved_vertex_data=positions;

    fclose(f);

    for(i=0;i<num_faces;i++) indices[i*3+0]-=1,indices[i*3+1]-=1,indices[i*3+2]-=1;

    if(flags & CVM_MESH_ADGACENCY) for(i=0;i<num_faces;i++)
    {
        adjacency[i*6+0]=indices[i*3+0];
        adjacency[i*6+2]=indices[i*3+1];
        adjacency[i*6+4]=indices[i*3+2];

        adj_count=0;

        for(j=0;j<num_faces;j++) if(i!=j)
        {
            pm=0;pa=3;n=0;
            for(k=0;k<3;k++)
            {
                if(indices[i*3+0]==indices[j*3+k])pm+=2,pa-=k,n++;
                if(indices[i*3+1]==indices[j*3+k])pm+=0,pa-=k,n++;
                if(indices[i*3+2]==indices[j*3+k])pm+=4,pa-=k,n++;
            }
            if(n==2)
            {
                adj_count+=pm;
                adjacency[i*6+pm-1]=indices[j*3+pa];
            }
        }

        if(adj_count!=12)puts("UNABLE TO CONSTRUCT ADJACENCY MESH FROM MODEL (IS IT NON-MANIFOLD ?)");
    }



    strcpy(buffer,name);
    strcat(buffer,".mesh");

    f=fopen(buffer,"wb");

    if(!f)
    {
        printf("UNABLE TO CREATE MESH FILE");
        return -1;
    }

    ///header/metadta
    printf("%zu\n",fwrite(&cvm_mesh_file_signiature,sizeof(uint16_t),1,f));
    printf("%zu\n",fwrite(&cvm_mesh_version_number,sizeof(uint16_t),1,f));
    printf("%zu\n",fwrite(&flags,sizeof(uint16_t),1,f));
    printf("%zu\n",fwrite(&num_verts,sizeof(uint16_t),1,f));
    printf("%zu\n",fwrite(&num_faces,sizeof(uint32_t),1,f));

    ///contents
    printf("%zu\n",fwrite(indices,sizeof(uint16_t),num_faces*3,f));
    if(flags & CVM_MESH_ADGACENCY) printf("%zu\n",fwrite(adjacency,sizeof(uint16_t),num_faces*6,f));
    if(flags & CVM_MESH_PER_FACE_MATERIAL) printf("%zu\n",fwrite(materials,sizeof(uint16_t),num_faces,f));
    printf("%zu\n",fwrite(interleaved_vertex_data,interleaved_vertex_data_size,num_verts,f));
    ///write norms
    ///write texture coordinates

    fclose(f);

    free(interleaved_vertex_data);
    free(indices);
    free(adjacency);
    free(materials);

    return 0;
}

size_t cvm_mesh_get_vertex_data_size(cvm_mesh * mesh)
{
    ///will (conditionally) change when texture coordinates and normals are implemented
    return 3*sizeof(float);
}


void cvm_mesh_load_file_header(FILE * f,cvm_mesh * mesh)
{
    uint32_t num_faces;
    uint16_t tmp;

    fread(&tmp,sizeof(uint16_t),1,f);
    if(tmp!=cvm_mesh_file_signiature)
    {
        fprintf(stderr,"ATTEMPTED TO LOAD NON MESH FILE AS MESH FILE (SIGNIATURE MISMATCH)\n");
        exit(-1);
    }

    fread(&tmp,sizeof(uint16_t),1,f);
    if(tmp>cvm_mesh_version_number)
    {
        fprintf(stderr,"MESH FILE VERSION MISMATCH\n");
        exit(-1);
    }

    fread(&mesh->flags,sizeof(uint16_t),1,f);
    fread(&mesh->vertex_count,sizeof(uint16_t),1,f);
    fread(&num_faces,sizeof(uint32_t),1,f);
    mesh->face_count=num_faces;
}

void cvm_mesh_load_file_body(FILE * f,cvm_mesh * mesh,uint16_t * indices,uint16_t * adjacency,uint16_t * materials,void * vertex_data)
{
    fread(indices,sizeof(uint16_t),mesh->face_count*3,f);
    if(mesh->flags&CVM_MESH_ADGACENCY)fread(adjacency,sizeof(uint16_t),mesh->face_count*6,f);
    if(mesh->flags&CVM_MESH_PER_FACE_MATERIAL)fread(materials,sizeof(uint16_t),mesh->face_count,f);
    fread(vertex_data,cvm_mesh_get_vertex_data_size(mesh),mesh->vertex_count,f);
}


bool cvm_mesh_load_file(cvm_mesh * mesh,char * filename,uint16_t flags,bool dynamic,cvm_vk_managed_buffer * mb)
{
    uint16_t *indices,*adjacency,*materials;
    void * vertex_data;
    uint64_t base_offset,current_offset;
    char * ptr;
    size_t vertex_data_size;
    uint32_t size;
    FILE * f;
    VkDeviceSize sb_offset;

    if(!mesh->started)
    {
        mesh->started=true;
        mesh->allocated=false;
        mesh->ready=false;
        mesh->dynamic=dynamic;
    }
    else if(dynamic!=mesh->dynamic)
    {
        fprintf(stderr,"TRIED TO CREATE MESH WITH DIFFERENT DYNAMIC SETTING TO WHEN MESH STARTED CREATION\n");
        exit(-1);
    }

    f=fopen(filename,"rb");
    if(!f)
    {
        fprintf(stderr,"MESH FILE MISSING: %s\n",filename);
        exit(-1);
    }

    cvm_mesh_load_file_header(f,mesh);

    #warning perhaps have way to just take all relevant flags from the mesh? (special mesh flag?)
    if((mesh->flags&flags)!=flags)
    {
        fprintf(stderr,"ATTEMPTED TO LOAD MESH WITH FLAGS IT WAS NOT CREATED WITH\n");
        exit(-1);
    }

    mesh->flags=flags;

    vertex_data_size=cvm_mesh_get_vertex_data_size(mesh);

    size=mesh->face_count*3*sizeof(uint16_t);
    if(mesh->flags&CVM_MESH_ADGACENCY)size+=mesh->face_count*6*sizeof(uint16_t);
    if(mesh->flags&CVM_MESH_PER_FACE_MATERIAL)size+=mesh->face_count*sizeof(uint16_t);
    size+=(mesh->vertex_count+1)*vertex_data_size;///1 extra needed for alignment

    if(dynamic)
    {
        if(!mesh->allocated)
        {
            mesh->dynamic_allocation=cvm_vk_managed_buffer_acquire_dynamic_allocation(mb,size);

            if(!mesh->dynamic_allocation)
            {
                fprintf(stderr,"INSUFFICIENT SPACE REMAINING IN BUFFER FOR MESH: %s\n",filename);
                exit(-1);
            }

            mesh->allocated=true;
        }

        #warning should probably be more specific with the stage and flags bits... (if possible)
        ptr=cvm_vk_managed_buffer_get_dynamic_allocation_mapping(mb,mesh->dynamic_allocation,size,VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,VK_ACCESS_2_MEMORY_READ_BIT_KHR);
        if(!ptr)return false;///could not allocate staging space!
        base_offset = current_offset = cvm_vk_managed_buffer_get_dynamic_allocation_offset(mb,mesh->dynamic_allocation);
    }
    else
    {
        if(!mesh->allocated)
        {
            mesh->static_offset = cvm_vk_managed_buffer_acquire_static_allocation(mb,size,2);

            if(!mesh->static_offset)
            {
                fprintf(stderr,"INSUFFICIENT SPACE REMAINING IN BUFFER FOR MESH: %s\n",filename);
                exit(-1);
            }

            mesh->allocated=true;
        }

        #warning should probably be more specific with the stage and flags bits... (if possible)
        ptr=cvm_vk_managed_buffer_get_static_allocation_mapping(mb,mesh->static_offset,size,VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,VK_ACCESS_2_MEMORY_READ_BIT_KHR);
        if(!ptr)return false;///could not allocate staging space!
        base_offset = current_offset = mesh->static_offset;
    }


    mesh->index_offset=current_offset/sizeof(uint16_t);
    indices=(uint16_t*)ptr;
    current_offset+=mesh->face_count*3*sizeof(uint16_t);

    if(mesh->flags&CVM_MESH_ADGACENCY)
    {
        mesh->adjacency_offset=current_offset/sizeof(uint16_t);
        adjacency=(uint16_t*)(ptr + (current_offset-base_offset));
        current_offset+=mesh->face_count*6*sizeof(uint16_t);
    }
    else adjacency=NULL;

    if(mesh->flags&CVM_MESH_PER_FACE_MATERIAL)
    {
        mesh->material_offset=current_offset/sizeof(uint16_t);
        materials=(uint16_t*)(ptr + (current_offset-base_offset));
        current_offset+=mesh->face_count*sizeof(uint16_t);
    }
    else materials=NULL;

    current_offset=(current_offset-1) / vertex_data_size + 1;
    mesh->vertex_offset=current_offset;
    current_offset*=vertex_data_size;
    vertex_data=(ptr + (current_offset-base_offset));
    current_offset+=mesh->vertex_count*vertex_data_size;

    cvm_mesh_load_file_body(f,mesh,indices,adjacency,materials,vertex_data);

    mesh->ready=true;

    return true;
}

void cvm_mesh_relinquish(cvm_mesh * mesh,cvm_vk_managed_buffer * mb)
{
    if(mesh->allocated)
    {
        if(mesh->dynamic)
        {
            cvm_vk_managed_buffer_relinquish_dynamic_allocation(mb,mesh->dynamic_allocation);
        }
    }
}

void cvm_mesh_render(cvm_mesh * mesh,VkCommandBuffer graphics_cb,uint32_t instance_count,uint32_t instance_offset)///assumes managed buffer used in creation was bound to appropriate points
{
    if(mesh->ready)vkCmdDrawIndexed(graphics_cb,mesh->face_count*3,instance_count,mesh->index_offset,mesh->vertex_offset,instance_offset);
}

//static GLuint assorted_ibo;
//static GLuint assorted_vbo;
//
//static GLuint assorted_line_vbo;
//
//
///// 6 - 8
//
//
//static GLfloat assorted_vertices[]=
//{
//    ///octahedron (bounds sphere of radius 1, centred on origin)
//    0.0,0.0,SQRT_3,
//    -SQRT_3,0.0,0.0,
//    0.0,SQRT_3,0.0,
//    SQRT_3,0.0,0.0,
//    0.0,0.0,-SQRT_3,
//    0.0,-SQRT_3,0.0,
//    ///triangular antiprism (bounds sphere of radius 1 and length 1, with its base centred on origin)
//    0.0,-2.0,0.0,
//    -SQRT_3,1.0,0.0,
//    SQRT_3,1.0,0.0,
//    -SQRT_3,-1.0,1.0,
//    0.0,2.0,1.0,
//    SQRT_3,-1.0,1.0
//};
//
//static GLushort assorted_indices[]=
//{
//    ///octahedron
//    1,0,2,
//    3,2,0,
//    3,4,2,
//    1,2,4,
//    3,0,5,
//    1,5,0,
//    3,5,4,
//    1,4,5,
//    1,5,0,3,2,4,
//    3,4,2,1,0,5,
//    3,5,4,1,2,0,
//    1,0,2,3,4,5,
//    3,2,0,1,5,4,
//    1,4,5,3,0,2,
//    3,0,5,1,4,2,
//    1,2,4,3,5,0,
//    ///triangular antiprism
//    6,7,8,
//    6,9,7,
//    9,10,7,
//    7,10,8,
//    10,11,8,
//    8,11,6,
//    11,9,6,
//    11,10,9,
//    6,9,7,10,8,11,
//    6,11,9,10,7,8,
//    9,11,10,8,7,6,
//    7,9,10,11,8,6,
//    10,9,11,6,8,7,
//    8,10,11,9,6,7,
//    11,10,9,7,6,8,
//    11,8,10,7,9,6
//};
//
//static const uint32_t circle_divisions=64;
//
//static GLfloat assorted_line_vertices[]=
//{
//    0.5,0.5,
//    1.5,1.5,
//    -0.5,0.5,
//    -1.5,1.5,
//    0.5,-0.5,
//    1.5,-1.5,
//    -0.5,-0.5,
//    -1.5,-1.5,
//
//    1.5,0.0,
//    0.0,1.5,
//    -1.5,0.0,
//    0.0,-1.5,
//
//    1.0,1.0,
//    -1.0,1.0,
//    -1.0,-1.0,
//    -1.0,-1.0,
//    1.0,-1.0,
//    1.0,1.0
//};
//
//void initialise_assorted_meshes(gl_functions * glf)
//{
//    glf->glGenBuffers(1,&assorted_vbo);
//    glf->glBindBuffer(GL_ARRAY_BUFFER,assorted_vbo);
//    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*36,assorted_vertices,GL_STATIC_DRAW);
//    glf->glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//    glf->glGenBuffers(1,&assorted_ibo);
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,assorted_ibo);
//    glf->glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(GLushort)*144,assorted_indices,GL_STATIC_DRAW);
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
//
//
//
//    GLfloat line_verts[36+2*circle_divisions];
//
//    uint32_t i;
//    float angle;
//
//    for(i=0;i<36;i++)
//    {
//        line_verts[i]=assorted_line_vertices[i];
//    }
//
//    for(i=0;i<circle_divisions;i++)
//    {
//        angle=((float)i)*2.0*PI/((float)circle_divisions);
//        line_verts[36+i*2+0]=cosf(angle);
//        line_verts[36+i*2+1]=sinf(angle);
//    }
//
//
//    glf->glGenBuffers(1,&assorted_line_vbo);
//    glf->glBindBuffer(GL_ARRAY_BUFFER,assorted_line_vbo);
//    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*(36+2*circle_divisions),line_verts,GL_STATIC_DRAW);
//    glf->glBindBuffer(GL_ARRAY_BUFFER, 0);
//}
//
//void bind_assorted_for_vao_vec3(gl_functions * glf)
//{
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, assorted_ibo);
//
//    glf->glBindBuffer(GL_ARRAY_BUFFER,assorted_vbo);
//    glf->glEnableVertexAttribArray(0);
//    glf->glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
//}
//
//void bind_assorted_for_vao_vec2(gl_functions * glf)
//{
//    glf->glBindBuffer(GL_ARRAY_BUFFER,assorted_line_vbo);
//    glf->glEnableVertexAttribArray(0);
//    glf->glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
//}
//
//void render_octahedron_colour(gl_functions * glf,uint32_t count)
//{
//    glf->glDrawElementsInstanced(GL_TRIANGLES,24,GL_UNSIGNED_SHORT,NULL,count);
//}
//
//void render_octahedron_shadow(gl_functions * glf,uint32_t count)
//{
//    glf->glDrawElementsInstanced(GL_TRIANGLES_ADJACENCY,48,GL_UNSIGNED_SHORT,(const void *)(24*sizeof(GLushort)),count);
//}
//
//void render_triangular_antiprism_colour(gl_functions * glf,uint32_t count)
//{
//    glf->glDrawElementsInstanced(GL_TRIANGLES,24,GL_UNSIGNED_SHORT,(const void *)(72*sizeof(GLushort)),count);
//}
//
//void render_triangular_antiprism_shadow(gl_functions * glf,uint32_t count)
//{
//    glf->glDrawElementsInstanced(GL_TRIANGLES_ADJACENCY,48,GL_UNSIGNED_SHORT,(const void *)(96*sizeof(GLushort)),count);
//}
//
//
//
//void render_cross_lines(gl_functions * glf,uint32_t count)
//{
//    glf->glDrawArraysInstanced(GL_LINES,0,8,count);
//}
//
//void render_square_lines(gl_functions * glf,uint32_t count)
//{
//    glf->glDrawArraysInstanced(GL_LINE_LOOP,8,4,count);
//}
//
//void render_square(gl_functions * glf,uint32_t count)
//{
//    glf->glDrawArraysInstanced(GL_TRIANGLES,12,6,count);
//}
//
//void render_circle_lines(gl_functions * glf,uint32_t count)
//{
//    glf->glDrawArraysInstanced(GL_LINE_LOOP,36,circle_divisions,count);
//}
