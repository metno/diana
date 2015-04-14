
#ifndef WebMapDialog_h
#define WebMapDialog_h 1

#include "qtDataDialog.h"

class WebMapDialog : public DataDialog
{
public:
  WebMapDialog(QWidget *parent, Controller *ctrl);
  ~WebMapDialog();

  std::string name() const;
  void updateDialog();
  std::vector<std::string> getOKString();
  void putOKString(const std::vector<std::string>& vstr);

public /*Q_SLOTS*/:
  void updateTimes();

private:
  std::vector<std::string> mOk;
};

#endif // WebMapDialog_h
