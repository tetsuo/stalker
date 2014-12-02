#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "uv.h"

#define VERSION "0.1.2"
#define INTERVAL 100
#define ARGS_MAX 128

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
static void redirect_stdout(const char *path);
static int option(const char *small, const char *large, const char *arg);
static void usage();

static uv_loop_t *loop;
static uv_process_t process;
static uv_process_options_t options;
static char *cmd[4] = { "sh", "-c", NULL, NULL };
static int quiet = 0;
static int halt = 0;
static int verbose = 0;

static void handle_close_walk(uv_handle_t *handle, void *arg) {
  if (!uv_is_closing(handle))
    uv_close(handle, NULL);
}

static void close_loop(uv_loop_t *loop) {
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
    "    -q, --quiet           only output stderr\n"
    "    -v, --verbose         make the operation more talkative\n"
    "    -V, --version         display the version number\n"
    "\n"
    );
  exit(1);
}

static void redirect_stdout(const char *path) {
  int fd = open(path, O_WRONLY);
  if (dup2(fd, 1) < 0) {
    perror("dup2()");
    EXIT1();
  }
}

static void handle_exit(uv_process_t *req, int64_t exit_status,
  int term_signal) {
  fprintf(stderr, "Process exited with status %lld, signal %d\n", exit_status, term_signal);
  uv_close((uv_handle_t*) req, NULL);
}

static void handle_update (uv_fs_event_t *handle, const char *file,
  int events, int status_) {

  if (verbose) {
    fprintf(stderr, "\033[36m");
    if (events == UV_RENAME)
        fprintf(stderr, "renamed");
    if (events == UV_CHANGE)
        fprintf(stderr, "changed");
    fprintf(stderr, ": \033[0m\n \033[90mpath:\033[0m %s\n", handle->path);
    if (file)
      fprintf(stderr, " \033[90mfile:\033[0m %s\n", file);
  }

  int r;
  if ((r = uv_spawn(loop, &process, &options))) {
    fprintf(stderr, "%s\n", uv_strerror(r));
    EXIT1();
  }

/*  pid_t pid;
  int status;

  if ((pid = fork()) < 0) {
    fprintf(stderr, "fork()");
    MAKE_VALGRIND_HAPPY();
    exit(1);
  } else if (pid == 0) {
    if (quiet) redirect_stdout("/dev/null");
    execvp(cmd[0], cmd);
  } else {
    if (waitpid(pid, &status, 0) < 0) {
      perror("waitpid()");
      MAKE_VALGRIND_HAPPY();
      exit(1);
    }
    if (WEXITSTATUS(status)) {
      fprintf(stderr, "\033[31mexit: %d\33[0m\n\n", WEXITSTATUS(status));
      if (halt) {
        MAKE_VALGRIND_HAPPY();
        exit(WEXITSTATUS(status));
      }
    }
  }*/

}

void init_fs_event(const char *file) {
  uv_fs_event_t *fs_event_req = malloc(sizeof(uv_fs_event_t));
  uv_fs_event_init(loop, fs_event_req);
  uv_fs_event_start(fs_event_req, handle_update, file, UV_FS_EVENT_RECURSIVE);
}

int main(int argc, const char **argv){
  signal(SIGPIPE, SIG_IGN);
  loop = uv_default_loop();

  if (1 == argc) usage();

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

    if (option("-q", "--quiet", arg)) {
      quiet = 1;
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
  options.exit_cb = handle_exit;
  options.file = cmd[0];
  options.args = cmd;
  
  init_fs_event(args[1]);
  uv_run(loop, UV_RUN_DEFAULT);

  return 0;
}
