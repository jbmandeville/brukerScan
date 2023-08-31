#include <QtWidgets>
#include "ImageIO.h"
#include "brukerScan.h"

MainWindow::~MainWindow()
{
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    QCoreApplication::setOrganizationName("Martinos");
    QCoreApplication::setOrganizationDomain("http://www.nmr.mgh.harvard.edu");
    QCoreApplication::setApplicationVersion("3.0");
    QCoreApplication::setApplicationName("fastmap");
    QSettings fmSettings;

    QCoreApplication::setOrganizationName("Martinos");
    QCoreApplication::setOrganizationDomain("http://www.nmr.mgh.harvard.edu");
    QCoreApplication::setApplicationVersion("3.0");
    QCoreApplication::setApplicationName("MRIProcess");

    readQSettings();

    _tabs = new QTabWidget();
    createScanPage();
    createCleanPage();
    _tabs->addTab(_scanPage,  tr("scans"));
    _tabs->addTab(_cleanPage, tr("cleanup"));
    _tabs->setTabToolTip(0,"Query scan directories");
    connect(_tabs, SIGNAL(currentChanged(int)), this, SLOT(changedPage(int)));

    _centralWidget = new QWidget(this);
    this->setCentralWidget( _centralWidget );
    auto *mainLayout = new QVBoxLayout( _centralWidget );
    mainLayout->addWidget(_tabs);
    _noteBox.resize(_tabs->count());
    for (int jNote=0; jNote<_tabs->count(); jNote++)
    {
        _noteBox[jNote] = new QTextEdit("");
        _noteBox[jNote]->setMaximumHeight(250);
        _noteBox[jNote]->setVisible(false);
        mainLayout->addWidget(_noteBox[jNote]);
    }

    _statusBar = this->statusBar();
    _statusBar->setStyleSheet("color:Darkred");
    mainLayout->addWidget(_statusBar);

    // add a menu
    auto *menuBar = new QMenuBar;
    mainLayout->setMenuBar(menuBar);
    QMenu *mainMenu  = new QMenu(tr("&Menu"), this);
    auto *helpMenu  = new QMenu(tr("Help"), this);
    menuBar->addMenu(mainMenu);
    menuBar->addMenu(helpMenu);

    QAction *aboutAppAct = helpMenu->addAction(tr("About this"));
    connect(aboutAppAct, &QAction::triggered, this, &MainWindow::aboutApp);

    QAction *quitAction = mainMenu->addAction(tr("&Quit"));
    // short-cuts and tooltips
    quitAction->setShortcut(Qt::ControlModifier | Qt::Key_Q);
    connect(quitAction, &QAction::triggered, this, &MainWindow::exitApp);

    _outputBrowser = new QTextBrowser;
    _helpBrowser   = new QTextBrowser;
    _helpBrowser->setOpenLinks(true);
    _helpBrowser->setOpenExternalLinks(true);
    _helpTool = new QWidget();
    auto *buttonLayout = new QHBoxLayout();
    auto *helpForward  = new QPushButton(">>");
    auto *helpBackward = new QPushButton("<<");
    buttonLayout->addWidget(helpBackward);
    buttonLayout->addWidget(helpForward);
    auto *layout   = new QVBoxLayout();
    layout->addLayout(buttonLayout);
    layout->addWidget(_helpBrowser);
    _helpTool->setLayout(layout);

    connect(helpBackward, SIGNAL(clicked()), this, SLOT(helpGoBackward()));
    connect(helpForward,  SIGNAL(clicked()), this, SLOT(helpGoForward()));

    auto *helpBrowserAction = new QAction("HELP",this);
    helpBrowserAction->setCheckable(true);
    helpBrowserAction->setChecked(false);
    connect(helpBrowserAction, SIGNAL(toggled(bool)), this, SLOT(showHelpBrowser(bool)));
    helpBrowserAction->setToolTip("Show or hide the help window");

    QSize iconSizeSmall(24,24);
    const QIcon *showOutputBrowser = new QIcon(":/My-Icons/textOutput.png");
    auto *outputBrowserAction = new QAction(*showOutputBrowser,"browser",this);
    outputBrowserAction->setCheckable(true);
    outputBrowserAction->setChecked(false);
    connect(outputBrowserAction, SIGNAL(toggled(bool)), this, SLOT(showOutputBrowser(bool)));
    outputBrowserAction->setToolTip("Show or hide the process output window");

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QFrame* separator1 = new QFrame();
    separator1->setFrameShape(QFrame::HLine);
    separator1->setLineWidth(3);
    separator1->setFixedHeight(20);
    separator1->setFrameShadow(QFrame::Raised);

    QFrame* separator2 = new QFrame();
    separator2->setFrameShape(QFrame::HLine);
    separator2->setFixedHeight(_statusBar->height());
    separator2->setFrameShadow(QFrame::Raised);

    _showNotesAction = new QAction("Notes",this);
    _showNotesAction->setToolTip("Show or hide the Notes");
    _showNotesAction->setCheckable(true);
    _showNotesAction->setChecked(false);
    connect(_showNotesAction, SIGNAL(toggled(bool)), this, SLOT(showNotes(bool)));

    QToolBar *sideToolBar = addToolBar(tr("tool bar"));
    sideToolBar->setIconSize(iconSizeSmall);
    sideToolBar->addAction(outputBrowserAction);
    sideToolBar->addAction(helpBrowserAction);
    sideToolBar->addWidget(spacer);
    sideToolBar->addAction(_showNotesAction);
    sideToolBar->addWidget(separator2);
    addToolBar(Qt::LeftToolBarArea, sideToolBar);

    /*
    QSize defaultWindowSize;
    QRect rec = QApplication::desktop()->screenGeometry();
    defaultWindowSize.setWidth(rec.width()/4);
    defaultWindowSize.setHeight(rec.height()/2);
    resize(defaultWindowSize);
    _outputBrowser->resize(defaultWindowSize);
    */

    loadNotes();
    QString subjectID = getParameterString("subject","SUBJECT_id");
    FUNC_INFO << "subjectID" << subjectID;
    _subjectID->setText(subjectID);
    scanDirectories();
//    readSubjectVariables();

    restoreGeometry(_savedSettings.imageWindowGeometry);
    _outputBrowser->restoreGeometry(_savedSettings.browserWindowGeometry);
    _helpTool->restoreGeometry(_savedSettings.imageWindowGeometry);
}

void MainWindow::helpGoBackward()
{
    FUNC_ENTER;
    _helpPageIndex--;
    if ( _helpPageIndex < page_scan ) _helpPageIndex = page_clean;
    loadHelp(_helpPageIndex);
}
void MainWindow::helpGoForward()
{
    FUNC_ENTER;
    _helpPageIndex++;
    if ( _helpPageIndex > page_clean ) _helpPageIndex = page_scan;
    loadHelp(_helpPageIndex);
}

void MainWindow::readSubjectVariables()
{
    FUNC_ENTER;
    QString fileName = "analyze.dat";
    QFileInfo checkFile(fileName);
    if ( !(checkFile.exists() && checkFile.isFile()) )
        return;

    // Read the time model file
    QFile infile(fileName);
    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in_stream(&infile);

    QString templateID;
    while (!in_stream.atEnd())
    {
        QString line = in_stream.readLine();
        QStringList stringList = line.split(QRegularExpression("\\s+"));
        if ( stringList.isEmpty() ) continue;
        QString variable = stringList.at(0);
        FUNC_INFO << variable << stringList.count();
        if ( !variable.compare("multi-subject-template") && stringList.count() > 1 )
            templateID = stringList.at(1);
    }
    infile.close();

    FUNC_EXIT;
}
void MainWindow::writeSubjectVariables()
{
    FUNC_ENTER;
    QString fileName = "analyze.dat";
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&file);

    file.close();
}

QString MainWindow::readFileTextArgument(QString fileName, QString parameterName)
{
    QFileInfo checkFile(fileName);
    if ( !(checkFile.exists() && checkFile.isFile()) )
        return "";

    QFile infile(fileName);
    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text))
        return "";
    QTextStream in_stream(&infile);

    while (!in_stream.atEnd())
    {
        QString line = in_stream.readLine();
        if ( line.isEmpty() ) continue;
        FUNC_INFO << "line" << line;
        QStringList stringList = line.split(QRegularExpression("\\s+"));
        FUNC_INFO << stringList << stringList;
        int iParameter=stringList.indexOf(parameterName);
        FUNC_INFO << "iParameter" << iParameter;
        if ( iParameter >= 0 && iParameter < stringList.size()-1 )
        {
            FUNC_INFO << "read iParameter";
            return stringList.at(iParameter+1);
        }
    }
    infile.close();
    FUNC_EXIT;
    return "";
}

void MainWindow::showNone()
{
    for (int jNote=0; jNote<_noteBox.count(); jNote++)
        _noteBox[jNote]->setVisible(false);
}

void MainWindow::showNotes(bool show)
{
    showNone();
    int tabIndex = _tabs->currentIndex();
    _noteBox[tabIndex]->setVisible(show);
}

void MainWindow::changedPage(int index)
{
    FUNC_ENTER << index;
    showNotes(_showNotesAction->isChecked());
    writeAllNotes();
    loadHelp(index);

    if ( index == page_clean )
        openedCleanPage();
}

void MainWindow::writeAllNotes()
{
    FUNC_ENTER;
    QString fileName = "notes.dat";
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&file);

    for (int jNote=0; jNote<_tabs->count(); jNote++)
    {
        out << "*tab* " << _tabs->tabText(jNote) << "\n";
        out << _noteBox[jNote]->toPlainText() << "\n";
    }
    file.close();
    FUNC_EXIT;
}

void MainWindow::loadHelp(int whichTab)
{
    FUNC_ENTER << whichTab;
    _helpBrowser->clear();

    QFile inFile;
    if ( whichTab == page_scan )
    {
        inFile.setFileName(":/My-Text/help-scan");
        _helpTool->setWindowTitle("Help - Directories");
    }
    else if ( whichTab == page_clean )
    {
        inFile.setFileName(":/My-Text/help-clean");
        _helpTool->setWindowTitle("Help - Cleanup");
    }
    else
        return;
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in_stream(&inFile);
    QString notesString;
    while ( !in_stream.atEnd() )
    {
        QString line = in_stream.readLine();
        notesString = notesString + line + "\n";
    }
    inFile.close();

    _helpBrowser->append(notesString);
    FUNC_EXIT;
}

void MainWindow::loadNotes()
{
    FUNC_ENTER;
    QFile inFile;
    inFile.setFileName("notes.dat");
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in_stream(&inFile);
    QRegularExpression whiteSpaceComma("[,\\s]");// match a comma or a space

    // find the 1st tab
    QStringList stringList;
    bool newTab = false;
    while ( !in_stream.atEnd() && !newTab )
    {
        QString line = in_stream.readLine();
        FUNC_INFO << "line" << line;
        stringList = line.split(whiteSpaceComma, Qt::SkipEmptyParts);
        FUNC_INFO << stringList;
        newTab = stringList.count() > 1 && !stringList.at(0).compare("*tab*");
        FUNC_INFO << "newTab" << newTab;
    }
    if ( !newTab ) return;

    // go through file and read text for each tab
    int whichTab = whichTabName(stringList.at(1));
    QString notesString;
    while ( !in_stream.atEnd() )
    {
        FUNC_INFO << "whichTab" << whichTab;
        QString line = in_stream.readLine();
        FUNC_INFO << "line" << line;
        stringList = line.split(whiteSpaceComma, Qt::SkipEmptyParts);
        FUNC_INFO << "stringList" << stringList;
        newTab = stringList.count() > 1 && !stringList.at(0).compare("*tab*");
        if ( !newTab )
        {
            if ( !line.isEmpty() )
            {
                notesString = notesString + line + "\n";
                FUNC_INFO << "append:" << notesString;
            }
        }
        else if ( whichTab >= 0 )
        {
            FUNC_INFO << "assign:" << whichTab << notesString;
            _noteBox[whichTab]->setText(notesString);
            notesString.clear();
            whichTab = whichTabName(stringList.at(1));
            FUNC_INFO << "next tab" << whichTab;
        }
    }
    if ( whichTab >= 0 )
        _noteBox[whichTab]->setText(notesString);

    inFile.close();
    FUNC_EXIT;
}

int MainWindow::whichTabName(QString name)
{
    FUNC_ENTER;
    int whichTab = -1;
    for (int jTab=0; jTab<_tabs->count(); jTab++)
    {
        QString tabName = _tabs->tabText(jTab);
        if ( !tabName.compare(name) ) whichTab = jTab;
    }
    FUNC_EXIT << whichTab;
    return whichTab;
}

void MainWindow::aboutApp()
{
    QMessageBox msgBox;
    QString version = qVersion();
    QString text = "Qt Version " + version;
    msgBox.setText(text);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();

    QMessageBox::information(nullptr, QGuiApplication::applicationDisplayName(),
                             QGuiApplication::applicationDisplayName() + ' '
                             + QCoreApplication::applicationVersion() + " , by Joe Mandeville;\n" +
                             "Request bug fixes by email to\njbm@nmr.mgh.harvard.edu\nwith subject line 'simulator bug'.");
}

void MainWindow::exitApp()
{
    writeAllNotes();
    writeQSettings();
    QCoreApplication::exit(0);
}

void MainWindow::readQSettings()
{
    QByteArray defaultImageWindowGeometry = saveGeometry();  // defined in ImageWindow constructor
    _savedSettings.imageWindowGeometry    = _savedQSettings.value("imageWindowGeometry",defaultImageWindowGeometry).toByteArray();
    _savedSettings.browserWindowGeometry  = _savedQSettings.value("browserWindowGeometry",defaultImageWindowGeometry).toByteArray();
    _savedSettings.helpWindowGeometry     = _savedQSettings.value("helpWindowGeometry",defaultImageWindowGeometry).toByteArray();
}

void MainWindow::writeQSettings()
{
    _savedQSettings.setValue("imageWindowGeometry",saveGeometry());

    _savedQSettings.sync();
}

QString MainWindow::getDimensions(QString fileName, iPoint4D &dim)
{
    QFileInfo checkFile(fileName);
    if ( checkFile.exists() && checkFile.isFile() )
    {
        ImageIO file;
        if ( !file.readFileHeader(fileName,false) )
        {
            dim = file.getDimensions();
            dPoint4D res = file.getResolution();
            double duration = dim.t * res.t / 60.;
            QString text = QString("%1 x %2 x %3 with %4 time points (%5 min)")
                    .arg(dim.x).arg(dim.y).arg(dim.z).arg(dim.t).arg(duration);
            return text;
        }
        else
        {
            dim={0,0,0,0};
            return "";
        }
    }
    else
    {
        dim={0,0,0,0};
        return "";
    }
}

QString MainWindow::twoDigits(short time)
{
    QString text;
    if ( time < 10 )
        text = QString("0%1").arg(time);
    else
        text = QString("%1").arg(time);
    return text;
}

QString MainWindow::getParameterString(QString fileName, QString parameterName)
{
    qInfo() << "\n\n";
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
            _scans.append(thisScan);
        }
    }

    // add scans to the table
    _scanItems.resize(_scans.size());
    for (int jList=0; jList<_scans.size(); jList++)
    {
        scanType scan = _scans.at(jList);
        QString volumes = "volumes";  if ( scan.dim.t == 1 ) volumes = "volume";
        QString text = QString("%1 %2 %3x%4x%5 (%6 %7)").arg(scan.scanNumber).arg(scan.sequenceName)
                .arg(scan.dim.x).arg(scan.dim.y).arg(scan.dim.z).arg(scan.dim.t).arg(volumes);
        _scanItems[jList].setText(text);
        _scanItems[jList].setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        if ( scan.selectedAsImportant )
            _scanItems[jList].setCheckState(Qt::Checked);
        else
            _scanItems[jList].setCheckState(Qt::Unchecked);
        _scanItems[jList].setHidden(false);
        _scanItemsBox->addItem(&_scanItems[jList]);
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
        dim.t = nZnT / nZ;
    }
    else
        dim.t = nZnT;
    return dim;
}

int MainWindow::getVisuCoreOrientation(QString fileName)
{
    qInfo() << "\n\n";
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
