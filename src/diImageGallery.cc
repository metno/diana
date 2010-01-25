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
#include <diImageGallery.h>
#include <diImageIO.h>
#include <fstream>
#include <puCtools/glob.h>
#include <GL/gl.h>

using namespace::miutil;

//int ImageGallery::numimages= 0;
//ImageGallery::image ImageGallery::Images[ImageGallery::maximages];
map<miString,ImageGallery::image> ImageGallery::Images;
map<miString,ImageGallery::pattern> ImageGallery::Patterns;
map<int, vector<miString> > ImageGallery::Type;


ImageGallery::image::image()
:alpha(true),width(0),height(0),data(0),type(basic),
read_error(false)
{
}

ImageGallery::image::~image()
{
  erase();
}

void ImageGallery::image::erase()
{
  name.clear();
  width=  0;
  height= 0;
  alpha= true;
  if (data!=0) delete[] data;
  data= 0;
  type= basic;
  read_error= false;
}

ImageGallery::pattern::pattern()
:pattern_data(0),read_error(false)
{
}

ImageGallery::pattern::~pattern()
{
  erase();
}

void ImageGallery::pattern::erase()
{
  name.clear();
  if (pattern_data!=0) delete[] pattern_data;
  pattern_data= 0;
  read_error= false;
}

// -------------- IMAGEGALLERY -------------------------------

ImageGallery::ImageGallery()
: Plot()
{
}


void ImageGallery::clear()
{
  map<miString,image>::iterator p= Images.begin();
  for( ; p!=Images.end(); p++){
    p->second.erase();
    Images.erase(p->first);
  }
  map<miString,pattern>::iterator q= Patterns.begin();
  for( ; q!=Patterns.end(); q++){
    q->second.erase();
    Patterns.erase(q->first);
  }
}


void ImageGallery::addImageName(const miString& filename, int type)
{

  int n = filename.find_last_of("/");
  int m = filename.find_last_of(".");
  miString name = filename.substr(n+1,m-n-1);

  if(type == fillpattern){
    if(Patterns.count(name)>0)
      Patterns[name].erase();

    Patterns[name].name=name;
    Patterns[name].filename=filename;
    Type[type].push_back(name);
    return;
  }

  // images, markers ..
  if(Images.count(name)>0)
    Images[name].erase();

  Images[name].name=name;
  Images[name].filename=filename;
  Images[name].type=type;
  Type[type].push_back(name);

  //marker
  if(type == marker){
    glob_t globBuf;
    miString markerFilename = filename;
    if(filename.contains("png"))
      markerFilename.replace("png","txt");
    else if(filename.contains("xpm"))
      markerFilename.replace("xpm","txt");
    glob(markerFilename.c_str(),0,0,&globBuf);
    if(globBuf.gl_pathc == 1){
      Images[name].markerFilename=markerFilename;
    } else {
      Images[name].type = basic;
    }
    globfree(&globBuf);
  }
}

bool ImageGallery::readImage(const miString& name)
{
  if(Images[name].data != 0)
    return true; //Image ok

  // if read_error == true, we have tried and failed before
  // Should we try again anyway?
  if (Images[name].read_error)
    return false;

  //read image from file
  imageIO::Image_data img(Images[name].filename);
  img.pattern = false;
  if( !imageIO::read_image(img)){
    if (!Images[name].read_error)
      cerr << "ImageGallery::readImage ERROR couldn't read image:"<<name<<endl;
    Images[name].read_error= true;
    return false;
  }

  return addImage(name,img.width,img.height,
      img.data,img.nchannels>3);
}

bool ImageGallery::readPattern(const miString& name)
{

  if( Patterns[name].pattern_data!=0)
    return true; //Pattern ok

  // if read_error == true, we have tried and failed before
  // Should we try again anyway?
  if (Patterns[name].read_error)
    return false;

  //read image from file
  imageIO::Image_data img(Patterns[name].filename);
  img.pattern = true;
  if( !imageIO::read_image(img)){
    if (!Patterns[name].read_error)
      cerr << "ImageGallery::readImage ERROR couldn't read image:"<<name<<endl;
    Patterns[name].read_error= true;
    return false;
  }

  return addPattern(name,img.data);

}

bool ImageGallery::addImage(const image& im)
{
  return addImage(im.name,im.width,im.height,im.data,im.alpha);
}

bool ImageGallery::addImage(const miString& name,
    const int w,
    const int h,
    const unsigned char* d,
    const bool a)
{

  int size= w*h;

  if (!name.exists()){
    cerr << "ImageGallery::addImage ERROR trying to add image with no name"
    << endl;
    return false;
  }
  if (size == 0){
    cerr << "ImageGallery::addImage ERROR trying to add image with zero width/height:"
    << name << endl;
    return false;
  }
  if (d==0){
    cerr << "ImageGallery::addImage ERROR trying to add image with no imagedata:"
    << name << endl;
    return false;
  }

  // add image
  Images[name].name=   name;
  Images[name].width=  w;
  Images[name].height= h;
  Images[name].alpha=  a;
  int fsize = size * (a ? 4 : 3);
  Images[name].data= new unsigned char [fsize];
  for (int j=0; j<fsize; j++)
    Images[name].data[j]= d[j];

  if(Images[name].type==marker)
    if(!readFile(name,Images[name].markerFilename))
      Images[name].type = basic;

  return true;
}

bool ImageGallery::addPattern(const miString& name,
    const unsigned char* d)
{


  if (!name.exists()){
    cerr << "ImageGallery::addPattern ERROR trying to add image with no name"
    << endl;
    return false;
  }
  if (d==0){
    cerr << "ImageGallery::addPattern ERROR trying to add image with no data:"
    << name << endl;
    return false;
  }

  // add pattern
  Patterns[name].name=   name;
  Patterns[name].pattern_data= new unsigned char [128];
  for (int j=0; j<128; j++)
    Patterns[name].pattern_data[j]= (GLubyte)d[j];

  return true;
}

float ImageGallery::width(const miString& name)
{
  float w= 0.0;
  if (!Images.count(name)){
    cerr << "ImageGallery::width ERROR image not found:"
    << name << endl;
  } else {
    readImage(name);
    w= Images[name].width*fullrect.width()/(pwidth > 0 ? pwidth*1.0 : 1.0);;
  }
  return w;
}

float ImageGallery::height(const miString& name)
{
  float h= 0.0;
  if (!Images.count(name)){
    cerr << "ImageGallery::height ERROR image not found:"
    << name << endl;
  } else {
    readImage(name);
    h= Images[name].height*fullrect.height()/(pheight > 0 ? pheight*1.0 : 1.0);
  }
  return h;
}

int ImageGallery::widthp(const miString& name)
{
  int w= 0;
  if (!Images.count(name)){
    cerr << "ImageGallery::pwidth ERROR image not found:"
    << name << endl;
  } else {
    readImage(name);
    if( Images[name].type == marker ){
      float max=0.0,min=0.0;
      for(unsigned int i=0;i<Images[name].line.size();i++)
        for(unsigned int j=0;j<Images[name].line[i].x.size();j++)
          if(Images[name].line[i].x[j]>max)
            max = Images[name].line[i].x[j];
          else if(Images[name].line[i].y[j]<min)
            min = Images[name].line[i].y[j];
      w=(int)(max-min);
    } else {
      w= Images[name].width;
    }
  }
  return w;
}

int ImageGallery::heightp(const miString& name)
{
  int h= 0;
  if (!Images.count(name)){
    cerr << "ImageGallery::pheight ERROR image not found:"
    << name << endl;
  } else {
    readImage(name);
    if( Images[name].type == marker ){
      float max=0.0,min=0.0;
      for(unsigned int i=0;i<Images[name].line.size();i++)
        for(unsigned int j=0;j<Images[name].line[i].y.size();j++)
          if(Images[name].line[i].y[j]>max)
            max = Images[name].line[i].y[j];
          else if(Images[name].line[i].y[j]<min)
            min = Images[name].line[i].y[j];
      h=int(max-min);
    } else {
      h= Images[name].height;
    }
  }
  return h;
}

bool ImageGallery::delImage(const miString& name)
{
  if (!Images.count(name)){
    cerr << "ImageGallery::delImage ERROR image not found:"
    << name << endl;
    return false;
  }
  Images[name].erase();
  return true;
}

bool ImageGallery::delPattern(const miString& name)
{
  if (!Patterns.count(name)){
    cerr << "ImageGallery::delPattern ERROR pattern not found:"
    << name << endl;
    return false;
  }
  Patterns[name].erase();
  return true;
}


bool ImageGallery::plotImage_(const miString name,
    const float& gx, const float& gy,
    const float scalex,
    const float scaley,
    const int alpha)
{

  if (gx < fullrect.x1 || gx >= fullrect.x2 ||
      gy < fullrect.y1 || gy >= fullrect.y2)
    return true;

  int nx= Images[name].width;
  int ny= Images[name].height;
  GLenum glformat= GL_RGBA;
  int ncomp= 4;

  if (!Images[name].alpha){
    glformat= GL_RGB;
    ncomp= 3;
  }

  glRasterPos2f(gx,gy);

  if (alpha != 255 && ncomp == 4 && !hardcopy) {
    int fsize = nx*ny*ncomp;
    unsigned char* newdata= new unsigned char [fsize];
    unsigned char av= static_cast<unsigned char>(alpha);
    for (int j=0; j<fsize; j++){
      newdata[j]= Images[name].data[j];
      // if original alpha-value lower: keep it
      if ((j+1) % 4 == 0 && newdata[j] > av) newdata[j] = av;
    }

    glDrawPixels((GLint)nx,
        (GLint)ny,
        glformat,
        GL_UNSIGNED_BYTE,
        newdata);

    delete[] newdata;

  } else {
    glDrawPixels((GLint)nx,
        (GLint)ny,
        glformat,
        GL_UNSIGNED_BYTE,
        Images[name].data);
  }

  // for postscript output, add imagedata to glpfile
  if (hardcopy){
    // change to pixel coordinates
    float sx= pwidth*1.0/(fullrect.width() > 0 ? fullrect.width() : 1.0);
    float sy= pheight*1.0/(fullrect.height() > 0 ? fullrect.height() : 1.0);
    float pgx= (gx-fullrect.x1)*sx;
    float pgy= (gy-fullrect.y1)*sy;

    if (!(pgx >= pwidth || pgy >= pheight ||
        // pgx+nx*scalex <= 0.0 || pgy+ny*scaley <= 0.0)){
        pgx <= 0.0 || pgy <= 0.0)){
      psAddImage(Images[name].data,
          ncomp*nx*ny, nx, ny,
          pgx, pgy, scalex, scaley,
          0, 0, nx-1, ny-1,
          glformat, GL_UNSIGNED_BYTE);
      //       psGrabImage(pgx,pgy,nx*scalex,ny*scaley,
      // 		  glformat,GL_UNSIGNED_BYTE);

      // for postscript output
      UpdateOutput();
    }
  }
  return true;
}

bool ImageGallery::plotMarker_(const miString name,
    const float& x, const float& y,
    const float scale)
{
  if (x < fullrect.x1 || x >= fullrect.x2 ||
      y < fullrect.y1 || y >= fullrect.y2)
    return true;

  int nlines=Images[name].line.size();
  if(nlines>0) {
    glPushMatrix();
    glTranslatef(x,y,0.0);
    float Scalex= scale*fullrect.width()/pwidth*0.7;
    float Scaley= scale*fullrect.width()/pwidth*0.7;
    glScalef(Scalex,Scaley,0.0);

    for(int k=0; k<nlines; k++){

      glLineWidth(Images[name].line[k].width);

      int num=Images[name].line[k].x.size();

      if(Images[name].line[k].fill){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBegin(GL_POLYGON);
        for (int j=0; j<num; j++) {
          glVertex2f(Images[name].line[k].x[j],Images[name].line[k].y[j]);
        }
        glEnd();
      }else{
        glBegin(GL_LINE_STRIP);
        for (int j=0; j<num; j++) {
          glVertex2f(Images[name].line[k].x[j],Images[name].line[k].y[j]);
        }
        glEnd();
      }
    }
    glPopMatrix();

  }
  return true;
}

bool ImageGallery::readFile(const miString name, const miString filename)
{
  ifstream inFile;
  miString line;
  vector<miString> vline;

  inFile.open(filename.c_str(),ios::in);
  if (inFile.bad()) {
    cerr << "ImageGallery: Can't open file: " << filename << endl;
    return false;
  }

  while (getline(inFile,line)) {
    if (line.length()>0) {
      line.trim();
      if (line.length()>0 && line[0]!='#')
        vline.push_back(line);
    }
  }

  inFile.close();

  Line l;
  int nlines = vline.size();
  for( int i=0; i<nlines; i++){
    vector<miString> tokens = vline[i].split(" ");
    if( tokens.size() !=2) continue;
    if( (tokens[0] == "mvto" && l.x.size()>0)
        || (tokens[0] == "lw") || tokens[0] == "mode") {
      Images[name].line.push_back(l);
      l.x.clear();
      l.y.clear();
      l.fill=false;
      l.width=1;
    }
    if( tokens[0] == "lto" || tokens[0] == "mvto" ){
      vector<miString> coor = tokens[1].split(",");
      if(coor.size() != 2) continue;
      l.x.push_back(atof(coor[0].c_str()));
      l.y.push_back(atof(coor[1].c_str()));
    } else if( tokens[0] == "lw" ) {
      l.width = atoi(tokens[1].c_str());
    } else if( tokens[0] == "mode" ) {
      if(tokens[1] == "fill")
        l.fill = true;
    }
  }
  if(l.x.size()>0) {
    Images[name].line.push_back(l);
  }

  if(Images[name].line.size() == 0 )
    return false;

  return true;
}

bool ImageGallery::plotImage(const miString& name,
    const float& x, const float& y,
    const bool center,
    const float scale,
    const int alpha)
{
  if(!readImage(name)) return false;

  if (!Images.count(name)){
    cerr << "ImageGallery::plot ERROR image not found:"
    << name << endl;
    return false;
  }

  if(Images[name].type == marker)
    return plotMarker_(name, x, y, scale);

  if (Images[name].data==0) {
    cerr << "ImageGallery::plot ERROR no image-data:"
    << name << endl;
    return false;
  }

  float gx= x, gy= y; // raster position
  int nx= Images[name].width;
  int ny= Images[name].height;
  float scalex=scale, scaley=scale;

  if (center){
    // center image on x,y: find scale
    float sx= fullrect.width()/(pwidth > 0 ? 2.0*pwidth : 2.0);
    float sy= fullrect.height()/(pheight > 0 ? 2.0*pheight : 2.0);
    gx-= nx*sx*scale;
    gy-= ny*sy*scale;
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glPixelZoom(scalex,scaley);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,nx);

  bool res= plotImage_(name, gx, gy, scalex, scaley, alpha);

  //Reset gl
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);
  glDisable(GL_BLEND);

  return res;
}


bool ImageGallery::plotImages(const int n,
    const vector<miString>& vn,
    const float* x, const float* y,
    const bool center,
    const float scale,
    const int alpha)
{

  if (n == 0){
    cerr << "ImageGallery::plotImages ERROR no positions:"
    << endl;
    return false;
  }
  if (n != int(vn.size())){
    cerr << "ImageGallery::plotImages ERROR names and positions do not match:"
    << n << endl;
    return false;
  }

  int nx;
  int ny;
  float scalex=scale, scaley=scale;
  miString oldname;
  float sx= scale*fullrect.width()/(pwidth > 0 ? 2.0*pwidth : 2.0);
  float sy= scale*fullrect.height()/(pheight > 0 ? 2.0*pheight : 2.0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glPixelZoom(scalex,scaley);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);

  for (int j=0; j<n; j++){
    if (!Images.count(vn[j])){
      cerr << "ImageGallery::plotImages ERROR image not found:"
      << vn[j] << endl;
      return false;
    }

    if(!readImage(vn[j])) return false;

    if(Images[vn[j]].type == marker) {
      plotMarker_(vn[j], x[j], y[j], scale);
      continue;
    }

    if (Images[vn[j]].data==0) {
      cerr << "ImageGallery::plotImages ERROR no image-data:"
      << vn[j] << endl;
      return false;
    }

    float gx= x[j], gy= y[j]; // raster position
    if (vn[j] != oldname){
      nx= Images[vn[j]].width;
      ny= Images[vn[j]].height;
      glPixelStorei(GL_UNPACK_ROW_LENGTH,nx);
    }

    if (center){
      // center image on x,y
      gx-= nx*sx;
      gy-= ny*sy;
    }

    plotImage_(vn[j], gx, gy, scalex, scaley, alpha);
    oldname = vn[j];
  }

  //Reset gl
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);
  glDisable(GL_BLEND);

  return true;
}

bool ImageGallery::plotImages(const int n,
    const miString& name,
    const float* x, const float* y,
    const bool center,
    const float scale,
    const int alpha)
{
  if(!readImage(name)) return false;

  if (n == 0){
    cerr << "ImageGallery::plotImages ERROR no positions:"
    << endl;
    return false;
  }

  vector<miString> vn(n,name);

  return plotImages(n, vn, x, y, center, scale, alpha);
}


bool ImageGallery::plotImageAtPixel(const miString& name,
    const float& x, const float& y,
    const bool center,
    const float scale,
    const int alpha)
{
  if(!readImage(name)) return false;

  if (!Images.count(name)){
    cerr << "ImageGallery::plot ERROR image not found:"
    << name << endl;
    return false;
  }

  if(Images[name].type == marker)
    return plotMarker_(name, x, y, scale);

  if (Images[name].data==0) {
    cerr << "ImageGallery::plot ERROR no image-data:"
    << name << endl;
    return false;
  }

  float gx= x, gy= y; // raster position
  int nx= Images[name].width;
  int ny= Images[name].height;
  float scalex=scale, scaley=scale;

  if (center){
    // center image on x,y
    gx-=  nx*scale/2.0;
    gy-=  ny*scale/2.0;
  }

  float sx= fullrect.width()/(pwidth > 0 ? 1.0*pwidth : 1.0);
  float sy= fullrect.height()/(pheight > 0 ? 1.0*pheight : 1.0);
  gx*= sx; gx+= fullrect.x1;
  gy*= sy; gy+= fullrect.y1;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glPixelZoom(scalex,scaley);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,nx);

  bool res= plotImage_(name, gx, gy, scalex, scaley, alpha);

  //Reset gl
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);
  glDisable(GL_BLEND);

  return res;
}

GLubyte* ImageGallery::getPattern(miString name)
{

  if(!readPattern(name)) return 0;
  return Patterns[name].pattern_data;

}

void ImageGallery::printInfo() const
{
  map<miString,image>::const_iterator p= Images.begin();
  for( ; p!=Images.end(); p++){
    cerr << "Image: " << p->second.name
    << " W:" << p->second.width
    << " H:" << p->second.height
    << " A:" << (p->second.alpha ? "YES" : "NO")
    << endl;
  }
}

void ImageGallery::ImageNames(vector<miString>& vnames,
    int type) const
    {
  vnames.clear();

  //   map<miString,image>::const_iterator p= Images.begin();

  //   for (; p!=Images.end(); p++)
  //     vnames.push_back(p->first);
  int n=Type[type].size();
  for(int i=0; i<n; i++)
    vnames.push_back(Type[type][i]);
    }

miString ImageGallery::getFilename(const miString& name, bool pattern)
{
  if(pattern)
    return Patterns[name].filename;

  //images, markers ...
  return Images[name].filename;
}

bool ImageGallery::parseSetup(SetupParser &sp)
{
  //  cerr << "ImageGallery: parseSetup"<<endl;
  const miString ig_name = "IMAGE_GALLERY";
  vector<miString> sect_ig;

  if (!sp.getSection(ig_name,sect_ig)){
    cerr << ig_name << " section not found" << endl;
    return false;
  }

  for(unsigned int i=0; i<sect_ig.size(); i++) {
    vector<miString> token = sect_ig[i].split("=");

    if(token.size() != 2){
      miString errmsg="Line must contain '='";
      sp.errorMsg(ig_name,i,errmsg);
      return false;
    }

    miString key = token[0].downcase();
    miString value = token[1];
    //      cerr <<"key: "<<key<<endl;
    //      cerr <<"Value: "<<value<<endl;
    if(key.contains("path")){
      key.replace("path","");
      value += "/*";
    }
    int type;
    if     (key=="basic")   type = basic;
    else if(key=="pattern") type = fillpattern;
    else if(key=="marker")  type = marker;
    else continue;
    glob_t globBuf;
    glob(value.c_str(),0,0,&globBuf);
    for( unsigned int k=0; k<globBuf.gl_pathc; k++) {
      miString fname = globBuf.gl_pathv[k];
      if((fname.contains(".png") || fname.contains(".xpm"))
          && !fname.contains("~"))
        addImageName(fname,type);
    }
    globfree(&globBuf);
  }



  return true;
}

