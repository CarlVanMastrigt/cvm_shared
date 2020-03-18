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
#define CVM_SHARED_H


///change to only include external headers that are actually needed try to reduce count overall
#include <stdbool.h>
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_vulkan.h>

#include <dirent.h>
#include <sys/stat.h>

#include <GL/gl.h>///remove eventually
#include <vulkan/vulkan.h>


///own headers

#include "cvm_math.h"
#include "cvm_data_structures.h"
#include "cvm_random.h"
#include "cvm_gl_functions.h"
#include "cvm_shader.h"
#include "cvm_overlay.h"
#include "cvm_widget.h"
#include "cvm_view.h"
#include "cvm_camera.h"
#include "cvm_managed_buffer.h"
#include "cvm_mesh.h"




#endif

