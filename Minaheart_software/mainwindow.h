#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QFile>
#include <QVector>
#include <QString>
#include <QByteArray>

class QLabel;
class QPushButton;
class QTableWidget;
class QProgressBar;
class QWidget;
class QVBoxLayout;
class QStackedWidget;
class QComboBox;
class QSerialPort;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateTelemetry();
    void manualLand();
    void resetSimulation();
    void toggleLogging();
    void startSimulation();
    void stopSimulation();
    void toggleManualMode();
    void refreshSerialPorts();
    void connectArduino();
    void disconnectArduino();
    void readArduinoData();
    void processArduinoLine(const QByteArray &line);

private:
    QTimer *timer;
    QFile *logFile;
    QLabel *altitudeValue, *temperatureValue, *batteryValue, *statusValue;
    QLabel *velocityValue, *connectionValue, *systemState, *visualAltitudeValue;
    QLabel *pressureValue, *gpsValue, *accelValue, *logState;
    QPushButton *startButton, *stopButton, *landButton, *manualButton, *resetButton, *logButton;
    QTableWidget *logTable;
    QSerialPort *serialPort;
    QComboBox *portCombo;
    QComboBox *baudCombo;
    QLabel *serialStatusLabel;
    QByteArray serialBuffer;
    bool hardwareMode;
    double latitude, longitude, accelX, accelY, accelZ;
    QStackedWidget *pageStack;
    QVector<QPushButton*> navigationButtons;
    QWidget *chartWidget;
    QWidget *rocketWidget;
    QProgressBar *batteryBar;
    double altitude, temperature, battery, velocity, pressure;
    int elapsedSeconds;
    bool landed, paused, manualMode, loggingEnabled;
    QVector<double> altitudeHistory;

    QWidget *makeCard(const QString &title, QLabel **valueLabel, const QString &accent);
    void updateDashboard();
    void appendLogRow();
    void writeCsvRow();
    QPushButton *addNavigationItem(QVBoxLayout *layout, const QString &icon, const QString &label, bool active = false);
    void showPage(int index);
};

#endif
