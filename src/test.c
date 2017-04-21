#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "cpfs.h"

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define MYASSERT(a) do { \
	if (!(a)) { \
		printf("Test failed at %s:%d: %s\n", __FILE__, __LINE__, STRINGIFY(a)); \
		return 1; \
	} \
} while (0)

#define CHECK_JOIN_(f, good, ...) do { \
	char *str = f(__VA_ARGS__); \
	MYASSERT(strcmp(str, good) == 0); \
	free(str); \
} while (0)

#define CHECK_JOIN2(good, a, b) CHECK_JOIN_(cpfs_join2, good, a, b)
#define CHECK_JOIN(good, ...) CHECK_JOIN_(cpfs_join, good, __VA_ARGS__)

int main()
{
	CHECK_JOIN2("foo/bar/baz", "foo", "bar/baz");
	CHECK_JOIN2("a", "", "a");
	CHECK_JOIN2("a", "a", "");

	CHECK_JOIN("foo/bar/baz/qux", 3, "foo", "bar/baz", "qux");
	CHECK_JOIN("foo/qux", 5, "", "foo", "", "qux", "");
	CHECK_JOIN("", 3, "", "", "");
	CHECK_JOIN("a/b", 3, "", "a", "b", "");

	printf("Tests completed.\n");
	return 0;
}
