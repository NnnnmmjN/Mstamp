// Copyright 2023 Alexey Kutepov <reximkut@gmail.com>
//
// before including:
//		#define NOB_IMPLEMENTATION
//		#include "nob.h"
//
// for local temporary allocator:
//		#define NOB_TEMP_CAPACITY ...
//
// verbose mode:
//		#define NOB_VERBOSE

#ifndef NOB_H
#define NOB_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	define _WINUSER_
#	define _WINGDI_
#	define _IMM_
#	define _WINCON_
#	include <windows.h>
#	include <direct.h>
#	include <shellapi.h>
#	undef WIN32_LEAN_AND_MEAN
#else
#	include <sys/types.h>
#	include <sys/wait.h>
#	include <sys/stat.h>
#	include <unistd.h>
#	include <fcntl.h>
#endif



#ifdef _WIN32
#	define NOB_LINE_END "\r\n"
#else
#	define NOB_LINE_END "\n"
#endif

#ifndef NOB_ASSERT
#	include <assert.h>
#	define NOB_ASSERT assert
#endif
#ifndef NOB_REALLOC
#	define NOB_REALLOC realloc
#endif
#ifndef NOB_FREE
#	define NOB_FREE free
#endif
#ifndef NOB_GET_ERRNO
#	define NOB_GET_ERRNO strerror(errno)
#endif

#define NOB_UNUSED(value) (void)(value)
#define NOB_TODO(message) do { fprintf(stderr, "%s:%d [TODO] %s"NOB_LINE_END, __FILE__, __LINE__, message); abort(); } while (0)
#define NOB_UNREACHABLE(message) do { fprintf(stderr, "%s:%d: [UNREACHABLE] %s"NOB_LINE_END, __FILE__, __LINE__, message); abort(); } while (0)

#define NOB_ARRAY_LEN(array) (sizeof(array) / sizeof(*array))
#define NOB_ARRAY_GET(array, index) (NOB_ASSERT(index >= 0), NOB_ASSERT(index < NOB_ARRAY_LEN(array)), array[index])

typedef enum {
	NOB_NONE,
	NOB_INFO,
	NOB_WARNING,
	NOB_ERROR,
	NOB_DEBUG,
} Nob_LogLevel;

#ifndef NOB_COLOR_LOG
#define NOB_COLOR_LOG 1
#endif
void nob_log(Nob_LogLevel level, const char *fmt, ...);
char *nob_shift_args(int *argc, char ***argv);
#define shift_args nob_shift_args(&argc, &argv)
#define nob_shift(xs, xs_size) (NOB_ASSERT((xs_size) > 0), (xs_size)--, *(xs)++)



typedef struct {
	const char **items;
	size_t count;
	size_t capacity;
} Nob_FilePaths;

typedef enum {
	NOB_FILE_REGULAR = 0,
	NOB_FILE_DIRECTORY,
	NOB_FILE_SYMLINK,
	NOB_FILE_OTHER,
} Nob_FileType;

bool nob_mkdir_if_not_exists(const char *path);
bool nob_copy_file(const char *src_path, const char *dst_path);
bool nob_copy_directory_recursively(const char *src_path, const char *dst_path);
bool nob_read_entire_dir(const char *parent, Nob_FilePaths *children);
bool nob_write_entire_file(const char *path, void *data, size_t size);
Nob_FileType nob_get_file_type(const char *path);



// requires declaration of `result` and `defer`
#define nob_return_label(value, label) do { result = (value); goto label; } while (0)
#define nob_return_defer(value) do { result = (value); goto defer; } while (0)
#define nob_return_defer_msg(value, ...) do { result = (value); fprintf(stderr, ""__VA_ARGS__); goto defer; } while (0)

// dynamic arrays
#ifndef DA_INIT_CAP
#define DA_INIT_CAP 256UL
#endif 
#define nob_da_append(da, item)																\
	do {																					\
		if ((da)->count >= (da)->capacity) {												\
			(da)->capacity = ((da)->capacity == 0UL) * DA_INIT_CAP + 						\
							 ((da)->capacity != 0UL) * ((da)->capacity * 2UL);				\
			(da)->items = NOB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));	\
			NOB_ASSERT((da)->items != NULL && "Need more RAM");								\
		}																					\
																							\
		(da)->items[(da)->count++] = (item);												\
	} while (0)

#define nob_da_append_many(da, new_items, new_items_count)										\
	do {																						\
		if ((da)->count + new_items_count >= (da)->capacity) {									\
			(da)->capacity = ((da)->capacity == 0UL) * DA_INIT_CAP + 							\
							 ((da)->capacity != 0UL) * (da)->capacity;							\
			while ((da)->count + new_items_count >= (da)->capacity) (da)->capacity *= 2UL;		\
			(da)->items = NOB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));		\
			NOB_ASSERT((da)->items != NULL && "Need more RAM");									\
		}																						\
		memcpy((da)->items + (da)->count, new_items, new_items_count * sizeof(*(da)->items));	\
		(da)->count += new_items_count;															\
	} while (0)

#define nob_da_foreach(da, type, item) \
	for (type *item = (da)->items; item < (da)->items + (da)->count; item++)
#define nob_da_enumerate(da, type, item, index, iterator) \
	for (struct {size_t index; type *item;} it = {0}; (it.item = &(da)->items[it.i], it.index < (da)->count); it.index++)

#define nob_da_free(da) do { if ((da)->items) NOB_FREE((da)->items); (da)->items = NULL; } while (0)



typedef struct {
	char *items;
	size_t count;
	size_t capacity;
} Nob_StringBuilder;

// Append a sized buffer to a string builder
#define nob_sb_append_buf(sb, buf, size) nob_da_append_many(sb, buf, size)

// Append a NULL-terminated string to a string builder
#define nob_sb_append_cstr(sb, cstr)	\
	do {								\
		const char *s = (cstr);			\
		size_t n = strlen(s);			\
		nob_da_append_many(sb, s, n);	\
	} while (0)

// Append a single NULL terminator `\0` at the end of a string builder
#define nob_sb_append_null(sb) nob_da_append_many(sb, "", 1)
#define nob_sb_free(sb) do { if ((sb)->items) NOB_FREE((sb)->items); (sb)->items = NULL; } while (0)

bool nob_read_entire_file(const char *path, Nob_StringBuilder *sb);



#ifdef _WIN32
typedef HANDLE Nob_Process;
#define NOB_INVALID_PROC NULL
#else
typedef int Nob_Process;
#define NOB_INVALID_PROC (-1)
#endif

typedef struct {
	Nob_Process *items;
	size_t count;
	size_t capacity;
} Nob_Procs;

// wait until the process has finished
bool nob_procs_wait(Nob_Procs procs);
bool nob_proc_wait(Nob_Process p);



// A command - the main workhorse of Nob
typedef struct {
	const char **items;
	size_t count;
	size_t capacity;
} Nob_Cmd;

// Render a string representation of a command into a string builder. Keep in mind that the string builder is not NULL-terminated by default. Use `nob_sb_append_null` if you plan to use it as a C string.
void nob_cmd_render(Nob_Cmd *cmd, Nob_StringBuilder *render);

// Append several argument to the command. The last argument must be NULL, because of C variadics. Use `nob_cmd_append` convenience macro instead.
void nob_cmd_append_null(Nob_Cmd *cmd, ...);

// Wrapper around `nob_cmd_append_null` that does not require NULL at the end.
#define nob_cmd_append(cmd, ...) nob_cmd_append_null(cmd, __VA_ARGS__, NULL)

// Nob_Cmd nob_cmd_inline_null(void *first, ...);
// #define nob_cmd_inline(...) nob_cmd_inline_null(NULL, __VA_ARGS__, NULL)
// #define NOB_CMD(...) nob_cmd_run_sync(nob_cmd_inline(__VA_ARGS__))

// Free all memory allocated by command arguments
#define nob_cmd_free(cmd) do { if ((cmd)->items) NOB_FREE((cmd)->items); (cmd)->items = NULL; } while (0)

Nob_Process nob_cmd_run_async(Nob_Cmd *cmd);
bool nob_cmd_run_sync(Nob_Cmd *cmd);
bool nob_cmd_run_sync_and_reset(Nob_Cmd *cmd);



# ifndef NOB_TEMP_CAPACITY
#  define NOB_TEMP_CAPACITY (8*1024)
# endif

void *nob_temp_alloc(size_t size);
char *nob_temp_strdup(const char *cstr);
char *nob_temp_sprintf(const char *format, ...);
void nob_temp_reset(void);
size_t nob_temp_save(void);
void nob_temp_rewind(size_t checkpoint);

bool nob_rename(const char *old_path, const char *new_path);
int nob_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count);
int nob_needs_rebuild1(const char *output_path, const char *input_path);
int nob_file_exists(const char *file_path);




#ifndef NOB_REBUILD_URSELF
#	if _WIN32
#		if defined(__GNUC__)
#			define NOB_REBUILD_URSELF(binary_path, source_path) "gcc", "-o", binary_path, source_path
#		elif defined(__clang__)
#			define NOB_REBUILD_URSELF(binary_path, source_path) "clang", "-o", binary_path, source_path
#		elif defined(_MSC_VER)
#			define NOB_REBUILD_URSELF(binary_path, source_path) "c1.exe", source_path
#		endif
#	else
#		define NOB_REBUILD_URSELF(binary_path, source_path) "gcc", "-o", binary_path, source_path
#	endif
#endif

// Go Rebuild Urself™ Technology
//
//   How to use it:
//     int main(int argc, char** argv) {
//         GO_REBUILD_URSELF(argc, argv);
//         // actual work
//         return 0;
//     }
//
//   After your added this macro every time you run ./nobuild it will
//   detect that you modified its original source code and will try to
//   rebuild itself before doing any actual work. So you only need to
//   bootstrap your build system once.
//
//   The modification is detected by comparing the last modified times
//   of the executable and its source code. The same way the make utility
//   usually does it.
//
//   The rebuilding is done by using the REBUILD_URSELF macro which you
//   can redefineif you need a special way of bootstraping your build system.
#define NOB_GO_REBUILD_URSELF(argc, argv)										\
	do {																		\
		const char *source_path = __FILE__;										\
		NOB_ASSERT(argc >= 1);													\
		const char *binary_path = argv[0];										\
																				\
		int rebuild_is_needed = nob_needs_rebuild1(binary_path, source_path);	\
		if (rebuild_is_needed < 0) exit(1);										\
		else if (rebuild_is_needed) {											\
			Nob_StringBuilder sb = {0};											\
			nob_sb_append_cstr(&sb, binary_path);								\
			nob_sb_append_cstr(&sb, ".old");									\
			nob_sb_append_null(&sb);											\
																				\
			if (!nob_rename(binary_path, sb.items)) exit(1);					\
			Nob_Cmd rebuild = {0};												\
			nob_cmd_append(&rebuild, NOB_REBUILD_URSELF(binary_path, source_path));	\
			bool rebuild_succeeded = nob_cmd_run_sync(&rebuild);				\
			nob_cmd_free(&rebuild);												\
			if (!rebuild_succeeded) {											\
				nob_rename(sb.items, binary_path);								\
				exit(1);														\
			}																	\
																				\
			Nob_Cmd cmd = {0};													\
			nob_da_append_many(&cmd, argv, argc);								\
			if (!nob_cmd_run_sync(&cmd)) exit(1);								\
			nob_cmd_free(&cmd);													\
			exit(0);															\
		}																		\
	} while (0)



typedef struct {
	const char *items;
	size_t count;
} Nob_StringView;

// USAGE:
//    Nob_StringView name = ...;
//    printf("Name: " SV_Fmt "\n", SV_Arg(name));
#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int) (sv).count, (sv).items

Nob_StringView nob_sv_chop_by_delim(Nob_StringView *sv, char delim);
Nob_StringView nob_sv_rchop_by_delim(Nob_StringView *sv, char delim);
Nob_StringView nob_sv_trim_left(Nob_StringView sv);
Nob_StringView nob_sv_trim_right(Nob_StringView sv);
Nob_StringView nob_sv_trim(Nob_StringView sv);
bool nob_sv_eq(Nob_StringView a, Nob_StringView b);
Nob_StringView nob_sv_from_cstr(const char *cstr);
Nob_StringView nob_sv_from_parts(const char *data, size_t count);
bool nob_sv_starts_with(Nob_StringView sv, Nob_StringView expected_prefix);
bool nob_sv_ends_with(Nob_StringView sv, Nob_StringView expected_prefix);
const char *nob_temp_sv_to_cstr(Nob_StringView sv);
Nob_StringView nob_sb_to_sv(Nob_StringBuilder sb);

char *nob_strstr_back(const char *haystack, const char *needle);



#ifndef _WIN32
#include <dirent.h>
#else
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#undef WIN32_LEAN_AND_MEAN

struct dirent {
	char d_name[MAX_PATH+1];
};

typedef struct DIR DIR;

DIR *opendir(const char *dirpath);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
#endif

#endif



#ifdef NOB_IMPLEMENTATION
#undef NOB_IMPLEMENTATION

void nob_log(Nob_LogLevel level, const char *fmt, ...) {
#if NOB_COLOR_LOG && !defined(_WIN32)
	switch (level) {
	case NOB_NONE: break;
	case NOB_INFO: fprintf(stderr, "\e[1;32m[INFO] \e[0m"); break;
	case NOB_WARNING: fprintf(stderr, "\e[1;33m[WARNING] \e[0m"); break;
	case NOB_ERROR: fprintf(stderr, "\e[1;91m[ERROR] \e[0m"); break;
	case NOB_DEBUG: fprintf(stderr, "[DEBUG] "); break;
	default: NOB_ASSERT(0 && "Unreachable");
	}
#else
	switch (level) {
	case NOB_NONE: break;
	case NOB_INFO: fprintf(stderr, "[INFO] "); break;
	case NOB_WARNING: fprintf(stderr, "[WARNING] "); break;
	case NOB_ERROR: fprintf(stderr, "[ERROR] "); break;
	case NOB_DEBUG: fprintf(stderr, "[DEBUG] "); break;
	default: NOB_ASSERT(0 && "Unreachable");
	}
#endif

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, NOB_LINE_END);
}

char *nob_shift_args(int *argc, char ***argv) {
	NOB_ASSERT(*argc > 0);
	char *result = **argv;
	(*argv)++;
	(*argc)--;
	return result;
}



bool nob_mkdir_if_not_exists(const char *path) {
#ifdef _WIN32
	int result = mkdir(path);
#else
	int result = mkdir(path, 0755);
#endif
	if (result < 0) {
		if (errno == EEXIST) {
			nob_log(NOB_INFO, "Directory `%s` already exists", path);
			return true;
		}
		nob_log(NOB_ERROR, "Could not create directory `%s`: %s", path, NOB_GET_ERRNO);
		return false;
	}

	nob_log(NOB_INFO, "Created directory `%s`", path);
	return true;
}

bool nob_copy_file(const char *src_path, const char *dst_path) {
#ifdef NOB_VERBOSE
	nob_log(NOB_INFO, "Copying `%s` -> `%s`", src_path, dst_path);
#endif
#ifdef _WIN32
	if (!CopyFile(src_path, dst_path, FALSE)) {
		nob_log(NOB_ERROR, "Could not copy file: %lu", GetLastError());
		return false;
	}
	return true;
#else
	int src_fd = -1;
	int dst_fd = -1;
	size_t buf_size = 32*1024;
	char *buf = NOB_REALLOC(NULL, buf_size);
	NOB_ASSERT(buf != NULL && "Need more RAM");
	bool result = true;

	src_fd = open(src_path, O_RDONLY);
	if (src_fd < 0) {
		nob_log(NOB_ERROR, "Could not open file `%s`: %s", src_path, NOB_GET_ERRNO);
		nob_return_defer(false);
	}

	struct stat src_stat;
	if (fstat(src_fd, &src_stat) < 0) {
		nob_log(NOB_ERROR, "Could not get mode of file `%s`: %s", src_path, NOB_GET_ERRNO);
		nob_return_defer(false);
	}

	dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
	if (dst_fd < 0) {
		nob_log(NOB_ERROR, "Could not create file `%s`: %s", dst_path, NOB_GET_ERRNO);
		nob_return_defer(false);
	}

	for (;;) {
		ssize_t n = read(src_fd, buf, buf_size);
		if (n == 0) break;
		if (n < 0) {
			nob_log(NOB_ERROR, "Could not read from file `%s`: %s", src_path, NOB_GET_ERRNO);
			nob_return_defer(false);
		}
		char *buf2 = buf;
		while (n > 0) {
			ssize_t m = write(dst_fd, buf2, n);
			if (m < 0) {
				nob_log(NOB_ERROR, "Could not write to file `%s`: %s", dst_path, NOB_GET_ERRNO);
				nob_return_defer(false);
			}
			n -= m;
			buf2 += m;
		}
	}
defer:
	free(buf);
	close(src_fd);
	close(dst_fd);
	return result;
#endif
}

bool nob_copy_directory_recursively(const char *src_path, const char *dst_path) {
	bool result = true;
	Nob_FilePaths children = {0};
	Nob_StringBuilder src_sb = {0};
	Nob_StringBuilder dst_sb = {0};
	size_t temp_checkpoint = nob_temp_save();

	Nob_FileType type = nob_get_file_type(src_path);
	if (type < 0) return false;

	switch (type) {
	case NOB_FILE_DIRECTORY: {
		if (!nob_mkdir_if_not_exists(dst_path)) nob_return_defer(false);
		if (!nob_read_entire_dir(src_path, &children)) nob_return_defer(false);

		for (size_t i = 0UL; i < children.count; i++) {
			if (strcmp(children.items[i], ".") == 0) continue;
			if (strcmp(children.items[i], "..") == 0) continue;

			src_sb.count = 0;
			nob_sb_append_cstr(&src_sb, src_path);
			nob_sb_append_cstr(&src_sb, "/");
			nob_sb_append_cstr(&src_sb, children.items[i]);
			nob_sb_append_null(&src_sb);

			dst_sb.count = 0;
			nob_sb_append_cstr(&dst_sb, dst_path);
			nob_sb_append_cstr(&dst_sb, "/");
			nob_sb_append_cstr(&dst_sb, children.items[i]);
			nob_sb_append_null(&dst_sb);

			if (!nob_copy_directory_recursively(src_sb.items, dst_sb.items)) {
				nob_return_defer(false);
			}
		}
	} break;

	case NOB_FILE_REGULAR: {
		if (!nob_copy_file(src_path, dst_path)) {
			nob_return_defer(false);
		}
	} break;

	case NOB_FILE_SYMLINK: {
		nob_log(NOB_WARNING, "Copying symlinks is not supported yet");
	} break;

	case NOB_FILE_OTHER: {
		nob_log(NOB_ERROR, "Unsupported type of file `%s`", src_path);
		nob_return_defer(false);
	} break;

	default: NOB_ASSERT(0 && "Unreachable");
	}

defer:
	nob_temp_rewind(temp_checkpoint);
	nob_sb_free(&src_sb);
	nob_sb_free(&dst_sb);
	nob_da_free(&children);
	return result;
}

bool nob_read_entire_dir(const char *parent, Nob_FilePaths *children) {
	bool result = true;
	DIR *dir = NULL;

	dir = opendir(parent);
	if (dir == NULL) {
		nob_log(NOB_ERROR, "Could not open directory `%s`: %s", parent, NOB_GET_ERRNO);
		nob_return_defer(false);
	}

	errno = 0;
	struct dirent *ent = readdir(dir);
	while (ent != NULL) {
		nob_da_append(children, nob_temp_strdup(ent->d_name));
		ent = readdir(dir);
	}

	if (errno != 0) {
		nob_log(NOB_ERROR, "Could not read directory `%s`: %s", parent, NOB_GET_ERRNO);
		nob_return_defer(false);
	}

defer:
	if (dir) closedir(dir);
	return result;
}

bool nob_write_entire_file(const char *path, void *data, size_t size) {
	bool result = true;

	FILE *f = fopen(path, "wb");
	if (f == NULL) {
		nob_log(NOB_ERROR, "Could not open file `%s` for writing: %s", path, NOB_GET_ERRNO);
		nob_return_defer(false);
	}
	
	char *buf = data;
	while (size > 0) {
		size_t n = fwrite(buf, 1, size, f);
		if (ferror(f)) {
			nob_log(NOB_ERROR, "Could not write into file `%s`: %s", path, NOB_GET_ERRNO);
			nob_return_defer(false);
		}
		size -= n;
		buf += n;
	}

defer:
	if (f) fclose(f);
	return result;
}

bool nob_read_entire_file(const char *path, Nob_StringBuilder *sb) {
	bool result = true;

	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		nob_log(NOB_ERROR, "Could not open `%s` for reading: %s", path, NOB_GET_ERRNO);
		return false;
	}

	if (fseek(f, 0L, SEEK_END) < 0) {
		nob_log(NOB_ERROR, "Failed to set cursor to the end of `%s`: %s", path, NOB_GET_ERRNO);
		nob_return_defer(false);
	}
	long buf_size = ftell(f);
	if (buf_size < 0) {
		nob_log(NOB_ERROR, "Failed to get cursor position in `%s`: %s", path, NOB_GET_ERRNO);
		nob_return_defer(false);
	}
	rewind(f);

	size_t new_count = sb->count + buf_size;
	if (new_count > sb->capacity) {
		sb->items = NOB_REALLOC(sb->items, new_count);
		sb->capacity = new_count;
	}



	fread(sb->items + sb->count, buf_size, 1, f);
	if (ferror(f)) {
		nob_log(NOB_ERROR, "Could not read `%s`: %s", path, NOB_GET_ERRNO);
		nob_return_defer(false);
	}
	sb->count = new_count;
	
defer:
	if (f) fclose(f);
	return result;
}

Nob_FileType nob_get_file_type(const char *path) {
#ifdef _WIN32
	DWORD attr = GetFileAttributesA(path);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		nob_log(NOB_ERROR, "Could not get file attributes of `%s`: %lu", path, GetLastError());
		return -1;
	}

	if (attr & FILE_ATTRIBUTE_DIRECTORY) return NOB_FILE_DIRECTORY;

	return NOB_FILE_REGULAR;
#else
	struct stat statbuf;
	if (stat(path, &statbuf) < 0) {
		nob_log(NOB_ERROR, "Could not get stat of `%s`: %s", path, NOB_GET_ERRNO);
		return -1;
	}

	switch (statbuf.st_mode & S_IFMT) {
	case S_IFDIR:	return NOB_FILE_DIRECTORY;
	case S_IFREG:	return NOB_FILE_REGULAR;
	case S_IFLNK:	return NOB_FILE_SYMLINK;
	default:		return NOB_FILE_OTHER;
	}
#endif
}




void nob_cmd_render(Nob_Cmd *cmd, Nob_StringBuilder *render) {
	for (size_t i = 0; i < cmd->count; i++) {
		const char *arg = cmd->items[i];
		if (arg == NULL) break;
		if (i > 0) nob_sb_append_cstr(render, " ");
		if (!strchr(arg, ' ')) {
			nob_sb_append_cstr(render, arg);
		} else {
			nob_da_append(render, '\'');
			nob_sb_append_cstr(render, arg);
			nob_da_append(render, '\'');
		}
	}
}

void nob_cmd_append_null(Nob_Cmd *cmd, ...) {
	va_list args;
	va_start(args, cmd);

	const char *arg = va_arg(args, const char *);
	while (arg != NULL) {
		nob_da_append(cmd, arg);
		arg = va_arg(args, const char *);
	}

	va_end(args);
}



bool nob_proc_wait(Nob_Process p) {
	if (p == NOB_INVALID_PROC) return false;

#ifdef _WIN32
	DWORD result = WaitForSingleObject(p, INFINITE);
	if (result == WAIT_FAILED) {
		nob_log(NOB_ERROR, "Could not wait on child process: %lu", GetLastError());
		return false;
	}

	DWORD exit_status;
	if (!GetExitCodeProcess(p, &exit_status)) {
		nob_log(NOB_ERROR, "Could not get process exit code: %lu", GetLastError());
		return false;
	}

	if (exit_status != 0) {
		nob_log(NOB_ERROR, "Command exited with exit code %lu", exit_status);
		return false;
	}

	CloseHandle(p);
	return true;
#else
	for (;;) {
		int wstatus = 0;
		if (waitpid(p, &wstatus, 0) < 0) {
			nob_log(NOB_ERROR, "Could not wait on command (pid %d): %s", p, NOB_GET_ERRNO);
			return false;
		}

		if (WIFEXITED(wstatus)) {
			int exit_status = WEXITSTATUS(wstatus);
			if (exit_status != 0) {
				nob_log(NOB_ERROR, "Command exited with exit code %d", exit_status);
				return false;
			}

			break;
		}

		if (WIFSIGNALED(wstatus)) {
			nob_log(NOB_ERROR, "Command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
			return false;
		}
	}

	return true;
#endif	
}

bool nob_procs_wait(Nob_Procs procs) {
	bool success = true;
	for (size_t i = 0UL; i < procs.count; i++) {
		success = nob_proc_wait(procs.items[i]) && success;
	}
	return success;
}

Nob_Process nob_cmd_run_async(Nob_Cmd *cmd) {
	if (cmd->count < 1) {
		nob_log(NOB_ERROR, "Could not run empty command");
		return NOB_INVALID_PROC;
	}

	Nob_StringBuilder sb = {0};
	nob_cmd_render(cmd, &sb);
	nob_sb_append_null(&sb);
	nob_log(NOB_INFO, "CMD: %s", sb.items);
	nob_sb_free(&sb);

#ifdef _WIN32
	STARTUPINFO siStartInfo;
	ZeroMemory(&siStartInfo, sizeof(siStartInfo));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	nob_cmd_render(cmd, &sb);
	nob_sb_append_null(&sb);
	BOOL bSuccess = CreateProcessA(NULL, sb.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
	nob_sb_free(&sb);

	if (!bSuccess) {
		nob_log(NOB_ERROR, "Could not create child process: %lu", GetLastError());
		return NOB_INVALID_PROC;
	}

	CloseHandle(piProcInfo.hThread);

	return piProcInfo.hProcess;
#else
	pid_t cpid = fork();
	if (cpid < 0) {
		nob_log(NOB_ERROR, "Could not fork child process: %s", NOB_GET_ERRNO);
		return NOB_INVALID_PROC;
	}

	if (cpid == 0) {
		// successful EXEC does not return anything and replaces process image; otherwise returns -1 on error, with setting ERRNO
		Nob_Cmd cmd_null = {0};
		nob_da_append_many(&cmd_null, cmd->items, cmd->count);
		nob_cmd_append(&cmd_null, NULL);

		if (execvp(cmd->items[0], (char *const *) cmd_null.items) == -1) {
			nob_log(NOB_ERROR, "Could not exec child process: %s", NOB_GET_ERRNO);
			nob_cmd_free(&cmd_null);
			exit(1);
		}
		NOB_ASSERT(0 && "unreachable");
	}

	return cpid;
#endif
}

Nob_Process nob_cmd_run_async_and_reset(Nob_Cmd *cmd) {
	Nob_Process p = nob_cmd_run_async(cmd);
	cmd->count = 0UL;
	return p;
}

bool nob_cmd_run_sync(Nob_Cmd *cmd) {
	Nob_Process p = nob_cmd_run_async(cmd);
	if (p == NOB_INVALID_PROC) return false;
	return nob_proc_wait(p);
}

bool nob_cmd_run_sync_and_reset(Nob_Cmd *cmd) {
	bool res = nob_cmd_run_sync(cmd);
	cmd->count = 0UL;
	return res;
}



static size_t nob_temp_size = 0;
static char nob_temp[NOB_TEMP_CAPACITY] = {0};



void *nob_temp_alloc(size_t size) {
	if (nob_temp_size + size > NOB_TEMP_CAPACITY) return NULL;
	void *result = &nob_temp[nob_temp_size];
	nob_temp_size += size;
	return result;
}

char *nob_temp_strdup(const char *cstr) {
	size_t n = strlen(cstr);
	char *result = nob_temp_alloc(n + 1);
	NOB_ASSERT(result != NULL && "Increase NOB_TEMP_CAPACITY");
	memcpy(result, cstr, n);
	result[n] = '\0';
	return result;
}

char *nob_temp_sprintf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	int n = vsnprintf(NULL, 0, format, args);
	va_end(args);

	NOB_ASSERT(n >= 0);
	char *result = nob_temp_alloc(n + 1);
	NOB_ASSERT(result != NULL && "Extend the size of temporary allocator");
	va_start(args, format);
	vsnprintf(result, n + 1, format, args);
	va_end(args);

	return result;
}

void nob_temp_reset(void) { nob_temp_size = 0; }
size_t nob_temp_save(void) { return nob_temp_size; }
void nob_temp_rewind(size_t checkpoint) { nob_temp_size = checkpoint; }

const char *nob_temp_sv_to_cstr(Nob_StringView sv) {
	char *result = nob_temp_alloc(sv.count + 1);
	NOB_ASSERT(result != NULL && "Extend the size of the temporary allocator");
	memcpy(result, sv.items, sv.count);
	result[sv.count] = '\0';
	return result;
}

Nob_StringView nob_sb_to_sv(Nob_StringBuilder sb) {
	return (Nob_StringView) { .items = sb.items, .count = sb.count };
}



bool nob_rename(const char *old_path, const char *new_path) {
	nob_log(NOB_INFO, "Renaming `%s` -> `%s`", old_path, new_path);
#ifdef _WIN32
	if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
		nob_log(NOB_ERROR, "Could not rename `%s` to `%s`: %lu", old_path, new_path, GetLastError());
		return false;
	}
#else
	if (rename(old_path, new_path) < 0) {
		nob_log(NOB_ERROR, "Could not rename `%s` to `%s`: %s", old_path, new_path, NOB_GET_ERRNO);
		return false;
	}
#endif
	return true;
}

// check if 
// \returns
// `0` - no need for rebuild, INPUTs were not updated,
// `1` - INPUTs have been updated, OUTPUT does need a rebuild,
// `-1` - an error occured
int nob_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count) {
#ifdef _WIN32
	BOOL bSuccess;

	HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (output_path_fd == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1;
		nob_log(NOB_ERROR, "Could not open file `%s`: %lu", output_path, GetLastError());
		return -1;
	}

	FILETIME output_path_time;
	bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
	CloseHandle(output_path_fd);
	if (!bSuccess) {
		nob_log(NOB_ERROR, "Could not get time of `%s`: %lu", output_path, GetLastError());
		return -1;
	}

	for (size_t i = 0UL; i < input_paths_count; i++) {
		const char *input_path = input_paths[i];
		HANDLE input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (input_path_fd == INVALID_HANDLE_VALUE) {
			nob_log(NOB_ERROR, "Could not open file `%s`: %lu", input_path, GetLastError());
			return -1;
		}

		FILETIME input_path_time;
		bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
		CloseHandle(input_path_fd);
		if (!bSuccess) {
			nob_log(NOB_ERROR, "Could not get time of `%s`: %lu", input_path, GetLastError());
			return -1;
		}

		if (CompareFileTime(&input_path_time, &output_path_time) == 1) return 1;
	}

	return 0;
#else
	struct stat statbuf = {0};

	if (stat(output_path, &statbuf) < 0) {
		if (errno == ENOENT) return 1;
		nob_log(NOB_ERROR, "Could not stat `%s`: %s", output_path, NOB_GET_ERRNO);
		return -1;
	}
#ifdef __APPLE__
	int output_path_time = statbuf.st_mtimespec.tv_sec;
#else
	int output_path_time = statbuf.st_mtime;
#endif

	for (size_t i = 0UL; i < input_paths_count; i++) {
		const char *input_path = input_paths[i];
		if (stat(input_path, &statbuf) < 0) {
			nob_log(NOB_ERROR, "Could not stat `%s`: %s", input_path, NOB_GET_ERRNO);
			return -1;
		}
#ifdef __APPLE__
		int input_path_time = statbuf.st_mtimespec.tv_sec;
#else
		int input_path_time = statbuf.st_mtime;
#endif
		if (input_path_time > output_path_time) return 1;
	}

	return 0;
#endif
}

int nob_needs_rebuild1(const char *output_path, const char *input_path) { return nob_needs_rebuild(output_path, &input_path, 1); }

// \returns
// `0` - file does not exist,
// `1` - file exists,
// `-1` - error while checking (logged)
int nob_file_exists(const char *file_path) {
#if _WIN32
	DWORD dwAttrib = GetFileAttributesA(file_path);
	return dwAttrib != INVALID_FILE_ATTRIBUTES;
#else
	struct stat statbuf;
	if (stat(file_path, &statbuf) < 0) {
		if (errno == ENOENT) return 0;
		nob_log(NOB_ERROR, "Could not check if file `%s` exists: %s", file_path, NOB_GET_ERRNO);
		return -1;
	}
	return 1;
#endif
}



Nob_StringView nob_sv_chop_by_delim(Nob_StringView *sv, char delim) {
	size_t i = 0UL;
	while (i < sv->count && sv->items[i] != delim) i++;

	Nob_StringView result = nob_sv_from_parts(sv->items, i);

	if (i < sv->count) {
		sv->count -= i + 1;
		sv->items += i + 1;
	} else {
		sv->count -= i;
		sv->items += i;
	}

	return result;
}

Nob_StringView nob_sv_rchop_by_delim(Nob_StringView *sv, char delim) {
	size_t i = 0UL;
	while (i < sv->count && sv->items[sv->count - 1UL - i] != delim) i++;

	Nob_StringView result = nob_sv_from_parts(&sv->items[sv->count - i], i);

	if (i < sv->count) sv->count -= i + 1;
	else sv->count -= i;

	return result;
}

Nob_StringView nob_sv_trim_left(Nob_StringView sv) {
	size_t i = 0UL;
	while (i < sv.count && isspace(sv.items[i])) i++;
	return nob_sv_from_parts(sv.items + i, sv.count - i);
}

Nob_StringView nob_sv_trim_right(Nob_StringView sv) {
	size_t i = 0UL;
	while (i < sv.count && isspace(sv.items[sv.count - 1 - i])) i++;
	return nob_sv_from_parts(sv.items, sv.count - i);
}

Nob_StringView nob_sv_trim(Nob_StringView sv) { return nob_sv_trim_right(nob_sv_trim_left(sv)); }

bool nob_sv_eq(Nob_StringView a, Nob_StringView b) {
	if (a.count != b.count) return false;
	else return memcmp(a.items, b.items, a.count) == 0;
}

Nob_StringView nob_sv_from_cstr(const char *cstr) { return nob_sv_from_parts(cstr, strlen(cstr)); }

Nob_StringView nob_sv_from_parts(const char *data, size_t count) {
	Nob_StringView sv;
	sv.count = count;
	sv.items = data;
	return sv;
}

bool nob_sv_starts_with(Nob_StringView sv, Nob_StringView expected_prefix) {
    if (expected_prefix.count <= sv.count) {
        Nob_StringView actual_prefix = nob_sv_from_parts(sv.items, expected_prefix.count);
        return nob_sv_eq(expected_prefix, actual_prefix);
    }

    return false;
}

bool nob_sv_ends_with(Nob_StringView sv, Nob_StringView expected_suffix) {
    if (expected_suffix.count <= sv.count) {
        Nob_StringView actual_suffix = nob_sv_from_parts(sv.items + sv.count - expected_suffix.count, expected_suffix.count);
        return nob_sv_eq(expected_suffix, actual_suffix);
    }

    return false;
}



char *nob_strstr_back(const char *haystack, const char *needle) {
	if (!haystack || !needle || !*haystack || !*needle) return (char *) haystack;

	int i;
	size_t needle_len = strlen(needle);
	const char *haystack_end = haystack + strlen(haystack) - 1UL;
	needle = needle + needle_len - 1UL;

	for (; haystack != haystack_end; haystack_end--) {
		if (*haystack_end != *needle) continue;
		for (i = (int) needle_len - 1; i >= 0; i--) {
			if (haystack_end[-i] != needle[-i]) break;
			if (!i) {
				i = (int) needle_len - 1;
				return (char *) &haystack_end[-i];
			}
		}
	}
	return NULL;
}



#ifdef _WIN32
struct DIR {
	HANDLE hFind;
	WIN32_FIND_DATA data;
	struct dirent *dirent;
};

DIR *opendir(const char *dirpath) {
	NOB_ASSERT(dirpath);

	char buffer[MAX_PATH];
	snprintf(buffer, MAX_PATH, "%s\\*", dirpath);

	DIR *dir = (DIR *) calloc(1, sizeof(DIR));

	dir->hFind = FindFirstFile(buffer, &dir->data);
	if (dir->hFind == INVALID_HANDLE_VALUE) {
		errno = ENOSYS;
		goto fail;
	}

	return dir;

fail:
	if (dir) free(dir);
	return NULL;
}

struct dirent *readdir(DIR *dirp) {
	NOB_ASSERT(dirp);

	if (dirp->dirent == NULL) {
		dirp->dirent = (struct dirent *) calloc(1, sizeof(struct dirent));
	} else {
		if (!FindNextFile(dirp->hFind, &dirp->data)) {
			if (GetLastError() != ERROR_NO_MORE_FILES) {
				errno = ENOSYS;
			}

			return NULL;
		}
	}

	memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

	strncpy(dirp->dirent->d_name, dirp->data.cFileName, sizeof(dirp->dirent->d_name) - 1);

	return dirp->dirent;
}

int closedir(DIR *dirp) {
	NOB_ASSERT(dirp);

	if (!FindClose(dirp->hFind)) {
		errno = ENOSYS;
		return -1;
	}

	if (dirp->dirent) free(dirp->dirent);
	free(dirp);
	return 0;
}

#endif		// minirent.h for _WIN32

#endif // NOB_IMPLEMENTATION