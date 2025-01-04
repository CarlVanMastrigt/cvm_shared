/**
Copyright 2025 Carl van Mastrigt

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

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static inline char* cvm_strdup(const char * in)
{
    if(!in) return NULL;
    size_t len=(strlen(in)+1)*sizeof(char);
    char * out=malloc(len);
    return memcpy(out,in,len);
}