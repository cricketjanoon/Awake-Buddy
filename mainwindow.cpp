#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QCloseEvent>
#include <QMenu>
#include <QCursor>
#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygon>
#include <QtMath>
#include <QSettings>
#include <QDir>

#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_trayIcon(nullptr)
    , m_jiggleTimer(new QTimer(this))
    , m_iconAnimTimer(new QTimer(this))
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    // Generate checkmark icon for checkbox
    QPixmap checkPix(16, 16);
    checkPix.fill(Qt::transparent);
    QPainter cp(&checkPix);
    cp.setRenderHint(QPainter::Antialiasing);
    cp.setPen(QPen(QColor("#1e1e2e"), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    cp.drawLine(3, 8, 6, 12);
    cp.drawLine(6, 12, 13, 4);
    cp.end();
    m_checkmarkPath = QDir::temp().filePath("awakebuddy_check.png");
    checkPix.save(m_checkmarkPath);

    // Generate play icon (triangle)
    QPixmap playPix(32, 32);
    playPix.fill(Qt::transparent);
    QPainter pp(&playPix);
    pp.setRenderHint(QPainter::Antialiasing);
    pp.setPen(Qt::NoPen);
    pp.setBrush(QColor("#1e1e2e"));
    QPolygonF playTri;
    playTri << QPointF(8, 4) << QPointF(8, 28) << QPointF(28, 16);
    pp.drawPolygon(playTri);
    pp.end();
    m_playIcon = QIcon(playPix);

    // Generate stop icon (square)
    QPixmap stopPix(32, 32);
    stopPix.fill(Qt::transparent);
    QPainter sp(&stopPix);
    sp.setRenderHint(QPainter::Antialiasing);
    sp.setPen(Qt::NoPen);
    sp.setBrush(QColor("#1e1e2e"));
    sp.drawRoundedRect(6, 6, 20, 20, 3, 3);
    sp.end();
    m_stopIcon = QIcon(stopPix);

    applyStyles();
    setupTrayIcon();

    connect(ui->toggleButton, &QPushButton::toggled, this, &MainWindow::onToggle);
    connect(m_jiggleTimer, &QTimer::timeout, this, &MainWindow::onJiggle);

    ui->toggleButton->setText("");
    ui->toggleButton->setIcon(m_playIcon);
    ui->toggleButton->setIconSize(QSize(24, 24));

    // Interval button group — exclusive toggle
    m_intervalGroup = new QButtonGroup(this);
    m_intervalGroup->setExclusive(true);
    m_intervalGroup->addButton(ui->intervalBtn5, 5);
    m_intervalGroup->addButton(ui->intervalBtn10, 10);
    m_intervalGroup->addButton(ui->intervalBtn30, 30);
    m_intervalGroup->addButton(ui->intervalBtn60, 60);
    ui->intervalBtn30->setChecked(true);

    // Startup checkbox
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);
    ui->startupCheckBox->setChecked(reg.contains("AwakeBuddy"));
    connect(ui->startupCheckBox, &QCheckBox::toggled, this, &MainWindow::onStartupToggled);
}

MainWindow::~MainWindow()
{
    setAwakeState(false);
    delete ui;
}

void MainWindow::applyStyles()
{
    QString checkPath = m_checkmarkPath;
    checkPath.replace("\\", "/");
    setStyleSheet(QStringLiteral(R"(
        QMainWindow {
            background-color: #1e1e2e;
        }
        #titleLabel {
            font-size: 18px;
            font-weight: 600;
            color: #cdd6f4;
            letter-spacing: 1px;
        }
        #statusLabel {
            font-size: 11px;
            color: #6c7086;
        }
        #toggleButton {
            font-size: 13px;
            font-weight: 600;
            border: none;
            border-radius: 25px;
            background-color: #a6e3a1;
            color: #1e1e2e;
        }
        #toggleButton:hover {
            background-color: #b4f0aa;
        }
        #toggleButton:checked {
            background-color: #f38ba8;
            color: #1e1e2e;
        }
        #toggleButton:checked:hover {
            background-color: #f5a0b8;
        }
        #intervalLabel {
            font-size: 11px;
            color: #585b70;
        }
        QSpinBox {
            padding: 4px 8px;
            border: 1px solid #313244;
            border-radius: 6px;
            background: #181825;
            color: #cdd6f4;
            selection-background-color: #45475a;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            border: none;
            background: #313244;
            width: 16px;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background: #45475a;
        }
        #intervalBtn5, #intervalBtn10, #intervalBtn30, #intervalBtn60 {
            font-size: 11px;
            font-weight: 600;
            border: none;
            border-radius: 6px;
            padding: 4px 0px;
            background: #313244;
            color: #6c7086;
        }
        #intervalBtn5:hover, #intervalBtn10:hover, #intervalBtn30:hover, #intervalBtn60:hover {
            background: #45475a;
            color: #cdd6f4;
        }
        #intervalBtn5:checked, #intervalBtn10:checked, #intervalBtn30:checked, #intervalBtn60:checked {
            background: #89b4fa;
            color: #1e1e2e;
        }
        #intervalBtn5:checked:hover, #intervalBtn10:checked:hover, #intervalBtn30:checked:hover, #intervalBtn60:checked:hover {
            background: #a4c4fb;
        }
        QCheckBox {
            font-size: 11px;
            color: #585b70;
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 2px solid #45475a;
            border-radius: 4px;
            background: #181825;
        }
        QCheckBox::indicator:checked {
            background: #a6e3a1;
            border-color: #a6e3a1;
            image: url(%1);
        }
    )").arg(checkPath));
}

void MainWindow::setupTrayIcon()
{
    // Pre-generate animation frames with varying steam positions
    const int frameCount = 8;
    for (int i = 0; i < frameCount; ++i) {
        double offset = static_cast<double>(i) / frameCount;
        m_iconFrames.append(buildCoffeeIcon(offset));
    }

    QIcon staticIcon = buildCoffeeIcon(0);
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(staticIcon);
    setWindowIcon(staticIcon);
    m_trayIcon->setToolTip("Awake Buddy");

    connect(m_iconAnimTimer, &QTimer::timeout, this, &MainWindow::updateAnimatedIcon);

    QMenu *trayMenu = new QMenu(this);
    QAction *showAction = trayMenu->addAction("Show");
    connect(showAction, &QAction::triggered, this, [this]() {
        showNormal();
        activateWindow();
    });

    QAction *toggleAction = trayMenu->addAction("Start");
    connect(toggleAction, &QAction::triggered, this, [this, toggleAction]() {
        ui->toggleButton->toggle();
    });

    trayMenu->addSeparator();
    QAction *quitAction = trayMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(trayMenu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

void MainWindow::onToggle(bool checked)
{
    m_active = checked;
    if (checked) {
        ui->toggleButton->setIcon(m_stopIcon);
        ui->toggleButton->setText("");
        ui->statusLabel->setText("Keeping your PC awake!");
        for (auto *btn : m_intervalGroup->buttons())
            btn->setEnabled(false);

        int intervalMs = m_intervalGroup->checkedId() * 1000;
        m_lastCursorPos = QCursor::pos();
        m_jiggleTimer->start(intervalMs);
        setAwakeState(true);
        m_iconFrame = 0;
        m_iconAnimTimer->start(300);
    } else {
        ui->toggleButton->setIcon(m_playIcon);
        ui->toggleButton->setText("");
        ui->statusLabel->setText("Your PC can sleep");
        for (auto *btn : m_intervalGroup->buttons())
            btn->setEnabled(true);

        m_jiggleTimer->stop();
        setAwakeState(false);
        m_iconAnimTimer->stop();
        m_trayIcon->setIcon(m_iconFrames.first());
    }

    // Update tray menu toggle text
    QMenu *menu = m_trayIcon->contextMenu();
    if (menu && menu->actions().size() >= 2) {
        menu->actions().at(1)->setText(checked ? "Stop" : "Start");
    }
}

void MainWindow::onJiggle()
{
    QPoint currentPos = QCursor::pos();
    if (currentPos != m_lastCursorPos) {
        m_lastCursorPos = currentPos;
        return;
    }

    INPUT inp = {};
    inp.type = INPUT_MOUSE;
    inp.mi.dx = m_jigglePhase ? -5 : 5;
    inp.mi.dy = m_jigglePhase ? -5 : 5;
    inp.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &inp, sizeof(INPUT));
    m_jigglePhase = !m_jigglePhase;
    m_lastCursorPos = QCursor::pos();
}

void MainWindow::setAwakeState(bool awake)
{
    if (awake) {
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    } else {
        SetThreadExecutionState(ES_CONTINUOUS);
    }
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        showNormal();
        activateWindow();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            hide();
            m_trayIcon->showMessage("Awake Buddy",
                                    m_active ? "Still keeping your PC awake!" : "Running in the background.",
                                    QSystemTrayIcon::Information, 2000);
            event->ignore();
            return;
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    m_trayIcon->showMessage("Awake Buddy",
                            "Minimized to tray. Right-click tray icon to quit.",
                            QSystemTrayIcon::Information, 2000);
    event->ignore();
}

void MainWindow::onStartupToggled(bool checked)
{
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);
    if (checked) {
        reg.setValue("AwakeBuddy",
                     QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    } else {
        reg.remove("AwakeBuddy");
    }
}

QIcon MainWindow::buildCoffeeIcon(double steamOffset)
{
    // Render at 256x256 for sharp scaling
    const int sz = 256;
    const double s = sz / 64.0;
    QPixmap pix(sz, sz);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // --- Owl icon (optimized for small sizes) ---
    // Dark outline behind body for contrast on any background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#11111b"));
    p.drawEllipse(QPointF(32*s, 34*s), 30*s, 30*s);

    // Body — fills most of the canvas
    p.setBrush(QColor("#89b4fa"));
    p.drawEllipse(QPointF(32*s, 34*s), 27*s, 27*s);

    // Ear tufts — larger
    QPolygonF leftEar, rightEar;
    leftEar << QPointF(8*s, 14*s) << QPointF(18*s, 6*s) << QPointF(16*s, 20*s);
    rightEar << QPointF(56*s, 14*s) << QPointF(46*s, 6*s) << QPointF(48*s, 20*s);
    p.setBrush(QColor("#74c7ec"));
    p.drawPolygon(leftEar);
    p.drawPolygon(rightEar);

    // Eye whites — large, prominent
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(QPointF(21*s, 32*s), 11*s, 11*s);
    p.drawEllipse(QPointF(43*s, 32*s), 11*s, 11*s);

    // Pupils — animate blink
    double blink = qAbs(qSin(steamOffset * 6.2832));
    double pupilH = 7.0 * s * blink;
    if (pupilH < 1.5 * s) pupilH = 1.5 * s;
    p.setBrush(QColor("#1e1e2e"));
    p.drawEllipse(QRectF(15*s, 32*s - pupilH, 12*s, pupilH * 2));
    p.drawEllipse(QRectF(37*s, 32*s - pupilH, 12*s, pupilH * 2));

    // Eye shine — larger
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(QPointF(18*s, 29*s), 3*s, 3*s);
    p.drawEllipse(QPointF(40*s, 29*s), 3*s, 3*s);

    // Beak — bigger
    QPolygonF beak;
    beak << QPointF(27*s, 42*s) << QPointF(32*s, 50*s) << QPointF(37*s, 42*s);
    p.setBrush(QColor("#fab387"));
    p.drawPolygon(beak);

    p.end();

    // Build icon with multiple sizes
    QIcon icon;
    icon.addPixmap(pix);
    icon.addPixmap(pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon.addPixmap(pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon.addPixmap(pix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon.addPixmap(pix.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon.addPixmap(pix.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    return icon;
}

void MainWindow::updateAnimatedIcon()
{
    m_iconFrame = (m_iconFrame + 1) % m_iconFrames.size();
    m_trayIcon->setIcon(m_iconFrames[m_iconFrame]);
}

bool MainWindow::exportIcon(const QString &path)
{
    QIcon icon = buildCoffeeIcon(0);
    QPixmap pix = icon.pixmap(256, 256);
    return pix.save(path);
}
