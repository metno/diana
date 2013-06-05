#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QVBoxLayout>
#include <QLabel>


#define MILOGGER_CATEGORY "diana.ProfetSingleControl"
#include <miLogger/miLogging.h>

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
  button->setMaximumWidth(30);
  button->setFlat(true);
  QLabel * lab= new QLabel(QString::fromStdString(data.validTime.format("%a %H")),this);
  vb->addWidget(lab);
  vb->addWidget(button);
  
  if(data.isParent()) 
    button->setIcon(QIcon(parent_object_xpm));

  
  connect(button,SIGNAL(pressed()),this,SLOT(buttonPressed()));
  
 
  map<miutil::miString,float>::iterator itr=data.parameters.begin();
  for(;itr!=data.parameters.end();itr++) {
    if(!data.guiComponents.count(itr->first) ){
      METLIBS_LOG_ERROR("could not build GUI for timeSmooth - guiComponents missinng for parameter: "
      << itr->first);
      continue;
    }
    fetDynamicGui::GuiComponent guic=data.guiComponents[itr->first];
    scale[itr->first] = (guic.stepvalue ? 1/guic.stepvalue : 1);    
    miSliderWidget * mySlider = new miSliderWidget(
        guic.minvalue,
        guic.maxvalue,
        guic.stepvalue,
        itr->second, 
        Qt::Vertical,
        itr->first,
        "",
        false,
        this);
    slider[itr->first] = mySlider;
    connect(slider[itr->first],SIGNAL(valueChangedForPar(float,std::string)),this,
        SLOT(valueChangedBySlider(float,std::string)));

    vb->addWidget(slider[itr->first]);
  }
  
  processed(data.id);
}

void ProfetSingleControl::set(fetObject::TimeValues tv)
{
  if(data.id.exists()) tv.id=data.id;
  data=tv;
  map<miutil::miString,float>::iterator itr=data.parameters.begin();
   for(;itr!=data.parameters.end();itr++) 
     setValue(itr->first,itr->second);
}

void ProfetSingleControl::processed(miutil::miString newid)
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

void ProfetSingleControl::setValue(miutil::miString par, float value)
{
  if(slider.count(par)) {
    value = float(int(value*scale[par]))/scale[par];
    
    slider[par]->setValue(value);
    valueChanged(value,par);
  }
  
}

void ProfetSingleControl::resetValue(miutil::miString par)
{
  if(!data.valuesForZeroImpact.count(par)) return;
  float z=data.valuesForZeroImpact[par];
  setValue(par,z);
  
}


void ProfetSingleControl::valueChangedBySlider(float v, std::string par)
{
  emit pushundo();
  valueChanged(v, par);
}
    
    
    
void ProfetSingleControl::valueChanged(float v, std::string par)
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
     
   



