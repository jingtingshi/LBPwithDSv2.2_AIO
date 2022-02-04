#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <opencv2/opencv.hpp>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <qfiledialog.h>
#include <QMessageBox>
#include <QImage>
#include <QApplication>
#include <QTimer>
#include <QDateTime>
#include <QSettings>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPainter>
#include <searchpattern.h>
#include <stdio.h>
#include <stdlib.h>
#include <datastructure.h>
#include <QMdiSubWindow>
#include <QDesktopWidget>

// ipcam source:
// rtsp://admin:admin@192.168.10.83:554/0
// rtsp://admin:aa123456@192.168.2.91:554/play1.sdp
// rtsp://admin:admin@192.168.10.85:554/profile1/media.smp
FILE *oofp;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);    
    systemInitial();    

}

MainWindow::~MainWindow()
{
    delete ui;    
}

// --------------------------------------------------------------- //
int MainWindow::systemInitial()
{
    vp.videoOpened = false;
    vp.videoPlaying = false;

    sp.frameTimer = new QTimer();
    connect(sp.frameTimer,SIGNAL(timeout()),this,SLOT(frameRendering()));

    sp.systemBufferPath = "c:/w/QtLBPwDSAIO_SystemBuffer/";
    sp.system3PartyPath = "c:/w/QtLBPwDSAIO_System3Party/";


    // generate sampling ini
    sp.systemPath = QApplication::applicationDirPath();
    sp.sampleINIfn = sp.system3PartyPath+"lbpd2aio_sampling.ini";
    if(!fileExit(sp.sampleINIfn))
        createSampleINIFile();    
    settingSampleINIFile();

    sp.sfpINIfn = sp.system3PartyPath+"lbp2aio_sfp.ini";
    if(!fileExit(sp.sfpINIfn))
        createSfpIniFile();
    settingSfpIniFile();

    if(ui->cbFullScreen->isChecked())
        this->showFullScreen();


    /*
    sp.rotate90degree = false;
    if(ui->cbRotate90degree->isChecked())
        sp.rotate90degree = true;    
        */
    if(ui->rbRotate0degree->isChecked())
    {
        sp.rotate0degree=true;
        sp.rotate90degree = false;
        sp.rotate270degree=false;
    }
    if(ui->rbRotate90degree->isChecked())
    {
        sp.rotate0degree=false;
        sp.rotate90degree = true;
        sp.rotate270degree=false;
    }
    if(ui->rbRotate270degree->isChecked())
    {
        sp.rotate0degree=false;
        sp.rotate90degree = false;
        sp.rotate270degree=true;
    }

    QDesktopWidget *desktop = QApplication::desktop();
    this->setGeometry(desktop->screenGeometry(0));

    if(ui->cbRecordMode->isChecked())
    {
        ui->cbConnectSVM->setChecked(false);
        ui->cbContinues2pAnalysis->setChecked(false);
        ui->cbEarlyStopWhenSearchDone->setChecked(false);
        ui->cbPlayResultOn2ndScreen->setChecked(false);
        ui->cbTrackingFirstPass->setChecked(false);
        ui->cbTrackingSecondPass->setChecked(false);
    }

    return 1;
}
// --------------------------------------------------------------- //
bool MainWindow::createSfpIniFile()
{
    FILE *fp;
    std::string str = (const char*)sp.sfpINIfn.toLocal8Bit();
    if((fp=fopen(str.c_str(),"w"))==NULL)
        return false;
    fprintf(fp,"[SFP]\n");
    fprintf(fp,"demoMode=1\n");
    fprintf(fp,"programName=LBPqt2ndScreenDisplay.exe\n");
    if(ui->rb2ndScreenEndAtLastFrame->isChecked())
        fprintf(fp,"MoveTo1=1\n");
    if(ui->rb2ndScreenMoveTo1->isChecked())
        fprintf(fp,"MoveTo1=0\n");
    fprintf(fp,"videoFn1=c:/sample/goodliftDemo/w_2020-06-16-14-27-49.avi\n");
    fprintf(fp,"videoFn2=c:/sample/goodliftDemo/w_2020-06-16-14-27-49.avi_output.avi\n");
    fprintf(fp,"svm=-1\n");
    if(ui->cb2ndScreenShowSVMResult->isChecked())
        fprintf(fp,"showSVMResult=1\n");
    else
        fprintf(fp,"showSVMResult=0\n");
    fclose(fp);
    return 1;
}
// --------------------------------------------------------------- //
bool MainWindow::settingSfpIniFile()
{
    // get ini
    sp.sfpIniSetting = new QSettings(sp.sfpINIfn,QSettings::IniFormat);
    sfp.demoMode = sp.sfpIniSetting->value("/SFP/demoMode").toInt();
    sfp.programName = sp.sfpIniSetting->value("/SFP/programName").toString();
    sfp.MoveTo1 = sp.sfpIniSetting->value("/SFP/MoveTo1").toInt();
    sfp.videoFn1 = sp.sfpIniSetting->value("/SFP/videoFn1").toString();
    sfp.videoFn2 = sp.sfpIniSetting->value("/SFP/videoFn2").toString();
    sfp.SVMString = sp.sfpIniSetting->value("/SFP/svm").toString();
    sfp.showSVMResult = sp.sfpIniSetting->value("/SFP/showSVMResult").toInt();

    // set ui
    ui->cb2ndScreenShowSVMResult->setChecked(sfp.showSVMResult);
    if(sfp.MoveTo1==0)
        ui->rb2ndScreenMoveTo1->setChecked(true);
    else
        ui->rb2ndScreenEndAtLastFrame->setChecked(true);

    return true;
}
// --------------------------------------------------------------- //
bool MainWindow::saveSFPIniFile()
{
    FILE *fp;
    std::string str = (const char*)sp.sfpINIfn.toLocal8Bit();
    if((fp=fopen(str.c_str(),"w"))==NULL)
        return false;
    fprintf(fp,"[SFP]\n");
    fprintf(fp,"demoMode=0\n");
    fprintf(fp,"programName=LBPqt2ndScreenDisplay.exe\n");
    if(ui->rb2ndScreenEndAtLastFrame->isChecked())
        fprintf(fp,"MoveTo1=1\n");
    if(ui->rb2ndScreenMoveTo1->isChecked())
        fprintf(fp,"MoveTo1=0\n");
    str = (const char*)vp.vfn.toLocal8Bit();
    fprintf(fp,"videoFn1=%s\n",str.c_str());
    str = (const char*)sp.mi2pVideoFn.toLocal8Bit();
    fprintf(fp,"videoFn2=%s\n",str.c_str());
    str = (const char*)sp.svmString.toLocal8Bit();
    fprintf(fp,"svm=%s\n",str.c_str());
    if(ui->cb2ndScreenShowSVMResult->isChecked())
        fprintf(fp,"showSVMResult=1\n");
    else
        fprintf(fp,"showSVMResult=0\n");
    fclose(fp);
    return 1;
}
// --------------------------------------------------------------- //
bool MainWindow::createSampleINIFile()
{
    FILE *fp;
    std::string str = (const char*)sp.sampleINIfn.toLocal8Bit();
    if((fp = fopen(str.c_str(),"w"))==NULL)
        return false;
    fprintf(fp,"[sampleINI]\n");
    fprintf(fp,"x=0\n");
    fprintf(fp,"y=0\n");
    fprintf(fp,"pixelDiam=45\n");
    fprintf(fp,"realDiam=45\n");
    fclose(fp);
    return true;
}
// --------------------------------------------------------------- //
bool MainWindow::saveSampleINIFile()
{
    FILE *fp;
    std::string str = (const char*)sp.sampleINIfn.toLocal8Bit();
    if((fp = fopen(str.c_str(),"w"))==NULL)
        return false;
    fprintf(fp,"[sampleINI]\n");
    fprintf(fp,"x=%d\n",sampleini.x);
    fprintf(fp,"y=%d\n",sampleini.y);
    fprintf(fp,"pixelDiam=%d\n",sampleini.pixelDiam);
    fprintf(fp,"realDiam=%d\n",sampleini.realDiam);
    fclose(fp);
    return true;
}
// --------------------------------------------------------------- //
bool MainWindow::settingSampleINIFile()
{
    sp.sampleINISetting = new QSettings(sp.sampleINIfn,QSettings::IniFormat);
    sampleini.x = sp.sampleINISetting->value("/sampleINI/x").toInt();
    sampleini.y = sp.sampleINISetting->value("/sampleINI/y").toInt();
    sampleini.pixelDiam = sp.sampleINISetting->value("/sampleINI/pixelDiam").toInt();
    sampleini.realDiam = sp.sampleINISetting->value("/sampleINI/realDiam").toInt();

    // set ui
    ui->edtSamplingX->setText(QString::number(sampleini.x));
    ui->edtSamplingY->setText(QString::number(sampleini.y));
    ui->edtPixelDiam->setText(QString::number(sampleini.pixelDiam));
    ui->edtRealDiam->setText(QString::number(sampleini.realDiam));

    // get geometory
    gemsample.realDiam = sampleini.realDiam;

    return true;
}
// --------------------------------------------------------------- //
bool MainWindow::fileExit(QString fn)
{
    QFileInfo fi(fn);
    if(fi.isFile())
        return true;
    return false;
}
// --------------------------------------------------------------- //
int MainWindow::videoInitial()
{
    std::string str;        
    vp.frameRate = returnFrameRate();
    /*
    if(ui->cbRotate90degree->isChecked())
        sp.rotate90degree = true;
    else
        sp.rotate90degree = false;
    */
    if(ui->rbRotate0degree->isChecked())
    {
        sp.rotate0degree=true;
        sp.rotate90degree = false;
        sp.rotate270degree=false;
    }
    if(ui->rbRotate90degree->isChecked())
    {
        sp.rotate0degree=false;
        sp.rotate90degree = true;
        sp.rotate270degree=false;
    }
    if(ui->rbRotate270degree->isChecked())
    {
        sp.rotate0degree=false;
        sp.rotate90degree = false;
        sp.rotate270degree=true;
    }
    // if(sp.rotate90degree)
    if(sp.rotate90degree || sp.rotate270degree)
    {
        vp.orgW = vp.vcap.get(CV_CAP_PROP_FRAME_HEIGHT);
        vp.orgH = vp.vcap.get(CV_CAP_PROP_FRAME_WIDTH);
    }
    else
    {
        vp.orgW = vp.vcap.get(CV_CAP_PROP_FRAME_WIDTH);
        vp.orgH = vp.vcap.get(CV_CAP_PROP_FRAME_HEIGHT);
    }
    if(sp.inputMode==0)
        vp.NumOfFrames = vp.vcap.get(CV_CAP_PROP_FRAME_COUNT);

    // calculate scale
    if(vp.orgH>=480 || vp.orgW>480)
    {
        if(vp.orgW >= vp.orgH)
        {
            // landscape
            vp.downRatio = 480.0 / vp.orgW;
        }
        else
        {
            // straight
            vp.downRatio = 480.0 / vp.orgH;
        }
    }
    else {
        vp.downRatio = 1.0;
    }
    vp.dispH = vp.orgH * vp.downRatio;
    vp.dispW = vp.orgW * vp.downRatio;

    // lifttype
    liftType = 0;
    if(ui->rbTypeSnatch->isChecked())
        liftType = 1;
    if(ui->rbTypeCJ->isChecked())
        liftType = 2;

    // generate frame
    // if(sp.rotate90degree)
    if(sp.rotate90degree || sp.rotate270degree)
        vp.tbuffer = cvCreateImage(cvSize(vp.orgH,vp.orgW),IPL_DEPTH_8U,3);
    vp.currFrame = cvCreateImage(cvSize(vp.orgW,vp.orgH),IPL_DEPTH_8U, 3); // vp.buffer.depth(),vp.buffer.channels());
    vp.prevFrame = cvCreateImage(cvSize(vp.orgW,vp.orgH),IPL_DEPTH_8U, 3);
    vp.dispFrame = cvCreateImage(cvSize(vp.dispW,vp.dispH),IPL_DEPTH_8U, 3); // vp.buffer.depth(),vp.buffer.channels());

    // adjust geometry
    ui->labelShowPic1->setGeometry(ui->labelShowPic1->x(),ui->labelShowPic1->y(),
                                   vp.dispW,vp.dispH);
    ui->labelShowPic2->setGeometry(ui->labelShowPic1->x()+vp.dispW+10,ui->labelShowPic1->y(),
                                   vp.dispW,vp.dispH);
    gemsample.x = ui->labelShowPic2->x()+vp.dispW+10;
    gemsample.y = ui->labelShowPic1->y();
    ui->labelShowPic3->setGeometry(gemsample.x,gemsample.y,gemsample.realDiam,gemsample.realDiam);

    // set framecnt
    vp.currentFrameCnt = 0;
    ui->labelCurrentFrameCnt->setText(QString::number(vp.currentFrameCnt));

    vp.videoPlaying = false;

    // set player timing
    if(ui->rbInputPlayTimer->isChecked())
        vp.playingFrameRate = 1000.0 / vp.frameRate;
    if(ui->rbInputPlayLoop->isChecked())
        vp.playingFrameRate = 0;

    if(sp.inputMode>0)// && sp.NumOfPassAnalysis>0)
    {
        QDateTime current_date_time = QDateTime::currentDateTime();
        vp.ovFn = sp.systemBufferPath+"w_"+current_date_time.toString("yyyy-MM-dd-hh-mm-ss")+".avi";
        str = (const char*)vp.ovFn.toLocal8Bit();
        vp.ov.open(str.c_str(),CV_FOURCC('D','I','V','3'),vp.frameRate,cvSize(vp.orgW,vp.orgH),1);
        if(!vp.ov.isOpened())
            MessageOut("video file create fail!",1,0);
        else
            MessageOut("video output checked, forget not press stop",1,0);
    }

    // for search
    sp.firstSearch = true;
    moinfo = new struct MOTIONINFORMATION[5000];

    // set params tabs -> enable=true
    ui->tabParams->setEnabled(true);

    // compose mi vn
    if(ui->cbSaveTrackingResultsData->isChecked())
    {
        if(sp.inputMode==0)
        {
            // file mode
            sp.mi1pFn = vp.vfn+".mv";
            sp.mi2pFn = vp.vfn+"_2p.mv";
            sp.vecFn = vp.vfn+"_vec.jpg";
        }
        else
        {
            // uvc or ipcam
            sp.mi1pFn = vp.ovFn+".mv";
            sp.mi2pFn = vp.ovFn+"_2p.mv";
            sp.vecFn = vp.ovFn+"_vec.jpg";
        }
    }
    // compose 2pvideo fn
    if(ui->cbSaveTrackingResultsVideo->isChecked())
    {
        if(sp.inputMode==0)
        {
            sp.mi2pVideoFn = vp.vfn+"_output.avi";
        }
        else
        {
            sp.mi2pVideoFn = vp.ovFn+"_output.avi";
        }
    }
    // connect svm
    if(ui->cbConnectSVM->isChecked())
    {
        if(sp.inputMode==0)
        {
            // file mode
            sp.vecFn = vp.vfn+"_vec.jpg";
        }
        else
        {
            // uvc or ipcam
            sp.vecFn = vp.ovFn+"_vec.jpg";
        }
        // qsvm param
        svm.param.svmMidLevel = ui->edtVecCompMidLevel->text().toInt();
        svm.param.svmNormFact = ui->edtVecCompNormFactor->text().toInt();
        svm.param.sw = ui->edtVecCompSW->text().toInt();
        svm.param.sh = ui->edtVecCompSH->text().toInt();
        svm.param.cjw = ui->edtVecCompCJW->text().toInt();
        svm.param.cjh = ui->edtVecCompCJH->text().toInt();
        svm.param.NumOfNodes = 0;
        svm.param.saveVector = 0;
        if(ui->cbVecCompSaveSample->isChecked())
            svm.param.saveVector = 1;
        svm.param.vecFn = sp.vecFn;
        svm.param.liftType = liftType;
        svm.param.svmClassfierXmlFn = sp.system3PartyPath+"/svmClassfier_newVector.xml";
        // str = (const char*)svm.param.svmClassfierXmlFn.toLocal8Bit();
        // // check xml exist
        // FILE *fp=fopen(str.c_str(),"r");
        if(fileExit(svm.param.svmClassfierXmlFn))
        {
            MessageOut("VideoInitial: svm xml open fail!",1,1);
        }
        else
        {
            MessageOut("VideoInitial: svm xml checked!",1,0);
        }
    }

    if(ui->cbRecordMode->isChecked())
    {
        ui->cbConnectSVM->setChecked(false);
        ui->cbContinues2pAnalysis->setChecked(false);
        ui->cbEarlyStopWhenSearchDone->setChecked(false);
        ui->cbPlayResultOn2ndScreen->setChecked(false);
        ui->cbTrackingFirstPass->setChecked(false);
        ui->cbTrackingSecondPass->setChecked(false);
    }

    sp.p1searchDone = false;
    sp.p2searchDone = false;


    return 1;
}
// --------------------------------------------------------------- //
IplImage* MainWindow::MatToIplImage(Mat buff)
{
    IplImage *img = cvCreateImage(buff.size(),IPL_DEPTH_8U,3);
    img->imageData = (char*)buff.data;
    return img;
}
// --------------------------------------------------------------- //
QImage MainWindow::convertIplImageToQImageAndShow(IplImage *orgImage,int LabelSelect)
{
    QImage tmp;
    IplImage *image;
    if(LabelSelect<3)
    {
        image = cvCreateImage(cvSize(vp.dispW,vp.dispH),vp.currFrame->depth,vp.currFrame->nChannels);
        cvResize(orgImage,image,CV_INTER_LINEAR);
    }
    else
    {
        image = cvCreateImage(cvSize(orgImage->width,orgImage->height),orgImage->depth,orgImage->nChannels);
        cvCopy(orgImage,image);
    }

    if(image->nChannels==1)
    {
       tmp = QImage((unsigned char*)image->imageData, image->width, image->height,
                      image->widthStep, QImage::Format_Indexed8);
    }
    else
    {
        cvConvertImage(image,image,CV_CVTIMG_SWAP_RB);
        tmp = QImage((unsigned char*)image->imageData, image->width, image->height,
                      image->widthStep, QImage::Format_RGB888);
    }

    if(LabelSelect==1)
        ui->labelShowPic1->setPixmap(QPixmap::fromImage(tmp));
    if(LabelSelect==2)
        ui->labelShowPic2->setPixmap(QPixmap::fromImage(tmp));
    if(LabelSelect==3)
        ui->labelShowPic3->setPixmap(QPixmap::fromImage(tmp));

    return tmp;
}
// --------------------------------------------------------------- //
void MainWindow::MessageOut(QString str, int method,int level)
{
    if(method==1)
    {
        QListWidgetItem *t = new QListWidgetItem();
        // level 0 for normal, level 1 for error, level 2 for good news
        if(level==0)
        {
            t->setText(str);
        }
        else if(level==1)
        {
            t->setBackgroundColor(Qt::red);
            t->setTextColor(Qt::blue);
            t->setText(str);
        }
        else if(level==2)
        {
            t->setBackgroundColor(Qt::darkGreen);
            t->setTextColor(Qt::white);
            t->setText(str);
        }

        ui->listStdMsg->addItem(t);
    }
    if(method==2)
    {
        QMessageBox::about(NULL, "LBPwithDS", str);
    }
}

// --------------------------------------------------------------- //
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    int px,py;
    px = event->pos().x() - ui->labelShowPic1->x();
    py = event->pos().y() - ui->labelShowPic1->y() - ui->mainToolBar->height();
    ui->labelSamplePos->setText(QString::number(px)+","+QString::number(py));

    if(ui->cbSamplingMode->isChecked())
    {
        int pixelDiam = ui->edtPixelDiam->text().toInt();

        updateSai(px,py,pixelDiam);

        const QPixmap *buff = ui->labelShowPic1->pixmap();
        QImage buffQImg(buff->toImage());
        QPixmap myPix(QPixmap::fromImage(buffQImg));
        drawCross(px,py,&myPix,1,Qt::red);
        drawCircle(px,py,pixelDiam,1,Qt::blue);
        QString stmp = "S: Org("+QString::number(orgSai.x)+","+QString::number(orgSai.y)+")";
        MessageOut(stmp,1,0);
    }
}
// --------------------------------------------------------------- //
void MainWindow::updateSai(int rX,int rY, int pixelDiam)
{
    ui->edtSamplingX->setText(QString::number(rX));
    ui->edtSamplingY->setText(QString::number(rY));

    sai.x = rX;
    sai.y = rY;
    sai.diam = pixelDiam;
    sai.x0 = sai.x - pixelDiam/2;
    sai.y0 = sai.y - pixelDiam/2;
    sai.x1 = sai.x + pixelDiam/2;
    sai.y1 = sai.y + pixelDiam/2;

    orgSai.x = sai.x / vp.downRatio;
    orgSai.y = sai.y / vp.downRatio;
    orgSai.x0 = sai.x0 / vp.downRatio;
    orgSai.y0 = sai.y0 / vp.downRatio;
    orgSai.x1 = sai.x1 / vp.downRatio;
    orgSai.y1 = sai.y1 / vp.downRatio;
    orgSai.diam = sai.diam / vp.downRatio;

    // update ui
    ui->labelShowPic3->setGeometry(ui->labelShowPic3->x(),ui->labelShowPic3->y(),orgSai.diam,orgSai.diam);
}

// --------------------------------------------------------------- //
void MainWindow::drawCross(int x0,int y0, QPixmap *myPix,int drawFrame,QColor color)
{
    QPainter myPainter(myPix);
    QPen myPen(color, 3);
    myPainter.setPen(myPen);
    myPainter.drawLine(QPoint(x0-4,y0),QPoint(x0+4,y0));
    myPainter.drawLine(QPoint(x0,y0-4),QPoint(x0,y0+4));
    if(drawFrame==1)
        ui->labelShowPic1->setPixmap(*myPix);
    if(drawFrame==2)
        ui->labelShowPic2->setPixmap(*myPix);
    if(drawFrame==3)
        ui->labelShowPic3->setPixmap(*myPix);

}
// --------------------------------------------------------------- //
void MainWindow::drawCircle(int x,int y, int diam,int canvas,QColor color)
{
    const QPixmap *buff;
    if(canvas==1)
        buff = ui->labelShowPic1->pixmap();
    else if(canvas==2)
        buff = ui->labelShowPic2->pixmap();

    int x0 = x - diam/2;
    int y0 = y - diam/2;

    QImage buffImg(buff->toImage());
    QPixmap myPix(QPixmap::fromImage(buffImg));
    QPainter myPainter(&myPix);
    QPen myPen(color, 2);
    myPainter.setPen(myPen);
    myPainter.drawEllipse(x0,y0,diam,diam);
    if(canvas==1)
        ui->labelShowPic1->setPixmap(myPix);
    else if(canvas==2)
        ui->labelShowPic2->setPixmap(myPix);
    else if(canvas==3)
        ui->labelShowPic3->setPixmap(myPix);
}
// --------------------------------------------------------------- //
int MainWindow::returnFrameRate()
{
    if(sp.inputMode==0)
        return vp.vcap.get(CV_CAP_PROP_FPS);

    if(!ui->cbAutocheckFrameRate->isChecked())
        return ui->edtInputFPSforWriteVideo->text().toInt();

    int testingFrames;
    int startTimeMs = QDateTime::currentMSecsSinceEpoch();
    int endTimeMs;
    int fps = 30;
    double duration;

    if(sp.inputMode==1)
        testingFrames = 600;
    else if(sp.inputMode==2)
        testingFrames = 30;
    for(int i=0;i<testingFrames;i++)
    {
        vp.vcap.read(vp.buffer);
        ui->labelCurrentFrameCnt->setText(QString::number(i));
        qApp->processEvents();
    }

    endTimeMs = QDateTime::currentMSecsSinceEpoch();
    duration = (endTimeMs - startTimeMs)/1000;

    fps = testingFrames / duration;

    if(fps<15)
        fps = 15;
    if(fps>=60)
        fps = 60;
    ui->edtInputFPSforWriteVideo->setText(QString::number(fps));
    MessageOut("Frame rate:"+QString::number(fps),1,0);
    return fps;
}
// --------------------------------------------------------------- //
IplImage *MainWindow::rotateFrame() //Mat mFrame)
{
    // Mat tbuffer = cvCreateImage(cvSize(vp.orgW,vp.orgH),IPL_DEPTH_8U,3);
    transpose(vp.buffer,vp.tbuffer);
    if(sp.rotate90degree)
        flip(vp.tbuffer,vp.tbuffer,0);
    if(sp.rotate270degree)
        flip(vp.tbuffer,vp.tbuffer,2);

    return MatToIplImage(vp.tbuffer);
}
// --------------------------------------------------------------- //
void MainWindow::quickRecordMode()
{
    ui->cbRecordMode->setChecked(true);
    if(ui->cbRecordMode->isChecked())
    {
        ui->cbConnectSVM->setChecked(false);
        ui->cbContinues2pAnalysis->setChecked(false);
        ui->cbEarlyStopWhenSearchDone->setChecked(false);
        ui->cbPlayResultOn2ndScreen->setChecked(false);
        ui->cbTrackingFirstPass->setChecked(false);
        ui->cbTrackingSecondPass->setChecked(false);
    }
}
// --------------------------------------------------------------- //
void MainWindow::quickDeRecordMode()
{
    ui->cbRecordMode->setChecked(false);
    if(!ui->cbRecordMode->isChecked())
    {
        ui->cbConnectSVM->setChecked(true);
        ui->cbContinues2pAnalysis->setChecked(true);
        ui->cbEarlyStopWhenSearchDone->setChecked(true);
        ui->cbPlayResultOn2ndScreen->setChecked(true);
        ui->cbTrackingFirstPass->setChecked(true);
        ui->cbTrackingSecondPass->setChecked(true);
    }
}
// --------------------------------------------------------------- //
void MainWindow::quickSecondFrameShow()
{
    // get ini file
    settingSfpIniFile();

    sp.secondPlayProgram = sfp.programName;
    sp.secondPlayContolString = sp.system3PartyPath+sp.secondPlayProgram+" ";
    if(ui->rb2ndScreenMoveTo1->isChecked())
        sp.secondPlayContolString += "0 ";
    if(ui->rb2ndScreenEndAtLastFrame->isChecked())
        sp.secondPlayContolString += "1 ";
    sp.secondPlayContolString = sp.secondPlayContolString+sfp.videoFn1+" "+sfp.videoFn2+" ";
    if(ui->cb2ndScreenShowSVMResult->isChecked())
        sp.secondPlayContolString += ("AI"+sfp.SVMString);

    MessageOut("sp:"+sp.secondPlayContolString,1,0);
    std::string str = (const char*)sp.secondPlayContolString.toLocal8Bit();
    system(str.c_str());
}
// --------------------------------------------------------------- //
void MainWindow::systemClose()
{
    this->close();
}
// --------------------------------------------------------------- //
/* --------------------- system function -------------------------- */
// --------------------------------------------------------------- //
void MainWindow::inputOpen()
{
    QFileDialog *d = new QFileDialog();
    std::string str;

    sp.NumOfPassAnalysis = -1;
    if(ui->rbInputFileMode->isChecked())
    {
        sp.inputMode = 0;
        if(d->exec()==QDialog::Accepted)
        {
            vp.vfn = d->selectedFiles()[0];
            str = (const char*)vp.vfn.toLocal8Bit();
            if(!vp.vcap.open(str.c_str()))
            {
                MessageOut("file open fail!",2,0);
                vp.videoOpened = false;
                return;
            }
            vp.videoOpened = true;
        }
        else
        {
            MessageOut("user abort or video fail!",1,0);
            vp.videoOpened = false;
            return;
        }
    }
    if(ui->rbInputUVC->isChecked())
    {
        int uvcNo = ui->edtUVCNo->text().toInt();
        sp.inputMode = 1;
        vp.vcap.open(uvcNo);
        if(!vp.vcap.isOpened())
        {
            MessageOut("no uvc device!",2,0);
            vp.videoOpened = false;
            return;
        }
        vp.NumOfFrames = 0;
        vp.videoOpened = true;
        sp.NumOfPassAnalysis = 0;
    }
    if(ui->rbInputIpcam->isChecked())
    {
        sp.inputMode = 2;
        vp.vfn = ui->edtIpcamUrl->text();
        // vp.vcap.open(0);
        str = (const char*)vp.vfn.toLocal8Bit();
        vp.vcap.open(str.c_str());
        if(!vp.vcap.isOpened())
        {
            MessageOut("ip cam no connect!",2,0);
            vp.videoOpened = false;
            return;
        }
        vp.NumOfFrames = 0;
        vp.videoOpened = true;
        sp.NumOfPassAnalysis = 0;
    }


    if(!vp.videoOpened)
        return;

    MessageOut("video initialing...",1,0);
    if(!videoInitial())
    {
        MessageOut("initial error!",1,1);
        return;
    }

    // show first frame;
    vp.vcap.read(vp.buffer);
    // if(sp.rotate90degree)
    if(sp.rotate90degree || sp.rotate270degree)
        vp.currFrame = rotateFrame(); //vp.buffer);
    else
        vp.currFrame = MatToIplImage(vp.buffer);
    convertIplImageToQImageAndShow(vp.currFrame,1);
    MessageOut("inputOpen: video opened/connected!",1,0);

    // draw current sample
    if(ui->cbSampleDrawCurrentSample->isChecked()&&(sampleini.x!=0&&sampleini.y!=0))
    {
        drawCircle(sampleini.x,sampleini.y,sampleini.pixelDiam,1,Qt::lightGray);
    }


}
// --------------------------------------------------------------- //
void MainWindow::inputPlay()
{
    if(!vp.videoOpened)
    {
        MessageOut("video sould be opened or connected!",1,0);
        return;
    }

    if(!vp.videoPlaying)
    {
        // uvc and ipcam, record only
        if(sp.inputMode>0)
            quickRecordMode();

        vp.videoPlaying = true;
        updateSai(ui->edtSamplingX->text().toInt(),ui->edtSamplingY->text().toInt(),
                  ui->edtPixelDiam->text().toInt());
        // without update, pick up original sample
        if(!ui->cbSearchWithUpdate->isChecked() && ui->cbTrackingFirstPass->isChecked())
        {
            uchar r;
            orgTemplate = new uchar[orgSai.diam*orgSai.diam];
            IplImage *gf = cvCreateImage(cvSize(vp.orgW,vp.orgH),IPL_DEPTH_8U,1);
            cvCvtColor(vp.currFrame,gf,CV_RGB2GRAY);
            for(int y=0;y<orgSai.diam;y++)
                for(int x=0;x<orgSai.diam;x++)
                {
                    r = gf->imageData[(y+orgSai.y0)*gf->width+(x+orgSai.x0)];
                    orgTemplate[y*orgSai.diam+x] = r;
                }
        }
        ui->tabParams->setEnabled(false);
        sp.frameTimer->start(vp.playingFrameRate);
        /*
        liftType = 0;
        if(ui->rbTypeSnatch->isChecked())
            liftType = 1;
        if(ui->rbTypeCJ->isChecked())
            liftType = 2;
        */
        // set tables, enable -> false

    }
    else
    {
        vp.videoPlaying = false;
        sp.frameTimer->stop();
        /*
        // save mi1file
        if(ui->cbSaveTrackingResultsData->isChecked()&&ui->cbTrackingFirstPass->isChecked())
        {
            std::string str = (const char*)sp.mi1pFn.toLocal8Bit();
            FILE *fp = fopen(str.c_str(),"wb");
            for(int i=0;i<vp.currentFrameCnt;i++)
                fwrite(&moinfo[i],sizeof(MOINFO),1,fp);
            fclose(fp);
        }
        // continues 2nd play
        if(ui->cbTrackingSecondPass->isChecked() && sp.p1searchDone)
            secondPassAnalysis();
        // connect to qSVM
        if(ui->cbConnectSVM->isChecked() && sp.p2searchDone)
        {
            ui->labelTrackingStatus->setText("-- SVM --");
            classfyResult = 0.0;
            svm.param.NumOfFrameInMI1 = vp.currentFrameCnt;
            svm.qsvmMain(mi2,&classfyResult);
            MessageOut("AI"+QString::number(classfyResult),1,2);
        }
        // display on 2nd screen
        // LBPqt2ndScreenDisplay playingMethod, fn1, fn2, resultString
        // playingMethod 0: move to 1, 1: stop at last, 2: terminate program
        if(ui->cb2ndScreenShowSVMResult->isChecked() && sp.p1searchDone)
        {
            sp.secondPlayProgram = "LBPqt2ndScreenDisplay.exe";
            sp.secondPlayContolString = sp.system3PartyPath+sp.secondPlayProgram+" ";
            if(ui->rb2ndScreenMoveTo1->isChecked())
                sp.secondPlayContolString += "0 ";
            if(ui->rb2ndScreenEndAtLastFrame->isChecked())
                sp.secondPlayContolString += "1 ";
            QString fn1, fn2;
            if(sp.inputMode==0)
                fn1 = vp.vfn;
            else
                fn1 = vp.ovFn;
            fn2 = sp.mi2pVideoFn;
            sp.secondPlayContolString = sp.secondPlayContolString+fn1+" "+fn2+" ";
            if(ui->cbConnectSVM->isChecked() && ui->cb2ndScreenShowSVMResult->isChecked())
            {
                QString resultString;
                resultString = "AI:goodlift"; // +QString::number(classfyResult);
                sp.secondPlayContolString += resultString;
            }
            MessageOut("sp:"+sp.secondPlayContolString,1,0);
            std::string str = (const char*)sp.secondPlayContolString.toLocal8Bit();
            system(str.c_str());
        }
        */

        // if(ui->rb2ndScreenMoveTo1->isChecked())
        //    sp.secondPlayProgram
    }
}
// --------------------------------------------------------------- //
int MainWindow::firstPassAnalysisForUVCAndIPCam()
{
    std::string strTmp;
    sp.NumOfPassAnalysis = 1;
    strTmp = (const char*)vp.vfn.toLocal8Bit();
    if(!vp.vcap.open(strTmp.c_str()))
    {
        MessageOut("firstPass(): video file open fail!",2,0);
        vp.videoOpened = false;
        return 0;
    }
    vp.videoOpened = true;
    MessageOut("firstPass(): video opened:"+vp.ovFn,1,0);

    // set rotate to 0 for video mode

    if(sp.rotate90degree || sp.rotate270degree)
        ui->rbRotate0degree->setChecked(true);
    sp.inputMode=0;

    if(!videoInitial())
    {
        MessageOut("firstPass(): initial error!",1,2);
        return 0;
    }

    qApp->processEvents();
    vp.vcap.read(vp.buffer);
    vp.currFrame = MatToIplImage(vp.buffer);
    convertIplImageToQImageAndShow(vp.currFrame,1);

    // draw current sample
    if(ui->cbSampleDrawCurrentSample->isChecked()&&(sampleini.x!=0&&sampleini.y!=0))
    {
        drawCircle(sampleini.x,sampleini.y,sampleini.pixelDiam,1,Qt::lightGray);
    }

    ui->labelTrackingStatus->setText("-- 1 pass --");

    // gather sample information
    updateSai(ui->edtSamplingX->text().toInt(),ui->edtSamplingY->text().toInt(),
              ui->edtPixelDiam->text().toInt());
    // without update, pick up original sample
    if(!ui->cbSearchWithUpdate->isChecked() && ui->cbTrackingFirstPass->isChecked())
    {
        uchar r;
        orgTemplate = new uchar[orgSai.diam*orgSai.diam];
        IplImage *gf = cvCreateImage(cvSize(vp.orgW,vp.orgH),IPL_DEPTH_8U,1);
        cvCvtColor(vp.currFrame,gf,CV_RGB2GRAY);
        for(int y=0;y<orgSai.diam;y++)
            for(int x=0;x<orgSai.diam;x++)
            {
                r = gf->imageData[(y+orgSai.y0)*gf->width+(x+orgSai.x0)];
                orgTemplate[y*orgSai.diam+x] = r;
            }
    }
    ui->tabParams->setEnabled(false);

    int mvYdropCount = 0;
    for(int i=0;i<vp.NumOfFrames-3;i++)
    {
        vp.currentFrameCnt = i;
        if(i>0)
            cvCopy(vp.currFrame,vp.prevFrame);
        ui->labelCurrentFrameCnt->setText(QString::number(i));
        vp.vcap.set(CV_CAP_PROP_POS_FRAMES,i);
        vp.vcap.read(vp.buffer);
        if(sp.rotate90degree)
        {
            // vp.currFrame = rotateFrame(); //vp.buffer);
            transpose(vp.buffer,vp.tbuffer);
            flip(vp.tbuffer,vp.tbuffer,0);
            vp.currFrame->imageData = (char*)vp.tbuffer.data;
            // vp.currFrame = MatToIplImage(vp.tbuffer);
        }
        else if(sp.rotate270degree)
        {
            transpose(vp.buffer,vp.tbuffer);
            flip(vp.tbuffer,vp.tbuffer,2);
            vp.currFrame->imageData = (char*)vp.tbuffer.data;
        }
        else
        {
            // IplImage *img = cvCreateImage(buff.size(),IPL_DEPTH_8U,3);
            // img->imageData = (char*)buff.data;
            vp.currFrame->imageData = (char*)vp.buffer.data;
            // vp.currFrame = MatToIplImage(vp.buffer);
        }
        convertIplImageToQImageAndShow(vp.currFrame,1);
        if(ui->cbTrackingFirstPass->isChecked()&&i>0)
        {
            if(!sp.p1searchDone)
                searchNext();
            if(ui->cbDrawPositionMark->isChecked() || ui->cbDrawTrajectory->isChecked())
                draw1Ptrajectory();
            if(i>4 && ui->cbEarlyStopWhenSearchDone->isChecked())
            {
                mvYdropCount = 0;
                if(moinfo[i-3].mvY>10) mvYdropCount++;
                if(moinfo[i-2].mvY>10) mvYdropCount++;
                if(moinfo[i-1].mvY>10) mvYdropCount++;
                if(moinfo[i].mvY>10) mvYdropCount++;

                if(mvYdropCount==3)
                {
                    sp.p1searchDone = true;
                    break;
                }
            }
        }
        qApp->processEvents();
    }
    sp.p1searchDone = true;
    return 1;

}
// --------------------------------------------------------------- //
void MainWindow::inputStop()
{    

    if(!vp.videoOpened)
    {
        MessageOut("video sould be opened or connected!",1,0);
        return;
    }

    MessageOut("stop",1,0);

    if(vp.videoPlaying)
    {
        vp.videoPlaying = false;
        sp.frameTimer->stop();
    }

    if(sp.inputMode>0)
    {
        vp.ov.release();
    }
    if(!ui->cbInputUVCIpCamSaveVideo->isChecked()&&sp.inputMode>0)
        QFile::remove(vp.ovFn);

    vp.vcap.release();

    if(sp.inputMode>0)
        vp.vfn = vp.ovFn;

    // RZvp.vfn = "C:/w/QtLBPwDSAIO_SystemBuffer/w_2020-06-18-12-06-31.avi";

    /* post process */
    // 1st. pass for uvc/ipcam
    bool processNoError = false;
    if(sp.inputMode>0)
        quickDeRecordMode();
    sp.NumOfPassAnalysis = 1;
    if(!firstPassAnalysisForUVCAndIPCam())
    {
        MessageOut("firstPass():fail!",1,2);
        return;
    }

    // save mi1file
    if(ui->cbSaveTrackingResultsData->isChecked()&&ui->cbTrackingFirstPass->isChecked())
    {
        std::string str = (const char*)sp.mi1pFn.toLocal8Bit();
        FILE *fp = fopen(str.c_str(),"wb");
        for(int i=0;i<vp.currentFrameCnt;i++)
            fwrite(&moinfo[i],sizeof(MOINFO),1,fp);
        fclose(fp);
    }

    // continues 2nd play
    sp.NumOfPassAnalysis = 2;
    if(ui->cbTrackingSecondPass->isChecked() && sp.p1searchDone)
        secondPassAnalysis();
    // connect to qSVM
    if(ui->cbConnectSVM->isChecked() && sp.p2searchDone)
    {
        ui->labelTrackingStatus->setText("-- SVM --");
        classfyResult = 0.0;
        svm.param.NumOfFrameInMI1 = vp.currentFrameCnt;
        svm.qsvmMain(mi2,&classfyResult);
        MessageOut("AI:"+QString::number(classfyResult),1,2);
    }
    // display on 2nd screen
    // LBPqt2ndScreenDisplay playingMethod, fn1, fn2, resultString
    // playingMethod 0: move to 1, 1: stop at last, 2: terminate program
    if(ui->cb2ndScreenShowSVMResult->isChecked() && sp.p1searchDone)
    {
        sp.secondPlayProgram = "LBPqt2ndScreenDisplay.exe";
        sp.secondPlayContolString = sp.system3PartyPath+sp.secondPlayProgram+" ";
        if(ui->rb2ndScreenMoveTo1->isChecked())
            sp.secondPlayContolString += "0 ";
        if(ui->rb2ndScreenEndAtLastFrame->isChecked())
            sp.secondPlayContolString += "1 ";
        QString fn1, fn2;
        if(sp.inputMode==0)
            fn1 = vp.vfn;
        else
            fn1 = vp.ovFn;
        fn2 = sp.mi2pVideoFn;
        sp.secondPlayContolString = sp.secondPlayContolString+fn1+" "+fn2+" ";
        if(ui->cbConnectSVM->isChecked() && ui->cb2ndScreenShowSVMResult->isChecked())
        {
            sp.svmString = "goodlift"; // +QString::number(classfyResult);
            QString resultString;
            resultString = "AI:"+sp.svmString;
            sp.secondPlayContolString += resultString;
        }
        MessageOut("sp:"+sp.secondPlayContolString,1,0);
        std::string str = (const char*)sp.secondPlayContolString.toLocal8Bit();
        system(str.c_str());
    }

    saveSFPIniFile();

    MessageOut("videoClose: done!",1,2);

    ui->tabParams->setEnabled(true);
}
// --------------------------------------------------------------- //
void MainWindow::input2pAnalysis()
{   
}
// --------------------------------------------------------------- //
void MainWindow::sampleTry()
{
    if(!vp.videoOpened || vp.videoPlaying)
        return;
    if(!ui->cbSamplingMode->isChecked())
        return;

    sai.x = ui->edtSamplingX->text().toInt();
    sai.y = ui->edtSamplingY->text().toInt();
    int pixelDiam = ui->edtPixelDiam->text().toInt();
    sai.diam = pixelDiam;

    if(sai.x < pixelDiam/2 || sai.y<pixelDiam/2)
        return;

    updateSai(sai.x,sai.y,sai.diam);

    drawCircle(sai.x,sai.y,sai.diam,1,Qt::blue);

    // generate sampling pic
    IplImage *roiTemp = cvCreateImage(cvSize(orgSai.diam,orgSai.diam),IPL_DEPTH_8U,3);
    cvSetImageROI(vp.currFrame,cvRect(orgSai.x0,orgSai.y0,orgSai.diam,orgSai.diam));
    cvCopy(vp.currFrame,roiTemp);
    cvResetImageROI(vp.currFrame);
    convertIplImageToQImageAndShow(roiTemp,3);
}
// --------------------------------------------------------------- //
void MainWindow::sampleSave()
{
    if(!vp.videoOpened || vp.videoPlaying)
        return;
    if(!ui->cbSamplingMode->isChecked())
        return;

    // redraw
    convertIplImageToQImageAndShow(vp.currFrame,1);
    drawCircle(sai.x,sai.y,sai.diam,1,Qt::darkGreen);

    // save ini
    sampleini.x = sai.x;
    sampleini.y = sai.y;
    sampleini.pixelDiam = sai.diam;
    sampleini.realDiam = ui->edtRealDiam->text().toInt();
    saveSampleINIFile();

    MessageOut("sample done!",1,0);
    ui->cbSamplingMode->setChecked(false);
}
// --------------------------------------------------------------- //
void MainWindow::frameRendering()
{
    vp.currentFrameCnt++;

    // test max frame
    /*
    oofp = fopen("c:/w/pages.txt","w");
    fprintf(oofp,"%d\n",vp.currentFrameCnt);
    fclose(oofp);
    */

    if(sp.inputMode==0)
    {
        if(vp.currentFrameCnt>=(vp.NumOfFrames-2))
        {            
            if(!ui->cbContinues2pAnalysis->isChecked())
            {
                MessageOut("Last frame",1,0);
                vp.currentFrameCnt = 0;
            }
            else
            {
                // save mi1file
                if(ui->cbSaveTrackingResultsData->isChecked()&&ui->cbTrackingFirstPass->isChecked())
                {
                    std::string str = (const char*)sp.mi1pFn.toLocal8Bit();
                    FILE *fp = fopen(str.c_str(),"wb");
                    for(int i=0;i<vp.currentFrameCnt;i++)
                        fwrite(&moinfo[i],sizeof(MOINFO),1,fp);
                    fclose(fp);
                }
            }
            // stop play
            inputPlay();
            /*
            vp.videoPlaying = false;
            sp.frameTimer->stop();
            */
        }
        vp.vcap.set(CV_CAP_PROP_POS_FRAMES,vp.currentFrameCnt);
    }
    if(vp.currentFrameCnt>0)
        cvCopy(vp.currFrame,vp.prevFrame);
    try
    {
        vp.vcap.read(vp.buffer);
        // if(sp.rotate90degree)
        if(sp.rotate90degree)
        {
            // vp.currFrame = rotateFrame(); //vp.buffer);
            transpose(vp.buffer,vp.tbuffer);
            flip(vp.tbuffer,vp.tbuffer,0);
            vp.currFrame->imageData = (char*)vp.tbuffer.data;
            // vp.currFrame = MatToIplImage(vp.tbuffer);
        }
        else if(sp.rotate270degree)
        {
            transpose(vp.buffer,vp.tbuffer);
            flip(vp.tbuffer,vp.tbuffer,2);
            vp.currFrame->imageData = (char*)vp.tbuffer.data;
        }
        else
        {
            // IplImage *img = cvCreateImage(buff.size(),IPL_DEPTH_8U,3);
            // img->imageData = (char*)buff.data;
            vp.currFrame->imageData = (char*)vp.buffer.data;
            // vp.currFrame = MatToIplImage(vp.buffer);
        }
    }
    catch(...)
    {
        MessageOut("rf: vcap read fail! #"+QString::number(vp.currentFrameCnt),1,1);
    }

    convertIplImageToQImageAndShow(vp.currFrame,1);
    ui->labelCurrentFrameCnt->setText(QString::number(vp.currentFrameCnt));
    if(ui->cbShowPreviousFrame->isChecked())
        convertIplImageToQImageAndShow(vp.prevFrame,2);
    if(sp.inputMode>0)
        vp.ov.write(vp.currFrame);

    if(ui->cbTrackingFirstPass->isChecked() || sp.inputMode==0)  // uvc or ipcam record only
    {
        if(!sp.p1searchDone)
            searchNext();
        if(ui->cbDrawPositionMark->isChecked() || ui->cbDrawTrajectory->isChecked())
            draw1Ptrajectory();

        ui->labelTrackingStatus->setText("--1 pass--");
        // check barbell down
        if(vp.currentFrameCnt>4)
        {
            int mvYdropCount = 0;
            if(moinfo[vp.currentFrameCnt-3].mvY>10) mvYdropCount++;
            if(moinfo[vp.currentFrameCnt-2].mvY>10) mvYdropCount++;
            if(moinfo[vp.currentFrameCnt-1].mvY>10) mvYdropCount++;
            if(moinfo[vp.currentFrameCnt].mvY>10) mvYdropCount++;

            if(mvYdropCount==3)
            {
                sp.p1searchDone = true;
                if(ui->cbEarlyStopWhenSearchDone->isChecked())
                {
                    inputPlay();
                }
            }
        }
    }
}
// --------------------------------------------------------------- //
void MainWindow::searchNext()
{
    int mvX0,mvY0;
    int posX, posY;
    int sr = ui->edtSR->text().toInt();
    const int sampleMethod = 0;
    QImage tmp(orgSai.diam,orgSai.diam,QImage::Format_RGB888);
    QImage updateSample(orgSai.diam,orgSai.diam,QImage::Format_RGB888);
    double consumeTimeEachSearch;

    if(sp.firstSearch)
    {
        // initial posX, posY
        posX = orgSai.x0;
        posY = orgSai.y0;
        sp.firstSearch = false;
    }
    else
    {
        posX = updatePosX;
        posY = updatePosY;        
    }

    // method
    if(ui->cbSearchWithUpdate->isChecked())
    {
        search.fullsearch(vp.prevFrame,vp.currFrame,posX,posY,&mvX0,&mvY0,
                          sampleMethod,sr,orgSai.diam,&updateSample,&tmp,
                          vp.currentFrameCnt,&consumeTimeEachSearch);
    }
    else
    {
        search.fullsearchWithoutUpdate(vp.currFrame,posX,posY,sampleMethod,
                                       sr,orgSai.diam,&mvX0,&mvY0,&updateSample,
                                       &tmp,vp.currentFrameCnt,&consumeTimeEachSearch,
                                       orgTemplate);
    }

    // update mvx, mvy
    updatePosX = posX + mvX0;
    updatePosY = posY + mvY0;

    // save mi
    moinfo[vp.currentFrameCnt-1].frameCnt = vp.currentFrameCnt-1;
    moinfo[vp.currentFrameCnt-1].currX = posX;
    moinfo[vp.currentFrameCnt-1].currY = posY;
    moinfo[vp.currentFrameCnt-1].nextX = updatePosX;
    moinfo[vp.currentFrameCnt-1].nextY = updatePosY;
    moinfo[vp.currentFrameCnt-1].mvX = mvX0;
    moinfo[vp.currentFrameCnt-1].mvY = mvY0;
    moinfo[vp.currentFrameCnt-1].eachTime = consumeTimeEachSearch;
    moinfo[vp.currentFrameCnt-1].totalTime = 0;
    moinfo[vp.currentFrameCnt-1].searched = true;
    moinfo[vp.currentFrameCnt-1].orgSai = orgSai;
    moinfo[vp.currentFrameCnt-1].liftType = liftType;
    moinfo[vp.currentFrameCnt-1].orgWidth = vp.orgW;
    moinfo[vp.currentFrameCnt-1].orgHeight = vp.orgH;
}
// --------------------------------------------------------------- //
void MainWindow::draw1Ptrajectory()
{
    const QPixmap *buff = ui->labelShowPic1->pixmap();
    QImage buffImg(buff->toImage());
    QPixmap myPix(QPixmap::fromImage(buffImg));
    QPainter myPainter(&myPix);
    QPen myPen;

    int tx0,ty0,tx1,ty1;
    for(int i=1;i<vp.currentFrameCnt-1;i++)
    {
        tx0 = moinfo[i-1].currX*vp.downRatio+sai.diam/2;
        ty0 = moinfo[i-1].currY*vp.downRatio+sai.diam/2;
        tx1 = moinfo[i].currX*vp.downRatio+sai.diam/2;
        ty1 = moinfo[i].currY*vp.downRatio+sai.diam/2;
        // draw trajectory
        if(ui->cbDrawTrajectory->isChecked())
        {
            myPen.setWidth(2);
            myPen.setColor(Qt::green);
            myPainter.setPen(myPen);
            myPainter.drawLine(QPoint(tx0,ty0),QPoint(tx1,ty1));
        }
        // draw circle
        if(ui->cbDrawPositionMark->isChecked())
        {
            myPen.setWidth(1);
            myPen.setColor(Qt::yellow);
            myPainter.setPen(myPen);
            myPainter.drawEllipse(QPoint(tx1,ty1),2,2);
        }
    }
    ui->labelShowPic1->setPixmap(myPix);
}
// --------------------------------------------------------------- //
void MainWindow::secondPassAnalysis()
{
    if(!vp.videoOpened && !ui->cbTrackingSecondPass->isChecked() && sp.p1searchDone)
        return;

    ui->labelTrackingStatus->setText("-- 2 pass --");

    // FILE *mi1fp;
    // MOINFO *mi1;

    int NumOfFramesInMi1;
    // SAI spaSai;
    // int liftType;
    double realScale;
    double frameRate;
    int firstZeroFrameNumber;
    // int orgW, orgH;
    // QString ivfn;
    CvCapture *cap;

    int segPfnum[13];
    int segP99fnum;
    // QString qstrTmp;
    std::string strTmp;

    // 2pass initial
    // spaSai = orgSai;
    NumOfFramesInMi1 = vp.currentFrameCnt;
    realScale = ui->edtRealDiam->text().toDouble() / orgSai.diam;
    frameRate = vp.frameRate;
    strTmp = (const char*)vp.vfn.toLocal8Bit();
    cap = cvCreateFileCapture(strTmp.c_str());
    ui->labelTrackingStatus->setText("--2nd pass--");
    strTmp = (const char*)sp.mi2pFn.toLocal8Bit();
    FILE *mv2fp = fopen(strTmp.c_str(),"wb");
    mi2 = new MI2[NumOfFramesInMi1];

    // copy mi to mi2
    for(int i=0;i<NumOfFramesInMi1;i++)
    {
        mi2[i].frameCnt = moinfo[i].frameCnt;
        mi2[i].currX = moinfo[i].currX;
        mi2[i].currY = moinfo[i].currY;
        mi2[i].mvX = moinfo[i].mvX;
        mi2[i].mvY = moinfo[i].mvY;
        mi2[i].liftType = liftType;
        mi2[i].normX = 0.0;
        mi2[i].normY = 0.0;
        mi2[i].scaleFactor = realScale;
        mi2[i].searched = 0;
        mi2[i].VerticalOffset = 0.0;
        mi2[i].speedFromPreviousFrame = -1.0;
        mi2[i].segmentationPoint = -1;
        mi2[i].acclerationRanking = -1;
        mi2[i].speedRanking = -1;
        mi2[i].acclerationFromPreviousFrame = 0.0;
        mi2[i].speedFromPreviousSegment = -1.0;
        mi2[i].acclerationFromPreviousSegment = -1.0;
    }

    // normalize to (0,0)
    // 1. get position fro vertical axile
    bool firstYsmallerThanZero = false;
    int firstFrameNum;
    int orgVertX0, orgVertY0;
    for(int i=0;i<NumOfFramesInMi1;i++)
        if(mi2[i].mvY<0&&!firstYsmallerThanZero)
        {
            firstFrameNum = i;
            mi2[i].segmentationPoint = 0;
            mi2[i+1].segmentationPoint = 1;
            segPfnum[0] = i;
            segPfnum[1] = i+1;
            orgVertX0 = mi2[i].currX;
            orgVertY0 = mi2[i].currY;
            firstZeroFrameNumber = i;
            firstYsmallerThanZero = true;
            break;
        }

    // 2. normalize to (0,0)
    for(int i=0;i<NumOfFramesInMi1;i++)
    {
        mi2[i].normX = (mi2[i].currX - orgVertX0)*realScale;
        mi2[i].normY = (mi2[i].currY - orgVertY0)*realScale;
    }

    // calculate speed for frame
    if(ui->cb2PSpeedForFrame->isChecked())
    {
        for(int i=1;i<NumOfFramesInMi1-1;i++)
        {
            double movingDistance = sqrt(pow(mi2[i+1].currX-mi2[i-1].currX,2)+
                                         pow(mi2[i+1].currY-mi2[i-1].currY,2));
            mi2[i].speedFromPreviousFrame = movingDistance*realScale/frameRate;
        }
    }

    // calculate accleration for frame
    if(ui->cb2PAcclerationForFrame->isChecked())
    {
        if(!ui->cb2PSpeedForFrame->isChecked())
        {
            MessageOut("2P: accleration need speed for frame",1,0);
        }
        else
        {
            for(int i=2;i<NumOfFramesInMi1-1;i++)
            {
                double speedDistance = mi2[i-2].speedFromPreviousFrame - mi2[i-1].speedFromPreviousFrame;
                mi2[i].acclerationFromPreviousFrame = speedDistance / frameRate;
            }
        }
    }

    // segmentation

    // 1. find p4, highest point
    bool sp4found = false;
    for(int i=firstZeroFrameNumber;i<NumOfFramesInMi1;i++)
    {
        if(mi2[i].mvY>0&&!sp4found)
        {
            segPfnum[4] = i-1;
            mi2[i-1].segmentationPoint = 4;
            sp4found = true;
            break;
        }
    }
    // 2-1. find p2
    int mvX1 = 1;
    double magnitude = 0.0;
    bool sp2found = false;
    for(int i=segPfnum[1]+2;i<segPfnum[4];i++)
    {
        double cosTheta = (double)(mi2[i].mvX)/sqrt(pow(mi2[i].mvX,2)+pow(mi2[i].mvY,2));
        double theta = acos(cosTheta)*180/3.1416;
        magnitude = sqrt(pow(mi2[i].mvX,2)+pow(mi2[i].mvY,2));
        if(magnitude>=3.0)
        {
            if(theta>90)
            {
                segPfnum[2] = i-1;
                mi2[i-1].segmentationPoint = 2;
                sp2found = true;
                break;
            }
        }
    }
    if(!sp2found)
    {
        segPfnum[2] = findTheNearestFarPointBodyFrame(mi2,segPfnum[1],segPfnum[4]-1,2);
        mi2[segPfnum[2]].segmentationPoint = 2;
    }

    bool sp3found = false;
    magnitude = 0.0;
    // 2-2. find p3
    for(int i=segPfnum[1]+2;i<segPfnum[4];i++)
    {
        double cosTheta = (double)(mi2[i].mvX)/sqrt(pow(mi2[i].mvX,2)+pow(mi2[i].mvY,2));
        double theta = acos(cosTheta)*180/3.1416;
        magnitude = sqrt(pow(mi2[i].mvX,2)+pow(mi2[i].mvY,2));
        if(magnitude>=3.0)
        {
            if(theta<=90)
            {
                segPfnum[3] = i+1;
                mi2[i+1].segmentationPoint = 3;
                sp3found = true;
                break;
            }
        }
    }
    if(!sp3found)
    {
        segPfnum[3] = findTheNearestFarPointBodyFrame(mi2,segPfnum[2],segPfnum[4]-1,1);
        mi2[segPfnum[3]].segmentationPoint = 3;
    }

    if(liftType==1) // snatch
    {

        // S-1. find p5

        bool p5find = false;
        for(int i=segPfnum[4]+1;i<NumOfFramesInMi1;i++)
        {
            if((mi2[i].mvY>=0)&&(mi2[i+1].mvY>mi2[i].mvY)&&(mi2[i+1].mvY<0))
            {
                p5find = true;
                segPfnum[5] = i;
                mi2[segPfnum[5]].segmentationPoint = 5;
                break;
            }
        }
        if(!p5find)
        {
            segPfnum[5] = findSmallestNonZeroY(mi2,segPfnum[4]+1,NumOfFramesInMi1,2);
            mi2[segPfnum[5]].segmentationPoint = 5;
        }

        // S-2. find p6
        int temp;
        temp = findSmallestNonZeroY(mi2,segPfnum[5]+1,NumOfFramesInMi1,1);
        segPfnum[6] = temp;
        mi2[segPfnum[6]].segmentationPoint = 6;
        // S-3. find snatch p99 (last)

        for(int i=segPfnum[6]+1;i<NumOfFramesInMi1;i++)
        {
            if((mi2[i].mvY>0)&&(mi2[i+1].mvY>mi2[i].mvY)&&(mi2[i+2].mvY>=mi2[i+1].mvY))
            {
                mi2[i].segmentationPoint = 99;
                segP99fnum = i;
                break;
            }
        }

        // calculation speed for each segmentation
        if(ui->cb2PSpeedForSegmentation->isChecked())
        {
            mi2[segPfnum[0]].speedFromPreviousSegment = 0.0;
            mi2[segPfnum[1]].speedFromPreviousSegment = mi2[segPfnum[1]].speedFromPreviousFrame;
            for(int i=2;i<=6;i++)
            {
                mi2[segPfnum[i]].speedFromPreviousSegment =
                        calculateSpeedBetweenFrames(mi2,segPfnum[i-1],segPfnum[i],realScale,frameRate);
            }
        }
    }
    if(liftType==2) // c&j
    {
        // c&j has 6-11
        // cj-1. find p11, highest
        segPfnum[11] = findSmallestNonZeroY(mi2,firstFrameNum,NumOfFramesInMi1,1);
        mi2[segPfnum[11]].segmentationPoint = 11;

        // cj-2. find p5
        segPfnum[5] = findSmallestNonZeroY(mi2,segPfnum[4]+1,segPfnum[11]-1,2);
        mi2[segPfnum[5]].segmentationPoint = 5;

        // cj-3. find p6, 2nd highest
        for(int i=segPfnum[5]+1;i<NumOfFramesInMi1-1;i++)
        {
            if((mi2[i].mvY>=0)&&(mi2[i+1].mvY>mi2[i].mvY))
            {
                segPfnum[6] = i;
                mi2[i].segmentationPoint = 6;
                break;
            }
        }

        // cj-4. find p7, 2nd chatch bar
        for(int i=segPfnum[6]+1;i<NumOfFramesInMi1;i++)
        {
            if((mi2[i].mvY<=0)&&(mi2[i+1].mvY<mi2[i].mvY))
            {
                segPfnum[7] = i;
                mi2[i].segmentationPoint = 7;
                break;
            }
        }

        // cj-5. find p8, pull up
        for(int i=segPfnum[7]+1;i<NumOfFramesInMi1;i++)
        {
            if((mi2[i].mvY>=0)&&(mi2[i-1].mvY))
            {
                segPfnum[8] = i;
                mi2[i].segmentationPoint = 8;
                break;
            }
        }

        // cj-6. 3rd catch bar
        for(int i=segPfnum[8]+1;i<NumOfFramesInMi1;i++)
        {
            if((mi2[i].mvY<=0)&&(mi2[i].mvY<mi2[i-1].mvY))
            {
                segPfnum[9] = i;
                mi2[i].segmentationPoint = 9;
                break;
            }
        }

        // cj-7. find p99 (last)
        for(int i=segPfnum[11]+1;i<NumOfFramesInMi1;i++)
        {
            if(mi2[i].mvY>0&&mi2[i].mvY<mi2[i+1].mvY&&mi2[i+1].mvY<mi2[i+2].mvY)
            {
                segP99fnum = i;
                mi2[i].segmentationPoint = 99;
                break;
            }
        }
    }

    if(ui->cb2PSaveVideo->isChecked())
    {
        VideoWriter p2ov;
        QString p2ovFn = vp.vfn+"_output.avi";
        IplImage *buff = cvCreateImage(cvSize(vp.orgW,vp.orgH),IPL_DEPTH_8U,3);
        IplImage *orgFrame = cvCreateImage(cvSize(vp.orgW,vp.orgH),IPL_DEPTH_8U,3);

        strTmp = (const char*)p2ovFn.toLocal8Bit();
        p2ov.open(strTmp.c_str(),CV_FOURCC('D','I','V','3'),frameRate,cvSize(vp.orgW,vp.orgH),1);
        if(!p2ov.isOpened())
        {
            MessageOut("2P: video ouput fail!",1,0);
            return;
        }

        for(int i=0;i<NumOfFramesInMi1;i++)
        {
            cvSetCaptureProperty(cap,CV_CAP_PROP_POS_FRAMES,i);
            orgFrame = cvQueryFrame(cap);

            if(ui->cb2PSaveVideoGray->isChecked())
            {
                for(int y=0;y<orgFrame->height;y++)
                    for(int x=0;x<orgFrame->widthStep;x+=3)
                    {
                        uchar b = orgFrame->imageData[y*orgFrame->widthStep+x];
                        uchar g = orgFrame->imageData[y*orgFrame->widthStep+x+1];
                        uchar r = orgFrame->imageData[y*orgFrame->widthStep+x+2];
                        uchar gray = (uchar)(0.299*r + 0.587*g + 0.114*b);
                        buff->imageData[y*orgFrame->widthStep+x] = gray;
                        buff->imageData[y*orgFrame->widthStep+x+1] = gray;
                        buff->imageData[y*orgFrame->widthStep+x+2] = gray;
                    }
            }
            else
                cvCopy(orgFrame,buff);

            CvPoint p0,p1,p3;
            CvScalar color;
            int thickness = ui->edt2Pthickness->text().toInt();
            int radiusOffset = orgSai.diam/2+0.5;

            // vertial line
            if(ui->cb2PDrawVerticalLine->isChecked())
            {
                p0 = cvPoint(orgVertX0+radiusOffset,orgVertY0+radiusOffset);
                p1 = cvPoint(orgVertX0+radiusOffset,mi2[segPfnum[6]].currY+radiusOffset);
                color = CV_RGB(0,112,192); // lightblue
                cvLine(buff,p0,p1,color,thickness,CV_AA,0);
            }
            // trajectory
            int r=2;
            int textGuidlineLength = ui->edt2PTextGuidlineLength->text().toInt();
            if(i>=firstFrameNum)
            {
                if(i<segP99fnum)
                {
                    for(int j=firstFrameNum;j<i;j++)
                    {
                        if(ui->cb2PDrawTrajectory->isChecked()&&j<(segP99fnum-1))
                        {
                            p0 = cvPoint(mi2[j].currX+radiusOffset,mi2[j].currY+radiusOffset);
                            p1 = cvPoint(mi2[j+1].currX+radiusOffset,mi2[j+1].currY+radiusOffset);
                            color = CV_RGB(60,202,87); // light green
                            cvLine(buff,p0,p1,color,thickness,CV_AA,0);
                        }
                        if(ui->cb2PDrawPositionMark->isChecked())
                        {
                            p3 = cvPoint(mi2[j].currX+radiusOffset,mi2[j].currY+radiusOffset);
                            color = CV_RGB(255,192,0);
                            cvCircle(buff,p3,r,color,thickness,CV_AA,0);
                        }
                        if(ui->cb2PMarkSegmentation->isChecked())
                        {
                            if(mi2[j].segmentationPoint!=-1)
                            {
                                p0 = cvPoint(mi2[j].currX+radiusOffset-r-2,mi2[j].currY+radiusOffset-r-2);
                                p1 = cvPoint(mi2[j].currX+radiusOffset+r+2,mi2[j].currY+radiusOffset+r+2);
                                color = CV_RGB(255,0,0);
                                cvRectangle(buff,p0,p1,color,thickness,CV_AA,0);
                            }
                        }
                        if(ui->cb2PDrawText->isChecked())
                        {
                            if(mi2[j].segmentationPoint>0 && mi2[j].segmentationPoint<99)
                            {
                                CvSize textSize;
                                int fontFace = CV_FONT_HERSHEY_DUPLEX;
                                double fontScale = 0.4;
                                int by;
                                int fontThickness = 1;
                                CvFont font = cvFont(fontScale,fontThickness);
                                cvInitFont(&font,fontFace,fontScale,fontScale,0,fontThickness,CV_AA);

                                // generate text
                                char cStrTemp[255];
                                // sprintf(cStrTemp,"p%d:sf=%3.2f,vd=%3.2f",mi2[j].segmentationPoint,mi2[j].speedFromPreviousFrame,mi2[j].normY*-1);
                                sprintf(cStrTemp,"p%d:vd=%3.2f",mi2[j].segmentationPoint,mi2[j].normY*-1);
                                cvGetTextSize(cStrTemp,&font,&textSize,&by);
                                if(mi2[j].segmentationPoint<=4)
                                {
                                    // draw in right hand side
                                    // draw guide line
                                    p0 = cvPoint(mi2[j].currX+radiusOffset-2-textGuidlineLength,mi2[j].currY+radiusOffset);
                                    p1 = cvPoint(mi2[j].currX+radiusOffset+2,mi2[j].currY+radiusOffset);
                                    color = CV_RGB(112,48,160);
                                    cvLine(buff,p0,p1,color,thickness,CV_AA,0);
                                    // draw boundingbox
                                    p0 = cvPoint(mi2[j].currX+radiusOffset-textGuidlineLength-textSize.width-5,mi2[j].currY+radiusOffset+textSize.height/2+5);
                                    p1 = cvPoint(mi2[j].currX+radiusOffset-textGuidlineLength-5,mi2[j].currY+radiusOffset-textSize.height/2-5);
                                    color = CV_RGB(242,242,242);
                                    cvRectangle(buff,p0,p1,color,-1,CV_AA,0);
                                    // draw text
                                    p0 = cvPoint(mi2[j].currX+radiusOffset-textGuidlineLength-5-textSize.width,mi2[j].currY+radiusOffset+textSize.height/2);
                                    color = CV_RGB(0,0,0);
                                    cvPutText(buff,cStrTemp,p0,&font,color);
                                }
                                else
                                {
                                    // draw in left hand side
                                    p0 = cvPoint(mi2[j].currX+radiusOffset+2,mi2[j].currY+radiusOffset);
                                    p1 = cvPoint(mi2[j].currX+radiusOffset+2+textGuidlineLength,mi2[j].currY+radiusOffset);
                                    color = CV_RGB(112,48,160);
                                    thickness = 1;
                                    cvLine(buff,p0,p1,color,thickness,CV_AA,0);
                                    // draw bundingbox
                                    p0 = cvPoint(mi2[j].currX+radiusOffset+textGuidlineLength,mi2[j].currY+radiusOffset+textSize.height/2+5);
                                    p1 = cvPoint(mi2[j].currX+radiusOffset+textGuidlineLength+textSize.width+5,mi2[j].currY+radiusOffset-textSize.height/2-5);
                                    color = CV_RGB(242,242,242);
                                    cvRectangle(buff,p0,p1,color,-1,CV_AA,0);
                                    // draw text
                                    p0 = cvPoint(mi2[j].currX+radiusOffset+textGuidlineLength+5,mi2[j].currY+radiusOffset+textSize.height/2);
                                    color = CV_RGB(0,0,0);
                                    cvPutText(buff,cStrTemp,p0,&font,color);
                                }
                            }
                        }
                    }
                }
                else
                {
                    for(int j=firstZeroFrameNumber;j<segP99fnum;j++)
                    {
                        if(ui->cb2PDrawTrajectory->isChecked()&&j<(segP99fnum-1))
                        {
                            p0 = cvPoint(mi2[j].currX+radiusOffset,mi2[j].currY+radiusOffset);
                            p1 = cvPoint(mi2[j+1].currX+radiusOffset,mi2[j+1].currY+radiusOffset);
                            color = CV_RGB(60,202,87);
                            cvLine(buff,p0,p1,color,thickness,CV_AA,0);
                        }
                        if(ui->cb2PDrawPositionMark->isChecked())
                        {
                            p3 = cvPoint(mi2[j].currX+radiusOffset,mi2[j].currY+radiusOffset);
                            color = CV_RGB(255,192,0);
                            cvCircle(buff,p3,r,color,thickness,CV_AA,0);
                        }
                        if(ui->cb2PMarkSegmentation->isChecked())
                        {
                            if(mi2[j].segmentationPoint!=-1)
                            {
                                p0 = cvPoint(mi2[j].currX+radiusOffset-r-2,mi2[j].currY+radiusOffset-r-2);
                                p1 = cvPoint(mi2[j].currX+radiusOffset+r+2,mi2[j].currY+radiusOffset+r+2);
                                color = CV_RGB(255,0,0);
                                cvRectangle(buff,p0,p1,color,thickness,CV_AA,0);
                            }
                        }
                        if(ui->cb2PDrawText->isChecked())
                        {
                            if(mi2[j].segmentationPoint>0 && mi2[j].segmentationPoint<99)
                            {
                                CvSize textSize;
                                int fontFace = CV_FONT_HERSHEY_DUPLEX;
                                double fontScale = 0.4;
                                int by;
                                int fontThickness = 1;
                                CvFont font = cvFont(fontScale,thickness);
                                cvInitFont(&font,fontFace,fontScale,fontScale,0,fontThickness,CV_AA);

                                // generate text
                                char cStrTemp[255];
                                // sprintf(cStrTemp,"p%d:sf=%3.2f,vd=%3.2f",mi2[j].segmentationPoint,mi2[j].speedFromPreviousFrame,mi2[j].normY*-1);
                                sprintf(cStrTemp,"p%d:vd=%3.2f",mi2[j].segmentationPoint,mi2[j].normY*-1);
                                cvGetTextSize(cStrTemp,&font,&textSize,&by);
                                if(mi2[j].segmentationPoint<=4)
                                {
                                    // draw in right hand side
                                    // draw line
                                    p0 = cvPoint(mi2[j].currX+radiusOffset-2-textGuidlineLength,mi2[j].currY+radiusOffset);
                                    p1 = cvPoint(mi2[j].currX+radiusOffset-2,mi2[j].currY+radiusOffset);
                                    color = CV_RGB(112,48,160);
                                    cvLine(buff,p0,p1,color,thickness,CV_AA,0);
                                    // draw bundingbox
                                    p0 = cvPoint(mi2[j].currX+radiusOffset-textGuidlineLength-textSize.width-5,mi2[j].currY+radiusOffset+textSize.height/2+5);
                                    p1 = cvPoint(mi2[j].currX+radiusOffset-textGuidlineLength-5,mi2[j].currY+radiusOffset-textSize.height/2-5);
                                    color = CV_RGB(242,242,242);
                                    cvRectangle(buff,p0,p1,color,-1,CV_AA,0);
                                    // draw text
                                    p0 = cvPoint(mi2[j].currX+radiusOffset-textGuidlineLength-5-textSize.width,mi2[j].currY+radiusOffset+textSize.height/2);
                                    color = CV_RGB(0,0,0);
                                    cvPutText(buff,cStrTemp,p0,&font,color);
                                }
                                else
                                {
                                    // draw in left hand side
                                    // draw line
                                    p0 = cvPoint(mi2[j].currX+radiusOffset+2,mi2[j].currY+radiusOffset);
                                    p1 = cvPoint(mi2[j].currX+radiusOffset+2+textGuidlineLength,mi2[j].currY+radiusOffset);
                                    color = CV_RGB(112,48,160);
                                    cvLine(buff,p0,p1,color,thickness,CV_AA,0);
                                    // draw bundingbox
                                    p0 = cvPoint(mi2[j].currX+radiusOffset+textGuidlineLength,mi2[j].currY+radiusOffset+textSize.height/2+5);
                                    p1 = cvPoint(mi2[j].currX+radiusOffset+textGuidlineLength+textSize.width+5,mi2[j].currY+radiusOffset-textSize.height/2-5);
                                    color = CV_RGB(242,242,242);
                                    cvRectangle(buff,p0,p1,color,-1,CV_AA,0);
                                    // draw text
                                    p0 = cvPoint(mi2[j].currX+radiusOffset+textGuidlineLength+5,mi2[j].currY+radiusOffset+textSize.height/2);
                                    color = CV_RGB(0,0,0);
                                    cvPutText(buff,cStrTemp,p0,&font,color);
                                }
                            }
                        }
                    }
                }
            }

            p2ov.write(buff);
            qApp->processEvents();
            ui->labelCurrentFrameCnt->setText(QString::number(i));
        }
        p2ov.release();
    }

    // write p2 result
    strTmp = (const char*)sp.mi2pFn.toLocal8Bit();
    FILE *fp = fopen(strTmp.c_str(),"wb");
    fwrite(mi2,sizeof(MI2),NumOfFramesInMi1,fp);
    fclose(fp);


    // dump text data
    if(ui->cb2PDumpTextData->isChecked())
    {
        strTmp = (const char*)sp.mi2pFn.toLocal8Bit();
        strTmp += ".csv";
        fp = fopen(strTmp.c_str(),"w");
        fprintf(fp,"frameCnt,currX,currY,mvX,mvY,liftType,searched,scaleFactor,normX,normY,"
                         "VerticalOffset,speedFromPreviousFrame,speedRanking,AccelerationFromPreviousFrame,"
                         "AccelerationRAnking,Segmentation,speedFromPreviousSegment,acclerationfromPreviousSegment\n");
        for(int i=0;i<NumOfFramesInMi1;i++)
        {
           fprintf(fp,"%d,%d,%d,%d,%d,%d,%d,%f,%f,%f,%f,%f,%d,%f,%d,%d,%f,%f\n",
                    mi2[i].frameCnt,mi2[i].currX,mi2[i].currY,mi2[i].mvX,mi2[i].mvY,mi2[i].liftType,mi2[i].searched,mi2[i].scaleFactor,
                    mi2[i].normX,mi2[i].normY,mi2[i].VerticalOffset,mi2[i].speedFromPreviousFrame,mi2[i].speedRanking,mi2[i].acclerationFromPreviousFrame,
                    mi2[i].acclerationRanking,mi2[i].segmentationPoint,mi2[i].speedFromPreviousSegment,mi2[i].acclerationFromPreviousSegment);
        }
        fclose(fp);
    }

    delete[] mi2;
    sp.p2searchDone = true;
    if(ui->cbConnectSVM->isChecked())
        MessageOut("2P: 2 pass done!",1,0);
    else
        MessageOut("2P: 2 pass done!",2,0);

}
// --------------------------------------------------------------------
int MainWindow::findTheNearestFarPointBodyFrame(MI2 *mi2temp,int beginFrame, int endFrame,int minmax)
{
    int n = endFrame - beginFrame + 1;
    // MI2 *mi2SortBuff = new MI2[n];
    MI2 mi2SortBuff[4096];
    MI2 mi2SwapTemp;
    int returnFrameNumber;
    // copy
    for(int i=0;i<n;i++)
        mi2SortBuff[i] = mi2temp[beginFrame+i];

    // bubble sort
    for(int i=0;i<n-1;i++)
    {
        for(int j=0;j<n-i;j++)
        {
            if(mi2SortBuff[j].currX>mi2SortBuff[j+1].currX)
            {
                mi2SwapTemp = mi2SortBuff[j];
                mi2SortBuff[j] = mi2SortBuff[j+1];
                mi2SortBuff[j+1] = mi2SwapTemp;
            }
        }
    }

    if(minmax==1)
        returnFrameNumber = mi2SortBuff[0].frameCnt;
    else
        returnFrameNumber = mi2SortBuff[n-1].frameCnt;
    // delete[] mi2SortBuff;

    return returnFrameNumber;
}
// --------------------------------------------------------------------
int MainWindow::findSmallestNonZeroY(MI2 *mi2temp,int beginFrame, int endFrame,int minmax)
{
    int n = endFrame - beginFrame+1;
    // MI2 *mi2SortBuff = new MI2[n];
    MI2 mi2SortBuff[4096];
    MI2 mi2SwapTemp;
    int returnFrameNumber;
    // copy
    for(int i=0;i<n;i++)
        mi2SortBuff[i] = mi2temp[beginFrame+i];

    // bubble sort
    for(int i=0;i<n-1;i++)
    {
        for(int j=0;j<n-i;j++)
        {
            if(mi2SortBuff[j].currY>mi2SortBuff[j+1].currY)
            {
                mi2SwapTemp = mi2SortBuff[j];
                mi2SortBuff[j] = mi2SortBuff[j+1];
                mi2SortBuff[j+1] = mi2SwapTemp;
            }
        }
    }

    // find smallest
    if(minmax==1)
    {
        for(int i=1;i<n;i++)
        {
            if(mi2SortBuff[i].currY!=0)
            {
                returnFrameNumber = mi2SortBuff[i].frameCnt;
                break;
            }
        }
    }
    else
        returnFrameNumber = mi2SortBuff[n-1].frameCnt;

    // delete[] mi2SortBuff;
    return returnFrameNumber;
}
// --------------------------------------------------------------------
double MainWindow::calculateSpeedBetweenFrames(
        MI2 *mi2temp,int beginFrame, int endFrame, double scale,double frameRate)
{
    int movingDistance;
    double realMovingDistance;
    double speed;
    movingDistance = 0;
    for(int i=beginFrame;i<=endFrame;i++)
        movingDistance += sqrt(pow(mi2temp[i].currX-mi2temp[i-1].currX,2)+pow(mi2temp[i].currY-mi2temp[i-1].currY,2));
    realMovingDistance = movingDistance*scale;
    speed = realMovingDistance/((endFrame-beginFrame)/frameRate);
    return speed;
}
// --------------------------------------------------------------- //
void MainWindow::quickGoodDemo()
{
    // get ini file
    settingSfpIniFile();

    sp.secondPlayProgram = sfp.programName;
    sp.secondPlayContolString = sp.system3PartyPath+sp.secondPlayProgram+" ";
    if(ui->rb2ndScreenMoveTo1->isChecked())
        sp.secondPlayContolString += "0 ";
    if(ui->rb2ndScreenEndAtLastFrame->isChecked())
        sp.secondPlayContolString += "1 ";
    sp.secondPlayContolString = sp.secondPlayContolString+"C:/w/sample/goodliftDemo/demo1-1.avi"+" "+"C:/w/sample/goodliftDemo/demo1-2.avi"+" ";
    if(ui->cb2ndScreenShowSVMResult->isChecked())
        sp.secondPlayContolString += ("AI:goodlift");

    MessageOut("sp:"+sp.secondPlayContolString,1,0);
    std::string str = (const char*)sp.secondPlayContolString.toLocal8Bit();
    system(str.c_str());
}
// --------------------------------------------------------------- //
// --------------------------------------------------------------- //
// --------------------------------------------------------------- //






























