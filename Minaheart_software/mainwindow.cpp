#include "mainwindow.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QProgressBar>
#include <QPushButton>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QByteArray>
#include <functional>
#include <QtMath>

namespace {
const QString BG = "#081426";
const QString PANEL = "#0d1b30";
const QString BORDER = "#1b416b";
const QString TEXT = "#e5edf8";
const QString MUTED = "#91a6c2";

QString applicationLogPath()
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directory.isEmpty()) directory = QDir::home().filePath(".rocket-monitoring-system");
    QDir().mkpath(directory);
    return QDir(directory).filePath("rocket_log.csv");
}

QLabel *label(const QString &text, int size = 12, const QString &color = TEXT, bool bold = false)
{
    auto *l = new QLabel(text);
    l->setStyleSheet(QString("color:%1;font-size:%2px;%3").arg(color).arg(size).arg(bold ? "font-weight:600;" : ""));
    return l;
}

QFrame *panel(const QString &title, QVBoxLayout **body = nullptr)
{
    auto *frame = new QFrame;
    frame->setStyleSheet(QString("QFrame{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #10243d,stop:1 %1);border:1px solid %2;border-radius:12px;}").arg(PANEL, BORDER));
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    auto *heading = label(title, 15, "#dff3ff", true);
    layout->addWidget(heading);
    if (body) *body = layout;
    return frame;
}

class AltitudeChart : public QWidget {
public:
    QVector<double> values;
    explicit AltitudeChart(QWidget *parent = nullptr) : QWidget(parent) { setMinimumHeight(230); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        QLinearGradient bg(0,0,0,height()); bg.setColorAt(0,QColor("#0b1e36")); bg.setColorAt(1,QColor("#071326")); p.fillRect(rect(),bg);
        const QRectF plot(48, 18, width()-68, height()-52);
        p.setPen(QPen(QColor("#1c4266"), 1, Qt::DotLine));
        for (int i=0;i<=4;i++) { qreal y=plot.bottom()-i*plot.height()/4; p.drawLine(QPointF(plot.left(),y),QPointF(plot.right(),y)); p.setPen(QColor("#b5c4d8")); p.drawText(QRectF(2,y-9,38,18),Qt::AlignRight|Qt::AlignVCenter,QString::number(i*100)); p.setPen(QPen(QColor("#244365"),1)); }
        for (int i=0;i<=5;i++) { qreal x=plot.left()+i*plot.width()/5; p.drawLine(QPointF(x,plot.top()),QPointF(x,plot.bottom())); p.setPen(QColor("#b5c4d8")); p.drawText(QRectF(x-15,plot.bottom()+7,30,18),Qt::AlignCenter,QString::number(i*20)); p.setPen(QPen(QColor("#244365"),1)); }
        p.setPen(QColor("#b5c4d8")); p.drawText(QRectF(plot.left(),height()-20,plot.width(),18),Qt::AlignCenter,"Time (s)");
        if (values.size()<2) return;
        QPainterPath path, area; double maxV=400.0;
        for(int i=0;i<values.size();++i){ qreal x=plot.left()+i*(plot.width()/qMax(1,values.size()-1)); qreal y=plot.bottom()-qMin(maxV,values[i])/maxV*plot.height(); if(i==0){path.moveTo(x,y);area.moveTo(x,plot.bottom());area.lineTo(x,y);}else{path.lineTo(x,y);area.lineTo(x,y);} }
        area.lineTo(plot.right(),plot.bottom()); area.closeSubpath();
        p.fillPath(area,QColor(34,126,230,75)); p.setPen(QPen(QColor("#36c7ff"),5)); p.drawPath(path); p.setPen(QPen(QColor("#66d9ff"),2.5)); p.drawPath(path);
        if(!values.isEmpty()){qreal x=plot.right();qreal y=plot.bottom()-qMin(maxV,values.last())/maxV*plot.height();p.setBrush(QColor("#eaf5ff"));p.setPen(QPen(QColor("#48a7ff"),2));p.drawEllipse(QPointF(x,y),5,5);}
    }
};

class RocketView : public QWidget {
public:
    explicit RocketView(QWidget *parent=nullptr):QWidget(parent){setMinimumHeight(205);}
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);QLinearGradient bg(0,0,0,height());bg.setColorAt(0,QColor("#06152b"));bg.setColorAt(1,QColor("#102d50"));p.fillRect(rect(),bg);
        for(int i=0;i<65;i++){int x=(i*47+13)%qMax(1,width()),y=(i*71+19)%qMax(1,height());p.setPen(QColor(160,185,220,130));p.drawPoint(x,y);}
        QPointF c(width()/2.0,height()/2.0+8);
        p.setBrush(QColor(255,130,40,35));p.setPen(Qt::NoPen);p.drawEllipse(c+QPointF(0,25),43,43);
        QPolygonF flame;flame<<c+QPointF(-10,38)<<c+QPointF(0,72)<<c+QPointF(10,38);p.setBrush(QColor("#ff8a20"));p.drawPolygon(flame);
        QPolygonF body;body<<c+QPointF(0,-55)<<c+QPointF(15,-30)<<c+QPointF(12,42)<<c+QPointF(-12,42)<<c+QPointF(-15,-30);p.setBrush(QColor("#e8edf5"));p.drawPolygon(body);
        QPolygonF nose;nose<<c+QPointF(0,-55)<<c+QPointF(15,-30)<<c+QPointF(-15,-30);p.setBrush(QColor("#e52c45"));p.drawPolygon(nose);
        QPolygonF fin1;fin1<<c+QPointF(-12,15)<<c+QPointF(-29,42)<<c+QPointF(-12,35);p.setBrush(QColor("#e52c45"));p.drawPolygon(fin1);
        QPolygonF fin2;fin2<<c+QPointF(12,15)<<c+QPointF(29,42)<<c+QPointF(12,35);p.drawPolygon(fin2);
        p.setBrush(QColor("#2c8eea"));p.setPen(QPen(QColor("#a9d8ff"),2));p.drawEllipse(c+QPointF(0,-12),5,5);
    }
};
}

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent),timer(new QTimer(this)),logFile(new QFile(this)),
    altitude(303.7),temperature(26.7),battery(91.6),velocity(2.4),pressure(977.2),elapsedSeconds(58),landed(false),paused(false),manualMode(false),loggingEnabled(true), serialPort(new QSerialPort(this)), hardwareMode(false), latitude(13.7566), longitude(100.5029), accelX(0.17), accelY(-0.08), accelZ(9.73)
{
    setWindowTitle("Rocket Monitoring System | Advanced Mission Control"); resize(1600,980); setMinimumSize(1180,760);
    auto *central=new QWidget; setCentralWidget(central);
    setStyleSheet(R"QSS(
        QMainWindow, QWidget { background-color:#071326; color:#eaf3ff; font-family:"Segoe UI"; font-size:12px; }
        QToolTip { background:#102945; color:#eaf3ff; border:1px solid #18bfff; padding:6px 9px; border-radius:5px; }
        QScrollBar:vertical { background:#08172b; width:10px; margin:2px; border-radius:5px; }
        QScrollBar::handle:vertical { background:#20517c; min-height:28px; border-radius:5px; }
        QScrollBar::handle:vertical:hover { background:#16a9ed; }
        QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
        QComboBox { background:#102641; color:#eaf3ff; border:1px solid #23608d; border-radius:7px; padding:9px 12px; min-height:20px; }
        QComboBox:hover { border:1px solid #17bfff; }
        QComboBox QAbstractItemView { background:#102641; color:#eaf3ff; selection-background-color:#087fc0; }
        QCheckBox { spacing:10px; }
        QCheckBox::indicator { width:18px; height:18px; border-radius:5px; border:1px solid #3879a8; background:#0b1c32; }
        QCheckBox::indicator:checked { background:#00bd83; border:1px solid #00e5a0; }
        QTableWidget { alternate-background-color:#0d2038; selection-background-color:#104b78; selection-color:#ffffff; outline:0; }
        QHeaderView::section { background:#102a48; color:#bfe4ff; border:0; border-bottom:1px solid #1b5b86; padding:10px 8px; font-weight:600; }
        QPushButton { padding:8px 12px; }
        QPushButton:disabled { background:#17283a; color:#61758b; border:1px solid #263d55; }
    )QSS");
    central->setStyleSheet("background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #071326,stop:0.55 #091a31,stop:1 #061020);");
    auto *root=new QHBoxLayout(central);root->setContentsMargins(0,0,0,0);root->setSpacing(0);
    auto *sidebar=new QFrame;sidebar->setFixedWidth(232);sidebar->setStyleSheet("background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #081a31,stop:1 #050e1c);border-right:1px solid #15517a;");
    auto *nav=new QVBoxLayout(sidebar);nav->setContentsMargins(12,22,12,16);nav->setSpacing(11);
    navigationButtons << addNavigationItem(nav,"⌂","Home",true);
    navigationButtons << addNavigationItem(nav,"◉","Telemetry");
    navigationButtons << addNavigationItem(nav,"▤","Data Logging");
    navigationButtons << addNavigationItem(nav,"▦","Sensor Monitoring");
    navigationButtons << addNavigationItem(nav,"◇","Manual Land");
    navigationButtons << addNavigationItem(nav,"⚙","Settings");nav->addStretch();
    root->addWidget(sidebar);
    pageStack=new QStackedWidget; root->addWidget(pageStack,1);
    auto *content=new QWidget;auto *main=new QVBoxLayout(content);main->setContentsMargins(18,12,18,12);main->setSpacing(14);
    auto *header=new QHBoxLayout;auto *brand=label("🚀   Rocket Monitoring System",26,TEXT,true);header->addWidget(brand);header->addWidget(label("REAL-TIME TELEMETRY  •  MISSION CONTROL",10,"#5ca9df",true));header->addStretch();connectionValue=label("●  CONNECTED  /  SIMULATION",12,"#28efad",true);header->addWidget(connectionValue);header->addSpacing(20);auto *clock=label(QDateTime::currentDateTime().toString("yyyy-MM-dd   HH:mm:ss"),11,"#b7c9df",true);header->addWidget(clock);auto *clockTimer=new QTimer(this);connect(clockTimer,&QTimer::timeout,this,[clock](){clock->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd   HH:mm:ss"));});clockTimer->start(1000);main->addLayout(header);
    auto *metrics=new QGridLayout;metrics->setSpacing(14);
    auto metric=[&](const QString &name,const QString &value,const QString &sub,const QString &color){auto *f=new QFrame;f->setMinimumHeight(148);f->setStyleSheet(QString("QFrame{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 #0b1a30);border:1px solid %2;border-radius:13px;}").arg(color+"30",color));auto *v=new QVBoxLayout(f);v->setContentsMargins(18,14,18,14);v->addWidget(label(name,14,"#dce8f8",true));auto *big=label(value,31,"#ffffff",true);v->addWidget(big,1,Qt::AlignCenter);v->addWidget(label(sub,13,color,true),0,Qt::AlignCenter);return qMakePair(f,big);};
    auto a=metric("🚀  Altitude","303.7 m","↑  +2.4 m/s","#267cff");altitudeValue=a.second;metrics->addWidget(a.first,0,0);
    auto t=metric("🌡  Temperature","26.7 °C","→  Stable","#ff9900");temperatureValue=t.second;metrics->addWidget(t.first,0,1);
    auto b=metric("▣  Battery","91.6 %","↓  -0.12 %/s","#00c878");batteryValue=b.second;metrics->addWidget(b.first,0,2);
    auto s=metric("⬡  Rocket Status","FLIGHT","Normal","#9b63ff");statusValue=s.second;metrics->addWidget(s.first,0,3);
    auto *telemetry=panel("◉  Telemetry Status");telemetry->setMinimumWidth(245);telemetry->layout()->addWidget(label("●  UART Connection :     Connected",12,"#20d889",true));telemetry->layout()->addWidget(label("Data Receive :                    OK",12,"#20d889"));telemetry->layout()->addWidget(label("Last Update :        2026-10-29 02:21:48",10,MUTED));metrics->addWidget(telemetry,0,4);main->addLayout(metrics);
    auto *middle=new QGridLayout;middle->setSpacing(14);
    auto *chartPanel=panel("⌁  Altitude  (Real-time)");chartWidget=new AltitudeChart;static_cast<AltitudeChart*>(chartWidget)->values=altitudeHistory;middle->addWidget(chartPanel,0,0,1,2);chartPanel->layout()->addWidget(chartWidget);
    auto *sensor=panel("▦  Sensor Data");auto addSensor=[&](const QString &n,const QString &v){auto *row=new QHBoxLayout;row->addWidget(label(n,12,MUTED,true));row->addStretch();row->addWidget(label(v,12,TEXT,true));static_cast<QVBoxLayout*>(sensor->layout())->addLayout(row);};
    addSensor("Altitude","303.7 m");addSensor("Temperature","26.7 °C");addSensor("Pressure","977.2 hPa");addSensor("Battery","91.6 %");addSensor("GPS (Lat, Lon)",QString::number(latitude,'f',5)+", "+QString::number(longitude,'f',5));addSensor("Acceleration (X,Y,Z)",QString::number(accelX,'f',2)+", "+QString::number(accelY,'f',2)+", "+QString::number(accelZ,'f',2));middle->addWidget(sensor,0,2);
    auto *manual=panel("◇  Flight Controls");manual->layout()->setSpacing(7);manual->layout()->addWidget(label("Control the rocket simulation",11,MUTED));
    startButton=new QPushButton("▶   START");startButton->setMinimumHeight(36);startButton->setCursor(Qt::PointingHandCursor);startButton->setStyleSheet("QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #0cc58d,stop:1 #087b61);color:white;border:1px solid #35f0b5;border-radius:8px;font-size:14px;font-weight:bold;padding:9px;}QPushButton:hover{background:#12dfa2;}QPushButton:pressed{background:#075b49;}");manual->layout()->addWidget(startButton);
    stopButton=new QPushButton("Ⅱ   STOP");stopButton->setMinimumHeight(36);stopButton->setCursor(Qt::PointingHandCursor);stopButton->setStyleSheet("QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #d99520,stop:1 #8d560b);color:white;border:1px solid #ffc45b;border-radius:8px;font-size:14px;font-weight:bold;padding:9px;}QPushButton:hover{background:#edae35;}");manual->layout()->addWidget(stopButton);
    landButton=new QPushButton("◇   LAND NOW");landButton->setMinimumHeight(44);landButton->setCursor(Qt::PointingHandCursor);landButton->setToolTip("Send a landing command to the simulated rocket");landButton->setStyleSheet("QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #ff526b,stop:1 #bd183d);color:white;border:1px solid #ff8291;border-radius:8px;font-size:15px;font-weight:bold;padding:10px;}QPushButton:hover{background:#ff6379;}");manual->layout()->addWidget(landButton);
    manualButton=new QPushButton("✋   MANUAL: OFF");manualButton->setMinimumHeight(36);manualButton->setCursor(Qt::PointingHandCursor);manualButton->setToolTip("Toggle manual control mode");manualButton->setStyleSheet("QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #174a76,stop:1 #102b4c);color:#b9e5ff;border:1px solid #258bc5;border-radius:8px;font-size:13px;font-weight:bold;padding:9px;}QPushButton:hover{background:#1d6599;}");manual->layout()->addWidget(manualButton);
    resetButton=new QPushButton("↻   Reset");resetButton->setMinimumHeight(36);resetButton->setCursor(Qt::PointingHandCursor);resetButton->setToolTip("Reset the rocket simulation");resetButton->setStyleSheet("QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #174a76,stop:1 #102b4c);color:#b9e5ff;border:1px solid #258bc5;border-radius:8px;font-size:14px;padding:9px;}QPushButton:hover{background:#1d6599;}");manual->layout()->addWidget(resetButton);middle->addWidget(manual,0,3);main->addLayout(middle,1);
    auto *bottom=new QGridLayout;bottom->setSpacing(14);
    auto *logs=panel("▤  Data Log");auto *logHead=new QHBoxLayout;logState=label("◉  Logging : ON",12,"#18d889",true);logHead->addWidget(logState);logHead->addStretch();logButton=new QPushButton("Stop Logging");logButton->setMaximumWidth(120);logHead->addWidget(logButton);static_cast<QVBoxLayout*>(logs->layout())->addLayout(logHead);
    logTable=new QTableWidget(0,5);logTable->setHorizontalHeaderLabels({"Time","Altitude (m)","Temperature (°C)","Battery (%)","Status"});logTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);logTable->verticalHeader()->hide();logTable->setAlternatingRowColors(true);logTable->setShowGrid(false);logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);logTable->setMinimumHeight(225);logTable->setStyleSheet("QTableWidget{background:#0b192d;color:#dbe7f7;gridline-color:#1b3555;border:0;}QHeaderView::section{background:#112846;color:#dbe7f7;padding:8px;border:0;font-weight:bold;}QTableWidget::item{padding:5px;border-bottom:1px solid #1b3555;}");logs->layout()->addWidget(logTable);bottom->addWidget(logs,0,0,1,2);
    auto *visual=panel("◎  Rocket Visual  (Simulation)");auto *visualRow=new QHBoxLayout;rocketWidget=new RocketView;visualRow->addWidget(rocketWidget,3);auto *stats=new QVBoxLayout;stats->addWidget(label("Altitude",11,MUTED));stats->addWidget(visualAltitudeValue=label("303.7 m",19,TEXT,true));stats->addSpacing(8);stats->addWidget(label("Velocity",11,MUTED));stats->addWidget(velocityValue=label("2.4 m/s",19,TEXT,true));stats->addSpacing(8);stats->addWidget(label("Status",11,MUTED));systemState=label("FLIGHT",14,"#16d889",true);stats->addWidget(systemState);stats->addStretch();visualRow->addLayout(stats,2);static_cast<QVBoxLayout*>(visual->layout())->addLayout(visualRow);bottom->addWidget(visual,0,2,1,2);main->addLayout(bottom,1);
    auto *footer=new QHBoxLayout;footer->addWidget(label("●  ALL SYSTEMS NOMINAL",12,"#28efad",true));footer->addSpacing(18);footer->addWidget(label("ROCKET MONITORING SYSTEM  •  ADVANCED EDITION v2.0",10,"#75bce9",true));footer->addStretch();footer->addWidget(label("◫  LOG FILE : rocket_log.csv",10,MUTED));main->addLayout(footer);

    pageStack->addWidget(content);
    auto makePage=[this](const QString &heading){
        auto *page=new QWidget; page->setStyleSheet(QString("background:%1;").arg(BG));
        auto *v=new QVBoxLayout(page); v->setContentsMargins(22,18,22,18); v->setSpacing(16);
        auto *top=new QHBoxLayout; top->addWidget(label("🚀   Rocket Monitoring System",24,TEXT,true)); top->addStretch(); top->addWidget(label("● Connected (SIMULATION)",12,"#16d889",true)); v->addLayout(top); v->addWidget(label(heading,21,TEXT,true));
        return qMakePair(page,v);
    };
    auto infoRow=[](QVBoxLayout *v,const QString &n,const QString &value)->QLabel*{
        auto *f=new QFrame;
        f->setStyleSheet("QFrame{background:#0d1b30;border:1px solid #1b416b;border-radius:8px;}");
        auto *r=new QHBoxLayout(f); r->setContentsMargins(14,9,14,9);
        r->addWidget(label(n,13,MUTED,true)); r->addStretch();
        auto *valueLabel=label(value,14,TEXT,true); r->addWidget(valueLabel); v->addWidget(f);
        return valueLabel;
    };
    // Telemetry
    {
        auto p=makePage("Telemetry Monitoring"); auto *g=new QGridLayout;
        auto *cp=panel("Altitude (Real-time)"); auto *ch=new AltitudeChart; ch->values=altitudeHistory;
        cp->layout()->addWidget(ch); g->addWidget(cp,0,0,2,2);
        connect(timer,&QTimer::timeout,this,[this,ch](){ch->values=altitudeHistory;ch->update();});
        auto *d=panel("Live Telemetry"); auto *dLayout=static_cast<QVBoxLayout*>(d->layout());
        auto *altLive=infoRow(dLayout,"Altitude",QString::number(altitude,'f',1)+" m");
        auto *tempLive=infoRow(dLayout,"Temperature",QString::number(temperature,'f',1)+" °C");
        auto *pressureLive=infoRow(dLayout,"Pressure",QString::number(pressure,'f',1)+" hPa");
        auto *batteryLive=infoRow(dLayout,"Battery",QString::number(battery,'f',1)+" %");
        auto *gpsLive=infoRow(dLayout,"GPS",QString::number(latitude,'f',5)+", "+QString::number(longitude,'f',5));
        connect(timer,&QTimer::timeout,this,[this,altLive,tempLive,pressureLive,batteryLive,gpsLive](){
            altLive->setText(QString::number(altitude,'f',1)+" m");
            tempLive->setText(QString::number(temperature,'f',1)+" °C");
            pressureLive->setText(QString::number(pressure,'f',1)+" hPa");
            batteryLive->setText(QString::number(battery,'f',1)+" %");
            gpsLive->setText(QString::number(latitude,'f',5)+", "+QString::number(longitude,'f',5));
        });
        g->addWidget(d,0,2);
        auto *status=panel("Telemetry Status");
        status->layout()->addWidget(label("UART Connection : Connected",13,"#16d889",true));
        status->layout()->addWidget(label("Data Receive : OK",13,"#16d889",true));
        g->addWidget(status,1,2); p.second->addLayout(g,1); pageStack->addWidget(p.first);
    }
    // Data logging, independent full-page table
    { auto p=makePage("Data Logging");auto *box=panel("Flight Data Records");auto *table=new QTableWidget(0,5);table->setHorizontalHeaderLabels({"Time","Altitude (m)","Temperature (°C)","Battery (%)","Status"});table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);table->verticalHeader()->hide();table->setAlternatingRowColors(true);table->setShowGrid(false);table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->setStyleSheet("QTableWidget{background:#0b192d;color:#dbe7f7;gridline-color:#1b3555;}QHeaderView::section{background:#112846;color:#dbe7f7;padding:8px;}");static_cast<QVBoxLayout*>(box->layout())->addWidget(table,1);auto *actions=new QHBoxLayout;auto *toggle=new QPushButton(loggingEnabled?"Stop Logging":"Start Logging");auto *exportBtn=new QPushButton("Export CSV");actions->addWidget(toggle);actions->addWidget(exportBtn);actions->addStretch();static_cast<QVBoxLayout*>(box->layout())->addLayout(actions);connect(toggle,&QPushButton::clicked,this,[this,toggle](){toggleLogging();toggle->setText(loggingEnabled?"Stop Logging":"Start Logging");});connect(exportBtn,&QPushButton::clicked,this,[this](){
        if(logFile->isOpen()) logFile->flush();
        const QString source = logFile->fileName().isEmpty() ? applicationLogPath() : logFile->fileName();
        if(!QFile::exists(source)) { QMessageBox::information(this,"CSV Export","ยังไม่มีไฟล์บันทึกข้อมูล"); return; }
        const QString target = QFileDialog::getSaveFileName(this,"Export flight log",QDir::home().filePath("rocket_log.csv"),"CSV files (*.csv)");
        if(target.isEmpty()) return;
        if(QFile::exists(target)) QFile::remove(target);
        if(!QFile::copy(source,target)) QMessageBox::warning(this,"CSV Export","ไม่สามารถส่งออกไฟล์ได้\n"+target);
        else QMessageBox::information(this,"CSV Export","ส่งออกไฟล์สำเร็จ\n"+target);
    });p.second->addWidget(box,1);pageStack->addWidget(p.first);connect(timer,&QTimer::timeout,this,[this,table](){if(hardwareMode)return;int r=table->rowCount();table->insertRow(r);QStringList x{QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"),QString::number(altitude,'f',1),QString::number(temperature,'f',1),QString::number(battery,'f',1),landed?"LANDED":paused?"STOPPED":manualMode?"MANUAL":"FLIGHT"};for(int i=0;i<x.size();++i)table->setItem(r,i,new QTableWidgetItem(x[i]));while(table->rowCount()>200)table->removeRow(0);}); }
    // Sensor monitoring
    {
        auto p=makePage("Sensor Monitoring"); auto *g=new QGridLayout;
        const QStringList names={"Altitude Sensor","Temperature Sensor","Pressure Sensor","Battery Monitor","GPS Receiver","3-Axis Accelerometer"};
        const QStringList values={QString::number(altitude,'f',1)+" m",QString::number(temperature,'f',1)+" °C",QString::number(pressure,'f',1)+" hPa",QString::number(battery,'f',1)+" %",QString::number(latitude,'f',5)+", "+QString::number(longitude,'f',5),QString::number(accelX,'f',2)+", "+QString::number(accelY,'f',2)+", "+QString::number(accelZ,'f',2)};
        QVector<QLabel*> sensorReadouts;
        for(int i=0;i<names.size();++i){
            auto *card=panel(names[i]);
            auto *readout=label(values[i],22,TEXT,true); sensorReadouts.append(readout);
            card->layout()->addWidget(readout);
            card->layout()->addWidget(label("Sensor status : ONLINE",12,"#16d889",true));
            g->addWidget(card,i/2,i%2);
        }
        connect(timer,&QTimer::timeout,this,[this,sensorReadouts](){
            sensorReadouts[0]->setText(QString::number(altitude,'f',1)+" m");
            sensorReadouts[1]->setText(QString::number(temperature,'f',1)+" °C");
            sensorReadouts[2]->setText(QString::number(pressure,'f',1)+" hPa");
            sensorReadouts[3]->setText(QString::number(battery,'f',1)+" %");
            sensorReadouts[4]->setText(QString::number(latitude,'f',5)+", "+QString::number(longitude,'f',5));
            sensorReadouts[5]->setText(QString::number(accelX,'f',2)+", "+QString::number(accelY,'f',2)+", "+QString::number(accelZ,'f',2));
        });
        p.second->addLayout(g,1); pageStack->addWidget(p.first);
    }
    // Manual controls
    { auto p=makePage("Manual Flight Control");auto *box=panel("Rocket Commands");box->layout()->addWidget(label("Control the simulated rocket using the commands below.",13,MUTED));auto add=[&](QString t,QString css,std::function<void()> fn){auto *b=new QPushButton(t);b->setMinimumHeight(52);b->setStyleSheet(css);b->setCursor(Qt::PointingHandCursor);box->layout()->addWidget(b);connect(b,&QPushButton::clicked,this,fn);};QString green="QPushButton{background:#087c55;color:white;border:0;border-radius:8px;font-size:15px;font-weight:bold;}";QString amber="QPushButton{background:#b87913;color:white;border:0;border-radius:8px;font-size:15px;font-weight:bold;}";QString red="QPushButton{background:#ef2945;color:white;border:0;border-radius:8px;font-size:15px;font-weight:bold;}";QString blue="QPushButton{background:#17345c;color:#8fc2ff;border:1px solid #2879c9;border-radius:8px;font-size:15px;font-weight:bold;}";add("▶ START",green,[this](){startSimulation();});add("Ⅱ STOP",amber,[this](){stopSimulation();});add("✋ MANUAL ON / OFF",blue,[this](){toggleManualMode();});add("◇ LAND NOW",red,[this](){if(!landed&&QMessageBox::question(this,"Confirm Landing","Send landing command?",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)manualLand();});add("↻ RESET",blue,[this](){if(QMessageBox::question(this,"Reset","Start a new simulated flight?",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)resetSimulation();});auto *state=label("Current status: FLIGHT",16,"#16d889",true);box->layout()->addWidget(state);p.second->addWidget(box,1);pageStack->addWidget(p.first);connect(timer,&QTimer::timeout,this,[this,state](){state->setText("Current status: "+QString(landed?"LANDED":paused?"STOPPED":manualMode?"MANUAL":"FLIGHT"));}); }
    // Settings: Arduino UNO R3 USB serial connection + simulation controls
    {
        auto p=makePage("System Settings / Hardware Connection");
        QVBoxLayout *connectionLayout = nullptr;
        auto *connection=panel("Arduino UNO R3  |  USB Serial Connection", &connectionLayout);
        auto *note=label("Connect the UNO R3 by USB. Select its COM port and baud rate, then press Connect.",12,MUTED);
        connection->layout()->addWidget(note);
        auto *portRow=new QHBoxLayout; portCombo=new QComboBox; portCombo->setMinimumWidth(220);
        auto *refreshPorts=new QPushButton("↻ Refresh Ports");
        baudCombo=new QComboBox; baudCombo->addItems({"9600","19200","38400","57600","115200"}); baudCombo->setCurrentText("115200");
        portRow->addWidget(label("COM Port",12,MUTED,true)); portRow->addWidget(portCombo,2);
        portRow->addWidget(refreshPorts); portRow->addSpacing(14);
        portRow->addWidget(label("Baud",12,MUTED,true)); portRow->addWidget(baudCombo);
        connectionLayout->addLayout(portRow);
        auto *buttonRow=new QHBoxLayout; auto *connectBtn=new QPushButton("●  CONNECT ARDUINO"); auto *disconnectBtn=new QPushButton("DISCONNECT");
        connectBtn->setStyleSheet("QPushButton{background:#087c55;color:white;border:1px solid #20c98a;border-radius:8px;font-weight:bold;padding:10px 18px;}QPushButton:hover{background:#0aa66f;}");
        disconnectBtn->setStyleSheet("QPushButton{background:#17345c;color:#8fc2ff;border:1px solid #2879c9;border-radius:8px;padding:10px 18px;}");
        buttonRow->addWidget(connectBtn); buttonRow->addWidget(disconnectBtn); buttonRow->addStretch();
        connectionLayout->addLayout(buttonRow);
        serialStatusLabel=label("●  DISCONNECTED  |  Simulation data active",13,"#ffbd59",true);
        connection->layout()->addWidget(serialStatusLabel);
        p.second->addWidget(connection);

        auto *protocol=panel("Serial Data Protocol");
        protocol->layout()->addWidget(label("Arduino sends one CSV line per sample, terminated by newline:",12,MUTED));
        protocol->layout()->addWidget(label("ALT,TEMP,PRESSURE,BATTERY,LAT,LON,AX,AY,AZ",14,"#6bd4ff",true));
        protocol->layout()->addWidget(label("Example: 125.4,27.1,1008.2,95.0,13.7566,100.5029,0.12,-0.04,9.81",11,TEXT));
        protocol->layout()->addWidget(label("UNO R3 alone has no altitude / GPS / temperature sensors. Connect compatible modules before treating readings as real.",11,"#ffbd59"));
        p.second->addWidget(protocol);

        auto *sim=panel("Simulation Settings"); sim->layout()->addWidget(label("Simulation speed",13,MUTED,true));
        auto *speed=new QComboBox; speed->addItems({"1x","2x","5x","10x"}); sim->layout()->addWidget(speed);
        connect(speed,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){timer->setInterval(i==0?1000:i==1?500:i==2?200:100);});
        auto *logging=new QCheckBox("Enable data logging"); logging->setChecked(loggingEnabled); logging->setStyleSheet("color:#e5edf8;font-size:14px;"); sim->layout()->addWidget(logging);
        connect(logging,&QCheckBox::toggled,this,[this](bool on){if(on!=loggingEnabled)toggleLogging();});
        sim->layout()->addWidget(label("CSV file: rocket_log.csv",12,MUTED)); p.second->addWidget(sim); p.second->addStretch();
        pageStack->addWidget(p.first);
        connect(refreshPorts,&QPushButton::clicked,this,&MainWindow::refreshSerialPorts);
        connect(connectBtn,&QPushButton::clicked,this,&MainWindow::connectArduino);
        connect(disconnectBtn,&QPushButton::clicked,this,&MainWindow::disconnectArduino);
        refreshSerialPorts();
    }
    for(int i=0;i<navigationButtons.size();++i)connect(navigationButtons[i],&QPushButton::clicked,this,[this,i](){showPage(i);});

    timer->setInterval(1000);
    connect(timer,&QTimer::timeout,this,&MainWindow::updateTelemetry);
    connect(serialPort,&QSerialPort::readyRead,this,&MainWindow::readArduinoData);
    connect(startButton,&QPushButton::clicked,this,&MainWindow::startSimulation);
    connect(stopButton,&QPushButton::clicked,this,&MainWindow::stopSimulation);
    startButton->setEnabled(false); // The simulation starts automatically on launch.
    stopButton->setEnabled(true);
    connect(manualButton,&QPushButton::clicked,this,&MainWindow::toggleManualMode);
    connect(landButton,&QPushButton::clicked,this,[this]() {
        if (landed) return;
        const auto answer = QMessageBox::question(this, "Confirm Manual Landing",
            "Send LAND NOW command to the rocket?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer == QMessageBox::Yes) manualLand();
    });
    connect(resetButton,&QPushButton::clicked,this,[this]() {
        const auto answer = QMessageBox::question(this, "Reset Simulation",
            "Reset telemetry and start a new simulated flight?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer == QMessageBox::Yes) resetSimulation();
    });
    connect(logButton,&QPushButton::clicked,this,&MainWindow::toggleLogging);
    for(int i=0;i<7;i++){altitudeHistory.append(qMax(0.0,altitude-(6-i)*18.0));appendLogRow();}
    updateDashboard();

    // Open the CSV log in a user-writable application data folder.
    logFile->setFileName(applicationLogPath());
    const bool newLogFile = !QFile::exists(logFile->fileName());
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        loggingEnabled = false;
        logState->setText("○  Logging : OFF (file unavailable)");
        logState->setStyleSheet("color:#ffb454;font-size:12px;font-weight:600;");
        logButton->setText("Start Logging");
    } else if (newLogFile) {
        QTextStream out(logFile);
        out << "Time,Altitude_m,Temperature_C,Pressure_hPa,Battery_percent,Latitude,Longitude,Accel_X,Accel_Y,Accel_Z,Status\n";
        out.flush();
    }
    timer->start();
}

MainWindow::~MainWindow(){if(serialPort && serialPort->isOpen())serialPort->close();if(logFile->isOpen())logFile->close();}
QPushButton *MainWindow::addNavigationItem(QVBoxLayout *layout,const QString &icon,const QString &name,bool active){auto *b=new QPushButton(icon+"    "+name);b->setMinimumHeight(52);b->setCursor(Qt::PointingHandCursor);b->setStyleSheet(active?"QPushButton{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #087fc4,stop:1 #124b91);color:white;border:1px solid #29c7ff;border-radius:9px;text-align:left;padding-left:16px;font-size:14px;font-weight:bold;}":"QPushButton{background:transparent;color:#a9c2dc;border:1px solid transparent;border-radius:9px;text-align:left;padding-left:16px;font-size:14px;}QPushButton:hover{background:#102c49;color:#e8f7ff;border:1px solid #1d6594;}");layout->addWidget(b);return b;}
void MainWindow::showPage(int index){if(!pageStack||index<0||index>=pageStack->count())return;pageStack->setCurrentIndex(index);for(int i=0;i<navigationButtons.size();++i){bool a=i==index;navigationButtons[i]->setStyleSheet(a?"QPushButton{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #087fc4,stop:1 #124b91);color:white;border:1px solid #29c7ff;border-radius:9px;text-align:left;padding-left:16px;font-size:14px;font-weight:bold;}":"QPushButton{background:transparent;color:#a9c2dc;border:1px solid transparent;border-radius:9px;text-align:left;padding-left:16px;font-size:14px;}QPushButton:hover{background:#102c49;color:#e8f7ff;border:1px solid #1d6594;}");}}
void MainWindow::refreshSerialPorts(){
    if(!portCombo) return;
    const QString current=portCombo->currentData().toString();
    portCombo->clear();
    const auto ports=QSerialPortInfo::availablePorts();
    for(const auto &info:ports){
        QString text=info.portName();
        if(!info.description().isEmpty()) text += "  |  "+info.description();
        portCombo->addItem(text,info.portName());
    }
    int index=portCombo->findData(current); if(index>=0) portCombo->setCurrentIndex(index);
    if(portCombo->count()==0) portCombo->addItem("No serial ports found",QString());
}
void MainWindow::connectArduino(){
    if(serialPort->isOpen()) return;
    const QString portName=portCombo?portCombo->currentData().toString():QString();
    if(portName.isEmpty()){QMessageBox::warning(this,"Arduino Connection","No COM port detected. Connect the Arduino UNO R3 and refresh the port list.");return;}
    serialPort->setPortName(portName);
    const int selectedBaud = baudCombo ? baudCombo->currentText().toInt() : 115200;
    if (!serialPort->setBaudRate(selectedBaud)) {
        QMessageBox::warning(this, "Arduino Connection",
                             "Unsupported baud rate: " + QString::number(selectedBaud));
        return;
    }
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    if(!serialPort->open(QIODevice::ReadWrite)){
        QMessageBox::critical(this,"Arduino Connection","Cannot open "+portName+". Close Arduino Serial Monitor / other apps and try again.\n"+serialPort->errorString());
        return;
    }
    hardwareMode=true; landed=false; paused=false; velocity=0;
    serialBuffer.clear();
    if(serialStatusLabel) serialStatusLabel->setText("●  CONNECTED  |  " + portName + " @ " + QString::number(selectedBaud) + " baud  |  Waiting for data...");
    if(connectionValue) connectionValue->setText("●  CONNECTED  /  ARDUINO UNO R3");
    if(connectionValue) connectionValue->setStyleSheet("color:#28efad;font-size:12px;font-weight:600;");
    updateDashboard();
}
void MainWindow::disconnectArduino(){
    if(serialPort->isOpen()) serialPort->close();
    hardwareMode=false; serialBuffer.clear();
    if(serialStatusLabel) serialStatusLabel->setText("●  DISCONNECTED  |  Simulation data active");
    if(connectionValue) connectionValue->setText("●  CONNECTED  /  SIMULATION");
    if(connectionValue) connectionValue->setStyleSheet("color:#ffbd59;font-size:12px;font-weight:600;");
    paused=false; landed=false; velocity=0; updateDashboard();
}
void MainWindow::readArduinoData(){
    serialBuffer.append(serialPort->readAll());
    if(serialBuffer.size()>4096){serialBuffer=serialBuffer.right(1024);}
    int newline=-1;
    while((newline=serialBuffer.indexOf('\n'))>=0){
        QByteArray line=serialBuffer.left(newline).trimmed();
        serialBuffer.remove(0,newline+1);
        if(!line.isEmpty()) processArduinoLine(line);
    }
}
void MainWindow::processArduinoLine(const QByteArray &line){
    const QList<QByteArray> fields=line.split(',');
    if(fields.size()!=9) return;
    bool ok[9]; double v[9];
    for(int i=0;i<9;++i){v[i]=fields[i].trimmed().toDouble(&ok[i]);if(!ok[i] || !qIsFinite(v[i]))return;}
    if(v[0]<-1000 || v[0]>100000 || v[1]<-80 || v[1]>200 || v[2]<100 || v[2]>1200 || v[3]<0 || v[3]>100 || v[4]<-90 || v[4]>90 || v[5]<-180 || v[5]>180) return;
    altitude=v[0]; temperature=v[1]; pressure=v[2]; battery=v[3]; latitude=v[4]; longitude=v[5]; accelX=v[6]; accelY=v[7]; accelZ=v[8];
    velocity=0; landed=false; paused=false;
    altitudeHistory.append(altitude); if(altitudeHistory.size()>60)altitudeHistory.removeFirst();
    if(chartWidget){static_cast<AltitudeChart*>(chartWidget)->values=altitudeHistory;chartWidget->update();}
    if(serialStatusLabel) serialStatusLabel->setText("●  CONNECTED  |  TELEMETRY RECEIVED  |  "+QDateTime::currentDateTime().toString("HH:mm:ss"));
    updateDashboard(); appendLogRow(); if(loggingEnabled)writeCsvRow();
}

void MainWindow::updateTelemetry(){
    if(hardwareMode || landed || paused) return;
    ++elapsedSeconds;
    const double climb = (manualMode ? 0.3 : 0.8) + QRandomGenerator::global()->generateDouble()*(manualMode ? 0.7 : 2.2);
    const double previousAltitude = altitude;
    altitude = qMin(390.0, altitude + climb);
    velocity = qFuzzyCompare(altitude + 1.0, previousAltitude + 1.0) ? 0.0
              : (manualMode ? 0.5 : 1.5) + QRandomGenerator::global()->generateDouble()*(manualMode ? 0.8 : 2.0);
    temperature = 26.2 + QRandomGenerator::global()->generateDouble()*1.2;
    battery = qMax(0.0, battery - (0.02 + QRandomGenerator::global()->generateDouble()*0.04));
    pressure = 977.2 - QRandomGenerator::global()->generateDouble()*0.4;
    altitudeHistory.append(altitude);
    if(altitudeHistory.size()>60) altitudeHistory.removeFirst();
    static_cast<AltitudeChart*>(chartWidget)->values=altitudeHistory;
    chartWidget->update();
    updateDashboard();
    appendLogRow();
    if(loggingEnabled) writeCsvRow();
    if(battery<=10.0){
        velocity=0;
        paused=true;
        timer->stop();
        updateDashboard();
        appendLogRow();
        if(loggingEnabled) writeCsvRow();
        QMessageBox::warning(this,"Low Battery","Battery is at or below 10%. Simulation paused for safety. Reset to begin a new mission.");
    }
}
void MainWindow::updateDashboard(){altitudeValue->setText(QString::number(altitude,'f',1)+" m");visualAltitudeValue->setText(QString::number(altitude,'f',1)+" m");temperatureValue->setText(QString::number(temperature,'f',1)+" °C");batteryValue->setText(QString::number(battery,'f',1)+" %");QString state=landed?"LANDED":(paused?"STOPPED":(manualMode?"MANUAL":"FLIGHT"));statusValue->setText(state);velocityValue->setText(QString::number(velocity,'f',1)+" m/s");systemState->setText(state);}
void MainWindow::appendLogRow(){int r=logTable->rowCount();logTable->insertRow(r);QStringList vals{QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"),QString::number(altitude,'f',1),QString::number(temperature,'f',1),QString::number(battery,'f',1),landed?"LANDED":(paused?"STOPPED":(manualMode?"MANUAL":"FLIGHT"))};for(int c=0;c<vals.size();++c)logTable->setItem(r,c,new QTableWidgetItem(vals[c]));while(logTable->rowCount()>100)logTable->removeRow(0);logTable->scrollToBottom();}
void MainWindow::startSimulation(){
    if(hardwareMode){QMessageBox::information(this,"Hardware Connected","START simulation control is disabled while Arduino telemetry is connected. This application is receiving sensor data only.");return;}
    if(landed)return;
    paused=false;
    timer->start();
    startButton->setEnabled(false);
    stopButton->setEnabled(true);
    updateDashboard();
    appendLogRow();
}
void MainWindow::stopSimulation(){
    if(hardwareMode){QMessageBox::information(this,"Hardware Connected","STOP does not send a flight command to hardware. Disconnect Arduino to control the local simulation.");return;}
    if(landed)return;
    paused=true;
    timer->stop();
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    velocity=0;
    updateDashboard();
    appendLogRow();
    if(loggingEnabled)writeCsvRow();
}
void MainWindow::toggleManualMode(){
    if(hardwareMode){QMessageBox::information(this,"Hardware Connected","Manual actuator commands are not enabled. Telemetry-only hardware mode is active.");return;}
    if(landed)return;
    manualMode=!manualMode;
    manualButton->setText(manualMode?"✋   MANUAL: ON":"✋   MANUAL: OFF");
    manualButton->setStyleSheet(manualMode?"QPushButton{background:#075b50;color:#8fffe0;border:1px solid #12c99a;border-radius:8px;font-size:13px;font-weight:bold;}":"QPushButton{background:#17345c;color:#8fc2ff;border:1px solid #2879c9;border-radius:8px;font-size:13px;font-weight:bold;}");
    updateDashboard();
    appendLogRow();
}

void MainWindow::manualLand(){
    if(hardwareMode){QMessageBox::warning(this,"Safety Lockout","LAND NOW is disabled for physical hardware. This version only receives telemetry and cannot control rocket actuators.");return;}
    if (landed) return;
    landed=true;
    timer->stop();
    velocity=0;
    landButton->setText("✓  ROCKET LANDED");
    landButton->setEnabled(false);
    startButton->setEnabled(false);
    stopButton->setEnabled(false);
    manualButton->setEnabled(false);
    landButton->setStyleSheet("QPushButton{background:#17634c;color:#d9fff1;border:1px solid #20c98a;border-radius:8px;font-size:14px;font-weight:bold;}");
    updateDashboard();
    appendLogRow();
    if (loggingEnabled) writeCsvRow();
}
void MainWindow::resetSimulation(){
    landed=false;
    paused=false;
    manualMode=false;
    manualButton->setText("✋   MANUAL: OFF");
    manualButton->setStyleSheet("QPushButton{background:#17345c;color:#8fc2ff;border:1px solid #2879c9;border-radius:8px;font-size:13px;font-weight:bold;}");
    altitude=0;
    temperature=25;
    battery=100;
    velocity=0;
    pressure=1013.2;
    elapsedSeconds=0;
    altitudeHistory.clear();
    logTable->setRowCount(0);
    static_cast<AltitudeChart*>(chartWidget)->values.clear();
    chartWidget->update();
    landButton->setText("◇   LAND NOW");
    landButton->setEnabled(true);
    startButton->setEnabled(false);
    stopButton->setEnabled(true);
    manualButton->setEnabled(true);
    landButton->setStyleSheet("QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #ff526b,stop:1 #bd183d);color:white;border:1px solid #ff8291;border-radius:8px;font-size:15px;font-weight:bold;padding:10px;}QPushButton:hover{background:#ff6379;}QPushButton:pressed{background:#c91d36;}");
    updateDashboard();
    appendLogRow();
    if(loggingEnabled) writeCsvRow();
    timer->start();
}
void MainWindow::toggleLogging(){
    if(!loggingEnabled){
        logFile->setFileName(applicationLogPath());
        const bool fresh=!QFile::exists(logFile->fileName());
        if(!logFile->open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)){
            QMessageBox::warning(this,"Data Logging","เปิดไฟล์ CSV ไม่สำเร็จ\n"+logFile->fileName());
            return;
        }
        if(fresh){QTextStream out(logFile);out<<"Time,Altitude_m,Temperature_C,Pressure_hPa,Battery_percent,Latitude,Longitude,Accel_X,Accel_Y,Accel_Z,Status\n";out.flush();}
        loggingEnabled=true;
        logButton->setText("Stop Logging");
        logState->setText("◉  Logging : ON");
        logState->setStyleSheet("color:#18d889;font-size:12px;font-weight:600;");
    }else{
        if(logFile->isOpen()){logFile->flush();logFile->close();}
        loggingEnabled=false;
        logButton->setText("Start Logging");
        logState->setText("○  Logging : OFF");
        logState->setStyleSheet("color:#91a6c2;font-size:12px;font-weight:600;");
    }
}
void MainWindow::writeCsvRow(){
    if(!loggingEnabled || !logFile->isOpen()) return;
    QTextStream out(logFile);
    out << QDateTime::currentDateTime().toString(Qt::ISODate) << ','
        << QString::number(altitude,'f',2) << ','
        << QString::number(temperature,'f',2) << ','
        << QString::number(pressure,'f',2) << ','
        << QString::number(battery,'f',2) << ','
        << QString::number(latitude,'f',6) << ','
        << QString::number(longitude,'f',6) << ','
        << QString::number(accelX,'f',3) << ','
        << QString::number(accelY,'f',3) << ','
        << QString::number(accelZ,'f',3) << ','
        << (landed?"LANDED":(paused?"STOPPED":(manualMode?"MANUAL":(hardwareMode?"ARDUINO":"FLIGHT")))) << '\n';
    out.flush();
    if(out.status()!=QTextStream::Ok){
        loggingEnabled=false;
        logFile->close();
        logButton->setText("Start Logging");
        logState->setText("○  Logging : ERROR");
        logState->setStyleSheet("color:#ff657a;font-size:12px;font-weight:600;");
    }
}
