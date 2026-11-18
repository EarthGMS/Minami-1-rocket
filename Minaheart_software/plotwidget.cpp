#include "plotwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

PlotWidget::PlotWidget(const QString &title, const QString &unit, QWidget *parent)
    : QWidget(parent), m_title(title), m_unit(unit), m_lineColor( QColor("#25c9ff") )
{
    setMinimumHeight(170);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void PlotWidget::addValue(double value)
{
    m_values.push_back(value);
    while (m_values.size() > m_maxPoints) m_values.removeFirst();
    update();
}

void PlotWidget::setSeries(const QVector<double> &values)
{
    m_values = values;
    while (m_values.size() > m_maxPoints) m_values.removeFirst();
    update();
}

void PlotWidget::setLineColor(const QColor &color) { m_lineColor = color; update(); }
void PlotWidget::setMaximumPoints(int count) { m_maxPoints = qMax(2, count); }

void PlotWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#07182d"));
    p.setPen(QColor("#153654"));
    p.drawRoundedRect(rect().adjusted(1,1,-2,-2), 10, 10);
    p.setPen(QColor("#e5f4ff"));
    QFont f = p.font(); f.setBold(true); f.setPointSize(10); p.setFont(f);
    p.drawText(14, 22, m_title);
    const QRectF area(42, 36, width()-58, height()-66);
    if (area.width() <= 0 || area.height() <= 0) return;
    p.setPen(QPen(QColor("#173451"), 1, Qt::DashLine));
    for (int i=0; i<=4; ++i) {
        const qreal y = area.top() + area.height()*i/4.0;
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }
    if (m_values.size() < 2) {
        p.setPen(QColor("#8aa8c3"));
        p.drawText(area, Qt::AlignCenter, "Waiting for telemetry...");
        return;
    }
    auto mm = std::minmax_element(m_values.cbegin(), m_values.cend());
    double lo = *mm.first, hi = *mm.second;
    if (std::abs(hi-lo) < 0.001) { hi += 1.0; lo -= 1.0; }
    const double pad = (hi-lo)*0.12;
    lo -= pad; hi += pad;
    QPainterPath path;
    for (int i=0; i<m_values.size(); ++i) {
        const qreal x = area.left() + area.width()*i/(m_values.size()-1.0);
        const qreal y = area.bottom() - (m_values[i]-lo)/(hi-lo)*area.height();
        if (i==0) path.moveTo(x,y); else path.lineTo(x,y);
    }
    QPainterPath fill = path;
    fill.lineTo(area.bottomRight()); fill.lineTo(area.left(), area.bottom()); fill.closeSubpath();
    p.fillPath(fill, QColor(m_lineColor.red(),m_lineColor.green(),m_lineColor.blue(),35));
    p.setPen(QPen(m_lineColor, 2.4)); p.drawPath(path);
    p.setPen(QColor("#9db8d0"));
    p.setFont(QFont("Segoe UI", 8));
    p.drawText(4, area.top()+4, QString::number(hi,'f',1));
    p.drawText(4, area.bottom(), QString::number(lo,'f',1));
    p.drawText(QRectF(0,height()-22,width(),16), Qt::AlignCenter, m_unit);
}
