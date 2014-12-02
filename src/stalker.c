#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "uv.h"

#define VERSION "0.1.0"
#define INTERVAL 100
#define ARGS_MAX 128

#define MAKE_VALGRIND_HAPPY() \
  do { \
    close_loop(uv_default_loop()); \
    uv_loop_delete(uv_default_loop()); \
  } while (0)

static void handle_poll(uv_fs_poll_t *handle,
  int status_, const uv_stat_t *prev, const uv_stat_t *curr);
static void handle_close_walk(uv_handle_t *handle, void *arg);
static void close_loop(uv_loop_t *loop);
static void run_loop(const char *file);

static void redirect_stdout(const char *path);
static int option(const char *small, const char *large, 
  const char *arg);
static void usage();

static uv_fs_poll_t poll_handle;
static uv_loop_t *loop;

static char *cmd[4] = { "sh", "-c", 0, 0 };
static int quiet = 0;
static int halt = 0;

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
    "    -q, --quiet           only output stderr\n"
    "    -x, --halt            halt on failure\n"
    "    -v, --version         output version number\n"
    "    -h, --help            output this help information\n"
    "\n"
    );
  exit(1);
}

static void redirect_stdout(const char *path) {
  int fd = open(path, O_WRONLY);
  if (dup2(fd, 1) < 0) {
    perror("dup2()");
    exit(1);
  }
}

static void handle_poll(uv_fs_poll_t *handle,
  int status_, const uv_stat_t *prev, const uv_stat_t *curr) {

  pid_t pid;
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
      fprintf(stderr, "\033[90mexit: %d\33[0m\n\n", WEXITSTATUS(status));
      if (halt) {
        MAKE_VALGRIND_HAPPY();
        exit(WEXITSTATUS(status));
      }
    }
  }
}

void run_loop(const char *file) {
  loop = uv_default_loop();
  uv_fs_poll_init(loop, &poll_handle);
  uv_fs_poll_start(&poll_handle, handle_poll, file, INTERVAL);
  uv_run(loop, UV_RUN_DEFAULT);
}

int main(int argc, const char **argv){
  signal(SIGPIPE, SIG_IGN);

  if (1 == argc) usage();

  int len = 0;
  int interpret = 1;
  char *args[ARGS_MAX] = {0};

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];

    if (!interpret) goto arg;

    if (option("-h", "--help", arg)) usage();

    if (option("-q", "--quiet", arg)) {
      quiet = 1;
      continue;
    }

    if (option("-x", "--halt", arg)) {
      halt = 1;
      continue;
    }

    if (option("-v", "--version", arg)) {
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

  run_loop(args[1]);
}
