#include <QtWidgets>
#include <QFile>
#include "brukerScan.h"

void MainWindow::createScanPage()
{
    _scanPage = new QWidget();

    auto *queryLayout = new QGridLayout();
    auto *subjectIDLabel = new QLabel("Subject ID",_scanPage);
    _subjectID = new QLineEdit("?");
    _subjectID->setFocusPolicy(Qt::ClickFocus);
    queryLayout->addWidget(subjectIDLabel,0,0);
    queryLayout->addWidget(_subjectID,0,1);

    auto *subjectGroupBox = new QGroupBox("Subject information");
    subjectGroupBox->setLayout(queryLayout);

    _scanItemsBox = new QListWidget();
    _scanItemsBox->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding);
    _scanItemsBox->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    _scanItemsBox->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(_scanItemsBox, SIGNAL(itemClicked(QListWidgetItem*)),this, SLOT(changedSelectScanCheckBox(QListWidgetItem*)));

    auto *runsLayout = new QVBoxLayout();
    runsLayout->addWidget(_scanItemsBox);
    auto *runsBox = new QGroupBox("Download data from server");
    runsBox->setLayout(runsLayout);

    auto *viewLayout = new QVBoxLayout();
    auto *viewPushButton = new QPushButton("view");
    connect(viewPushButton, SIGNAL(pressed()), this, SLOT(viewScanUsingFastMap()));

    viewLayout->addWidget(viewPushButton);
    auto *viewBox = new QGroupBox("View selected scan");
    viewBox->setLayout(viewLayout);

    auto *pageLayout = new QVBoxLayout();
    pageLayout->addWidget(subjectGroupBox);
    pageLayout->addWidget(runsBox);
    pageLayout->addWidget(viewBox);
    _scanPage->setLayout(pageLayout);

    QString downFile = "download-list.dat";
    QFileInfo checkDownFile(downFile);
}

void MainWindow::changedSelectScanCheckBox(QListWidgetItem *item)
{
    FUNC_ENTER;
    int iSelected=-1;
    for ( int jItem=0; jItem<_scanItems.size(); jItem++)
    {
        if ( item == &_scanItems.at(jItem) )
            iSelected = jItem;
    }
    _scans[iSelected].selectedAsImportant = _scanItems[iSelected].checkState();
}

void MainWindow::viewScanUsingFastMap()
{
    FUNC_ENTER;
    int iSelect = -1;
    for (int jList=0; jList<_scans.size(); jList++)
    {
        if ( _scanItems[jList].isSelected() )
            iSelect = jList;
    }
    FUNC_INFO << "iSelect" << iSelect;
    if ( iSelect > 0 )
        displayFM(iSelect);
}

void MainWindow::displayFM(int iScan)
{
    FUNC_ENTER << iScan;
    scanType thisScan = _scans.at(iScan);
    QString exe;
    if ( thisScan.reorderEchoes )
        exe = _scriptDirectory + "reorder.script";
    else
        exe = _scriptDirectory + "view.script";

    QString fileName = thisScan.scanNumber;
    QStringList arguments;
    arguments.append(fileName);

    auto *process = new QProcess();
    FUNC_EXIT << "exe arguments" << exe << arguments;
    process->startDetached(exe,arguments);
}
