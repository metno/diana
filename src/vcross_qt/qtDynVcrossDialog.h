
#ifndef diana_DynVcrossDialog_h
#define diana_DynVcrossDialog_h 1

#include <diField/VcrossData.h>

#include <QtGui/QDialog>
class QTextEdit;

#include <vector>

class DynVcrossDialog : public QDialog
{
public:
  DynVcrossDialog(QWidget* parent=0, Qt::WindowFlags f = 0);

  struct LabelledCrossection {
    QString label;
    vcross::LonLat_v points;
  };
  typedef std::vector<LabelledCrossection> LabelledCrossection_v;

  LabelledCrossection_v extractCrossections() const;

  //! Try to parse text into some crossections
  static LabelledCrossection_v extractCrossections(const QString& text);

private:
  QTextEdit* mEditor;
};

#endif // diana_DynVcrossDialog_h
