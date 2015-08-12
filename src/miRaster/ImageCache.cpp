
#include "ImageCache.h"
#include <iostream>
#include <fstream>

#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <boost/bind.hpp>

#include <puTools/miStringFunctions.h>

using namespace std;

typedef boost::mutex::scoped_lock scoped_lock;

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
  m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&ImageCache::cleanCache, this)));

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
  if (mCache.count(fileName) > 0) {
    memcpy(image, mCache[fileName].data, mCache[fileName].length);
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
					cerr << "Cannot open: " << cacheFilePath << endl;
					//return false;
					return;
				} else {
					std::string fullPath;
					while ((dirp = readdir(dp)) != NULL) {
						//cerr << dirp->d_name << endl;
						fullPath = cacheFilePath + std::string("/") + std::string(dirp->d_name);
						if (!miutil::contains(fullPath, ".cache"))
							continue;
						if (stat(fullPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
#ifdef DEBUGPRINT
							cerr << fullPath << " atim: " << st.st_atime
								<< " mtime: " << st.st_mtime <<" ctime: " << st.st_ctime
								<< " mode: " << st.st_mode << endl;
#endif
							if (time(NULL) - st.st_atime > keepTime) {
								cerr << "file: " << fullPath << " is older than "
									<< keepTime << " seconds, removing" << endl;
								if (remove(fullPath.c_str()) != 0)
									cerr << "Error deleting file" << endl;
							}
						} else
							if(errno == EACCES)
								cerr <<"EACCES"<< endl;
							else if (errno == EBADF)
								cerr <<"EBADF"<< endl;
							else if (errno == EFAULT)
								cerr <<"EFAULT"<< endl;
							else if (errno == ENOENT)
								cerr <<"ENOENT"<< endl;
							else if (errno == ENOTDIR)
								cerr <<"ENOTDIR"<< endl;
							else
								cerr <<"Unknown"<< endl;
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

    ifstream in(file.c_str(), ios::in | ios::binary);

    if (!in) {
#ifdef DEBUGPRINT
      cerr << "Cannot open input file: " << file << endl;
#endif
      return false;
    } else {
#ifdef DEBUGPRINT
      cerr << "File: " << file << endl;
#endif
      try {
        int length;

        in.seekg(0,ios::end);
        length = in.tellg();
        in.seekg(0, ios::beg);
#ifdef DEBUGPRINT
        cerr << "length of data in file: " << length << endl;
#endif
        in.read((char*)image, length);
      } catch (exception& e) {
        cerr << "exception caught: " << e.what() << endl;
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
  cerr << fileName << " length: " << length << endl;
#endif
  // If cacheFilePath is set, save a copy of the image
  if (cacheFilePath != "") {
    // Set everything created to 0666
    umask (0);
    struct stat st;
    if (stat(cacheFilePath.c_str(), &st) != 0) {
      // Create the temporary directory if it's not there
      vector<std::string> filePathParts = miutil::split(cacheFilePath,"/", true);
      std::string realFilePath;
      for(size_t j=0; j < filePathParts.size();j++) {
        realFilePath += "/" + filePathParts[j];
        if (stat(realFilePath.c_str(), &st) != 0) {
          cerr << "Creating directory: " << realFilePath;
#ifdef __MINGW32__
          if (mkdir(realFilePath.c_str()) != 0)
#else
            if (mkdir(realFilePath.c_str(), 0777) != 0)
#endif
              cerr << " ERROR" << endl;
            else
              cerr << " SUCCESS" << endl;
        }
      }
    }
  }
  std::string file = cacheFilePath + std::string("/") + fileName + ".cache";
  ofstream out(file.c_str(), ios::out | ios::binary);

  if (!out) {
#ifdef DEBUGPRINT
    cerr << "Cannot open file.\n";
#endif
    return false;
  } else {
    out.write((const char*)data, length);
    if (out.fail()) {
      out.close();
      if (remove(file.c_str()) != 0)
        cerr << "Error deleting file" << endl;
      return false;
    }
    else
      out.close();
  }
  return true;
}
