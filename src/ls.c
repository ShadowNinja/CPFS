#include "cpfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>


void print_size(size_t size)
{
	if (size < 10000) {
		printf("%4u", (unsigned)size);
		return;
	}
	static const char postfixes[] = {'K', 'M', 'G', 'T', 'P', 'E'};
	unsigned pidx = 0;
	size /= 1000;
	while (pidx < sizeof(postfixes) && size >= 1000) {
		++pidx;
		size /= 1000;
	}
	printf("%3u%c", (unsigned)size, postfixes[pidx]);
}

int main(int argc, char *argv[])
{
	CpfsDirIter it;
	CpfsStat st;
	CpfsPath path;
	const char *os_path = argc > 1 ? argv[1] : ".";

	cpfs_path_create(&path, os_path);
	if (!cpfs_dir_create(&it, &path)) {
		cpfs_path_destroy(&path);
		return 1;
	}

#ifdef WIN32
	printf("T|Size|   Last Mod Time    | Filename\n");
#else
	printf("T|Size|      Last Modification Time      | Filename\n");
#endif

	while (cpfs_dir_next(&it)) {
		CpfsPath entry_name, entry_path;
		cpfs_dir_name(&it, &entry_name);
		char *entry_utf8 = cpfs_path_utf8(&entry_name);

		bool is_relative = strcmp(entry_utf8, ".") == 0 || strcmp(entry_utf8, "..") == 0;
		if (is_relative) {
			cpfs_path_utf8_destroy(entry_utf8);
			continue;
		}

		const char *tp_str = cpfs_dir_type(&it) == CPFS_FT_DIRECTORY ? "d" : "-";

		cpfs_path_join(&entry_path, &path, &entry_name);
		if (!cpfs_stat(&st, &entry_path))
			return 1;

		size_t size = cpfs_stat_size(&st);

		CpfsTimeSpec mtime = cpfs_stat_mtime(&st);
		time_t mtime_time = mtime.s;
#ifdef WIN32
		// Windows's %z format specifier is unpredictable, and it doesn't support nanosecond timestamps.
		struct tm *mtime_tm = gmtime(&mtime_time);
		char mtime_str[sizeof("YYYY-MM-DDTHH:MM:SSZ")];
		strftime(mtime_str, sizeof(mtime_str), "%Y-%m-%dT%H:%M:%SZ", mtime_tm);
#else
		struct tm *mtime_tm = localtime(&mtime_time);
		char mtime_str[sizeof("YYYY-MM-DDTHH:MM:SS.000000000-0000")];
		strftime(mtime_str, sizeof(mtime_str), "%Y-%m-%dT%H:%M:%S.*********%z", mtime_tm);
		char mnano_str[10];
		snprintf(mnano_str, sizeof(mnano_str), "%09" PRIu64, mtime.n);
		memcpy(mtime_str + sizeof("YYYY-MM-DDTHH:MM:SS"), mnano_str, 9);
#endif

		printf("%s ", tp_str);
		print_size(size);
		printf(" %s %s\n", mtime_str, entry_utf8);

		cpfs_path_utf8_destroy(entry_utf8);
		cpfs_path_destroy(&entry_path);
	}
	cpfs_dir_destroy(&it);
	cpfs_path_destroy(&path);
	return 0;
}
