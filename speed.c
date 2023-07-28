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
#include <assert.h>
#include <fcntl.h>
#include "open.h"


#ifdef PLATFORM_SunOS
# include <sys/types.h>
# include <sys/processor.h>
double get_cpuspeed() 
{ 
  processor_info_t pi;
  assert(processor_info(0, &pi) == 0);
  return pi.pi_clock;
}
#elif defined(PLATFORM_FreeBSD)
# include <sys/types.h>
# include <sys/sysctl.h>
double get_cpuspeed()
{
  int ret;
  char buf[1000];
  int buflen = sizeof(buf);
  int multiplier = 1;
  float speed;

  ret = sysctlbyname("hw.model", buf, &buflen, NULL, 0);
  assert(ret == 0);

  assert (sscanf(buf, "Intel(R) Pentium(R) 4 CPU %fMHz", &speed) == 1);
  return speed;
}
#else
double get_cpuspeed()
{
  static double speed = 0.0;
  if (speed > 0.001)  return speed;
  {  
    FILE * f = fopen("/proc/cpuinfo", "r");
    char buf[256];
    assert(f> 0);
    while (fgets(buf, 255, f)) {
      float ff;
      if (sscanf(buf, "cpu MHz         : %f", &ff)) { speed = ff; break; }
    }
    fclose(f);
    return speed;
  }
}
#endif

#ifdef PLATFORM_SunOS
# include <sys/time.h>
# define rdtscl(tm) ((tm) = gethrtime() * get_cpuspeed() / 1000)
#else
#define rdtscl(tm) { int tmi; \
     __asm__ __volatile__("rdtsc" : "=a" (tmi) : : "edx"); tm = tmi; }
#endif

#define REPORT(c, s) printf("%20s || %12.3f cycles | %8.3f u_seconds\n", s, \
               ((double)c), ((double)(c)) / get_cpuspeed())

#define N_TRIALS 1000

void trial(aopen opener, char * path, int flags, char * desc)
{
  int fd;
  long long t_start;
  long long t_finish;
  int i, trials = 0;
  double csum = 0;
  char buf[256];

  while (trials < N_TRIALS) {
    rdtscl(t_start);
    fd = opener(path, flags);
    rdtscl(t_finish);
    if (fd <= 0) { perror("Couldn't open file; exiting this trial"); return;}
    if (trials) {
      if (t_finish > t_start) {
        csum += (t_finish - t_start);
        trials++;
      }
    } else trials++;
    close(fd);
  }
  csum = csum / trials;
  REPORT(csum , desc);
}
           
#define KOPEN_ROUNDS 7
#define KOPEN_WAIT 0
#define KOPEN_RAND 0
#define KOPEN_SUSER 0

int main(int argc, char * argv[])
{
  char buf [256];
  if (argc != 2) { printf("Usage: %s <path>\n", argv[0]); exit(1); }

  kopen_params(KOPEN_WAIT, KOPEN_ROUNDS, KOPEN_RAND, KOPEN_SUSER);
  sprintf(buf, "kopen (%d,%d,%d, %d)", 
	  KOPEN_WAIT, KOPEN_ROUNDS, KOPEN_RAND, KOPEN_SUSER);

  printf("\n%d trials opening %s ; CPU-speed %f MHz\n\n",
         N_TRIALS, argv[1], get_cpuspeed());
  trial (normalopen, argv[1], O_RDONLY, "open");
  trial (accessopen1, argv[1], O_RDONLY, "access-open1");  
  //  trial (open_access, argv[1], O_RDONLY, "open_access(user)");
  trial (kopen, argv[1], O_RDONLY, buf);
  trial (forkopen, argv[1], O_RDONLY, "forkopen");
  printf("\n");
}
