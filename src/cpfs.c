#define _DEFAULT_SOURCE  // For DT_*
#define _FILE_OFFSET_BITS 64

#include "cpfs.h"

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#	include <unistd.h>
#endif


#ifdef _WIN32

#ifndef MB_ERR_INVALID_CHARS
#define MB_ERR_INVALID_CHARS 0
#endif

#ifndef WC_ERR_INVALID_CHARS
#define WC_ERR_INVALID_CHARS 0
#endif

#define cpfs_utf8_to_wide(str) cpfs_utf8_to_wide_extra(str, 0)

static wchar_t* cpfs_utf8_to_wide_extra(const char *str, int extra_space)
{
	int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			str, -1, NULL, 0);
	if (wlen == 0)
		return NULL;
	wchar_t *wstr = calloc(wlen + extra_space, sizeof(wchar_t));
	int res = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			str, -1, wstr, wlen);
	if (res == 0) {
		free(wstr);
		return NULL;
	}
	return wstr;
}

static char* cpfs_wide_to_utf8(const wchar_t *wstr)
{
	int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			wstr, -1, NULL, 0, NULL, NULL);
	if (len == 0)
		return NULL;
	char *str = calloc(len, sizeof(char));
	int res = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			wstr, -1, str, len, NULL, NULL);
	if (res == 0) {
		free(str);
		return NULL;
	}
	return str;
}

#endif  // _WIN32


void cpfs_path_create(CpfsPath *path, const char *utf8)
{
#ifdef _WIN32
	wchar_t *wpath = cpfs_utf8_to_wide(utf8);
	*path = (CpfsPath) { wcslen(wpath), wpath, true };
#else
	*path = (CpfsPath) { strlen(utf8), (char*) utf8, false };
#endif
}


void cpfs_path_destroy(CpfsPath *path)
{
	if (path->owned)
		free(path->path);
}


char* cpfs_path_utf8(CpfsPath *path)
{
#ifdef _WIN32
	return cpfs_wide_to_utf8(path->path);
#else
	return path->path;
#endif
}


void cpfs_path_utf8_destroy(char *path)
{
#ifdef _WIN32
	free(path);
#else
	(void) path;
#endif
}


void cpfs_path_join(CpfsPath *out, const CpfsPath *p1, const CpfsPath *p2)
{
	*out = (CpfsPath) {
		p1->size + p2->size + 1 + (p1->size > 0 && p2->size > 0 ? 1 : 0),
		malloc((p1->size + p2->size + 2) * sizeof(CpfsChar)),
		true
	};
	CpfsChar *cur = out->path;
	memcpy(cur, p1->path, p1->size * sizeof(CpfsChar));
	cur = &cur[p1->size];
	if (p2->size > 0)
		*cur++ = (CpfsChar) DIR_SEP;
	memcpy(cur, p2->path, p2->size * sizeof(CpfsChar));
	cur[p2->size] = (CpfsChar) '\0';
}


char* cpfs_join(unsigned nargs, ...)
{
	assert(nargs > 0);
	va_list argp;
	const char **paths = malloc(nargs * sizeof(const char*));
	size_t *sizes = malloc(nargs * sizeof(size_t));
	ssize_t total_size = 0;  // Size of all paths plus a seperator after each path and NUL

	va_start(argp, nargs);

	for (unsigned char i = 0; i < nargs; ++i) {
		paths[i] = va_arg(argp, const char*);
		sizes[i] = strlen(paths[i]);
		// If the component is empty we just skip it, but if it isn't
		// we'll need space for the dir sep or NUL after the path.
		total_size += sizes[i] == 0 ? 0 : sizes[i] + 1;
	}
	// Make sure we have space for NUL
	char *out = malloc(total_size == 0 ? 1 : total_size);
	char *cur = out;
	for (unsigned char i = 0; i < nargs; ++i) {
		if (sizes[i] == 0)
			continue;
		memcpy(cur, paths[i], sizes[i]);
		cur += sizes[i];
		if (cur - out < total_size - 1)
			*cur++ = DIR_SEP;
	}
	*cur = '\0';

	va_end(argp);
	free(paths);
	free(sizes);

	return out;
}


char* cpfs_join2(const char *p1, const char *p2)
{
	size_t len1 = strlen(p1);
	size_t len2 = strlen(p2);
	char *out = malloc(len1 + len2 + 2);
	char *cur = out;
	if (len1 > 0) {
		memcpy(cur, p1, len1);
		cur += len1;
		if (len2 > 0)
			*cur++ = DIR_SEP;
	}
	memcpy(cur, p2, len2);
	cur[len2] = '\0';
	return out;
}


bool cpfs_stat(CpfsStat *st, const CpfsPath *path)
{
#ifdef _WIN32
	return _wstat64(path->path, &st->st) == 0;
#else
	return stat(path->path, &st->st) != -1;
#endif
}


CpfsFileType cpfs_stat_type(CpfsStat *st)
{
#ifdef _WIN32
	switch (st->st.st_mode & _S_IFMT) {
	case _S_IFDIR: return CPFS_FT_DIRECTORY;
	case _S_IFREG: return CPFS_FT_REGULAR;
	};
	return CPFS_FT_UNKNOWN;
#else
	switch (st->st.st_mode & S_IFMT) {
	case S_IFBLK: return CPFS_FT_BLOCK_DEVICE;
	case S_IFCHR: return CPFS_FT_CHAR_DEVICE;
	case S_IFDIR: return CPFS_FT_DIRECTORY;
	case S_IFIFO: return CPFS_FT_FIFO;
	case S_IFLNK: return CPFS_FT_SYMLINK;
	case S_IFREG: return CPFS_FT_REGULAR;
	case S_IFSOCK: return CPFS_FT_SOCKET;
	};
	return CPFS_FT_UNKNOWN;
#endif
}


uint64_t cpfs_stat_size(CpfsStat *st)
{
	return st->st.st_size;
}


#ifdef st_atime
// st_*time are alliased to st_*tim, grab the values directly
#define GET_STAT_TIME(t) (CpfsTimeSpec){ st->st.st_##t##tim.tv_sec, st->st.st_##t##tim.tv_nsec }
#else
#define GET_STAT_TIME(t) (CpfsTimeSpec){ st->st.st_##t##time, 0 }
#endif


CpfsTimeSpec cpfs_stat_atime(CpfsStat *st) { return GET_STAT_TIME(a); }
CpfsTimeSpec cpfs_stat_mtime(CpfsStat *st) { return GET_STAT_TIME(m); }
CpfsTimeSpec cpfs_stat_ctime(CpfsStat *st) { return GET_STAT_TIME(c); }


bool cpfs_exists(const CpfsPath *path)
{
	CpfsStat st;
	return cpfs_stat(&st, path);
}


bool cpfs_is_file(const CpfsPath *path)
{
	CpfsStat st;
	if (!cpfs_stat(&st, path)) return false;
	return cpfs_stat_type(&st) == CPFS_FT_REGULAR;
}


bool cpfs_is_directory(const CpfsPath *path)
{
	CpfsStat st;
	if (!cpfs_stat(&st, path)) return false;
	return cpfs_stat_type(&st) == CPFS_FT_DIRECTORY;
}


bool cpfs_create_directory(const CpfsPath *path)
{
#ifdef _WIN32
	return CreateDirectoryW(path->path, NULL);
#else
	return mkdir(path->path, 0777) == 0;
#endif
}


FILE* cpfs_file_open(const char *path, const char *mode)
{
#ifdef _WIN32
	// _wfopen requires the mode to also be a wide string, even though no
	// wide characters are supported.  We'll just do a quick assignment
	// instead of a full-blown encoding conversion.
	wchar_t wmode[4];
	unsigned mode_len = strlen(mode);
	assert(mode_len < sizeof(wmode) / sizeof(*wmode));
	for (unsigned i = 0; i < mode_len; ++i) {
		wmode[i] = (wchar_t) mode[i];
	}

	wchar_t *wpath = cpfs_utf8_to_wide(path);
	FILE *stream = _wfopen(wpath, wmode);
	free(wpath);
	return stream;
#else
	return fopen(path, mode);
#endif
}


bool cpfs_remove(const CpfsPath *path)
{
#ifdef _WIN32
	struct _stat64 st;
	if (_wstat64(path->path, &st) != 0)
		return false;
	if ((st.st_mode & _S_IFMT) == _S_IFDIR) {
		return RemoveDirectoryW(path->path);
	} else {
		return DeleteFileW(path->path);
	}
#else
	return remove(path->path) == 0;
#endif
}

bool cpfs_remove_recursive(const CpfsPath *path)
{
	CpfsDirIter it;
	bool good = cpfs_dir_create(&it, path);
	if (!good)
		return false;
	while (cpfs_dir_next(&it)) {
		CpfsPath entry_name, entry_path;
		cpfs_dir_name(&it, &entry_name);
		cpfs_path_join(&entry_path, path, &entry_name);
		if (cpfs_dir_type(&it) == CPFS_FT_DIRECTORY) {
			good = cpfs_remove_recursive(&entry_path);
		}
		if (good)
			good = cpfs_remove(&entry_path);
		cpfs_path_destroy(&entry_path);
		if (!good)
			break;
	}
	cpfs_dir_destroy(&it);
	return good;
}


bool cpfs_dir_create(CpfsDirIter *dir, const CpfsPath *path)
{
#ifdef _WIN32
	dir->path = malloc((path->size + 2) * sizeof(wchar_t));
	memcpy(dir->path, path->path, path->size * sizeof(wchar_t));
	dir->path[path->size] = L'\\';
	dir->path[path->size+1] = L'*';
	dir->path[path->size+2] = L'\0';
	dir->handle = INVALID_HANDLE_VALUE;
	return true;
#else
	dir->dir = opendir(path->path);
	return dir->dir != NULL;
#endif
}


void cpfs_dir_destroy(CpfsDirIter *dir)
{
#ifdef _WIN32
	free(dir->path);
	if (dir->handle != INVALID_HANDLE_VALUE)
		FindClose(dir->handle);
#else
	closedir(dir->dir);
#endif
}


// When this returns NULL you've finished traversing the directory.
bool cpfs_dir_next(CpfsDirIter *dir)
{
#ifdef _WIN32
	if (dir->path != NULL) {  // First call ro read_dir
		dir->handle = FindFirstFileW(dir->path, &dir->data);
		free(dir->path);
		dir->path = NULL;
		return dir->handle != INVALID_HANDLE_VALUE;
	} else {
		return FindNextFileW(dir->handle, &dir->data) != 0;
	}
#else
	dir->entry = readdir(dir->dir);
	return dir->entry != NULL;
#endif
}


void cpfs_dir_name(CpfsDirIter *dir, CpfsPath *name)
{
#ifdef _WIN32
	*name = (CpfsPath) { wcslen(dir->data.cFileName), dir->data.cFileName, false };
#else
	*name = (CpfsPath) { strlen(dir->entry->d_name), dir->entry->d_name, false };
#endif
}


CpfsFileType cpfs_dir_type(CpfsDirIter *dir)
{
#ifdef _WIN32
	return (dir->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
		CPFS_FT_DIRECTORY : CPFS_FT_REGULAR;
#elif defined(_DIRENT_HAVE_D_TYPE)
	switch (dir->entry->d_type) {
	case DT_BLK: return CPFS_FT_BLOCK_DEVICE;
	case DT_CHR: return CPFS_FT_CHAR_DEVICE;
	case DT_DIR: return CPFS_FT_DIRECTORY;
	case DT_FIFO: return CPFS_FT_FIFO;
	case DT_LNK: return CPFS_FT_SYMLINK;
	case DT_REG: return CPFS_FT_REGULAR;
	case DT_SOCK: return CPFS_FT_SOCKET;
	}
	return CPFS_FT_UNKNOWN;
#else
	return CPFS_FT_UNKNOWN;
#endif
}
