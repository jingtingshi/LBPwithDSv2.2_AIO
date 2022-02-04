#ifndef DATASTRUCTURE_H
#define DATASTRUCTURE_H


typedef struct SelectAreaInformation {
    int x,y;
    int x0,y0,x1,y1;
    int diam;
} SAI;


typedef struct MOTIONINFORMATION2 {
    int frameCnt;
    int currX, currY;
    int mvX, mvY;
    int searched;
    double scaleFactor;
    double normX, normY;
    double VerticalOffset;
    double speedFromPreviousFrame;
    int segmentationPoint;
    // snatch
    // 0 up, 1 first frame, 2 nearst body, 3 far from body, 4 highest position
    // 5 catch bar, 6 highest position, 99 analysis end
    // c&j
    // 0 up, 1 first frame, 2 nearest body, 3 far from body, 4 highest position
    // 5 catch bar, 6 highest position, 7 catch bar, 8 highest position
    int liftType; // 1 for snatch, 2 for clean & Jerk
    int speedRanking;
    double acclerationFromPreviousFrame;
    int acclerationRanking;
    double speedFromPreviousSegment;
    double acclerationFromPreviousSegment;
} MI2;

typedef struct MOTIONINFORMATION {
    int frameCnt;
    int nextX, nextY;
    int currX, currY;
    int mvX, mvY;
    double eachTime;
    double totalTime;
    bool searched;
    // for 2.0
    SAI orgSai;
    int liftType;
    double scaleFactor;
    double frameRate;
    int orgWidth;
    int orgHeight;
} MOINFO;


#endif // DATASTRUCTURE_H
