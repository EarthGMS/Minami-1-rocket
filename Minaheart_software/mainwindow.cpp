#include "mainwindow.h"
#include "plotwidget.h"

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QProgressBar>
#include <QCheckBox>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextCursor>

static QLabel *valueLabel(const QString &text, QWidget *parent=nullptr)
{
    auto *l = new QLabel(text, parent);
    l->setObjectName("metricValue");
    return l;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Rocket Monitoring System");
    resize(1500, 900);
    applyStyle();

    auto *central = new QWidget;
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(12,12,12,12);
    root->setSpacing(12);

    auto *sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(205);
    auto *side = new QVBoxLayout(sidebar);
    auto *brand = new QLabel("🚀  ROCKET\nMONITORING");
    brand->setObjectName("brand");
    brand->setAlignment(Qt::AlignCenter);
    side->addWidget(brand);
    side->addSpacing(12);
    const QStringList names = {"⌂   Home", "◉   Telemetry", "▤   Data Logging",
                               "▦   Sensor Monitoring", "▱   Manual Land",
                               "◈   Auto Land", "⚙   Settings"};
    for (int i=0; i<names.size(); ++i) side->addWidget(makeNavButton(names[i], i));
    side->addStretch();
    auto *sideFoot = new QLabel("Rocket Monitoring\nSystem v1.0");
    sideFoot->setAlignment(Qt::AlignCenter);
    sideFoot->setStyleSheet("color:#6c91b2;padding:12px;");
    side->addWidget(sideFoot);

    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0,0,0,0);
    auto *header = new QFrame;
    header->setObjectName("header");
    auto *head = new QHBoxLayout(header);
    auto *heading = new QLabel("Rocket Monitoring System");
    heading->setObjectName("pageHeading");
    auto *sub = new QLabel("Real-Time Telemetry & Monitoring Platform");
    sub->setObjectName("muted");
    auto *headLeft = new QVBoxLayout;
    headLeft->addWidget(heading); headLeft->addWidget(sub);
    head->addLayout(headLeft); head->addStretch();
    m_connectionLabel = new QLabel("● SIMULATION");
    m_connectionLabel->setObjectName("connected");
    m_clockLabel = new QLabel;
    m_clockLabel->setObjectName("muted");
    head->addWidget(m_connectionLabel); head->addSpacing(18); head->addWidget(m_clockLabel);
    contentLayout->addWidget(header);

    m_pages = new QStackedWidget;
    m_pages->addWidget(buildHomePage());
    m_pages->addWidget(buildTelemetryPage());
    m_pages->addWidget(buildLoggingPage());
    m_pages->addWidget(buildSensorsPage());
    m_pages->addWidget(buildLandingPage(false));
    m_pages->addWidget(buildLandingPage(true));
    m_pages->addWidget(buildSettingsPage());
    contentLayout->addWidget(m_pages, 1);
    m_notice = new QLabel("System initialized. Simulation mode is active.");
    m_notice->setObjectName("notice");
    contentLayout->addWidget(m_notice);

    root->addWidget(sidebar);
    root->addWidget(content, 1);
    setCentralWidget(central);

    connect(&m_simTimer, &QTimer::timeout, this, &MainWindow::updateSimulation);
    m_simTimer.start(1000);
    connect(&m_clockTimer, &QTimer::timeout, this, [this] {
        m_clockLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    m_clockTimer.start(1000);
    connect(&m_serial, &QSerialPort::readyRead, this, &MainWindow::readSerial);
    connect(&m_serial, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError e) {
        if (e != QSerialPort::NoError && e != QSerialPort::TimeoutError)
            setNotice("Serial error: " + m_serial.errorString());
    });
    refreshUi();
}

MainWindow::~MainWindow()
{
    if (m_serial.isOpen()) m_serial.close();
    if (m_logFile.isOpen()) m_logFile.close();
}

void MainWindow::applyStyle()
{
    setStyleSheet(R"(
      QWidget { background:#061326; color:#e7f4ff; font-family:"Segoe UI"; font-size:10pt; }
      #sidebar { background:#081b32; border:1px solid #123c61; border-radius:14px; }
      #brand { color:#f2f8ff; font-size:17pt; font-weight:800; padding:22px 6px; }
      QPushButton { background:#102c4a; border:1px solid #1d527b; border-radius:8px; padding:10px 12px; color:#dceeff; text-align:left; }
      QPushButton:hover { background:#124a75; border:1px solid #25c9ff; }
      QPushButton:checked { background:#07558a; border:1px solid #27cfff; color:white; }
      QPushButton:disabled { color:#587087; background:#101d2c; border-color:#26394a; }
      #header, QGroupBox, #panel { background:#0a1e35; border:1px solid #16466d; border-radius:12px; }
      #header { padding:8px; }
      #pageHeading { font-size:19pt; font-weight:800; color:#f4f9ff; }
      #muted { color:#86a7c4; }
      #connected { color:#4dffad; font-weight:bold; padding:7px 12px; border:1px solid #168e68; border-radius:12px; background:#073326; }
      #metricValue { font-size:23pt; font-weight:800; color:#f4fbff; }
      #metricTitle { color:#8bb8d8; font-weight:bold; }
      #notice { color:#70cfff; padding:6px 10px; background:#081d33; border:1px solid #123d5e; border-radius:7px; }
      QTableWidget { background:#07182b; alternate-background-color:#0b223a; gridline-color:#163b5c; border:1px solid #17466b; border-radius:8px; }
      QHeaderView::section { background:#102d4a; color:#d9efff; padding:8px; border:0; }
      QLineEdit, QComboBox, QPlainTextEdit { background:#06172a; border:1px solid #205176; border-radius:6px; padding:8px; }
      QComboBox QAbstractItemView { background:#0a2038; selection-background-color:#07558a; }
      QProgressBar { border:1px solid #205176; border-radius:5px; text-align:center; background:#07182b; }
      QProgressBar::chunk { background:#14c98a; border-radius:4px; }
    )");
}

QPushButton *MainWindow::makeNavButton(const QString &text, int page)
{
    auto *b = new QPushButton(text);
    b->setCheckable(true);
    b->setMinimumHeight(48);
    if (page == 0) b->setChecked(true);
    connect(b, &QPushButton::clicked, this, [this,page] { navigate(page); });
    return b;
}

void MainWindow::navigate(int page)
{
    if (page < 0 || page >= m_pages->count()) return;
    m_pages->setCurrentIndex(page);
    auto buttons = findChildren<QPushButton *>();
    for (auto *b : buttons) {
        if (b->isCheckable()) {
            bool selected = false;
            const QString t = b->text();
            if (page==0) selected=t.contains("Home");
            if (page==1) selected=t.contains("Telemetry");
            if (page==2) selected=t.contains("Data Logging");
            if (page==3) selected=t.contains("Sensor Monitoring");
            if (page==4) selected=t.contains("Manual Land");
            if (page==5) selected=t.contains("Auto Land");
            if (page==6) selected=t.contains("Settings");
            b->setChecked(selected);
        }
    }
}

QWidget *MainWindow::makeCard(const QString &title, QLabel **value, const QString &unit)
{
    auto *box = new QFrame;
    box->setObjectName("panel");
    auto *v = new QVBoxLayout(box);
    auto *t = new QLabel(title); t->setObjectName("metricTitle");
    *value = valueLabel("-- " + unit);
    v->addWidget(t); v->addWidget(*value);
    return box;
}

QWidget *MainWindow::buildHomePage()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);
    auto *metrics = new QGridLayout;
    metrics->addWidget(makeCard("🚀  ALTITUDE", &m_altValue, "m"),0,0);
    metrics->addWidget(makeCard("🌡  TEMPERATURE", &m_tempValue, "°C"),0,1);
    metrics->addWidget(makeCard("▰  BATTERY", &m_batteryValue, "%"),0,2);
    metrics->addWidget(makeCard("◈  ROCKET STATUS", &m_statusValue, ""),0,3);
    v->addLayout(metrics);
    auto *middle = new QHBoxLayout;
    auto *graphPanel = new QFrame; graphPanel->setObjectName("panel");
    auto *gv = new QVBoxLayout(graphPanel);
    m_altPlot = new PlotWidget("ALTITUDE / REAL-TIME", "Time samples", graphPanel);
    gv->addWidget(m_altPlot);
    middle->addWidget(graphPanel, 2);
    auto *right = new QFrame; right->setObjectName("panel");
    auto *rv = new QVBoxLayout(right);
    auto *controlTitle = new QLabel("MISSION CONTROL (SIMULATION)");
    controlTitle->setObjectName("metricTitle"); rv->addWidget(controlTitle);
    m_startBtn = new QPushButton("▶   START SIMULATION");
    m_stopBtn = new QPushButton("■   STOP SIMULATION");
    m_manualBtn = new QPushButton("⬇   MANUAL LAND (SIM)");
    m_autoBtn = new QPushButton("◉   AUTO LAND (SIM)");
    auto *reset = new QPushButton("↻   RESET SIMULATION");
    m_startBtn->setStyleSheet("background:#075b42;border-color:#19c98c;");
    m_stopBtn->setStyleSheet("background:#633b13;border-color:#d99528;");
    m_manualBtn->setStyleSheet("background:#61242d;border-color:#e34d62;");
    for (auto *b : {m_startBtn,m_stopBtn,m_manualBtn,m_autoBtn,reset}) { b->setMinimumHeight(42); rv->addWidget(b); }
    connect(m_startBtn,&QPushButton::clicked,this,&MainWindow::startSimulation);
    connect(m_stopBtn,&QPushButton::clicked,this,&MainWindow::stopSimulation);
    connect(m_manualBtn,&QPushButton::clicked,this,&MainWindow::simulatedManualLand);
    connect(m_autoBtn,&QPushButton::clicked,this,&MainWindow::simulatedAutoLand);
    connect(reset,&QPushButton::clicked,this,&MainWindow::resetSimulation);
    rv->addStretch();
    auto *safe = new QLabel("Hardware mode is read-only.\nFlight actions are simulation-only.");
    safe->setObjectName("muted"); rv->addWidget(safe);
    middle->addWidget(right,1);
    v->addLayout(middle,1);
    auto *bottom = new QHBoxLayout;
    auto *sensorBox = new QFrame; sensorBox->setObjectName("panel");
    auto *sv = new QVBoxLayout(sensorBox);
    auto *sensorTitle = new QLabel("LIVE SENSOR SNAPSHOT"); sensorTitle->setObjectName("metricTitle");
    sv->addWidget(sensorTitle);
    auto *sensorInfo = new QLabel("Pressure: -- hPa     Velocity: -- m/s\nGPS: -- , --     Acceleration: --");
    sensorInfo->setObjectName("homeSensorInfo");
    sv->addWidget(sensorInfo);
    bottom->addWidget(sensorBox,1);
    auto *logBox = new QFrame; logBox->setObjectName("panel");
    auto *lv = new QVBoxLayout(logBox);
    auto *logTitle = new QLabel("DATA LOGGING"); logTitle->setObjectName("metricTitle");
    lv->addWidget(logTitle);
    auto *logInfo = new QLabel("Use Data Logging to record telemetry to CSV.");
    logInfo->setObjectName("muted"); lv->addWidget(logInfo);
    bottom->addWidget(logBox,1);
    v->addLayout(bottom);
    return page;
}

QWidget *MainWindow::buildTelemetryPage()
{
    auto *page = new QWidget; auto *v = new QVBoxLayout(page);
    auto *title = new QLabel("Telemetry / Live Data"); title->setObjectName("pageHeading"); v->addWidget(title);
    auto *grid = new QGridLayout;
    const QStringList labels = {"Altitude (m)","Temperature (°C)","Pressure (hPa)","Battery (%)",
                                "Velocity (m/s)","GPS Latitude","GPS Longitude","Acceleration X / Y / Z"};
    for (int i=0;i<labels.size();++i) {
        auto *box = new QFrame; box->setObjectName("panel");
        auto *l = new QLabel(labels[i]); l->setObjectName("metricTitle");
        auto *val = valueLabel("--");
        val->setObjectName(QString("telemetry_%1").arg(i));
        auto *bl = new QVBoxLayout(box); bl->addWidget(l); bl->addWidget(val);
        grid->addWidget(box,i/4,i%4);
    }
    v->addLayout(grid);
    m_telemetryPlot = new PlotWidget("TEMPERATURE TREND", "Samples / °C");
    m_telemetryPlot->setLineColor(QColor("#ffad36"));
    v->addWidget(m_telemetryPlot,1);
    m_rawTelemetry = new QPlainTextEdit; m_rawTelemetry->setReadOnly(true);
    m_rawTelemetry->setMaximumHeight(110); m_rawTelemetry->setPlaceholderText("Incoming serial JSON lines...");
    v->addWidget(m_rawTelemetry);
    return page;
}

QWidget *MainWindow::buildLoggingPage()
{
    auto *page = new QWidget; auto *v = new QVBoxLayout(page);
    auto *title = new QLabel("Data Logging"); title->setObjectName("pageHeading"); v->addWidget(title);
    auto *bar = new QHBoxLayout;
    auto *start = new QPushButton("●  Start Logging");
    auto *stop = new QPushButton("■  Stop Logging");
    auto *exportB = new QPushButton("⇩  Export CSV");
    connect(start,&QPushButton::clicked,this,&MainWindow::startLogging);
    connect(stop,&QPushButton::clicked,this,&MainWindow::stopLogging);
    connect(exportB,&QPushButton::clicked,this,&MainWindow::exportCsv);
    bar->addWidget(start); bar->addWidget(stop); bar->addWidget(exportB); bar->addStretch();
    v->addLayout(bar);
    m_logTable = new QTableWidget(0,6);
    m_logTable->setHorizontalHeaderLabels({"Timestamp","Altitude (m)","Temp (°C)","Battery (%)","Velocity (m/s)","Status"});
    m_logTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_logTable->setAlternatingRowColors(true);
    v->addWidget(m_logTable,1);
    return page;
}

QWidget *MainWindow::buildSensorsPage()
{
    auto *page = new QWidget; auto *v = new QVBoxLayout(page);
    auto *title = new QLabel("Sensor Monitoring"); title->setObjectName("pageHeading"); v->addWidget(title);
    m_sensorTable = new QTableWidget(0,4);
    m_sensorTable->setHorizontalHeaderLabels({"Sensor","Current Value","Unit","Health"});
    m_sensorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    v->addWidget(m_sensorTable,1);
    updateSensorTable();
    return page;
}

QWidget *MainWindow::buildLandingPage(bool automatic)
{
    auto *page = new QWidget; auto *v = new QVBoxLayout(page);
    auto *title = new QLabel(automatic ? "Auto Land / Simulation" : "Manual Land / Simulation");
    title->setObjectName("pageHeading"); v->addWidget(title);
    auto *panel = new QFrame; panel->setObjectName("panel");
    auto *pv = new QVBoxLayout(panel);
    auto *warning = new QLabel("SIMULATION ONLY — no flight-control command is sent to hardware.");
    warning->setStyleSheet("color:#ffc15b;font-weight:bold;font-size:12pt;");
    pv->addWidget(warning);
    auto *description = new QLabel(automatic
        ? "Demonstration page for an automatic landing state. This does not calculate or command a real landing."
        : "Demonstration page for manual landing state. This does not command actuators or flight surfaces.");
    description->setWordWrap(true); pv->addWidget(description);
    auto *state = new QLabel("Current status: " + m_data.status);
    state->setObjectName(automatic ? "autoLandingState" : "manualLandingState");
    pv->addWidget(state);
    auto *action = new QPushButton(automatic ? "Run Auto-Land Simulation" : "Run Manual-Land Simulation");
    pv->addWidget(action);
    if (automatic) connect(action,&QPushButton::clicked,this,&MainWindow::simulatedAutoLand);
    else connect(action,&QPushButton::clicked,this,&MainWindow::simulatedManualLand);
    pv->addStretch();
    v->addWidget(panel,1);
    return page;
}

QWidget *MainWindow::buildSettingsPage()
{
    auto *page = new QWidget; auto *v = new QVBoxLayout(page);
    auto *title = new QLabel("Settings & Board Connection"); title->setObjectName("pageHeading"); v->addWidget(title);
    auto *panel = new QFrame; panel->setObjectName("panel");
    auto *form = new QFormLayout(panel);
    m_modeCombo = new QComboBox; m_modeCombo->addItems({"Simulation Mode","Hardware Read-only"});
    m_portCombo = new QComboBox;
    for (const auto &p : QSerialPortInfo::availablePorts()) m_portCombo->addItem(p.portName() + " — " + p.description(), p.portName());
    if (m_portCombo->count()==0) m_portCombo->addItem("No serial ports detected","");
    auto *refresh = new QPushButton("Refresh Ports");
    auto *portRow = new QHBoxLayout; portRow->addWidget(m_portCombo,1); portRow->addWidget(refresh);
    auto *portWidget = new QWidget; portWidget->setLayout(portRow);
    m_baudCombo = new QComboBox; m_baudCombo->addItems({"9600","19200","38400","57600","115200","230400"});
    m_baudCombo->setCurrentText("115200");
    m_connectBtn = new QPushButton("Connect");
    auto *disconnect = new QPushButton("Disconnect");
    auto *connRow = new QHBoxLayout; connRow->addWidget(m_connectBtn); connRow->addWidget(disconnect);
    auto *connWidget = new QWidget; connWidget->setLayout(connRow);
    form->addRow("Operating mode",m_modeCombo);
    form->addRow("Serial port",portWidget);
    form->addRow("Baud rate",m_baudCombo);
    form->addRow("Connection",connWidget);
    v->addWidget(panel);
    auto *info = new QLabel("Protocol: JSON Lines (one JSON object per line, UTF-8). Hardware mode only receives telemetry. No control/actuator commands are transmitted.");
    info->setWordWrap(true); info->setObjectName("notice"); v->addWidget(info);
    auto *pathRow = new QHBoxLayout;
    m_logPathEdit = new QLineEdit(QDir::homePath()+"/rocket_telemetry.csv");
    auto *browse = new QPushButton("Browse...");
    pathRow->addWidget(new QLabel("CSV file:")); pathRow->addWidget(m_logPathEdit,1); pathRow->addWidget(browse);
    v->addLayout(pathRow); v->addStretch();
    connect(refresh,&QPushButton::clicked,this,[this] {
        m_portCombo->clear();
        for (const auto &p : QSerialPortInfo::availablePorts()) m_portCombo->addItem(p.portName()+" — "+p.description(),p.portName());
        if (!m_portCombo->count()) m_portCombo->addItem("No serial ports detected","");
    });
    connect(browse,&QPushButton::clicked,this,[this] {
        const QString path = QFileDialog::getSaveFileName(this,"Choose CSV file",m_logPathEdit->text(),"CSV (*.csv)");
        if (!path.isEmpty()) m_logPathEdit->setText(path);
    });
    connect(m_connectBtn,&QPushButton::clicked,this,&MainWindow::connectSerial);
    connect(disconnect,&QPushButton::clicked,this,&MainWindow::disconnectSerial);
    connect(m_modeCombo,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int idx) {
        m_simulation = (idx==0);
        if (m_simulation) {
            if (m_serial.isOpen()) m_serial.close();
            m_connectionLabel->setText("● SIMULATION");
            setNotice("Simulation mode active.");
        } else {
            m_connectionLabel->setText("● HARDWARE (READ-ONLY)");
            setNotice("Hardware mode selected. Connect to receive telemetry only.");
        }
        refreshUi();
    });
    return page;
}

void MainWindow::connectSerial()
{
    if (!m_modeCombo || m_modeCombo->currentIndex()!=1) {
        QMessageBox::information(this,"Select hardware mode","Choose Hardware Read-only before connecting.");
        return;
    }
    if (m_serial.isOpen()) m_serial.close();
    const QString port = m_portCombo->currentData().toString();
    if (port.isEmpty()) { setNotice("No serial port selected."); return; }
    m_serial.setPortName(port);
    m_serial.setBaudRate(m_baudCombo->currentText().toInt());
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);
    if (!m_serial.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this,"Serial connection failed",m_serial.errorString());
        return;
    }
    m_simulation = false;
    m_connectionLabel->setText("● CONNECTED / READ-ONLY");
    setNotice("Connected to " + port + ". Receiving telemetry only.");
    refreshUi();
}

void MainWindow::disconnectSerial()
{
    if (m_serial.isOpen()) m_serial.close();
    m_connectionLabel->setText(m_simulation ? "● SIMULATION" : "● DISCONNECTED");
    setNotice("Serial disconnected.");
}

void MainWindow::readSerial()
{
    static QByteArray buffer;
    buffer += m_serial.readAll();
    if (buffer.size() > 65536) buffer.clear();
    int newline = -1;
    while ((newline = buffer.indexOf('\n')) >= 0) {
        const QByteArray line = buffer.left(newline).trimmed();
        buffer.remove(0,newline+1);
        if (!line.isEmpty()) parseTelemetryLine(line);
    }
}

void MainWindow::parseTelemetryLine(const QByteArray &line)
{
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(line,&error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        setNotice("Ignored invalid JSON telemetry line.");
        return;
    }
    const QJsonObject o = doc.object();
    auto setNum=[&](const char *key,double &target) { if (o.contains(key) && o.value(key).isDouble()) target=o.value(key).toDouble(); };
    setNum("altitude_m",m_data.altitude); setNum("temperature_c",m_data.temperature);
    setNum("pressure_hpa",m_data.pressure); setNum("battery_pct",m_data.battery);
    setNum("velocity_mps",m_data.velocity); setNum("latitude",m_data.latitude);
    setNum("longitude",m_data.longitude); setNum("ax",m_data.ax);
    setNum("ay",m_data.ay); setNum("az",m_data.az);
    if (o.value("status").isString()) m_data.status=o.value("status").toString();
    if (m_rawTelemetry) {
        m_rawTelemetry->appendPlainText(QString::fromUtf8(line));
        if (m_rawTelemetry->blockCount()>100) {
            QTextCursor c=m_rawTelemetry->textCursor(); c.movePosition(QTextCursor::Start);
            c.select(QTextCursor::LineUnderCursor); c.removeSelectedText(); c.deleteChar();
        }
    }
    m_altPlot->addValue(m_data.altitude);
    m_telemetryPlot->addValue(m_data.temperature);
    refreshUi();
    if (m_logging) appendLogRow();
}

void MainWindow::updateSimulation()
{
    if (!m_simulation) return;
    if (m_data.status=="FLIGHT") {
        m_simTime += 1.0;
        m_data.altitude += qMax(0.1, 4.0*std::exp(-m_simTime/45.0));
        m_data.velocity = 4.0*std::exp(-m_simTime/45.0);
        m_data.temperature = 26.0 + 0.9*std::sin(m_simTime/18.0);
        m_data.battery = qMax(0.0,m_data.battery-0.015);
        m_data.pressure = 1013.25 - m_data.altitude*0.12;
    }
    m_altPlot->addValue(m_data.altitude);
    m_telemetryPlot->addValue(m_data.temperature);
    refreshUi();
    if (m_logging) appendLogRow();
}

void MainWindow::refreshUi()
{
    if (m_altValue) m_altValue->setText(QString::number(m_data.altitude,'f',1)+" m");
    if (m_tempValue) m_tempValue->setText(QString::number(m_data.temperature,'f',1)+" °C");
    if (m_batteryValue) m_batteryValue->setText(QString::number(m_data.battery,'f',1)+" %");
    if (m_statusValue) m_statusValue->setText(m_data.status);
    if (m_startBtn) {
        const bool sim = m_simulation;
        for (auto *b : {m_startBtn,m_stopBtn,m_manualBtn,m_autoBtn}) b->setEnabled(sim);
    }
    if (m_pages) {
        const auto labels=findChildren<QLabel *>();
        for (auto *l: labels) {
            if (l->objectName()=="telemetry_0") l->setText(QString::number(m_data.altitude,'f',2));
            if (l->objectName()=="telemetry_1") l->setText(QString::number(m_data.temperature,'f',2));
            if (l->objectName()=="telemetry_2") l->setText(QString::number(m_data.pressure,'f',2));
            if (l->objectName()=="telemetry_3") l->setText(QString::number(m_data.battery,'f',1));
            if (l->objectName()=="telemetry_4") l->setText(QString::number(m_data.velocity,'f',2));
            if (l->objectName()=="telemetry_5") l->setText(QString::number(m_data.latitude,'f',6));
            if (l->objectName()=="telemetry_6") l->setText(QString::number(m_data.longitude,'f',6));
            if (l->objectName()=="telemetry_7") l->setText(QString("%1 / %2 / %3").arg(m_data.ax,0,'f',2).arg(m_data.ay,0,'f',2).arg(m_data.az,0,'f',2));
            if (l->objectName()=="homeSensorInfo") l->setText(QString("Pressure: %1 hPa     Velocity: %2 m/s\nGPS: %3 , %4     Acceleration: %5 / %6 / %7")
                .arg(m_data.pressure,0,'f',1).arg(m_data.velocity,0,'f',2).arg(m_data.latitude,0,'f',5)
                .arg(m_data.longitude,0,'f',5).arg(m_data.ax,0,'f',2).arg(m_data.ay,0,'f',2).arg(m_data.az,0,'f',2));
            if (l->objectName()=="manualLandingState" || l->objectName()=="autoLandingState")
                l->setText("Current status: "+m_data.status);
        }
    }
    updateSensorTable();
}

void MainWindow::updateSensorTable()
{
    if (!m_sensorTable) return;
    const QList<QStringList> rows = {
        {"Altitude",QString::number(m_data.altitude,'f',2),"m"},
        {"Temperature",QString::number(m_data.temperature,'f',2),"°C"},
        {"Pressure",QString::number(m_data.pressure,'f',2),"hPa"},
        {"Battery",QString::number(m_data.battery,'f',1),"%"},
        {"GPS",QString("%1, %2").arg(m_data.latitude,0,'f',6).arg(m_data.longitude,0,'f',6),"lat/lon"},
        {"Acceleration",QString("%1 / %2 / %3").arg(m_data.ax,0,'f',2).arg(m_data.ay,0,'f',2).arg(m_data.az,0,'f',2),"m/s²"}
    };
    m_sensorTable->setRowCount(rows.size());
    for (int r=0;r<rows.size();++r) {
        for (int c=0;c<3;++c) m_sensorTable->setItem(r,c,new QTableWidgetItem(rows[r][c]));
        auto *health = new QTableWidgetItem("RECEIVED / SIMULATED");
        health->setForeground(QColor("#4dffad"));
        m_sensorTable->setItem(r,3,health);
    }
}

QString MainWindow::csvHeader() const
{
    return "timestamp,altitude_m,temperature_c,pressure_hpa,battery_pct,velocity_mps,latitude,longitude,ax,ay,az,status\n";
}
QString MainWindow::csvRow() const
{
    return QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12\n")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
        .arg(m_data.altitude,0,'f',3).arg(m_data.temperature,0,'f',3)
        .arg(m_data.pressure,0,'f',3).arg(m_data.battery,0,'f',2)
        .arg(m_data.velocity,0,'f',3).arg(m_data.latitude,0,'f',7)
        .arg(m_data.longitude,0,'f',7).arg(m_data.ax,0,'f',3)
        .arg(m_data.ay,0,'f',3).arg(m_data.az,0,'f',3).arg(m_data.status);
}

void MainWindow::startLogging()
{
    if (m_logging) return;
    m_logPath = m_logPathEdit ? m_logPathEdit->text() : QDir::homePath()+"/rocket_telemetry.csv";
    if (m_logPath.isEmpty()) m_logPath = QDir::homePath()+"/rocket_telemetry.csv";
    m_logFile.setFileName(m_logPath);
    const bool exists=QFileInfo::exists(m_logPath);
    if (!m_logFile.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)) {
        QMessageBox::warning(this,"Logging failed",m_logFile.errorString()); return;
    }
    if (!exists || QFileInfo(m_logPath).size()==0) m_logFile.write(csvHeader().toUtf8());
    m_logging=true; setNotice("CSV logging started: "+m_logPath);
}
void MainWindow::stopLogging()
{
    if (m_logFile.isOpen()) m_logFile.close();
    m_logging=false; setNotice("CSV logging stopped.");
}
void MainWindow::appendLogRow()
{
    if (m_logFile.isOpen()) { m_logFile.write(csvRow().toUtf8()); m_logFile.flush(); }
    if (m_logTable) {
        const int row=m_logTable->rowCount();
        m_logTable->insertRow(row);
        const QStringList vals={QDateTime::currentDateTime().toString("HH:mm:ss"),
            QString::number(m_data.altitude,'f',2),QString::number(m_data.temperature,'f',2),
            QString::number(m_data.battery,'f',1),QString::number(m_data.velocity,'f',2),m_data.status};
        for (int c=0;c<vals.size();++c) m_logTable->setItem(row,c,new QTableWidgetItem(vals[c]));
        if (m_logTable->rowCount()>500) m_logTable->removeRow(0);
        m_logTable->scrollToBottom();
    }
}
void MainWindow::exportCsv()
{
    const QString path=QFileDialog::getSaveFileName(this,"Export telemetry CSV",
        QDir::homePath()+"/rocket_export.csv","CSV (*.csv)");
    if (path.isEmpty()) return;
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly|QIODevice::Text)) {
        QMessageBox::warning(this,"Export failed",out.errorString()); return;
    }
    out.write("timestamp,altitude_m,temperature_c,battery_pct,velocity_mps,status\\n");
    for (int r=0;r<(m_logTable?m_logTable->rowCount():0);++r) {
        QStringList cells;
        for (int c=0;c<6;++c) cells << (m_logTable->item(r,c)?m_logTable->item(r,c)->text():"");
        out.write((cells.join(",")+"\n").toUtf8());
    }
    out.close(); setNotice("CSV exported: "+path);
}

void MainWindow::startSimulation()
{
    if (!m_simulation) return;
    m_data.status="FLIGHT"; m_simTime=0; setNotice("Simulation started. No hardware command sent."); refreshUi();
}
void MainWindow::stopSimulation()
{
    if (!m_simulation) return;
    m_data.status="STOPPED"; m_data.velocity=0; setNotice("Simulation stopped."); refreshUi();
}
void MainWindow::simulatedManualLand()
{
    if (!m_simulation) return;
    m_data.status="MANUAL LAND (SIM)"; m_data.velocity=0; setNotice("Manual landing state simulated only."); refreshUi();
}
void MainWindow::simulatedAutoLand()
{
    if (!m_simulation) return;
    m_data.status="AUTO LAND (SIM)"; m_data.velocity=0; setNotice("Automatic landing state simulated only."); refreshUi();
}
void MainWindow::resetSimulation()
{
    if (!m_simulation) return;
    m_data=Telemetry{}; m_simTime=0;
    m_altHistory.clear(); m_tempHistory.clear();
    setNotice("Simulation values reset."); refreshUi();
}
void MainWindow::setNotice(const QString &message)
{
    if (m_notice) m_notice->setText(message);
}
