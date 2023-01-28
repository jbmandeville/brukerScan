#include <QtWidgets>
#include "ImageIO.h"
#include "MRIProcess.h"

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
    _FastmapMSTemplateDirectories = fmSettings.value("templateDirectories","").toStringList();
    _savedSettings.fmSmoothing    = fmSettings.value("smoothing1stLevel",1.).toDouble();
    FUNC_INFO << "template directories" << _FastmapMSTemplateDirectories;

    QCoreApplication::setOrganizationName("Martinos");
    QCoreApplication::setOrganizationDomain("http://www.nmr.mgh.harvard.edu");
    QCoreApplication::setApplicationVersion("3.0");
    QCoreApplication::setApplicationName("MRIProcess");

    readQSettings();

    _radioButtonSiemensHuman = new QRadioButton("Siemens Human");
    _radioButtonSiemensNonHuman = new QRadioButton("Siemens NonHuman");
    _radioButtonBruker   = new QRadioButton("Bruker");
    connect(_radioButtonSiemensHuman, SIGNAL(clicked(bool)), this, SLOT(dataOriginChanged()));
    connect(_radioButtonSiemensNonHuman, SIGNAL(clicked(bool)), this, SLOT(dataOriginChanged()));
    connect(_radioButtonBruker,   SIGNAL(clicked(bool)), this, SLOT(dataOriginChanged()));
    _radioButtonSiemensHuman->setChecked(true);
    auto *radioLayout = new QHBoxLayout();
    radioLayout->addWidget(_radioButtonBruker);
    radioLayout->addWidget(_radioButtonSiemensHuman);
    radioLayout->addWidget(_radioButtonSiemensNonHuman);
    auto *radioBox = new QGroupBox("Data origin");
    radioBox->setLayout(radioLayout);
    radioBox->setStyleSheet("background-color:lightBlue;");

    _tabs = new QTabWidget();
    createDownloadPage();
    createAnatomyPage();
    createfMRIPage();
    createCleanPage();
    _tabs->addTab(_downLoadPage,  tr("Download"));
    _tabs->addTab(_anatomyPage, tr("Anatomy"));
    _tabs->addTab(_fmriPage, tr("fMRI"));
    _tabs->addTab(_cleanPage, tr("cleanup"));
    _tabs->setTabToolTip(0,"Query database and download data");
    _tabs->setTabToolTip(1,"Align anatomy; potentially run freeSurfer;\nClick tab to refresh state");
    connect(_tabs, SIGNAL(currentChanged(int)), this, SLOT(changedPage(int)));

    _centralWidget = new QWidget(this);
    this->setCentralWidget( _centralWidget );
    auto *mainLayout = new QVBoxLayout( _centralWidget );
    mainLayout->addWidget(radioBox);
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
    quitAction->setShortcut(Qt::ControlModifier + Qt::Key_Q);
    connect(quitAction, &QAction::triggered, this, &MainWindow::exitApp);

    QSize iconSizeSmall(24,24);
    const QIcon *showOutputBrowser = new QIcon(":/My-Icons/textOutput.png");
    auto *outputBrowserAction = new QAction(*showOutputBrowser,"browser",this);
    outputBrowserAction->setCheckable(true);
    outputBrowserAction->setChecked(false);
    connect(outputBrowserAction, SIGNAL(toggled(bool)), this, SLOT(showOutputBrowser(bool)));
    outputBrowserAction->setToolTip("Show or hide the process output window");

    auto *helpBrowserAction = new QAction("HELP",this);
    helpBrowserAction->setCheckable(true);
    helpBrowserAction->setChecked(false);
    connect(helpBrowserAction, SIGNAL(toggled(bool)), this, SLOT(showHelpBrowser(bool)));
    helpBrowserAction->setToolTip("Show or hide the help window");

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
    loadHelp(page_download);
    openedAnatomyPage();
    readUnpackLog();
    readAvailableScanList();
    readSubjectVariables();
    setTemplate();
    dataOriginChanged();

    restoreGeometry(_savedSettings.imageWindowGeometry);
    _outputBrowser->restoreGeometry(_savedSettings.browserWindowGeometry);
    _helpTool->restoreGeometry(_savedSettings.browserWindowGeometry);
}

void MainWindow::helpGoBackward()
{
    FUNC_ENTER;
    _helpPageIndex--;
    if ( _helpPageIndex < page_download ) _helpPageIndex = page_clean;
    loadHelp(_helpPageIndex);
}
void MainWindow::helpGoForward()
{
    FUNC_ENTER;
    _helpPageIndex++;
    if ( _helpPageIndex > page_clean ) _helpPageIndex = page_download;
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

    int iLine=0;
    QString templateID;
    while (!in_stream.atEnd())
    {
        iLine++;
        QString line = in_stream.readLine();
        QStringList stringList = line.split(QRegularExpression("\\s+"));
        if ( stringList.isEmpty() ) continue;
        QString variable = stringList.at(0);
        FUNC_INFO << variable << stringList.count();
        if ( !variable.compare("multi-subject-template") && stringList.count() > 1 )
            templateID = stringList.at(1);
        if ( !variable.compare("analysis-type") && stringList.count() > 1 )
        {
            QString type = stringList.at(1);
            FUNC_INFO << type;
            if ( !type.compare("Siemens-Human") )
                _radioButtonBruker->setChecked(true);
            else if ( !type.compare("Bruker") )
                _radioButtonSiemensHuman->setChecked(true);
            else
                _radioButtonSiemensNonHuman->setChecked(true);
        }
    }
    infile.close();

    // find the template directory
    int iSelection = 0;
    FUNC_INFO << "save template" << templateID;
    for (int jList=0; jList<_FastmapMSTemplateDirectories.count(); jList+=2)
    {
        if ( !templateID.compare(_FastmapMSTemplateDirectories.at(jList)) )
            iSelection = jList/2;
    }
    _anatomyTemplateDirectory->setCurrentIndex(iSelection);
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

    out << "# analysis variables for this session" << "\n";
    out << "multi-subject-template " << _anatomyTemplateDirectory->currentText() << "\n";
    if ( _radioButtonSiemensHuman->isChecked() )
        out << "analysis-type Siemens-Human\n";
    else if ( _radioButtonBruker->isChecked() )
        out << "analysis-type Bruker\n";
    else
        out << "analysis-type Siemens-NonHuman\n";
    file.close();
}

void MainWindow::setTemplate()
{
    FUNC_ENTER;
    QString fileName = alignCOMFileName(0); // anatomy align.com
    FUNC_INFO << 1;
    QString argument = readFileTextArgument(fileName, "template-file");
    FUNC_INFO << 2;
    if ( !argument.isEmpty() )
    {
        FUNC_INFO << "argument =" << argument;
        QFileInfo checkFile(argument);
        QString dirName = checkFile.path();
        FUNC_INFO << "dirName =" << dirName;
        int found=-1;
        for (int jList=0; jList<_FastmapMSTemplateDirectories.count(); jList+=2)
        {
            FUNC_INFO << "FM template dir" << _FastmapMSTemplateDirectories.at(jList+1);
            if ( !dirName.compare(_FastmapMSTemplateDirectories.at(jList+1)) )
                 found = jList;
        }
        if ( found >= 0) _anatomyTemplateDirectory->setCurrentIndex(found/2);
    }
    FUNC_EXIT;
}

void MainWindow::readSmoothing(int which)
{ // which: 0=anatomy, 1=fMRI
    QString fileName = alignCOMFileName(which);
    FUNC_ENTER << which << fileName;
    QString argument = readFileTextArgument(fileName, "smoothing");
    if ( !argument.isEmpty() )
    {
        FUNC_INFO << "argument" << argument;
        bool ok;
        double smoothing = argument.toDouble(&ok);
        if ( !ok ) smoothing = 0.;
        QString number; number.setNum(smoothing);
        FUNC_INFO << "number" << number;
        if ( which == 0 )
        {
            _smoothingAnatomy->setText(number);
            _smoothingfMRI->setText(number);
        }
        else // if ( which == 1 )
            _smoothingfMRI->setText(number);
    }
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

    int iLine=0;
    while (!in_stream.atEnd())
    {
        iLine++;
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

QString MainWindow::alignCOMFileName(int which)
{ // which: 0=anatomy, 1=fMRI
    FUNC_ENTER << which << _fMRITemplateDirBox->count();
    QString fileName = "";
    if ( which == 0 && anatomyFileExists("align.com") )
        fileName = "anatomy/" + _anatomyDirBox->currentText() + "/align.com";
    else // if ( which == 1 && epiFileExists("align.com") )
        fileName = "epi/" + _fMRITemplateDirBox->currentText() + "/align.com";
    return fileName;
}

void MainWindow::changedSmoothingAnatomy()
{
    bool ok;
    double smoothing = _smoothingAnatomy->text().toDouble(&ok);
    if ( !ok ) smoothing = 0.;
    QString number; number.setNum(smoothing);
    _smoothingAnatomy->setText(number);
    _smoothingfMRI->setText(number);
}

void MainWindow::changedSmoothingfMRI()
{
    bool ok;
    double smoothing = _smoothingfMRI->text().toDouble(&ok);
    if ( !ok ) smoothing = 0.;
    QString number; number.setNum(smoothing);
    _smoothingAnatomy->setText(number);
    _smoothingfMRI->setText(number);
}


void MainWindow::dataOriginChanged()
{
    _freeSurferGroupBox->setVisible(_radioButtonSiemensHuman->isChecked());
    _extractFreeSurferOverlaysButton->setVisible(_radioButtonSiemensHuman->isChecked());
    _convert2dseqWidget->setVisible(_radioButtonBruker->isChecked());
    _tabs->setTabEnabled(0,!_radioButtonBruker->isChecked());
    writeSubjectVariables();
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

    if ( index == page_download )
        openedAnatomyPage();
    else if ( index == page_anatomy )
        openedAnatomyPage();
    else if ( index == page_fMRI )
        openedfMRIPage();
    else if ( index == page_clean )
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
    if ( whichTab == page_download )
    {
        inFile.setFileName(":/My-Text/help-download");
        _helpTool->setWindowTitle("Help - Downloads");
    }
    else if ( whichTab == page_anatomy )
    {
        inFile.setFileName(":/My-Text/help-anatomy");
        _helpTool->setWindowTitle("Help - Anatomy");
    }
    else if ( whichTab == page_fMRI )
    {
        inFile.setFileName(":/My-Text/help-fmri");
        _helpTool->setWindowTitle("Help - fMRI");
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
    QRegExp rx("[,\\s]");// match a comma or a space

    // find the 1st tab
    QStringList stringList;
    bool newTab = false;
    while ( !in_stream.atEnd() && !newTab )
    {
        QString line = in_stream.readLine();
        FUNC_INFO << "line" << line;
        stringList = line.split(rx, QString::SkipEmptyParts);
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
        stringList = line.split(rx, QString::SkipEmptyParts);
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
}

void MainWindow::writeQSettings()
{
    if ( !isMaximized() )
        _savedQSettings.setValue("imageWindowGeometry",saveGeometry());
    if ( !isMaximized() )
        _savedQSettings.setValue("browserWindowGeometry",_outputBrowser->saveGeometry());

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
