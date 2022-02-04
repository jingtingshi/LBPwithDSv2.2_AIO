#include "searchpattern.h"
#include "mainwindow.h"
#include "QImage"
#include "QRgb"
#include <qdebug.h>
#include <sys/time.h>

SearchPattern::SearchPattern()
{

}
// --------------------------------------------------------------------

char *SearchPattern::QStringToChar(QString str)
{
    QByteArray byteArray = str.toLocal8Bit();
    char *c = byteArray.data();
    return c;
}
// --------------------------------------------------------------------
void SearchPattern::sampleLBP(uchar *f, int cx, int cy, int frameW, int frameH,
               int sampleW,int sampleH, int *histogram,double *mean)
{
    for(int y=1;y<sampleH-1;y+=3)
        for(int x=1;x<sampleW-1;x+=3)
        {
            int center = f[(cy+y)*frameW+(cx+x)];
            int powerCnt = 8;
            int vector = 0;
            for(int j=-1;j<=1;j++)
                for(int i=-1;i<=1;i++)
                {
                    powerCnt--;
                    if(i==0&&j==0)
                        continue;
                    if(f[(cy+y+j)*frameW+(cx+x+i)]==center)
                    {
                        f[(cy+y+j)*frameW+(cx+x+i)]==1;
                        vector+=pow(2.0,powerCnt);
                    }
                    else
                    {
                        f[(cy+y+j)*frameW+(cx+x+i)]==0;
                        vector+=0;
                    }
                }
            histogram[vector]++;
        }

    double tmpmean=0.0;
    for(int i=0;i<256;i++)
        tmpmean+=histogram[i];
    tmpmean /= 256;
    *mean = tmpmean;

}
// --------------------------------------------------------------------
void SearchPattern::fullsearch(IplImage *currFrame, IplImage *nextFrame, int orgX0, int orgY0,
                               int *mvX0, int *mvY0, int SamplingMethod, int sr, int radius,
                               QImage *updateImage, QImage *tmp,int currentFrameCnt,double *consumeTime)
{
    uchar *cf, *nf;
    IplImage *currGrayFrame, *nextGrayFrame;

    int w = currFrame->width;
    int h = currFrame->height;

    int sad, minsad;
    int cx,cy;

    QImage testSample(radius,radius,QImage::Format_RGB888);
    QImage testOrg(radius,radius,QImage::Format_RGB888);
    QTime timer;

    // struct timeval timerStart, timerStop;

    currGrayFrame = cvCreateImage(cvSize(w,h),IPL_DEPTH_8U,1);
    nextGrayFrame = cvCreateImage(cvSize(w,h),IPL_DEPTH_8U,1);
    cvCvtColor(currFrame,currGrayFrame,CV_RGB2GRAY);
    cvCvtColor(nextFrame,nextGrayFrame,CV_RGB2GRAY);
    cf = new uchar [w*h];
    nf = new uchar [w*h];

    uchar r;
    for(int y=0;y<h;y++)
        for(int x=0;x<w;x++)
        {
            r = currGrayFrame->imageData[y*w+x];
            cf[y*w+x] = r;
            r = nextGrayFrame->imageData[y*w+x];
            nf[y*w+x] = r;
        }

    if(SamplingMethod==0)
    {
        // pixel base full search
        timer.start();
        // gettimeofday(&timerStart,NULL);
        minsad = 2147483646;
        for(int posy =-sr;posy<sr;posy++)
            for(int posx=-sr;posx<sr;posx++)
            {
                sad = 0;
                cx = orgX0+posx;
                cy = orgY0+posy;
                if(cx<0 || (cx+radius)>=currFrame->width ||
                   cy<0 || (cy+radius)>=currFrame->height)
                        continue;
                for(int y=0;y<radius;y++)
                    for(int x=0;x<radius;x++)
                    {
                        sad += abs(cf[(orgY0+y)*w+(orgX0+x)]-
                                   nf[(cy+y)*w+(cx+x)]);
                    }

                if(sad<=minsad)
                {
                    *mvX0 = posx;
                    *mvY0 = posy;
                    minsad = sad;
                }

            }        
        *consumeTime = timer.elapsed()/1000.0;
        // gettimeofday(&timerStop,NULL);
        // *consumeTime = (1000000*(timerStop.tv_sec-timerStart.tv_sec)+timerStop.tv_usec-timerStart.tv_usec)/100000.0;
    }

    if(SamplingMethod==1)
    {
        // local binary pattern
        int histogramOrg[256]={0};
        int histogramCandidate[256] = {0};
        timer.start();
        // minsad = 2147483646;
        // double minmse = 214747346;
        // gettimeofday(&timerStart,NULL);
        double orgMean = 0.0;
        double candidateMean = 0.0;
        double MinCov = -1.0;
        double cov;
        sampleLBP(cf,orgX0,orgY0,w,h,radius,radius,histogramOrg,&orgMean);
        for(int posy=-sr;posy<sr;posy++)
            for(int posx=-sr;posx<sr;posx++)
            {
                // sad = 0;
                // double mse = 0.0;
                candidateMean = 0.0;
                memset(histogramCandidate,0,sizeof(int)*256);
                cx = orgX0 + posx;
                cy = orgY0 + posy;
                sampleLBP(nf,cx,cy,w,h,radius,radius,histogramCandidate,&candidateMean);

                // use pearson correlation coefficient
                double tmpcov1, tmpcov2, tmpcov3;
                tmpcov1 = 0.0;
                tmpcov2 = 0.0;
                tmpcov3 = 0.0;
                for(int i=0;i<256;i++)
                {
                    tmpcov1 += ((histogramOrg[i]-orgMean)*(histogramCandidate[i]-candidateMean));
                    tmpcov2 += pow(histogramOrg[i]-orgMean,2);
                    tmpcov3 += pow(histogramCandidate[i]-candidateMean,2);
                }
                cov = tmpcov1 / (sqrt(tmpcov2)*sqrt(tmpcov3));

                if(cov>=MinCov)
                {
                    *mvX0 = posx;
                    *mvY0 = posy;
                    MinCov=cov;
                }
            }
        *consumeTime = timer.elapsed()/1000.0;
        // gettimeofday(&timerStop,NULL);
        // *consumeTime = (1000000*(timerStop.tv_sec-timerStart.tv_sec)+timerStop.tv_usec-timerStart.tv_usec)/100000.0;

    }


    // save update sample
    for(int y=0;y<radius;y++)
        for(int x=0;x<radius;x++)
        {
            char r = cf[(orgY0+y)*w+(orgX0+x)];
            updateImage->setPixel(x,y,qRgb(r,r,r));
        }

    // save search result
    for(int y=0;y<radius;y++)
        for(int x=0;x<radius;x++)
        {
            char r = nf[(orgY0+y+*mvY0)*w+(orgX0+x+*mvX0)];
            tmp->setPixel(x,y,qRgb(r,r,r));
        }


}
// --------------------------------------------------------------------
void SearchPattern::DiamondSearch(IplImage *currFrame, IplImage *nextFrame, int orgX0, int orgY0,
                                  int *mvX0, int *mvY0, int SamplingMethod, int radius,
                                  QImage *updateImage, QImage *tmp, int currentFrameCnt, double *consumeTime)
{
    // PATTERN ldsp[9] = {{0,0},{0,-2},{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1}};
    PATTERN ldsp[11] = {{0,0},{0,-2},{0,-3},{1,-1},{2,0},{3,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1}};
    PATTERN sdsp[5] = {{0,0},{0,-1},{1,0},{0,1},{-1,0}};

    uchar *cf, *nf;
    IplImage *currGrayFrame, *nextGrayFrame;
    // struct timeval timerStart, timerStop;

    int w = currFrame->width;
    int h = currFrame->height;

    int sad, minsad;
    int cx,cy;

    bool sadinCenter;
    QImage testSample(radius,radius,QImage::Format_RGB888);
    QImage testOrg(radius,radius,QImage::Format_RGB888);
    QTime timer;

    currGrayFrame = cvCreateImage(cvSize(w,h),IPL_DEPTH_8U,1);
    nextGrayFrame = cvCreateImage(cvSize(w,h),IPL_DEPTH_8U,1);
    cvCvtColor(currFrame,currGrayFrame,CV_RGB2GRAY);
    cvCvtColor(nextFrame,nextGrayFrame,CV_RGB2GRAY);
    cf = new uchar [w*h];
    nf = new uchar [w*h];

    uchar r;
    for(int y=0;y<h;y++)
        for(int x=0;x<w;x++)
        {
            r = currGrayFrame->imageData[y*w+x];
            cf[y*w+x] = r;
            r = nextGrayFrame->imageData[y*w+x];
            nf[y*w+x] = r;
        }


    // for large pattern
    sadinCenter = false;
    int tmpmvx = 0;
    int tmpmvy = 0;
    int finMvX = 0;
    int finMvY = 0;
    int offX, offY;
    bool firstTime;
    firstTime = true;

    timer.start();

    // gettimeofday(&timerStart,NULL);
    if(SamplingMethod==0)
    {
        // large pattern
        while(!sadinCenter)
        {
            minsad = 2147483646;
            for(int i=0;i<11;i++)
            {
                if(firstTime)
                {
                    cx = orgX0+ldsp[i].x;
                    cy = orgY0+ldsp[i].y;
                }
                else
                {
                    cx = offX+ldsp[i].x;
                    cy = offY+ldsp[i].y;
                }
                sad = 0;
                for(int y=0;y<radius;y++)
                    for(int x=0;x<radius;x++)
                    {
                        sad+=abs(cf[(orgY0+y)*w+(orgX0+x)]-nf[(cy+y)*w+(cx+x)]);
                    }
                if(sad<minsad)
                {
                    tmpmvx = ldsp[i].x;
                    tmpmvy = ldsp[i].y;
                    minsad = sad;
                }
            }
            if(tmpmvx==0&&tmpmvy==0)
            {
                if(firstTime)
                {
                    offX = orgX0;
                    offY = orgY0;
                }
                sadinCenter = true;
            }
            else
            {
                if(firstTime)
                {
                    offX = orgX0+tmpmvx;
                    offY = orgY0+tmpmvy;
                }
                else
                {
                    offX += tmpmvx;
                    offY += tmpmvy;
                }
                finMvX+=tmpmvx;
                finMvY+=tmpmvy;

                firstTime = false;
            }
        }
        // small pattern refinement
        minsad = 2147483646;
        for(int i=0;i<5;i++)
        {
            cx = offX + sdsp[i].x;
            cy = offY + sdsp[i].y;
            sad = 0;
            for(int y=0;y<radius;y++)
                for(int x=0;x<radius;x++)
                {
                    sad+=abs(cf[(orgY0+y)*w+(orgX0+x)]-nf[(cy+y)*w+(cx+x)]);
                }
            if(sad<minsad)
            {
                tmpmvx = sdsp[i].x;
                tmpmvy = sdsp[i].y;
                minsad = sad;
            }
        }
        finMvX += tmpmvx;
        finMvY += tmpmvy;
    }
    else if(SamplingMethod==1)
    {
        int histogramOrg[256]={0};
        int histogramCandidate[256]={0};
        double orgMean = 0.0;
        double candidateMean = 0.0;
        double minCov = -1.0;
        double cov;
        sampleLBP(cf,orgX0,orgY0,w,h,radius,radius,histogramOrg,&orgMean);
        // large pattern
        while(!sadinCenter)
        {
            minCov = -1.0;
            for(int i=0;i<11;i++)
            {
                if(firstTime)
                {
                    cx = orgX0+ldsp[i].x;
                    cy = orgY0+ldsp[i].y;
                }
                else
                {
                    cx = offX+ldsp[i].x;
                    cy = offY+ldsp[i].y;
                }
                candidateMean=0.0;
                memset(histogramCandidate,0,sizeof(int)*256);
                sampleLBP(nf,cx,cy,w,h,radius,radius,histogramCandidate,&candidateMean);
                // pearson correlation coefficent
                double tmpcov1, tmpcov2, tmpcov3;
                tmpcov1=0.0;
                tmpcov2=0.0;
                tmpcov3=0.0;
                for(int i=0;i<256;i++)
                {
                    tmpcov1 += ((histogramOrg[i]-orgMean)*(histogramCandidate[i]-candidateMean));
                    tmpcov2 += pow(histogramOrg[i]-orgMean,2);
                    tmpcov3 += pow(histogramCandidate[i]-candidateMean,2);
                }
                cov = tmpcov1 / (sqrt(tmpcov2)*sqrt(tmpcov3));
                if(cov>=minCov)
                {
                    tmpmvx = ldsp[i].x;
                    tmpmvy = ldsp[i].y;
                    minCov = cov;
                }
            }
            if(tmpmvx==0&&tmpmvy==0)
            {
                if(firstTime)
                {
                    offX = orgX0;
                    offY = orgY0;
                }
                sadinCenter = true;                
            }
            else
            {
                if(firstTime)
                {
                    offX = orgX0+tmpmvx;
                    offY = orgY0+tmpmvy;
                }
                else
                {
                    offX += tmpmvx;
                    offY += tmpmvy;
                }
                finMvX+=tmpmvx;
                finMvY+=tmpmvy;

                firstTime = false;
            }
        }
        // small pattern refinement
        minCov = -1.0;
        for(int i=0;i<5;i++)
        {
            cx = offX + sdsp[i].x;
            cy = offY + sdsp[i].y;
            sampleLBP(nf,cx,cy,w,h,radius,radius,histogramCandidate,&candidateMean);
            double tmpcov1, tmpcov2, tmpcov3;
            tmpcov1=0.0;
            tmpcov2=0.0;
            tmpcov3=0.0;
            for(int i=0;i<256;i++)
            {
                tmpcov1 += ((histogramOrg[i]-orgMean)*(histogramCandidate[i]-candidateMean));
                tmpcov2 += pow(histogramOrg[i]-orgMean,2);
                tmpcov3 += pow(histogramCandidate[i]-candidateMean,2);
            }
            cov = tmpcov1 / (sqrt(tmpcov2)*sqrt(tmpcov3));
            if(cov>=minCov)
            {
                tmpmvx = sdsp[i].x;
                tmpmvy = sdsp[i].y;
                minCov = cov;
            }
        }
        finMvX += tmpmvx;
        finMvY += tmpmvy;
    }

    *consumeTime = timer.elapsed()/1000.0;
    // gettimeofday(&timerStop,NULL);
    // *consumeTime = (1000000*(timerStop.tv_sec-timerStart.tv_sec)+timerStop.tv_usec-timerStart.tv_usec)/100000.0;

    // save motion vector
    *mvX0 = finMvX;
    *mvY0 = finMvY;

    // save update sample
    for(int y=0;y<radius;y++)
        for(int x=0;x<radius;x++)
        {
            char r = cf[(orgY0+y)*w+(orgX0+x)];
            updateImage->setPixel(x,y,qRgb(r,r,r));
        }

    // save search result
    for(int y=0;y<radius;y++)
        for(int x=0;x<radius;x++)
        {
            char r = nf[(orgY0+y+*mvY0)*w+(orgX0+x+*mvX0)];
            tmp->setPixel(x,y,qRgb(r,r,r));
        }
}
// --------------------------------------------------------------------
void SearchPattern::fullsearchWithoutUpdate(IplImage *currFrame,int orgX0, int orgY0, int SamplingMethod, int sr, int radius,int *mvX0, int *mvY0,
                               QImage *updateImage, QImage *tmp,int currentFrameCnt, double *consumeTime, uchar *orgTemplate)
{
    uchar *cf;
    IplImage *currGrayFrame;

    int w = currFrame->width;
    int h = currFrame->height;

    int sad, minsad;
    int cx,cy;

    // QImage testSample(radius,radius,QImage::Format_RGB888);
    // QImage testOrg(radius,radius,QImage::Format_RGB888);
    QTime timer;

    // struct timeval timerStart, timerStop;

    currGrayFrame = cvCreateImage(cvSize(w,h),IPL_DEPTH_8U,1);
    cvCvtColor(currFrame,currGrayFrame,CV_RGB2GRAY);
    cf = new uchar [w*h];

    uchar r;
    for(int y=0;y<h;y++)
        for(int x=0;x<w;x++)
        {
            r = currGrayFrame->imageData[y*w+x];
            cf[y*w+x] = r;
        }

    if(SamplingMethod==0)
    {
        // pixel base full search
        timer.start();
        // gettimeofday(&timerStart,NULL);
        minsad = 2147483646;
        for(int posy =-sr;posy<sr;posy++)
            for(int posx=-sr;posx<sr;posx++)
            {
                sad = 0;
                cx = orgX0+posx;
                cy = orgY0+posy;
                if(cx<0 || (cx+radius)>=currFrame->width ||
                   cy<0 || (cy+radius)>=currFrame->height)
                        continue;
                for(int y=0;y<radius;y++)
                    for(int x=0;x<radius;x++)
                    {
                        sad += abs(orgTemplate[y*radius+x]-
                                   cf[(cy+y)*w+(cx+x)]);
                    }

                if(sad<=minsad)
                {
                    *mvX0 = posx;
                    *mvY0 = posy;
                    minsad = sad;
                }

            }
        *consumeTime = timer.elapsed()/1000.0;
        // gettimeofday(&timerStop,NULL);
        // *consumeTime = (1000000*(timerStop.tv_sec-timerStart.tv_sec)+timerStop.tv_usec-timerStart.tv_usec)/100000.0;
    }
#if 0
    if(SamplingMethod==1)
    {
        // local binary pattern
        int histogramOrg[256]={0};
        int histogramCandidate[256] = {0};
        timer.start();
        // minsad = 2147483646;
        // double minmse = 214747346;
        // gettimeofday(&timerStart,NULL);
        double orgMean = 0.0;
        double candidateMean = 0.0;
        double MinCov = -1.0;
        double cov;
        sampleLBP(cf,orgX0,orgY0,w,h,radius,radius,histogramOrg,&orgMean);
        for(int posy=-sr;posy<sr;posy++)
            for(int posx=-sr;posx<sr;posx++)
            {
                // sad = 0;
                // double mse = 0.0;
                candidateMean = 0.0;
                memset(histogramCandidate,0,sizeof(int)*256);
                cx = orgX0 + posx;
                cy = orgY0 + posy;
                sampleLBP(nf,cx,cy,w,h,radius,radius,histogramCandidate,&candidateMean);

                // use pearson correlation coefficient
                double tmpcov1, tmpcov2, tmpcov3;
                tmpcov1 = 0.0;
                tmpcov2 = 0.0;
                tmpcov3 = 0.0;
                for(int i=0;i<256;i++)
                {
                    tmpcov1 += ((histogramOrg[i]-orgMean)*(histogramCandidate[i]-candidateMean));
                    tmpcov2 += pow(histogramOrg[i]-orgMean,2);
                    tmpcov3 += pow(histogramCandidate[i]-candidateMean,2);
                }
                cov = tmpcov1 / (sqrt(tmpcov2)*sqrt(tmpcov3));

                if(cov>=MinCov)
                {
                    *mvX0 = posx;
                    *mvY0 = posy;
                    MinCov=cov;
                }
            }
        *consumeTime = timer.elapsed()/1000.0;
        // gettimeofday(&timerStop,NULL);
        // *consumeTime = (1000000*(timerStop.tv_sec-timerStart.tv_sec)+timerStop.tv_usec-timerStart.tv_usec)/100000.0;

    }

#endif    
    // save update sample
    for(int y=0;y<radius;y++)
        for(int x=0;x<radius;x++)
        {
            char r = cf[(orgY0+y)*w+(orgX0+x)];
            updateImage->setPixel(x,y,qRgb(r,r,r));
        }

    // save search result
    for(int y=0;y<radius;y++)
        for(int x=0;x<radius;x++)
        {
            char r = cf[(orgY0+y+*mvY0)*w+(orgX0+x+*mvX0)];
            tmp->setPixel(x,y,qRgb(r,r,r));
        }

    delete cf;
}

