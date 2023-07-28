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
#include <fcntl.h>
#include "open.h"

int main (int argc, char ** argv)
{
  char *path;
  int fd, k, randomized, wait_method;
  char buf[100];
  int suser;

  if (argc != 6) {
    printf("victim <s> <k> <r|d> <0|1|2> <path>\n"
	   "  s:      0: do real access/open, 1: use single-user testing trick\n"
           "  k:      rounds of strengthening\n"
	   "  r|d:    randomized or deterministic strengthening\n"
	   "  0|1|2:  0: no waits, 1: timed waits, 2: busyloop waits\n"
	   "  path:   file to access/open\n");
    exit(0);
  }

  suser = atoi(argv[1]);
  k = atoi(argv[2]);
  randomized = argv[3][0] == 'r' ? 1 : 0;
  wait_method = atoi(argv[4]);
  path = argv[5];

  srandom(time(NULL));
  kopen_params(wait_method, k, randomized, suser);
  fd = kopen(path, O_RDONLY);
  if (fd < 0)
    return 2;

  memset(buf, 0, sizeof(buf));
  read(fd, buf, sizeof(buf));
  close(fd);

  printf("%s", buf);

  if (buf[0] == 'b')
    return 0;
  if (buf[0] == 'g')
    return 1;

  abort();
}

