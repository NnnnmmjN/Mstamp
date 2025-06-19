#ifndef AUDIO_H_
#define AUDIO_H_
#include "tracks.h"
#include <miniaudio.h>

typedef struct {
	Tracks tracks;
	ma_decoder decoder;
} MusicCollection;

ma_result audio_init();
void audio_deinit();

bool audio_unpause();
bool audio_pause();
void audio_restart();

MusicCollection audio_load_tracks(Arena *a, const char *music_path, const char *timestamp_path);
void audio_unload_tracks(MusicCollection *music);
void audio_select_track(MusicCollection *music, size_t index);

#endif // AUDIO_H_

#ifdef AUDIO_IMPLEMENTATION
#undef AUDIO_IMPLEMENTATION
#define TRACKS_IMPLEMENTATION
#include "tracks.h"

void play_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);

static ma_decoder_config decoder_config = {0};
static ma_device device = {0};
static Track *current_track = NULL;
static MusicCollection *current_music = NULL;



#define check_ma_result(fmt, ...) do { \
		if (result != MA_SUCCESS) { \
			nob_log(NOB_ERROR, __FILE__":%d: " fmt ": %s", __LINE__, ##__VA_ARGS__, ma_result_description(result)); \
			goto defer; \
		} \
	} while (0)

#define SAMPLE_FORMAT	ma_format_f32
#define CHANNEL_COUNT	2
#define SAMPLE_RATE		48000
#define CHUNK_SIZE		(1<<11)
ma_result audio_init() {
	ma_result result;
	ma_device_config device_config = {0};

	device_config = ma_device_config_init(ma_device_type_playback);
	device_config.playback.format = SAMPLE_FORMAT;
	device_config.playback.channels = CHANNEL_COUNT;
	device_config.sampleRate = SAMPLE_RATE;
	device_config.periodSizeInFrames = CHUNK_SIZE;
	device_config.dataCallback = play_callback;

	result = ma_device_init(NULL, &device_config, &device);
	check_ma_result("Failed to initialize play device");



	decoder_config = ma_decoder_config_init(SAMPLE_FORMAT, CHANNEL_COUNT, SAMPLE_RATE);
	decoder_config.seekPointCount = 1<<10;	// seek table to avoid reading from the beggining

	return result;
defer:
	audio_deinit();
	return result;
}
#undef SAMPLE_FORMAT
#undef SAMPLE_RATE
#undef CHUNK_SIZE

void audio_deinit() {
	ma_device_uninit(&device);
}

MusicCollection audio_load_tracks(Arena *a, const char *music_path, const char *timestamp_path) {
	ma_result result;
	MusicCollection music = {0};

    if (!tracks_read_from_file(a, timestamp_path, &music.tracks)) nob_return_defer(MA_INVALID_FILE);

	result = ma_decoder_init_file(music_path, &decoder_config, &music.decoder);
	check_ma_result("Failed to load music file `%s`", music_path);

    nob_log(NOB_INFO, "Opened `%s`", strrchr(music_path, '/'));
	nob_log(NOB_INFO, "Opened `%s`", strrchr(timestamp_path, '/'));



	ma_uint64 ilength;
	ma_uint32 sample_rate;
	ma_data_source_get_length_in_pcm_frames(&music.decoder, &ilength);
	ma_data_source_get_data_format(&music.decoder, NULL, NULL, &sample_rate, NULL, CHANNEL_COUNT);
	tracks_set_end_time(music.tracks, ilength / sample_rate);
	
defer:
	nob_temp_reset();
	return music;
}

void audio_unload_tracks(MusicCollection *music) {
	ma_decoder_uninit(&music->decoder);
	nob_da_free(&music->tracks);
}

void audio_select_track(MusicCollection *music, size_t index) {
	ma_uint32 sample_rate;
	ma_data_source_get_data_format(&music->decoder, NULL, NULL, &sample_rate, NULL, CHANNEL_COUNT);

	current_music = music;
	current_track = track_get(music->tracks, index);
	ma_data_source_set_range_in_pcm_frames(&music->decoder, current_track->start * sample_rate, current_track->stop * sample_rate);
	ma_data_source_set_looping(&music->decoder, MA_TRUE);
    nob_log(NOB_INFO, "Selected song %zu: `%s`", index, current_track->title);
}



#undef check_ma_result
#define check_ma_result(res, fmt, ...) do { \
		ma_result result = res; \
		if (result != MA_SUCCESS) { \
			nob_log(NOB_ERROR, __FILE__":%d: " fmt ": %s", __LINE__, ##__VA_ARGS__, ma_result_description(result)); \
			return false; \
		} \
	} while (0)

bool audio_unpause() {
    if (current_track == NULL) return false;
	check_ma_result(ma_device_start(&device), "Failed to start playback audio device");
	return true;
}

bool audio_pause() {
    if (current_track == NULL) return false;
	check_ma_result(ma_device_stop(&device), "Failed to stop playback audio device");
	return true;
}

void audio_restart() {
    if (current_track == NULL || current_music == NULL) return;
	ma_decoder_seek_to_pcm_frame(&current_music->decoder, 0);
}



void play_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    if (current_track == NULL || current_music == NULL) return;
	NOB_UNUSED(pInput);
	NOB_UNUSED(pDevice);

    ma_uint64 framesRead;
    ma_data_source_read_pcm_frames(&current_music->decoder, pOutput, frameCount, &framesRead);

	// total_frames += framesRead;
	// if (total_frames >= 100000) {
	// 	nob_log(NOB_DEBUG, "%lld", total_frames);
	// 	total_frames = 0ULL;
		// ma_event_signal(&event_close);
	// }
}

// #undef check_ma_result
#endif // AUDIO_IMPLEMENTATION