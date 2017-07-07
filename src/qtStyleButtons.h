#ifndef QTSTYLEBUTTONS_H
#define QTSTYLEBUTTONS_H

#include <QPushButton>

class LineStyleButton : public QPushButton {
  Q_OBJECT

public:
  LineStyleButton(QWidget* parent=0);

  /*! What the line style applies to, e.g. "frame". */
  void setWhat(const QString& w);

  void enableColor(bool on)
    { m_enableColor = on; }

  void enableWidth(bool on)
    { m_enableWidth = on; }

  void enableType(bool on)
    { m_enableType = on; }

  void setLineColor(const std::string& c);
  void setLineWidth(const std::string& lt);
  void setLineType(const std::string& lt);

  const std::string& lineColor() const
    { return m_linecolor; }
  const std::string& lineWidth() const
    { return m_linewidth; }
  const std::string& lineType() const
    { return m_linetype; }

Q_SIGNALS:
  void changed();

private Q_SLOTS:
  void showEditor();

private:
  void updatePixmap();

private:
  QString m_what;

  bool m_enableColor;
  bool m_enableWidth;
  bool m_enableType;

  std::string m_linecolor;
  std::string m_linewidth;
  std::string m_linetype;
};

#endif // QTSTYLEBUTTONS_H
