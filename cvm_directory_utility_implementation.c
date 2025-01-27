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

#ifdef CVM_DIRECTORY_UTILITY_IMPLEMENTATION



#include <sys/stat.h>
#include <dirent.h>

/// this function sets length so it must not use extant length
bool cvm_directory_utility_set_path(struct cvm_directory* directory, const char* path)
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
	else
	{
		fprintf(stderr,"attempted to set invalid directory (%s)\n", directory->path);
		return false;
	}
}

void cvm_directory_utility_scan(struct cvm_directory* directory)
{
	DIR * d;
    struct dirent* e;
    struct stat s;
    size_t len;
    uint32_t flags;

    directory->entry_count_total = 0;
    directory->entry_count_visible = 0;
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

    	cvm_directory_add_entry(directory, e->d_name, flags, s.st_mtim.tv_sec, s.st_mtim.tv_nsec);
    	directory->path[directory->path_length] = '\0';
    }
}

void cvm_directory_utility_goto_parent(struct cvm_directory* directory)
{
	bool parent_exists;
	// lil bit hacky
	cvm_directory_ensure_path_space(directory, directory->path_length + 3);
	strcpy(directory->path + directory->path_length, "..");
	parent_exists = cvm_directory_utility_set_path(directory, directory->path);
	assert(parent_exists);
}

void cvm_directory_utility_get_path(struct cvm_directory* directory, const char* to_append, size_t* required_space, char* path_buffer)
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


#endif