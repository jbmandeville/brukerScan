#include <QtWidgets>
#include <QFileInfo>
#include "brukerScan.h"

void MainWindow::createCleanPage()
{
    FUNC_ENTER;
    _cleanPage = new QWidget();

    _cleanScanTypesBox = new QListWidget();
    _cleanScanTypesBox->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding);
    _cleanScanTypesBox->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    connect(_cleanScanTypesBox, SIGNAL(itemChanged(QListWidgetItem*)),this, SLOT(changedScanTypeCheckBox(QListWidgetItem*)));

    auto *totalSizeLabel = new QLabel("total size of all sub-directories");
    _totalSizeSubDirs = new QLabel();
    auto *sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(totalSizeLabel);
    sizeLayout->addWidget(_totalSizeSubDirs);

    auto *typeLayout = new QVBoxLayout();
    typeLayout->addWidget(_cleanScanTypesBox);
    typeLayout->addLayout(sizeLayout);
    auto *typeBox = new QGroupBox("Directories by scan type");
    typeBox->setLayout(typeLayout);

    _cleanDICOMs         = new QPushButton("clean DICOMS",_cleanPage);
    _cleanNII_auxilliary = new QPushButton("clean intermediate NIFTIs",_cleanPage);
    _cleanNII_raw        = new QPushButton("clean raw.nii",_cleanPage);
    connect(_cleanDICOMs,         SIGNAL(pressed()), this, SLOT(cleanDICOMFiles()));
    connect(_cleanNII_auxilliary, SIGNAL(pressed()), this, SLOT(cleanAuxNIIFiles()));
    connect(_cleanNII_raw,        SIGNAL(pressed()), this, SLOT(cleanRawNIIFiles()));
    _cleanDICOMs->setToolTip("No reason to keep DICOMs after extracting time tags.");
    _cleanNII_auxilliary->setToolTip("These intermediate files can be recovered if you have the raw files.");
    _cleanNII_raw->setToolTip("These can be recovered by down-loading again, but it takes time.");

    auto *actionLayout = new QVBoxLayout();
    actionLayout->addWidget(_cleanDICOMs);
    actionLayout->addWidget(_cleanNII_auxilliary);
    actionLayout->addWidget(_cleanNII_raw);
    auto *actionBox = new QGroupBox("Clean directories (remove files)");
    actionBox->setLayout(actionLayout);

    auto *pageLayout = new QVBoxLayout();
    pageLayout->addWidget(typeBox);
    pageLayout->addWidget(actionBox);

    _cleanPage->setLayout(pageLayout);
}

void MainWindow::changedScanTypeCheckBox(QListWidgetItem *item)
{
    int iSelected=-1;
    for ( int jItem=0; jItem<_cleanScanTypeItems.size(); jItem++)
    {
        if ( item == &_cleanScanTypeItems.at(jItem) )
            iSelected = jItem;
    }
    FUNC_INFO << iSelected;
    updateCleaningList();
}

void MainWindow::setupScanTypes()
{
    FUNC_ENTER;
    for (int jType=0; jType<_scans.size(); jType++)
    {
        scanType scan = _scans.at(jType);
        bool found = false;
        if ( !found )
        {
            QListWidgetItem item;
            item.setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
//            item.setCheckState(Qt::Checked);
            item.setHidden(false);
            _cleanScanTypeItems.append(item);
        }
    }
    for (int jItem=0; jItem<_cleanScanTypeItems.count(); jItem++)
        _cleanScanTypesBox->addItem(&_cleanScanTypeItems[jItem]);
    FUNC_INFO << "new sizes" << _cleanScanTypeItems.count() << _cleanScanTypesBox->count();

    FUNC_EXIT;
}

void MainWindow::updateCleaningList()
{
    for (int jType=0; jType<_scans.size(); jType++)
    {
        scanType scan = _scans.at(jType);
        _scans[jType].selectedForCleaning = false;
    }
    findDICOMs(false);
    findAuxFiles(false);
    findRawFiles(false);
    findAllFiles();
}

void MainWindow::openedCleanPage()
{
    FUNC_ENTER;
    setupScanTypes();
    updateCleaningList();

    FUNC_EXIT;
}

void MainWindow::findDICOMs(bool remove)
{
}

void MainWindow::findAuxFiles(bool remove)
{
}

void MainWindow::findRawFiles(bool remove)
{
}

void MainWindow::findAllFiles()
{
    FUNC_ENTER;
    QDirIterator it(".", QDirIterator::Subdirectories);
    qint64 total = 0;
    while (it.hasNext())
    {
        FUNC_INFO << "directory" << it.fileName();
        total += it.fileInfo().size();
        it.next();
    }
    QString gigabytes; gigabytes.setNum(static_cast<double>(total)/(1024.*1024.*1024.),'g',3);
    total /= 1024*1024;
    _totalSizeSubDirs->setText(gigabytes + " Gb");
    FUNC_INFO << "gb" << gigabytes;
    FUNC_EXIT << total << "Mb";
}
