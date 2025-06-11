#define NOB_IMPLEMENTATION
#include <nob.h>

#define nob_cc(cmd) nob_cmd_append(cmd, "gcc")
#define nob_cc_flags(cmd) nob_cmd_append(cmd, "-Wall", "-Wextra", "-fsanitize=address")
#define nob_cc_in(cmd, files...) nob_cmd_append(cmd, ##files)
#define nob_cc_out(cmd, file) nob_cmd_append(cmd, "-o", file)

#define MAIN_BINARY "main"
#define MAIN_SOURCE MAIN_BINARY ".c"



static inline bool is_flag(const char *test, const char *flag) { return strcmp(test, flag) == 0; }
#define FLAG_CLEAN "-clean"



// ./nob <OPTIONS>...
//		-clean - remove BINARY
int main(int argc, char *argv[]) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	int result = 0;
	Nob_Cmd cmd = {0};

	struct {
		bool clean : 1;
	} flags;


	
	const char *program = nob_shift_args(&argc, &argv);
	const char *flag;
	while (argc > 0) {
		flag = nob_shift_args(&argc, &argv);

		if (is_flag(flag, FLAG_CLEAN)) {
			nob_cmd_append(&cmd, "rm", MAIN_BINARY);
			nob_cmd_run_sync(&cmd);
			nob_return_defer(0);
		}
	}



	Nob_Cmd paths = {0};
	nob_cmd_append(&paths, MAIN_SOURCE, "tracks.h", "audio.h");

	nob_cc(&cmd);
	nob_cc_flags(&cmd);
	nob_cc_in(&cmd, MAIN_SOURCE);
	nob_cc_out(&cmd, MAIN_BINARY);
	if (nob_needs_rebuild(MAIN_BINARY, paths.items, paths.count) && !nob_cmd_run_sync_and_reset(&cmd)) nob_return_defer(1);

defer:
	nob_cmd_free(&cmd);
	return result;
}