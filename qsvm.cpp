#include "qsvm.h"
#include "opencv2/opencv.hpp"
#include "opencv2/ml.hpp"
#include "QString"
#include <mainwindow.h>
#include <datastructure.h>

#define GOODLIFT 1
#define NOLIFT -1


qSVM::qSVM()
{

}

// ---------------------------------------------------- //
Mat qSVM::convertQImageToMat(QImage qImg)
{
    Mat m;
    m = Mat(qImg.height(),qImg.width(),CV_8UC3,(void*)qImg.constBits(),qImg.bytesPerLine());
    cvtColor(m,m,CV_BGR2RGB);
    return m;
}
// ---------------------------------------------------- //
double qSVM::qSvmGoodNoLiftClassfier(int *result, QImage vecImg)// , QString svmClassfierXmlFn)
{
    Mat testImg = convertQImageToMat(vecImg);
    Mat testImg1D(1,testImg.size().width*testImg.size().height*3,CV_32FC1);
    std:;string strTmp;

    CvSVM svmCalssfier;
    double classfyResult;

    // load svm classfier
    strTmp = (const char*)param.svmClassfierXmlFn.toLocal8Bit();
    svmCalssfier.load(strTmp.c_str());
    // change to gray
    int sampleCnt =0;
    for(int y=0;y<testImg.size().height;y++)
        for(int x=0;x<testImg.size().width*3;x+=3)
        {
            testImg1D.at<float>(0,y*testImg.size().width+x) = testImg.at<Vec3b>(y,x/3)[2];
            testImg1D.at<float>(0,y*testImg.size().width+x+1) = testImg.at<Vec3b>(y,x/3)[1];
            testImg1D.at<float>(0,y*testImg.size().width+x+2) = testImg.at<Vec3b>(y,x/3)[0];
        }
    // classfy
    classfyResult = svmCalssfier.predict(testImg1D);
    return classfyResult;
}
// ---------------------------------------------------- //
// void qSVM::qsvmMain(MI2 *mi2,QString vecFn,int midLevel,int normFact,int dumpText,int *NumOfNodes, int sw, int sh, int cjw, int cjh,int NumOfNodesInMI2,int saveVector)
void qSVM::qsvmMain(MI2 *mi2,double *result)
{
    int dumpText = 0; // for debug
    int w, h;
    QImage *vecImg;
    if(param.liftType==1)
    {
        w = param.sw;
        h = param.sh;
    }
    else if(param.liftType==2)
    {
        w = param.cjw;
        h = param.cjh;
    }
    vecImg = new QImage(w,h,QImage::Format_RGB888);

    qSvmVecotrComposer(mi2,param.vecFn,param.svmMidLevel,param.svmNormFact,dumpText,
                       param.NumOfNodes,w,h,param.NumOfFrameInMI1,vecImg);
    if(param.saveVector)
        vecImg->save(param.vecFn);
}
// ---------------------------------------------------- //
void qSVM::qSvmComposeSingleVector(MI2 *mi2,QString vecFn,int normFact, int midLevel,int sw,int sh, int cjw, int cjh,int NumOfNodesInMI2)
{
    /*
    int dumpText = 0;
    int composedNodes;
    qSvmVecotrComposer(mi2,vecFn,midLevel,normFact,dumpText,&composedNodes,sw,sh,cjw,cjh,NumOfNodesInMI2);
    a.MessageOut("qSVM: vector composed!",1);
    */
}
// ---------------------------------------------------- //
// void qSVM::qSvmVecotrComposer(MI2 *mi2,QString vecFn,int midLevel,int normFact,int dumpText,int *NumOfNodes, int sw, int sh, int cjw, int cjh,int NumOfNodesInMI2)
void qSVM::qSvmVecotrComposer(MI2 *mi2,QString vecFn,int midLevel,int normFact,int dumpText,int NumOfNodes, int w, int h,int NumOfNodesInMI2,QImage *vecImg)
{
    std::string strTmp;
    // QImage *vecImg;
    double mvx,mvy;
    bool start;
    int modeVector;
    int modeVectorLevel = 75;
    int modeVectorInterval = (255-modeVectorLevel)/9;
    int vecCnt;
    int vectorLimit;

    struct VECTORPOINT {
        int mvX;
        int mvY;
        int modeVector;
    } *vecBuffer;

    vecImg = new QImage(w,h,QImage::Format_RGB888);
    vecBuffer = new struct VECTORPOINT[w*h];
    vectorLimit = w*h;

    memset(vecBuffer,0,sizeof(struct VECTORPOINT)*NumOfNodesInMI2);
    start = false;
    vecCnt = 0;
    for(int i=0;i<NumOfNodesInMI2;i++)
    {
        // check 0
        if(mi2[i].segmentationPoint==0)
        {
            // -1: 0, 0-8: 75-255, (@20)
            mvx = midLevel;
            mvy = midLevel;
            modeVector = modeVectorLevel;
            vecBuffer[vecCnt].mvX = mvx;
            vecBuffer[vecCnt].mvY = mvy;
            vecBuffer[vecCnt].modeVector = modeVector;
            vecCnt++;
        }
        // check 1 and 99
        if(mi2[i].segmentationPoint==1)
            start = true;
        if(mi2[i].segmentationPoint==99)
            start = false;

        if(start)
        {
            mvx = (mi2[i].normX - mi2[i-1].normX)*-1;
            mvy = (mi2[i].normY - mi2[i-1].normY)*-1;
            mvx = (mvx/normFact)*255+midLevel;
            mvy = (mvy/normFact)*255+midLevel;
            if(mi2[i].segmentationPoint!=-1)
                modeVector = modeVectorLevel+mi2[i].segmentationPoint*modeVectorLevel;
            else {
                modeVector = 0;
            }

            mvx = bound(mvx);
            mvy = bound(mvy);
            modeVector = bound(modeVector);
            vecCnt++;
        }

        /*
        if(vecCnt>vectorLimit)
        {
            a.MessageOut("QSVM: vector compose error!",1);
            return;
        }
        */

    }
    param.NumOfNodes = vecCnt;

    // compose image
    for(int y=0;y<h;y++)
        for(int x=0;x<w;x++)
        {
            int r = vecBuffer[y*w+x].mvX;
            int g = vecBuffer[y*w+x].mvY;
            int b = vecBuffer[y*w+x].modeVector;
            vecImg->setPixel(x,y,qRgb(r,g,b));
        }

    // vecImg->save(vecFn);

    // dump text
    if(dumpText==1)
    {
        strTmp = (const char*)vecFn.toLocal8Bit();
        strTmp += ".csv";
        FILE *fp = fopen(strTmp.c_str(),"w");
        fprintf(fp,"node#,mvx,mvy,node\n");
        for(int y=0;y<h;y++)
            for(int x=0;x<w;x++)
            {
                int dmvx = vecBuffer[y*w+x].mvX;
                int dmvy = vecBuffer[y*w+x].mvY;
                int dmode = vecBuffer[y*w+x].modeVector;
                fprintf(fp,"%d,%d,%d,%d\n",y*w+x,dmvx,dmvy,dmode);
            }
        fclose(fp);
    }
}
// ---------------------------------------------------- //
int qSVM::bound(int a)
{
    if(a>255) return 255;
    if(a<0) return 0;
    return a;
}
// ---------------------------------------------------- //
