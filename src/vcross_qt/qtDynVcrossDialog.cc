
#include "qtDynVcrossDialog.h"

#include <QtCore/QRegExp>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QIcon>
#include <QtGui/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>

namespace {
#include "vcross.xpm"
}

#define MILOGGER_CATEGORY "diana.DynVcrossDialog"
#include <miLogger/miLogging.h>

DynVcrossDialog::DynVcrossDialog(QWidget* parent, Qt::WindowFlags f)
  : QDialog(parent, f)
{
  setWindowTitle(tr("Dynamic crossections"));
  setWindowIcon(QIcon(QPixmap(vcross_xpm)));

  QLabel* label = new QLabel(tr("&Enter crossections here:"));
  mEditor = new QTextEdit(this);
  label->setBuddy(mEditor);
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
      Qt::Horizontal, this);

  QVBoxLayout* box = new QVBoxLayout(this);
  setLayout(box);
  box->addWidget(label);
  box->addWidget(mEditor, 1);
  box->addWidget(buttons);

  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

DynVcrossDialog::LabelledCrossection_v DynVcrossDialog::extractCrossections() const
{
  return extractCrossections(mEditor->toPlainText());
}

// static
DynVcrossDialog::LabelledCrossection_v DynVcrossDialog::extractCrossections(const QString& text)
{
  METLIBS_LOG_SCOPE(LOGVAL(text.toStdString()));

  LabelledCrossection_v lcs;

  if (text.startsWith("Number of items")) {
    QStringList lines = text.split(QChar('\n'));
    QRegExp latlon("\\(([\\d.+-]+), ?([\\d.+-]+)\\)");
    for (int l=1; l<lines.size(); ++l) {
      LabelledCrossection lc;

      const QString& line = lines.at(l);
      for (int from = 0; (from = line.indexOf(latlon, from)) >= 0; from += latlon.matchedLength()) {
        const QStringList captures = latlon.capturedTexts();
        const double londeg = captures.at(2).toDouble(), latdeg = captures.at(1).toDouble();
        METLIBS_LOG_DEBUG(LOGVAL(londeg) << LOGVAL(latdeg));
        lc.points.push_back(LonLat::fromDegrees(londeg, latdeg));
      }

      if (lc.points.size() >= 2) {
        lc.label = QString("dyn_%1_%2").arg(lc.points[0].lonDeg()).arg(lc.points[0].latDeg());
        lcs.push_back(lc);
        METLIBS_LOG_DEBUG(LOGVAL(lc.label.toStdString()) << LOGVAL(lc.points.size()));
      }
    }
  }
  return lcs;
}
