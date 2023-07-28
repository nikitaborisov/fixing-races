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

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

static int wait_method = 1;
static int krounds = 7;
static int randomized = 0;
static int single_user = 0;

void kopen_params(int waitmethod, int rounds, int rnd, int suser)
{
  wait_method = waitmethod;
  krounds = rounds;
  randomized = rnd;
  single_user = suser;
}

static void busywait1(int usecs)
{
  struct timeval tv;
  struct timezone tz;
  long starttime;
  long now;

  gettimeofday(&tv, &tz);
  starttime = 1000 * tv.tv_sec + tv.tv_usec;
  do {
    gettimeofday(&tv, &tz);
    now = 1000 * tv.tv_sec + tv.tv_usec;
  } while (now - starttime < usecs);
}

static void busywait2(int iters)
{
  int i;
  for (i = 0; i < iters; i++)
    ;
}

static void busywait(void)
{
  switch (wait_method) {
  case 0:
    return;
  case 1:
    return busywait1(random() % 5);
  case 2:
    return busywait2(random() % 10000);
  }
}

int kopen(char * path, int flags)
{
  int fd, rept_fd;
  struct stat buffer;
  ino_t orig_inode;
  dev_t orig_device;
  int i;
  int k = krounds;
  char buf[100];
  int last_action;
  int accessmode = single_user ? W_OK : flagstomode(flags);

  busywait();
  if (access(path, accessmode) != 0) {
    perror("access");
    return -1;
  }

  busywait();
  fd = open(path, flags);
  if (fd < 0) {
    perror("open");
    return -1;
  }
  if (fstat(fd, &buffer) != 0) {
    perror("fstat");
    return -1;
  }
  orig_device = buffer.st_dev;
  orig_inode = buffer.st_ino;


  last_action = 1;
  for (i = 0; i < 2*k; i++) {
    int action;

    if (randomized)
      action = random() % 2;
    else
      action = 1 - last_action;

    last_action = action;

    busywait();

    if (action == 0) {
      if (access(path, accessmode) != 0) {
	perror("access");
	return -1;
      }
    }
    else if (action == 1) {
      rept_fd = open(path, flags);
      if (rept_fd < 0) {
	perror("open");
	return -1;
      }
      
      if (fstat(rept_fd, &buffer)) {
	perror("fstat");
	return -1;
      }
      
      if (close(rept_fd)) {
	perror("close");
	return -1;
      }

      if (orig_inode != buffer.st_ino ||
	  orig_device != buffer.st_dev) {
	fprintf(stderr, "race detected in round %d\n", i);
	return -1;
      }
    }
  }
  return fd;
}

#ifdef TEST
#include <assert.h>
int main(int argc, char * argv[])
{
  int fd;
  char buf[256];
  int count;

  fd = kopen(argv[1], O_RDONLY);
  assert(fd > 0);
  while (count = read(fd, buf, 255)) {
    buf[count] = 0;
    printf("%s", buf);
  }
  close(fd);
}
#endif
