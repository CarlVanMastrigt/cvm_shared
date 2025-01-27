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

// no flag for being a regular file
#define CVM_DIRECTORY_FLAG_REGULAR_FILE	0x00000001
#define CVM_DIRECTORY_FLAG_DIRECTORY	0x00000002
// convenience listings are utilities like .. (parent) and . (current)
#define CVM_DIRECTORY_FLAG_CONVENIENCE	0x00000004
#define CVM_DIRECTORY_FLAG_HIDDEN		0x00000008
#define CVM_DIRECTORY_FLAG_LINK			0x00000010
#define CVM_DIRECTORY_FLAG_BLOCK_DEV	0x00000020
#define CVM_DIRECTORY_FLAG_CHAR_DEV		0x00000040
#define CVM_DIRECTORY_FLAG_PIPE			0x00000080
#define CVM_DIRECTORY_FLAG_SOCKET		0x00000100
#define CVM_DIRECTORY_FLAG_UNKNOWN		0x00000200

struct cvm_directory_entry
{
	size_t entry_name_offset;
	// bool is_directory;// subtype value?
	uint32_t flags;
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
	size_t entry_space;
	size_t entry_count_total;
	size_t entry_count_visible;

	uint32_t filter_exclude_flags;
    /// composite buffer to iterate, with double null terminating character ['\0','\0'] at the end to know when to finish
    char* filter_extension_buffer;
    size_t filter_extension_buffer_space;

	char* filename_buffer;///shared buffer for all entry names, allows a good deal of compaction
    size_t filename_buffer_space;
    size_t filename_buffer_offset;

    bool (*sort_function)(const struct cvm_directory_entry*, const struct cvm_directory_entry*, const struct cvm_directory*);/// if null_ptr take as-is (don't sort)
};

static inline const char* cvm_directory_get_entry_name(const struct cvm_directory* directory, const struct cvm_directory_entry* entry)
{
	return directory->filename_buffer + entry->entry_name_offset;
}

// to avoid reloading entries, flagged out ones could be put after the end of some valid_count

// if `initial_path` is NULL will use `utils->fallback_directory`
void cvm_directory_initialise(struct cvm_directory* directory, const char* initial_path, const char* fallback_path);
void cvm_directory_terminate(struct cvm_directory* directory);


void cvm_directory_goto_parent(struct cvm_directory* directory);
void cvm_directory_get_entry_path(struct cvm_directory* directory, uint32_t entry_index, size_t* required_space, char* path_buffer);
void cvm_directory_get_file_path(struct cvm_directory* directory, const char* path_relative_filename, size_t* required_space, char* path_buffer);
void cvm_directory_reload(struct cvm_directory* directory);
void cvm_directory_set_ordering_name(struct cvm_directory* directory, bool reversed);
void cvm_directory_set_ordering_time(struct cvm_directory* directory, bool reversed);
void cvm_directory_set_ordering(struct cvm_directory* directory, bool (*sort_function)(const struct cvm_directory_entry*, const struct cvm_directory_entry*, const struct cvm_directory*));

#endif // CVM_DIRECTORY_H