#ifndef QSVM_H
#define QSVM_H

#include <QMainWindow>
#include <datastructure.h>
#include "opencv2/opencv.hpp"

using namespace cv;

class qSVM
{
public:
    qSVM();    
    // void qsvmMain(MI2 *mi2,QString vecFn,int midLevel,int normFact,int dumpText,int *NumOfNodes, int sw, int sh, int cjw, int cjh,int NumOfNodesInMI2,int saveVector);
    void qsvmMain(MI2 *mi2,double *result);
    // void qSvmGoodNoliftClassfier(int *result, QString vecFn, QString svmClassfierXmlFn);
    double qSvmGoodNoLiftClassfier(int *result, QImage vecImg);
    void qSvmComposeSingleVector(MI2 *mi2,QString vecFn,int normFact, int midLevel,int sw,int sh, int cjw, int cjh, int NumOfNodesInMI2);
    // void qSvmVecotrComposer(MI2 *mi2,QString vecFn,int midLevel,int normFact,int dumpText,int *NumOfNodes, int sw, int sh, int cjw, int cjh,int NumOfNodesInMI2);
    void qSvmVecotrComposer(MI2 *mi2,QString vecFn,int midLevel,int normFact,int dumpText,int NumOfNodes, int w, int h, int NumOfNodesInMI2,QImage *vecImg);
    int bound(int a);
    Mat convertQImageToMat(QImage qImg);

    struct qSVMParam {
        int svmMidLevel;
        int svmNormFact;
        int sw;
        int sh;
        int cjw;
        int cjh;
        int NumOfNodes;
        int NumOfFrameInMI1;
        int saveVector;
        QString vecFn;
        int liftType;
        QString svmClassfierXmlFn;
    } param;

    // MainWindow a;
};

#endif // QSVM_H
