/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2021 met.no

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

#include "ImageCache.h"
#include <iostream>
#include <fstream>

#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <puTools/miStringFunctions.h>

#include <memory>
#include <mutex>
#include <thread>

namespace {
typedef std::unique_lock<std::mutex> scoped_lock;
} // namespace

bool ImageCache::init = false;
ImageCache* ImageCache::instance = NULL;

//#define DEBUGPRINT

ImageCache::ImageCache(std::string path) :
  stop(false), cacheFilePath(path), useMem(false)
{
  if (getenv("DIANA_TMP") != NULL) {
    cacheFilePath = getenv("DIANA_TMP");
  }
  if (getenv("DIANA_TMP_MEM") != NULL) {
    useMem = true;
  }
  m_thread = std::make_shared<std::thread>(&ImageCache::cleanCache, this);
}

ImageCache::~ImageCache()
{
  stop = true;
  cond.notify_one();

  m_thread->join();
}

ImageCache* ImageCache::getInstance()
{
  if (!init) {
    instance = new ImageCache("");
    init = true;
  }
  return instance;
}

void ImageCache::setCacheFilePath(const std::string& path)
{
    cacheFilePath = path;
}


bool ImageCache::getFromCache(const std::string& fileName, uint8_t* image)
{
  if (useMem) {
    return getFromMemCache(fileName, image);
  }
  else {
    return getFromFileCache(fileName, image);
  }
}


bool ImageCache::getFromMemCache(const std::string& fileName, uint8_t* image)
{
  const auto it = mCache.find(fileName);
  if (it != mCache.end()) {
    memcpy(image, it->second.data, it->second.length);
    return true;
  }
  return false;
}

void ImageCache::cleanCache()
{
	// Get keeptime, if not found set it to 1 day
	time_t keepTime = 3600*24;
	while (!stop) {
		// If cacheFilePath is set, look for a cached copy of the image
		if (!useMem){
			if (cacheFilePath != "") {
				/* Remove cached files older than a certain time
				* specified in metadataMap["cacheFileKeepTime"]
				*/
				struct stat st;

				DIR *dp;
				struct dirent *dirp;
				if((dp  = opendir(cacheFilePath.c_str())) == NULL) {
					std::cerr << "Cannot open: " << cacheFilePath << std::endl;
					//return false;
					return;
				} else {
					std::string fullPath;
					while ((dirp = readdir(dp)) != NULL) {
						//std::cerr << dirp->d_name << std::endl;
						fullPath = cacheFilePath + std::string("/") + std::string(dirp->d_name);
						if (!miutil::contains(fullPath, ".cache"))
							continue;
						if (stat(fullPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
#ifdef DEBUGPRINT
							std::cerr << fullPath << " atim: " << st.st_atime
								<< " mtime: " << st.st_mtime <<" ctime: " << st.st_ctime
								<< " mode: " << st.st_mode << std::endl;
#endif
							if (time(NULL) - st.st_atime > keepTime) {
								std::cerr << "file: " << fullPath << " is older than "
									<< keepTime << " seconds, removing" << std::endl;
								if (remove(fullPath.c_str()) != 0)
									std::cerr << "Error deleting file" << std::endl;
							}
						} else
							if(errno == EACCES)
								std::cerr <<"EACCES"<< std::endl;
							else if (errno == EBADF)
								std::cerr <<"EBADF"<< std::endl;
							else if (errno == EFAULT)
								std::cerr <<"EFAULT"<< std::endl;
							else if (errno == ENOENT)
								std::cerr <<"ENOENT"<< std::endl;
							else if (errno == ENOTDIR)
								std::cerr <<"ENOTDIR"<< std::endl;
							else
								std::cerr <<"Unknown"<< std::endl;
					}
				}
				closedir(dp);
			}
		}
		else //useMem
		{
			//Check the map
			time_t now = time(NULL);
			std::map<std::string, Item>::iterator it = mCache.begin();
			for (; it != mCache.end(); it++)
			{
				if (now - it->second.time_stamp > keepTime)
				{
					//remove the image from cache...
					if(it->second.data != NULL)
					{
						free(it->second.data);
						it->second.data = NULL;
					}
					mCache.erase(it);
				}
			}
		}
		// Wait for cond
		{
			scoped_lock lock(m_mutex);

			cond.wait(lock);
		}
	}
}

bool ImageCache::getFromFileCache(const std::string& fileName, uint8_t* image)
{
    if (cacheFilePath == "") return false;

	std::string file = cacheFilePath + std::string("/") + fileName + ".cache";

    std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);

    if (!in) {
#ifdef DEBUGPRINT
      std::cerr << "Cannot open input file: " << file << std::endl;
#endif
      return false;
    } else {
#ifdef DEBUGPRINT
      std::cerr << "File: " << file << std::endl;
#endif
      try {
        int length;

        in.seekg(0, std::ios::end);
        length = in.tellg();
        in.seekg(0, std::ios::beg);
#ifdef DEBUGPRINT
        std::cerr << "length of data in file: " << length << std::endl;
#endif
        in.read((char*)image, length);
      } catch (std::exception& e) {
        std::cerr << "exception caught: " << e.what() << std::endl;
      }
      in.close();
    }
    return true;
}


bool ImageCache::putInCache(const std::string& fileName, uint8_t* data, int length)
{
  if (useMem) {
    return putInMemCache(fileName, data, length);
  }
  else {
    return putInFileCache(fileName, data, length);
  }
  // Call cleanup in cache
  cond.notify_one();

}

bool ImageCache::putInMemCache(const std::string& fileName, uint8_t* data, int length)
{

  Item new_image;
  new_image.data = (uint8_t*)malloc(length);
  new_image.length = length;
  new_image.time_stamp = time(NULL);
  memcpy(new_image.data, data, length);
  mCache[fileName] = new_image;
  return true;
}


bool ImageCache::putInFileCache(const std::string& fileName, uint8_t* data, int length)
{

#ifdef DEBUGPRINT
  std::cerr << fileName << " length: " << length << std::endl;
#endif
  // If cacheFilePath is set, save a copy of the image
  if (cacheFilePath != "") {
    // Set everything created to 0666
    umask (0);
    struct stat st;
    if (stat(cacheFilePath.c_str(), &st) != 0) {
      // Create the temporary directory if it's not there
      std::vector<std::string> filePathParts = miutil::split(cacheFilePath,"/", true);
      std::string realFilePath;
      for(size_t j=0; j < filePathParts.size();j++) {
        realFilePath += "/" + filePathParts[j];
        if (stat(realFilePath.c_str(), &st) != 0) {
          std::cerr << "Creating directory: " << realFilePath;
#ifdef __MINGW32__
          if (mkdir(realFilePath.c_str()) != 0)
#else
            if (mkdir(realFilePath.c_str(), 0777) != 0)
#endif
              std::cerr << " ERROR" << std::endl;
            else
              std::cerr << " SUCCESS" << std::endl;
        }
      }
    }
  }
  std::string file = cacheFilePath + std::string("/") + fileName + ".cache";
  std::ofstream out(file.c_str(), std::ios::out | std::ios::binary);

  if (!out) {
#ifdef DEBUGPRINT
    std::cerr << "Cannot open file.\n";
#endif
    return false;
  } else {
    out.write((const char*)data, length);
    if (out.fail()) {
      out.close();
      if (remove(file.c_str()) != 0)
        std::cerr << "Error deleting file" << std::endl;
      return false;
    }
    else
      out.close();
  }
  return true;
}
