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
#ifndef ObjectPlot_h
#define ObjectPlot_h

#include <vector>
#include <deque>
#include <iostream>
#include <diPlot.h>
#include <puTools/miString.h>
#include <diCommonTypes.h>
#include <diField/diColour.h>
#include <diObjectPoint.h>
#include <diField/diLinetype.h>

using namespace std;



enum objectType{Anything,wFront,wSymbol,wArea,wText,Border,RegionName,ShapeXXX};

enum frontType{ Cold,Warm,Occluded,ColdOccluded, WarmOccluded,Stationary,TroughLine,
          TroughLine2,ShortDashedLine,LongDashedLine,Jetstream,SquallLine, SigweatherFront,
          BlackSharpLine,BlackSmoothLine,RedSharpLine,RedSmoothLine,AshLineRed,AshLineGreen,AshLineBlue};

enum AreaType{ Rain, Rainarea, Showers, Clouds, Fog, Ice, Sigweather,ReducedVisibility,Genericarea};

//spline points for fronts/areas/borders
const int divSpline= 5;

/**

  \brief plotting and editing weather objects (symbols, fronts and areas)

  this class holds one weather object

*/
class ObjectPlot : public Plot {
public:

  bool addTop; ///< add elements to top instead of bottom
  bool test;   ///< show area for marking points

  /// returns true if markers changed after cursor move (in editCheckPosition)
  bool getMarkedChanged() {return markedChanged;}
  /// sets markedChanged to c
  void setMarkedChanged(bool c) {markedChanged=c;}
  // returns true if object is joined and marked
  bool getJoinedMarked() {return joinedMarked;}
  /// sets  joinedMarked to j
  void setJoinedMarked(bool j){joinedMarked=j;}
  /// returns true if object to stay marked when cursor moved
  bool getStayMarked(){return stayMarked;}
  /// sets stayMarked to s
  void setStayMarked(bool s){stayMarked=s;}
  /// sets inBoundBox variable to in
  void setInBoundBox(bool in){inBoundBox=in;}

  enum state {active, passive, locked}; // this controlles the state of the points
				  // if active addpoints is legal
				  // if passive no points could be added
				  // if locked no changes can be made




  /// translate norwegian->english in old files
  static void defineTranslations();

private:

  float window_dw;  // scale of main window
  float window_dh;
  float w, h;
  miutil::miString name;
  miutil::miString basisColor;   // Object basis color if any;

  void initVariables();

  //translate norwegian->english in old files
  static map <miutil::miString,miutil::miString> editTranslations;

  // Copy members
  void memberCopy(const ObjectPlot &rhs);

  void drawTest();
  void drawPoints(vector <float> xdraw, vector <float> ydraw);
  bool isInsideBox(float x, float y,float x1,float y1,float x2,float y2);
  void  setRotation(float r){rotation=r;}
  miutil::miString region; //from which region (i.e. VA,VV,VNN)

protected:
  bool rubber;
  bool spline;                    // draw spline
  float rubberx,rubbery;
  bool inBoundBox;
  int type;
  int typeOfObject;
  int drawIndex;
  static int siglinewidth;
  bool stayMarked;    // object stays marked if cursor is moved
  bool joinedMarked;  // object is joined and marked
  bool markedChanged; // markers changed in editCheckPosition
  bool isVisible;        // is the object to be drawn on screen ?
  bool isSelected;       // is the object to be drawn on screen ?
  float rotation;        // Rotation around center of object;
  Colour objectColour;           // colour used
  Linetype itsLinetype;  // linetype used
  int currentState;              // holds current state
  float fSense;

  Rectangle boundBox; // smallest boundingbox;

  deque <ObjectPoint> nodePoints;
  void changeBoundBox(float x, float y);
  //fronts, areas..
  float *x,*y,*x_s,*y_s; // arrays for holding smooth line
  int s_length;          // nr of smooth line points
  int insert;

  float distX,distY; //cursor distance to line

  float scaleToField;

  void setWindowInfo();
  int smoothline(int npos, float x[], float y[],
		 int nfirst, int nlast,
                 int ismooth, float xsmooth[],
		 float ysmooth[]); // B-spline smooth

  virtual void recalculate();
  void plotRubber();
  float getDwidth(){return  window_dw;}           // returns width of main window
  float getDheight(){return  window_dh;}          // returns height of main window
  virtual void setType(int ty){type = ty;}
  virtual bool setType(miutil::miString tystring){return false;}
  virtual void setIndex(int index){drawIndex=index;}

public:
  /// default constructor
  ObjectPlot();
  /// constructor taking type of object as argument
  ObjectPlot(int objTy);
  /// copy constructor
  ObjectPlot(const ObjectPlot &rhs);
  /// Destructor
  virtual ~ObjectPlot();

  /// Assignment operator
  ObjectPlot& operator=(const ObjectPlot &rhs);
  /// returns true if typeOfObject equals Obtype
  bool objectIs(int Obtype){return (typeOfObject ==Obtype);}
  virtual bool plot(){return false;}
  virtual bool plot(const int){return false; }
  miutil::miString getBasisColor() {return basisColor;}   ///< gets basis color of object
  void   setBasisColor(miutil::miString);                 ///< sets basis color of object
  void   setObjectColor(miutil::miString);                /// < sets actual color of object
  void   setObjectColor(Colour::ColourInfo);      ///< sets actual color of object
  void   setObjectRGBColor(miutil::miString);             /// < sets actual color of object from rgb
  /// set alpha value of object colour
  void   setColorAlpha(int alpha){ objectColour.set(Colour::alpha,alpha);}
  Colour::ColourInfo getObjectColor();            ///< gets actual colour of object

  void updateBoundBox();                          ///< finds the new bound box
  Rectangle getBoundBox(){return boundBox;}       ///< returns bound box
  virtual void addPoint(float,float);             ///< adds new point
  /// marks node points close to x,y returns true if sucsess
  bool markPoint(float x,float y);
  void markAllPoints();                           ///< marks all points on this object
  void unmarkAllPoints();                         ///< unmarks all points on this object
  bool deleteMarkPoints();                        ///< deletes all marked points on object
  bool ismarkPoint(float,float);                  ///< returns true if point marked
  bool ismarkAllPoints();                         ///< returns true if all points marked
  bool ismarkSomePoint();                         ///< returns true if some points marked
  bool ismarkEndPoint();                          ///< returns true if end point marked
  bool ismarkBeginPoint();                        ///< returns true if begin point marked
  bool ismarkJoinPoint();                         ///< returns true if joined points marked
  /// makes join point when joining fronts
  bool joinPoint(float,float);
  bool isJoinPoint(float,float,float&,float&);    ///< returns true if joined point
  bool isJoinPoint(float,float);                  ///< returns true if joined point
  void unjoinAllPoints();                         ///< unjoins all points
  void unJoinPoint(float,float);                  ///< unmarks point inside rect at x.y
  virtual bool isEmpty();                         ///< returns true if object contains no points
  bool isSinglePoint();                           ///< returns true if object contain only one point
  /// moves point from x,y to new_x,new_y
  bool movePoint(float x,float y,float new_x,float new_y);
  bool moveMarkedPoints(float,float);             ///< moves marked points d_x d_y
  bool rotateLine(float,float);                   ///< "rotates" front y d_x d_y
  /// returns true if any node points in rectangle around x,y
  bool isInside(float x,float y);
  /// returns true if any node points in rectangle around x,y point values in xin,yin
  bool isInside(float x,float y,float& xin,float& yin);
  /// returns true if begin point in rectangle around x,y point values returned in xin,yin
  bool isBeginPoint(float x,float y,float& xin,float& yin);
  /// returns true if end point in rectangle around x,y point values returned in xin,yin
  bool isEndPoint(float d,float y,float& xin,float& yin);
  int endPoint(){return nodePoints.size()-1;}     ///< returns index to end point
  bool readObjectString(miutil::miString objectString);   ///< reads string with object type, coordinates, colour
  miutil::miString writeObjectString();                   ///< writes string with object type, coordinates, colour
  void drawNodePoints();                          ///< draws all the node points
  void drawJoinPoints();                          ///< draws all the join points

  bool isInRegion(int region, int matrix_x, int matrix_y, double resx, double resy, int *);  ///< returns true if object inside geographical region
  int combIndex(int matrix_nx, int matrix_ny, double resx ,double resy, int *);             ///< returns index of combinematrix
  void setScaleToField(float s) { scaleToField= s; }

  virtual void setState(const state s) {currentState= s;}     ///< sets current state of object to s
  virtual bool isOnObject(float x,float y) {return false;}    ///< returns true if x,y is on object
  virtual bool resumeDrawing();                               ///< resume drawing of marked front or insert point
  virtual bool insertPoint(float x,float y);                  ///< insert node point at x,y
  virtual bool addFront(ObjectPlot * qfront){return false;}
  virtual int getType(){return type;}                         ///< returns object type
  int getIndex(){return drawIndex;}                           ///< returns draw index
  virtual ObjectPlot* splitFront(float x, float y){return 0;}
  virtual void flip(){}
  virtual void setSize(float si){};
  virtual void increaseSize(float val){}
  virtual void increaseType(int val){setType(type+val);}      ///< increase symboltype
  virtual bool onLine(float x, float y);                      ///< returns true if x,y on line
  virtual float getDistX(){return 0;}
  virtual float  getDistY(){return 0;}
  virtual miutil::miString writeTypeString(){return " ";}
  virtual void setDefaultSize( ){}
  virtual void changeDefaultSize(){}
  virtual float getTransitionWidth(){return 0.0;}
  virtual miutil::miString getString(){return miutil::miString();}
  virtual void setString(miutil::miString s){}
  virtual void applyFilters(vector <miutil::miString>){};

  virtual void getComplexText(vector <miutil::miString> & symbolText, vector <miutil::miString> & xText){}
  virtual void getMultilineText(vector <miutil::miString> & symbolText){}
  virtual void changeComplexText(const vector <miutil::miString> & symbolText,const vector <miutil::miString> & xText){}
  virtual void changeMultilineText(const vector <miutil::miString> & symbolText){}
  virtual void readComplexText(miutil::miString complexString){}
  virtual void rotateObject(float val){} //only works for complex objects
  virtual void hideBox(){} //only works for complex objects
  virtual void setWhiteBox(int on){} //only works for complex objects


  bool oktoJoin(bool joinAll);                                ///< returns true if front ok to join
  bool oktoMerge(bool mergeAll,int index);                    ///< returns true if front ok to merge
  /// sets rubber flag (to draw rubberband) from x,y
  void setRubber(bool,float x,float y);
  /// returns true if object is text symbol
  bool isText(){return (objectIs(wSymbol) && drawIndex==0);}
  /// returns true if object is text symbol and colored
  bool isTextColored(){return (objectIs(wSymbol) && drawIndex==900);}
  /// returns true if object is complex symbol
  bool isComplex(){return (objectIs(wSymbol) && (drawIndex>=1000 && drawIndex<3000));}
  /// returns true if object is complex symbol and multiline text
  bool isTextMultiline(){return (objectIs(wSymbol) && drawIndex>=3000);}

  void setRegion(miutil::miString tt){region=tt;}                    ///< set from which region object come
  miutil::miString getRegion(){return region;}                       ///< get from which region object come


  float getFdeltaw(){return fSense*window_dw*w*0.5;}

  virtual int getXYZsize(){return nodePoints.size();}        ///< returns number of nodepoints
  virtual vector<float> getX();                              ///< returns x-values for all nodepoints
  virtual vector<float> getY();                              ///< returns y-valyes for all nodepoints
  virtual bool getAnnoTable(miutil::miString & str){return false;}
  vector<float> getXjoined();                                ///< returns x-values for all joined nodepoints
  vector<float> getYjoined();                                ///< returns y-values for all joined nodepoints
  vector<float> getXmarked();                                ///< returns x-values for all marked nodepoints
  vector<float> getYmarked();                                ///< returns y-values for all marked nodepoints
  vector<float> getXmarkedJoined();                          ///< returns x-values for all marked and joined nodepoints
  vector<float> getYmarkedJoined();                          ///< returns x-values for all marked and joined nodepoints
  virtual void setXY(vector<float> x,vector <float> y);      ///< sets x and y values for all nodepoints

  virtual float getLineWidth(){return 0;}
  virtual void setLineWidth(float w){}
  virtual void setVisible(bool v){isVisible=v;}              ///< set if objects is visible
  virtual void setSelected(bool s){isSelected=s;}            ///< set if objects is selectd
  virtual bool visible(){return isVisible;}                  ///< returns true if object visible
  virtual bool selected(){return isSelected;}                ///< returns true if object selected
  virtual bool isInsideArea(float x, float y){return true;}
  miutil::miString getName(){return name;}                           ///< returns object name
  void setName(miutil::miString n){name=n;}                          ///< sets object name

};

#endif























