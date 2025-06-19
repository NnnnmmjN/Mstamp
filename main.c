#define NOB_IMPLEMENTATION
#include <nob.h>
#define ARENA_IMPLEMENTATION
#include <arena.h>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#define TIMESTAMPS_FOLDER "timestamps/"
#define TIMESTAMP_RIMWORLD_BASE "rimworld.time"
#define TIMESTAMP_RIMWORLD_ROYALTY "rimworld_royalty.time"
#define TIMESTAMP_RIMWORLD_ANOMALY "rimworld_anomaly.time"
#define MUSIC_FOLDER "music/"
#define MUSIC_RIMWORLD_BASE "RimWorld OST.mp3"
#define MUSIC_RIMWORLD_ROYALTY "RimWorld Royalty OST.mp3"
#define MUSIC_RIMWORLD_ANOMALY "RimWorld Anomaly OST.mp3"

static const char *map_music_to_timestamp[][2] = {
	{MUSIC_RIMWORLD_BASE, TIMESTAMP_RIMWORLD_BASE},
	{MUSIC_RIMWORLD_ROYALTY, TIMESTAMP_RIMWORLD_ROYALTY},
	{MUSIC_RIMWORLD_ANOMALY, TIMESTAMP_RIMWORLD_ANOMALY},
};
static const size_t map_music_to_timestamp_count = NOB_ARRAY_LEN(map_music_to_timestamp);

static inline Nob_StringView get_last_in_path(Nob_StringView *path) {
	return nob_sv_rchop_by_delim(path, '/');
}

const char *music_file_get_name(const char *music_file) {
	const char *name = strrchr(music_file, '/');
	if (!name++) name = music_file;
	return name;
}

const char *timestamps_from_music_name(const char *music_file) {
	for (size_t i = 0UL; i < map_music_to_timestamp_count; i++) {
		const char *name = music_file_get_name(music_file);
		if (strcmp(map_music_to_timestamp[i][0], name) == 0)
			return map_music_to_timestamp[i][1];
	}
	return NULL;
}

const char *get_relative_path_to_music(const char *music_file) {
	size_t music_file_path_len = strlen(music_file) + 1;
	
	char *buffer = nob_temp_alloc(music_file_path_len);
	memcpy(buffer, music_file, music_file_path_len);

	char *name;
	name = strrchr(buffer, '/');
	if (name) *name = '\0';
	else snprintf(buffer, music_file_path_len, "../");
	name = strrchr(buffer, '/');
	if (name++) *name = '\0';
	else *buffer = '\0';

	return buffer;
}

#define AUDIO_IMPLEMENTATION
#include "audio.h"



static inline void usage(const char *program) {
	fprintf(stderr, "Usage: %s <input.mp3> [track_index=0]\n", program);
}

int main(int argc, char *argv[]) {
	int result = 0;
    size_t index = 0;

	Nob_StringView program_path = nob_sv_from_cstr(nob_shift_args(&argc, &argv));
	Nob_StringView program = get_last_in_path(&program_path);
	if (argc <= 0) {
		nob_log(NOB_ERROR, "Missing input file");
		usage(program.items);
		return 1;
	}
	const char *music_file = nob_shift_args(&argc, &argv);
    if (argc > 0) index = atoi(nob_shift_args(&argc, &argv));
	const char *timestamp_file = nob_temp_sprintf("%s" TIMESTAMPS_FOLDER "%s", get_relative_path_to_music(music_file), timestamps_from_music_name(music_file));



	if (audio_init() != MA_SUCCESS) nob_return_defer(2);

    Arena a = {0};
	MusicCollection music = audio_load_tracks(&a, music_file, timestamp_file);

    // for (size_t i = 0UL; i < tracks.count; i++) {		// list tracks and their indices
    //     Track *t = track_get(tracks, i);
    //     printf("%zu : `%s` %u-%u\n", i, t->title, t->start, t->stop);
    // }

	audio_select_track(&music, index);
	audio_unpause();

	getchar();

	audio_unload_tracks(&music);
defer:
	audio_deinit();
    arena_free(&a);
	return result;
}