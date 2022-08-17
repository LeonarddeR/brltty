/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>

#include "log.h"
#include "options.h"
#include "pty_object.h"
#include "pty_screen.h"
#include "file.h"
#include "async_handle.h"
#include "async_wait.h"
#include "async_io.h"
#include "async_signal.h"

static int opt_showPath;
static int opt_logOutputActions;
static int opt_logUnexpectedOutput;
static int opt_logInsertedBytes;

BEGIN_OPTION_TABLE(programOptions)
  { .word = "show-path",
    .letter = 'p',
    .setting.flag = &opt_showPath,
    .description = strtext("show the path of the slave pty device")
  },

  { .word = "log-output-actions",
    .letter = 'A',
    .flags = OPT_Hidden,
    .setting.flag = &opt_logOutputActions,
    .description = strtext("log output actions")
  },

  { .word = "log-unexpected-output",
    .letter = 'U',
    .flags = OPT_Hidden,
    .setting.flag = &opt_logUnexpectedOutput,
    .description = strtext("log unexpected output")
  },

  { .word = "log-inserted-bytes",
    .letter = 'I',
    .flags = OPT_Hidden,
    .setting.flag = &opt_logInsertedBytes,
    .description = strtext("log inserted bytes")
  },
END_OPTION_TABLE

static int
setEnvironmentString (const char *variable, const char *string) {
  int result = setenv(variable, string, 1);
  if (result != -1) return 1;

  logSystemError("setenv");
  return 0;
}

static int
setEnvironmentInteger (const char *variable, int integer) {
  char string[0X10];
  snprintf(string, sizeof(string), "%d", integer);
  return setEnvironmentString(variable, string);
}

static int
setEnvironmentVariables (void) {
  {
    size_t width, height;

    if (getConsoleSize(&width, &height)) {
      if (!setEnvironmentInteger("COLUMNS", width)) return 0;
      if (!setEnvironmentInteger("LINES", height)) return 0;
    }
  }

  return setEnvironmentString("TERM", ptyGetScreenType());
}

static int
prepareChild (PtyObject *pty) {
  setsid();
  ptyCloseMaster(pty);

  if (setEnvironmentVariables()) {
    int tty;
    if (!ptyOpenSlave(pty, &tty)) return 0;
    int keep = 0;

    for (int fd=0; fd<=2; fd+=1) {
      if (fd == tty) {
        keep = 1;
      } else {
        int result = dup2(tty, fd);

        if (result == -1) {
          if (fd != 2) logSystemError("dup2");
          return 0;
        }
      }
    }

    if (!keep) close(tty);
  }

  return 1;
}

static int
runChild (PtyObject *pty, char **command) {
  char *defaultCommand[2];

  if (!(command && *command)) {
    char *shell = getenv("SHELL");
    if (!(shell && *shell)) shell = "/bin/sh";

    defaultCommand[0] = shell;
    defaultCommand[1] = NULL;
    command = defaultCommand;
  }

  if (prepareChild(pty)) {
    int result = execvp(*command, command);

    if (result == -1) {
      switch (errno) {
        case ENOENT:
          logMessage(LOG_ERR, "command not found: %s", *command);
          return PROG_EXIT_SEMANTIC;

        default:
          logSystemError("execvp");
          break;
      }
    } else {
      logMessage(LOG_ERR, "unexpected return from execvp");
    }
  }

  return PROG_EXIT_FATAL;
}

static
ASYNC_MONITOR_CALLBACK(standardInputMonitor) {
  PtyObject *pty = parameters->data;
  ptyProcessInputCharacter(ptyGetMaster(pty));
  return 1;
}

static unsigned char childHasTerminated;
static unsigned char slaveIsClosed;

static
ASYNC_CONDITION_TESTER(childTerminationTester) {
  return childHasTerminated && slaveIsClosed;
}

static
ASYNC_INPUT_CALLBACK(ptyInputHandler) {
  if (!(parameters->error || parameters->end)) {
    size_t length = parameters->length;

    if (ptyParseOutputBytes(parameters->buffer, length)) {
      ptyRefreshScreen();
    }

    return length;
  }

  slaveIsClosed = 1;
  return 0;
}

static void
childTerminationHandler (int signalNumber) {
  childHasTerminated = 1;
}

static int
getExitStatus (pid_t pid) {
  while (1) {
    int status;
    pid_t result = waitpid(pid, &status, 0);

    if (result == -1) {
      if (errno == EINTR) continue;
      logSystemError("waitpid");
      break;
    }

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 0X80 | WTERMSIG(status);

    #ifdef WCOREDUMP
    if (WCOREDUMP(status)) return 0X80 | WTERMSIG(status);
    #endif /* WCOREDUMP */

    #ifdef WIFSTOPPED
    if (WIFSTOPPED(status)) continue;
    #endif /* WIFSTOPPED */

    #ifdef WIFCONTINUED
    if (WIFCONTINUED(status)) continue;
    #endif /* WIFCONTINUED */
  }

  return PROG_EXIT_FATAL;
}

static int
runParent (PtyObject *pty, pid_t child) {
  int exitStatus = PROG_EXIT_FATAL;
  AsyncHandle ptyInputHandle;

  childHasTerminated = 0;
  slaveIsClosed = 0;

  if (asyncReadFile(&ptyInputHandle, ptyGetMaster(pty), 1, ptyInputHandler, NULL)) {
    AsyncHandle standardInputHandle;

    if (asyncMonitorFileInput(&standardInputHandle, 0, standardInputMonitor, pty)) {
      if (asyncHandleSignal(SIGCHLD, childTerminationHandler, NULL)) {
        {
          unsigned char oldLogLevel = stderrLogLevel;
          if (isatty(2)) stderrLogLevel = LOG_ERR;

          ptyBeginScreen();
          asyncAwaitCondition(INT_MAX, childTerminationTester, NULL);
          ptyEndScreen();

          stderrLogLevel = oldLogLevel;
        }

        exitStatus = getExitStatus(child);
      }

      asyncCancelRequest(standardInputHandle);
    }

    asyncCancelRequest(ptyInputHandle);
  }

  return exitStatus;
}

int
main (int argc, char *argv[]) {
  int exitStatus = PROG_EXIT_FATAL;
  PtyObject *pty;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-pty",
      .argumentsSummary = "[command [arg ...a]]"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  if (opt_logOutputActions) ptyLogOutputActions(1);
  if (opt_logUnexpectedOutput) ptyLogUnexpectedOutput(1);
  if (opt_logInsertedBytes) ptyLogInsertedBytes(1);

  if ((pty = ptyNewObject())) {
    if (opt_showPath) {
      FILE *stream = stdout;
      fprintf(stream, "%s\n", ptyGetPath(pty));
      fflush(stream);
    }

    pid_t child = fork();
    switch (child) {
      case -1:
        logSystemError("fork");
        break;

      case 0:
        _exit(runChild(pty, argv));

      default:
        exitStatus = runParent(pty, child);
        break;
    }

    ptyDestroyObject(pty);
  }

  return exitStatus;
}