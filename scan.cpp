#include <QtWidgets>
#include <QFile>
#include "brukerScan.h"

QWidget *MainWindow::createScanPanel()
{
    FUNC_ENTER;
    auto *scanPanel = new QWidget();

    auto *viewLayout = new QVBoxLayout();
    auto *viewPushButton = new QPushButton("view using FastMap");
    connect(viewPushButton, SIGNAL(pressed()), this, SLOT(viewScanUsingFastMap()));

    viewLayout->addWidget(viewPushButton);
    auto *viewBox = new QGroupBox("View selected scan");
    viewBox->setLayout(viewLayout);

    auto *pageLayout = new QVBoxLayout();
    pageLayout->addWidget(viewBox);
    scanPanel->setLayout(pageLayout);

    return scanPanel;
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
    for (int jList=0; jList<folderList.size(); jList++)
    {
        QDir scanDir(folderList.at(jList));
        scanDir.setNameFilters(QStringList()<<"fid");
        QStringList fileList = scanDir.entryList();
        if ( fileList.count() == 1 )
        {
            QString name = folderList.at(jList);
            int scanNumber = name.toInt();
            scanType thisScan;
            thisScan.scanNumber = QString("%1").arg(scanNumber);
            QString fileName = topDir.dirName() + "/" + name + "/method";
            thisScan.sequenceName = getParameterString(fileName,"Method");
            thisScan.sequenceName.remove("Bruker:");
            thisScan.timeDuration = getTimeDuration(fileName);
            FUNC_INFO << "jList" << jList << "scanNumber" << thisScan.scanNumber << "sequence" << thisScan.sequenceName;
            thisScan.dim = getImageDimensions(name);
            fileName = topDir.dirName() + "/" + name + "/pdata/1/visu_pars";
            QString visCorSize = getParameterString(fileName,"VisuCoreSize");
            FUNC_INFO << "visCorSize" << visCorSize;

            QString visOrder = getParameterString(fileName,"VisuFGOrderDesc");
            QRegularExpression comma("[,\\s]");// match a comma
            QStringList orderList = visOrder.split(comma);
            FUNC_INFO << "orderList for scan" << jList << "name" << name << "=" << orderList;
            if ( orderList.contains("FG_ECHO") ) thisScan.reorderEchoes = true;
            FUNC_INFO << "visOrder" << visOrder;
            if ( _saveSelectedScans.count() > 0 )
                thisScan.selectedAsImportant = _saveSelectedScans.contains(name);
            else
                thisScan.selectedAsImportant = thisScan.dim.z > 3 || thisScan.dim.t > 1;
            _scans.append(thisScan);
        }
    }

    // add scans to the table
    _scanItems.clear();
    _scanItemsBox->clear();
    _scanItems.resize(_scans.size());
    for (int jList=0; jList<_scans.size(); jList++)
    {
        scanType scan = _scans.at(jList);
        QString volumes = "volumes";  if ( scan.dim.t == 1 ) volumes = "volume";
        QString text = QString("%1 %2 %3x%4x%5 (%6 %7), %8").arg(scan.scanNumber).arg(scan.sequenceName)
                .arg(scan.dim.x).arg(scan.dim.y).arg(scan.dim.z).arg(scan.dim.t).arg(volumes).arg(scan.timeDuration);
        _scanItems[jList].setText(text);
        _scanItems[jList].setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        if ( scan.selectedAsImportant )
            _scanItems[jList].setCheckState(Qt::Checked);
        else
            _scanItems[jList].setCheckState(Qt::Unchecked);
        _scanItems[jList].setHidden(false);
        _scanItemsBox->addItem(&_scanItems[jList]);
    }

    // select the first important scan
    for (int jList=0; jList<_scans.size(); jList++)
    {
        if ( _scans.at(jList).selectedAsImportant )
        {
            FUNC_INFO << "** select scan **" << jList;
            _scanItems[jList].setSelected(true);
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

QString MainWindow::getTimeDuration(QString fileName)
{
    FUNC_ENTER << fileName;
    QString timeSpan;
    QFileInfo checkFile(fileName);
    if ( !(checkFile.exists() && checkFile.isFile()) )
        return timeSpan;

    QFile infile(fileName);
    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text))
        return timeSpan;
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
                QTime startTime = QTime(startHours, startMinutes, startSeconds, 0);

                // get the length of the scan
                QString lengthScan = getParameterString(fileName,"PVM_ScanTimeStr");
                QRegularExpression breakUnits("[hms]");// match an equal
                QStringList lengthPieces = lengthScan.split(breakUnits);
                int lengthHours   = lengthPieces.at(0).toInt(&ok);
                int lengthMinutes = lengthPieces.at(1).toInt(&ok);
                int lengthSeconds = lengthPieces.at(2).toInt(&ok);
                QTime lengthTime = QTime(lengthHours, lengthMinutes, lengthSeconds, 0);
                QTime endTime = startTime.addSecs(lengthHours * 60 * 60);
                endTime = endTime.addSecs(lengthMinutes * 60);
                endTime = endTime.addSecs(lengthSeconds);
                timeSpan = QString("@ %1 to %2").arg(startTime.toString()).arg(endTime.toString());
                break;
            }
        }
    }
    infile.close();
    FUNC_EXIT;
    return timeSpan;
}
