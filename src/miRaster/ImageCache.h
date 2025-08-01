/*
  ImageCache

  Copyright (C) 2006-2019 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * PURPOSE:
 * Header file for ImageCache.
 *
 * AUTHOR:
 * Stefan Fagerstrøm, stefan.fagerstrom@smhi.se, 10/09/2009
 */

#ifndef _IMAGECACHE_H
#define	_IMAGECACHE_H

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class ImageCache
{
private:
  struct Item {
    uint8_t* data;
    int length;
    time_t time_stamp;
  };

  volatile bool stop;
  std::string cacheFilePath;
  std::map<std::string, Item> mCache;
  bool useMem;
  std::shared_ptr<std::thread> m_thread;
  std::mutex m_mutex;
  std::condition_variable cond;
  static bool init;
  static ImageCache* instance;

  ImageCache(std::string path);
  void cleanCache();

  bool getFromMemCache(const std::string& fileName, uint8_t* image);
  bool getFromFileCache(const std::string& fileName, uint8_t* image);

  bool putInMemCache(const std::string& fileName, uint8_t* data, int length);
  bool putInFileCache(const std::string& fileName, uint8_t* data, int length);


public:
  ~ImageCache();

  /** Set cacheFilPath    */
  void setCacheFilePath(const std::string& path);

  /** Read file from cache */
  bool getFromCache(const std::string& fileName, uint8_t* image);

  /** Put file in cache. */
  bool putInCache(const std::string& fileName, uint8_t* data, int length);

  static ImageCache* getInstance();
};

#endif /* _IMAGECACHE_H */
