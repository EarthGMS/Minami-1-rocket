#pragma once
#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>
#include <QFile>
#include <QDateTime>
#include <QVector>

class QLabel;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QComboBox;
class QLineEdit;
class QCheckBox;
class PlotWidget;
class QPlainTextEdit;

struct Telemetry {
    double altitude = 0.0, temperature = 25.0, pressure = 1013.25;
    double battery = 100.0, velocity = 0.0, latitude = 0.0, longitude = 0.0;
    double ax = 0.0, ay = 0.0, az = 9.81;
    QString status = "READY";
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
private slots:
    void updateSimulation();
    void readSerial();
    void connectSerial();
    void disconnectSerial();
    void startLogging();
    void stopLogging();
    void exportCsv();
    void startSimulation();
    void stopSimulation();
    void simulatedManualLand();
    void simulatedAutoLand();
    void resetSimulation();
    void navigate(int page);
private:
    QWidget *buildHomePage();
    QWidget *buildTelemetryPage();
    QWidget *buildLoggingPage();
    QWidget *buildSensorsPage();
    QWidget *buildLandingPage(bool automatic);
    QWidget *buildSettingsPage();
    QWidget *makeCard(const QString &title, QLabel **value, const QString &unit);
    QPushButton *makeNavButton(const QString &text, int page);
    void applyStyle();
    void refreshUi();
    void updateSensorTable();
    void appendLogRow();
    void parseTelemetryLine(const QByteArray &line);
    void setNotice(const QString &message);
    QString csvHeader() const;
    QString csvRow() const;

    Telemetry m_data;
    QSerialPort m_serial;
    QTimer m_simTimer;
    QTimer m_clockTimer;
    bool m_simulation = true;
    bool m_logging = false;
    QFile m_logFile;
    QString m_logPath;
    double m_simTime = 0.0;
    QVector<double> m_altHistory, m_tempHistory;

    QStackedWidget *m_pages = nullptr;
    QLabel *m_connectionLabel = nullptr;
    QLabel *m_clockLabel = nullptr;
    QLabel *m_altValue = nullptr, *m_tempValue = nullptr, *m_batteryValue = nullptr;
    QLabel *m_statusValue = nullptr, *m_velocityValue = nullptr, *m_notice = nullptr;
    QLabel *m_modeLabel = nullptr;
    PlotWidget *m_altPlot = nullptr, *m_telemetryPlot = nullptr;
    QTableWidget *m_logTable = nullptr, *m_sensorTable = nullptr;
    QComboBox *m_portCombo = nullptr, *m_baudCombo = nullptr, *m_modeCombo = nullptr;
    QLineEdit *m_logPathEdit = nullptr;
    QPushButton *m_startBtn = nullptr, *m_stopBtn = nullptr;
    QPushButton *m_manualBtn = nullptr, *m_autoBtn = nullptr;
    QPushButton *m_connectBtn = nullptr;
    QPlainTextEdit *m_rawTelemetry = nullptr;
};
