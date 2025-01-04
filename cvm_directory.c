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

#include <stdlib.h>
#include <stdio.h>

#include "cvm_utils.h"

#include "cvm_directory.h"

#include <sys/stat.h>
#include <dirent.h>

// all the tools to access a directory, should be created ONCE based on operating system
struct cvm_directory_utils
{
	uint32_t acquire_count;// basically reference count, used to ensure it gets cleaned up properly


	// in byres (for UTF-8), 0 for unlimited, should include extension and null terminator
	uint32_t max_path_length;
	uint32_t max_entry_length;
	// fallback_directory is guaranteed to exist
	const char* fallback_directory;

	// if query is true, will not attempt to change directory, just query if its possible
	bool (*const up_dir)   (struct cvm_directory*, uint32_t count, bool query);

	bool (*const make_dir) (struct cvm_directory*, char* dir_name);

	// not sure this one should exist...
	bool (*const set_dir)  (struct cvm_directory*, char* full_path);

	// entry index must correspond to a directory
	bool (*const enter_index) (struct cvm_directory*, uint32_t entry_index);

	// entry index must correspond to a directory
	bool (*const enter_name) (struct cvm_directory*, uint32_t entry_index);

	/**
	 * can be used to acquire a file to fopen
	 * if `path` is NULL, will return the required size of path in `path_bytes`, otherwise `path` MUST have `*path_bytes` bytes of space, which must have been returned by a prior call
	 * */
	bool (*const get_entry_full_path) (struct cvm_directory*, uint32_t entry_index, char* path, uint32_t* path_bytes);

	/**
	 * can be used to acquire a file to fopen
	 * if `path` is NULL, will return the required size of path in `path_bytes`, otherwise `path` MUST have `*path_bytes` bytes of space, which must have been returned by a prior call
	 * */
	bool (*const get_file_full_path)  (struct cvm_directory*, const char* filename, char* path, uint32_t* path_bytes);
};

/// have these per platform ??
struct cvm_directory_utils* cvm_directory_utils_acquire(void);
struct cvm_directory_utils* cvm_directory_utils_release(void);


#warning instead of above could employ a base set of functions that get used







static inline void add_entry(struct cvm_directory* directory, char* name, bool is_dir, uint64_t mod_t_sec, uint32_t mod_t_nsec)
{
	size_t len = strlen(name) + 1;//+1 for /0 terminator character

	if(directory->entry_count == directory->entry_space)
	{
		directory->entry_space *= 2;
		directory->entries = realloc(directory->entries, sizeof(struct cvm_directory_entry) * directory->entry_space);
	}
	directory->entries[directory->entry_count++] = (struct cvm_directory_entry)
	{
		.entry_name_offset = directory->filename_buffer_offset,
		.is_dir = is_dir,
		.mod_t_nsec = mod_t_nsec,
		.mod_t_sec = mod_t_sec,
	};

	while(directory->filename_buffer_offset + len > directory->filename_buffer_space)
	{
		directory->filename_buffer_space *= 2;
		directory->filename_buffer = realloc(directory->filename_buffer, sizeof(char) * directory->filename_buffer_space);
	}
	
	memcpy(directory->filename_buffer + directory->filename_buffer_offset, name, sizeof(char) * len);
	directory->filename_buffer_offset += len;
}




///file directory should map indices (0-n in display) to relevant lookup information (type and index in dir) 
static void ensure_path_space(struct cvm_directory* directory, size_t required_space)
{
	if(required_space > directory->path_space)
    {
    	do directory->path_space *= 2;
    	while(required_space > directory->path_space);
    	directory->path = realloc(directory->path, sizeof(char) * directory->path_space);
    }
}





void cvm_clean_path_dirent(struct cvm_directory* directory, bool append_dir_marker)
{
	const char* read;
	char* write;
	size_t len;

	if(append_dir_marker)
	{
		len = strlen(directory->path);
		ensure_path_space(directory, len + 2);
		if(len && (directory->path[len-1] != '/'))
		{
			directory->path[len] = '/';
			directory->path[len+1] = '\0';
		}
		else
		{
			directory->path[0] = '/';
			directory->path[1] = '\0';
		}
	}

	read = directory->path;
	write = directory->path;

    while(*read)
    {
        while(*read=='/')
        {
            if((read[1] == '.') && (read[2] == '.') && (read[3] == '/'))// go up a layer when encountering ../
            {
                read += 3;// skip read over ../
                // for both: ensure we don't skip over root /
                if(write > directory->path) write--;// reverse over most recent '/'
                while((write > directory->path) && (*write != '/')) write--; // move write back one dir marker if possible
            }
            else if((read[1]=='.') && (read[2]=='/')) read += 2;///remove ./
            else if(read[1]=='/') read++;// remove multiple /
            else break;
        }

        if(write != read) *write = *read;// avoid reading and writing same char

        read++;
        write++;
    }

    *write = '\0';

    directory->path_length = write - directory->path;
}

#warning should probably only support regular files and directories unless requested otherwise
/// bitmask of options??
static inline bool cvm_scan_dir_dirent(struct cvm_directory* directory, const char* path)
{
	DIR * d;
    struct dirent* e;
    struct stat s;
    size_t len;

    len = strlen(path);
    ensure_path_space(directory, len + 2);// need space to pas with / and \0
    memcpy(directory->path, path, sizeof(char) * (len + 1));// copy with \0
    cvm_clean_path_dirent(directory, true);// also sets length

    directory->entry_count = 0;
    directory->filename_buffer_offset = 0;

    d = opendir(path);

    if(!d)
    {
    	return false;
    }

    while((e=readdir(d)))
    {
    	len = strlen(e->d_name);
    	ensure_path_space(directory, directory->path_length + len + 1);
    	memcpy(directory->path + directory->path_length, e->d_name, sizeof(char) * (len + 1));
    	stat(directory->path, &s);
    	add_entry(directory, e->d_name, S_ISDIR(s.st_mode), s.st_mtime, 0);
    	directory->path[directory->path_length] = '\0';
    }
    // directory->path[directory->path_length] = '\0';

}



void cvm_directory_initialise(struct cvm_directory* directory, const char* initial_path, const char* fallback_path)
{
	size_t initial_path_length;

	*directory = (struct cvm_directory)
	{
		.fallback_path = cvm_strdup(fallback_path),
		.path = malloc(sizeof(char)*64),
		.path_space = 64,

		.entries = malloc(sizeof(struct cvm_directory_entry)*16),
		.entry_space = 16,
		.entry_count = 0,

		.filename_buffer = malloc(sizeof(char)*4096),
		.filename_buffer_space = 4096,
		.filename_buffer_offset = 0,
	};

	cvm_scan_dir_dirent(directory, initial_path);


}

void cvm_directory_terminate(struct cvm_directory* directory)
{
	free(directory->fallback_path);
}