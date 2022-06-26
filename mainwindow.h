#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QLabel>

#include <QtSql>
#include <QDebug>
#include <QFileInfo>



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnCalc_clicked();

    void on_btnExport_clicked();

    void on_btnClean_clicked();

    void on_btnRemove_clicked();

    void on_tabWidget_currentChanged();

    void on_btnClearHistory_clicked();

    void on_btnImage_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
