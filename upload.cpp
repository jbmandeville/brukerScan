#include <QtWidgets>
#include <QFileInfo>
#include "brukerScan.h"

QWidget *MainWindow::createUploadPanel()
{
    FUNC_ENTER;
    auto *uploadPanel = new QWidget();

    auto *dataBaseLabel = new QLabel("Data base dir");
    _dataBaseDir = new QLineEdit(_dataBaseDirectory,uploadPanel);
    auto *tempDirLabel = new QLabel("Temp upload dir");
    _uploadTempDir = new QLineEdit(_uploadTempDirectory,uploadPanel);
    auto *javaLabel = new QLabel("Java exe");
    _javaExe = new QLineEdit(_javaExeLocation,uploadPanel);
    auto *IDALabel = new QLabel("IDA uploader");
    _IDAUploader = new QLineEdit(_IDAUploaderFile,uploadPanel);

    _dataBaseDir->setEnabled(false);
    _uploadTempDir->setEnabled(false);
    _javaExe->setEnabled(false);
    _IDAUploader->setEnabled(false);

    auto *prepareData = new QPushButton("prepare data");
    auto *runIDAJava  = new QPushButton("run IDA");

    connect(prepareData, SIGNAL(pressed()), this, SLOT(prepareDataForUpload()));
    connect(runIDAJava,  SIGNAL(pressed()), this, SLOT(runIDUploader()));

    auto *locationLayout = new QGridLayout();
    locationLayout->addWidget(dataBaseLabel,0,0);
    locationLayout->addWidget(_dataBaseDir,0,1);

    locationLayout->addWidget(tempDirLabel,1,0);
    locationLayout->addWidget(_uploadTempDir,1,1);
    locationLayout->addWidget(prepareData,1,2);

    locationLayout->addWidget(javaLabel,2,0);
    locationLayout->addWidget(_javaExe,2,1);

    locationLayout->addWidget(IDALabel,3,0);
    locationLayout->addWidget(_IDAUploader,3,1);
    locationLayout->addWidget(runIDAJava,3,2);

    uploadPanel->setLayout(locationLayout);
    return uploadPanel;
}

void MainWindow::runIDUploader()
{
    FUNC_ENTER;
    QString exe = _javaExeLocation;

    QStringList arguments;
    arguments.append("-jar");
    arguments.append(_IDAUploaderFile);

    FUNC_INFO << exe << arguments;
    auto *process = new QProcess();
    process->startDetached(exe,arguments);
}

void MainWindow::prepareDataForUpload()
{
    _centralWidget->setEnabled(false);
    // first clean the source directory
    QDir uploadBase(_uploadTempDirectory);
    QStringList subDirList = uploadBase.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    FUNC_INFO << "subDirList" << subDirList;
    for (int jList=0; jList<subDirList.count(); jList++)
    {
        QDir subDir(_uploadTempDirectory + subDirList.at(jList));
        FUNC_INFO << "remove old dir" << subDir.absolutePath();
        subDir.removeRecursively();
    }

    // Now create a directory at the location
    QDir thisDir = QDir::currentPath();
    subDirList = thisDir.absolutePath().split("/");
    QString thisDirName = subDirList.last();
    uploadBase.mkdir(thisDirName);
    QDir uploadDir(_uploadTempDirectory+thisDirName);

    for (int jScan=0; jScan<_scans.size(); jScan++)
    {
        scanType scan = _scans.at(jScan);

        // Make the destination subdirectory
        if ( scan.selectedAsImportant )
        {
            QString numberedDestintionPath = uploadDir.absolutePath() + "/" + scan.scanName;
            uploadDir.mkdir(numberedDestintionPath);
            // Find the dicoms and copy them to the destination
            QString sourceDir = "./" + scan.scanName + "/pdata/1/dicom";
            QDir dicomDir(sourceDir);
            FUNC_INFO << "dicomDir" << dicomDir.absolutePath();
            QStringList const fileList = dicomDir.entryList( {"*.dcm"}, QDir::Files | QDir::NoSymLinks);
//            QStringList const fileList = dicomDir.entryList( {"MR*"}, QDir::Files | QDir::NoSymLinks);
            FUNC_INFO << "fileList" << fileList;
            for (int jFile=0; jFile<fileList.size(); jFile++)
            {
                QString sourceName = sourceDir + "/" + fileList.at(jFile);
                QString destName = numberedDestintionPath + "/" + fileList.at(jFile);
                FUNC_INFO << "copy source" << sourceName << "to dest" << destName;
                QFile sourceFile(sourceName);
                sourceFile.copy(destName);
            }
        }
    }
    _centralWidget->setEnabled(true);
}
