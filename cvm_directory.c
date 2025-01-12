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

#define _XOPEN_SOURCE 700

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "cvm_utils.h"

#include "cvm_directory.h"

#include <sys/stat.h>
#include <dirent.h>




#include <errno.h>



static inline void cvm_directory_add_entry(struct cvm_directory* directory, char* name, uint32_t flags, uint64_t mod_t_sec, uint32_t mod_t_nsec)
{
	size_t len = strlen(name) + 1;//+1 for /0 terminator character
	assert(mod_t_nsec < 1000000000);

	if(directory->entry_total_count == directory->entry_space)
	{
		directory->entry_space *= 2;
		directory->entries = realloc(directory->entries, sizeof(struct cvm_directory_entry) * directory->entry_space);
	}
	directory->entries[directory->entry_total_count++] = (struct cvm_directory_entry)
	{
		.entry_name_offset = directory->filename_buffer_offset,
		.flags = flags,
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
static inline void cvm_directory_ensure_path_space(struct cvm_directory* directory, size_t required_space)
{
	if(required_space > directory->path_space)
    {
    	do directory->path_space *= 2;
    	while(required_space > directory->path_space);
    	directory->path = realloc(directory->path, sizeof(char) * directory->path_space);
    }
}


// static inline int cvm_directory_sort_function(const void* a, const void* b, void* context)
// {
// 	const struct cvm_directory* directory = context;
// 	const struct cvm_directory_entry* entry_1 = a;
// 	const struct cvm_directory_entry* entry_2 = b;

// 	#warning need to apply filter here as that overrides sort
// 	/// ALSO can track count for filtering!

// 	if(directory->sort_function)
// 	{
// 		return directory->sort_function(directory, entry_1, entry_2);
// 	}
// 	else
// 	{
// 		return 0;
// 	}
// }

static inline void cvm_directory_sort(struct cvm_directory* directory)
{
	// qsort_s(directory->entries, directory->entry_total_count, sizeof(struct cvm_directory_entry), &cvm_directory_sort_function, directory);
	//cannot use qsort() easily as entries need to access external data, instead need to sort with own function...
}






bool cvm_directory_util_set_path(struct cvm_directory* directory, const char* path);
void cvm_directory_util_scan(struct cvm_directory* directory);
void cvm_directory_util_goto_parent(struct cvm_directory* directory);
void cvm_directory_util_get_path(struct cvm_directory* directory, const char* to_append, size_t* required_space, char* path_buffer);



/// this function sets length so it must not use extant length
bool cvm_directory_util_set_path(struct cvm_directory* directory, const char* path)
{
	char* new_path;
	size_t len;

	#warning need to handle errors (in a better way), specifically a missing folder path, which should be resolvable/constructable? -- create with force
	
	new_path = realpath(path, NULL);
	if(new_path)
	{
		len = strlen(new_path);
		cvm_directory_ensure_path_space(directory, len + 2);
		strcpy(directory->path, new_path);
		if((len>0) && (directory->path[len-1] != '/'))
		{
			directory->path[len++] = '/';
			directory->path[len] = '\0';
		}
		free(new_path);
		directory->path_length = len;
		return true;
	}
	return false;
}

#warning should probably only support regular files and directories unless requested otherwise
/// bitmask of options?? -- options could include listing .. and . "LIST_CONVENIENCE_ENTRIES"
void cvm_directory_util_scan(struct cvm_directory* directory)
{
	DIR * d;
    struct dirent* e;
    struct stat s;
    size_t len;
    uint32_t flags;

    directory->entry_total_count = 0;
    directory->entry_filtered_count = 0;
    directory->filename_buffer_offset = 0;

    d = opendir(directory->path);

    if(!d)
    {
    	// this shouldn't happen, should fix this issue when path is set (cvm_directory_util_set_path)
    	fprintf(stderr,"dir to open invalid (%s)\n", directory->path);
    	return;
    }

    while((e=readdir(d)))
    {
    	len = strlen(e->d_name);
    	// use directory->path to stash the filepath being assessed
    	cvm_directory_ensure_path_space(directory, directory->path_length + len + 1);
    	memcpy(directory->path + directory->path_length, e->d_name, sizeof(char) * (len + 1));
    	stat(directory->path, &s);

    	assert(__builtin_popcount(s.st_mode & S_IFMT) <= 1);// only 1 should be set
    	switch (s.st_mode & S_IFMT) {
    		case S_IFBLK:	flags = CVM_DIRECTORY_FLAG_BLOCK_DEV;    break;
			case S_IFCHR:	flags = CVM_DIRECTORY_FLAG_CHAR_DEV;     break;
			case S_IFDIR:	flags = CVM_DIRECTORY_FLAG_DIRECTORY;    break;
			case S_IFIFO:	flags = CVM_DIRECTORY_FLAG_PIPE;         break;
			case S_IFLNK:	flags = CVM_DIRECTORY_FLAG_LINK;         break;// can a link also be a directory?? if so we should handle that
			case S_IFREG:	flags = CVM_DIRECTORY_FLAG_REGULAR_FILE; break;
			case S_IFSOCK:	flags = CVM_DIRECTORY_FLAG_SOCKET;       break;
			default:        flags = CVM_DIRECTORY_FLAG_UNKNOWN;      break;
    	}

    	// are convenience files implicitly hidden?
    	if( !strcmp(e->d_name, ".") || !strcmp(e->d_name, ".") ) flags |= CVM_DIRECTORY_FLAG_CONVENIENCE;
    	if( len && e->d_name[0] == '.') flags |= CVM_DIRECTORY_FLAG_HIDDEN;

    	printf("%s : %s : %lu : %u\n", S_ISDIR(s.st_mode)?"folder":(S_ISREG(s.st_mode)?"file":"?"), directory->path, s.st_mtim.tv_sec, s.st_mtim.tv_nsec);
    	cvm_directory_add_entry(directory, e->d_name, flags, s.st_mtim.tv_sec, s.st_mtim.tv_nsec);
    	directory->path[directory->path_length] = '\0';
    }
}

void cvm_directory_util_goto_parent(struct cvm_directory* directory)
{
	bool parent_exists;
	// lil bit hacky
	cvm_directory_ensure_path_space(directory, directory->path_length + 3);
	strcpy(directory->path + directory->path_length, "..");
	parent_exists = cvm_directory_util_set_path(directory, directory->path);
	assert(parent_exists);
}

void cvm_directory_util_get_path(struct cvm_directory* directory, const char* to_append, size_t* required_space, char* path_buffer)
{
	size_t append_len;
	struct stat s;
	int err;

	// when making space, add 2, one of '\0' and one ofr a directory marking '/' in case the entry accessed is a directory
	append_len = to_append ? strlen(to_append) : 0;

	if(path_buffer)
	{
		assert(*required_space >= directory->path_length + append_len + 2);
		strcpy(path_buffer, directory->path);
		if(to_append)
		{
			strcpy(path_buffer + directory->path_length, to_append);
			// for consistency, if accessed entry is a directory append a '/'
			err = stat(path_buffer, &s);
			if(!err && S_ISDIR(s.st_mode) && (path_buffer[directory->path_length + append_len - 1] != '/'))
			{
				strcpy(path_buffer + directory->path_length + append_len , "/");
			}
		}
	}
	else
	{
		*required_space = directory->path_length + append_len + 2;
	}
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
		.entry_total_count = 0,
		.entry_filtered_count = 0,

		.filter_exclude_flags = 0,/// set this!
		.filter_extension_buffer = malloc(sizeof(char)*64),
		.filter_extension_buffer_space = 64,

		.filename_buffer = malloc(sizeof(char)*4096),
		.filename_buffer_space = 4096,
		.filename_buffer_offset = 0,

		.sort_function = NULL,// set this!
	};

	cvm_directory_util_set_path(directory, initial_path);
	cvm_directory_util_scan(directory);
}

void cvm_directory_terminate(struct cvm_directory* directory)
{
	free(directory->fallback_path);
	free(directory->path);
	free(directory->entries);
	free(directory->filter_extension_buffer);
	free(directory->filename_buffer);
}

void cvm_directory_goto_parent(struct cvm_directory* directory)
{
	cvm_directory_util_goto_parent(directory);
	cvm_directory_util_scan(directory);
}

void cvm_directory_get_entry_path(struct cvm_directory* directory, uint32_t entry_index, size_t* required_space, char* path_buffer)
{
	char* entry_name;

	assert(entry_index < directory->entry_filtered_count);

	entry_name = directory->filename_buffer + directory->entries[entry_index].entry_name_offset;

	cvm_directory_util_get_path(directory, entry_name, required_space, path_buffer);
}

void cvm_directory_get_file_path(struct cvm_directory* directory, const char* path_relative_filename, size_t* required_space, char* path_buffer)
{
	cvm_directory_util_get_path(directory, path_relative_filename, required_space, path_buffer);
}

void cvm_directory_reload(struct cvm_directory* directory)
{
	///
}

void cvm_directory_set_ordering_name(struct cvm_directory* directory, bool reversed)
{
	//
}

void cvm_directory_set_ordering_time(struct cvm_directory* directory, bool reversed)
{
	//
}

void cvm_directory_set_ordering_custom(struct cvm_directory* directory, int (*sort_function)(const struct cvm_directory*, const struct cvm_directory_entry*, const struct cvm_directory_entry*))
{
	if(sort_function != directory->sort_function)
	{
		directory->sort_function = sort_function;
		cvm_directory_sort(directory);
	}
}


// for widgets that reference this we should avoid having anything built on top of it, this becomes complicated in the case of text bar though (for selections)
// this can be done in custom fashion though (validating contents and querying basis before invalidating local contents)
// it DOES however mean we should move all sort/filter operations to the directory