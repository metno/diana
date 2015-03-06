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
#ifndef _diEditManager_h
#define _diEditManager_h

#include <diCommonTypes.h>
#include <diDrawingTypes.h>
#include <diMapMode.h>
#include <diObjectManager.h>
#include "diPlot.h"
#include <diFieldEdit.h>
#include <diEditSpec.h>

#include <diField/diGridConverter.h>

#include <vector>
#include <QString>

class AnnotationPlot;
class PlotModule;
class ObjectManager;
class FieldPlotManager;
class Field;

class QKeyEvent;
class QMouseEvent;

/**
  \brief Manager for editing fields and objects
*/
class EditManager {
private:
  PlotModule* plotm;
  ObjectManager* objm;
  FieldPlotManager* fieldPlotManager;

  std::vector<FieldEdit*> fedits;

  std::vector<EditProduct> editproducts; //all edit products
  EditProduct EdProd; //production currently in use
  EditProductId EdProdId; //production id info

  std::vector<mapModeInfo> mapmodeinfo;
  mapMode mapmode;
  int editmode; // edit mode index
  int edittool; // edit tools index
  bool editpause; //pause in editing.....

  GridConverter gc;   // gridconverter class

  float first_x;
  float first_y;
  float newx,newy;

  bool moved;

  bool fieldsCombined;
  int matrix_nx;
  int matrix_ny;
  double gridResolutionX;
  double gridResolutionY;
  int *combinematrix;
  std::vector< std::vector<FieldEdit*> > combinefields;
  std::vector<EditObjects> combineobjects;
  int numregs;
  std::vector<std::string> regnames;
  bool hiddenObjects;
  bool hiddenCombining;
  bool hiddenCombineObjects;
  int  showRegion;

  AnnotationPlot* apEditmessage; // special edit message (region shown,...)

  bool inEdit;                      //! edit in progress
  bool producttimedefined;          //! producttime is set
  miutil::miTime producttime;       //! proper product time

  std::vector<savedProduct> combineprods; // list of available product elements

  void setEditMessage(const std::string&); // special Edit message (shown region,...)

  std::string editFileName(const std::string directory,
                        const std::string region,
                        const std::string name,
			const miutil::miTime& t);

  bool recalcCombineMatrix();
  bool combineFields(float zoneWidth);

  void cleanCombineData(bool cleanData);

  void findSavedProducts(std::vector<savedProduct>& prods,
			 const std::string fileString,
			 dataSource dsource, int element,
			 int autoremove=-1);

  std::vector<std::string> findAcceptedCombine(int ibegin, int iend,
				       const EditProduct& ep,
				       const EditProductId& ei);


  bool unsentProduct; // true if last saved product has not been sent !

  void plotSingleRegion(Plot::PlotOrder zorder);

  void initEditTools();
  void setMapmodeinfo();

  // fedit_mode types
  std::vector<editToolInfo> eToolFieldStandard;
  std::vector<editToolInfo> eToolFieldClasses;
  std::vector<editToolInfo> eToolFieldNumbers;
  std::vector<editToolInfo> fronts;
  std::vector<editToolInfo> symbols;
  std::vector<editToolInfo> areas;
  std::vector<editToolInfo> sigsymbols;

public:
  EditManager(PlotModule*, ObjectManager*, FieldPlotManager*);
  ~EditManager();

  bool isInEdit() const
    { return inEdit; }

  /// parse EDIT section of setup file. (defines Edit products)
  bool parseSetup();
  /// reads the command file with OKstrings to be executed when we start an edit session
  void readCommandFile(EditProduct& ep);
  /// return names of existing fields valid for editing
  std::vector<std::string> getValidEditFields(const EditProduct& ep,
				      const int element);
  /// get list of saved products matching pname (and pid)
  std::vector<savedProduct> getSavedProducts(const EditProduct& ep,
					int element);
  /// get list of saved products matching pname (and pid)
  std::vector<savedProduct> getSavedProducts(const EditProduct& ep,
					std::string fieldname);
  /// returns miutil::miTime std::vector of combined products from EditProduct and EditProductId
  std::vector<miutil::miTime> getCombineProducts(const EditProduct& ep,
  				    const EditProductId & ei);
  /// returns std::string std::vector of combined Ids from time,EditProduct and EditProductId
  std::vector <std::string> getCombineIds(const miutil::miTime & valid,
				  const EditProduct& ep,
				  const EditProductId& ei);
  ///find product from name
  bool findProduct(EditProduct& ep,std::string pname);
  /// returns true if changed product has not been saved !
  bool unsavedEditChanges();
  /// returns true if last saved product has not been sent !
  bool unsentEditChanges();
  /// save field and objects to file and/or database
  bool writeEditProduct(QString&  message,
			const bool wfield =true,
			const bool wobjects =true,
                        const bool send = false,
			const bool isapproved = false);
  /// returns the current product time
  bool getProductTime(miutil::miTime& t);
  /// returns the current product name
  std::string getProductName();
  /// save edited annotations in EditObjects
  void saveProductLabels(std::vector <std::string> labels);
  bool fileExists(const EditProduct& ep, const EditProductId& ci, const miutil::miTime& time, QString& message);
  bool makeNewFile(int fnum, bool local, QString& message);
  /// start editing product
  bool startEdit(const EditProduct& ep,
		 const EditProductId& ei,
		 miutil::miTime& valid,
		 QString& message);
  /// stop editing product
  void stopEdit();
  /// start editing combine product
  bool startCombineEdit(const EditProduct& ep,
			const EditProductId& ei,
			const miutil::miTime& valid,
			std::vector<std::string>& pids,
			QString& message);
  /// stop editing combine product
  void stopCombine();
  /// combine products (when borders or data sources change)
  bool editCombine();
  /// set and get mapmode, editmode and edittool
  void setEditMode(const std::string mmode,  // mapmode
		   const std::string emode,  // editmode
		   const std::string etool); // edittool

  /// get mapmode
  mapMode getMapMode();
  /// returns true if pause in editing
  bool getEditPause(){return editpause;}
  /// sets pause in editing on or off
  void setEditPause(bool on){editpause=on;}
  /// handle mouse event
  void sendMouseEvent(QMouseEvent* me, EventResult& res);
  /// handle keyboard event
  void sendKeyboardEvent(QKeyEvent* me, EventResult& res);
  /// notifies field edits about EditEvent
  bool notifyEditEvent(const EditEvent& ee);
  /// activated field edit index
  void activateField(int index);
  /// returns current EditDialogInfo
  EditDialogInfo getEditDialogInfo();
  /// set plotting parameters for EditFields from inp
  void prepareEditFields(const std::string& plotName, const std::vector<std::string>& inp);
  /// gets area from field
  bool getFieldArea(Area& a);
  /// plot edit fields and objects (under=true->plot inactive fields/objects, over=true plot active fields/objects)
  void plot(Plot::PlotOrder zorder);
  /// show difference between observed mslp and edited mslp
  bool obs_mslp(ObsPositions& obsPositions);
  /// shows all hidden edit objects
  bool showAllObjects();
  /// returns EditProducts defined in setup file
  std::vector<EditProduct> getEditProducts();
  /// returns name of all EditProducts defined in setup file
  std::vector<std::string> getEditProductNames();
  /// returns a string with product id, name, time and object types
  std::string savedProductString(savedProduct sp);
  /// get fieldEdit annotations
  bool getAnnotations(std::vector<std::string>& anno);
  /// insert time in text string
  const std::string insertTime(const std::string&, const miutil::miTime&);
};


//useful functions not belonging to EditManager
editToolInfo newEditToolInfo(const std::string & newName,
			     const int newIndex,
			     const std::string & newColour="black",
			     const std::string & newBorderColour="black",
			     const int & newSizeIncrement=0,
			     const bool & newSpline=true,
			     const std::string& newLinetype ="solid",
			     const std::string& newFilltype="");
editModeInfo newEditModeInfo(const std::string & newmode,
			     const std::vector <editToolInfo> newtools);
mapModeInfo newMapModeInfo(const std::string & newmode,
			   const std::vector <editModeInfo> newmodeinfo);

#endif
