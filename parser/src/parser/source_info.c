#include "source_info.h"

LineInfo line_info_from_source(Arena* allocator, String source) {
	LineInfo line_info = {};
	line_info.line_starts = arena_alloc_array(allocator, uint32_t, 0);
	line_info.line_count = 0;

	size_t line_start = 0;
	size_t read_pos = 0;
	while (read_pos < source.length) {
		if (source.v[read_pos] == '\n') {
			arena_alloc(allocator, uint32_t);
			line_info.line_starts[line_info.line_count] = (uint32_t)line_start;
			line_info.line_count += 1;

			line_start = read_pos + 1;
		}

		read_pos += 1;
	}

	arena_alloc(allocator, uint32_t);
	line_info.line_starts[line_info.line_count] = (uint32_t)line_start;
	line_info.source_length = source.length;

	return line_info;
}

SourceLocation line_info_pos_to_source_location(const LineInfo* line_info, size_t string_pos) {
	assert(string_pos <= line_info->source_length);

	uint32_t left = 0;
	uint32_t right = line_info->line_count + 1;

	while (right - left > 1) {
		uint32_t mid = (right + left) / 2;
		if (line_info->line_starts[mid] > string_pos) {
			right = mid;
		} else {
			left = mid;
		}
	}

	uint32_t line_index = left;
	SourceLocation location = {};
	location.line = line_index;
	location.column = (uint32_t)string_pos - line_info->line_starts[line_index];
	return location;
}

//
// SourceStorage
//

void source_storage_init(SourceStorage* storage, StringArray include_dirs, Arena* allocator) {
	assert(allocator);

	storage->allocator = allocator;
	storage->count = 0;
	storage->capacity = 256;
	storage->files = arena_alloc_array(storage->allocator, SourceFile, storage->capacity);
	storage->include_dirs = include_dirs;
}

String source_storage_resolve_include_path(const SourceStorage* storage,
		String include_path,
		const SourceFile* current_file,
		Arena* allocator,
		Arena* temp_allocator) {

	// First check for relative includes

	{
		ArenaRegion temp = arena_begin_temp(temp_allocator);

		StringBuilder builder = { .arena = temp_allocator };
		str_builder_append(&builder, path_get_parent(current_file->path));
		str_builder_append_char(&builder, '/');
		str_builder_append(&builder, include_path);

		bool exists = path_exists(temp_allocator, builder.string);
		String result = {};
		if (exists) {
			result = path_canonicalize(builder.string, allocator, temp_allocator);
		}

		arena_end_temp(temp);

		if (exists) {
			return result;
		}
	}

	for (size_t i = 0; i < storage->include_dirs.count; i += 1) {
		ArenaRegion temp = arena_begin_temp(temp_allocator);

		StringBuilder builder = { .arena = temp_allocator };
		str_builder_append(&builder, path_trim_trailing_slash(storage->include_dirs.values[i]));
		str_builder_append_char(&builder, '/');
		str_builder_append(&builder, include_path);

		bool exists = path_exists(temp_allocator, builder.string);
		String result = {};
		if (exists) {
			result = path_canonicalize(builder.string, allocator, temp_allocator);
		}

		arena_end_temp(temp);

		if (exists) {
			return result;
		}
	}

	return (String) {};
}

SourceFile* _source_storage_insert(SourceStorage* storage, String path, Arena* temp_allocator) {
	assert(storage->count < storage->capacity);
	assert(path.length > 0);

	SourceFile* file = &storage->files[storage->count];
	storage->count += 1;

	file->path = path;

	ArenaRegion temp = arena_begin_temp(temp_allocator);
	file->source_code = read_entire_file_to_str(str_to_cstr(path, temp_allocator), storage->allocator);
	arena_end_temp(temp);

	file->line_info = line_info_from_source(storage->allocator, file->source_code);
	return file;
}

SourceFile* source_storage_append(SourceStorage* storage, String path, String source_code) {
	assert(storage->count < storage->capacity);
	assert(path.length > 0);

	SourceFile* file = &storage->files[storage->count];
	storage->count += 1;

	file->path = path;
	file->source_code = source_code;
	file->line_info = line_info_from_source(storage->allocator, file->source_code);
	return file;
}

SourceFile* source_storage_append_from_path(SourceStorage* storage, String path, Arena* temp_allocator) {
	assert(path.length > 0);
	assert(temp_allocator);

	ArenaRegion temp = arena_begin_temp(temp_allocator);
	String source_code = read_entire_file_to_str(str_to_cstr(path, temp_allocator), storage->allocator);
	SourceFile* source_file = source_storage_append(storage, path, source_code);
	arena_end_temp(temp);

	return source_file;
}

SourceFile* source_storage_find_file(SourceStorage* storage, String path) {
	assert(path.length > 0);

	for (size_t i = 0; i < storage->count; i += 1) {
		if (str_equal(storage->files[i].path, path)) {
			return &storage->files[i];
		}
	}

	return NULL;
}
