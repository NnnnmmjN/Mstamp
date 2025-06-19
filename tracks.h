#ifndef TRACKS_H_
#define TRACKS_H_
#include <nob.h>
#include <arena.h>
#include <stdint.h>

typedef struct {
	const char *title;
	uint32_t start;
	uint32_t stop;
} Track;

typedef struct {
	Track *items;
	size_t count;
	size_t capacity;
} Tracks;

static inline Track *track_get(Tracks tracks, size_t i);
static inline Track *track_get_inbound(Tracks tracks, size_t i);
static inline Track *track_get_first(Tracks tracks);
static inline Track *track_get_last(Tracks tracks);

static inline void moddiv(unsigned a, unsigned b, unsigned *mod, unsigned *div);
const char *time_from_seconds(Arena *a, uint32_t seconds);
static uint32_t seconds_from_time(Nob_StringView time);

bool tracks_read_from_file(Arena *a, const char *timestamp_file, Tracks *tracks);
static inline bool tracks_set_end_time(Tracks tracks, uint32_t seconds);

#endif // TRACKS_H_

#ifdef TRACKS_IMPLEMENTATION
#undef TRACKS_IMPLEMENTATION

static inline void moddiv(unsigned a, unsigned b, unsigned *mod, unsigned *div) {
	*div = a / b;
	*mod = a - *div * b;
}

#define SEC60 60
const char *time_from_seconds(Arena *a, uint32_t seconds) {
	uint32_t value[3] = {0};

	moddiv(seconds, SEC60, &value[0], &value[1]);
	if (value[1] >= SEC60) {
		moddiv(value[1], SEC60, &value[1], &value[2]);
		return arena_sprintf(a, "%u:%02u:%02u", value[2], value[1], value[0]);
	}
	return arena_sprintf(a, "%01u:%02u", value[1], value[0]);
}

static uint32_t seconds_from_time(Nob_StringView time) {
	uint32_t value[3] = {0};
	uint8_t i = 0;

	for (; i < 3 && time.count; i++) {
		Nob_StringView sv_value = nob_sv_chop_by_delim(&time, ':');
		const char *cstr_value = nob_temp_sv_to_cstr(sv_value);
		value[i] = atoi(cstr_value);
	}

	uint32_t res = 0;
	for (uint8_t j = 0; j < i; j++) {
		uint32_t mult = 1;
		for (uint8_t k = 0; k < j; k++) mult *= SEC60;
		res += value[i - j - 1] * mult;
	}
	return res;
}
#undef SEC60



static inline Track *track_get(Tracks tracks, size_t i) {
	return &tracks.items[i];
}

static inline Track *track_get_inbound(Tracks tracks, size_t i) {
	return i < tracks.count ? &tracks.items[i] : NULL;
}

static inline Track *track_get_first(Tracks tracks) {
	return tracks.count != 0UL ? &tracks.items[0] : NULL;
}

static inline Track *track_get_last(Tracks tracks) {
	return tracks.count != 0UL ? &tracks.items[tracks.count - 1UL] : NULL;
}



char *arena_sv_to_cstr(Arena *a, Nob_StringView sv) {
	char *result = arena_alloc(a, sv.count + 1);
	ARENA_ASSERT(result != NULL && "Arena allocation returned NULL");
	memcpy(result, sv.items, sv.count);
	result[sv.count] = '\0';
	return result;
}

bool tracks_read_from_file(Arena *a, const char *timestamp_file, Tracks *tracks) {
	Nob_StringBuilder sb = {0};
	if (!nob_read_entire_file(timestamp_file, &sb)) return false;

	Nob_StringView sv = nob_sb_to_sv(sb);
	Nob_StringView line, time;

	while ((line = nob_sv_chop_by_delim(&sv, '\n'), line.count)) {
		time = nob_sv_chop_by_delim(&line, '\t');
		nob_da_append(tracks, ((Track) {
			.title = arena_sv_to_cstr(a, line),
			.start = seconds_from_time(time),
		}));
	}

	for (size_t i = 1UL; i < tracks->count; i++) {
		Track *prev = &tracks->items[i - 1UL];
		Track *next = &tracks->items[i];
		prev->stop = next->start;
	}

	nob_temp_reset();
	nob_sb_free(&sb);
	return true;
}

static inline bool tracks_set_end_time(Tracks tracks, uint32_t seconds) {
	Track *t = track_get_last(tracks);
	if (t) return (t->stop = seconds);
	return false;
}

#endif // TRACKS_IMPLEMENTATION