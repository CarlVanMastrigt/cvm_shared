/**
Copyright 2024,2025 Carl van Mastrigt

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
#include <stddef.h>
#include <stdbool.h>

#ifndef CVM_DIRECTORY_H
#define CVM_DIRECTORY_H

/// cannot and will not do path length validation as no guarantees exist cross platform

struct cvm_directory_entry
{
	uint32_t entry_name_offset;
	// bool is_directory;// subtype value?
	uint32_t _reserved:1;
	uint32_t is_dir:1;
	uint32_t mod_t_nsec:30;
	uint64_t mod_t_sec;
};

struct cvm_directory
{
	char* fallback_path;

	char* path;// current path, may be relative or absolute (prefer absolute, perhaps enforce)
	size_t path_length;
	size_t path_space;



	struct cvm_directory_entry* entries;
	size_t entry_count;
	size_t entry_space;

	char * filename_buffer;///shared buffer for all entry names, allows a good deal of compaction
    size_t filename_buffer_space;
    size_t filename_buffer_offset;
};

// if `initial_path` is NULL will use `utils->fallback_directory`
void cvm_directory_initialise(struct cvm_directory* directory, const char* initial_path, const char* fallback_path);
void cvm_directory_terminate(struct cvm_directory* directory);

#endif // CVM_DIRECTORY_H