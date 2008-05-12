#include "qtProfetTimeControl.h"
#include <QLabel>

ProfetTimeControl::ProfetTimeControl(QWidget* parent, vector<fetObject::TimeValues>& obj,vector<miTime>& tim) :
  QWidget(parent)
{
  changed=false;
  method=LINEAR;
  
  fetObject::TimeValues parent_obj;
  
  map<miTime,fetObject::TimeValues> obj_idx;
  for(int i=0;i<obj.size();i++) {
    obj_idx[ obj[i].validTime ] = obj[i];
      if(obj[i].isParent()) 
        parent_obj=obj[i];
  }
  
  
  int cols = tim.size();
  int rows = 2;
  int col;
  if (!tim.empty()) {

    gridl = new QGridLayout(this);

    miString dayname = tim[0].format("%A %d.","no");
    int day = tim[0].day();
    int startcol = 0;

    QFont dfont = parent->font( );
    dfont.setBold(true);     
    QFont hfont;
    int psize = dfont.pointSize();
    dfont.setPointSize(psize+4);

      
    bool nextday=false;

    for (col=0; col<cols; col++) {
      time_index[ tim[col] ] = col;
      
      
      if (tim[col].day() != day || col==cols-1) {
        QLabel * lab = new QLabel(dayname.cStr(),this);
        lab->setFrameStyle(QFrame::Panel | QFrame::Raised);
        lab->setAlignment(Qt::AlignCenter);
        lab->setFont(dfont);
       
        if(col!=cols-1)
          nextday=!nextday;
        
        int lastcol = (col==cols-1 ? cols : col);
    
        gridl->addWidget(lab, 0,startcol, 1, lastcol-startcol );
        startcol = col;
        day = tim[col].day();
        dayname = tim[col].format("%A %d.","no");
      }

     
      ProfetSingleControl *psc;
      if (obj_idx.count(tim[col])) {
         psc= new ProfetSingleControl(this,obj_idx[tim[col]],col);
        if(obj_idx[tim[col]].isParent())
            parenttimestep=col;     
      }
      else
        psc= new ProfetSingleControl(this,parent_obj.child(tim[col]),col);
      objects.push_back(psc);
      gridl->addWidget(objects[col], 1, col,Qt::AlignCenter);
      connect(objects[col],SIGNAL(buttonAtPressed(int)),this,SLOT(buttonAtPressed(int)));
      connect(objects[col],SIGNAL(pushundo()),this, SLOT(pushundo()));
    }
  }
}

void ProfetTimeControl::processed(miTime tim, miString obj_id)
{
  
  if(time_index.count(tim)) {
    int i=time_index[tim];
    objects[i]->processed(obj_id);
  } 
}


void ProfetTimeControl::buttonAtPressed(int col)
{
    if (col == parenttimestep) return;
    
     pushundo();
    
    fetObject::TimeValues data=objects[parenttimestep]->Data();
    map<miString,float>::iterator itr=data.parameters.begin();
    for(;itr!=data.parameters.end();itr++) {
        miString par=itr->first;
        interpolate(par,col);  
    } 
      
}


vector<fetObject::TimeValues> ProfetTimeControl::collect(bool removeDiscardables)
{
  vector<fetObject::TimeValues> co;

  for ( int i=0; i<objects.size();i++) {
    fetObject::TimeValues data = objects[i]->Data();
    if(removeDiscardables && data.isDiscardable())
      continue;
    co.push_back(objects[i]->Data());
  }
  return co;
}




void ProfetTimeControl::setAll(vector<fetObject::TimeValues> a)
{
  for(int i=0;i<a.size();i++) {
    if(time_index.count(a[i].validTime))
      objects[ time_index[ a[i].validTime] ]->set(a[i]);
  }
}

void ProfetTimeControl::pushundo()
{
  changed=true;
  if(redobuffer.size())
      redobuffer.clear();
  undobuffer.push(collect());
}

bool ProfetTimeControl::undo()
{
  if(!undobuffer.size())
      return false;

    redobuffer.push(collect());
    setAll(undobuffer.pop());

    return true;
  
}
bool ProfetTimeControl::redo()
{
  if(!redobuffer.size())
    return false;

  undobuffer.push(collect());
  setAll(redobuffer.pop());
 
  return true;
}


void ProfetTimeControl::clearline(int from,int to,miString par)
{
  for(int i=from; i<to;i++)
    objects[i]->resetValue(par);
}

void ProfetTimeControl::interpolate(miString par, int col)
{
  if(parameters.count(par)) return;
  
  int from     = (col>parenttimestep ? parenttimestep+1 : col           );
  int to       = (col>parenttimestep ? col+1 : parenttimestep           );
  int fromall  = (col>parenttimestep ? parenttimestep+1 : 0             );
  int toall    = (col>parenttimestep ? objects.size()  : parenttimestep );
  
  if(method==RESETLINE) {
    clearline(fromall,toall,par);
    return;
  }
  
  if(method==COPY ) {
    float value=objects[parenttimestep]->value(par);
    clearline(fromall,toall,par);
    for(int i=from; i<to;i++) {
      objects[i]->setValue(par,value);  
    }
    return;
  }
  
  if(method==RESETSINGLE) {
    objects[col]->resetValue(par);
    return;
  }
  
  if(method==LINEAR) {
    clearline(fromall,toall,par);
    float fromvalue;
    float tovalue;
    miTime a;
    miTime b;
    if(col>parenttimestep) {
      fromvalue = objects[parenttimestep]->value(par);
      tovalue   = objects[parenttimestep]->zero(par);
      a=objects[parenttimestep]->time();
      if(to>=toall-1) {
        b        = objects[to-1]->time();
        miTime c = objects[to-2]->time();

        int aa=miTime::hourDiff(b,c);

        b.addHour(aa);
      } else
        b=objects[to]->time();

    } else {
      tovalue   = objects[parenttimestep]->value(par);
      fromvalue = objects[parenttimestep]->zero(par);
      b         = objects[to]->time();
      
      if(from==0) {
        a        = objects[from]->time();
        miTime c = objects[from+1]->time();
        int aa=miTime::hourDiff(a,c);
        a.addHour(aa);
      } else  
        a=objects[from-1]->time();
    }    

    float totaltime=float(miTime::minDiff(b,a));            

    for(int i=from; i<to;i++) {
      float done=float(miTime::minDiff(objects[i]->time(),a))/totaltime;
      float value=fromvalue-(fromvalue-tovalue)*done;
      objects[i]->setValue(par,value);  
    }


  }

  if(method==GAUSS) {
      cout << "GAUSS" << endl;
  } 
}

void ProfetTimeControl::toggleParameters(miString p)
{
  if(parameters.count(p)) {
    parameters.erase(p);
  } else
    parameters.insert(p);
  
}

void ProfetTimeControl::setMethod(ProfetTimeControl::methodTypes m)
{
  method=m;  
}
  
ProfetSingleControl* ProfetTimeControl::focusObject() const
{
  if(objects.empty()) return 0;
  
  if(parenttimestep > 3) 
    return objects[parenttimestep-3]; 
  return objects[0];
}
