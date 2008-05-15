/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: qtMainWindow.h 477 2008-05-06 09:53:22Z lisbethb $

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
#include <sys/stat.h>

#include "MovieMaker.h"

MovieMaker::MovieMaker(string &filename, int quality, int delay) {
	this->filename = filename;
	this->quality = quality;
	this->delay = delay;
}

void MovieMaker::addFrame(string &frameName) {
	frameNames.push_back(frameName);
}
	
void MovieMaker::make() {
	for(int i=0; i < frameNames.size(); ++i) {
		cout << "Adding frame " << frameNames [i] << " to animation.." << endl;	
		readImages(&frames, frameNames[i]);
		frames[i].quality(quality);
		frames[i].resolutionUnits(PixelsPerInchResolution);
		frames[i].density("500x500"); ///< could/should be made adjustable..
		frames[i].animationDelay(delay);
	}
	
	writeImages(frames.begin(), frames.end(), filename);
}

void MovieMaker::cleanup() {
	for(int i=0; i < frameNames.size(); ++i) {
		unlink(frameNames[i].c_str());
	}
}
