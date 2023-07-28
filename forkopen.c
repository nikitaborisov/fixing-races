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

#ifdef PLATFORM_SunOS
#define _XPG4_2
#endif 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/un.h>
#include <unistd.h>

#ifdef PLATFORM_SunOS
#define CMSG_ALIGN(len) (((len) + sizeof (size_t) - 1) \
                         & (size_t) ~(sizeof (size_t) - 1))
#define CMSG_SPACE(len) (CMSG_ALIGN (len) \
                         + CMSG_ALIGN (sizeof (struct cmsghdr)))
#define CMSG_LEN(len)   (CMSG_ALIGN (sizeof (struct cmsghdr)) + (len))
#endif

#ifdef PLATFORM_FreeBSD
/* For definition of struct iovec */
#include <sys/uio.h>
#endif

int forkopen(char * path, int flags)
{
  pid_t child;
  int sv[2];
  uid_t uid = getuid();

  if (socketpair(PF_UNIX, SOCK_DGRAM, 0, sv)) {
    perror("Couldn't create socket pair");
  }
  
  if ( (child = fork()) > 0) {
    // parent: waits for fd 
    char buf[2]; // for the data payload, in this case "x"
    struct msghdr msg;
    struct iovec iov[1];
    ssize_t n;
    int childstatus;
    union {
      struct cmsghdr cm;
      char control[CMSG_SPACE(sizeof(int))]; // space for 1 fd
    } ch;
    struct cmsghdr * cmptr;

    close(sv[0]);

    msg.msg_control = ch.control;
    msg.msg_controllen = sizeof(ch.control);
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = buf;
    iov[0].iov_len = 2; 
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    if (wait(&childstatus) != child) {
      perror("wait returned an unexpected child.");
      return -1;
    }
    if (childstatus) {
      return -1;
    }
                      
    if ( (n = recvmsg(sv[1], &msg, 0)) <= 0) perror("Couldn't read");
    if ( (cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
         cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
      if (cmptr->cmsg_level != SOL_SOCKET) {
        perror("control level not sol socket");
        return -1;
      }
      if (cmptr->cmsg_type != SCM_RIGHTS) {
        perror("control type not scm_rights");
        return -1;
      }
      close(sv[1]);
      return  *((int *) CMSG_DATA(cmptr));
    } else {
      perror("descriptor not passed\n");
      return -2;
    }
  } else if (!child) {
    // child
    struct msghdr mh;
    struct iovec iov[1];
    union {
      struct cmsghdr cm;
      char control[CMSG_SPACE(sizeof(int))]; // space for 1 fd.
    } ch;
    struct cmsghdr * cmptr;
    int fd;

    setuid(uid);
    close(sv[1]);

    mh.msg_name = NULL;
    mh.msg_namelen = 0;
    mh.msg_iov = iov;
    iov[0].iov_base = "x"; // string payload. iov != NULL easier to debug
    iov[0].iov_len = 2;
    mh.msg_iovlen = 1;
    
    mh.msg_control = &ch;
    mh.msg_controllen = sizeof(ch.control);
    mh.msg_flags = 0;

    cmptr = CMSG_FIRSTHDR(&mh);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    fd = *((int*) CMSG_DATA(cmptr)) = open (path, flags);
    if (sendmsg(sv[0], &mh, 0) != 2) { // num chars in iov
      perror("Couldn't properly send.");
      exit(1);
    }
    close (sv[0]);
    close(fd);
    exit(0);
  } else
    perror ("Couldn't fork");
}


#ifdef TEST
#include <stdio.h>
#include <assert.h>

int main(int argc, char * argv[])
{
  int fd;
  char buf[256];
  int count;

  fd = forkopen(argv[1], O_RDONLY);
  assert(fd > 0);
  while (count = read(fd, buf, 255)) {
    buf[count] = 0;
    printf("%s", buf);
  }
  close(fd);
}
#endif
