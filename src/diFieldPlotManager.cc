/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diPlotOptions.cc 369 2007-11-02 08:55:24Z lisbethb $

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

#include <diFieldPlotManager.h>
#include <diPlotOptions.h>

using namespace std; 


FieldPlotManager::FieldPlotManager(FieldManager* fm)
  :fieldManager(fm)
{
}



void FieldPlotManager::getAllFieldNames(vector<miString>& fieldNames)
{

  for (int i =0; i<vPlotField.size(); i++){
    fieldNames.push_back(vPlotField[i].name);
  }

}

bool FieldPlotManager::parseSetup(SetupParser &sp)
{
  
//   cerr <<"bool FieldPlotManager::parseSetup"<<endl;

  fieldManager->getPrefixandSuffix(fieldprefixes,fieldsuffixes);

  const miString sect_name = "FIELD_PLOT";
  vector<miString> lines;

  if (!sp.getSection(sect_name,lines)){
    cerr << sect_name << " section not found" << endl;
    return false;
  }

  const miString key_loop=       "loop";
  const miString key_field=      "field";
  const miString key_endfield=   "end.field";
  const miString key_fieldgroup= "fieldgroup";
  const miString key_plot=       "plot";

  const miString key_plottype= "ftype";


//   if (!sp.getSection(section,lines)) {
//     cerr<<"Missing section "<< section<<" in setupfile."<<endl;
//     return false;
//   }


  // parse setup

  int nlines= lines.size();

  vector<miString> vstr;
  miString key,str,str2,option;
  vector<miString> vpar;

  vector<miString> loopname;
  vector<vector<miString> > loopvars;
  map<miString,int>::const_iterator pfp;
  int firstLine,lastLine,nv;
  bool waiting= true;

  for (int l=0; l<nlines; l++) {

    vstr= splitComStr(lines[l],true);
    int n= vstr.size();
    key= vstr[0].downcase();

    if (waiting) {
      if (key==key_loop && n>=4) {
	vpar.clear();
	for (int i=3; i<vstr.size(); i++) {
	  if (vstr[i][0]=='"')
	    vpar.push_back(vstr[i].substr(1,vstr[i].length()-2));
	  else
	    vpar.push_back(vstr[i]);
	}
	loopname.push_back(vstr[1]);
	loopvars.push_back(vpar);
      } else if (key==key_field) {
	firstLine= l;
        waiting= false;
      }
    } else if (key==key_endfield) {
      lastLine= l;

      int nl= loopname.size();
      if (nl>loopvars.size()) nl= loopvars.size();
      int ml= 1;
      if (nl>0) {
	ml= loopvars[0].size();
	for (int il=0; il<nl; il++)
	  if (ml>loopvars[il].size())
	    ml= loopvars[il].size();
      }

      for (int m=0; m<ml; m++) {
	miString name;
	miString fieldgroup;
        vector<miString> input;
	for (int i=firstLine; i<lastLine; i++) {
	  str=lines[i];
          for (int il=0; il<nl; il++)
	    str.replace(loopname[il],loopvars[il][m]);
	  if (i==firstLine) {
	    // resplit to keep names with ()
	    vstr= str.split('=',false);
	    if (vstr.size()<2) {
	      miString errm="Missing field name";
	      sp.errorMsg(sect_name,i,errm);
	      return false;
	    }
	    name= vstr[1];
	    if (name[0]=='"' && name[name.length()-1]=='"')
	      name= name.substr(1,name.length()-2);
	  } else {
	    vstr= splitComStr(str,false);
	    nv= vstr.size();
	    int j= 0;
	    while (j<nv-2) {
	      key= vstr[j].downcase();
	      if (key==key_plot && vstr[j+1]=="=" && j<nv-3) {
		option= key_plottype + "=" + vstr[j+2];

		if (!PlotOptions::updateFieldPlotOptions(name,option)) {
		  miString errm="|Unknown fieldplottype in plotcommand";
		  sp.errorMsg(sect_name,i,errm);
		  return false;
	        }
	        str2=vstr[j+3].downcase().substr(1,vstr[j+3].length()-2);
	        input=str2.split(',',true);
		if (input.size()<1 || input.size()>3) {
		  miString errm="Bad specification of plot arguments";
		  sp.errorMsg(sect_name,i,errm);
       		  return false;
		}
		for (int k=0; k<input.size(); k++)
		  input[k]= input[k].downcase();
	      } else if (key==key_fieldgroup && vstr[j+1]=="=") {
		fieldgroup= vstr[j+2];
		if (fieldgroup[0]=='"' && fieldgroup[fieldgroup.length()-1]=='"')
		  fieldgroup= fieldgroup.substr(1,fieldgroup.length()-2);
	      } else if (vstr[j+1]=="=") {
		// this should be a plot option
	        option= vstr[j] + "=" + vstr[j+2];

		if (!PlotOptions::updateFieldPlotOptions(name,option)) {
		  miString errm="Something wrong in plotoption specifications";
		  sp.errorMsg(sect_name,i,errm);
		  return false;
 		}
	      } else {
		miString errm="Unknown keyword in field specifications: " + vstr[0];
		sp.errorMsg(sect_name,i,errm);
		return false;
		//j-=2;
	      }
	      j+=3;
	    }
	  }
	}

	if (!name.empty() && !input.empty()) {
	  int i=0;
	  while(i<vPlotField.size() && vPlotField[i].name.downcase() != name.downcase()) i++;
	  if (i<vPlotField.size()) {
 	    cerr<<"  replacing plot specs. for field "<<name<<endl;
	    vPlotField[i].input=input;
	  } else {
	    PlotField pf;
	    pf.name = name;
	    pf.fieldgroup = fieldgroup;
	    pf.input = input;
	    vPlotField.push_back(pf);
	  }
	  mapPlotField[name.downcase()]=vPlotField[i];
	}
      }

      loopname.clear();
      loopvars.clear();
      waiting= true;
    }
  }


  return true;
}


vector<miString> FieldPlotManager::splitComStr(const miString& s, bool splitall)
{
// split commandstring into tokens.
// split on '=', ',' and multiple blanks, keep blocks within () and ""
// split on ',' only if <splitall> is true

  vector<miString> tmp;

  int i=0,j=0,n= s.size();
  if (n){
    while (i<n && s[i]==' ') i++;
    j= i;
    for (; i<n; i++){
      if (s[i]=='='){ // split on '=', but keep it
	if (i-j>0) 
	  tmp.push_back(s.substr(j,i-j));
	tmp.push_back("=");
	j=i+1;
      } else if (s[i]==',' && splitall){ // split on ','
	if (i-j>0)
	  tmp.push_back(s.substr(j,i-j));
	j=i+1;
      } else if (s[i]=='('){ // keep () blocks
	if (i-j>0)
	  tmp.push_back(s.substr(j,i-j));
	j=i; i++;
	while (i<n && s[i]!=')') i++;
	tmp.push_back(s.substr(j,(i<n)?(i-j+1):(i-j)));
	j=i+1;
      } else if (s[i]=='"'){ // keep "" blocks
	if (i-j>0)
	  tmp.push_back(s.substr(j,i-j));
	j=i; i++;
	while (i<n && s[i]!='"') i++;
	tmp.push_back(s.substr(j,(i<n)?(i-j+1):(i-j)));
	j=i+1;
      } else if (s[i]==' '){ // split on (multiple) blanks
	if (i-j>0)
	  tmp.push_back(s.substr(j,i-j));
	while (i<n && s[i]==' ') i++;
	j=i; i--;
      } else if (i==n-1)
	tmp.push_back(s.substr(j,i-j+1));
    }
  }

  return tmp;
}

vector<miString>  FieldPlotManager::getFields()
{

  set<miString> paramSet;
  for(int i=0;i<vPlotField.size();i++){
    for(int j=0;j<vPlotField[i].input.size();j++){
      paramSet.insert(vPlotField[i].input[j]);
    }
  }

  vector<miString> param;
  set<miString>::iterator p=paramSet.begin();
  for (; p!=paramSet.end(); p++) param.push_back(*p);

  return param;

}

vector<miTime> FieldPlotManager::getFieldTime(const vector<miString>& pinfos, 
					  bool& constTimes)
{
  vector<miTime> fieldtime;

  int numf= pinfos.size();

  vector<FieldTimeRequest> request;
  FieldTimeRequest ftr;
  miString modelName, modelName2, fieldName;
  bool allTimeSteps= false;

  for (int nf=0; nf<numf; nf++) {
    vector<miString> tokens= pinfos[nf].split('"','"');
    
    int n= tokens.size();
    // at least FIELD <model> <field>..
    //      or  FIELD ( <model> <field> .. - <model> <field> .. ) .....
    if (n<3) continue;

    if (tokens[1] == "(") {

      // field difference ... two models and/or fields

      int km=2;
      while (km<n && tokens[km]!="-") km++;
      int ke=km+1;
      while (ke<n && tokens[ke]!=")") ke++;
      if (km>=n || ke>=n || km>ke-3) continue;

      modelName= tokens[2].downcase();
      fieldName= tokens[3].downcase();

      modelName2= tokens[km+1].downcase();

      vector<miString> atokens= tokens;
      tokens.clear();
      tokens.push_back(atokens[0]);
      for (int k=2; k<km; k++)
        tokens.push_back(atokens[k]);
      for (int k=ke+1; k<n; k++)
        tokens.push_back(atokens[k]);
      n= tokens.size();

    } else {

      modelName= tokens[1].downcase();
      fieldName= tokens[2].downcase();
    }

    vector<miString> vtoken,values;
    miString levelName,idnumName;
    int hourOffset=0, hourDiff=0, fctype=0;
    vector<int> vfc;

    for (int k=3; k<n; k++) {
      vtoken= tokens[k].downcase().split('=');
      if (vtoken.size()>=2) {
	if (vtoken[0]=="level") {
	  levelName = vtoken[1];
	} else if (vtoken[0]=="idnum") {
	  idnumName = vtoken[1];
	} else if (vtoken[0]=="hour.offset") {
	  hourOffset = atoi(vtoken[1].cStr());
	} else if (vtoken[0]=="alltimesteps") {
	  if (vtoken[1]=="1" || vtoken[1]=="on" || vtoken[1]=="true")
	    allTimeSteps= true;
	} else if (vtoken[0]=="forecast.hour") {
	  values= vtoken[1].split(',');
	  for (int i=0; i<values.size(); i++)
	    vfc.push_back(atoi(values[i].cStr()));
	  fctype=1;
	} else if (vtoken[0]=="forecast.hour.loop") {
	  values= vtoken[1].split(',');
	  if (values.size()==3) {  // first,last,step
	    for (int i=0; i<3; i++)
	      vfc.push_back(atoi(values[i].cStr()));
	    fctype=2;
	  }
	}
      }
    }

    int nr= request.size();
    request.push_back(ftr);
    request[nr].modelName=    modelName;
    request[nr].fieldName=    fieldName;
    request[nr].levelName=    levelName;
    request[nr].idnumName=    idnumName;
    request[nr].hourOffset=   hourOffset;
    request[nr].forecastSpec= fctype;
    request[nr].forecast=     vfc;
  }

  if (request.size()==0) return fieldtime;

  return getFieldTime(request,allTimeSteps,constTimes);
}

void FieldPlotManager::getCapabilitiesTime(vector<miTime>& normalTimes,
				       miTime& constTimes,
				       int& timediff,
				       const miString& pinfo)
{
  //Finding times from pinfo
  //TODO: find const time

  vector<miString> pinfos;
  pinfos.push_back(pinfo);

  //finding timediff
  timediff=0;
  vector<miString> tokens= pinfo.split('"','"');
  int m= tokens.size();
  for(int j=0; j<tokens.size();j++){
    vector<miString> stokens= tokens[j].split("=");
    if(stokens.size()==2 && stokens[0].downcase()=="timediff"){
      timediff=stokens[1].toInt();
    }
  }

  //getting times
  bool constT;
  normalTimes = getFieldTime(pinfos,constT);
  if(constT ){
    if(normalTimes.size())
      constTimes = normalTimes[0];
    normalTimes.clear();
  }

}

vector<miString> FieldPlotManager::getFieldLevels(const miString& pinfo)
{

  vector<miString> levels;

  vector<miString> tokens = pinfo.split(" ");
  if(tokens.size()<3 || tokens[0]!="FIELD") return levels;

  vector<FieldGroupInfo> vfgi;
  miString name;
  getFieldGroups(tokens[1], name, vfgi);
  for(int i=0;i<vfgi.size();i++){
    levels.push_back(vfgi[i].groupName);
    int k=0;
    int n=vfgi[i].fieldNames.size();
    while(k<n && vfgi[i].fieldNames[k]!=tokens[2]) k++;
    if(k<n){
      //      levels.push_back("Levelgroup:" + vfgi[i].groupName);
      for(int j=0;j<vfgi[i].levelNames.size();j++){
	levels.push_back(vfgi[i].levelNames[j]);
      }
    }
  }

  return levels;

}

vector<miString>  FieldPlotManager::getPlotFields()
{

  vector<miString> param;
  for(int i=0;i<vPlotField.size();i++){
    param.push_back(vPlotField[i].name);
  }

  return param;

}

vector<miTime> FieldPlotManager::getFieldTime(vector<FieldTimeRequest>& request,
					      bool allTimeSteps,
					      bool& constTimes)

{
//   cerr <<"FieldPlotManager::getFieldTime"<<endl;
  vector<miTime> vtime;
  for(int j=0;j<request.size();j++){
    
    miString fieldName;
    miString plotName=request[j].fieldName.downcase();
    miString suffix;
    splitSuffix(plotName,suffix);
    if(!mapPlotField.count(plotName)) return vtime;
    for(int i=0;i<mapPlotField[plotName].input.size();i++){
      miString fieldName=mapPlotField[plotName].input[i].downcase();
      fieldName += suffix;
      request[j].fieldName=fieldName;
    }

  }
  return   fieldManager->getFieldTime(request,allTimeSteps,constTimes);
}

bool FieldPlotManager::makeFields(const miString& pin, 
				  const miTime& const_ptime,
				  vector<Field*>& vfout,
				  const miString& levelSpec, 
				  const miString& levelSet,
				  const miString& idnumSpec, 
				  const miString& idnumSet,
				  bool toCache)
{
  
  vfout.clear();
  miString modelName;
  miString plotName;
  vector<miString> fieldName;
  miString levelName;
  miString idnumName;
  int hourOffset=0;
  int hourDiff=0;
  miTime ptime = const_ptime; 
  parsePin(pin,modelName,plotName,fieldName,levelName,idnumName,hourOffset,hourDiff,ptime);

  if (hourOffset!=0) {
    ptime.addHour(hourOffset);
  } 


  miString levelSpecified= levelName;
  miString idnumSpecified= idnumName;
  
  if (!levelSpec.empty() && !levelSet.empty()
      && levelName.downcase()==levelSpec.downcase())
    levelName= levelSet;

  if (!idnumSpec.empty() && !idnumSet.empty()
      && idnumName.downcase()==idnumSpec.downcase())
    idnumName= idnumSet;

  bool ok=false;
  for(int i=0;i<fieldName.size();i++){
    Field* fout;
    ok=fieldManager->makeField(fout,
			       modelName, 
			       fieldName[i], 
			       ptime,
			       levelName,
			       idnumName,
			       hourDiff,
			       toCache);
    
    if (!ok) return false;

    makeFieldText(fout,plotName,levelSpecified,idnumSpecified);
    vfout.push_back(fout);
  
  }

  return true;

}

void FieldPlotManager::makeFieldText(Field* fout,
				     const miString& plotName,
				     const miString& levelSpecified,
				     const miString& idnumSpecified)
{

    miString fieldtext = fout->modelName + " " + plotName;
    if (!fout->leveltext.empty()) fieldtext+= " " + fout->leveltext;
    if (!fout->idnumtext.empty()) fieldtext+= " " + fout->idnumtext;

    ostringstream ostr;
    if (fout->forecastHour>=0) ostr << "+" << fout->forecastHour;
    else              ostr << fout->forecastHour;
    miString progtext= "(" + ostr.str() + ")";
    
    miString sclock= fout->validFieldTime.isoClock();
    miString shour=  sclock.substr(0,2);
    miString smin=   sclock.substr(3,2);
    miString timetext;
    if (smin=="00")
      timetext = fout->validFieldTime.isoDate() + " " + shour + " UTC";
    else
      timetext = fout->validFieldTime.isoDate() + " " + shour + ":" + smin + " UTC";
    fout->name           = plotName;
    fout->text           = fieldtext + " " + progtext;
    fout->fulltext       = fieldtext + " " + progtext + " " + timetext;
    fout->fieldText      = fieldtext;
    fout->progtext       = progtext;
    fout->timetext       = timetext;
    fout->levelSpec      = levelSpecified;
    fout->idnumSpec      = idnumSpecified;


}

bool FieldPlotManager::makeDifferenceField(const miString& fspec1, 
					   const miString& fspec2,
					   const miTime& const_ptime,
					   vector<Field*>& fv,
					   const miString& levelSpec, 
					   const miString& levelSet,
					   const miString& idnumSpec, 
					   const miString& idnumSet,
					   int vectorIndex)
{

  fv.clear();
  vector<Field*> fv1;
  vector<Field*> fv2;

  if(makeFields(fspec1, const_ptime,fv1,
		levelSpec, levelSet, idnumSpec, idnumSet)){
    if(!makeFields(fspec2, const_ptime,fv2,
		   levelSpec, levelSet, idnumSpec, idnumSet)){
      fieldManager->fieldcache->freeFields(fv1);
      return false;
    }
  } else {
    return false;
  }

  //make copy of fields, do not change the field cache
  for(int i=0;i<fv1.size();i++){
    Field* ff= new Field();
    fv.push_back(ff);
    *fv[i] = *fv1[i];
  }
  fieldManager->fieldcache->freeFields(fv1);


    //make Difference Field text
    Field* f1= fv[0];
    Field* f2= fv2[0];
    
    const int mdiff=6;
    miString text1[mdiff], text2[mdiff];
    bool diff[mdiff];
    text1[0]= f1->modelName;
    text1[1]= f1->name;
    text1[2]= f1->leveltext;
    text1[3]= f1->idnumtext;
    text1[4]= f1->progtext;
    text1[5]= f1->timetext;
    text2[0]= f2->modelName;
    text2[1]= f2->name;
    text2[2]= f2->leveltext;
    text2[3]= f2->idnumtext;
    text2[4]= f2->progtext;
    text2[5]= f2->timetext;
    int nbgn= -1;
    int nend= 0;
    int ndiff= 0;
    for (int n=0; n<mdiff; n++) {
      if (text1[n]!=text2[n]) {
	diff[n]= true;
	if (nbgn<0) nbgn= n;
	nend= n;
	ndiff++;
      } else {
	diff[n]= false;
      }
    }

    if (ndiff==0) {
      // may happen due to level/idnum up/down change or equal difference,
      // make an explaining text
      if (!f1->leveltext.empty())
	diff[2]= true;
      else if (!f1->idnumtext.empty())
	diff[3]= true;
      else
	diff[1]= true;
      ndiff= 1;
    }

    if (diff[0])
      f1->modelName = "( " + text1[0] + " - " + text2[0] + " )";
    if (diff[1] && (diff[2] || diff[3])) {
      f1->name= "( " + text1[1];
      if (!text1[2].empty()) f1->name+= " " + text1[2];
      if (!text1[3].empty()) f1->name+= " " + text1[3];
      f1->name+= " - " + text2[1];
      if (!text2[2].empty()) f1->name+= " " + text2[2];
      if (!text2[3].empty()) f1->name+= " " + text2[3];
      f1->name+= " )";
      f1->leveltext.clear();
      if (diff[2] && diff[3]) ndiff-=2;
      else                    ndiff--;
    } else {
      if (diff[1]) f1->name      = "( " + text1[1] + " - " + text2[1] + " )";
      if (diff[2]) f1->leveltext = "( " + text1[2] + " - " + text2[2] + " )";
      if (diff[3]) f1->idnumtext = "( " + text1[3] + " - " + text2[3] + " )";
    }
    if (diff[4]) f1->progtext = "( " + text1[4] + " - " + text2[4] + " )";
    if (diff[5]) f1->timetext = "( " + text1[5] + " - " + text2[5] + " )";
    if (ndiff==1) {
      f1->fieldText= f1->modelName + " "  + f1->name;
      if (f1->leveltext.exists()) f1->fieldText += " " + f1->leveltext;
      f1->text= f1->fieldText + " " + f1->progtext;
      f1->fulltext= f1->text + " " + f1->timetext;
    } else {
      if (nbgn==1 && nend<=3) {
	if (!text1[2].empty()) text1[1]+= " " + text1[2];
	if (!text1[3].empty()) text1[1]+= " " + text1[3];
	if (!text2[2].empty()) text2[1]+= " " + text2[2];
	if (!text2[3].empty()) text2[1]+= " " + text2[3];
	text1[2].clear();
	text1[3].clear();
	text2[2].clear();
	text2[3].clear();
	nend= 1;
      }
      int nmax[3]= { 5,4,3 };
      miString ftext[3];
      for (int t=0; t<3; t++) {
	if (nbgn>nmax[t]) nbgn= nmax[t];
	if (nend>nmax[t]) nend= nmax[t];
	bool first= true;
	for (int n=0; n<nbgn; n++) {
	  if (first) ftext[t]= text1[n];
	  else       ftext[t]+= " " + text1[n];
	  first= false;
	}
	if (first) ftext[t]= "(";
	else       ftext[t]+= " (";
	for (int n=nbgn; n<=nend; n++)
	  if (!text1[n].empty()) ftext[t]+= " " + text1[n];
	ftext[t]+= " -";
	for (int n=nbgn; n<=nend; n++)
	  if (!text2[n].empty()) ftext[t]+= " " + text2[n];
	ftext[t]+= " )";
	for (int n=nend+1; n<=nmax[t]; n++)
	  if (!text1[n].empty()) ftext[t]+= " " + text1[n];
      }
      f1->fulltext=  ftext[0];
      f1->text=      ftext[1];
      f1->fieldText= ftext[2];
    }

#ifdef DEBUGFDIFF
    cerr<<"F1-F2: validFieldTime: "<<f1->validFieldTime<<endl;
    cerr<<"F1-F2: analysisTime:   "<<f1->analysisTime<<endl;
    cerr<<"F1-F2: name:           "<<f1->name<<endl;
    cerr<<"F1-F2: text:           "<<f1->text<<endl;
    cerr<<"F1-F2: fulltext:       "<<f1->fulltext<<endl;
    cerr<<"F1-F2: modelName:      "<<f1->modelName<<endl;
    cerr<<"F1-F2: fieldText:      "<<f1->fieldText<<endl;
    cerr<<"F1-F2: leveltext:      "<<f1->leveltext<<endl;
    cerr<<"F1-F2: levelSpec:      "<<f1->levelSpec<<endl;
    cerr<<"F1-F2: idnumtext:      "<<f1->idnumtext<<endl;
    cerr<<"F1-F2: idnumSpec:      "<<f1->idnumSpec<<endl;
    cerr<<"F1-F2: progtext:       "<<f1->progtext<<endl;
    cerr<<"F1-F2: timetext:       "<<f1->timetext<<endl;
    cerr<<"-----------------------------------------------------"<<endl;
#endif
  bool ok=   fieldManager->makeDifferenceFields(fv,fv2); 
  if(!ok) return false;

  return true;

}

void FieldPlotManager::getFieldGroups(const miString& modelNameRequest,
				  miString& modelName,
				  vector<FieldGroupInfo>& vfgi)
{
  fieldManager->getFieldGroups(modelNameRequest, modelName, vfgi);

  //replace fieldnames with plotnames
  for(int i=0;i<vfgi.size();i++){

    //sort fieldnames and suffixes 
    map<miString, vector<miString> > fieldName_suffix;
    for(int l=0; l<vfgi[i].fieldNames.size();l++){
      miString suffix;
      miString fieldName = vfgi[i].fieldNames[l];
      splitSuffix(fieldName,suffix);
      fieldName_suffix[fieldName].push_back(suffix);
    }

    //find plotNames
    vector<miString> plotNames;
    for(int j=0;j<vPlotField.size();j++){
      miString plotName = vPlotField[j].name;
      //check that all fields needed exist with same suffix
      map<miString,int> suffixmap;
      for(int k=0;k<vPlotField[j].input.size();k++){
	miString fieldName = vPlotField[j].input[k];
	if(!fieldName_suffix.count(fieldName)) {
	  break;
	}
	for(int l=0;l<fieldName_suffix[fieldName].size();l++){
	  suffixmap[fieldName_suffix[fieldName][l]]+=1;
	}
      }

      //add plotNames without suffix
      if(suffixmap[""]>=vPlotField[j].input.size()){
	plotNames.push_back(plotName);
      }

      //add plotNames with suffix
      set<miString>::iterator p;
      for(p=fieldsuffixes.begin();p!=fieldsuffixes.end();p++){
	if(suffixmap[*p]>=vPlotField[j].input.size()){
	  miString pN = plotName+*p;
	  plotNames.push_back(pN);
	}
      }
    }
    vfgi[i].fieldNames=plotNames;
      
  }

  
  
}


void FieldPlotManager::getAllFieldNames(vector<miString> & fieldNames,
				      set<miString>& fprefixes,
				      set<miString>& fsuffixes)
{

  fieldNames = getPlotFields();  
  fprefixes = fieldprefixes;
  fsuffixes = fieldsuffixes;

}


bool FieldPlotManager::splitSuffix(miString& plotName,
				   miString& suffix)
{

  set<miString>::const_iterator ps=fieldsuffixes.begin();
  for(;ps!=fieldsuffixes.end();ps++){
    if(plotName.contains(*ps)){
      suffix=*ps;
      plotName.replace(suffix,"");
      return true;
    }
  }

  return false;
}

bool FieldPlotManager::parsePin(const miString& pin,
				miString& modelName,
				miString& plotName,
				vector<miString>& fieldName,
				miString& levelName,
				miString& idnumName,
				int& hourOffset,
				int& hourDiff,
				miTime& time)
{

  vector<miString> tokens= pin.split('"','"');
  int n= tokens.size();

  // at least FIELD <modelName> <plotName>
  if (n<3) return false;

  modelName= tokens[1];
  plotName= tokens[2].downcase();

  //if pin contains time, replace ptime
  int i=0;
  while(i<n && !tokens[i].downcase().contains("time=")) i++;
  if(i<n){
    vector<miString> stokens=tokens[i].split("=");
    if(stokens.size()==2){
      time = miTime(stokens[1]);
    }
  }


  vector<miString> vtoken;
  miString str,key;
  hourOffset=0;
  hourDiff=0;

  for (int k=3; k<n; k++) {
    vtoken= tokens[k].split('=');
    if (vtoken.size()>=2) {
      key= vtoken[0].downcase();
      if (key=="level") {
	levelName = vtoken[1];
      } else if (key=="idnum") {
	idnumName = vtoken[1];
      } else if (key=="hour.offset") {
	hourOffset = atoi(vtoken[1].cStr());
      } else if (key=="hour.diff") {
	hourDiff = atoi(vtoken[1].cStr());
      }
    }
  }


  //plotName -> fieldName

  miString suffix;
  splitSuffix(plotName,suffix);

  if(!mapPlotField.count(plotName)) return false;
  for(int i=0;i<mapPlotField[plotName].input.size();i++){
    miString fName=mapPlotField[plotName].input[i].downcase();
    fName += suffix;
    fieldName.push_back(fName);
  }
  
  plotName += suffix;

}
