/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.14.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QTabWidget *tabWidget;
    QWidget *tab_1;
    QFrame *line;
    QPushButton *btnImage;
    QPushButton *btnCalc;
    QLabel *labelResult;
    QLabel *displayResult;
    QScrollArea *scrollResult;
    QWidget *scrollAreaWidgetContents;
    QLabel *displayImage;
    QPushButton *btnExport;
    QPushButton *btnClean;
    QPushButton *btnRemove;
    QFrame *frame;
    QLineEdit *replicate;
    QLineEdit *species;
    QLabel *label_5;
    QLabel *label_4;
    QLabel *label_3;
    QLineEdit *treatment;
    QLabel *label_6;
    QFrame *frame_2;
    QCheckBox *checkBoxAveDev;
    QLabel *label_2;
    QCheckBox *checkBoxWid;
    QLabel *pathImage;
    QCheckBox *checkBoxLen;
    QLineEdit *areaSquare;
    QCheckBox *checkBoxWidLen;
    QLabel *label;
    QCheckBox *checkBoxArea;
    QLabel *label_7;
    QCheckBox *checkBoxSumAreas;
    QCheckBox *checkBoxPerimeter;
    QWidget *tab_3;
    QScrollArea *scrollHist;
    QWidget *scrollAreaWidgetContents_2;
    QLabel *displayHist;
    QPushButton *btnClearHistory;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1346, 674);
        MainWindow->setMaximumSize(QSize(1400, 710));
        MainWindow->setLayoutDirection(Qt::RightToLeft);
        MainWindow->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        centralWidget->setAutoFillBackground(true);
        tabWidget = new QTabWidget(centralWidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setGeometry(QRect(0, 0, 1351, 681));
        tabWidget->setLayoutDirection(Qt::LeftToRight);
        tab_1 = new QWidget();
        tab_1->setObjectName(QString::fromUtf8("tab_1"));
        line = new QFrame(tab_1);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(20, 150, 1291, 20));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        btnImage = new QPushButton(tab_1);
        btnImage->setObjectName(QString::fromUtf8("btnImage"));
        btnImage->setGeometry(QRect(400, 130, 131, 23));
        btnCalc = new QPushButton(tab_1);
        btnCalc->setObjectName(QString::fromUtf8("btnCalc"));
        btnCalc->setGeometry(QRect(620, 130, 75, 23));
        labelResult = new QLabel(tab_1);
        labelResult->setObjectName(QString::fromUtf8("labelResult"));
        labelResult->setEnabled(false);
        labelResult->setGeometry(QRect(1170, 170, 41, 20));
        displayResult = new QLabel(tab_1);
        displayResult->setObjectName(QString::fromUtf8("displayResult"));
        displayResult->setGeometry(QRect(880, 170, 171, 481));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(displayResult->sizePolicy().hasHeightForWidth());
        displayResult->setSizePolicy(sizePolicy);
        scrollResult = new QScrollArea(tab_1);
        scrollResult->setObjectName(QString::fromUtf8("scrollResult"));
        scrollResult->setGeometry(QRect(1060, 200, 251, 381));
        scrollResult->setWidgetResizable(true);
        scrollResult->setAlignment(Qt::AlignCenter);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 249, 379));
        scrollResult->setWidget(scrollAreaWidgetContents);
        displayImage = new QLabel(tab_1);
        displayImage->setObjectName(QString::fromUtf8("displayImage"));
        displayImage->setEnabled(true);
        displayImage->setGeometry(QRect(330, 180, 651, 451));
        displayImage->setToolTipDuration(0);
        displayImage->setAlignment(Qt::AlignCenter);
        btnExport = new QPushButton(tab_1);
        btnExport->setObjectName(QString::fromUtf8("btnExport"));
        btnExport->setGeometry(QRect(1220, 600, 93, 28));
        btnClean = new QPushButton(tab_1);
        btnClean->setObjectName(QString::fromUtf8("btnClean"));
        btnClean->setGeometry(QRect(780, 130, 75, 23));
        btnRemove = new QPushButton(tab_1);
        btnRemove->setObjectName(QString::fromUtf8("btnRemove"));
        btnRemove->setGeometry(QRect(1060, 600, 93, 28));
        frame = new QFrame(tab_1);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setGeometry(QRect(840, 0, 471, 121));
        frame->setFrameShape(QFrame::NoFrame);
        frame->setFrameShadow(QFrame::Sunken);
        replicate = new QLineEdit(frame);
        replicate->setObjectName(QString::fromUtf8("rep"));
        replicate->setGeometry(QRect(90, 90, 291, 20));
        replicate->setAlignment(Qt::AlignCenter);
        species = new QLineEdit(frame);
        species->setObjectName(QString::fromUtf8("spe"));
        species->setGeometry(QRect(90, 30, 291, 20));
        species->setAlignment(Qt::AlignCenter);
        label_5 = new QLabel(frame);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(10, 90, 71, 16));
        label_4 = new QLabel(frame);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(10, 60, 71, 16));
        label_3 = new QLabel(frame);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(10, 30, 56, 16));
        treatment = new QLineEdit(frame);
        treatment->setObjectName(QString::fromUtf8("trat"));
        treatment->setGeometry(QRect(90, 60, 291, 20));
        treatment->setAlignment(Qt::AlignCenter);
        label_6 = new QLabel(frame);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setEnabled(false);
        label_6->setGeometry(QRect(10, 0, 121, 16));
        frame_2 = new QFrame(tab_1);
        frame_2->setObjectName(QString::fromUtf8("frame_2"));
        frame_2->setGeometry(QRect(20, 0, 821, 121));
        frame_2->setFrameShape(QFrame::NoFrame);
        frame_2->setFrameShadow(QFrame::Plain);
        checkBoxAveDev = new QCheckBox(frame_2);
        checkBoxAveDev->setObjectName(QString::fromUtf8("checkBoxAveDev"));
        checkBoxAveDev->setGeometry(QRect(560, 90, 211, 20));
        label_2 = new QLabel(frame_2);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(10, 60, 51, 16));
        checkBoxWid = new QCheckBox(frame_2);
        checkBoxWid->setObjectName(QString::fromUtf8("checkBoxWid"));
        checkBoxWid->setGeometry(QRect(180, 90, 61, 20));
        pathImage = new QLabel(frame_2);
        pathImage->setObjectName(QString::fromUtf8("pathImage"));
        pathImage->setEnabled(false);
        pathImage->setGeometry(QRect(70, 60, 681, 20));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(pathImage->sizePolicy().hasHeightForWidth());
        pathImage->setSizePolicy(sizePolicy1);
        pathImage->setScaledContents(true);
        pathImage->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        checkBoxLen = new QCheckBox(frame_2);
        checkBoxLen->setObjectName(QString::fromUtf8("checkBoxLen"));
        checkBoxLen->setGeometry(QRect(260, 90, 71, 20));
        areaSquare = new QLineEdit(frame_2);
        areaSquare->setObjectName(QString::fromUtf8("areaSquare"));
        areaSquare->setGeometry(QRect(170, 30, 71, 20));
        areaSquare->setAlignment(Qt::AlignCenter);
        checkBoxWidLen = new QCheckBox(frame_2);
        checkBoxWidLen->setObjectName(QString::fromUtf8("checkBoxWidLen"));
        checkBoxWidLen->setGeometry(QRect(340, 90, 111, 20));
        label = new QLabel(frame_2);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(10, 30, 151, 16));
        checkBoxArea = new QCheckBox(frame_2);
        checkBoxArea->setObjectName(QString::fromUtf8("checkBoxArea"));
        checkBoxArea->setGeometry(QRect(10, 90, 51, 20));
        checkBoxArea->setChecked(true);
        label_7 = new QLabel(frame_2);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setEnabled(false);
        label_7->setGeometry(QRect(10, 0, 56, 16));
        checkBoxSumAreas = new QCheckBox(frame_2);
        checkBoxSumAreas->setObjectName(QString::fromUtf8("checkBoxSumAreas"));
        checkBoxSumAreas->setGeometry(QRect(80, 90, 91, 20));
        checkBoxPerimeter = new QCheckBox(frame_2);
        checkBoxPerimeter->setObjectName(QString::fromUtf8("checkBoxPerimeter"));
        checkBoxPerimeter->setGeometry(QRect(460, 90, 83, 20));
        tabWidget->addTab(tab_1, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        scrollHist = new QScrollArea(tab_3);
        scrollHist->setObjectName(QString::fromUtf8("scrollHist"));
        scrollHist->setGeometry(QRect(30, 20, 391, 591));
        scrollHist->setWidgetResizable(true);
        scrollAreaWidgetContents_2 = new QWidget();
        scrollAreaWidgetContents_2->setObjectName(QString::fromUtf8("scrollAreaWidgetContents_2"));
        scrollAreaWidgetContents_2->setGeometry(QRect(0, 0, 389, 589));
        scrollHist->setWidget(scrollAreaWidgetContents_2);
        displayHist = new QLabel(tab_3);
        displayHist->setObjectName(QString::fromUtf8("displayHist"));
        displayHist->setGeometry(QRect(330, -120, 271, 661));
        sizePolicy.setHeightForWidth(displayHist->sizePolicy().hasHeightForWidth());
        displayHist->setSizePolicy(sizePolicy);
        displayHist->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        btnClearHistory = new QPushButton(tab_3);
        btnClearHistory->setObjectName(QString::fromUtf8("btnClearHistory"));
        btnClearHistory->setGeometry(QRect(710, 290, 321, 23));
        tabWidget->addTab(tab_3, QString());
        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        btnImage->setText(QCoreApplication::translate("MainWindow", "Select an image", nullptr));
        btnCalc->setText(QCoreApplication::translate("MainWindow", "Compute", nullptr));
        labelResult->setText(QCoreApplication::translate("MainWindow", "Result", nullptr));
        displayResult->setText(QString());
        displayImage->setText(QCoreApplication::translate("MainWindow", "Select image will come here.", nullptr));
        btnExport->setText(QCoreApplication::translate("MainWindow", "Result export", nullptr));
        btnClean->setText(QCoreApplication::translate("MainWindow", "Clean", nullptr));
        btnRemove->setText(QCoreApplication::translate("MainWindow", "Remove", nullptr));
        replicate->setText(QString());
        replicate->setPlaceholderText(QString());
        species->setText(QString());
        species->setPlaceholderText(QString());
        label_5->setText(QCoreApplication::translate("MainWindow", "Replicate:", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Treatment:", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Species:", nullptr));
        treatment->setText(QString());
        treatment->setPlaceholderText(QString());
        label_6->setText(QCoreApplication::translate("MainWindow", "Experimental design:", nullptr));
        checkBoxAveDev->setText(QCoreApplication::translate("MainWindow", "Average and standard deviation", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Image:", nullptr));
        checkBoxWid->setText(QCoreApplication::translate("MainWindow", "Width", nullptr));
        pathImage->setText(QCoreApplication::translate("MainWindow", "The image path will come here", nullptr));
        checkBoxLen->setText(QCoreApplication::translate("MainWindow", "Length", nullptr));
        areaSquare->setText(QCoreApplication::translate("MainWindow", "1", nullptr));
        areaSquare->setPlaceholderText(QString());
        checkBoxWidLen->setText(QCoreApplication::translate("MainWindow", "Width/Length", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Scale pattern area (cm\302\262):", nullptr));
        checkBoxArea->setText(QCoreApplication::translate("MainWindow", "Area", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "Test:", nullptr));
        checkBoxSumAreas->setText(QCoreApplication::translate("MainWindow", "Sum areas", nullptr));
        checkBoxPerimeter->setText(QCoreApplication::translate("MainWindow", "Perimeter", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_1), QCoreApplication::translate("MainWindow", "Home", nullptr));
        displayHist->setText(QString());
        btnClearHistory->setText(QCoreApplication::translate("MainWindow", "Do you want to clear the historic?", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QCoreApplication::translate("MainWindow", "Historic", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
