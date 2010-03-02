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
#ifndef __SIGNAL_HELPER__
#define __SIGNAL_HELPER__


/**
 * \brief Init the signalhandlers we want to use.
 */
int
signalInit();

/**
 * \brief Wait for a signal to occure.
 *
 * waitOnSignal pause the program and continues only after 
 * one of the signals SIGHUP, SIGTERM or SIGINT is receieved. The function 
 * will also return after timeout_in_seconds seconds if no signal is received.
 * if timeout_in_seconds=0 the function will wait until one of the 
 * signals occure.
 *
 * \param timeout_in_seconds specify a timeout value. If 0 no timeout
 *        is used.
 * \param[out] timeout, true on time out, false otherwise. 
 * \return 0 - SIGHUP was received.
 *         1 - SIGINT, SIGTERM or timeout.
 *        -1 - An error occured, for more information look at errno.
 */
int 
waitOnSignal(int timeout_in_seconds, bool &timeout);

/**
 * \brief Wait for fd to be ready for reading.
 *
 * waitOnFifo pause the program and continues only after 
 * one of the signals SIGHUP, SIGTERM or the filedescriptor fd is ready
 * for reading and one caharcter is read. The function 
 * will also return after timeout_in_seconds seconds if nothing has happend.
 * If timeout_in_seconds=0 no timeout logic is in effect.
 *
 * \param fd a fildescriptor to wait on.
 * \param timeout_in_seconds specify a timeout value. If 0 no timeout
 *        is used.
 * \param[out] timeout, true on time out, false otherwise. 
 * \return 0 - A successful read on fd has happend.
 *         1 - SIGINT, SIGTERM or timeout.
 *        -1 - An error occured, for more information look at errno.
 */

int
waitOnFifo(int fd, int timeout_in_seconds, bool &timeout);

/**
 * \brief Check if we shall exit.
 * 
 * \return true if one either the signal SIGINT or SIGTERM is received, and
 *         false otherwise.
 */
bool
signalQuit();

#endif
