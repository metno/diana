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
#include <diQuickMenues.h>
#include <diSetupParser.h>
#include <fstream>

// write a quick-menu to file
bool writeQuickMenu(const quickMenu& qm, bool newSyntax)
{

  SetupParser setup;
  miString filename;

  if( !qm.filename.contains("/") ){
    filename = setup.basicValue("homedir") + "/";
  }
  filename += qm.filename;
  ofstream menufile(filename.c_str());

  if (!menufile){
    if(newSyntax){
      cerr << "QuickMenu Warning: Old syntax in quick-menu file:"
	   << filename << endl;
    }else {
      cerr << "QuickMenu Error: Could not write quick-menu file:"
	   << filename << endl;
    }
    return false;
  }

  menufile << "# Tekst og Plottestreng for Quick-menyen" << endl;
  menufile << "# '#' angir kommentarer" << endl; 
  menufile << "#------------------------------------------------" << endl;
  menufile << "#-- Navn paa hurtigmeny, angis i dialogen" << endl;
  menufile << "# \"navn\"" << endl;
  menufile << "#------------------------------------------------" << endl;
  menufile << "#-- Definisjon av variable" << endl;
  menufile << "# [XX=a,b,c,d,..]" << endl;
  menufile << "#------------------------------------------------" << endl;
  menufile << "#-- Plottevalg" << endl;
  menufile << "# '>Plottenavn' er skilletegn for plotte-valgene" << endl;
  menufile << "#------------------------------------------------" << endl;
  menufile << endl;
  menufile << "# quickmenu name" << endl;

  menufile << "\"" << qm.name << "\"" << endl;
  menufile << endl;

  menufile << "# variables" << endl;
  
  // write options
  int m,n= qm.opt.size();
  for (int i=0; i<n; i++){
    menufile << "[" << qm.opt[i].key << "=";
    m= qm.opt[i].options.size();
    for (int j=0; j<m; j++){
      menufile << qm.opt[i].options[j];
      if (j<m-1) menufile << ",";
    }
    menufile << endl;
  }

  menufile << endl;
  
  // the plots
  n= qm.menuitems.size();
  for (int i=0; i<n; i++){
    menufile << "#" << endl;
    menufile << ">" << qm.menuitems[i].name << endl;
    m= qm.menuitems[i].command.size();
    for (int j=0; j<m; j++){
      menufile << qm.menuitems[i].command[j] << endl;
    }
  }

  return true;
}


// parse quick-menu-file
bool readQuickMenu(quickMenu& qm)
{
  quickMenuItem mi;
  quickMenuOption op;
  miString value;
  vector<miString> tokens, stokens;
  int updates=0;

  miString filename= qm.filename;
  ifstream menufile(filename.c_str());
  int numitems=0;
  qm.menuitems.clear();
  qm.opt.clear();
  qm.plotindex= 0;
    
  if (!menufile){ // menufile not ok
    cerr << "QuickMenu Warning: Could not open quickmenu file "
	 << filename << endl;
    return false;
  }


  miString line;
  while (getline(menufile,line)){
    line.trim();
    if (line.length()==0) continue;
    if (line[0]=='#') continue;
    
    if (line[0]=='"'){
      // name of quickmenu
      if (line[line.length()-1]=='"')
	qm.name = line.substr(1,line.length()-2);
      else
	qm.name = line.substr(1,line.length()-1);
      
    } else if (line[0]=='['){
      // variable/options
      if (line[line.length()-1]==']')
	line = line.substr(1,line.length()-2);
      else
	line = line.substr(1,line.length()-1);
      tokens= line.split("=");
      if (tokens.size()>1){
	op.key= tokens[0];
	value= tokens[1];
	op.options= value.split(",");
	op.def= (op.options.size()>0 ? op.options[0] : "");
	// add a new option
	qm.opt.push_back(op);
      } else {
	cerr << "QuickMenu Error: defined option without items in file "
	     << filename << endl;
	menufile.close();
	return false;
      }
    } else if (line[0]=='>'){
      // new menuitem
      qm.menuitems.push_back(mi);
      qm.menuitems[numitems].name= line.substr(1,line.length()-1);
      qm.menuitems[numitems].command.clear();
      numitems++;
    } else {
      // commands
      if (numitems>0) {
	updates += updateSyntax(line);
	qm.menuitems[numitems-1].command.push_back(line);
      } else {
	cerr << "QuickMenu Error: command line without defined menuitem in file:"
	     << filename << endl;
	menufile.close();
	return false;
      }
    }
  }
  menufile.close();
  if (!qm.name.exists())
    qm.name= "Udefinert navn";
  
  //if old syntax changed, update file
  if(updates > 0)
    writeQuickMenu(qm,true);

  return true;
}

int updateSyntax(miString& line)
{

  if( line.contains("_3_farger")){
    line.replace("_3_farger","");
    return 1;
  }

  if( line.contains("test.contour.shading=1")){
    line.replace("test.contour.shading=1","palettecolours=standard");
    return 1;
  }
   
  if( line.contains("st.nr")){
    if( line.contains("st.nr("))
      line.replace("st.nr","st.no");
    else
      line.replace("st.nr","st.no(5)");
    return 1;
  }
   
  if( line.contains("(74,504)")){
    line.replace("(74,504)","(uk)");
    return 1;
  }
  if( line.contains("(74,533)")){
    line.replace("(74,533)","(uk)");
    return 1;
  }
  if( line.contains("(74,604)")){
    line.replace("(74,604)","(uk)");
    return 1;
  }


  return 0;

}


