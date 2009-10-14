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

#include <vector>
#include <puTools/miString.h>
#include <diCommonTypes.h>
#include <diDrawingTypes.h>
#include <diMapMode.h>
#include <diField/diGridConverter.h>
#include <diObjectManager.h>
#include <diFieldEdit.h>
#include <diSetupParser.h>
#include <diEditSpec.h>

#ifdef METNOPRODDB
#include <diSQL/diProductionGate.h>
#endif

using namespace std;


class PlotModule;
class ObjectManager;
class Field;


/**

  \brief Manager for editing fields and objects


*/

class EditManager {
private:
  PlotModule* plotm;
  ObjectManager* objm;

  vector<FieldEdit*> fedits;

  vector<EditProduct> editproducts; //all edit products
  EditProduct EdProd; //production currently in use
  EditProductId EdProdId; //production id info

  vector<mapModeInfo> mapmodeinfo;
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
  int *combinematrix;
  vector< vector<FieldEdit*> > combinefields;
  vector<EditObjects> combineobjects;
  int numregs;
  vector<miString> regnames;
  bool hiddenObjects;
  bool hiddenCombining;
  bool hiddenCombineObjects;
  int  showRegion;

  vector<savedProduct> combineprods; // list of available product elements

  miString editFileName(const miString directory,
                        const miString region,
                        const miString name,
			const miTime& t);

  bool recalcCombineMatrix();
  bool combineFields(float zoneWidth);

  void cleanCombineData(bool cleanData);

#ifdef METNOPRODDB
  ProductionGate gate; //database gate
#endif

  void findSavedProducts(vector<savedProduct>& prods,
			 const miString fileString,
			 dataSource dsource, int element);

  vector<miString> findAcceptedCombine(int ibegin, int iend,
				       const EditProduct& ep,
				       const EditProductId& ei);


  bool unsentProduct; // true if last saved product has not been sent !

  void plotSingleRegion();

  void initEditTools();
  void setMapmodeinfo();

  // fedit_mode types
  vector<editToolInfo> eToolFieldStandard;
  vector<editToolInfo> eToolFieldClasses;
  vector<editToolInfo> eToolFieldNumbers;
  vector<editToolInfo> fronts;
  vector<editToolInfo> symbols;
  vector<editToolInfo> areas;
  vector<editToolInfo> sigsymbols;

public:
  EditManager(PlotModule*, ObjectManager*);
  ~EditManager();

  /// parse EDIT section of setup file. (defines Edit products)
  bool parseSetup(SetupParser& sp);
  /// reads the command file with OKstrings to be executed when we start an edit session
  void readCommandFile(EditProduct& ep);
  /// return names of existing fields valid for editing
  vector<miString> getValidEditFields(const EditProduct& ep,
				      const int element);
  /// get list of saved products matching pname (and pid)
  vector<savedProduct> getSavedProducts(const EditProduct& ep,
					int element);
  /// get list of saved products matching pname (and pid)
  vector<savedProduct> getSavedProducts(const EditProduct& ep,
					miString fieldname);
  /// returns miTime vector of combined products from EditProduct and EditProductId
  vector<miTime> getCombineProducts(const EditProduct& ep,
  				    const EditProductId & ei);
  /// returns miString vector of combined Ids from time,EditProduct and EditProductId
  vector <miString> getCombineIds(const miTime & valid,
				  const EditProduct& ep,
				  const EditProductId& ei);
  ///find product from name
  bool findProduct(EditProduct& ep,miString pname);
  /// returns true if changed product has not been saved !
  bool unsavedEditChanges();
  /// returns true if last saved product has not been sent !
  bool unsentEditChanges();
  /// save field and objects to file and/or database
  bool writeEditProduct(miString&  message,
			const bool wfield =true,
			const bool wobjects =true,
                        const bool send = false,
			const bool isapproved = false);
  /// returns the current product time
  bool getProductTime(miTime& t);
  /// returns the current product name
  miString getProductName();
  /// save edited annotations in EditObjects
  void saveProductLabels(vector <miString> labels);
  /// start editing product
  bool startEdit(const EditProduct& ep,
		 const EditProductId& ei,
		 const miTime& valid);
  /// stop editing product
  void stopEdit();
  /// start editing combine product
  bool startCombineEdit(const EditProduct& ep,
			const EditProductId& ei,
			const miTime& valid,
			vector<miString>& pids);
  /// stop editing combine product
  void stopCombine();
  /// combine products (when borders or data sources change)
  bool editCombine();
  /// set and get mapmode, editmode and edittool
  void setEditMode(const miString mmode,  // mapmode
		   const miString emode,  // editmode
		   const miString etool); // edittool

  /// get mapmode
  mapMode getMapMode();
  /// returns true if pause in editing
  bool getEditPause(){return editpause;}
  /// sets pause in editing on or off
  void setEditPause(bool on){editpause=on;}
  /// handle mouse event
  void sendMouseEvent(const mouseEvent& me, EventResult& res);
  /// handle keyboard event
  void sendKeyboardEvent(const keyboardEvent& me, EventResult& res);
  /// notifies field edits about EditEvent
  bool notifyEditEvent(const EditEvent& ee);
  /// activated field edit index
  void activateField(int index);
  /// returns current EditDialogInfo
  EditDialogInfo getEditDialogInfo();
  /// set plotting parameters for EditFields from inp
  void prepareEditFields(const vector<miString>& inp);
  /// gets area from field
  bool getFieldArea(Area& a);
  /// plot edit fields and objects (under=true->plot inactive fields/objects, over=true plot active fields/objects)
  void plot(bool under, bool over);
  /// show difference between observed mslp and edited mslp
  bool obs_mslp(ObsPositions& obsPositions);
  /// shows all hidden edit objects
  bool showAllObjects();
  /// login to the database
  bool loginDatabase(editDBinfo& db, miString& message);
  /// logout of the database
  bool logoutDatabase(editDBinfo& db);
  /// terminate one production
  bool killProduction(const miString&,const miString&,
		      const miTime&,miString&);
  /// check if OK to start editing this product.
  /** If production already started return false, else return true,
      in: prodname,pid,prodtime out: message */
  bool checkProductAvailability(const miString&,const miString&,
				const miTime&,miString&);
  /// returns EditProducts defined in setup file
  vector<EditProduct> getEditProducts();
  /// returns a string with product id, name, time and object types
  miString savedProductString(savedProduct sp);
  /// get fieldEdit annotations
  bool getAnnotations(vector<miString>& anno);
  /// insert time in text string
  const miString insertTime(const miString&, const miTime&);
};


//useful functions not belonging to EditManager
editToolInfo newEditToolInfo(const miString & newName,
			     const int newIndex,
			     const miString & newColour="black",
			     const miString & newBorderColour="black");
editModeInfo newEditModeInfo(const miString & newmode,
			     const vector <editToolInfo> newtools);
mapModeInfo newMapModeInfo(const miString & newmode,
			   const vector <editModeInfo> newmodeinfo);

#endif
