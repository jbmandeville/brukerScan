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
#include <QTableWidget>
#include <QListWidget>
#include <QGroupBox>
#include <QTextBrowser>
#include <QAction>
#include <QRadioButton>
#include <QDebug>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QTime>
#include <QSplitter>

#include "io.h"

////////////////////////////////////////////////////
// Global variables:
//
// TO DO:

////////////////////////////////////////////////////

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineHelpRequested
};
struct CommandOptions
{
    QString startupText;      // help
    bool helpRequested=false;
    bool spanUpload=false;
    bool enableCleanup=true;  // disable for acquisition mode
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    enum tabPages
    {
        page_scan,
        page_clean
    };
    struct scanType
    {
        int scanNumber;
        QString scanName;        // could be symbolic link: e.g.: rare --> 3
        QString sequenceName;
        QString timeStartString;
        QString timeEndString;
        double durationMinutes;
        QTime timeStart;
        QTime timeEnd;
        iPoint4D dim={0,0,0,0};  // x,y,z,t
        bool completedScan=false;   // contains 2dseq and visu_pars
        bool recoScan=false;   // contains at least reco
        bool selectedAsImportant=false;
        bool reorderEchoes=false;
    };
    struct savedSettings
    {
        // Image Window
        QByteArray imageWindowGeometry;
        QByteArray browserWindowGeometry;
        QByteArray helpWindowGeometry;
    };

    QString _scriptDirectory    = "/space/deltabp/1/users/public/script/Bruker/";

    QString _dataBaseDirectory   = "/homes/1/jbm/space2/projects/span2/";
    QString _uploadTempDirectory = "/space/deltabp/2/users/jbm/projects/upload/";
    QString _javaExeLocation     = "/homes/1/jbm/space1/dev/java/jdk-16.0.1/bin/java";
    QString _IDAUploaderFile     = "/space/deltabp/1/users/public/bins/IDA-Uploader-2.0.jar";

    QString _subjectIDSavedAtScanTime;
    QStringList _selectedScansLoadedFromNotes;
    bool _reverseOrderScans=true;
    bool _refreshStudy=false;
    QVector<scanType> _scans;
    savedSettings _savedSettings;
    QSettings _savedQSettings;

    CommandOptions _inputOptions;
    CommandLineParseResult parseCommandLine(QStringList commandLine);

    QSplitter *_centralWidget;
    QVBoxLayout *_mainLayout;
    QVBoxLayout *_notesLayout;
    QTabWidget *_tabs;
    QStatusBar *_statusBar;
    QVector<QTextEdit *> _noteBox;
    QAction *_showNotesAction;

    QWidget *_helpTool;
    QTextBrowser *_helpBrowser;
    int _helpPageIndex=0;

    // Scans page
    QLineEdit *_subjectID;
    QLabel *_subjectScanTimes;
    QTableWidget *_scanTable;
    QPushButton *_viewUsingFastmap;

    // upload page
    QLineEdit *_dataBaseDir;
    QLineEdit *_uploadTempDir;
    QLineEdit *_javaExe;
    QLineEdit *_IDAUploader;

    // clean page
    QPushButton *_cleanUnselectedFIDs;
    QPushButton *_cleanUnselectedDICOMs;
    QPushButton *_cleanAllFIDs;
    QPushButton *_cleanAllDICOMs;
    QLabel *_totalSizeSubDirs;

    void createGUI();
    QWidget *createScanPanel();
    QWidget *createUploadPanel();
    QWidget *createCleanPanel();
    void readQSettings();
    void writeQSettings();
    QString readFileTextArgument(QString fileName, QString parameterName);
    void readSubjectVariables();
    void readSmoothing(int which);
    void scanDirectories();
    void writeAllNotes();
    void loadNotes();
    void loadHelp(int whichTab);
    int whichTabName(QString name);
    QString getParameterString(QString fileName, QString parameterName);
    int getVisuCoreOrientation(QString fileName);
    iPoint4D getImageDimensions(QString dirname);
    void getSequenceTimes(QString fileName, scanType &scan);
    QString concatenateSelectedScanString();
    void readStudyDirectory();

    void updateCleaningList();

    void findFIDs(bool all, bool remove);
    void findDICOMs(bool all, bool remove);
    void findAllFiles();

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void readCommandLine();
private slots:
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
    void changedHighlightScan(int row, int column);
    void headerClicked(int column);
    void tableItemClicked(QTableWidgetItem *item);
    void openNewSubject();
    void refreshStudy();

    void prepareDataForUpload();
    void runIDUploader();

    void aboutApp();
    void exitApp();
    void changedPage(int index);
    void writeSubjectVariables();

    inline void cleanUnselelectedFIDs() {findFIDs(false, true);
                                         findFIDs(false, false); findFIDs(true, false); findAllFiles();}
    inline void cleanAllFIDs()          {findFIDs(true, true);
                                         findFIDs(false, false); findFIDs(true, false); findAllFiles();}

    inline void cleanUnselelectedDICOMs() {findDICOMs(false, true);
                                           findDICOMs(false, false); findDICOMs(true, false); findAllFiles();}
    inline void cleanAllDICOMs()          {findDICOMs(true, true);
                                           findDICOMs(false, false); findDICOMs(true, false); findAllFiles();}

};

#endif // MRIPROCESS_H
