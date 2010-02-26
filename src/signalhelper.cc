/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <sys/types.h>

#include <errno.h>
#include <signal.h>

#include <iostream>

#include "signalhelper.h"

using namespace std;

namespace{
  void sig_func(int i);
  volatile sig_atomic_t sigTerm=0;
  volatile sig_atomic_t sigAlarm=0;
  volatile sig_atomic_t sigUsr1=0;
}

bool
signalQuit()
{
  return sigTerm!=0;
}


int
signalInit()
{
  sigset_t     oldmask;
  struct sigaction act, oldact;

  sigemptyset(&act.sa_mask);
  act.sa_flags=0;
  act.sa_handler=sig_func;

  if(sigaction(SIGTERM, &act, &oldact)<0){
    cerr << "ERROR: Can't install signal handler for SIGTERM!" << endl;
    return -1;
  }

  if(sigaction(SIGINT, &act, &oldact)<0){
    cerr << "ERROR: Can't install signal handler for SIGINT!" << endl;
    return -1;
  }

  if(sigaction(SIGUSR1, &act, &oldact)<0){
    cerr << "ERROR: Can't install signal handler for SIGUSR1!" << endl;
    return -1;
  }


  if(sigaction(SIGALRM, &act, &oldact)<0){
    cerr << "ERROR: Can't install signal handler for SIGALRM!" << endl;
    return -1;
  }

  return 0;

}


int
waitOnSignal(int timeout_in_seconds, bool &timeout)
{
  int ret=-1;
  sigset_t mask, oldmask;

  timeout=false;

  sigemptyset(&mask);

  sigaddset(&mask, SIGALRM);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGUSR1);

  if(sigprocmask(SIG_BLOCK, &mask, &oldmask)<0)
    return -1;

  sigAlarm=0;

  if(timeout_in_seconds>0)
    alarm(timeout_in_seconds);

  while(!sigAlarm && !sigTerm && !sigUsr1){

    sigsuspend(&oldmask);

    if(errno!=EINTR)
      break;
  }

  if(sigTerm){
     ret=1;
  }else  if(sigUsr1){
    sigUsr1=0;
    ret=0;
  }else{
    timeout=true;
    ret=1;
  }

  sigprocmask(SIG_SETMASK, &oldmask, NULL);

  return ret;
}


int
waitOnFifo(int fd, int timeout_in_seconds, bool &timeout)
{
  timeval tv;
  fd_set  fds;
  int     ret;

  timeout=false;

  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  if(timeout_in_seconds>0){
    tv.tv_sec=timeout_in_seconds;
    tv.tv_usec=0;

    ret=select(fd+1, &fds, NULL, NULL, &tv) ;
  }else{
    ret=select(fd+1, &fds, NULL, NULL, NULL) ;
  }

  if(ret==-1){
    if(errno==EINTR)
      return 1;
    else
      return -1;
  }else if(ret==0){
    timeout=true;
    return 1;
  }else{
    char buf[1];
    int n;
  RETRY:

    n=read(fd, buf, 1);
    if(n==-1){
      if(errno==EINTR)
	goto RETRY;
      else
	return -1;
    }else if(n==0){
      return -1;
    }
    return 0;
  }
}

int
mysleep(int ms)
{
  timespec t1, t2;
  int ret;

  t1.tv_sec=ms/1000;
  t1.tv_nsec=(ms%1000)*1000000;

  ret=nanosleep(&t1, &t2);

  while(ret==-1 && errno==EINTR){
    t1=t2;
    ret=nanosleep(&t1, &t2);
  }

  return ret;

}


namespace{
  void
  sig_func(int i)
  {
    switch(i){
    case SIGUSR1:
      sigUsr1=1;
      break;
    case SIGINT:
    case SIGTERM:
      sigTerm=1;
      break;
    case SIGALRM:
      sigAlarm=1;
      break;
    default:
      break;
    }
  }
}
