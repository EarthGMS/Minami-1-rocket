#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QTextStream>
#include <QtMath>

static const char *kValueStyle = "font-size:20px;font-weight:bold;";
static const char *kValueStyleLow = "font-size:20px;font-weight:bold;color:#d32f2f;";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // value labels in channel order
    m_valueLabels = {ui->lblAltitude, ui->lblSpeed, ui->lblBattery,
                     ui->lblRoll, ui->lblPitch, ui->lblYaw};

    // baud rates
    for (int b : {9600, 19200, 38400, 57600, 115200, 230400})
        ui->comboBaud->addItem(QString::number(b), b);
    ui->comboBaud->setCurrentText("57600");

    // graph widget goes into the placeholder layout from the .ui file
    m_plot = new PlotWidget;
    ui->plotLayout->addWidget(m_plot);

    // default log file name
    ui->editPath->setText(QString("telemetry_%1.csv")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));

    // connections
    connect(ui->btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(ui->btnLand, &QPushButton::clicked, this, &MainWindow::onManualLand);
    connect(ui->btnBrowse, &QPushButton::clicked, this, &MainWindow::browseLogFile);
    connect(ui->btnLog, &QPushButton::clicked, this, &MainWindow::toggleLogging);
    connect(ui->btnClearPlot, &QPushButton::clicked, m_plot, &PlotWidget::clear);
    connect(ui->comboChannel, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int i) { m_plot->setChannel(i, ui->comboChannel->itemText(i)); });
    connect(ui->spinWindow, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int s) { m_plot->setWindowSeconds(s); });
    connect(&m_serial, &QSerialPort::readyRead, this, &MainWindow::onSerialData);
    connect(&m_simTimer, &QTimer::timeout, this, &MainWindow::onSimTick);

    statusBar()->showMessage("Disconnected");
    refreshPorts();
}

MainWindow::~MainWindow() {
    stopLogging();
    delete ui;
}

// ---------- Telemetry setup ----------
void MainWindow::refreshPorts() {
    ui->comboPort->clear();
    for (const auto &p : QSerialPortInfo::availablePorts())
        ui->comboPort->addItem(p.portName() + " - " + p.description(), p.portName());
}

void MainWindow::toggleConnection() {
    if (m_connected) { disconnectLink(); return; }

    if (ui->chkSim->isChecked()) {
        m_simT = 0;
        m_simTimer.start(100);
    } else {
        if (ui->comboPort->currentIndex() < 0) {
            QMessageBox::warning(this, "Port", "No serial port selected.");
            return;
        }
        m_serial.setPortName(ui->comboPort->currentData().toString());
        m_serial.setBaudRate(ui->comboBaud->currentData().toInt());
        if (!m_serial.open(QIODevice::ReadWrite)) {
            QMessageBox::critical(this, "Error", m_serial.errorString());
            return;
        }
    }
    m_connected = true;
    ui->btnConnect->setText("Disconnect");
    ui->comboPort->setEnabled(false);
    ui->comboBaud->setEnabled(false);
    ui->chkSim->setEnabled(false);
    ui->btnLand->setEnabled(true);
    statusBar()->showMessage(ui->chkSim->isChecked() ? "Connected (simulation)" : "Connected");
    appendLog("Connected");
}

void MainWindow::disconnectLink() {
    m_simTimer.stop();
    if (m_serial.isOpen()) m_serial.close();
    m_connected = false;
    ui->btnConnect->setText("Connect");
    ui->comboPort->setEnabled(true);
    ui->comboBaud->setEnabled(true);
    ui->chkSim->setEnabled(true);
    ui->btnLand->setEnabled(false);
    statusBar()->showMessage("Disconnected");
    appendLog("Disconnected");
}

// ---------- Manual land ----------
void MainWindow::onManualLand() {
    auto r = QMessageBox::warning(this, "Manual land",
                                  "Send LAND command to the vehicle now?",
                                  QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;

    if (m_serial.isOpen()) m_serial.write("LAND\n");
    m_landing = true;   // also affects the simulator
    appendLog("LAND command sent");
}

// ---------- Incoming data ----------
void MainWindow::onSerialData() {
    m_buffer += m_serial.readAll();
    int idx;
    while ((idx = m_buffer.indexOf('\n')) >= 0) {
        QByteArray line = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);
        parseLine(line);
    }
}

void MainWindow::parseLine(const QByteArray &line) {
    const auto parts = line.split(',');
    if (parts.size() < 6) return;
    Telemetry t;
    bool ok[6];
    t.altitude = parts[0].toDouble(&ok[0]);
    t.speed    = parts[1].toDouble(&ok[1]);
    t.battery  = parts[2].toDouble(&ok[2]);
    t.roll     = parts[3].toDouble(&ok[3]);
    t.pitch    = parts[4].toDouble(&ok[4]);
    t.yaw      = parts[5].toDouble(&ok[5]);
    for (bool b : ok) if (!b) return;
    updateValues(t);
}

void MainWindow::onSimTick() {
    m_simT += 0.1;
    Telemetry t;
    if (m_landing) {
        m_simAlt = qMax(0.0, m_simAlt - 0.3);
        if (m_simAlt == 0) m_landing = false;
    } else {
        m_simAlt = 50 + 10 * qSin(m_simT * 0.5);
    }
    m_simBat = qMax(0.0, m_simBat - 0.02);
    t.altitude = m_simAlt;
    t.speed = m_landing ? 1.0 : 8 + 2 * qSin(m_simT);
    t.battery = m_simBat;
    t.roll = 5 * qSin(m_simT);
    t.pitch = 3 * qCos(m_simT);
    t.yaw = fmod(m_simT * 10, 360);
    updateValues(t);
}

// ---------- Value monitor ----------
void MainWindow::updateValues(const Telemetry &t) {
    m_valueLabels[0]->setText(QString::number(t.altitude, 'f', 1));
    m_valueLabels[1]->setText(QString::number(t.speed, 'f', 1));
    m_valueLabels[2]->setText(QString::number(t.battery, 'f', 0));
    m_valueLabels[3]->setText(QString::number(t.roll, 'f', 1));
    m_valueLabels[4]->setText(QString::number(t.pitch, 'f', 1));
    m_valueLabels[5]->setText(QString::number(t.yaw, 'f', 1));
    ui->progressBattery->setValue(int(t.battery));

    QStringList warn;
    if (t.battery < 20) warn << "LOW BATTERY";
    if (qAbs(t.roll) > 45 || qAbs(t.pitch) > 45) warn << "EXTREME ATTITUDE";
    ui->lblWarning->setText(warn.join("  |  "));
    m_valueLabels[2]->setStyleSheet(t.battery < 20 ? kValueStyleLow : kValueStyle);

    m_plot->addSample(t);
    writeLog(t);
}

// ---------- Data logging ----------
void MainWindow::browseLogFile() {
    QString f = QFileDialog::getSaveFileName(this, "Log file", ui->editPath->text(), "CSV (*.csv)");
    if (!f.isEmpty()) ui->editPath->setText(f);
}

void MainWindow::toggleLogging() {
    m_logFile.isOpen() ? stopLogging() : startLogging();
}

void MainWindow::startLogging() {
    m_logFile.setFileName(ui->editPath->text());
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", m_logFile.errorString());
        return;
    }
    QTextStream(&m_logFile) << "timestamp,altitude,speed,battery,roll,pitch,yaw\n";
    m_logRows = 0;
    ui->btnLog->setText("Stop logging");
    ui->lblLogInfo->setText("Logging to " + m_logFile.fileName());
    appendLog("Logging started");
}

void MainWindow::stopLogging() {
    if (!m_logFile.isOpen()) return;
    m_logFile.close();
    ui->btnLog->setText("Start logging");
    ui->lblLogInfo->setText(QString("Stopped (%1 rows saved)").arg(m_logRows));
    appendLog("Logging stopped");
}

void MainWindow::writeLog(const Telemetry &t) {
    if (!m_logFile.isOpen()) return;
    QTextStream(&m_logFile)
        << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << ','
        << t.altitude << ',' << t.speed << ',' << t.battery << ','
        << t.roll << ',' << t.pitch << ',' << t.yaw << '\n';
    ++m_logRows;
    ui->lblLogInfo->setText(QString("Logging: %1 rows").arg(m_logRows));
}

void MainWindow::appendLog(const QString &msg) {
    ui->txtLog->appendPlainText(QDateTime::currentDateTime().toString("HH:mm:ss  ") + msg);
}
