#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QPoint>
#include <QIcon>
#include <QVector>
#include <QButtonGroup>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onToggle(bool checked);
    void onJiggle();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onStartupToggled(bool checked);

private:
    void setupTrayIcon();
    void applyStyles();
    void setAwakeState(bool awake);
    QIcon buildCoffeeIcon(double steamOffset);
    void updateAnimatedIcon();

    Ui::MainWindow *ui;
    QSystemTrayIcon *m_trayIcon;
    QTimer *m_jiggleTimer;
    bool m_active = false;
    bool m_jigglePhase = false;
    QPoint m_lastCursorPos;
    QTimer *m_iconAnimTimer;
    QVector<QIcon> m_iconFrames;
    int m_iconFrame = 0;
    QButtonGroup *m_intervalGroup;
    QString m_checkmarkPath;
    QIcon m_playIcon;
    QIcon m_stopIcon;
};
#endif // MAINWINDOW_H
