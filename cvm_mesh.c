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

#include "cvm_shared.h"


int cvm_mesh_generate_file_from_objs(const char * name,uint32_t flags)
{
    uint32_t num_verts,num_faces;


    uint16_t * indices;
    float * verts;
    uint16_t * adj_indices;
    uint16_t * colours;
    //float * norms;
    //float * uvs;

    FILE *f_in,*f_out;

    uint32_t i,j,k,current_colour,pm,pa,n,adj_count;

    char c;

    char buffer[1024];

    strcpy(buffer,name);
    strcat(buffer,".obj");

    printf("converting to mesh file: %s\n",buffer);

    num_verts=num_faces=0;

    f_in=fopen(buffer,"r");

    if(f_in==NULL)
    {
        puts("UNABLE TO LOAD OBJ FILE");
        return -1;
    }


    for(c='a';c !=EOF;c=fgetc(f_in))
    {
        if (c=='\n')
        {
            c=fgetc(f_in);
            num_verts+=(c=='v');
            num_faces+=(c=='f');
        }
    }

    fclose(f_in);

    printf("num_verts: %d\nnum_faces: %d\n",num_verts,num_faces);

    indices=malloc(sizeof(uint16_t)*num_faces*3);
    verts=malloc(sizeof(float)*num_verts*3);
    adj_indices=malloc(sizeof(uint16_t)*num_faces*6);
    colours=malloc(sizeof(uint16_t)*num_faces);


    num_verts=num_faces=0;
    current_colour=0;

    f_in=fopen(buffer,"r");

    for(c='a';c!=EOF;c=fgetc(f_in))
    {
        #warning could really do with error checking here, use switch statement deviate behaviour??
        if (c=='\n')
        {
            c=fgetc(f_in);
            if(c=='v')
            {
                if(fscanf(f_in,"%f %f %f",verts+num_verts*3,verts+num_verts*3+1,verts+num_verts*3+2)!=3)puts("FAILED LOADING FACE INDICES");
                num_verts++;
            }
            else if(c=='f')
            {
                if(fscanf(f_in,"%hu %hu %hu",indices+num_faces*3,indices+num_faces*3+1,indices+num_faces*3+2)!=3)puts("FAILED LOADING VERTEXES");
                colours[num_faces]=current_colour;
                num_faces++;
            }
            else if(c=='u') if(fscanf(f_in,"semtl %u",&current_colour)!=1)puts("FAILED LOADING MATERIAL (COLOUR)");
        }
    }

    fclose(f_in);

    for(i=0;i<num_faces;i++) indices[i*3+0]-=1,indices[i*3+1]-=1,indices[i*3+2]-=1;

    if(flags & CVM_MESH_ADGACENCY) for(i=0;i<num_faces;i++)
    {
        adj_indices[i*6+0]=indices[i*3+0];
        adj_indices[i*6+2]=indices[i*3+1];
        adj_indices[i*6+4]=indices[i*3+2];

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
                adj_indices[i*6+pm-1]=indices[j*3+pa];
            }
        }

        if(adj_count!=12)puts("UNABLE TO CONSTRUCT ADJACENCY MESH FROM MODEL (IS IT NON-MANIFOLD ?)");
    }

    strcpy(buffer,name);
    strcat(buffer,".mesh");
    printf("%s\n",buffer);



    f_out=fopen(buffer,"wb");

    if(f_out==NULL)
    {
        printf("UNABLE TO CREATE MESH FILE");
        return -1;
    }

    printf("%zu\n",fwrite(&flags,sizeof(uint32_t),1,f_out));
    printf("%zu\n",fwrite(&num_faces,sizeof(uint32_t),1,f_out));
    printf("%zu\n",fwrite(&num_verts,sizeof(uint32_t),1,f_out));

    ///required elements
    printf("%zu\n",fwrite(indices,sizeof(uint16_t),num_faces*3,f_out));
    printf("%zu\n",fwrite(verts,sizeof(float),num_verts*3,f_out));
    ///conditional elements
    if(flags & CVM_MESH_ADGACENCY) printf("%zu\n",fwrite(adj_indices,sizeof(uint16_t),num_faces*6,f_out));
    if(flags & CVM_MESH_PER_FACE_COLOUR) printf("%zu\n",fwrite(colours,sizeof(uint16_t),num_faces,f_out));
    ///write norms
    ///write uvs

    fclose(f_out);

    free(verts);
    free(indices);
    free(adj_indices);
    free(colours);

    return 0;
}















//mesh load_mesh_file(char * filename,mesh_group group)
//{
//    mesh rtrn=(mesh){0,0};
//
//    char * tmp=filename;
//
//    while(*tmp!='.')
//    {
//        if(*tmp=='\0')
//        {
//            puts("error loading mesh file: not of correct type");
//            return rtrn;
//        }
//        tmp++;
//    }
//
//    if(strcmp(tmp,".cvm_mesh"))
//    {
//        puts("error loading mesh file: not of correct type");
//        return rtrn;
//    }
//
//
//
//    FILE * f_in;
//    f_in=fopen(filename,"rb");
//    uint32_t num_verts,num_faces,num_parts,i,j,k;
//    uint32_t *part_ids,*part_face_counts;
//
//
//    num_verts=num_faces=num_parts=0;
//
//    if(group==SHADOW_MESH_GROUP)
//    {
//        puts("error loading mesh file: cannot load regular mesh file to shadow group");
//        return rtrn;
//    }
//
//    if(group==COLOUR_MESH_GROUP)
//    {
//        puts("error loading mesh file: cannot load regular mesh file to colour group as this group shares data with the shadow group");
//        return rtrn;
//    }
//
//
//
//    if (f_in!=NULL)
//    {
//        if(mesh_draw_data_count[group]==mesh_draw_data_space[group])
//        {
//            mesh_draw_data_space[group]*=2;
//            mesh_draw_data[group]=realloc(mesh_draw_data[group],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[group]);
//        }
//
//        fread(&num_faces, sizeof(uint32_t), 1, f_in);
//        fread(&num_verts, sizeof(uint32_t), 1, f_in);
//        fread(&num_parts, sizeof(uint32_t), 1, f_in);
//
//        if((mesh_index_count+num_faces*3) >= mesh_index_space)
//        {
//            while((mesh_index_count+num_faces*3) >= mesh_index_space)mesh_index_space*=2;
//
//            mesh_indices=realloc(mesh_indices,sizeof(uint16_t)*mesh_index_space);
//        }
//
//        if((mesh_vertex_count+num_verts) >= mesh_vertex_space)
//        {
//            while((mesh_vertex_count+num_verts) >= mesh_vertex_space)mesh_vertex_space*=2;
//
//            mesh_vertices=realloc(mesh_vertices,sizeof(float)*mesh_vertex_space*3);
//        }
//
//        if((mesh_colour_count+num_faces) >= mesh_colour_space)
//        {
//            while((mesh_colour_count+num_faces) >= mesh_colour_space)mesh_colour_space*=2;
//
//            mesh_colours=realloc(mesh_colours,sizeof(uint16_t)*mesh_colour_space);
//        }
//
//
//
//
//        part_ids=malloc(sizeof(uint32_t)*num_parts);
//        part_face_counts=malloc(sizeof(uint32_t)*num_parts);
//
//        fread(part_ids, sizeof(uint32_t), num_parts, f_in);
//        fread(part_face_counts, sizeof(uint32_t), num_parts, f_in);
//
//        k=0;
//        for(i=0;i<num_parts;i++)
//        {
//            for(j=0;j<part_face_counts[i];j++)
//            {
//                mesh_colours[mesh_colour_count+k]=part_ids[i];
//                k++;
//            }
//        }
//
//        free(part_ids);
//        free(part_face_counts);
//
//
//
//        fread(mesh_indices+mesh_index_count, sizeof(uint16_t), num_faces*3, f_in);
//        fread(mesh_vertices+(mesh_vertex_count*3), sizeof(float), num_verts*3, f_in);
//
//
//        mesh_draw_data[group][mesh_draw_data_count[group]].base_instance=0;///set_elsewhere
//        mesh_draw_data[group][mesh_draw_data_count[group]].instance_count=0;///set_elsewhere
//
//        mesh_draw_data[group][mesh_draw_data_count[group]].base_vertex=mesh_vertex_count;
//        mesh_draw_data[group][mesh_draw_data_count[group]].count=num_faces*3;
//        mesh_draw_data[group][mesh_draw_data_count[group]].first_index=mesh_index_count;
//
//
//        rtrn.index=mesh_draw_data_count[group];
//        rtrn.colour_offset=mesh_colour_count;
//
//        mesh_draw_data_count[group]++;
//        mesh_vertex_count+=num_verts;
//        mesh_index_count+=num_faces*3;
//        mesh_colour_count+=num_faces;
//    }
//
//    fclose(f_in);
//
//    return rtrn;
//}
//
//
//mesh load_mesh_adjacency_file(char * filename)
//{
//    mesh rtrn=(mesh){0,0};
//    char * tmp=filename;
//
//    while(*tmp!='.')
//    {
//        if(*tmp=='\0')
//        {
//            puts("error loading mesh file: not of correct type (no extension)");
//            return rtrn;
//        }
//        tmp++;
//    }
//
//    if(strcmp(tmp,".cvm_mesh_adj"))
//    {
//        printf("error loading mesh file: not of correct type (wrong extension) %s",tmp);
//        return rtrn;
//    }
//
//
//
//    FILE * f_in;
//    f_in=fopen(filename,"rb");
//    uint32_t num_verts,num_faces,num_parts,i,j,k;
//    uint32_t *part_ids,*part_face_counts;
//
//
//    num_verts=num_faces=num_parts=0;
//
//
//
//    if (f_in==NULL)
//    {
//        printf("error mesh file not found: %s\n",filename);
//    }
//    else
//    {
//        if(mesh_draw_data_count[SHADOW_MESH_GROUP]==mesh_draw_data_space[SHADOW_MESH_GROUP])
//        {
//            mesh_draw_data_space[SHADOW_MESH_GROUP]*=2;
//            mesh_draw_data[SHADOW_MESH_GROUP]=realloc(mesh_draw_data[SHADOW_MESH_GROUP],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[SHADOW_MESH_GROUP]);
//        }
//
//        if(mesh_draw_data_count[COLOUR_MESH_GROUP]==mesh_draw_data_space[COLOUR_MESH_GROUP])
//        {
//            mesh_draw_data_space[COLOUR_MESH_GROUP]*=2;
//            mesh_draw_data[COLOUR_MESH_GROUP]=realloc(mesh_draw_data[COLOUR_MESH_GROUP],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[COLOUR_MESH_GROUP]);
//        }
//
//        fread(&num_faces, sizeof(uint32_t), 1, f_in);
//        fread(&num_verts, sizeof(uint32_t), 1, f_in);
//        fread(&num_parts, sizeof(uint32_t), 1, f_in);
//
//        if((mesh_index_count+num_faces*9) >= mesh_index_space)
//        {
//            while((mesh_index_count+num_faces*9) >= mesh_index_space)mesh_index_space*=2;
//
//            mesh_indices=realloc(mesh_indices,sizeof(uint16_t)*mesh_index_space);
//        }
//
//        if((mesh_vertex_count+num_verts) >= mesh_vertex_space)
//        {
//            while((mesh_vertex_count+num_verts) >= mesh_vertex_space)mesh_vertex_space*=2;
//
//            mesh_vertices=realloc(mesh_vertices,sizeof(float)*mesh_vertex_space*3);
//        }
//
//        if((mesh_colour_count+num_faces) >= mesh_colour_space)
//        {
//            while((mesh_colour_count+num_faces) >= mesh_colour_space)mesh_colour_space*=2;
//
//            mesh_colours=realloc(mesh_colours,sizeof(uint16_t)*mesh_colour_space);
//        }
//
//
//
//
//        part_ids=malloc(sizeof(uint32_t)*num_parts);
//        part_face_counts=malloc(sizeof(uint32_t)*num_parts);
//
//        fread(part_ids, sizeof(uint32_t), num_parts, f_in);
//        fread(part_face_counts, sizeof(uint32_t), num_parts, f_in);
//
//        k=0;
//        for(i=0;i<num_parts;i++)
//        {
//            for(j=0;j<part_face_counts[i];j++)
//            {
//                mesh_colours[mesh_colour_count+k]=part_ids[i];
//                k++;
//            }
//        }
//
//        free(part_ids);
//        free(part_face_counts);
//
//
//
//        fread(mesh_indices+mesh_index_count, sizeof(uint16_t), num_faces*6, f_in);
//        fread(mesh_vertices+(mesh_vertex_count*3), sizeof(float), num_verts*3, f_in);
//
//
//        for(i=0;i<num_faces;i++)
//        {
//            mesh_indices[mesh_index_count+num_faces*6+i*3+0]=mesh_indices[mesh_index_count+i*6+0];
//            mesh_indices[mesh_index_count+num_faces*6+i*3+1]=mesh_indices[mesh_index_count+i*6+2];
//            mesh_indices[mesh_index_count+num_faces*6+i*3+2]=mesh_indices[mesh_index_count+i*6+4];
//        }
//
//        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].base_instance=0;///set_elsewhere
//        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].instance_count=0;///set_elsewhere
//
//        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].base_vertex=mesh_vertex_count;
//        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].count=num_faces*6;
//        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].first_index=mesh_index_count;
//
//        mesh_draw_data_count[SHADOW_MESH_GROUP]++;
//        mesh_index_count+=num_faces*6;
//
//
//
//        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].base_instance=0;///set_elsewhere
//        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].instance_count=0;///set_elsewhere
//
//        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].base_vertex=mesh_vertex_count;
//        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].count=num_faces*3;
//        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].first_index=mesh_index_count;
//
//        rtrn.index=mesh_draw_data_count[COLOUR_MESH_GROUP];
//        mesh_draw_data_count[COLOUR_MESH_GROUP]++;
//        mesh_index_count+=num_faces*3;
//
//
//
//        rtrn.colour_offset=mesh_colour_count;
//
//        mesh_vertex_count+=num_verts;
//        mesh_colour_count+=num_faces;
//
//        if(mesh_draw_data_count[COLOUR_MESH_GROUP]!=mesh_draw_data_count[SHADOW_MESH_GROUP])
//        {
//            puts("error somehow loaded to colour or shadow mesh group without also loading to the other");
//        }
//    }
//
//    fclose(f_in);
//
//    return rtrn;
//}
//
//mesh load_mesh_transparent_adjacency_file(char * filename)
//{
//    mesh rtrn=(mesh){0,0};
//    char * tmp=filename;
//
//    while(*tmp!='.')
//    {
//        if(*tmp=='\0')
//        {
//            puts("error loading mesh file: not of correct type (no extension)");
//            return rtrn;
//        }
//        tmp++;
//    }
//
//    if(strcmp(tmp,".cvm_mesh_adj"))
//    {
//        printf("error loading mesh file: not of correct type (wrong extension) %s",tmp);
//        return rtrn;
//    }
//
//
//
//    FILE * f_in;
//    f_in=fopen(filename,"rb");
//    uint32_t num_verts,num_faces,num_parts,i;
//    uint32_t *part_ids,*part_face_counts;
//
//
//    num_verts=num_faces=num_parts=0;
//
//
//
//    if (f_in==NULL)
//    {
//        printf("error mesh file not found: %s\n",filename);
//    }
//    else
//    {
//        if(mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]==mesh_draw_data_space[TRANSPARENT_SHADOW_MESH_GROUP])
//        {
//            mesh_draw_data_space[TRANSPARENT_SHADOW_MESH_GROUP]*=2;
//            mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP]=realloc(mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[TRANSPARENT_SHADOW_MESH_GROUP]);
//        }
//
//        if(mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]==mesh_draw_data_space[TRANSPARENT_COLOUR_MESH_GROUP])
//        {
//            mesh_draw_data_space[TRANSPARENT_COLOUR_MESH_GROUP]*=2;
//            mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP]=realloc(mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[TRANSPARENT_COLOUR_MESH_GROUP]);
//        }
//
//        fread(&num_faces, sizeof(uint32_t), 1, f_in);
//        fread(&num_verts, sizeof(uint32_t), 1, f_in);
//        fread(&num_parts, sizeof(uint32_t), 1, f_in);
//
//        if((mesh_index_count+num_faces*9) >= mesh_index_space)
//        {
//            while((mesh_index_count+num_faces*9) >= mesh_index_space)mesh_index_space*=2;
//
//            mesh_indices=realloc(mesh_indices,sizeof(uint16_t)*mesh_index_space);
//        }
//
//        if((mesh_vertex_count+num_verts) >= mesh_vertex_space)
//        {
//            while((mesh_vertex_count+num_verts) >= mesh_vertex_space)mesh_vertex_space*=2;
//
//            mesh_vertices=realloc(mesh_vertices,sizeof(float)*mesh_vertex_space*3);
//        }
//
//
//
//
//        part_ids=malloc(sizeof(uint32_t)*num_parts);
//        part_face_counts=malloc(sizeof(uint32_t)*num_parts);
//
//        fread(part_ids, sizeof(uint32_t), num_parts, f_in);
//        fread(part_face_counts, sizeof(uint32_t), num_parts, f_in);
//
//        free(part_ids);
//        free(part_face_counts);
//
//
//
//        fread(mesh_indices+mesh_index_count, sizeof(uint16_t), num_faces*6, f_in);
//        fread(mesh_vertices+(mesh_vertex_count*3), sizeof(float), num_verts*3, f_in);
//
//
//        for(i=0;i<num_faces;i++)
//        {
//            mesh_indices[mesh_index_count+num_faces*6+i*3+0]=mesh_indices[mesh_index_count+i*6+0];
//            mesh_indices[mesh_index_count+num_faces*6+i*3+1]=mesh_indices[mesh_index_count+i*6+2];
//            mesh_indices[mesh_index_count+num_faces*6+i*3+2]=mesh_indices[mesh_index_count+i*6+4];
//        }
//
//        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].base_instance=0;///set_elsewhere
//        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].instance_count=0;///set_elsewhere
//
//        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].base_vertex=mesh_vertex_count;
//        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].count=num_faces*6;
//        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].first_index=mesh_index_count;
//
//        mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]++;
//        mesh_index_count+=num_faces*6;
//
//
//
//        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].base_instance=0;///set_elsewhere
//        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].instance_count=0;///set_elsewhere
//
//        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].base_vertex=mesh_vertex_count;
//        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].count=num_faces*3;
//        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].first_index=mesh_index_count;
//
//        rtrn.index=mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP];
//        mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]++;
//        mesh_index_count+=num_faces*3;
//
//
//
//        rtrn.colour_offset=0;
//
//        mesh_vertex_count+=num_verts;
//
//        if(mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]!=mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP])
//        {
//            puts("error somehow loaded to colour or shadow mesh group without also loading to the other");
//        }
//    }
//
//    fclose(f_in);
//
//    return rtrn;
//}
//
//
//
//
//
//
//
//
//static GLuint mesh_ibo=0;
//static GLuint mesh_vbo=0;
//static GLuint mesh_cbo=0;
//static GLuint mesh_dcbo=0;
//
//void transfer_mesh_draw_data(gl_functions * glf)
//{
//    glf->glGenBuffers(1,&mesh_ibo);
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_ibo);
//    glf->glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(uint16_t)*mesh_index_count,mesh_indices,GL_STATIC_DRAW);
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
//
//    glf->glGenBuffers(1,&mesh_vbo);
//    glf->glBindBuffer(GL_ARRAY_BUFFER,mesh_vbo);
//    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(float)*mesh_vertex_count*3,mesh_vertices,GL_STATIC_DRAW);
//    glf->glBindBuffer(GL_ARRAY_BUFFER,0);
//
//    glf->glGenTextures(1,&mesh_cbo);
//    glf->glBindTexture(GL_TEXTURE_1D,mesh_cbo);
//    glf->glTexImage1D(GL_TEXTURE_1D,0,GL_R16UI,mesh_colour_count,0,GL_RED_INTEGER,GL_UNSIGNED_SHORT,mesh_colours);
//    glf->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glf->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glf->glBindTexture(GL_TEXTURE_1D,0);
//
//
//
//    int i;
//    uint32_t total_size=0;
//
//    for(i=0;i<NUM_MESH_GROUPS;i++)
//    {
//        mesh_draw_data_offset[i]=total_size;
//        total_size+=mesh_draw_data_count[i];
//    }
//    total_size*=sizeof(draw_elements_indirect_command_data);
//
//    glf->glGenBuffers(1,&mesh_dcbo);
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,mesh_dcbo);
//    glf->glBufferStorage(GL_DRAW_INDIRECT_BUFFER,total_size,NULL,GL_MAP_WRITE_BIT);
//    draw_elements_indirect_command_data * dcd=glf->glMapBufferRange(GL_DRAW_INDIRECT_BUFFER,0,total_size,GL_MAP_WRITE_BIT);
//
//    for(i=0;i<NUM_MESH_GROUPS;i++)
//    {
//        memcpy(dcd,mesh_draw_data[i],mesh_draw_data_count[i]*sizeof(draw_elements_indirect_command_data));
//        dcd+=mesh_draw_data_count[i];
//    }
//
//    glf->glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
//
//
//
//    for(i=0;i<NUM_MESH_GROUPS;i++)
//    {
//        free(mesh_draw_data[i]);
//    }
//    free(mesh_indices);
//    free(mesh_vertices);
//    free(mesh_colours);
//}
//
//draw_elements_indirect_command_data * map_mesh_dcbo_for_writing(gl_functions * glf,mesh_group group)
//{
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER, mesh_dcbo);
//    draw_elements_indirect_command_data * dcd=glf->glMapBufferRange(GL_DRAW_INDIRECT_BUFFER,
//        mesh_draw_data_offset[group]*sizeof(draw_elements_indirect_command_data),
//        mesh_draw_data_count[group]*sizeof(draw_elements_indirect_command_data),GL_MAP_WRITE_BIT);
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
//    return dcd;
//}
//
//void unmap_mesh_dcbo(gl_functions * glf)
//{
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER, mesh_dcbo);
//    glf->glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
//}
//
//
//void render_meshes(gl_functions * glf,mesh_group group_to_render)
//{
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,mesh_dcbo);
//
//    if((group_to_render==SHADOW_MESH_GROUP)||(group_to_render==TRANSPARENT_SHADOW_MESH_GROUP))
//    {
//        glf->glMultiDrawElementsIndirect(GL_TRIANGLES_ADJACENCY,GL_UNSIGNED_SHORT,(const void*)(mesh_draw_data_offset[group_to_render]*sizeof(draw_elements_indirect_command_data)),mesh_draw_data_count[group_to_render],0);
//    }
//    else
//    {
//        glf->glActiveTexture(GL_TEXTURE0);
//        glf->glBindTexture(GL_TEXTURE_1D,mesh_cbo);
//
//        glf->glMultiDrawElementsIndirect(GL_TRIANGLES,GL_UNSIGNED_SHORT,(const void*)(mesh_draw_data_offset[group_to_render]*sizeof(draw_elements_indirect_command_data)),mesh_draw_data_count[group_to_render],0);
//    }
//
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
//}
//
//void bind_mesh_buffers_for_vao(gl_functions * glf)
//{
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_ibo);
//
//    glf->glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo);
//    glf->glEnableVertexAttribArray(0);
//    glf->glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
//}
//
//
//
//
//
//
//
//
//
//
//
//void initialise_mesh_group(gl_functions * glf,mesh_group_ * mg,uint32_t flags)
//{
//    mg->flags=flags;
//
//
//    mg->mesh_count=0;
//    mg->mesh_space=1;
//
//    mg->face_count=0;
//    mg->face_space=1;
//
//    mg->vertex_count=0;
//    mg->vertex_space=1;
//
//    mg->draw_command_buffer=malloc(sizeof(draw_elements_indirect_command_data)*mg->mesh_space);
//    glf->glGenBuffers(1,&mg->dcbo);
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,mg->dcbo);
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
//
//    mg->index_buffer=malloc(sizeof(GLshort)*3*mg->face_space);
//    glf->glGenBuffers(1,&mg->ibo);
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mg->ibo);
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
//
//    mg->vertex_buffer=malloc(sizeof(GLfloat)*3*mg->vertex_space);
//    glf->glGenBuffers(1,&mg->vbo);
//    glf->glBindBuffer(GL_ARRAY_BUFFER,mg->vbo);
//    glf->glBindBuffer(GL_ARRAY_BUFFER,0);
//
//    if(mg->flags&MESH_GROUP_ADGACENCY)
//    {
//        mg->adjacent_draw_command_buffer=malloc(sizeof(draw_elements_indirect_command_data)*mg->mesh_space);
//        glf->glGenBuffers(1,&mg->adj_dcbo);
//        glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,mg->adj_dcbo);
//        glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
//
//        mg->adjacent_index_buffer=malloc(sizeof(GLshort)*6*mg->face_space);
//        glf->glGenBuffers(1,&mg->adj_ibo);
//        glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mg->adj_ibo);
//        glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
//    }
//
//    if(mg->flags&MESH_GROUP_PER_FACE_COLOUR)
//    {
//        mg->colour_buffer=malloc(sizeof(GLshort)*mg->face_space);
//
//        glf->glGenBuffers(1,&mg->cbo);
//        glf->glBindBuffer(GL_TEXTURE_BUFFER,mg->cbo);///bind the name to a buffer object (why tf is this necessary)
//        glf->glBindBuffer(GL_TEXTURE_BUFFER,0);
//
//        glf->glGenTextures(1,&mg->cto);
//
//        /// use glTextureBuffer instead
//
//        glf->glBindTexture(GL_TEXTURE_BUFFER,mg->cto);
//        glf->glTexBuffer(GL_TEXTURE_BUFFER,GL_R16UI,mg->cbo);
//        glf->glBindTexture(GL_TEXTURE_BUFFER,0);
//    }
//
////    if(mg->flags&MESH_GROUP_VERTEX_NORMS)
////    {
////        mg->normal_buffer=malloc(sizeof(GLfloat)*3*mg->vertex_space);
////        glf->glGenBuffers(1,&mg->nbo);
////    }
////
////    if(mg->flags&MESH_GROUP_UV)
////    {
////        mg->uv_buffer=malloc(sizeof(GLfloat)*2*mg->vertex_space);
////        glf->glGenBuffers(1,&mg->uvbo);
////    }
//}
//
//mesh load_mesh_file_to_group(mesh_group_ * mg,const char * filename)
//{
//    uint32_t i,num_verts,num_faces,flags;
//
//    uint16_t * indices=NULL;
//    float * verts=NULL;
//    uint16_t * adj_indices=NULL;
//    uint16_t * colours=NULL;
//
//
//    FILE * f_in;
//    f_in=fopen(filename,"rb");
//
//    if(f_in==NULL)
//    {
//        puts("MESH FILE DOES NOT EXIST");
//        goto MESH_FILE_INVALID;
//    }
//
//    if(fread(&flags,sizeof(uint32_t),1,f_in) != 1) goto MESH_FILE_INVALID;
//    if(fread(&num_faces,sizeof(uint32_t),1,f_in)!=1) goto MESH_FILE_INVALID;
//    if(fread(&num_verts,sizeof(uint32_t),1,f_in)!=1) goto MESH_FILE_INVALID;
//
//    indices=malloc(sizeof(uint16_t)*num_faces*3);
//    verts=malloc(sizeof(float)*num_verts*3);
//    adj_indices=malloc(sizeof(uint16_t)*num_faces*6);
//    colours=malloc(sizeof(uint16_t)*num_faces);
//
//    ///required elements
//    if(fread(indices,sizeof(uint16_t),num_faces*3,f_in)!=num_faces*3) goto MESH_FILE_INVALID;
//    if(fread(verts,sizeof(float),num_verts*3,f_in)!=num_verts*3) goto MESH_FILE_INVALID;
//    ///conditional elements
//    if((flags & MESH_GROUP_ADGACENCY)&&(fread(adj_indices,sizeof(uint16_t),num_faces*6,f_in)!=num_faces*6)) goto MESH_FILE_INVALID;
//    if((flags & MESH_GROUP_PER_FACE_COLOUR)&&(fread(colours,sizeof(uint16_t),num_faces,f_in)!=num_faces)) goto MESH_FILE_INVALID;
//    ///read norms
//    ///read uvs
//
//    ///check that all needed mesh components are present in loaded mesh
//    if((mg->flags&MESH_GROUP_ADGACENCY) && (!(flags&MESH_GROUP_ADGACENCY)))
//    {
//        puts("TRYING TO LOAD MESH LACKING ADGACENCY TO MESH GROUP THAT REQUIRES IT");
//        goto MESH_FILE_INVALID;
//    }
//    if((mg->flags&MESH_GROUP_PER_FACE_COLOUR) && (!(flags&MESH_GROUP_PER_FACE_COLOUR)))
//    {
//        puts("TRYING TO LOAD MESH LACKING PER_FACE_COLOUR TO MESH GROUP THAT REQUIRES IT");
//        goto MESH_FILE_INVALID;
//    }
//    ///check norms required
//    ///check uvs required
//
//
//    while(mg->vertex_count+num_verts > mg->vertex_space)
//    {
//        mg->vertex_space*=2;
//        mg->vertex_buffer=realloc(mg->vertex_buffer,sizeof(GLfloat)*3*mg->vertex_space);
//        ///conditional norm realloc here
//        ///conditional uv realloc here
//    }
//
//    while(mg->face_count+num_faces > mg->face_space)
//    {
//        mg->face_space*=2;
//        mg->index_buffer=realloc(mg->index_buffer,sizeof(GLshort)*3*mg->face_space);
//        if(mg->flags & MESH_GROUP_ADGACENCY) mg->adjacent_index_buffer=realloc(mg->adjacent_index_buffer,sizeof(GLshort)*6*mg->face_space);
//        if(mg->flags & MESH_GROUP_PER_FACE_COLOUR) mg->colour_buffer=realloc(mg->colour_buffer,sizeof(GLshort)*mg->face_space);
//    }
//
//    ///assign to GL-type buffers (type conversion happens here)
//    ///required elements
//    for(i=0;i<num_faces*3;i++) mg->index_buffer[mg->face_count*3+i]=indices[i];
//    for(i=0;i<num_verts*3;i++) mg->vertex_buffer[mg->vertex_count*3+i]=verts[i];
//    ///conditional elements
//    if(mg->flags & MESH_GROUP_ADGACENCY) for(i=0;i<num_faces*6;i++) mg->adjacent_index_buffer[mg->face_count*6+i]=adj_indices[i];
//    if(mg->flags & MESH_GROUP_PER_FACE_COLOUR) for(i=0;i<num_faces;i++) mg->colour_buffer[mg->face_count+i]=colours[i];
//    ///conditional norm assignments here
//    ///conditional uv assignments here
//
//
//    ///set dcbo data here
//    if(mg->mesh_count==mg->mesh_space)
//    {
//        mg->mesh_space*=2;
//        mg->draw_command_buffer=realloc(mg->draw_command_buffer,sizeof(draw_elements_indirect_command_data)*mg->mesh_space);
//        if(mg->flags & MESH_GROUP_ADGACENCY)mg->adjacent_draw_command_buffer=realloc(mg->adjacent_draw_command_buffer,sizeof(draw_elements_indirect_command_data)*mg->mesh_space);
//    }
//
//    mg->draw_command_buffer[mg->mesh_count].base_vertex=mg->vertex_count;
//    mg->draw_command_buffer[mg->mesh_count].count=num_faces*3;///check this is right
//    mg->draw_command_buffer[mg->mesh_count].first_index=mg->face_count*3;
//    mg->draw_command_buffer[mg->mesh_count].base_instance=0;
//    mg->draw_command_buffer[mg->mesh_count].instance_count=0;
//
//    if(mg->flags & MESH_GROUP_ADGACENCY)
//    {
//        mg->adjacent_draw_command_buffer[mg->mesh_count].base_vertex=mg->vertex_count;
//        mg->adjacent_draw_command_buffer[mg->mesh_count].count=num_faces*6;///check this is right
//        mg->adjacent_draw_command_buffer[mg->mesh_count].first_index=mg->face_count*6;
//        mg->adjacent_draw_command_buffer[mg->mesh_count].base_instance=0;
//        mg->adjacent_draw_command_buffer[mg->mesh_count].instance_count=0;
//    }
//
//    mesh m=(mesh){.index=mg->mesh_count,.colour_offset=mg->face_count};///face count used in rendering MESH_GROUP_PER_FACE_COLOUR
//
//    mg->mesh_count++;
//    mg->vertex_count+=num_verts;
//    mg->face_count+=num_faces;
//
//    free(verts);
//    free(indices);
//    free(adj_indices);
//    free(colours);
//
//    return m;
//
//    MESH_FILE_INVALID:;
//
//    free(verts);
//    free(indices);
//    free(adj_indices);
//    free(colours);
//
//    printf("MESH FILE INVALID  %s\n",filename);
//    return (mesh){0,0};
//}
//
//void transfer_mesh_group_buffer_data(gl_functions * glf,mesh_group_ * mg)
//{
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,mg->dcbo);
//    glf->glBufferData(GL_DRAW_INDIRECT_BUFFER,sizeof(draw_elements_indirect_command_data)*mg->mesh_count,mg->draw_command_buffer,GL_DYNAMIC_DRAW);
//    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
//
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mg->ibo);
//    glf->glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(GLshort)*mg->face_count*3,mg->index_buffer,GL_STATIC_DRAW);
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
//
//    glf->glBindBuffer(GL_ARRAY_BUFFER,mg->vbo);
//    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*mg->vertex_count*3,mg->vertex_buffer,GL_STATIC_DRAW);
//    glf->glBindBuffer(GL_ARRAY_BUFFER,0);
//
//
//    if(mg->flags&MESH_GROUP_ADGACENCY)
//    {
//        glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,mg->adj_dcbo);
//        glf->glBufferData(GL_DRAW_INDIRECT_BUFFER,sizeof(draw_elements_indirect_command_data)*mg->mesh_count,mg->adjacent_draw_command_buffer,GL_DYNAMIC_DRAW);
//        glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
//
//        glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mg->adj_ibo);
//        glf->glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(GLshort)*mg->face_count*6,mg->adjacent_index_buffer,GL_STATIC_DRAW);
//        glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
//    }
//
//    if(mg->flags&MESH_GROUP_PER_FACE_COLOUR)
//    {
//        glf->glBindBuffer(GL_TEXTURE_BUFFER,mg->cbo);
//        glf->glBufferData(GL_TEXTURE_BUFFER,sizeof(GLshort)*mg->face_count,mg->colour_buffer,GL_STATIC_DRAW);
//        glf->glBindBuffer(GL_TEXTURE_BUFFER,0);
//    }
//
////    if(mg->flags&MESH_GROUP_VERTEX_NORMS)
////    {
////        glf->glBindBuffer(GL_ARRAY_BUFFER,mg->nbo);
////        glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*mg->vertex_count*3,mg->normal_buffer,GL_STATIC_DRAW);
////        glf->glBindBuffer(GL_ARRAY_BUFFER,0);
////    }
////
////    if(mg->flags&MESH_GROUP_UV)
////    {
////        glf->glBindBuffer(GL_ARRAY_BUFFER,mg->uvbo);
////        glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*mg->vertex_count*2,mg->uv_buffer,GL_STATIC_DRAW);
////        glf->glBindBuffer(GL_ARRAY_BUFFER,0);
////    }
//}
//
//
//
//void bind_mesh_group_vertex_buffer(gl_functions * glf,mesh_group_ * mg,GLuint attribute_index)
//{
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mg->ibo);
//
//    glf->glBindBuffer(GL_ARRAY_BUFFER,mg->vbo);
//    glf->glEnableVertexAttribArray(attribute_index);
//    glf->glVertexAttribPointer(attribute_index,3,GL_FLOAT,GL_FALSE,0,0);
//}
//
//void bind_mesh_group_adjacent_vertex_buffer(gl_functions * glf,mesh_group_ * mg,GLuint attribute_index)
//{
//    if(!(mg->flags&MESH_GROUP_ADGACENCY))
//    {
//        puts("MESH GROUP ADJACENCY NOT PRESENT TO BIND");
//        return;
//    }
//
//    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mg->adj_ibo);
//
//    glf->glBindBuffer(GL_ARRAY_BUFFER,mg->vbo);
//    glf->glEnableVertexAttribArray(attribute_index);
//    glf->glVertexAttribPointer(attribute_index,3,GL_FLOAT,GL_FALSE,0,0);
//}
//
//void bind_mesh_group_normal_buffer(gl_functions * glf,mesh_group_ * mg,GLuint attribute_index)
//{
////    if(mg->flags&MESH_GROUP_VERTEX_NORMS)
////    {
////        glf->glBindBuffer(GL_ARRAY_BUFFER,mg->nbo);
////        glf->glEnableVertexAttribArray(attribute_index);
////        glf->glVertexAttribPointer(attribute_index,3,GL_FLOAT,GL_FALSE,0,0);
////    }
////    else puts("MESH GROUP NORMAL BUFFER NOT PRESENT TO BIND");
//}
//
//void bind_mesh_group_uv_buffer(gl_functions * glf,mesh_group_ * mg,GLuint attribute_index)
//{
////    if(mg->flags&MESH_GROUP_UV)
////    {
////        glf->glBindBuffer(GL_ARRAY_BUFFER,mg->uvbo);
////        glf->glEnableVertexAttribArray(attribute_index);
////        glf->glVertexAttribPointer(attribute_index,2,GL_FLOAT,GL_FALSE,0,0);
////    }
////    else puts("MESH GROUP NORMAL BUFFER NOT PRESENT TO BIND");
//}
//
//
//
//void delete_mesh_group(gl_functions * glf,mesh_group_ * mg)
//{
//    free(mg->draw_command_buffer);
//
//    free(mg->index_buffer);
//    glf->glDeleteBuffers(1,&mg->ibo);
//
//
//    mg->vertex_buffer=malloc(sizeof(GLfloat)*3*mg->vertex_space);
//    glf->glDeleteBuffers(1,&mg->vbo);
//
//    if(mg->flags&MESH_GROUP_ADGACENCY)
//    {
//        free(mg->adjacent_draw_command_buffer);
//
//        free(mg->adjacent_index_buffer);
//        glf->glDeleteBuffers(1,&mg->adj_ibo);
//    }
//
//    if(mg->flags&MESH_GROUP_PER_FACE_COLOUR)
//    {
//        free(mg->colour_buffer);
//
//        glf->glDeleteBuffers(1,&mg->cbo);
//        glf->glDeleteTextures(1,&mg->cto);
//    }
//
////    if(mg->flags&MESH_GROUP_VERTEX_NORMS)
////    {
////        free(mg->normal_buffer);
////        glf->glDeleteBuffers(1,&mg->nbo);
////    }
////
////    if(mg->flags&MESH_GROUP_UV)
////    {
////        free(mg->uv_buffer);
////        glf->glDeleteBuffers(1,&mg->uvbo);
////    }
//}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
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
