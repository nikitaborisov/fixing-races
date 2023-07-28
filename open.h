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

#ifndef __OPEN_H
#define __OPEN_H

#include <unistd.h>

typedef int(*aopen)(char*, int);

/*
 * converts open style flags to the access "mode" param.
 */
inline int flagstomode(int flags)
{
  if ( (flags & 3) == O_RDONLY) return R_OK;
  if ( (flags & 3) == O_WRONLY) return W_OK;
  if ( (flags & 3) == O_RDWR) return R_OK | W_OK;
  return -1;
}

/*
  these calls are all meant to be drop in replacements for open.
  as such, flags are O_RDONLY, O_WRONLY, or O_RDWR xored with other options.
  (the other options may not be obeyed with these replacements).
  
  flags are NOT of the access variety (R_OK, W_OK, X_OK, F_OK).
*/
int kopen(char * path, int flags);
int forkopen(char * path, int flags);
int open_access(char * path, int flags);
inline int normalopen(char * path, int flags) { return open (path, flags);}
inline int accessopen1(char * path, int flags) {
  if (access(path, flagstomode(flags) )) { return -1;}
  return open (path, flags);
}

/*
  sets params for kopen;
     waitmethod \in {0 = nowait, 1 = gettimeofday, 2 = busywait1
     rounds = number of rounds
     rnd \in {0 = not randomized (DH scheme), 1 = randmoized (strengthened)}
     suser \in {0 = do real access/open,
                1 = use access(..., W_OK)/open(...,O_RDONLY) trick
                    for single-user testing}
*/
void kopen_params(int waitmethod, int rounds, int rnd, int suser);

#endif
