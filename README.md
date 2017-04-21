CPFS - simple Cross-Platform FileSystem library
===

Documentation
---

See `src/cpfs.h` for documentation on all of the included functions.

### Example

Here's a snippet of code to list all entries in a directory.  See `src/ls.c` for a full example.
```C
CpfsPath path;
cpfs_path_create(&path, "/tmp");
CpfsDirIter it;
if (!cpfs_dir_create(&it, &path)) {
	cpfs_path_destroy(&path);
	return;
}
while (cpfs_dir_next(&it)) {
	CpfsPath name;
	cpfs_dir_name(&it, &name);
	char *name_utf8 = cpfs_path_utf8(&name);
	printf("%s\n", name_utf8);
	cpfs_path_utf8_destroy(name_utf8);
}
cpfs_dir_destroy(&it);
cpfs_path_destroy(&path);
```
