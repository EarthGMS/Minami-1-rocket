#pragma once

#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>
#include <QFile>
#include <QWidget>
#include <QElapsedTimer>
#include <QPainter>
#include <QPainterPath>
#include <QByteArray>
#include <array>
#include <deque>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QLabel;

struct Telemetry {
    double altitude = 0, speed = 0, battery = 100, roll = 0, pitch = 0, yaw = 0;
    double value(int ch) const {
        switch (ch) {
        case 0: return altitude;
        case 1: return speed;
        case 2: return battery;
        case 3: return roll;
        case 4: return pitch;
        default: return yaw;
        }
    }
};

// Real-time scrolling line plot (QPainter only, no extra Qt modules needed)
class PlotWidget : public QWidget {
public:
    explicit PlotWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(240);
        m_clock.start();
        m_repaint.setInterval(50); // ~20 fps
        connect(&m_repaint, &QTimer::timeout, this, qOverload<>(&QWidget::update));
        m_repaint.start();
    }

    void setChannel(int ch, const QString &name) { m_channel = ch; m_name = name; m_data.clear(); update(); }
    void setWindowSeconds(int s) { m_window = qMax(5, s); }
    void clear() { m_data.clear(); update(); }

    void addSample(const Telemetry &t) {
        const double now = m_clock.elapsed() / 1000.0;
        m_data.push_back({now, t.value(m_channel)});
        while (!m_data.empty() && now - m_data.front().first > m_window) m_data.pop_front();
        if (m_data.size() > 20000) m_data.pop_front();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#1e1e1e"));

        const QRectF plot(55, 15, width() - 70, height() - 40);
        p.setPen(QColor("#555"));
        p.drawRect(plot);

        const double now = m_clock.elapsed() / 1000.0;
        double lo = 0, hi = 1;
        if (!m_data.empty()) {
            lo = hi = m_data.front().second;
            for (const auto &s : m_data) { lo = qMin(lo, s.second); hi = qMax(hi, s.second); }
            double pad = qMax((hi - lo) * 0.1, 0.5);
            lo -= pad; hi += pad;
        }

        p.setFont(QFont("sans", 8));
        for (int i = 0; i <= 4; ++i) {
            double y = plot.bottom() - plot.height() * i / 4.0;
            p.setPen(QColor("#333"));
            p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
            p.setPen(QColor("#aaa"));
            p.drawText(QRectF(0, y - 8, plot.left() - 5, 16), Qt::AlignRight | Qt::AlignVCenter,
                       QString::number(lo + (hi - lo) * i / 4.0, 'f', 1));
        }
        for (int i = 0; i <= 4; ++i) {
            double x = plot.left() + plot.width() * i / 4.0;
            p.setPen(QColor("#333"));
            p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
            p.setPen(QColor("#aaa"));
            p.drawText(QRectF(x - 25, plot.bottom() + 3, 50, 14), Qt::AlignCenter,
                       QString("-%1s").arg(m_window - m_window * i / 4));
        }

        p.setPen(QColor("#ddd"));
        p.drawText(QPointF(plot.left() + 6, plot.top() + 14), m_name);

        if (m_data.size() >= 2) {
            QPainterPath path;
            bool first = true;
            for (const auto &s : m_data) {
                double x = plot.right() - (now - s.first) / m_window * plot.width();
                double y = plot.bottom() - (s.second - lo) / (hi - lo) * plot.height();
                if (first) { path.moveTo(x, y); first = false; } else path.lineTo(x, y);
            }
            p.setClipRect(plot);
            p.setPen(QPen(QColor("#4fc3f7"), 2));
            p.drawPath(path);
        }
    }

private:
    std::deque<std::pair<double, double>> m_data;
    QElapsedTimer m_clock;
    QTimer m_repaint;
    int m_channel = 0;
    int m_window = 30;
    QString m_name = "Altitude (m)";
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // telemetry setup
    void refreshPorts();
    void toggleConnection();
    // manual land
    void onManualLand();
    // incoming data
    void onSerialData();
    void onSimTick();
    // logging
    void browseLogFile();
    void toggleLogging();

private:
    void disconnectLink();
    void parseLine(const QByteArray &line);
    void updateValues(const Telemetry &t);
    void startLogging();
    void stopLogging();
    void writeLog(const Telemetry &t);
    void appendLog(const QString &msg);

    Ui::MainWindow *ui;
    PlotWidget *m_plot = nullptr;
    std::array<QLabel *, 6> m_valueLabels{};

    QSerialPort m_serial;
    QTimer m_simTimer;
    QByteArray m_buffer;
    bool m_connected = false, m_landing = false;
    double m_simT = 0, m_simAlt = 50, m_simBat = 100;

    QFile m_logFile;
    qint64 m_logRows = 0;
};
