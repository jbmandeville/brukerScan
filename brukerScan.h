#ifndef MRIPROCESS_H
#define MRIPROCESS_H

#include <QMainWindow>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProcess>
#include <QComboBox>
#include <QCheckBox>
#include <QSettings>
#include <QListWidget>
#include <QGroupBox>
#include <QTextBrowser>
#include <QAction>
#include <QRadioButton>
#include <QDebug>
#include <QStatusBar>
#include <QHBoxLayout>

#include "io.h"

////////////////////////////////////////////////////
// Global variables:
//
// TO DO:

////////////////////////////////////////////////////

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:

//    QString _fastmapProcess     = "/usr/pubsw/packages/jip/bin/Linux-x86_64/FM";

    QString _fastmapProcess     = "/usr/pubsw/packages/FASTMAP/current/bin/FM";
    QString _scriptDirectory    = "/space/deltabp/1/users/public/script/Bruker/";

    QString _subjectIDSavedAtScanTime;
    enum tabPages
    {
        page_scan,
        page_clean
    };
    struct scanType
    {
        QString scanNumber;
        QString sequenceName;
        iPoint4D dim={0,0,0,0};  // x,y,z,t
        bool selectedAsImportant=false;
        bool selectedForCleaning;
        bool reorderEchoes=false;
    };
    struct savedSettings
    {
        // Image Window
        QByteArray imageWindowGeometry;
        QByteArray browserWindowGeometry;
        QByteArray helpWindowGeometry;
    };

    QWidget *_centralWidget;
    QTabWidget *_tabs;
    QWidget *_scanPage;
    QWidget *_cleanPage;
    QStatusBar *_statusBar;
    QVector<QTextEdit *> _noteBox;
    QAction *_showNotesAction;

    QTextBrowser *_outputBrowser;
    QWidget *_helpTool;
    QTextBrowser *_helpBrowser;
    int _helpPageIndex=0;

    // Scans page
    QLineEdit *_subjectID;
    QListWidget *_scanItemsBox;
    QVector<QListWidgetItem> _scanItems;

    // clean page
    QListWidget *_cleanScanTypesBox;
    QVector<QListWidgetItem> _cleanScanTypeItems;
    QPushButton *_cleanDICOMs;
    QPushButton *_cleanNII_auxilliary;  // MR*.nii, test.nii, matchingMRI-rs.nii, matchingMRI-mc.nii
    QPushButton *_cleanNII_raw;         // raw.nii
    QLabel *_totalSizeSubDirs;

    // non-GUI variables
    QVector<scanType> _scans;
    savedSettings _savedSettings;
    QSettings _savedQSettings;

    void createScanPage();
    void createCleanPage();
    void openedCleanPage();
    void readQSettings();
    void writeQSettings();
    QString readFileTextArgument(QString fileName, QString parameterName);
    void readSubjectVariables();
    void readSmoothing(int which);
    void writeAllNotes();
    void loadNotes();
    void loadHelp(int whichTab);
    int whichTabName(QString name);
    void setupScanTypes();
    QString getParameterString(QString fileName, QString parameterName);
    int getVisuCoreOrientation(QString fileName);
    iPoint4D getImageDimensions(QString dirname);
    void scanDirectories();
    QString concatenateSelectedScanString();

    void updateCleaningList();

    QString getDimensions(QString fileName, iPoint4D &dim);
    QString twoDigits(short time);
    void writeJipCommandFileForMatchingMRI();

    QString alignCOMFileName(int which);

    inline void spawnProcess(QProcess *process, QString exe, QStringList arguments,
                             QString message, QString browserTitle )
    {
        _statusBar->showMessage(message);
        _centralWidget->setEnabled(false);
        if ( !browserTitle.isEmpty() )
        {
            _outputBrowser->setWindowTitle("Query Progress");
            _outputBrowser->show();
        }
        qInfo() <<  exe << arguments;
        process->start(exe,arguments);
    }

    inline void finishedProcess()
    {
        _statusBar->clearMessage();
        showOutputBrowser(false);
        _centralWidget->setEnabled(true);
    }

    void findDICOMs(bool remove);
    void findAuxFiles(bool remove);
    void findRawFiles(bool remove);
    void findAllFiles();

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    inline void enableGUI(int exitCode, QProcess::ExitStatus exitStatus )
    {
        qInfo() << "finished";
        finishedProcess();
    }
    inline void outputToBrowser()
    {
        QProcess *process = qobject_cast<QProcess*>(sender());
        _outputBrowser->append(process->readAllStandardOutput());
    }
    inline void showOutputBrowser(bool show)
    {
        if ( show ) _outputBrowser->show();
        else        _outputBrowser->hide();
    }
    inline void showHelpBrowser(bool show)
    {
        FUNC_ENTER << show;
        FUNC_INFO << _helpTool;
        if ( show ) _helpTool->show();
        else        _helpTool->hide();
        FUNC_EXIT;
    }
    void showNone();
    void showNotes(bool show);
    void helpGoBackward();
    void helpGoForward();
    void viewScanUsingFastMap();
    void displayFM(int iScan);

    void changedSelectScanCheckBox(QListWidgetItem *item);

    void aboutApp();
    void exitApp();
    void changedPage(int index);
    void writeSubjectVariables();

    void changedScanTypeCheckBox(QListWidgetItem *item);
    inline void cleanDICOMFiles()     {findDICOMs(true);         findDICOMs(false);         findAllFiles();}
    inline void cleanAuxNIIFiles()    {findAuxFiles(true);       findAuxFiles(false);       findAllFiles();}
    inline void cleanRawNIIFiles()    {findRawFiles(true);       findRawFiles(false);       findAllFiles();}

};

#endif // MRIPROCESS_H
