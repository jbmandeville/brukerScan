#include <QtWidgets>
#include <QFile>
#include "brukerScan.h"

QWidget *MainWindow::createScanPanel()
{
    FUNC_ENTER;
    auto *scanPanel = new QWidget();

    auto *viewLayout = new QVBoxLayout();
    _viewUsingFastmap = new QPushButton("view using FastMap");
    _viewUsingFastmap->setToolTip("Display using fastmap after creating header file 2dseq.hdr\nor reordering data into image.nii");
    connect(_viewUsingFastmap, SIGNAL(pressed()), this, SLOT(viewScanUsingFastMap()));

    viewLayout->addWidget(_viewUsingFastmap);
    auto *viewBox = new QGroupBox("View selected scan");
    viewBox->setLayout(viewLayout);

    auto *pageLayout = new QVBoxLayout();
    pageLayout->addWidget(viewBox);
    scanPanel->setLayout(pageLayout);

    return scanPanel;
}
void MainWindow::changedHighlightScan(int row, int column)
{
    FUNC_ENTER << row << column;
    _scanTable->selectRow(row);
    _viewUsingFastmap->setEnabled(_scans.at(row).completedScan);
}
void MainWindow::headerClicked(int column)
{
    qInfo() << "column" << column;
    if ( column == 1) _reverseOrderScans = !_reverseOrderScans;
    updateStudy();
}

void MainWindow::viewScanUsingFastMap()
{
    FUNC_ENTER;
    int iSelect = -1;
    for (int jScan=0; jScan<_scans.size(); jScan++)
    {
        auto *item = _scanTable->item(jScan,0);
        if ( item->isSelected() )
            iSelect = jScan;
    }
    FUNC_INFO << "iSelect" << iSelect;
    if ( iSelect >= 0 )
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

    QStringList arguments;
    arguments.append(thisScan.scanName);

    auto *process = new QProcess();
    FUNC_EXIT << "exe arguments" << exe << arguments;
    process->startDetached(exe,arguments);
}

QString MainWindow::getParameterString(QString fileName, QString parameterName)
{
    FUNC_ENTER << fileName << parameterName;
    QFileInfo checkFile(fileName);
    if ( !(checkFile.exists() && checkFile.isFile()) )
        return "";

    QFile infile(fileName);
    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text))
        return "";
    QTextStream in_stream(&infile);

    QString fullParameterName = "##$" + parameterName;
    FUNC_INFO << "fullParameterName" << fullParameterName;

    while (!in_stream.atEnd())
    {
        QString line = in_stream.readLine();
        if ( line.isEmpty() ) continue;
        FUNC_INFO << "line" << line;
        QRegularExpression filter("[=,]");// match a comma or a space
        QStringList stringList = line.split(filter);  // clazy:exclude=use-static-qregularexpression
        QString parameter = stringList.at(0);
        FUNC_INFO << "stringList" << stringList;
        FUNC_INFO << "parameter" << parameter;
        if ( !parameter.compare(fullParameterName,Qt::CaseInsensitive) && stringList.count() == 2)
        {
            QString parName = stringList.at(1);
            if ( parName.contains("(") ) // the value is on the next line
                parName = in_stream.readLine();
            parName.remove('<'); parName.remove('>'); parName.remove('('); parName.remove(')');
            return parName;
        }
    }
    infile.close();
    FUNC_EXIT;
    return "";
}

void MainWindow::scanDirectories()
{
    FUNC_ENTER;

    QDir const topDir("./");
    QStringList const folderList = topDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    FUNC_INFO << folderList;

    _scans.clear();
    _saveSelectedScans.clear();
    QTime maxTime = QTime(0, 0, 0, 0);
    QTime minTime = QTime(23, 59, 59, 0);
    for (int jScan=0; jScan<folderList.size(); jScan++)
    {
        QDir scanDir(folderList.at(jScan));
        QString name2dseq = scanDir.dirName() + "/pdata/1/2dseq";
        QString nameVisuPars = scanDir.dirName() + "/pdata/1/visu_pars";
        QFileInfo check2dseq(name2dseq);
        QFileInfo checkVisuPars(nameVisuPars);
        bool file2dseqExists    = check2dseq.exists()    && check2dseq.isFile();
        bool fileVisuParsExists = checkVisuPars.exists() && checkVisuPars.isFile();
        if ( file2dseqExists )
        {
            QString name = folderList.at(jScan);
            int scanNumber = name.toInt();
            scanType thisScan;
            thisScan.completedScan = fileVisuParsExists;
            thisScan.scanNumber = scanNumber;
            thisScan.scanName   = name;
            QString fileName = topDir.dirName() + "/" + name + "/method";
            thisScan.sequenceName = getParameterString(fileName,"Method");
            thisScan.sequenceName.remove("Bruker:");
            getSequenceTimes(fileName,thisScan);
            FUNC_INFO << "jScan" << jScan << "scanNumber" << thisScan.scanNumber << "sequence" << thisScan.sequenceName;
            thisScan.dim = getImageDimensions(name);
            fileName = topDir.dirName() + "/" + name + "/pdata/1/visu_pars";
            QString visCorSize = getParameterString(fileName,"VisuCoreSize");
            FUNC_INFO << "visCorSize" << visCorSize;

            QString visOrder = getParameterString(fileName,"VisuFGOrderDesc");
            QRegularExpression comma("[,\\s]");// match a comma
            QStringList orderList = visOrder.split(comma);
            FUNC_INFO << "orderList for scan" << jScan << "name" << name << "=" << orderList;
            if ( orderList.contains("FG_ECHO") ) thisScan.reorderEchoes = true;
            FUNC_INFO << "visOrder" << visOrder;
            if ( _saveSelectedScans.count() > 0 )
                thisScan.selectedAsImportant = _saveSelectedScans.contains(name);
            else
                thisScan.selectedAsImportant = thisScan.dim.z > 3 || thisScan.dim.t > 1;
            if ( thisScan.timeStart < minTime ) minTime = thisScan.timeStart;
            if ( thisScan.timeEnd   > maxTime ) maxTime = thisScan.timeEnd;
            _scans.append(thisScan);
        }
    }
    int diffMin = minTime.secsTo(maxTime) / 60;
    QString text = QString("%1    -->   %2   ,   duration %3 min").arg(minTime.toString()).arg(maxTime.toString()).arg(diffMin);
    _subjectScanTimes->setText(text);

    // sort the scans by start time
    iVector scanIndex;
    dVector scanNumber;
    for (int jScan=0; jScan<_scans.count(); jScan++)
    {
        scanType scan = _scans.at(jScan);
        scanIndex.append(jScan);
        scanNumber.append(scan.scanNumber);
    }
    utilMath::topDownMergeSort(scanNumber,scanIndex);
    QVector<scanType> saveScans = _scans;
    _scans.clear();
    if ( _reverseOrderScans )
    {
        for (int jScan=saveScans.count()-1; jScan>=0; jScan--)
            _scans.append(saveScans.at(scanIndex.at(jScan)));
    }
    else
    {
        for (int jScan=0; jScan<saveScans.count(); jScan++)
            _scans.append(saveScans.at(scanIndex.at(jScan)));
    }

    // add scans to the table
    _scanTable->clearContents();
    _scanTable->setColumnCount(10);
    QPixmap pixmap;
    if ( _reverseOrderScans )
        pixmap.load(":/My-Icons/upArrow.png");
    else
        pixmap.load(":/My-Icons/downArrow.png");
    QIcon arrowIcon(pixmap);
    QTableWidgetItem *scanHeaderItem = new QTableWidgetItem(arrowIcon,"Scan");
    QTableWidgetItem *seqHeaderItem   = new QTableWidgetItem(tr("Sequence"));
    QTableWidgetItem *xHeaderItem = new QTableWidgetItem(tr("x"));
    QTableWidgetItem *yHeaderItem = new QTableWidgetItem(tr("y"));
    QTableWidgetItem *zHeaderItem = new QTableWidgetItem(tr("z"));
    QTableWidgetItem *tHeaderItem = new QTableWidgetItem(tr("t"));
    QTableWidgetItem *startHeaderItem = new QTableWidgetItem(tr("start"));
    QTableWidgetItem *endHeaderItem = new QTableWidgetItem(tr("end"));
    QTableWidgetItem *durHeaderItem = new QTableWidgetItem(tr("minutes"));
    _scanTable->setHorizontalHeaderItem(1, scanHeaderItem);
    _scanTable->setHorizontalHeaderItem(2, seqHeaderItem);
    _scanTable->setHorizontalHeaderItem(3, xHeaderItem);
    _scanTable->setHorizontalHeaderItem(4, yHeaderItem);
    _scanTable->setHorizontalHeaderItem(5, zHeaderItem);
    _scanTable->setHorizontalHeaderItem(6, tHeaderItem);
    _scanTable->setHorizontalHeaderItem(7, startHeaderItem);
    _scanTable->setHorizontalHeaderItem(8, endHeaderItem);
    _scanTable->setHorizontalHeaderItem(9, durHeaderItem);
    _scanTable->verticalHeader()->setVisible(false);
//    scanHeaderItem->setFlags(scanHeaderItem->flags() & ~Qt::ItemIsSelectable);

    _scanTable->setRowCount(_scans.size());

    for (int jScan=0; jScan<_scans.size(); jScan++)
    {
        scanType scan = _scans.at(jScan);

        QTableWidgetItem *checkItem= new QTableWidgetItem("");
        QTableWidgetItem *scanItem = new QTableWidgetItem(QString("%1").arg(scan.scanName));
        QTableWidgetItem *seqItem  = new QTableWidgetItem(QString("%1").arg(scan.sequenceName));
        QTableWidgetItem *xItem    = new QTableWidgetItem(QString("%1").arg(scan.dim.x));
        QTableWidgetItem *yItem    = new QTableWidgetItem(QString("%1").arg(scan.dim.y));
        QTableWidgetItem *zItem    = new QTableWidgetItem(QString("%1").arg(scan.dim.z));
        QTableWidgetItem *tItem    = new QTableWidgetItem(QString("%1").arg(scan.dim.t));
        QTableWidgetItem *startItem= new QTableWidgetItem(QString("%1").arg(scan.timeStartString));
        QTableWidgetItem *endItem  = new QTableWidgetItem(QString("%1").arg(scan.timeEndString));
        QString minutes; minutes.setNum(scan.durationMinutes,'f',1);
        QTableWidgetItem *durItem  = new QTableWidgetItem(QString("%1").arg(minutes));

        checkItem->setFlags(checkItem->flags() | Qt::ItemIsUserCheckable);
        if ( _scans.at(jScan).selectedAsImportant )
            checkItem->setCheckState(Qt::Checked);
        else
            checkItem->setCheckState(Qt::Unchecked);
//        scanItem->setFlags(scanItem->flags() | Qt::ItemIsSelectable);

        if ( !scan.completedScan ) checkItem->setBackground(Qt::gray);

        _scanTable->setItem(jScan,0,checkItem);
        _scanTable->setItem(jScan,1,scanItem);
        _scanTable->setItem(jScan,2,seqItem);
        _scanTable->setItem(jScan,3,xItem);
        _scanTable->setItem(jScan,4,yItem);
        _scanTable->setItem(jScan,5,zItem);
        _scanTable->setItem(jScan,6,tItem);
        _scanTable->setItem(jScan,7,startItem);
        _scanTable->setItem(jScan,8,endItem);
        _scanTable->setItem(jScan,9,durItem);
    }
    _scanTable->resizeColumnsToContents();
    _scanTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    _scanTable->update();

    // select the first important scan
    for (int jScan=0; jScan<_scans.size(); jScan++)
    {
        if ( _scans.at(jScan).selectedAsImportant )
        {
            _scanTable->selectRow(jScan);
            break;
        }
    }
}

iPoint4D MainWindow::getImageDimensions(QString dirname)
{
    iPoint4D dim={0,0,0,0};
    QString visuParsName = dirname + "/pdata/1/visu_pars";
    QString visCorSize = getParameterString(visuParsName,"VisuCoreSize");
    QRegularExpression filter("[\\s]");// match a space
    QStringList list = visCorSize.split(filter);
    bool ok;
    if ( list.count() > 0 )
        dim.x = list.at(0).toInt(&ok);
    if ( list.count() > 1 )
        dim.y = list.at(1).toInt(&ok);
    if ( list.count() > 2 )
        dim.z = list.at(2).toInt(&ok);
    QString visCorFrameCount = getParameterString(visuParsName,"VisuCoreFrameCount");
    int nZnT = visCorFrameCount.toInt(&ok);
    int nZ = getVisuCoreOrientation(visuParsName);
    FUNC_INFO << "nZnT" << nZnT << "nZ" << nZ;
    if ( dim.z == 0 )
    {
        dim.z = nZ;
        dim.t = nZnT / qMax(1,nZ);
    }
    else
        dim.t = nZnT;
    return dim;
}

int MainWindow::getVisuCoreOrientation(QString fileName)
{
    FUNC_ENTER << fileName;
    QFileInfo checkFile(fileName);
    if ( !(checkFile.exists() && checkFile.isFile()) )
        return 0;

    QFile infile(fileName);
    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;
    QTextStream in_stream(&infile);

    QString fullParameterName = "##$VisuCoreOrientation";

    while (!in_stream.atEnd())
    {
        QString line = in_stream.readLine();
        if ( line.isEmpty() ) continue;
        FUNC_INFO << "line" << line;
        QRegularExpression filter("[=]");// match a comma or a space
        QStringList stringList = line.split(filter);  // clazy:exclude=use-static-qregularexpression
        QString parameter = stringList.at(0);
        FUNC_INFO << "stringList" << stringList;
        FUNC_INFO << "parameter" << parameter;
        if ( !parameter.compare(fullParameterName,Qt::CaseInsensitive) && stringList.count() == 2)
        {
            QString parName = stringList.at(1);
            if ( parName.contains("(") ) // this should always be the case
            {
                QRegularExpression newFilter("[,]");// match a comma or a space
                QStringList newList = parName.split(newFilter);  // clazy:exclude=use-static-qregularexpression
                FUNC_INFO << "newList" << newList;
                parName = newList.at(0);  parName.remove("(");
                bool ok;  int nFrames = parName.toInt(&ok);
                return nFrames;
            }
        }
    }
    infile.close();
    FUNC_EXIT;
    return 0;
}

void MainWindow::getSequenceTimes(QString fileName, scanType &scan)
{
    FUNC_ENTER << fileName;
    QFileInfo checkFile(fileName);
    if ( !(checkFile.exists() && checkFile.isFile()) )
        return;

    QFile infile(fileName);
    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in_stream(&infile);

    QString fullParameterName = "##Owner";

    while (!in_stream.atEnd())
    {
        QString line = in_stream.readLine();
        if ( line.isEmpty() ) continue;
        FUNC_INFO << "line" << line;
        QRegularExpression filter("[=]");// match an equal
        QStringList stringList = line.split(filter);  // clazy:exclude=use-static-qregularexpression
        QString parameter = stringList.at(0);
        FUNC_INFO << "stringList" << stringList;
        FUNC_INFO << "parameter" << parameter;
        if ( !parameter.compare(fullParameterName,Qt::CaseInsensitive) && stringList.count() == 2)
        {
            QString line = in_stream.readLine();
            QRegularExpression space("[\\s+]");// match an equal
            QStringList stringList = line.split(space);  // clazy:exclude=use-static-qregularexpression
            if ( stringList.count() > 2 )
            {
                QRegularExpression splitTime("[:.]");// match an equal
                QStringList timePieces = stringList.at(2).split(splitTime);
                bool ok;
                int startHours   = timePieces.at(0).toInt(&ok);
                int startMinutes = timePieces.at(1).toInt(&ok);
                int startSeconds = timePieces.at(2).toInt(&ok);
                scan.timeStart = QTime(startHours, startMinutes, startSeconds, 0);

                // get the length of the scan
                QString lengthScan = getParameterString(fileName,"PVM_ScanTimeStr");
                QRegularExpression breakUnits("[hms]");// match an equal
                QStringList lengthPieces = lengthScan.split(breakUnits);
                int lengthHours   = lengthPieces.at(0).toInt(&ok);
                int lengthMinutes = lengthPieces.at(1).toInt(&ok);
                int lengthSeconds = lengthPieces.at(2).toInt(&ok);
                QTime lengthTime = QTime(lengthHours, lengthMinutes, lengthSeconds, 0);
                scan.durationMinutes = static_cast<double>(lengthTime.msecsSinceStartOfDay()) / 1000. / 60.;
                // qInfo() << "duration minutes" << scan.durationMinutes;
                scan.timeEnd = scan.timeStart.addSecs(lengthHours * 60 * 60);
                scan.timeEnd = scan.timeEnd.addSecs(lengthMinutes * 60);
                scan.timeEnd = scan.timeEnd.addSecs(lengthSeconds);
                scan.timeStartString = scan.timeStart.toString();
                scan.timeEndString = scan.timeEnd.toString();
                break;
            }
        }
    }
    infile.close();
    FUNC_EXIT;
}
