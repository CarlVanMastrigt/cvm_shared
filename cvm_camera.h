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

#ifndef CVM_CAMERA_H
#define CVM_CAMERA_H

typedef enum
{
    frus_face_t=0,
    frus_face_b,
    frus_face_l,
    frus_face_r,
    num_frustrum_bounds
}
frustrum_bound_names;

typedef struct camera
{
    vec3f position;

    ///these angles represent the direction from which the camera is observing the scene, NOT the direction the camera is facing, within the cameras coordinate system
    float azimuthal_angle;
    float zenith_angle;

    //float focal_distance;
    float max_focal_distance;
    float min_focal_distance;
    int current_zoom_step;
    int max_zoom_step;

    matrix4f view_matrix;
    matrix4f view_matrix_inverse;

    plane bounds[num_frustrum_bounds];

    float fov;///vertical fov
    float near;

    GLsizei screen_w;
    GLsizei screen_h;
    GLfloat aspect_ratio;///
    GLfloat pixel_to_fov_angle_ratio;
    GLfloat inverse_screen_dimensions[2];///x,y
    GLfloat position_buffer[3];
}
camera;

void change_camera_azimuthal_angle(float delta,camera * c);
void change_camera_zenith_angle(float delta,camera * c);
void change_camera_zoom(int delta,camera * c);


void initialise_camera(int screen_w,int screen_h,float fov,float near,int zoom_steps,camera * c);

void update_camera(int screen_w,int screen_h,camera * c);


static inline matrix4f * get_view_matrix_pointer(camera * c)
{
    return &c->view_matrix;
}
static inline matrix4f * get_view_matrix_inverse_pointer(camera * c)
{
    return &c->view_matrix_inverse;
}
matrix4f get_view_matrix(camera * c);
matrix4f get_view_matrix_inverse(camera * c);

bool test_in_camera_bounds(vec3f position,float radius,camera * c);



#endif
