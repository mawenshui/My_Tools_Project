#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

class DataHandler;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

Q_SIGNALS:
    void StartWork(const QString&, qint64);

private slots:
    void process(int);

    void on_pbt_Choose_clicked();

    void on_pbt_Send_clicked();

    void on_pbt_ContinueSend_clicked();

    void on_checkBox_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;

    bool m_bSend;
    bool m_bContinue;
    qint64  m_fileSeekPos;
    Qt::CheckState  m_isRemberPos;
    QString m_fileName;

    DataHandler *m_handler;
    QThread     *m_handlerThread;
};

#endif // MAINWINDOW_H
