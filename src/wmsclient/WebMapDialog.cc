
#include "WebMapDialog.h"

#include <QAction>

WebMapDialog::WebMapDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  m_action = new QAction("Web Maps", this);
}

WebMapDialog::~WebMapDialog()
{
}

std::string WebMapDialog::name() const
{
  static const std::string WEBMAP = "WEBMAP";
  return WEBMAP;
}

void WebMapDialog::updateDialog()
{
}

std::vector<std::string> WebMapDialog::getOKString()
{
  return mOk;
}

void WebMapDialog::putOKString(const std::vector<std::string>& ok)
{
  mOk = ok;
}

void WebMapDialog::updateTimes()
{
}
