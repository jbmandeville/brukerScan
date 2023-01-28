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
//    QString _jipProcess         = "/usr/pubsw/packages/jip/bin/Linux-x86_64/jip";
//    QString _scriptDirectory    = "/space/deltabp/1/users/public/script/analyze-petmr/";
    QString _fastmapProcess     = "/Users/jbm/QtApps/build-FM-Desktop_Qt_5_13_0_clang_64bit-Release/FM.app/Contents/MacOS/FM";
    QString _jipProcess         = "/Users/jbm/unix/jip-new/jip/bin/Linux-x86_64/jip";
    QString _scriptDirectory    = "/Users/jbm/unix/script/analyze-fm";
    QString _findsessionProcess = "/usr/pubsw/bin/findsession";
    QString _dicomDumpProcess   = "/usr/bin/dcmdump";

    enum tabPages
    {
        page_download,
        page_anatomy,
        page_fMRI,
        page_clean,
        page_size
    };
    enum scanCategory
    {
        category_scout,
        category_anatomy,
        category_EPI,
        category_MRAC,
        category_VIBE,
        category_UTE,
        category_UNKNOWN
    };
    struct FourDFile
    {
        QString name;       // "epi/004/raw.nii"
        iPoint4D dim;       // x,y,z,t
        dVector timeTags;   // seconds[dim.t]
        sVector timeText;   // string[dim.t]
    };
    struct programVariables
    {
        QString downloadID;
        QString downloadPath;
        QString subjectID;
    } _savedToDisk;
    struct downloadScan
    {
        QString scanNumber;
        QString scanNumberNew;  // "1" --> "001"
        QString sequenceName;
        iPoint4D dim;
        scanCategory category;
        QString categoryName;
        bool isAnatomy=false;
        bool isMRAC=false;
        bool existsOnDisk;
        bool selectedForDownload;
        bool selectedForCleaning;
    };
    struct savedSettings
    {
        // Image Window
        QByteArray imageWindowGeometry;
        QByteArray browserWindowGeometry;
        double fmSmoothing=0.;
    };

    QWidget *_centralWidget;
    QTabWidget *_tabs;
    QWidget *_downLoadPage;
    QWidget *_anatomyPage;
    QWidget *_fmriPage;
    QWidget *_cleanPage;
    QStatusBar *_statusBar;
    QVector<QTextEdit *> _noteBox;
    QAction *_showNotesAction;

    QTextBrowser *_outputBrowser;
    QWidget *_helpTool;
    QTextBrowser *_helpBrowser;
    int _helpPageIndex=0;

    // Radiobuttons
    QRadioButton *_radioButtonSiemensHuman;
    QRadioButton *_radioButtonSiemensNonHuman;
    QRadioButton *_radioButtonBruker;

    // Download page
    QLineEdit *_subjectIDDownload;
    QComboBox *_downloadIDBox;
    QComboBox *_downloadPathBox;
    QListWidget *_scanItemsBox;
    QVector<QListWidgetItem> _scanItems;
    QPushButton *_querySubjectButton;
    QPushButton *_generateScanListButton;
    QPushButton *_readAvailableScanList;
    QPushButton *_downloadDataButton;
    QGroupBox *_queryDownloadGroupBox;

    // Anatomy page
    QGroupBox *_freeSurferGroupBox;
    QStringList _FastmapMSTemplateDirectories;
    QComboBox *_anatomyDirBox; // "003 004"
    QLineEdit *_subjectIDFreeSurfer;
    QWidget *_convert2dseqWidget;
    QPushButton *_generateNifti;
    QCheckBox *_reorderRaw;
    QComboBox *_anatomyFileBox;       // "raw.nii or "brain.nii"
    QComboBox *_anatomyTemplateDirectory; // multi-subject template directory
    QPushButton *_runFreeSurferButton;
    QPushButton *_alignAnatomyButton;
    QPushButton *_extractFreeSurferOverlaysButton;
    QLineEdit *_smoothingAnatomy;

    // fMRI page
    QListWidget *_fMRIRunItemBox;
    QVector<QListWidgetItem> _fMRIRunItems;
    QComboBox *_fMRITemplateDirBox; // an existing directory
    QComboBox *_fMRIFileBox;          // "raw.nii or "mrangeLabelc.nii"
    QLineEdit *_fMRIMCRange;              // e.g. 1-10
    QPushButton *_doEverythingEPIButton;
    QPushButton *_alignEPIToAnatomyButton;
    QPushButton *_resliceEPIButton;
    QPushButton *_motionCorrectEPIButton;
    QPushButton *_alignEPIButton;
    QLineEdit *_smoothingfMRI;

    // clean page
    QListWidget *_cleanScanTypesBox;
    QVector<QListWidgetItem> _cleanScanTypeItems;
    QPushButton *_cleanDICOMs;
    QPushButton *_cleanNII_auxilliary;  // MR*.nii, test.nii, matchingMRI-rs.nii, matchingMRI-mc.nii
    QPushButton *_cleanNII_mc;          // mc.nii: this needs to be kept in order to change smoothing
    QPushButton *_cleanNII_raw;         // raw.nii
    QLabel *_totalSizeSubDirs;

    // non-GUI variables
    QVector<downloadScan> _scans;
    QVector<FourDFile> _fMRIFiles;
    iPoint4D _dimEPITemplate;
    i2Matrix _matchingEPI;  // [_petFile.dim.t][list of pairs (fMRI file,time point)]
    savedSettings _savedSettings;
    QSettings _savedQSettings;

    void createDownloadPage();
    void createAnatomyPage();
    void createfMRIPage();
    void createCleanPage();
    void openedAnatomyPage();
    void openedfMRIPage();
    void openedCleanPage();
    void readQSettings();
    void writeQSettings();
    void outputDownloadList();
    void getSubjectNameFromFreeDir();
    void readAvailableScanList();
    void readUnpackLog();
    QString readFileTextArgument(QString fileName, QString parameterName);
    bool enableDownloadData();
    bool enableTransferDirectory();
    void reformatAcquisitionTimes(downloadScan scan);
    void readSubjectVariables();
    void readSmoothing(int which);
    void writeAllNotes();
    void loadNotes();
    void loadHelp(int whichTab);
    int whichTabName(QString name);
    void setupScanTypes();

    void updateAnatomyFileName();
    void setTemplate();

    void populateEPIFileNameBox();
    void setDefaultIndexEPIFileNameBox();
    void updateEPIFileNameBox(QString fileName);
    void writeJipCommandFileForMCAveraging(QString &comFileName);
    QString readJipCommandFileForMCAveraging();
    void createEPITemplate(bool MC);
    void motionCorrectEPI();

    void updateCleaningList();

    QString getDimensions(QString fileName, iPoint4D &dim);
    QString twoDigits(short time);
    void writeJipCommandFileForMatchingMRI();

    inline bool anatomyFileExists(QString fileName) {return anatomyFileExists(_anatomyDirBox->currentText(),fileName);}
    inline bool epiFileExists(QString fileName)     {return epiFileExists(_fMRITemplateDirBox->currentText(),fileName);}
    bool anatomyFileExists(QString dirName, QString fileName);
    bool epiFileExists(QString dirName, QString fileName);
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

    void enableEPIActionButtons();
    void enableAnatomyActionButtons();

    void findDICOMs(bool remove);
    void findAuxFiles(bool remove);
    void findMCFiles(bool remove);
    void findRawFiles(bool remove);
    void findAllFiles();

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void dataOriginChanged();

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
        if ( show ) _helpTool->show();
        else        _helpTool->hide();
    }
    void showNone();
    void showNotes(bool show);
    void helpGoBackward();
    void helpGoForward();

    inline void changedAnatomyFileName(int indexInBox) {enableAnatomyActionButtons();}
    void changedAnatomyDirName(int indexInBox);
    inline void generateBrukerNiftiAnatomical() {generateBrukerNifti(true);}
    void generateBrukerNifti(bool anatomy);

    inline void changedDownloadIDBox(int indexInBox)   {_downloadPathBox->setCurrentIndex(indexInBox);}
    inline void changedDownloadPathBox(int indexInBox) {_downloadIDBox->setCurrentIndex(indexInBox);}
    void finishedGeneratingScanList(int exitCode, QProcess::ExitStatus exitStatus);
    void finishedDownloadData(int exitCode, QProcess::ExitStatus exitStatus);

    void queryDownloadPaths();
    void generateScanList();
    void downloadData();
    void aboutApp();
    void exitApp();
    void changedPage(int index);
    void alignAnatomyToTemplate();
    void finishedFMAnatomyAlignment(int exitCode, QProcess::ExitStatus exitStatus );
    void extractFreeSurferOverlays();
    void finishedExtractOverlays(int exitCode, QProcess::ExitStatus exitStatus );
    void runFreeSurfer();
    void finishedRunFreeSurfer(int exitCode, QProcess::ExitStatus exitStatus );
    void displayAnatomy();
    void writeSubjectVariables();

    void doEverthingEPI();
    void resliceEPI();
    inline void createTemplateAndMotionCorrectEPI() {createEPITemplate(true);}
    inline void alignEPIToAnatomy() {createEPITemplate(false);}
    inline void motionCorrectEPI(int exitCode, QProcess::ExitStatus exitStatus ) {motionCorrectEPI();}
    void alignEPIToReslicedAnatomy(int exitCode, QProcess::ExitStatus exitStatus);
    void resliceAnatomyToEPI(int exitCode, QProcess::ExitStatus exitStatus);
    void alignEPI();
    void changefMRITemplateDirectory(int indexInBox);
    void changedfMRIFileName(int indexInBox);
    void finishedFMResliceEPI(int exitCode, QProcess::ExitStatus exitStatus );
    void finishedLinkResliceEPI(int exitCode, QProcess::ExitStatus exitStatus );
    void finishedFMAlignEPI(int exitCode, QProcess::ExitStatus exitStatus );
    void finishedAlignEPIToReslicedAnatomy(int exitCode, QProcess::ExitStatus exitStatus);
    void finishedGenerateBrukerNiftiAnatomical(int exitCode, QProcess::ExitStatus exitStatus );
    void finishedGenerateBrukerNiftiFunctional(int exitCode, QProcess::ExitStatus exitStatus );
    void changedfMRIRunCheckBox(QListWidgetItem* item);
    void changedDownloadScanCheckBox(QListWidgetItem *item);
    void changedSmoothingAnatomy();
    void changedSmoothingfMRI();

    void finishedMotionCorrectEPI(int exitCode, QProcess::ExitStatus exitStatus);
    void displayEPI();

    void changedScanTypeCheckBox(QListWidgetItem *item);
    inline void cleanDICOMFiles()     {findDICOMs(true);         findDICOMs(false);         findAllFiles();}
    inline void cleanMCFiles()        {findMCFiles(true);        findMCFiles(false);        findAllFiles();}
    inline void cleanAuxNIIFiles()    {findAuxFiles(true);       findAuxFiles(false);       findAllFiles();}
    inline void cleanRawNIIFiles()    {findRawFiles(true);       findRawFiles(false);       findAllFiles();}

};

#endif // MRIPROCESS_H
