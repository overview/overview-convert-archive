#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const static size_t ARR_SIZE = 20; // for tiny buffers

void
perrmsg(int is_error, const char* message)
{
	if (is_error) {
		perror(message);
		exit(1);
	}
}

void
easywrite_len(int fd, const void* buf, size_t len)
{
	while (len > 0) {
		size_t n = write(fd, buf, len);
		if (n == -1) {
			perror("Error writing multipart data");
			exit(1);
		}
		buf += n;
		len -= n;
	}
}

void
easywrite(int fd, const char* message)
{
	easywrite_len(fd, message, strlen(message));
}

void
print_boundary(int fd, const char* boundary)
{
	easywrite(fd, "--");
	easywrite(fd, boundary);
}

void
print_file_start(int fd, const char* boundary, const char* filename)
{
	print_boundary(fd, boundary);
	easywrite(fd, "\r\nContent-Disposition: form-data; name=\"");
	easywrite(fd, filename);
	easywrite(fd, "\"\r\n\r\n");
}

void
print_file_end(int fd)
{
	easywrite(fd, "\r\n");
}

void
print_epilogue(int fd, const char* boundary)
{
	print_boundary(fd, boundary);
	easywrite(fd, "--");
}

void
print_error_and_exit(int fd, const char* boundary, const char* message)
{
	print_file_start(fd, boundary, "error");
	easywrite(fd, message);
	print_file_end(fd);
	print_epilogue(fd, boundary);
	exit(0); // SUCCESS: we successfully detected an error in the file
}

/**
 * Returns s, with unsafe characters escaped.
 *
 * The longest possible output is 6x the size of the input string; and that's
 * the size of the string we allocate.
 */
char*
json_escape(const char* s)
{
	char* out = calloc(1, 6 * strlen(s) + 1);
	if (out == NULL) return NULL;

	for (char* o = out; *s != '\0'; s++) {
		switch (*s) {
			case '\x00': strcpy(o, "\\u0000"); o += 6; break;
			case '\x01': strcpy(o, "\\u0001"); o += 6; break;
			case '\x02': strcpy(o, "\\u0002"); o += 6; break;
			case '\x03': strcpy(o, "\\u0003"); o += 6; break;
			case '\x04': strcpy(o, "\\u0004"); o += 6; break;
			case '\x05': strcpy(o, "\\u0005"); o += 6; break;
			case '\x06': strcpy(o, "\\u0006"); o += 6; break;
			case '\x07': strcpy(o, "\\u0007"); o += 6; break;
			case '\x08': strcpy(o, "\\u0008"); o += 6; break;
			case '\x09': strcpy(o, "\\u0009"); o += 6; break;
			case '\x0a': strcpy(o, "\\u000a"); o += 6; break;
			case '\x0b': strcpy(o, "\\u000b"); o += 6; break;
			case '\x0c': strcpy(o, "\\u000c"); o += 6; break;
			case '\x0d': strcpy(o, "\\u000d"); o += 6; break;
			case '\x0e': strcpy(o, "\\u000e"); o += 6; break;
			case '\x0f': strcpy(o, "\\u000f"); o += 6; break;
			case '\x10': strcpy(o, "\\u0010"); o += 6; break;
			case '\x11': strcpy(o, "\\u0011"); o += 6; break;
			case '\x12': strcpy(o, "\\u0012"); o += 6; break;
			case '\x13': strcpy(o, "\\u0013"); o += 6; break;
			case '\x14': strcpy(o, "\\u0014"); o += 6; break;
			case '\x15': strcpy(o, "\\u0015"); o += 6; break;
			case '\x16': strcpy(o, "\\u0016"); o += 6; break;
			case '\x17': strcpy(o, "\\u0017"); o += 6; break;
			case '\x18': strcpy(o, "\\u0018"); o += 6; break;
			case '\x19': strcpy(o, "\\u0019"); o += 6; break;
			case '\x1a': strcpy(o, "\\u001a"); o += 6; break;
			case '\x1b': strcpy(o, "\\u001b"); o += 6; break;
			case '\x1c': strcpy(o, "\\u001c"); o += 6; break;
			case '\x1d': strcpy(o, "\\u001d"); o += 6; break;
			case '\x1e': strcpy(o, "\\u001e"); o += 6; break;
			case '\x1f': strcpy(o, "\\u001f"); o += 6; break;
			case '"': strcpy(o, "\\\""); o += 2; break;
			case '\\': strcpy(o, "\\\\"); o += 2; break;
			default: *o = *s; o += 1;
		}
	}

	return out;
}

void
print_archive_entry_json(int fd, const char* json_template, const char* filename)
{
	// JSON, with "FILENAME" replaced by the actual filename
	const char* json_filename_pos = strstr(json_template, "FILENAME\"");
	if (!json_filename_pos) {
		fprintf(stderr, "Expected placeholder 'FILENAME' to exist in JSON template; got %s", json_template);
		exit(1);
	}

	char* json_filename = json_escape(filename);
	perrmsg(!json_filename, "Could not allocate json_escape output");
	char* json = calloc(1, strlen(json_template) - strlen("FILENAME") + strlen(json_filename) + 1);
	perrmsg(!json, "Could not allocate json output");
	memcpy(json, json_template, (json_filename_pos - json_template));
	memcpy(json + (json_filename_pos - json_template), json_filename, strlen(json_filename));
	memcpy(
		json + (json_filename_pos - json_template) + strlen(json_filename),
		json_filename_pos + 8,
		strlen(json_filename_pos + 8)
	);
	easywrite(fd, json);
	free(json);
	free(json_filename);
}

void
print_archive_entry(int fd, int index_in_parent, const char* json_template, const char* boundary, struct archive* archive, struct archive_entry* entry)
{
	char arr[ARR_SIZE];

	// "0.json": metadata
	snprintf(&arr[0], ARR_SIZE, "%d.json", index_in_parent);
	print_file_start(fd, boundary, &arr[0]);
	const char* filename = archive_entry_pathname(entry);
	print_archive_entry_json(fd, json_template, filename);
	print_file_end(fd);

	// "0.blob": contents
	snprintf(&arr[0], ARR_SIZE, "%d.blob", index_in_parent);
	print_file_start(fd, boundary, &arr[0]);
	int r = archive_read_data_into_fd(archive, fd);
	if (r != ARCHIVE_OK) {
		fprintf(stderr, "Error reading from archive: %s\n", archive_error_string(archive));
		print_file_end(fd);
		print_error_and_exit(fd, boundary, archive_error_string(archive));
	}
	print_file_end(fd);
}

void
print_progress(int fd, const char* boundary, size_t n_bytes_processed, size_t n_bytes_total)
{
	char arr[ARR_SIZE];

	// "progress"
	print_file_start(fd, boundary, "progress");
	easywrite(fd, "{\"bytes\":{\"nProcessed\":");
	snprintf(arr, ARR_SIZE, "%zu", n_bytes_processed);
	easywrite(fd, arr);
	easywrite(fd, ",\"nTotal\":");
	snprintf(arr, ARR_SIZE, "%zu", n_bytes_total);
	easywrite(fd, arr);
	easywrite(fd, "}}");
	print_file_end(fd);
}

size_t
filename_to_n_bytes(const char* filename)
{
	struct stat statbuf;
	perrmsg(stat(filename, &statbuf), "Error determining input file size");
	return statbuf.st_size;
}

void
print_archive_contents(int fd, const char* filename, const char* json_template, const char* boundary)
{
	struct archive* a;
	struct archive_entry* entry;
	int index_in_parent = 0;

	a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);

	if (ARCHIVE_OK != archive_read_open_filename(a, filename, 10240)) {
		fprintf(stderr, "Error opening %s: %s\n", filename, archive_error_string(a));
		print_error_and_exit(fd, boundary, archive_error_string(a));
	}

	size_t n_bytes_total = filename_to_n_bytes(filename);

	while (ARCHIVE_OK == archive_read_next_header(a, &entry)) {
		switch (archive_entry_filetype(entry)) {
		case AE_IFREG:
			print_archive_entry(
				fd,
				index_in_parent,
				json_template,
				boundary,
				a,
				entry
			);

			print_progress(fd, boundary, archive_filter_bytes(a, -1), n_bytes_total);

			index_in_parent += 1;
			break;
		case AE_IFLNK:  // symlink or hardlink: no data to extract
		case AE_IFSOCK: // socket: no data to extract
		case AE_IFCHR:  // special file: no data to extract
		case AE_IFBLK:  // block device: no data to extract
		case AE_IFDIR:  // directory: not a document (we'll see its contents later)
		case AE_IFIFO:  // pipe: no data to extract
		default:
			break;
		}
	}

	if (ARCHIVE_OK != archive_read_free(a)) {
		fprintf(stderr, "Error closing %s: %s\n", filename, archive_error_string(a));
		print_error_and_exit(fd, boundary, archive_error_string(a));
	}

	print_file_start(fd, boundary, "done");
	print_file_end(fd);
	print_epilogue(fd, boundary);
}

int
main(int argc, char** argv)
{
	if (argc != 4) {
		fprintf(stderr, "Usage: %s INPUT-FILE JSON-TEMPLATE BOUNDARY\n", argv[0]);
		exit(1);
	}

	const char* filename = argv[1];
	const char* json_template = argv[2];
	const char* boundary = argv[3];

	print_archive_contents(fileno(stdout), filename, json_template, boundary);
}
