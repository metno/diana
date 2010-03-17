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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <netdb.h>

#include <iostream>
#include <stdexcept>

#include "orderbook.h"

const int OrderBook::DEFAULT_PORT = 3190;

OrderBook::~OrderBook()
{

	while (!sockets.empty()) {
		close(sockets.back());
		sockets.pop_back();
	}
}

void
OrderBook::addListenAddress(const char *addr, unsigned int port)
{
	char buf[6];

	if (port > 65535)
		throw std::invalid_argument("invalid port number");
	snprintf(buf, sizeof buf, "%d", port);
	addListenAddress(addr, buf);
}

void
OrderBook::addListenAddress(const char *addr, const char *port)
{
	struct addrinfo hints;
	struct addrinfo *ais, *ai;
	int ret, sd, serrno;
#ifdef IPV6_V6ONLY
	int v6only = 1;
#endif

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME | AI_PASSIVE;
	ret = getaddrinfo(addr, port, &hints, &ais);
	if (ret != 0) {
		std::cerr << "getaddrinfo(): " << addr <<
		    ": " << gai_strerror(ret) << std::endl;
		throw std::runtime_error(gai_strerror(ret));
	}
	for (ai = ais; ai != NULL; ai = ai->ai_next) {
		sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sd == -1) {
			serrno = errno;
			std::cerr << "socket(): " << ai->ai_canonname <<
			    ": " << strerror(errno) << std::endl;
			continue;
		}
#ifdef IPV6_V6ONLY
		// defaults to 1 on BSD, 0 on Linux; we want 1
		if (ai->ai_family == AF_INET6) {
			ret = setsockopt(sd, IPPROTO_IPV6, IPV6_V6ONLY,
			    &v6only, sizeof v6only);
			if (ret != 0) {
				serrno = errno;
				std::cerr << "setsockopt(): " << ai->ai_canonname <<
				    ": " << strerror(serrno) << std::endl;
				close(sd);
				continue;
			}
		}
#endif
		if (bind(sd, ai->ai_addr, ai->ai_addrlen) != 0) {
			serrno = errno;
			std::cerr << "bind(): " << ai->ai_canonname <<
			    ": " << strerror(serrno) << std::endl;
			close(sd);
			continue;
		}
		if (listen(sd, SOMAXCONN) != 0) {
			serrno = errno;
			std::cerr << "listen(): " << ai->ai_canonname <<
			    ": " << strerror(serrno) << std::endl;
			close(sd);
			continue;
		}
		std::cerr << "listening on " << ai->ai_canonname << std::endl;
		sockets.push_back(sd);
	}
	freeaddrinfo(ais);
}

std::string
OrderBook::getNextOrder()
{

	return getNextOrder((struct timeval *)NULL);
}

std::string
OrderBook::getNextOrder(int timeout)
{
	struct timeval tv;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	return getNextOrder(&tv);
}

std::string
OrderBook::getNextOrder(struct timeval *timeout)
{
	std::vector<int>::iterator it;
	fd_set fds;
	int nfds, serrno;

	FD_ZERO(&fds);
	nfds = 0;
	for (it = sockets.begin(); it != sockets.end(); it++) {
		FD_SET(*it, &fds);
		if (*it >= nfds)
			nfds = *it + 1;
	}
	if (select(nfds, &fds, NULL, NULL, timeout) < 0) {
		serrno = errno;
		std::cerr << "select(): " << strerror(serrno) << std::endl;
		return ""; // XXX
	}
	for (it = sockets.begin(); it != sockets.end(); it++) {
		if (FD_ISSET(*it, &fds)) {
			struct sockaddr_storage ss;
			struct sockaddr *sa = (struct sockaddr *)&ss;
			socklen_t sa_len = sizeof ss;
			int sd = accept(*it, sa, &sa_len);
			// receive and process work order
			close(sd);
		}
	}

}
