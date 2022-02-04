#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <QImage>
#include <QSettings>
#include <searchpattern.h>
// #include <qsvm.h>
#include <datastructure.h>
#include <qsvm.h>

using namespace cv;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // system parameter
    struct SYSTEMPARAM {
        int inputMode; // 0: file, 1: uvc, 2: ipcam
        bool inputUVCIpCamSaveVideo;
        QTimer *frameTimer;
        QString systemBufferPath;
        QString systemPath;
        QString system3PartyPath;
        QString sampleINIfn;
        QString sfpINIfn;
        QSettings *sampleINISetting;
        QSettings *sfpIniSetting;
        bool rotate0degree;
        bool rotate90degree;
        bool rotate270degree;
        // search
        bool firstSearch;
        // search information
        QString mi1pFn;
        QString mi2pFn;
        QString mi2pVideoFn;
        QString vecFn;
        bool p1searchDone;
        bool p2searchDone;
        // 2nd play
        QString secondPlayContolString;
        QString secondPlayProgram;
        int NumOfPassAnalysis;
        // svm
        QString svmString;
    } sp;

    SAI sai, orgSai;

    struct SAMPLEINI {
        int x;
        int y;
        int pixelDiam;
        int realDiam;
    } sampleini,gemsample;

    struct SECONDFRAMEPLAYINI {
        int demoMode;
        QString programName;
        int MoveTo1;
        QString videoFn1;
        QString videoFn2;
        QString SVMString;
        int showSVMResult;
    } sfp;

    struct VIDEOPARAM {
        QString vfn;
        bool videoOpened;
        bool videoPlaying;
        VideoCapture vcap;
        Mat buffer;
        Mat tbuffer;
        int orgW;
        int orgH;
        int TorgW;
        int TorgH;
        int dispW;
        int dispH;
        int NumOfFrames;
        int frameRate;
        double downRatio;
        int currentFrameCnt;

        // IplImage *tbuffer;
        IplImage *dispFrame;
        IplImage *currFrame;
        IplImage *prevFrame;
        IplImage *orgSelectedCurrentImage;
        double playingFrameRate;

        VideoWriter ov;
        QString ovFn;
    } vp;


    MOINFO *moinfo;

    int updatePosX, updatePosY;
    SearchPattern search;
    qSVM svm;
    double classfyResult;

    int systemInitial();
    int videoInitial();
    void MessageOut(QString str, int method,int level);
    QImage convertIplImageToQImageAndShow(IplImage *orgImage,int LabelSelect);
    IplImage* MatToIplImage(Mat buff);
    bool fileExit(QString fn);
    bool createSampleINIFile();
    bool settingSampleINIFile();
    bool saveSampleINIFile();
    void drawCross(int x0,int y0, QPixmap *myPix,int drawFrame,QColor color);
    void drawCircle(int x,int y, int radius,int canvas,QColor color);
    void updateSai(int rx,int ry,int pixelDiam);
    void searchNext();
    void draw1Ptrajectory();
    void secondPassAnalysis();
    uchar *orgTemplate;
    int liftType;
    int findTheNearestFarPointBodyFrame(MI2 *mi2temp,int beginFrame, int endFrame,int minmax);
    int findSmallestNonZeroY(MI2 *mi2temp,int beginFrame, int endFrame,int minmax);
    double calculateSpeedBetweenFrames(
            MI2 *mi2temp,int beginFrame, int endFrame, double scale,double frameRate);
    int returnFrameRate();
    IplImage *rotateFrame(); //Mat mFrame);
    MI2 *mi2;
    int firstPassAnalysisForUVCAndIPCam();
    bool createSfpIniFile();
    bool settingSfpIniFile();
    bool saveSFPIniFile();


    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void inputOpen();
    void inputPlay();
    void inputStop();
    void input2pAnalysis();
    void frameRendering();
    void sampleTry();
    void sampleSave();
    void quickDeRecordMode();
    void quickRecordMode();
    void quickSecondFrameShow();
    void quickGoodDemo();
    void systemClose();

private:
    Ui::MainWindow *ui;

protected:
    void mouseReleaseEvent(QMouseEvent *e);
};

#endif // MAINWINDOW_H
