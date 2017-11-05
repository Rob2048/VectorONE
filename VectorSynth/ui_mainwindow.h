/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionNew;
    QWidget *centralWidget;
    QStatusBar *statusBar;
    QDockWidget *dockProps;
    QWidget *dockWidgetContents_2;
    QVBoxLayout *verticalLayout;
    QLabel *label_16;
    QVBoxLayout *verticalLayout_11;
    QPushButton *btnResetFrameIds;
    QPushButton *btnStartRecording;
    QFormLayout *formLayout_3;
    QLabel *label_7;
    QLineEdit *txtFps;
    QLabel *label_13;
    QFormLayout *formLayout;
    QLabel *label_12;
    QLineEdit *txtId;
    QLabel *label;
    QLineEdit *txtExposure;
    QLabel *label_6;
    QLineEdit *txtIso;
    QLabel *label_3;
    QSlider *horizontalSlider_2;
    QLabel *label_4;
    QSlider *horizontalSlider_3;
    QLabel *label_10;
    QLineEdit *txtFrameSkip;
    QLabel *label_8;
    QCheckBox *chkDrawGuides;
    QLabel *label_9;
    QCheckBox *chkMarkers;
    QLabel *label_11;
    QCheckBox *chkFindCalib;
    QLabel *label_5;
    QVBoxLayout *verticalLayout_4;
    QLabel *lblCalibImageCount;
    QLabel *lblCalibError;
    QPushButton *btnStartCalibration;
    QPushButton *btnStopCalibration;
    QPushButton *btnGenerateMask;
    QSpacerItem *verticalSpacer_3;
    QDockWidget *dockTimeline;
    QWidget *dockWidgetContents_3;
    QHBoxLayout *horizontalLayout_4;
    QVBoxLayout *verticalLayout_14;
    QSpacerItem *verticalSpacer_2;
    QLabel *lblTimecode;
    QHBoxLayout *timelineLayout;
    QLineEdit *txtPlaySpeed;
    QPushButton *btnPrevFrameJump;
    QPushButton *btnPrevFrame;
    QPushButton *btnPlay;
    QPushButton *btnNextFrame;
    QPushButton *btnNextFrameJump;
    QDockWidget *dockTrackers;
    QWidget *dockWidgetContents_4;
    QVBoxLayout *verticalLayout_3;
    QWidget *trackerViewMain;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *toolbar;
    QToolButton *btnToggleUpdate;
    QToolButton *btnToggleMask;
    QFrame *line;
    QToolButton *btnToggleDistortedMarkers;
    QToolButton *btnToggleUndistortedMarkers;
    QToolButton *btnToggleReprojectedMarkers;
    QFrame *line_2;
    QToolButton *btnTogglePixelGrid;
    QToolButton *btnToggleRays_2;
    QSpacerItem *horizontalSpacer;
    QWidget *widget;
    QVBoxLayout *verticalLayout_13;
    QVBoxLayout *cameraViewPanel;
    QDockWidget *dockSynth;
    QWidget *dockWidgetContents;
    QVBoxLayout *verticalLayout_5;
    QLabel *label_18;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *btnLoadTake;
    QPushButton *btnSaveTake;
    QLabel *label_14;
    QVBoxLayout *verticalLayout_7;
    QPushButton *btnBuildFundamental;
    QPushButton *btnBundleAdjust;
    QPushButton *btnAssignWorldBasis;
    QLabel *label_15;
    QVBoxLayout *verticalLayout_12;
    QPushButton *btnBuild3D;
    QPushButton *btnIdentify3D;
    QSpacerItem *verticalSpacer;
    QDockWidget *dockSceneView;
    QWidget *sceneViewDockContents;
    QVBoxLayout *verticalLayout_6;
    QHBoxLayout *toolbar_2;
    QToolButton *btnToggleMarkerSources;
    QToolButton *btnToggleRays;
    QToolButton *btnToggleExpandedMarkers;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(1123, 773);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        MainWindow->setStyleSheet(QLatin1String("QWidget {\n"
"	background: rgb(28, 30, 32);\n"
"	color: white;\n"
"}\n"
"\n"
"QMenuBar {\n"
"	background-color: rgb(50, 54, 62);\n"
"}\n"
"\n"
"QMenuBar::item {\n"
"    spacing: 3px; /* spacing between menu bar items */\n"
"    padding: 5px;\n"
"    background: transparent;\n"
"    border-radius: 4px;\n"
"	color: rgb(230, 230, 255);\n"
"}\n"
"\n"
"QToolBar {\n"
"    background: rgb(32, 34, 39);\n"
"    spacing: 3px; /* spacing between items in the tool bar */\n"
"	border: 0;\n"
"	padding: 6px;\n"
"}\n"
"\n"
"QToolButton {\n"
"	border: 0px;\n"
"	background-color: rgb(50, 54, 62);\n"
"	color: white;\n"
"	padding: 0px;\n"
"}\n"
"\n"
"QToolButton:hover {\n"
"	border: 0px;\n"
"	background-color: rgb(60, 64, 72);\n"
"	color: white;\n"
"	padding: 0px;\n"
"}\n"
"\n"
"QToolButton:pressed {\n"
"	border: 1px;\n"
"	background-color: rgb(50, 70, 100);\n"
"	color: white;\n"
"	padding: 0px;\n"
"}\n"
"\n"
"QToolButton:checked {\n"
"	border: 1px;\n"
"	background-color: rgb(50, 70, 100);\n"
"	color: white;\n"
"	padding: 0px;\n"
""
                        "}\n"
"\n"
"QDockWidget {\n"
"	color: white;\n"
"	background-color: rgb(28, 30, 32);\n"
"}\n"
"\n"
"QDockWidget QWidget {\n"
"	background-color: rgb(43, 45, 51);\n"
"}\n"
"\n"
"QDockWidget QLineEdit {\n"
"	background: rgb(26, 29, 33);\n"
"	border: 0px;\n"
"	border-radius: 4px;\n"
"	padding: 3px;\n"
"	color: rgb(200, 200, 200);\n"
"}\n"
"\n"
"QDockWidget QTableWidget {\n"
"	background:  rgb(26, 29, 33);\n"
"}\n"
"\n"
"QDockWidget QTableWidget QWidget {\n"
"	background-color: rgb(26, 29, 33);\n"
"}\n"
"\n"
"QDockWidget QHeaderView {\n"
"	background-color: rgb(26, 29, 33);\n"
"	border-bottom: 1px solid rgb(50, 54, 62);\n"
"}\n"
"\n"
"QDockWidget QHeaderView::section {\n"
"	background-color: rgb(26, 29, 33);\n"
"	border: 0px;\n"
"	padding: 8px;\n"
"	border-bottom: 1px solid rgb(50, 54, 62);\n"
"}\n"
"\n"
"QDockWidget::title {\n"
"    text-align: left;\n"
"	color: white;\n"
"    background-color: rgb(50, 54, 62);\n"
"    padding: 6px;\n"
"}\n"
"\n"
"QDockWidget::close-button, QDockWidget::float-button {\n"
"    bord"
                        "er: 1px solid transparent;\n"
"	color: white;\n"
"    padding: 0px;\n"
"}\n"
"\n"
"QDockWidget::close-button:hover, QDockWidget::float-button:hover {\n"
"    background: gray;\n"
"}\n"
"\n"
"QDockWidget::close-button:pressed, QDockWidget::float-button:pressed {\n"
"    padding: 1px -1px -1px 1px;\n"
"}\n"
"\n"
"QWidget#centralWidget QFrame {\n"
"	background-color: rgb(43, 45, 51);\n"
"}\n"
"\n"
"QDockWidget QPushButton {\n"
"	border: 0px;\n"
"	background-color: rgb(50, 54, 62);\n"
"	color: white;\n"
"	padding: 5px;\n"
"}\n"
"\n"
"QDockWidget QTableWidget QPushButton {\n"
"	border: 0px;\n"
"	background-color: rgb(50, 54, 62);\n"
"	color: white;\n"
"	padding: 5px;\n"
"}\n"
"\n"
"QDockWidget QPushButton:hover {\n"
"	border: 0px;\n"
"	background-color: rgb(60, 64, 72);\n"
"	color: white;\n"
"	padding: 5px;\n"
"}\n"
"\n"
"QDockWidget QPushButton:pressed {\n"
"	border: 1px;\n"
"	background-color: rgb(50, 70, 100);\n"
"	color: white;\n"
"	padding: 5px;\n"
"}\n"
"\n"
"QSlider::groove:horizontal {\n"
"	border: 0px;\n"
""
                        "    height: 8px; /* the groove expands to the size of the slider by default. by giving it a height, it has a fixed size */\n"
"    background: rgb(26, 29, 33);\n"
"    margin: 2px 0;\n"
"}\n"
"\n"
"QSlider::handle:horizontal {\n"
"    background: rgb(200, 200, 200);\n"
"    border: 0px;\n"
"    width: 10px;\n"
"    margin: -8px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */\n"
"    border-radius: 4px;\n"
"}\n"
"\n"
"QTabBar\n"
"{\n"
"	background-color: rgb(50, 54, 62);\n"
"}\n"
"\n"
"QTabBar::tab {\n"
"	border: 0px;\n"
"	background-color: rgb(50, 54, 62);\n"
"	color: white;\n"
"	padding: 10px;\n"
"    min-width: 8ex;\n"
"}\n"
"\n"
"QTabBar::tab:selected \n"
"{\n"
"	color: rgb(80, 180, 255);\n"
"	border-top: 2px solid rgb(80, 180, 255);\n"
"}"));
        MainWindow->setToolButtonStyle(Qt::ToolButtonIconOnly);
        MainWindow->setDockNestingEnabled(true);
        MainWindow->setDockOptions(QMainWindow::AllowNestedDocks|QMainWindow::AllowTabbedDocks);
        actionNew = new QAction(MainWindow);
        actionNew->setObjectName(QStringLiteral("actionNew"));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        centralWidget->setEnabled(true);
        MainWindow->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);
        dockProps = new QDockWidget(MainWindow);
        dockProps->setObjectName(QStringLiteral("dockProps"));
        sizePolicy.setHeightForWidth(dockProps->sizePolicy().hasHeightForWidth());
        dockProps->setSizePolicy(sizePolicy);
        dockProps->setStyleSheet(QStringLiteral(""));
        dockProps->setFloating(false);
        dockWidgetContents_2 = new QWidget();
        dockWidgetContents_2->setObjectName(QStringLiteral("dockWidgetContents_2"));
        dockWidgetContents_2->setEnabled(true);
        dockWidgetContents_2->setStyleSheet(QStringLiteral(""));
        verticalLayout = new QVBoxLayout(dockWidgetContents_2);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        label_16 = new QLabel(dockWidgetContents_2);
        label_16->setObjectName(QStringLiteral("label_16"));
        label_16->setMinimumSize(QSize(0, 32));
        label_16->setMaximumSize(QSize(16777215, 32));
        label_16->setStyleSheet(QLatin1String("background-color: rgb(32, 34, 39);\n"
"padding: 5px;"));

        verticalLayout->addWidget(label_16);

        verticalLayout_11 = new QVBoxLayout();
        verticalLayout_11->setSpacing(6);
        verticalLayout_11->setObjectName(QStringLiteral("verticalLayout_11"));
        verticalLayout_11->setContentsMargins(6, 6, 6, -1);
        btnResetFrameIds = new QPushButton(dockWidgetContents_2);
        btnResetFrameIds->setObjectName(QStringLiteral("btnResetFrameIds"));

        verticalLayout_11->addWidget(btnResetFrameIds);

        btnStartRecording = new QPushButton(dockWidgetContents_2);
        btnStartRecording->setObjectName(QStringLiteral("btnStartRecording"));

        verticalLayout_11->addWidget(btnStartRecording);

        formLayout_3 = new QFormLayout();
        formLayout_3->setSpacing(6);
        formLayout_3->setObjectName(QStringLiteral("formLayout_3"));
        label_7 = new QLabel(dockWidgetContents_2);
        label_7->setObjectName(QStringLiteral("label_7"));

        formLayout_3->setWidget(0, QFormLayout::LabelRole, label_7);

        txtFps = new QLineEdit(dockWidgetContents_2);
        txtFps->setObjectName(QStringLiteral("txtFps"));
        txtFps->setStyleSheet(QStringLiteral(""));

        formLayout_3->setWidget(0, QFormLayout::FieldRole, txtFps);


        verticalLayout_11->addLayout(formLayout_3);


        verticalLayout->addLayout(verticalLayout_11);

        label_13 = new QLabel(dockWidgetContents_2);
        label_13->setObjectName(QStringLiteral("label_13"));
        label_13->setMinimumSize(QSize(0, 32));
        label_13->setMaximumSize(QSize(16777215, 32));
        label_13->setStyleSheet(QLatin1String("background-color: rgb(32, 34, 39);\n"
"padding: 5px;"));

        verticalLayout->addWidget(label_13);

        formLayout = new QFormLayout();
        formLayout->setSpacing(6);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setContentsMargins(6, 6, 6, -1);
        label_12 = new QLabel(dockWidgetContents_2);
        label_12->setObjectName(QStringLiteral("label_12"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_12);

        txtId = new QLineEdit(dockWidgetContents_2);
        txtId->setObjectName(QStringLiteral("txtId"));

        formLayout->setWidget(1, QFormLayout::FieldRole, txtId);

        label = new QLabel(dockWidgetContents_2);
        label->setObjectName(QStringLiteral("label"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label);

        txtExposure = new QLineEdit(dockWidgetContents_2);
        txtExposure->setObjectName(QStringLiteral("txtExposure"));

        formLayout->setWidget(3, QFormLayout::FieldRole, txtExposure);

        label_6 = new QLabel(dockWidgetContents_2);
        label_6->setObjectName(QStringLiteral("label_6"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_6);

        txtIso = new QLineEdit(dockWidgetContents_2);
        txtIso->setObjectName(QStringLiteral("txtIso"));

        formLayout->setWidget(4, QFormLayout::FieldRole, txtIso);

        label_3 = new QLabel(dockWidgetContents_2);
        label_3->setObjectName(QStringLiteral("label_3"));

        formLayout->setWidget(5, QFormLayout::LabelRole, label_3);

        horizontalSlider_2 = new QSlider(dockWidgetContents_2);
        horizontalSlider_2->setObjectName(QStringLiteral("horizontalSlider_2"));
        horizontalSlider_2->setOrientation(Qt::Horizontal);

        formLayout->setWidget(5, QFormLayout::FieldRole, horizontalSlider_2);

        label_4 = new QLabel(dockWidgetContents_2);
        label_4->setObjectName(QStringLiteral("label_4"));

        formLayout->setWidget(6, QFormLayout::LabelRole, label_4);

        horizontalSlider_3 = new QSlider(dockWidgetContents_2);
        horizontalSlider_3->setObjectName(QStringLiteral("horizontalSlider_3"));
        horizontalSlider_3->setOrientation(Qt::Horizontal);

        formLayout->setWidget(6, QFormLayout::FieldRole, horizontalSlider_3);

        label_10 = new QLabel(dockWidgetContents_2);
        label_10->setObjectName(QStringLiteral("label_10"));

        formLayout->setWidget(7, QFormLayout::LabelRole, label_10);

        txtFrameSkip = new QLineEdit(dockWidgetContents_2);
        txtFrameSkip->setObjectName(QStringLiteral("txtFrameSkip"));

        formLayout->setWidget(7, QFormLayout::FieldRole, txtFrameSkip);

        label_8 = new QLabel(dockWidgetContents_2);
        label_8->setObjectName(QStringLiteral("label_8"));

        formLayout->setWidget(8, QFormLayout::LabelRole, label_8);

        chkDrawGuides = new QCheckBox(dockWidgetContents_2);
        chkDrawGuides->setObjectName(QStringLiteral("chkDrawGuides"));

        formLayout->setWidget(8, QFormLayout::FieldRole, chkDrawGuides);

        label_9 = new QLabel(dockWidgetContents_2);
        label_9->setObjectName(QStringLiteral("label_9"));

        formLayout->setWidget(9, QFormLayout::LabelRole, label_9);

        chkMarkers = new QCheckBox(dockWidgetContents_2);
        chkMarkers->setObjectName(QStringLiteral("chkMarkers"));

        formLayout->setWidget(9, QFormLayout::FieldRole, chkMarkers);

        label_11 = new QLabel(dockWidgetContents_2);
        label_11->setObjectName(QStringLiteral("label_11"));

        formLayout->setWidget(10, QFormLayout::LabelRole, label_11);

        chkFindCalib = new QCheckBox(dockWidgetContents_2);
        chkFindCalib->setObjectName(QStringLiteral("chkFindCalib"));

        formLayout->setWidget(10, QFormLayout::FieldRole, chkFindCalib);


        verticalLayout->addLayout(formLayout);

        label_5 = new QLabel(dockWidgetContents_2);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setMinimumSize(QSize(0, 32));
        label_5->setMaximumSize(QSize(16777215, 32));
        label_5->setStyleSheet(QLatin1String("background-color: rgb(32, 34, 39);\n"
"padding: 5px;"));

        verticalLayout->addWidget(label_5);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setSpacing(6);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(6, -1, 6, 6);
        lblCalibImageCount = new QLabel(dockWidgetContents_2);
        lblCalibImageCount->setObjectName(QStringLiteral("lblCalibImageCount"));

        verticalLayout_4->addWidget(lblCalibImageCount);

        lblCalibError = new QLabel(dockWidgetContents_2);
        lblCalibError->setObjectName(QStringLiteral("lblCalibError"));

        verticalLayout_4->addWidget(lblCalibError);

        btnStartCalibration = new QPushButton(dockWidgetContents_2);
        btnStartCalibration->setObjectName(QStringLiteral("btnStartCalibration"));
        btnStartCalibration->setStyleSheet(QStringLiteral(""));

        verticalLayout_4->addWidget(btnStartCalibration);

        btnStopCalibration = new QPushButton(dockWidgetContents_2);
        btnStopCalibration->setObjectName(QStringLiteral("btnStopCalibration"));
        btnStopCalibration->setStyleSheet(QStringLiteral(""));

        verticalLayout_4->addWidget(btnStopCalibration);

        btnGenerateMask = new QPushButton(dockWidgetContents_2);
        btnGenerateMask->setObjectName(QStringLiteral("btnGenerateMask"));

        verticalLayout_4->addWidget(btnGenerateMask);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_3);


        verticalLayout->addLayout(verticalLayout_4);

        dockProps->setWidget(dockWidgetContents_2);
        MainWindow->addDockWidget(static_cast<Qt::DockWidgetArea>(4), dockProps);
        dockTimeline = new QDockWidget(MainWindow);
        dockTimeline->setObjectName(QStringLiteral("dockTimeline"));
        dockWidgetContents_3 = new QWidget();
        dockWidgetContents_3->setObjectName(QStringLiteral("dockWidgetContents_3"));
        horizontalLayout_4 = new QHBoxLayout(dockWidgetContents_3);
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        verticalLayout_14 = new QVBoxLayout();
        verticalLayout_14->setSpacing(6);
        verticalLayout_14->setObjectName(QStringLiteral("verticalLayout_14"));
        verticalLayout_14->setContentsMargins(-1, 9, -1, -1);
        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_14->addItem(verticalSpacer_2);

        lblTimecode = new QLabel(dockWidgetContents_3);
        lblTimecode->setObjectName(QStringLiteral("lblTimecode"));
        QFont font;
        font.setPointSize(12);
        lblTimecode->setFont(font);
        lblTimecode->setAlignment(Qt::AlignCenter);

        verticalLayout_14->addWidget(lblTimecode);

        timelineLayout = new QHBoxLayout();
        timelineLayout->setSpacing(6);
        timelineLayout->setObjectName(QStringLiteral("timelineLayout"));
        timelineLayout->setContentsMargins(9, 0, 9, 9);
        txtPlaySpeed = new QLineEdit(dockWidgetContents_3);
        txtPlaySpeed->setObjectName(QStringLiteral("txtPlaySpeed"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(txtPlaySpeed->sizePolicy().hasHeightForWidth());
        txtPlaySpeed->setSizePolicy(sizePolicy1);
        txtPlaySpeed->setMinimumSize(QSize(30, 0));
        txtPlaySpeed->setMaximumSize(QSize(30, 16777215));

        timelineLayout->addWidget(txtPlaySpeed);

        btnPrevFrameJump = new QPushButton(dockWidgetContents_3);
        btnPrevFrameJump->setObjectName(QStringLiteral("btnPrevFrameJump"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(btnPrevFrameJump->sizePolicy().hasHeightForWidth());
        btnPrevFrameJump->setSizePolicy(sizePolicy2);
        QIcon icon;
        icon.addFile(QStringLiteral("icons8-rewind.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnPrevFrameJump->setIcon(icon);

        timelineLayout->addWidget(btnPrevFrameJump);

        btnPrevFrame = new QPushButton(dockWidgetContents_3);
        btnPrevFrame->setObjectName(QStringLiteral("btnPrevFrame"));
        QIcon icon1;
        icon1.addFile(QStringLiteral("icons8-less-than.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnPrevFrame->setIcon(icon1);

        timelineLayout->addWidget(btnPrevFrame);

        btnPlay = new QPushButton(dockWidgetContents_3);
        btnPlay->setObjectName(QStringLiteral("btnPlay"));
        QIcon icon2;
        icon2.addFile(QStringLiteral("icons8-play.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnPlay->setIcon(icon2);

        timelineLayout->addWidget(btnPlay);

        btnNextFrame = new QPushButton(dockWidgetContents_3);
        btnNextFrame->setObjectName(QStringLiteral("btnNextFrame"));
        QIcon icon3;
        icon3.addFile(QStringLiteral("icons8-more-than.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnNextFrame->setIcon(icon3);

        timelineLayout->addWidget(btnNextFrame);

        btnNextFrameJump = new QPushButton(dockWidgetContents_3);
        btnNextFrameJump->setObjectName(QStringLiteral("btnNextFrameJump"));
        QIcon icon4;
        icon4.addFile(QStringLiteral("icons8-fast-forward.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnNextFrameJump->setIcon(icon4);

        timelineLayout->addWidget(btnNextFrameJump);


        verticalLayout_14->addLayout(timelineLayout);


        horizontalLayout_4->addLayout(verticalLayout_14);

        dockTimeline->setWidget(dockWidgetContents_3);
        MainWindow->addDockWidget(static_cast<Qt::DockWidgetArea>(8), dockTimeline);
        dockTrackers = new QDockWidget(MainWindow);
        dockTrackers->setObjectName(QStringLiteral("dockTrackers"));
        dockTrackers->setFloating(false);
        dockTrackers->setFeatures(QDockWidget::AllDockWidgetFeatures);
        dockWidgetContents_4 = new QWidget();
        dockWidgetContents_4->setObjectName(QStringLiteral("dockWidgetContents_4"));
        verticalLayout_3 = new QVBoxLayout(dockWidgetContents_4);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setContentsMargins(11, 11, 11, 11);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        trackerViewMain = new QWidget(dockWidgetContents_4);
        trackerViewMain->setObjectName(QStringLiteral("trackerViewMain"));
        QSizePolicy sizePolicy3(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(trackerViewMain->sizePolicy().hasHeightForWidth());
        trackerViewMain->setSizePolicy(sizePolicy3);
        verticalLayout_2 = new QVBoxLayout(trackerViewMain);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        toolbar = new QHBoxLayout();
        toolbar->setSpacing(0);
        toolbar->setObjectName(QStringLiteral("toolbar"));
        btnToggleUpdate = new QToolButton(trackerViewMain);
        btnToggleUpdate->setObjectName(QStringLiteral("btnToggleUpdate"));
        QIcon icon5;
        icon5.addFile(QStringLiteral("icons8-exercise.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnToggleUpdate->setIcon(icon5);
        btnToggleUpdate->setIconSize(QSize(30, 30));
        btnToggleUpdate->setCheckable(true);
        btnToggleUpdate->setChecked(true);

        toolbar->addWidget(btnToggleUpdate);

        btnToggleMask = new QToolButton(trackerViewMain);
        btnToggleMask->setObjectName(QStringLiteral("btnToggleMask"));
        QIcon icon6;
        icon6.addFile(QStringLiteral("icons8-iron-man.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnToggleMask->setIcon(icon6);
        btnToggleMask->setIconSize(QSize(30, 30));
        btnToggleMask->setCheckable(true);

        toolbar->addWidget(btnToggleMask);

        line = new QFrame(trackerViewMain);
        line->setObjectName(QStringLiteral("line"));
        line->setMinimumSize(QSize(0, 0));
        line->setMaximumSize(QSize(2, 20));
        line->setStyleSheet(QLatin1String("QFrame {\n"
"	border: none;\n"
"	background: rgb(80, 80, 80);\n"
"}"));
        line->setFrameShadow(QFrame::Plain);
        line->setLineWidth(1);
        line->setFrameShape(QFrame::VLine);

        toolbar->addWidget(line);

        btnToggleDistortedMarkers = new QToolButton(trackerViewMain);
        btnToggleDistortedMarkers->setObjectName(QStringLiteral("btnToggleDistortedMarkers"));
        QIcon icon7;
        icon7.addFile(QStringLiteral("icons8-full-moon.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnToggleDistortedMarkers->setIcon(icon7);
        btnToggleDistortedMarkers->setIconSize(QSize(30, 30));
        btnToggleDistortedMarkers->setCheckable(true);

        toolbar->addWidget(btnToggleDistortedMarkers);

        btnToggleUndistortedMarkers = new QToolButton(trackerViewMain);
        btnToggleUndistortedMarkers->setObjectName(QStringLiteral("btnToggleUndistortedMarkers"));
        QIcon icon8;
        icon8.addFile(QStringLiteral("icons8-first-quarter.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnToggleUndistortedMarkers->setIcon(icon8);
        btnToggleUndistortedMarkers->setIconSize(QSize(30, 30));
        btnToggleUndistortedMarkers->setCheckable(true);
        btnToggleUndistortedMarkers->setChecked(true);

        toolbar->addWidget(btnToggleUndistortedMarkers);

        btnToggleReprojectedMarkers = new QToolButton(trackerViewMain);
        btnToggleReprojectedMarkers->setObjectName(QStringLiteral("btnToggleReprojectedMarkers"));
        QIcon icon9;
        icon9.addFile(QStringLiteral("icons8-new-moon.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnToggleReprojectedMarkers->setIcon(icon9);
        btnToggleReprojectedMarkers->setIconSize(QSize(30, 30));
        btnToggleReprojectedMarkers->setCheckable(true);
        btnToggleReprojectedMarkers->setChecked(true);

        toolbar->addWidget(btnToggleReprojectedMarkers);

        line_2 = new QFrame(trackerViewMain);
        line_2->setObjectName(QStringLiteral("line_2"));
        line_2->setMinimumSize(QSize(0, 0));
        line_2->setMaximumSize(QSize(2, 20));
        line_2->setStyleSheet(QLatin1String("QFrame {\n"
"	border: none;\n"
"	background: rgb(80, 80, 80);\n"
"}"));
        line_2->setFrameShadow(QFrame::Plain);
        line_2->setLineWidth(1);
        line_2->setFrameShape(QFrame::VLine);

        toolbar->addWidget(line_2);

        btnTogglePixelGrid = new QToolButton(trackerViewMain);
        btnTogglePixelGrid->setObjectName(QStringLiteral("btnTogglePixelGrid"));
        QIcon icon10;
        icon10.addFile(QStringLiteral("icons8-grid.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnTogglePixelGrid->setIcon(icon10);
        btnTogglePixelGrid->setIconSize(QSize(30, 30));
        btnTogglePixelGrid->setCheckable(true);

        toolbar->addWidget(btnTogglePixelGrid);

        btnToggleRays_2 = new QToolButton(trackerViewMain);
        btnToggleRays_2->setObjectName(QStringLiteral("btnToggleRays_2"));
        QIcon icon11;
        icon11.addFile(QStringLiteral("icons8-day-camera.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnToggleRays_2->setIcon(icon11);
        btnToggleRays_2->setIconSize(QSize(30, 30));
        btnToggleRays_2->setCheckable(true);

        toolbar->addWidget(btnToggleRays_2);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        toolbar->addItem(horizontalSpacer);


        verticalLayout_2->addLayout(toolbar);

        widget = new QWidget(trackerViewMain);
        widget->setObjectName(QStringLiteral("widget"));
        QSizePolicy sizePolicy4(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
        widget->setSizePolicy(sizePolicy4);
        verticalLayout_13 = new QVBoxLayout(widget);
        verticalLayout_13->setSpacing(0);
        verticalLayout_13->setContentsMargins(11, 11, 11, 11);
        verticalLayout_13->setObjectName(QStringLiteral("verticalLayout_13"));
        verticalLayout_13->setContentsMargins(0, 0, 0, 0);
        cameraViewPanel = new QVBoxLayout();
        cameraViewPanel->setSpacing(6);
        cameraViewPanel->setObjectName(QStringLiteral("cameraViewPanel"));

        verticalLayout_13->addLayout(cameraViewPanel);


        verticalLayout_2->addWidget(widget);


        verticalLayout_3->addWidget(trackerViewMain);

        dockTrackers->setWidget(dockWidgetContents_4);
        MainWindow->addDockWidget(static_cast<Qt::DockWidgetArea>(4), dockTrackers);
        dockSynth = new QDockWidget(MainWindow);
        dockSynth->setObjectName(QStringLiteral("dockSynth"));
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QStringLiteral("dockWidgetContents"));
        verticalLayout_5 = new QVBoxLayout(dockWidgetContents);
        verticalLayout_5->setSpacing(6);
        verticalLayout_5->setContentsMargins(11, 11, 11, 11);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        verticalLayout_5->setContentsMargins(0, 0, 0, 0);
        label_18 = new QLabel(dockWidgetContents);
        label_18->setObjectName(QStringLiteral("label_18"));
        label_18->setMinimumSize(QSize(0, 32));
        label_18->setMaximumSize(QSize(16777215, 32));
        label_18->setStyleSheet(QLatin1String("background-color: rgb(32, 34, 39);\n"
"padding: 5px;"));

        verticalLayout_5->addWidget(label_18);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(6, 6, 6, 6);
        btnLoadTake = new QPushButton(dockWidgetContents);
        btnLoadTake->setObjectName(QStringLiteral("btnLoadTake"));

        horizontalLayout_3->addWidget(btnLoadTake);

        btnSaveTake = new QPushButton(dockWidgetContents);
        btnSaveTake->setObjectName(QStringLiteral("btnSaveTake"));

        horizontalLayout_3->addWidget(btnSaveTake);


        verticalLayout_5->addLayout(horizontalLayout_3);

        label_14 = new QLabel(dockWidgetContents);
        label_14->setObjectName(QStringLiteral("label_14"));
        label_14->setMinimumSize(QSize(0, 32));
        label_14->setMaximumSize(QSize(16777215, 32));
        label_14->setStyleSheet(QLatin1String("background-color: rgb(32, 34, 39);\n"
"padding: 5px;"));

        verticalLayout_5->addWidget(label_14);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setSpacing(6);
        verticalLayout_7->setObjectName(QStringLiteral("verticalLayout_7"));
        verticalLayout_7->setContentsMargins(6, 6, 6, 6);
        btnBuildFundamental = new QPushButton(dockWidgetContents);
        btnBuildFundamental->setObjectName(QStringLiteral("btnBuildFundamental"));

        verticalLayout_7->addWidget(btnBuildFundamental);

        btnBundleAdjust = new QPushButton(dockWidgetContents);
        btnBundleAdjust->setObjectName(QStringLiteral("btnBundleAdjust"));

        verticalLayout_7->addWidget(btnBundleAdjust);

        btnAssignWorldBasis = new QPushButton(dockWidgetContents);
        btnAssignWorldBasis->setObjectName(QStringLiteral("btnAssignWorldBasis"));

        verticalLayout_7->addWidget(btnAssignWorldBasis);


        verticalLayout_5->addLayout(verticalLayout_7);

        label_15 = new QLabel(dockWidgetContents);
        label_15->setObjectName(QStringLiteral("label_15"));
        label_15->setMinimumSize(QSize(0, 32));
        label_15->setMaximumSize(QSize(16777215, 32));
        label_15->setStyleSheet(QLatin1String("background-color: rgb(32, 34, 39);\n"
"padding: 5px;"));

        verticalLayout_5->addWidget(label_15);

        verticalLayout_12 = new QVBoxLayout();
        verticalLayout_12->setSpacing(6);
        verticalLayout_12->setObjectName(QStringLiteral("verticalLayout_12"));
        verticalLayout_12->setContentsMargins(6, 6, 6, 6);
        btnBuild3D = new QPushButton(dockWidgetContents);
        btnBuild3D->setObjectName(QStringLiteral("btnBuild3D"));

        verticalLayout_12->addWidget(btnBuild3D);

        btnIdentify3D = new QPushButton(dockWidgetContents);
        btnIdentify3D->setObjectName(QStringLiteral("btnIdentify3D"));

        verticalLayout_12->addWidget(btnIdentify3D);


        verticalLayout_5->addLayout(verticalLayout_12);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer);

        dockSynth->setWidget(dockWidgetContents);
        MainWindow->addDockWidget(static_cast<Qt::DockWidgetArea>(4), dockSynth);
        dockSceneView = new QDockWidget(MainWindow);
        dockSceneView->setObjectName(QStringLiteral("dockSceneView"));
        sceneViewDockContents = new QWidget();
        sceneViewDockContents->setObjectName(QStringLiteral("sceneViewDockContents"));
        verticalLayout_6 = new QVBoxLayout(sceneViewDockContents);
        verticalLayout_6->setSpacing(0);
        verticalLayout_6->setContentsMargins(11, 11, 11, 11);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        verticalLayout_6->setContentsMargins(0, 0, 0, 0);
        toolbar_2 = new QHBoxLayout();
        toolbar_2->setSpacing(0);
        toolbar_2->setObjectName(QStringLiteral("toolbar_2"));
        btnToggleMarkerSources = new QToolButton(sceneViewDockContents);
        btnToggleMarkerSources->setObjectName(QStringLiteral("btnToggleMarkerSources"));
        QIcon icon12;
        icon12.addFile(QStringLiteral("icons8-sun.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnToggleMarkerSources->setIcon(icon12);
        btnToggleMarkerSources->setIconSize(QSize(30, 30));
        btnToggleMarkerSources->setCheckable(true);
        btnToggleMarkerSources->setChecked(false);

        toolbar_2->addWidget(btnToggleMarkerSources);

        btnToggleRays = new QToolButton(sceneViewDockContents);
        btnToggleRays->setObjectName(QStringLiteral("btnToggleRays"));
        btnToggleRays->setIcon(icon11);
        btnToggleRays->setIconSize(QSize(30, 30));
        btnToggleRays->setCheckable(true);

        toolbar_2->addWidget(btnToggleRays);

        btnToggleExpandedMarkers = new QToolButton(sceneViewDockContents);
        btnToggleExpandedMarkers->setObjectName(QStringLiteral("btnToggleExpandedMarkers"));
        QIcon icon13;
        icon13.addFile(QStringLiteral("icons8-expand.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnToggleExpandedMarkers->setIcon(icon13);
        btnToggleExpandedMarkers->setIconSize(QSize(30, 30));
        btnToggleExpandedMarkers->setCheckable(true);

        toolbar_2->addWidget(btnToggleExpandedMarkers);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        toolbar_2->addItem(horizontalSpacer_2);


        verticalLayout_6->addLayout(toolbar_2);

        dockSceneView->setWidget(sceneViewDockContents);
        MainWindow->addDockWidget(static_cast<Qt::DockWidgetArea>(4), dockSceneView);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "Vector Synth", Q_NULLPTR));
        actionNew->setText(QApplication::translate("MainWindow", "New", Q_NULLPTR));
        dockProps->setWindowTitle(QApplication::translate("MainWindow", "Live", Q_NULLPTR));
        label_16->setText(QApplication::translate("MainWindow", "Take", Q_NULLPTR));
        btnResetFrameIds->setText(QApplication::translate("MainWindow", "Reset Frame IDs", Q_NULLPTR));
        btnStartRecording->setText(QApplication::translate("MainWindow", "Start Recording", Q_NULLPTR));
        label_7->setText(QApplication::translate("MainWindow", "FPS", Q_NULLPTR));
        txtFps->setText(QApplication::translate("MainWindow", "40", Q_NULLPTR));
        label_13->setText(QApplication::translate("MainWindow", "Tracker Properties", Q_NULLPTR));
        label_12->setText(QApplication::translate("MainWindow", "Name", Q_NULLPTR));
        txtId->setText(QApplication::translate("MainWindow", "Tracker Id", Q_NULLPTR));
        label->setText(QApplication::translate("MainWindow", "Exposure", Q_NULLPTR));
        txtExposure->setText(QApplication::translate("MainWindow", "7000", Q_NULLPTR));
        label_6->setText(QApplication::translate("MainWindow", "ISO", Q_NULLPTR));
        txtIso->setText(QApplication::translate("MainWindow", "100", Q_NULLPTR));
        label_3->setText(QApplication::translate("MainWindow", "Threshold", Q_NULLPTR));
        label_4->setText(QApplication::translate("MainWindow", "Sensitivity", Q_NULLPTR));
        label_10->setText(QApplication::translate("MainWindow", "Frame Skip", Q_NULLPTR));
        txtFrameSkip->setText(QApplication::translate("MainWindow", "0", Q_NULLPTR));
        label_8->setText(QApplication::translate("MainWindow", "Draw Guides", Q_NULLPTR));
        chkDrawGuides->setText(QString());
        label_9->setText(QApplication::translate("MainWindow", "Markers", Q_NULLPTR));
        chkMarkers->setText(QString());
        label_11->setText(QApplication::translate("MainWindow", "Undistort", Q_NULLPTR));
        chkFindCalib->setText(QString());
        label_5->setText(QApplication::translate("MainWindow", "Calibration", Q_NULLPTR));
        lblCalibImageCount->setText(QApplication::translate("MainWindow", "Image Count: 0", Q_NULLPTR));
        lblCalibError->setText(QApplication::translate("MainWindow", "Error: 0", Q_NULLPTR));
        btnStartCalibration->setText(QApplication::translate("MainWindow", "Start Calibration", Q_NULLPTR));
        btnStopCalibration->setText(QApplication::translate("MainWindow", "Stop Calibration", Q_NULLPTR));
        btnGenerateMask->setText(QApplication::translate("MainWindow", "Generate Mask", Q_NULLPTR));
        dockTimeline->setWindowTitle(QApplication::translate("MainWindow", "Take Timeline", Q_NULLPTR));
        lblTimecode->setText(QApplication::translate("MainWindow", "00:00:000", Q_NULLPTR));
        txtPlaySpeed->setText(QApplication::translate("MainWindow", "1.0", Q_NULLPTR));
        btnPrevFrameJump->setText(QString());
        btnPrevFrame->setText(QString());
        btnPlay->setText(QString());
        btnNextFrame->setText(QString());
        btnNextFrameJump->setText(QString());
        dockTrackers->setWindowTitle(QApplication::translate("MainWindow", "Tracker View", Q_NULLPTR));
        btnToggleUpdate->setText(QApplication::translate("MainWindow", "F", Q_NULLPTR));
        btnToggleMask->setText(QApplication::translate("MainWindow", "M", Q_NULLPTR));
        btnToggleDistortedMarkers->setText(QString());
        btnToggleUndistortedMarkers->setText(QString());
        btnToggleReprojectedMarkers->setText(QString());
        btnTogglePixelGrid->setText(QString());
        btnToggleRays_2->setText(QApplication::translate("MainWindow", "M", Q_NULLPTR));
        dockSynth->setWindowTitle(QApplication::translate("MainWindow", "Takes", Q_NULLPTR));
        label_18->setText(QApplication::translate("MainWindow", "Take", Q_NULLPTR));
        btnLoadTake->setText(QApplication::translate("MainWindow", "Load", Q_NULLPTR));
        btnSaveTake->setText(QApplication::translate("MainWindow", "Save", Q_NULLPTR));
        label_14->setText(QApplication::translate("MainWindow", "Calibration", Q_NULLPTR));
        btnBuildFundamental->setText(QApplication::translate("MainWindow", "Find Extrinsic Parameters", Q_NULLPTR));
        btnBundleAdjust->setText(QApplication::translate("MainWindow", "Bundle Adjust", Q_NULLPTR));
        btnAssignWorldBasis->setText(QApplication::translate("MainWindow", "Assign World Basis", Q_NULLPTR));
        label_15->setText(QApplication::translate("MainWindow", "Tools", Q_NULLPTR));
        btnBuild3D->setText(QApplication::translate("MainWindow", "Build 3D Markers", Q_NULLPTR));
        btnIdentify3D->setText(QApplication::translate("MainWindow", "Label Markers", Q_NULLPTR));
        dockSceneView->setWindowTitle(QApplication::translate("MainWindow", "Scene View", Q_NULLPTR));
        btnToggleMarkerSources->setText(QApplication::translate("MainWindow", "F", Q_NULLPTR));
        btnToggleRays->setText(QApplication::translate("MainWindow", "M", Q_NULLPTR));
        btnToggleExpandedMarkers->setText(QApplication::translate("MainWindow", "M", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
