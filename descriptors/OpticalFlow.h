//
// Created by hya on 21.05.16.
//

#ifndef REIDENTIFICATION_OPTICALFLOW_H
#define REIDENTIFICATION_OPTICALFLOW_H

#include <cv.hpp>

using namespace cv;
using namespace std;

class OpticalFlow {
public:
    vector<Point2f> points[2];
    TermCriteria termcrit;
    Size subPixWinSize;
    Size winSize;
    const int MAX_COUNT = 500;

    Point centroid;
    bool blocked;
    Rect boundingBox;

    void resetNotFoundPoints(vector<uchar> vector1, Point lastPoint);

    OpticalFlow();

    void calculate(Mat &prev, Mat &img, Point lastPoint, int lastWidth, int lastHeight);
    void getOpticalFlowPoints(const Rect &rect, Mat &img);
    void drawPoints(Mat img, Scalar scalar_);
    void swapPoints();

    void calculateWithCollision(Point move);

private:
    void calculateRegion();

    void movePoints(int i, int i1);
};


#endif //REIDENTIFICATION_OPTICALFLOW_H
