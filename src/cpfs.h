#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#	define DIR_SEP '\\'
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#else
#	define DIR_SEP '/'
#	include <dirent.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Opaque struct that holds directory iteration state
typedef struct {
#ifdef _WIN32
	wchar_t *path;
	HANDLE handle;
	WIN32_FIND_DATAW data;
#else
	DIR *dir;
	struct dirent *entry;
#endif
} CpfsDirIter;

// Opaque struct that holds information about a filesystem entry
typedef struct {
#ifdef _WIN32
	struct _stat64 st;
#else
	struct stat st;
#endif
} CpfsStat;

#ifdef _WIN32
typedef wchar_t CpfsChar;
#else
typedef char CpfsChar;
#endif

// Holds an os-specific filesystem path.
typedef struct {
	size_t size;
	CpfsChar* path;
	bool owned;
} CpfsPath;

// Public emun of file types.  Not all are supported on all platforms.
typedef enum {
	CPFS_FT_BLOCK_DEVICE,
	CPFS_FT_CHAR_DEVICE,
	CPFS_FT_DIRECTORY,
	CPFS_FT_FIFO,
	CPFS_FT_SYMLINK,
	CPFS_FT_REGULAR,
	CPFS_FT_SOCKET,
	CPFS_FT_UNKNOWN,
} CpfsFileType;

// Holds a time in unix format, seperated into seconds and nanoseconds.
typedef struct { uint64_t s, n; } CpfsTimeSpec;


// Creates a new CpfsPath from a UTF-8 string.
void cpfs_path_create(CpfsPath *path, const char *utf8);

// Converts a CpfsPath to a new UTF-8 string.  The resources associated with
// the string must be released with cpfs_path_utf8_destroy().
char* cpfs_path_utf8(CpfsPath *path);

// Releases resources associated with a UTF-8 string created by cpfs_path_utf8().
void cpfs_path_utf8_destroy(char *path);

// Releasess resources associated with a CpfsPath.
void cpfs_path_destroy(CpfsPath *path);

// Creates a new path composed of the two passed path components.
// Note: path resources must be released with cpfs_path_destroy().
void cpfs_path_join(CpfsPath *out, const CpfsPath *p1, const CpfsPath *p2);

// Joins an arbitrary number of path segments into a single path,
// with each path seperated by the system's directory seperator.
// Note: nargs must be correct or this will invoke undefined behavior.
// Use cpfs_join2 if you only need to join two elements.
char *cpfs_join(unsigned nargs, ...);

// Joins two path segments into one, each path seperated by the system's directory seperator.
// Simpler and faster than cpfs_join.
char *cpfs_join2(const char *p1, const char *p2);


// Retreives information about a file and loads it into a stat structure.
// Returns whether the information was successfully fetched.
bool cpfs_stat(CpfsStat *st, const CpfsPath *path);

// Retrieves the file type from a stat structure.
CpfsFileType cpfs_stat_type(CpfsStat *st);

// Retrieves the file size from a stat structure.
uint64_t cpfs_stat_size(CpfsStat *st);

// Retrieves the access time from a stat structure.
CpfsTimeSpec cpfs_stat_atime(CpfsStat *st);

// Retrieves the modification time from a stat structure.
CpfsTimeSpec cpfs_stat_mtime(CpfsStat *st);

// Retrieves the creation time from a stat structure.
CpfsTimeSpec cpfs_stat_ctime(CpfsStat *st);

// Checks if the path exists.
bool cpfs_exists(const CpfsPath *path);

// Checks if the paths points to an existing file.
bool cpfs_is_file(const CpfsPath *path);

// Checks if the path points to an existing directory.
bool cpfs_is_directory(const CpfsPath *path);


// Attempts to creates a directory at the specified path.
// Returns whether the directory was successfully created.
bool cpfs_create_directory(const CpfsPath *path);

// Opens a new file with the specified mode.
// Like fopen(), but supports wide chars on Windows.
FILE* cpfs_file_open(const char *path, const char *mode);

// Removes a file or empty directory.  Returns whether the operation succeeded.
bool cpfs_remove(const CpfsPath *path);

// Recursively removes all files and directories in a directory tree.
// Returns whether the operation succeeded.
bool cpfs_remove_recursive(const CpfsPath *path);


// Initializes a directory iterator for the directory at the specified path.
// Note: You must call cpfs_dir_destroy() on the iterator once you're finished with it.
bool cpfs_dir_create(CpfsDirIter *dir, const CpfsPath *path);

// Releases resources used by a directory iterator.
// Note: The iterator must be initialized (via a call to cpfs_dir_create()).
void cpfs_dir_destroy(CpfsDirIter *dir);

// Advances the directory iterator to the next item in the directory.
// Returns whether there are further items in the directory.
bool cpfs_dir_next(CpfsDirIter *dir);

// Places the name of the current entry pointed to by a directory iterator in name.
// Note: the name does not need to be destroyed with cpfs_path_destroy().
void cpfs_dir_name(CpfsDirIter *dir, CpfsPath *name);

// Returns the type of the entry pointed to by the directory iterator.
// Note: This is not always supported well if at all.  If the type can
// not be determined CPFS_FT_UNKNOWN will be returned.  You can use
// cpfs_stat() to try to find the entry type if this fails.
CpfsFileType cpfs_dir_type(CpfsDirIter *dir);

#ifdef __cplusplus
}
#endif
