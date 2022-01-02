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

void change_camera_azimuthal_angle(float delta,cvm_camera * c)
{
    c->azimuthal_angle+=delta;
    if(c->azimuthal_angle>PI)c->azimuthal_angle-=2.0*PI;
    if(c->azimuthal_angle<-PI)c->azimuthal_angle+=2.0*PI;
}

void change_camera_zenith_angle(float delta,cvm_camera * c)
{
    c->zenith_angle+=delta;
    if(c->zenith_angle>PI)c->zenith_angle=PI;
    if(c->zenith_angle<0.0)c->zenith_angle=0.0;
}

void change_camera_zoom(int delta,cvm_camera * c)
{
    c->current_zoom_step+=delta;
    if(c->current_zoom_step<0)c->current_zoom_step=0;
    if(c->current_zoom_step>c->max_zoom_step)c->current_zoom_step=c->max_zoom_step;
}

void initialise_camera(int screen_w,int screen_h,float fov, float near,int zoom_steps,cvm_camera * c)
{
    c->min_focal_distance=3.0f;
    c->max_focal_distance=100.0f;

    c->max_zoom_step=zoom_steps;
    c->current_zoom_step=zoom_steps/2;
    c->azimuthal_angle=0.0;//PI*-0.25;
    c->zenith_angle=PI*0.75;//-fov*0.5;


    c->fov=fov;
    c->near=near;

    update_camera(screen_w,screen_h,c);
}

///should probably move ops that use plane to their own function

void update_camera(int screen_w,int screen_h,cvm_camera * c)
{
    matrix4f vm,vmi,rx,rz,z,proj;
    float ca,sa,ar,tan_half_fov;
    vec3f ftr,ftl,fbr,fbl;

    float focal_distance=c->min_focal_distance*expf((logf(c->max_focal_distance) - logf(c->min_focal_distance))*((float)c->current_zoom_step)/((float)c->max_zoom_step));
    c->position.x=-focal_distance*sinf(c->azimuthal_angle)*sinf(c->zenith_angle);
    c->position.y=-focal_distance*cosf(c->azimuthal_angle)*sinf(c->zenith_angle);
    c->position.z=-focal_distance*cosf(c->zenith_angle);


    ar=((float)screen_w)/((float)screen_h);

//    c->screen_w=screen_w;
//    c->screen_h=screen_h;
//    c->inverse_screen_dimensions[0]=1.0f/((float)screen_w);
//    c->inverse_screen_dimensions[1]=1.0f/((float)screen_h);
//    c->aspect_ratio=ar;
//    c->pixel_to_fov_angle_ratio=((float)screen_h)/c->fov;


    tan_half_fov=tanf(c->fov*0.5);



    ca=cosf(c->zenith_angle);
    sa=sinf(c->zenith_angle);
    rx.x.x=1.0f;rx.y.x=0.0f;rx.z.x=0.0f;rx.w.x=0.0f;
    rx.x.y=0.0f;rx.y.y=ca;  rx.z.y=-sa; rx.w.y=0.0f;
    rx.x.z=0.0f;rx.y.z=sa;  rx.z.z=ca;  rx.w.z=0.0f;
    rx.x.w=0.0f;rx.y.w=0.0f;rx.z.w=0.0f;rx.w.w=1.0f;

    ca=cosf(c->azimuthal_angle);
    sa=sinf(c->azimuthal_angle);
    rz.x.x=ca;  rz.y.x=-sa; rz.z.x=0.0f;rz.w.x=0.0f;
    rz.x.y=sa;  rz.y.y=ca;  rz.z.y=0.0f;rz.w.y=0.0f;
    rz.x.z=0.0f;rz.y.z=0.0f;rz.z.z=1.0f;rz.w.z=0.0f;
    rz.x.w=0.0f;rz.y.w=0.0f;rz.z.w=0.0f;rz.w.w=1.0f;

    z.x.x=1.0f; z.y.x=0.0f; z.z.x=0.0f; z.w.x=0.0f;
    z.x.y=0.0f; z.y.y=1.0f; z.z.y=0.0f; z.w.y=0.0f;
    z.x.z=0.0f; z.y.z=0.0f; z.z.z=1.0f; z.w.z=focal_distance;///move camera backwards along its vector (direction from which it views scene)
    z.x.w=0.0f; z.y.w=0.0f; z.z.w=0.0f; z.w.w=1.0f;

    proj.x.x=1.0f/(tan_half_fov*ar);proj.y.x=0.0f;              proj.z.x=0.0f;  proj.w.x=0.0;
    proj.x.y=0.0f;                  proj.y.y=1.0f/tan_half_fov; proj.z.y=0.0f;  proj.w.y=0.0;
    proj.x.z=0.0f;                  proj.y.z=0.0f;              proj.z.z=0.0f;  proj.w.z=c->near;
    proj.x.w=0.0f;                  proj.y.w=0.0f;              proj.z.w=1.0f; proj.w.w=0.0;

    c->view_matrix=vm=m4f_mult(proj,m4f_mult(z,m4f_mult(rx,rz)));
    c->view_matrix_inverse=vmi=m4f_inv(vm);

    #warning make sure z of 1.0 vs 0.0 is appropriate, not zero and not massive/infinite
    #warning is frustrum_corners valid data to store? maybe just make local variables
    ftr=v3f_sub(m4f_v4f_mult_p(vmi,(vec4f){.x=  1.0f,.y=  1.0f,.z=1.0f,.w=1.0f}),c->position);
    ftl=v3f_sub(m4f_v4f_mult_p(vmi,(vec4f){.x= -1.0f,.y=  1.0f,.z=1.0f,.w=1.0f}),c->position);
    fbl=v3f_sub(m4f_v4f_mult_p(vmi,(vec4f){.x= -1.0f,.y= -1.0f,.z=1.0f,.w=1.0f}),c->position);
    fbr=v3f_sub(m4f_v4f_mult_p(vmi,(vec4f){.x=  1.0f,.y= -1.0f,.z=1.0f,.w=1.0f}),c->position);

    c->bounds[frus_face_t]=plane_from_normal_and_point(v3f_norm_cross(ftr,ftl),c->position);
    c->bounds[frus_face_b]=plane_from_normal_and_point(v3f_norm_cross(fbl,fbr),c->position);
    c->bounds[frus_face_l]=plane_from_normal_and_point(v3f_norm_cross(ftl,fbl),c->position);
    c->bounds[frus_face_r]=plane_from_normal_and_point(v3f_norm_cross(fbr,ftr),c->position);
}

bool test_in_camera_bounds(vec3f position,float radius,cvm_camera * c)
{
    int i;
    for(i=0;i<num_frustrum_bounds;i++) if(plane_point_dist(c->bounds[i],position) > radius) return false;
    return true;
}



