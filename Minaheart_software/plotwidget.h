#pragma once
#include <QWidget>
#include <QVector>
#include <QColor>
#include <QString>

class PlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(const QString &title, const QString &unit,
                        QWidget *parent = nullptr);
    void addValue(double value);
    void setSeries(const QVector<double> &values);
    void setLineColor(const QColor &color);
    void setMaximumPoints(int count);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QString m_title, m_unit;
    QVector<double> m_values;
    QColor m_lineColor;
    int m_maxPoints = 100;
};
