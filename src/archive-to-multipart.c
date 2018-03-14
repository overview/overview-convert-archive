#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const static size_t ARR_SIZE = 20; // for tiny buffers

void
perr(int is_error)
{
	if (is_error) {
		perror("Error");
		exit(1);
	}
}

void
perrmsg(int is_error, const char* message)
{
	if (is_error) {
		perror(message);
		exit(1);
	}
}

void
easywrite(FILE* f, const char* message) {
	perr(!fwrite(message, 1, strlen(message), f));
}

void
print_boundary(FILE* f, const char* boundary)
{
	easywrite(f, "--");
	easywrite(f, boundary);
}

void
print_file_start(FILE* f, const char* boundary, const char* filename)
{
	print_boundary(f, boundary);
	easywrite(f, "Content-Disposition: form-data; name=\"");
	easywrite(f, filename);
	easywrite(f, "\"\r\n");
}

void
print_file_end(FILE* f)
{
	easywrite(f, "\r\n");
}

void
print_epilogue(FILE* f, const char* boundary)
{
	print_boundary(f, boundary);
	easywrite(f, "--");
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
print_archive_entry_json(FILE* f, const char* json_template, const char* filename)
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
	easywrite(f, json);
	free(json);
	free(json_filename);
}

void
print_archive_entry(FILE* f, int index_in_parent, const char* json_template, const char* boundary, struct archive* archive, struct archive_entry* entry)
{
	char arr[ARR_SIZE];

	// "0.json": metadata
	snprintf(&arr[0], ARR_SIZE, "%d.json", index_in_parent);
	print_file_start(f, boundary, &arr[0]);
	const char* filename = archive_entry_pathname(entry);
	print_archive_entry_json(f, json_template, filename);
	print_file_end(f);

	// "0.blob": contents
	snprintf(&arr[0], ARR_SIZE, "%d.blob", index_in_parent);
	print_file_start(f, boundary, &arr[0]);
	for (;;) {
		const void* buf;
		size_t len;
		la_int64_t offset;

		int r = archive_read_data_block(archive, &buf, &len, &offset);
		if (r == ARCHIVE_EOF) break;
		if (r != ARCHIVE_OK) {
			fprintf(stderr, "Error reading from archive: %s\n", archive_error_string(archive));
			exit(1);
		}

		perrmsg(len != fwrite(buf + offset, 1, len, f), "Error writing extracted contents");
	}
	print_file_end(f);
}

void
print_progress(FILE* f, const char* boundary, size_t n_bytes_processed, size_t n_bytes_total)
{
	char arr[ARR_SIZE];

	// "progress"
	print_file_start(f, boundary, "progress");
	easywrite(f, "{\"bytes\":{\"nProcessed\":\"");
	snprintf(arr, ARR_SIZE, "%d", n_bytes_processed);
	easywrite(f, arr);
	easywrite(f, "\",\"nTotal\":\"");
	snprintf(arr, ARR_SIZE, "%d", n_bytes_total);
	easywrite(f, arr);
	easywrite(f, "\"}}");
	print_file_end(f);
}

size_t
filename_to_n_bytes(const char* filename)
{
	struct stat statbuf;
	perrmsg(stat(filename, &statbuf), "Error determining input file size");

}

void
print_archive_contents(FILE* f, const char* filename, const char* json_template, const char* boundary)
{
	struct archive* a;
	struct archive_entry* entry;
	int index_in_parent = 0;

	a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);

	if (ARCHIVE_OK != archive_read_open_filename(a, filename, 10240)) {
		fprintf(stderr, "Error opening %s: %s\n", filename, archive_error_string(a));
		exit(1);
	}

	size_t n_bytes_total = filename_to_n_bytes(filename);

	while (ARCHIVE_OK == archive_read_next_header(a, &entry)) {
		print_archive_entry(
			f,
			index_in_parent,
			json_template,
			boundary,
			a,
			entry
		);

		print_progress(f, boundary, n_bytes_total, archive_filter_bytes(a, -1));

		index_in_parent += 1;
	}

	if (ARCHIVE_OK != archive_read_free(a)) {
		fprintf(stderr, "Error closing %s: %s\n", filename, archive_error_string(a));
		exit(1);
	}

	print_file_start(f, boundary, "done");
	print_file_end(f);
	print_epilogue(f, boundary);
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

	print_archive_contents(stdout, filename, json_template, boundary);
}
