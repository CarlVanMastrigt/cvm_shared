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


int generate_mesh_from_objs(const char * name,float scale)
{
    ///each obj file should contain all the polys corresponding to the material of that index. e.g. objs[0] is the material with indez 0.

    //printf("sc> %f\n",scale);

    uint32_t num_verts,num_faces,num_parts;

    uint32_t * part_ids;
    uint32_t * part_face_counts;

    float * verts;
    uint16_t * faces;


    FILE * f_in;
    FILE * f_out;

    //char * str=malloc(256);



    uint32_t i,j;

    char c;

    char buffer[1024];

    strcpy(buffer,name);
    strcat(buffer,".obj");


    printf("%s\n",buffer);


    ///==========================================================
    num_verts=0;
    num_faces=0;
    num_parts=0;

    c='a';

    f_in=fopen(buffer,"r");

    if(f_in==NULL)
    {
        printf("derp");
        return -1;
    }


    while(c !=EOF)
    {
        if (c=='\n')
        {
            c=getc(f_in);
            if(c=='v')
            {
                num_verts++;
            }
            if(c=='f')
            {
                num_faces++;
            }
            if(c=='u')
            {
                num_parts++;
            }
        }
        c=getc(f_in);
    }


    fclose(f_in);

    ///printf("%d,%d,%d\n",num_verts,num_faces,num_parts);

    ///==========================================================
    c='a';

    verts=malloc(sizeof(float)*num_verts*3);
    faces=malloc(sizeof(uint16_t)*num_faces*3);

    part_ids=malloc(sizeof(uint32_t)*num_parts);
    part_face_counts=malloc(sizeof(uint32_t)*num_parts);

    for(i=0;i<num_parts;i++)
    {
        part_face_counts[i]=0;
    }

    num_verts=0;
    num_faces=0;
    num_parts=0;




    f_in=fopen(buffer,"r");


    while(c !=EOF)
    {
        if (c=='\n')
        {
            c=getc(f_in);
            if(c=='v')
            {
                fscanf(f_in,"%f %f %f",&verts[num_verts*3],&verts[num_verts*3+1],&verts[num_verts*3+2]);
                verts[num_verts*3+0]*=scale;
                verts[num_verts*3+1]*=scale;
                verts[num_verts*3+2]*=scale;
                num_verts++;
            }
            else if(c=='f')
            {
                fscanf(f_in,"%hu %hu %hu",&faces[num_faces*3],&faces[num_faces*3+1],&faces[num_faces*3+2]);
                part_face_counts[num_parts-1]++;
                num_faces++;
            }
            else if(c=='u')
            {
                fscanf(f_in,"semtl %u",part_ids+num_parts);
                num_parts++;
            }
        }
        c=getc(f_in);
    }



    for(j=0;j<num_faces;j++)
    {
        faces[j*3+0]-=1;
        faces[j*3+1]-=1;
        faces[j*3+2]-=1;
    }



    strcpy(buffer,name);
    strcat(buffer,".cvm_mesh");
    printf("%s\n",buffer);

    f_out=fopen(buffer,"wb");

    if(f_out==NULL)
    {
        printf("derp");
        return -1;
    }




    //printf("%d %d\n",new_num_verts,num_verts);


    printf("%zu,",fwrite(&num_faces,    sizeof(uint32_t), 1, f_out));
    printf("%zu,",fwrite(&num_verts,    sizeof(uint32_t), 1, f_out));
    printf("%zu,",fwrite(&num_parts,    sizeof(uint32_t), 1, f_out));

    printf("%zu,",fwrite(part_ids,          sizeof(uint32_t), num_parts, f_out));
    printf("%zu,",fwrite(part_face_counts,  sizeof(uint32_t), num_parts, f_out));


    printf("%zu,",fwrite(faces,     sizeof(uint16_t),   num_faces*3, f_out));
    printf("%zu,",fwrite(verts,     sizeof(float),      num_verts*3, f_out));

    for(i=0;i<num_parts;i++)
    {
        printf("\n%d\n\n",part_face_counts[i]);
    }

    fclose(f_in);
    fclose(f_out);

    return 0;
}



int generate_mesh_with_adjacent_from_objs(const char * name,float scale)
{
    ///each obj file should contain all the polys corresponding to the material of that index. e.g. objs[0] is the material with indez 0.

    //printf("sc> %f\n",scale);

    uint32_t num_verts,num_faces,num_parts;

    uint32_t * part_ids;
    uint32_t * part_face_counts;

    float * verts;
    uint16_t * faces;


    FILE * f_in;
    FILE * f_out;

    //char * str=malloc(256);



    uint32_t i,j,k,l;

    char c;


    int pm,pa,n;///change back to char as test if fails

    ///char * str=malloc(1024);

    char buffer[1024];

    strcpy(buffer,name);
    strcat(buffer,".obj");


    printf("%s\n",buffer);


    ///==========================================================
    num_verts=0;
    num_faces=0;
    num_parts=0;

    c='a';

    f_in=fopen(buffer,"r");

    if(f_in==NULL)
    {
        printf("derp");
        return -1;
    }






    while(c !=EOF)
    {
        if (c=='\n')
        {
            c=getc(f_in);
            if(c=='v')
            {
                num_verts++;
            }
            if(c=='f')
            {
                num_faces++;
            }
            if(c=='u')
            {
                num_parts++;
            }
        }
        c=getc(f_in);
    }


    fclose(f_in);

    ///printf("%d,%d,%d\n",num_verts,num_faces,num_parts);

    ///==========================================================
    c='a';

    verts=malloc(sizeof(float)*num_verts*3);
    faces=malloc(sizeof(uint16_t)*num_faces*6);

    part_ids=malloc(sizeof(uint32_t)*num_parts);
    part_face_counts=malloc(sizeof(uint32_t)*num_parts);

    for(i=0;i<num_parts;i++)
    {
        part_face_counts[i]=0;
    }

    num_verts=0;
    num_faces=0;
    num_parts=0;




    f_in=fopen(buffer,"r");


    while(c !=EOF)
    {
        if (c=='\n')
        {
            c=getc(f_in);
            if(c=='v')
            {
                fscanf(f_in,"%f %f %f",&verts[num_verts*3],&verts[num_verts*3+1],&verts[num_verts*3+2]);
                verts[num_verts*3+0]*=scale;
                verts[num_verts*3+1]*=scale;
                verts[num_verts*3+2]*=scale;
                num_verts++;
            }
            else if(c=='f')
            {
                fscanf(f_in,"%hu %hu %hu",&faces[num_faces*6],&faces[num_faces*6+2],&faces[num_faces*6+4]);
                part_face_counts[num_parts-1]++;
                num_faces++;
            }
            else if(c=='u')
            {
                fscanf(f_in,"semtl %u",part_ids+num_parts);
                num_parts++;
            }
        }
        c=getc(f_in);
    }



    for(j=0;j<num_faces;j++)
    {
        faces[j*6+0]-=1;
        faces[j*6+2]-=1;
        faces[j*6+4]-=1;
    }

    int adj_cnt=0;

    int adj_found[3];

    for(j=0;j<num_faces;j++)
    {
        adj_found[0]=0;
        adj_found[1]=0;
        adj_found[2]=0;

        for(k=0;k<num_faces;k++)
        {
            if (j==k)
            {
                continue;
            }
            pm=3;pa=6;n=0;
            for(l=0;l<5;l+=2)
            {
                if (faces[j*6+0]==faces[k*6+l]){pm-=0;pa-=l;n++;}
                if (faces[j*6+2]==faces[k*6+l]){pm-=1;pa-=l;n++;}
                if (faces[j*6+4]==faces[k*6+l]){pm-=2;pa-=l;n++;}
            }
            if(n==2)
            {
                if(pm==0)
                {
                    faces[j*6+3]=faces[k*6+pa];
                }
                if(pm==1)
                {
                    faces[j*6+5]=faces[k*6+pa];
                }
                if(pm==2)
                {
                    faces[j*6+1]=faces[k*6+pa];
                }

                adj_found[pm]=1;

                adj_cnt++;
            }
        }

        if(adj_found[0]==0)
        {
            puts("unfound_side_0");
            faces[j*6+3]=faces[j*6+0];
        }
        if(adj_found[1]==0)
        {
            puts("unfound_side_1");
            faces[j*6+5]=faces[j*6+2];
        }
        if(adj_found[2]==0)
        {
            puts("unfound_side_2");
            faces[j*6+1]=faces[j*6+4];
        }
    }


    /**for(i=0;i<num_verts;i++)
    {
        printf("%f,%f,%f,\n",verts[i*3+0],verts[i*3+1],verts[i*3+2]);
    }
    int ind_off=14;

    for(i=0;i<num_faces;i++)
    {
        printf("%d,%d,%d,\n",faces[i*6+0]+ind_off,faces[i*6+2]+ind_off,faces[i*6+4]+ind_off);
    }

    for(i=0;i<num_faces;i++)
    {
        printf("%d,%d,%d,%d,%d,%d,\n",faces[i*6+0]+ind_off,faces[i*6+1]+ind_off,faces[i*6+2]+ind_off,faces[i*6+3]+ind_off,faces[i*6+4]+ind_off,faces[i*6+5]+ind_off);
    }*/




    strcpy(buffer,name);
    strcat(buffer,".cvm_mesh_adj");
    printf("%s\n",buffer);

    f_out=fopen(buffer,"wb");

    if(f_out==NULL)
    {
        printf("derp");
        return -1;
    }


    printf("%zu,",fwrite(&num_faces,sizeof(uint32_t),1,f_out));
    printf("%zu,",fwrite(&num_verts,sizeof(uint32_t),1,f_out));
    printf("%zu,",fwrite(&num_parts,sizeof(uint32_t),1,f_out));

    printf("%zu,",fwrite(part_ids,sizeof(uint32_t),num_parts,f_out));
    printf("%zu,",fwrite(part_face_counts,sizeof(uint32_t),num_parts,f_out));


    printf("%zu,",fwrite(faces,sizeof(uint16_t),num_faces*6,f_out));
    printf("%zu,",fwrite(verts,sizeof(float),num_verts*3,f_out));

    for(i=0;i<num_parts;i++)
    {
        printf("\n%d\n\n",part_face_counts[i]);
    }

    fclose(f_in);
    fclose(f_out);

    return 0;
}



#warning later change following buffer types to GL types    GLfloat GLushort GLuint

static uint16_t * mesh_indices;
static uint32_t mesh_index_count;
static uint32_t mesh_index_space;

static float * mesh_vertices;
static uint32_t mesh_vertex_count;
static uint32_t mesh_vertex_space;

static uint16_t * mesh_colours;
static uint32_t mesh_colour_count;
static uint32_t mesh_colour_space;

static draw_elements_indirect_command_data * mesh_draw_data[NUM_MESH_GROUPS];///could be put in separate data structure (for separate mesh types/groups)
static uint32_t mesh_draw_data_count[NUM_MESH_GROUPS];
static uint32_t mesh_draw_data_space[NUM_MESH_GROUPS];
static uint32_t mesh_draw_data_offset[NUM_MESH_GROUPS];


uint32_t get_mesh_group_count(mesh_group group)
{
    return mesh_draw_data_count[group];
}

void initialise_mesh_buffers(void)
{
    mesh_index_count=0;
    mesh_index_space=1;
    mesh_indices=malloc(sizeof(uint16_t)*mesh_index_space);

    mesh_vertex_count=0;
    mesh_vertex_space=1;
    mesh_vertices=malloc(sizeof(float)*mesh_vertex_space*3);

    mesh_colour_count=0;
    mesh_colour_space=1;
    mesh_colours=malloc(sizeof(uint16_t)*mesh_colour_space);

    int i;
    for(i=0;i<NUM_MESH_GROUPS;i++)
    {
        mesh_draw_data_count[i]=0;
        mesh_draw_data_space[i]=1;
        mesh_draw_data[i]=malloc(sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[i]);
    }
}












mesh load_mesh_file(char * filename,mesh_group group)
{
    mesh rtrn=(mesh){0,0};

    char * tmp=filename;

    while(*tmp!='.')
    {
        if(*tmp=='\0')
        {
            puts("error loading mesh file: not of correct type");
            return rtrn;
        }
        tmp++;
    }

    if(strcmp(tmp,".cvm_mesh"))
    {
        puts("error loading mesh file: not of correct type");
        return rtrn;
    }



    FILE * f_in;
    f_in=fopen(filename,"rb");
    uint32_t num_verts,num_faces,num_parts,i,j,k;
    uint32_t *part_ids,*part_face_counts;


    num_verts=num_faces=num_parts=0;

    if(group==SHADOW_MESH_GROUP)
    {
        puts("error loading mesh file: cannot load regular mesh file to shadow group");
        return rtrn;
    }

    if(group==COLOUR_MESH_GROUP)
    {
        puts("error loading mesh file: cannot load regular mesh file to colour group as this group shares data with the shadow group");
        return rtrn;
    }



    if (f_in!=NULL)
    {
        if(mesh_draw_data_count[group]==mesh_draw_data_space[group])
        {
            mesh_draw_data_space[group]*=2;
            mesh_draw_data[group]=realloc(mesh_draw_data[group],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[group]);
        }

        fread(&num_faces, sizeof(uint32_t), 1, f_in);
        fread(&num_verts, sizeof(uint32_t), 1, f_in);
        fread(&num_parts, sizeof(uint32_t), 1, f_in);

        if((mesh_index_count+num_faces*3) >= mesh_index_space)
        {
            while((mesh_index_count+num_faces*3) >= mesh_index_space)mesh_index_space*=2;

            mesh_indices=realloc(mesh_indices,sizeof(uint16_t)*mesh_index_space);
        }

        if((mesh_vertex_count+num_verts) >= mesh_vertex_space)
        {
            while((mesh_vertex_count+num_verts) >= mesh_vertex_space)mesh_vertex_space*=2;

            mesh_vertices=realloc(mesh_vertices,sizeof(float)*mesh_vertex_space*3);
        }

        if((mesh_colour_count+num_faces) >= mesh_colour_space)
        {
            while((mesh_colour_count+num_faces) >= mesh_colour_space)mesh_colour_space*=2;

            mesh_colours=realloc(mesh_colours,sizeof(uint16_t)*mesh_colour_space);
        }




        part_ids=malloc(sizeof(uint32_t)*num_parts);
        part_face_counts=malloc(sizeof(uint32_t)*num_parts);

        fread(part_ids, sizeof(uint32_t), num_parts, f_in);
        fread(part_face_counts, sizeof(uint32_t), num_parts, f_in);

        k=0;
        for(i=0;i<num_parts;i++)
        {
            for(j=0;j<part_face_counts[i];j++)
            {
                mesh_colours[mesh_colour_count+k]=part_ids[i];
                k++;
            }
        }

        free(part_ids);
        free(part_face_counts);



        fread(mesh_indices+mesh_index_count, sizeof(uint16_t), num_faces*3, f_in);
        fread(mesh_vertices+(mesh_vertex_count*3), sizeof(float), num_verts*3, f_in);


        mesh_draw_data[group][mesh_draw_data_count[group]].base_instance=0;///set_elsewhere
        mesh_draw_data[group][mesh_draw_data_count[group]].instance_count=0;///set_elsewhere

        mesh_draw_data[group][mesh_draw_data_count[group]].base_vertex=mesh_vertex_count;
        mesh_draw_data[group][mesh_draw_data_count[group]].count=num_faces*3;
        mesh_draw_data[group][mesh_draw_data_count[group]].first_index=mesh_index_count;


        rtrn.index=mesh_draw_data_count[group];
        rtrn.colour_offset=mesh_colour_count;

        mesh_draw_data_count[group]++;
        mesh_vertex_count+=num_verts;
        mesh_index_count+=num_faces*3;
        mesh_colour_count+=num_faces;
    }

    fclose(f_in);

    return rtrn;
}

mesh load_mesh_adjacency_file(char * filename)
{
    mesh rtrn=(mesh){0,0};
    char * tmp=filename;

    while(*tmp!='.')
    {
        if(*tmp=='\0')
        {
            puts("error loading mesh file: not of correct type (no extension)");
            return rtrn;
        }
        tmp++;
    }

    if(strcmp(tmp,".cvm_mesh_adj"))
    {
        printf("error loading mesh file: not of correct type (wrong extension) %s",tmp);
        return rtrn;
    }



    FILE * f_in;
    f_in=fopen(filename,"rb");
    uint32_t num_verts,num_faces,num_parts,i,j,k;
    uint32_t *part_ids,*part_face_counts;


    num_verts=num_faces=num_parts=0;



    if (f_in==NULL)
    {
        printf("error mesh file not found: %s\n",filename);
    }
    else
    {
        if(mesh_draw_data_count[SHADOW_MESH_GROUP]==mesh_draw_data_space[SHADOW_MESH_GROUP])
        {
            mesh_draw_data_space[SHADOW_MESH_GROUP]*=2;
            mesh_draw_data[SHADOW_MESH_GROUP]=realloc(mesh_draw_data[SHADOW_MESH_GROUP],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[SHADOW_MESH_GROUP]);
        }

        if(mesh_draw_data_count[COLOUR_MESH_GROUP]==mesh_draw_data_space[COLOUR_MESH_GROUP])
        {
            mesh_draw_data_space[COLOUR_MESH_GROUP]*=2;
            mesh_draw_data[COLOUR_MESH_GROUP]=realloc(mesh_draw_data[COLOUR_MESH_GROUP],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[COLOUR_MESH_GROUP]);
        }

        fread(&num_faces, sizeof(uint32_t), 1, f_in);
        fread(&num_verts, sizeof(uint32_t), 1, f_in);
        fread(&num_parts, sizeof(uint32_t), 1, f_in);

        if((mesh_index_count+num_faces*9) >= mesh_index_space)
        {
            while((mesh_index_count+num_faces*9) >= mesh_index_space)mesh_index_space*=2;

            mesh_indices=realloc(mesh_indices,sizeof(uint16_t)*mesh_index_space);
        }

        if((mesh_vertex_count+num_verts) >= mesh_vertex_space)
        {
            while((mesh_vertex_count+num_verts) >= mesh_vertex_space)mesh_vertex_space*=2;

            mesh_vertices=realloc(mesh_vertices,sizeof(float)*mesh_vertex_space*3);
        }

        if((mesh_colour_count+num_faces) >= mesh_colour_space)
        {
            while((mesh_colour_count+num_faces) >= mesh_colour_space)mesh_colour_space*=2;

            mesh_colours=realloc(mesh_colours,sizeof(uint16_t)*mesh_colour_space);
        }




        part_ids=malloc(sizeof(uint32_t)*num_parts);
        part_face_counts=malloc(sizeof(uint32_t)*num_parts);

        fread(part_ids, sizeof(uint32_t), num_parts, f_in);
        fread(part_face_counts, sizeof(uint32_t), num_parts, f_in);

        k=0;
        for(i=0;i<num_parts;i++)
        {
            for(j=0;j<part_face_counts[i];j++)
            {
                mesh_colours[mesh_colour_count+k]=part_ids[i];
                k++;
            }
        }

        free(part_ids);
        free(part_face_counts);



        fread(mesh_indices+mesh_index_count, sizeof(uint16_t), num_faces*6, f_in);
        fread(mesh_vertices+(mesh_vertex_count*3), sizeof(float), num_verts*3, f_in);


        for(i=0;i<num_faces;i++)
        {
            mesh_indices[mesh_index_count+num_faces*6+i*3+0]=mesh_indices[mesh_index_count+i*6+0];
            mesh_indices[mesh_index_count+num_faces*6+i*3+1]=mesh_indices[mesh_index_count+i*6+2];
            mesh_indices[mesh_index_count+num_faces*6+i*3+2]=mesh_indices[mesh_index_count+i*6+4];
        }

        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].base_instance=0;///set_elsewhere
        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].instance_count=0;///set_elsewhere

        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].base_vertex=mesh_vertex_count;
        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].count=num_faces*6;
        mesh_draw_data[SHADOW_MESH_GROUP][mesh_draw_data_count[SHADOW_MESH_GROUP]].first_index=mesh_index_count;

        mesh_draw_data_count[SHADOW_MESH_GROUP]++;
        mesh_index_count+=num_faces*6;



        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].base_instance=0;///set_elsewhere
        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].instance_count=0;///set_elsewhere

        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].base_vertex=mesh_vertex_count;
        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].count=num_faces*3;
        mesh_draw_data[COLOUR_MESH_GROUP][mesh_draw_data_count[COLOUR_MESH_GROUP]].first_index=mesh_index_count;

        rtrn.index=mesh_draw_data_count[COLOUR_MESH_GROUP];
        mesh_draw_data_count[COLOUR_MESH_GROUP]++;
        mesh_index_count+=num_faces*3;



        rtrn.colour_offset=mesh_colour_count;

        mesh_vertex_count+=num_verts;
        mesh_colour_count+=num_faces;

        if(mesh_draw_data_count[COLOUR_MESH_GROUP]!=mesh_draw_data_count[SHADOW_MESH_GROUP])
        {
            puts("error somehow loaded to colour or shadow mesh group without also loading to the other");
        }
    }

    fclose(f_in);

    return rtrn;
}

mesh load_mesh_transparent_adjacency_file(char * filename)
{
    mesh rtrn=(mesh){0,0};
    char * tmp=filename;

    while(*tmp!='.')
    {
        if(*tmp=='\0')
        {
            puts("error loading mesh file: not of correct type (no extension)");
            return rtrn;
        }
        tmp++;
    }

    if(strcmp(tmp,".cvm_mesh_adj"))
    {
        printf("error loading mesh file: not of correct type (wrong extension) %s",tmp);
        return rtrn;
    }



    FILE * f_in;
    f_in=fopen(filename,"rb");
    uint32_t num_verts,num_faces,num_parts,i;
    uint32_t *part_ids,*part_face_counts;


    num_verts=num_faces=num_parts=0;



    if (f_in==NULL)
    {
        printf("error mesh file not found: %s\n",filename);
    }
    else
    {
        if(mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]==mesh_draw_data_space[TRANSPARENT_SHADOW_MESH_GROUP])
        {
            mesh_draw_data_space[TRANSPARENT_SHADOW_MESH_GROUP]*=2;
            mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP]=realloc(mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[TRANSPARENT_SHADOW_MESH_GROUP]);
        }

        if(mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]==mesh_draw_data_space[TRANSPARENT_COLOUR_MESH_GROUP])
        {
            mesh_draw_data_space[TRANSPARENT_COLOUR_MESH_GROUP]*=2;
            mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP]=realloc(mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP],sizeof(draw_elements_indirect_command_data)*mesh_draw_data_space[TRANSPARENT_COLOUR_MESH_GROUP]);
        }

        fread(&num_faces, sizeof(uint32_t), 1, f_in);
        fread(&num_verts, sizeof(uint32_t), 1, f_in);
        fread(&num_parts, sizeof(uint32_t), 1, f_in);

        if((mesh_index_count+num_faces*9) >= mesh_index_space)
        {
            while((mesh_index_count+num_faces*9) >= mesh_index_space)mesh_index_space*=2;

            mesh_indices=realloc(mesh_indices,sizeof(uint16_t)*mesh_index_space);
        }

        if((mesh_vertex_count+num_verts) >= mesh_vertex_space)
        {
            while((mesh_vertex_count+num_verts) >= mesh_vertex_space)mesh_vertex_space*=2;

            mesh_vertices=realloc(mesh_vertices,sizeof(float)*mesh_vertex_space*3);
        }




        part_ids=malloc(sizeof(uint32_t)*num_parts);
        part_face_counts=malloc(sizeof(uint32_t)*num_parts);

        fread(part_ids, sizeof(uint32_t), num_parts, f_in);
        fread(part_face_counts, sizeof(uint32_t), num_parts, f_in);

        free(part_ids);
        free(part_face_counts);



        fread(mesh_indices+mesh_index_count, sizeof(uint16_t), num_faces*6, f_in);
        fread(mesh_vertices+(mesh_vertex_count*3), sizeof(float), num_verts*3, f_in);


        for(i=0;i<num_faces;i++)
        {
            mesh_indices[mesh_index_count+num_faces*6+i*3+0]=mesh_indices[mesh_index_count+i*6+0];
            mesh_indices[mesh_index_count+num_faces*6+i*3+1]=mesh_indices[mesh_index_count+i*6+2];
            mesh_indices[mesh_index_count+num_faces*6+i*3+2]=mesh_indices[mesh_index_count+i*6+4];
        }

        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].base_instance=0;///set_elsewhere
        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].instance_count=0;///set_elsewhere

        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].base_vertex=mesh_vertex_count;
        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].count=num_faces*6;
        mesh_draw_data[TRANSPARENT_SHADOW_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]].first_index=mesh_index_count;

        mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP]++;
        mesh_index_count+=num_faces*6;



        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].base_instance=0;///set_elsewhere
        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].instance_count=0;///set_elsewhere

        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].base_vertex=mesh_vertex_count;
        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].count=num_faces*3;
        mesh_draw_data[TRANSPARENT_COLOUR_MESH_GROUP][mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]].first_index=mesh_index_count;

        rtrn.index=mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP];
        mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]++;
        mesh_index_count+=num_faces*3;



        rtrn.colour_offset=0;

        mesh_vertex_count+=num_verts;

        if(mesh_draw_data_count[TRANSPARENT_COLOUR_MESH_GROUP]!=mesh_draw_data_count[TRANSPARENT_SHADOW_MESH_GROUP])
        {
            puts("error somehow loaded to colour or shadow mesh group without also loading to the other");
        }
    }

    fclose(f_in);

    return rtrn;
}








static GLuint mesh_ibo=0;
static GLuint mesh_vbo=0;
static GLuint mesh_cbo=0;
static GLuint mesh_dcbo=0;

void transfer_mesh_draw_data(gl_functions * glf)
{
    glf->glGenBuffers(1,&mesh_ibo);
    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_ibo);
    glf->glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(uint16_t)*mesh_index_count,mesh_indices,GL_STATIC_DRAW);
    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

    glf->glGenBuffers(1,&mesh_vbo);
    glf->glBindBuffer(GL_ARRAY_BUFFER,mesh_vbo);
    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(float)*mesh_vertex_count*3,mesh_vertices,GL_STATIC_DRAW);
    glf->glBindBuffer(GL_ARRAY_BUFFER,0);

    glf->glGenTextures(1,&mesh_cbo);
    glf->glBindTexture(GL_TEXTURE_1D,mesh_cbo);
    glf->glTexImage1D(GL_TEXTURE_1D,0,GL_R16UI,mesh_colour_count,0,GL_RED_INTEGER,GL_UNSIGNED_SHORT,mesh_colours);
    glf->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glf->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glf->glBindTexture(GL_TEXTURE_1D,0);



    int i;
    uint32_t total_size=0;

    for(i=0;i<NUM_MESH_GROUPS;i++)
    {
        mesh_draw_data_offset[i]=total_size;
        total_size+=mesh_draw_data_count[i];
    }
    total_size*=sizeof(draw_elements_indirect_command_data);

    glf->glGenBuffers(1,&mesh_dcbo);
    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,mesh_dcbo);
    glf->glBufferStorage(GL_DRAW_INDIRECT_BUFFER,total_size,NULL,GL_MAP_WRITE_BIT);
    draw_elements_indirect_command_data * dcd=glf->glMapBufferRange(GL_DRAW_INDIRECT_BUFFER,0,total_size,GL_MAP_WRITE_BIT);

    for(i=0;i<NUM_MESH_GROUPS;i++)
    {
        memcpy(dcd,mesh_draw_data[i],mesh_draw_data_count[i]*sizeof(draw_elements_indirect_command_data));
        dcd+=mesh_draw_data_count[i];
    }

    glf->glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);



    for(i=0;i<NUM_MESH_GROUPS;i++)
    {
        free(mesh_draw_data[i]);
    }
    free(mesh_indices);
    free(mesh_vertices);
    free(mesh_colours);
}

draw_elements_indirect_command_data * map_mesh_dcbo_for_writing(gl_functions * glf,mesh_group group)
{
    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER, mesh_dcbo);
    draw_elements_indirect_command_data * dcd=glf->glMapBufferRange(GL_DRAW_INDIRECT_BUFFER,
        mesh_draw_data_offset[group]*sizeof(draw_elements_indirect_command_data),
        mesh_draw_data_count[group]*sizeof(draw_elements_indirect_command_data),GL_MAP_WRITE_BIT);
    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    return dcd;
}

void unmap_mesh_dcbo(gl_functions * glf)
{
    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER, mesh_dcbo);
    glf->glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
}


void render_meshes(gl_functions * glf,mesh_group group_to_render)
{
    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,mesh_dcbo);

    if((group_to_render==SHADOW_MESH_GROUP)||(group_to_render==TRANSPARENT_SHADOW_MESH_GROUP))
    {
        glf->glMultiDrawElementsIndirect(GL_TRIANGLES_ADJACENCY,GL_UNSIGNED_SHORT,(const void*)(mesh_draw_data_offset[group_to_render]*sizeof(draw_elements_indirect_command_data)),mesh_draw_data_count[group_to_render],0);
    }
    else
    {
        glf->glActiveTexture(GL_TEXTURE0);
        glf->glBindTexture(GL_TEXTURE_1D,mesh_cbo);

        glf->glMultiDrawElementsIndirect(GL_TRIANGLES,GL_UNSIGNED_SHORT,(const void*)(mesh_draw_data_offset[group_to_render]*sizeof(draw_elements_indirect_command_data)),mesh_draw_data_count[group_to_render],0);
    }

    glf->glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
}

void bind_mesh_buffers_for_vao(gl_functions * glf)
{
    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_ibo);

    glf->glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo);
    glf->glEnableVertexAttribArray(0);
    glf->glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
}






static GLuint assorted_ibo;
static GLuint assorted_vbo;

static GLuint assorted_line_vbo;


/// 6 - 8


static GLfloat assorted_vertices[]=
{
    ///octahedron (bounds sphere of radius 1, centred on origin)
    0.0,0.0,SQRT_3,
    -SQRT_3,0.0,0.0,
    0.0,SQRT_3,0.0,
    SQRT_3,0.0,0.0,
    0.0,0.0,-SQRT_3,
    0.0,-SQRT_3,0.0,
    ///triangular antiprism (bounds sphere of radius 1 and length 1, with its base centred on origin)
    0.0,-2.0,0.0,
    -SQRT_3,1.0,0.0,
    SQRT_3,1.0,0.0,
    -SQRT_3,-1.0,1.0,
    0.0,2.0,1.0,
    SQRT_3,-1.0,1.0
};

static GLushort assorted_indices[]=
{
    ///octahedron
    1,0,2,
    3,2,0,
    3,4,2,
    1,2,4,
    3,0,5,
    1,5,0,
    3,5,4,
    1,4,5,
    1,5,0,3,2,4,
    3,4,2,1,0,5,
    3,5,4,1,2,0,
    1,0,2,3,4,5,
    3,2,0,1,5,4,
    1,4,5,3,0,2,
    3,0,5,1,4,2,
    1,2,4,3,5,0,
    ///triangular antiprism
    6,7,8,
    6,9,7,
    9,10,7,
    7,10,8,
    10,11,8,
    8,11,6,
    11,9,6,
    11,10,9,
    6,9,7,10,8,11,
    6,11,9,10,7,8,
    9,11,10,8,7,6,
    7,9,10,11,8,6,
    10,9,11,6,8,7,
    8,10,11,9,6,7,
    11,10,9,7,6,8,
    11,8,10,7,9,6
};



//static GLfloat assorted_vertices[]=
//{
//    ///octahedron
//    0.0,0.0,SQRT_3,
//    -SQRT_3,0.0,0.0,
//    0.0,SQRT_3,0.0,
//    SQRT_3,0.0,0.0,
//    0.0,0.0,-SQRT_3,
//    0.0,-SQRT_3,0.0,
//    ///half cuboid
//    1.0,1.0,-0.0,
//    1.0,-1.0,-0.0,
//    -1.0,-1.0,-0.0,
//    -1.0,1.0,-0.0,
//    1.0,1.0,1.0,
//    1.0,-1.0,1.0,
//    -1.0,-1.0,1.0,
//    -1.0,1.0,1.0,
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
//    ///half cuboid
//    6,7,8,
//    10,13,11,
//    6,10,7,
//    7,11,8,
//    8,12,13,
//    10,6,13,
//    9,6,8,
//    10,11,7,
//    6,9,13,
//    13,12,11,
//    9,8,13,
//    11,12,8,
//    6,10,7,11,8,9,
//    10,6,13,12,11,7,
//    6,13,10,11,7,8,
//    7,10,11,12,8,6,
//    8,11,12,11,13,9,
//    10,7,6,9,13,11,
//    9,13,6,7,8,13,
//    10,13,11,8,7,6,
//    6,8,9,8,13,10,
//    13,8,12,8,11,10,
//    9,6,8,12,13,6,
//    11,13,12,13,8,7,
//};

static const uint32_t circle_divisions=64;

static GLfloat assorted_line_vertices[]=
{
    0.5,0.5,
    1.5,1.5,
    -0.5,0.5,
    -1.5,1.5,
    0.5,-0.5,
    1.5,-1.5,
    -0.5,-0.5,
    -1.5,-1.5,

    1.5,0.0,
    0.0,1.5,
    -1.5,0.0,
    0.0,-1.5,

    1.0,1.0,
    -1.0,1.0,
    -1.0,-1.0,
    -1.0,-1.0,
    1.0,-1.0,
    1.0,1.0
};

void initialise_assorted_meshes(gl_functions * glf)
{
    glf->glGenBuffers(1,&assorted_vbo);
    glf->glBindBuffer(GL_ARRAY_BUFFER,assorted_vbo);
    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*36,assorted_vertices,GL_STATIC_DRAW);
    glf->glBindBuffer(GL_ARRAY_BUFFER, 0);

    glf->glGenBuffers(1,&assorted_ibo);
    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,assorted_ibo);
    glf->glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(GLushort)*144,assorted_indices,GL_STATIC_DRAW);
    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);



    GLfloat line_verts[36+2*circle_divisions];

    uint32_t i;
    float angle;

    for(i=0;i<36;i++)
    {
        line_verts[i]=assorted_line_vertices[i];
    }

    for(i=0;i<circle_divisions;i++)
    {
        angle=((float)i)*2.0*PI/((float)circle_divisions);
        line_verts[36+i*2+0]=cosf(angle);
        line_verts[36+i*2+1]=sinf(angle);
    }


    glf->glGenBuffers(1,&assorted_line_vbo);
    glf->glBindBuffer(GL_ARRAY_BUFFER,assorted_line_vbo);
    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*(36+2*circle_divisions),line_verts,GL_STATIC_DRAW);
    glf->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void bind_assorted_for_vao_vec3(gl_functions * glf)
{
    glf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, assorted_ibo);

    glf->glBindBuffer(GL_ARRAY_BUFFER,assorted_vbo);
    glf->glEnableVertexAttribArray(0);
    glf->glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
}

void bind_assorted_for_vao_vec2(gl_functions * glf)
{
    glf->glBindBuffer(GL_ARRAY_BUFFER,assorted_line_vbo);
    glf->glEnableVertexAttribArray(0);
    glf->glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
}

void render_octahedron_colour(gl_functions * glf,uint32_t count)
{
    glf->glDrawElementsInstanced(GL_TRIANGLES,24,GL_UNSIGNED_SHORT,NULL,count);
}

void render_octahedron_shadow(gl_functions * glf,uint32_t count)
{
    glf->glDrawElementsInstanced(GL_TRIANGLES_ADJACENCY,48,GL_UNSIGNED_SHORT,(const void *)(24*sizeof(GLushort)),count);
}

void render_triangular_antiprism_colour(gl_functions * glf,uint32_t count)
{
    glf->glDrawElementsInstanced(GL_TRIANGLES,24,GL_UNSIGNED_SHORT,(const void *)(72*sizeof(GLushort)),count);
}

void render_triangular_antiprism_shadow(gl_functions * glf,uint32_t count)
{
    glf->glDrawElementsInstanced(GL_TRIANGLES_ADJACENCY,48,GL_UNSIGNED_SHORT,(const void *)(96*sizeof(GLushort)),count);
}



void render_cross_lines(gl_functions * glf,uint32_t count)
{
    glf->glDrawArraysInstanced(GL_LINES,0,8,count);
}

void render_square_lines(gl_functions * glf,uint32_t count)
{
    glf->glDrawArraysInstanced(GL_LINE_LOOP,8,4,count);
}

void render_square(gl_functions * glf,uint32_t count)
{
    glf->glDrawArraysInstanced(GL_TRIANGLES,12,6,count);
}

void render_circle_lines(gl_functions * glf,uint32_t count)
{
    glf->glDrawArraysInstanced(GL_LINE_LOOP,36,circle_divisions,count);
}
