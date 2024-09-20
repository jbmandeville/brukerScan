#include <QtWidgets>
#include <QFileInfo>
#include "brukerScan.h"

QWidget *MainWindow::createCleanPanel()
{
    FUNC_ENTER;
    auto *cleanPanel = new QWidget();

    auto *totalSizeLabel = new QLabel("total size of all sub-directories");
    _totalSizeSubDirs = new QLabel();
    auto *sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(totalSizeLabel);
    sizeLayout->addWidget(_totalSizeSubDirs);

    _cleanUnselectedFIDs   = new QPushButton("clean fid",cleanPanel);
    _cleanUnselectedDICOMs = new QPushButton("clean DICOM",cleanPanel);
    _cleanAllFIDs          = new QPushButton("clean fid",cleanPanel);
    _cleanAllDICOMs        = new QPushButton("clean DICOM",cleanPanel);

    connect(_cleanUnselectedFIDs,   SIGNAL(pressed()), this, SLOT(cleanUnselelectedFIDs()));
    connect(_cleanUnselectedDICOMs, SIGNAL(pressed()), this, SLOT(cleanUnselelectedDICOMs()));
    connect(_cleanAllFIDs,   SIGNAL(pressed()), this, SLOT(cleanAllFIDs()));
    connect(_cleanAllDICOMs, SIGNAL(pressed()), this, SLOT(cleanAllDICOMs()));

    auto *unselectedLayout = new QVBoxLayout();
    unselectedLayout->addWidget(_cleanUnselectedFIDs);
    unselectedLayout->addWidget(_cleanUnselectedDICOMs);
    auto *unselectedBox = new QGroupBox("Clean unselected directories");
    unselectedBox->setLayout(unselectedLayout);

    auto *allLayout = new QVBoxLayout();
    allLayout->addWidget(_cleanAllFIDs);
    allLayout->addWidget(_cleanAllDICOMs);
    auto *allBox = new QGroupBox("Clean all directories");
    allBox->setLayout(allLayout);

    auto *actionLayout = new QHBoxLayout();
    actionLayout->addWidget(unselectedBox);
    actionLayout->addWidget(allBox);

    auto *pageLayout = new QVBoxLayout();
    pageLayout->addLayout(sizeLayout);
    pageLayout->addLayout(actionLayout);

    pageLayout->setSpacing(0);
    cleanPanel->setLayout(pageLayout);
    return cleanPanel;
}

void MainWindow::updateCleaningList()
{
    findDICOMs(true, false);
    findDICOMs(false, false);
    findFIDs(true, false);
    findFIDs(false, false);
    findAllFiles();
    _cleanUnselectedFIDs->setEnabled(_inputOptions.enableCleanup);
    _cleanUnselectedDICOMs->setEnabled(_inputOptions.enableCleanup);
    _cleanAllFIDs->setEnabled(_inputOptions.enableCleanup);
    _cleanAllDICOMs->setEnabled(_inputOptions.enableCleanup);
}

void MainWindow::findDICOMs(bool all, bool remove)
{
    qint64 totalSizeAll=0;

    for (int jScan=0; jScan<_scans.size(); jScan++)
    {
        scanType scan = _scans.at(jScan);
        bool includeScan = all || scan.selectedAsImportant;
        qint64 totalSizeThisScan=0;
        if ( includeScan )
        {
            QString spec;
            spec = "MR*";
            QString dirName = scan.scanName + "/pdata/1/dicom";
            QDir dir(dirName);
            QStringList const fileList = dir.entryList( {spec}, QDir::Files | QDir::NoSymLinks);

            qint64 totalSizeThisDir=0;
            for (int jFile=0; jFile<fileList.size(); jFile++)
            {
                QString fileName = dirName + "/" + fileList.at(jFile);
                QFileInfo checkFile(fileName);
                if ( remove )
                {
                    QFile file(fileName);
                    bool success = file.remove();
                    if ( !success )
                        qInfo() << "failed to remove file" << fileName;
                    else
                        totalSizeThisDir += checkFile.size();
                }
                else
                    totalSizeThisDir += checkFile.size();
            }
            totalSizeThisScan += totalSizeThisDir;
            FUNC_INFO << "total size of DICOMS =" << totalSizeThisScan/(1024*1024) << "Mb";
        }
        totalSizeAll += totalSizeThisScan;
    }
    double Mb = static_cast<double>(totalSizeAll)/(1024.*1024.);
    double Gb = static_cast<double>(totalSizeAll)/(1024.*1024.*1024.);
    QString MbString; MbString.setNum(Mb,'g',3);
    QString GbString; GbString.setNum(Gb,'g',3);
    totalSizeAll /= 1024*1024;  // b -> Gb
    if ( remove )
        qInfo() << "Removed" << GbString << "Gb (=" << MbString << "Mb)";
    else
    {
        if ( all )
        {
            _cleanAllDICOMs->setEnabled(totalSizeAll > 0);
            if ( Gb > 1. )
                _cleanAllDICOMs->setText(QString("clean DICOMS (%1 Gb)").arg(GbString));
            else
                _cleanAllDICOMs->setText(QString("clean DICOMS (%1 Mb").arg(MbString));
        }
        else
        {
            _cleanUnselectedDICOMs->setEnabled(totalSizeAll > 0);
            if ( Gb > 1. )
                _cleanUnselectedDICOMs->setText(QString("clean DICOMS (%1 Gb").arg(GbString));
            else
                _cleanUnselectedDICOMs->setText(QString("clean DICOMS (%1 Mb").arg(MbString));
        }
    }
    FUNC_EXIT;
}

void MainWindow::findFIDs(bool all, bool remove)
{
    qint64 totalSizeAll=0;

    for (int jScan=0; jScan<_scans.size(); jScan++)
    {
        scanType scan = _scans.at(jScan);
        bool includeScan = all || scan.selectedAsImportant;
        qint64 totalSizeThisScan=0;
        if ( includeScan )
        {
            QString spec;
            spec = "fid";
            QDir dir(scan.scanName);
            QStringList const fileList = dir.entryList( {spec}, QDir::Files | QDir::NoSymLinks);

            qint64 totalSizeThisDir=0;
            for (int jFile=0; jFile<fileList.size(); jFile++)
            {
                QString fileName = scan.scanName + "/" + fileList.at(jFile);
                QFileInfo checkFile(fileName);
                if ( remove )
                {
                    QFile file(fileName);
                    bool success = file.remove();
                    if ( !success )
                        qInfo() << "failed to remove file" << fileName;
                    else
                        totalSizeThisDir += checkFile.size();
                }
                else
                    totalSizeThisDir += checkFile.size();
            }
            totalSizeThisScan += totalSizeThisDir;
            FUNC_INFO << "total size of DICOMS =" << totalSizeThisScan/(1024*1024) << "Mb";
        }
        totalSizeAll += totalSizeThisScan;
    }
    double Mb = static_cast<double>(totalSizeAll)/(1024.*1024.);
    double Gb = static_cast<double>(totalSizeAll)/(1024.*1024.*1024.);
    QString MbString; MbString.setNum(Mb,'g',3);
    QString GbString; GbString.setNum(Gb,'g',3);
    totalSizeAll /= 1024*1024;  // b -> Gb
    if ( remove )
        qInfo() << "Removed" << GbString << "Gb (=" << MbString << "Mb)";
    else
    {
        if ( all )
        {
            _cleanAllFIDs->setEnabled(totalSizeAll > 0);
            if ( Gb > 1. )
                _cleanAllFIDs->setText(QString("clean fid files (%1 Gb)").arg(GbString));
            else
                _cleanAllFIDs->setText(QString("clean fid files (%1 Mb)").arg(MbString));
        }
        else
        {
            _cleanUnselectedFIDs->setEnabled(totalSizeAll > 0);
            if ( Gb > 1. )
                _cleanUnselectedFIDs->setText(QString("clean fid files (%1 Gb)").arg(GbString));
            else
                _cleanUnselectedFIDs->setText(QString("clean fid files (%1 Mb)").arg(MbString));
        }
    }

    FUNC_EXIT;
}

void MainWindow::findAllFiles()
{
    FUNC_ENTER;
    QDirIterator it(".", QDirIterator::Subdirectories);
    double total = 0;
    while (it.hasNext())
    {
        FUNC_INFO << "directory" << it.fileName();
        total += it.fileInfo().size();
        it.next();
    }
    total /= 1024.*1024.*1024.;  // Gb
    QString gigabytes; gigabytes.setNum(total,'g',3);
    QString megabytes; megabytes.setNum(total*1024.,'g',3);
    if ( total > 1. )
        _totalSizeSubDirs->setText(gigabytes + " Gb");
    else
        _totalSizeSubDirs->setText(megabytes + " Mb");
    FUNC_INFO << "gb" << gigabytes;
    FUNC_EXIT << total << "Mb";
}
