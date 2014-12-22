#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "uv.h"

#define VERSION "0.1.2"
#define ARGS_MAX 128

// cleanup before exiting
#define MAKE_VALGRIND_HAPPY() \
  do { \
    close_loop(uv_default_loop()); \
    uv_loop_delete(uv_default_loop()); \
  } while (0)

#define EXIT1() \
  MAKE_VALGRIND_HAPPY(); \
  exit(1);

static void handle_update (uv_fs_event_t *handle, const char *file,
  int events, int status);
static void handle_close_walk(uv_handle_t *handle, void *arg);
static void handle_exit(uv_process_t *req, int64_t exit_status, int term_signal);
static void close_loop(uv_loop_t *loop);
static void init_fs_event(const char *file);
static int option(const char *small, const char *large, const char *arg);
static void usage();

static uv_loop_t *loop;
static uv_process_t process;
static uv_process_options_t options;

static char *cmd[4] = { "sh", "-c", NULL, NULL };

static int halt = 0; // halt if cmd exits with 1
static int verbose = 0; // makes the operation more talkative

static void handle_close_walk(uv_handle_t *handle, void *arg) {
  if (!uv_is_closing(handle))
    uv_close(handle, NULL);
}

static void close_loop(uv_loop_t *loop) {
  // walk the list of handles and request them to be closed
  uv_walk(loop, handle_close_walk, NULL);
}

static int option(const char *small, const char *large, const char *arg) {
  if (!strcmp(small, arg) || !strcmp(large, arg)) return 1;
  return 0;
}

static void usage() {
  printf(
    "\n"
    "  usage: stalker [options] <cmd> <file>\n"
    "\n"
    "  options:\n"
    "\n"
    "    -x, --halt            halt on failure\n"
    "    -h, --help            display this help message\n"
    "    -v, --verbose         make the operation more talkative\n"
    "    -V, --version         display the version number\n"
    "\n"
    );
  exit(1);
}

static void handle_exit(uv_process_t *process, int64_t exit_status,
  int term_signal) {
  if (verbose) {
    fprintf(stderr, "\033[32mexit \033[0m\033[0m%lld\n", exit_status);
    
    if (exit_status == 127) {
      fprintf(stderr, "\033[31mnot found\033[0m %s\n", cmd[2]);
    }
  }

  uv_close((uv_handle_t*) process, NULL);

  if (halt && exit_status != 0) {
    if (verbose) {
      fprintf(stderr, "\033[36mhalting\033[0m\n");
    }
    EXIT1();
  }
}

static void handle_update (uv_fs_event_t *handle, const char *file,
  int events, int status_) {

  if (verbose) {
    fprintf(stderr, "\033[36m");
    if (events == UV_RENAME)
        fprintf(stderr, "rename");
    if (events == UV_CHANGE)
        fprintf(stderr, "change");
    // fprintf(stderr, "\033[90m%s\033[0m", handle->path);
    if (file) fprintf(stderr, "\033[0m %s\n", file);
    else fprintf(stderr, "\033[0m\n");
  }

  int err;
  // send a signum == 0 to check if child process is still alive
  err = uv_kill(process.pid, 0);

  if (process.pid == 0 || UV_ESRCH == err) {
    int r;
    if ((r = uv_spawn(loop, &process, &options))) {
      fprintf(stderr, "%s\n", uv_strerror(r));
      EXIT1();
    }
  }
}

void init_fs_event(const char *file) {
  // declare & initialize our fs event handle
  uv_fs_event_t *fs_event_req = malloc(sizeof(uv_fs_event_t));
  uv_fs_event_init(loop, fs_event_req);
  // start watching for changes recursively
  uv_fs_event_start(fs_event_req, handle_update, file, UV_FS_EVENT_RECURSIVE);
}

int main(int argc, const char **argv){
  // declare our main loop
  loop = uv_default_loop();

  // ignore SIGPIPE
  // see: http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod#The_special_problem_of_SIGPIPE
  signal(SIGPIPE, SIG_IGN);

  if (1 == argc) usage();

  // collect args
  int len = 0;
  int interpret = 1;
  char *args[ARGS_MAX] = {0};

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];

    if (!interpret) goto arg;

    if (option("-h", "--help", arg)) usage();

    if (option("-v", "--verbose", arg)) {
      verbose = 1;
      continue;
    }

    if (option("-x", "--halt", arg)) {
      halt = 1;
      continue;
    }

    if (option("-V", "--version", arg)) {
      printf("%s\n", VERSION);
      exit(1);
    }

    if (len == ARGS_MAX) {
      fprintf(stderr, "number of arguments exceeded %d\n", len);
      exit(1);
    }

  arg:
    args[len++] = (char *) arg;
    interpret = 0;
  }

  if (!len) {
    fprintf(stderr, "\n  <cmd> required\n");
    usage();
  }

  if (len != 2) {
    fprintf(stderr, "\n  <file> required\n");
    usage();
  }

  *(cmd + 2) = args[0];

  // options to be passed to uv_spawn
  options.exit_cb = handle_exit;
  options.file = cmd[0];
  options.args = cmd;

  if (verbose)
    fprintf(stderr, "\033[36mwatching \033[0m\033[0m%s\n", args[1]);
  
  // start monitoring the given path
  init_fs_event(args[1]);

  // run our main loop
  uv_run(loop, UV_RUN_DEFAULT);

  return 0;
}
