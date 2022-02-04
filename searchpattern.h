#ifndef SEARCHPATTERN_H
#define SEARCHPATTERN_H
#include <cv.h>
#include <opencv2/opencv.hpp>
#include <cvaux.h>
#include <QImage>
#include <QTime>

class SearchPattern
{
public:
    SearchPattern();
    int w, h;

    char *QStringToChar(QString str);

    void fullsearch(IplImage *currFrame, IplImage *nextFrame, int orgX0, int orgY0,
                    int *mvX0, int *mvY0, int SamplingMethod, int sr,
                    int radius,QImage *updateImg, QImage *tmp,int currentFrameCnt,
                    double *consumeTime);


    void DiamondSearch(IplImage *currFrame, IplImage *nextFrame,int orgX0, int orgY0,
                          int *mvX0, int *mvY0, int SamplingMethod, int radius, QImage *updateImg,
                          QImage *tmp,int currentFrameCnt, double *consumeTime);

    void sampleLBP(uchar *f, int cx, int cy, int frameW, int frameH,
                   int sampleW,int sampleH, int *histogram,double *mean);

    void fullsearchWithoutUpdate(IplImage *currFrame,int orgX0, int orgY0, int SamplingMethod, int sr, int radius,int *mvX0, int *mvY0,
                                   QImage *updateImage, QImage *tmp,int currentFrameCnt, double *consumeTime, uchar *orgTemplate);
    typedef struct PATT {
        int x, y;
    } PATTERN;

};

#endif // SEARCHPATTERN_H
