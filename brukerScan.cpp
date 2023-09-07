#include <QtWidgets>
#include "brukerScan.h"

void MainWindow::readCommandLine()
{
    QStringList commandLine = QCoreApplication::arguments();

    switch (parseCommandLine(commandLine))
    {
    case CommandLineOk:
        break;
    case CommandLineError:
        exit(1);
    case CommandLineHelpRequested:
        QCoreApplication::exit(0);
    }
}

CommandLineParseResult MainWindow::parseCommandLine(QStringList commandLine)
{
    FUNC_ENTER << commandLine;
    QCommandLineParser parser;
    QString HelpText = "\nCalculate Ki values for several ROIs using table files.\n";
    HelpText.append("Syntax:  exe [Variable-TR table] [short-TE table] [long TE-table] [optional args]\n\n");
    HelpText.append("where\n\n");
    HelpText.append("tables have 1 header line (x lesion contra sinus) followed by time points");
    parser.setApplicationDescription(HelpText);

    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption spanOption({"s","span"}, "add SPAN upload page");
    parser.addOption(spanOption);
    const QCommandLineOption noCleanOption({"n","no-cleanup"}, "add SPAN upload page");
    parser.addOption(noCleanOption);

    bool success = parser.parse(commandLine);
    if ( !success || parser.isSet(helpOption))
        QTextStream(stdout) << parser.helpText();
    if ( !success ) return CommandLineError;

    // parsing is done at this point
    if ( parser.isSet(spanOption) )    _inputOptions.spanUpload = true;
    if ( parser.isSet(noCleanOption) ) _inputOptions.enableCleanup = false;

    FUNC_INFO << "span is set??" << _inputOptions.spanUpload;

    int numberArguments = commandLine.count();
    if ( numberArguments > 2 )   // executable [--span]
        return CommandLineError;
    _inputOptions.startupText = parser.helpText();

    return CommandLineOk;
}

MainWindow::~MainWindow()
{
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    FUNC_ENTER;
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
    readCommandLine();

    FUNC_INFO << "create subject";
    auto *subjectIDLabel = new QLabel("Subject ID");
    _subjectID = new QLineEdit("?");
    _subjectID->setFocusPolicy(Qt::ClickFocus);

    auto *openDirectory = new QPushButton("open subject");
    QPixmap pixmapSave(":/My-Icons/openFile.png");
    QIcon openIcon(pixmapSave);
    openDirectory->setIcon(openIcon);
    connect(openDirectory, SIGNAL(clicked()), this, SLOT(openNewSubject()));

    auto *refreshSubject = new QPushButton("refresh");
    QPixmap pixmapRefresh(":/My-Icons/editundo.png");
    QIcon refreshIcon(pixmapRefresh);
    refreshSubject->setIcon(refreshIcon);
    connect(refreshSubject, SIGNAL(clicked()), this, SLOT(updateStudy()));

    auto *queryLayout = new QGridLayout();
    queryLayout->addWidget(subjectIDLabel,0,0);
    queryLayout->addWidget(_subjectID,0,1);
    queryLayout->addWidget(refreshSubject,0,2);
    queryLayout->addWidget(openDirectory,0,3);

    _subjectScanTimes = new QLabel();

    auto *subjectLayout = new QVBoxLayout();
    subjectLayout->addLayout(queryLayout);
    subjectLayout->addWidget(_subjectScanTimes);

    auto *subjectGroupBox = new QGroupBox("Subject information");
    subjectGroupBox->setLayout(subjectLayout);

    FUNC_INFO << "create tabs";
    int iPanel=0;
    _tabs = new QTabWidget();
    auto *scanPanel   = createScanPanel();
    _tabs->addTab(scanPanel,  tr("scans"));
    _tabs->setTabToolTip(iPanel++,"Query scan directories");

    if ( _inputOptions.spanUpload )
    {
        FUNC_INFO << "create upload panel";
        auto *uploadPanel = createUploadPanel();
        _tabs->addTab(uploadPanel, tr("upload"));
        _tabs->setTabToolTip(iPanel++,"Upload DICOMs to SPAN (LONI) database");
    }

    auto *cleanPanel  = createCleanPanel();
    _tabs->addTab(cleanPanel, tr("cleanup"));
    _tabs->setTabToolTip(iPanel++,"Clean (remove) unnessary files to save space");

    connect(_tabs, SIGNAL(currentChanged(int)), this, SLOT(changedPage(int)));

    _scanTable = new QTableWidget(this);
//    _scanTable->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding);
//    _scanTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
    connect( _scanTable, SIGNAL(cellClicked(int,int)), this, SLOT(changedHighlightScan(int,int)));

    auto *scansLayout = new QVBoxLayout();
    scansLayout->addWidget(_scanTable);
    auto *scansBox = new QGroupBox("List of scans (check interesting ones)");
    scansBox->setLayout(scansLayout);

    FUNC_INFO << "create mainLayout";
    _centralWidget = new QWidget(this);
    this->setCentralWidget( _centralWidget );
    auto *mainLayout = new QVBoxLayout( _centralWidget );
    mainLayout->addWidget(subjectGroupBox);
    mainLayout->addWidget(_tabs);
    mainLayout->addWidget(scansBox);
    FUNC_INFO << 1;
    _noteBox.resize(_tabs->count());
    for (int jNote=0; jNote<_tabs->count(); jNote++)
    {
        FUNC_INFO << "jNote" << jNote;
        _noteBox[jNote] = new QTextEdit("");
        _noteBox[jNote]->setMaximumHeight(250);
        _noteBox[jNote]->setVisible(false);
        mainLayout->addWidget(_noteBox[jNote]);
    }

    FUNC_INFO << "status bar";
    _statusBar = this->statusBar();
    _statusBar->setStyleSheet("color:Darkred");
    mainLayout->addWidget(_statusBar);
    FUNC_INFO << "last stretch" << mainLayout->count()-1;
    mainLayout->setStretch(0,1);        // subject box
    mainLayout->setStretch(1,2);        // top panel (page-specific)
    mainLayout->setStretch(2,20);       // scans
    mainLayout->setStretch(3,1);        // note 1
    mainLayout->setStretch(4,1);        // note 2
    mainLayout->setStretch(5,1);        // status bar

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
    quitAction->setShortcut(Qt::ControlModifier + Qt::Key_Q);
    connect(quitAction, &QAction::triggered, this, &MainWindow::exitApp);

    _helpBrowser   = new QTextBrowser;
    _helpBrowser->setOpenLinks(true);
    _helpBrowser->setOpenExternalLinks(true);
    _helpTool = new QWidget();
    auto *buttonLayout = new QHBoxLayout();
    auto *helpForward  = new QPushButton(">>");
    auto *helpBackward = new QPushButton("<<");
    auto *helpDismiss  = new QPushButton("Dismiss");
    buttonLayout->addWidget(helpBackward);
    buttonLayout->addWidget(helpDismiss);
    buttonLayout->addWidget(helpForward);
    auto *layout   = new QVBoxLayout();
    layout->addLayout(buttonLayout);
    layout->addWidget(_helpBrowser);
    _helpTool->setLayout(layout);

    auto *helpBrowserAction = new QAction("HELP",this);
    helpBrowserAction->setCheckable(true);
    helpBrowserAction->setChecked(false);
    connect(helpBrowserAction, SIGNAL(toggled(bool)), this, SLOT(showHelpBrowser(bool)));
    helpBrowserAction->setToolTip("Show or hide the help window");

    connect(helpBackward, SIGNAL(clicked()), this, SLOT(helpGoBackward()));
    connect(helpForward,  SIGNAL(clicked()), this, SLOT(helpGoForward()));
    connect(helpDismiss,  SIGNAL(clicked()), helpBrowserAction, SLOT(trigger()));
    loadHelp(_helpPageIndex);

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
    connect(_showNotesAction, SIGNAL(toggled(bool)), this, SLOT(showNotes(bool)));
    _showNotesAction->setChecked(true);

    QSize iconSizeSmall(24,24);
    QToolBar *sideToolBar = addToolBar(tr("tool bar"));
    sideToolBar->setIconSize(iconSizeSmall);
    sideToolBar->addAction(helpBrowserAction);
    sideToolBar->addWidget(spacer);
    sideToolBar->addAction(_showNotesAction);
    sideToolBar->addWidget(separator2);
    addToolBar(Qt::LeftToolBarArea, sideToolBar);

    /*
    QSize defaultWindowSize;
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect rec = screen->availableGeometry();
    defaultWindowSize.setWidth(rec.width()/2);
    defaultWindowSize.setHeight(rec.height()/2);
    */

    restoreGeometry(_savedSettings.imageWindowGeometry);
    _helpTool->restoreGeometry(_savedSettings.imageWindowGeometry);

    updateStudy();
}

void MainWindow::updateStudy()
{
    // read notes, subject file, and scan directories
    loadNotes();
    _subjectIDSavedAtScanTime = getParameterString("subject","SUBJECT_id");
    _subjectID->setText(_subjectIDSavedAtScanTime);
    scanDirectories();

    // update window title
    QDir thisDir = QDir::currentPath();
    QStringList subDirs = thisDir.absolutePath().split("/");
    int nList = qMin(2,subDirs.count());
    QString list;  list.append("[..]/");
    for (int jList=nList; jList>0; jList--)
    {
        list.append(subDirs.at(subDirs.count()-jList));
        if ( jList != 1 ) list.append("/");
    }
    setWindowTitle(QString("%1").arg(list));
}

void MainWindow::openNewSubject()
{
    QString message;
    message = "Select a Bruker scan directory";
    _statusBar->showMessage(message,20000);

    QFileDialog fileDialog;
    fileDialog.setHidden(true);
    QString startingPath = QDir::currentPath() + "/..";
    FUNC_INFO << "startingPath" << startingPath;
    QString dirName = fileDialog.getExistingDirectory(this,
                                                      "Select existing directory",
                                                      startingPath,
                                                      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    FUNC_INFO << "dirName" << dirName;
    if ( dirName.isEmpty() ) return;
    QDir::setCurrent(dirName);
    updateStudy();
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

    updateCleaningList();
}

void MainWindow::writeAllNotes()
{
    FUNC_ENTER;
    QString fileName = "notes.dat";
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&file);

    // First write program variables: selected-scans and potential subject-ID
    out << "selected-scans " << concatenateSelectedScanString() << "\n";
    if ( _subjectID->text() != _subjectIDSavedAtScanTime )
        out << "subject-ID " << _subjectID->text() << "\n";

    // Now write tab-specific notes
    for (int jNote=0; jNote<_tabs->count(); jNote++)
    {
        out << "*tab* " << _tabs->tabText(jNote) << "\n";
        out << _noteBox[jNote]->toPlainText() << "\n";
    }
    file.close();
    FUNC_EXIT;
}

QString MainWindow::concatenateSelectedScanString()
{
    QString list;
    for (int jList=0; jList<_scans.size(); jList++)
    {
        scanType scan = _scans.at(jList);
        QString number = QString("%1 ").arg(scan.scanNumber);
        if ( scan.selectedAsImportant )
            list.append(number);
    }
    return list;
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

    // clear notes
    for (int jNote=0; jNote<_noteBox.count(); jNote++)
        _noteBox[jNote]->setText("");

    // find the 1st tab; also, read parameters
    QStringList stringList;
    _saveSelectedScans.clear();
    bool newTab = false;
    while ( !in_stream.atEnd() && !newTab )
    {
        QString line = in_stream.readLine();
        FUNC_INFO << "line" << line;
        stringList = line.split(whiteSpaceComma, Qt::SkipEmptyParts);
        QString keyword = stringList.at(0);
        if ( !keyword.compare("selected-scans") )
        {
            for (int jList=1; jList<stringList.count(); jList++)
                _saveSelectedScans.append(stringList.at(jList));
        }

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
            if ( notesString.isEmpty() ) notesString = "Notes: ";
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
