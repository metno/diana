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

#ifndef __ORDER_BOOK__
#define __ORDER_BOOK__

#include <string>
#include <vector>

class OrderBook {
public:
	~OrderBook();

	// listen for orders on the specified address and port
	// listen(NULL, DEFAULT_PORT) will listen on all available interfaces
	// can be called repeatedly for additional addresses / ports
	void addListenAddress(const char *addr = NULL, unsigned int port = DEFAULT_PORT);
	void addListenAddress(const char *addr, const char *port);

	// wait indefinitely for an order to come in
	std::string getNextOrder();

	// as above, give up after <timeout> seconds
	std::string getNextOrder(int timeout);

	// as above, using struct timeval
	std::string getNextOrder(struct timeval *tv);

	// default TCP port to listen on (3190)
	static const int DEFAULT_PORT;

private:
	std::vector<int> sockets;
};

#endif
