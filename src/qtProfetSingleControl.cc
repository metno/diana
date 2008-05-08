#include <QVBoxLayout>


#include "qtProfetSingleControl.h"


#include <QIcon>

#include "active_object.xpm"
#include "inactive_object.xpm"
#include "no_object.xpm"
#include "parent_object.xpm"



ProfetSingleControl::ProfetSingleControl(QWidget* p, fetObject::TimeValues tv, int col)
 : QFrame(p) , column(col)
{
  data=tv;  
  
  
  setLineWidth(3);
  setMidLineWidth(1);
  isSunken=!data.isParent();
  if(data.isParent()) {
    setFrameStyle(QFrame::Panel | QFrame::Raised);
  } else
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
 
  
  QVBoxLayout* vb = new QVBoxLayout(this);
  
  vb->setAlignment(Qt::AlignHCenter);
  
  button=new QPushButton(this);
  QString a;
  a.setNum(data.validTime.hour());
  button->setText(a);
  vb->addWidget(button);
  
  if(data.isParent()) 
    button->setIcon(QIcon(parent_object_xpm));

  
  connect(button,SIGNAL(pressed()),this,SLOT(buttonPressed()));
  
 
  map<miString,float>::iterator itr=data.parameters.begin();
  for(;itr!=data.parameters.end();itr++) {
      miSliderWidget * mySlider = new miSliderWidget(-10,10,1,itr->second, 
          Qt::Vertical,
          itr->first,"m/s",
          false,
          this);
      slider[itr->first] = mySlider;
      connect(slider[itr->first],SIGNAL(valueChangedForPar(float,miString)),this,
          SLOT(valueChangedBySlider(float,miString)));
      
      vb->addWidget(slider[itr->first]);
  }
  
  processed(data.id);
}

void ProfetSingleControl::set(fetObject::TimeValues tv)
{
  if(data.id.exists()) tv.id=data.id;
  data=tv;
  map<miString,float>::iterator itr=data.parameters.begin();
   for(;itr!=data.parameters.end();itr++) 
     setValue(itr->first,itr->second);
}

void ProfetSingleControl::processed(miString newid)
{
  data.id=newid;  
  if(data.isParent())
    return;
  if(data.id.exists()) {
     data.valuesInRealObject=data.parameters;
     button->setIcon(QIcon(active_object_xpm));
  } else {
    button->setIcon(QIcon(no_object_xpm));
  }
}

void ProfetSingleControl::buttonPressed()
{
  emit buttonAtPressed(column);
}

void ProfetSingleControl::setValue(miString par, float value)
{
  if(slider.count(par)) {
    value = float(int(value*100))/100;
    
    slider[par]->setValue(value);
    valueChanged(value,par);
  }
  
}

void ProfetSingleControl::resetValue(miString par)
{
  if(!data.valuesForZeroImpact.count(par)) return;
  float z=data.valuesForZeroImpact[par];
  setValue(par,z);
  
}


void ProfetSingleControl::valueChangedBySlider(float v, miString par)
{
  emit pushundo();
  valueChanged(v, par);
}
    
    
    
void ProfetSingleControl::valueChanged(float v, miString par)
{
  if(!data.parameters.count(par))
    return;
   data.parameters[par]=v;
   
   if(data.isParent()) return;

    if(data.hasZeroImpact() != isSunken ) {
      if(isSunken)
        setFrameStyle(QFrame::Panel | QFrame::Raised);
      else
        setFrameStyle(QFrame::Panel | QFrame::Sunken);
      isSunken = !isSunken;
    }  
   
    if(data.id.exists()) {  
      if(data.unchanged())
        button->setIcon(QIcon(active_object_xpm));
      else
        button->setIcon(QIcon(inactive_object_xpm));
    }   
}
     
   



