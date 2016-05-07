#include <iostream>
#include "main.h"
#include "Human.h"

using namespace std;
using namespace cv;

int detect_interval = 5;
int id = 0;
HOGDescriptor hog;
Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor

Rect trimRect(Rect r);

void draw_identified(Mat img, vector<Ptr<Human>> identified) {
    for (Ptr<Human> human : identified) {
        string str = "ID:";
        str.append(to_string(human->id));

        putText(img, str, human->point, CV_FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 255), 1);
        circle(img, human->point, 4, human->color);
    }
}

void draw_detections(Mat frame, vector<Rect> rects) {
    for (Rect rect : rects) {
        int pad_w = (int) (0.15 * rect.width);
        int pad_h = (int) (0.05 * rect.height);
        Point pt1(rect.x + pad_w, rect.y + pad_h);
        Point pt2(rect.x + rect.width - pad_w, rect.y + rect.height - pad_h);

        putText(frame, "Human", pt1, CV_FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 255), 1);
        rectangle(frame, pt1, pt2, Scalar(0, 255, 0));
        circle(frame, Point(rect.x + rect.width / 2, rect.y + rect.height / 2), 3, Scalar(0, 0, 255));
    }
}

cv::Scalar randColor() {
    return Scalar(rand() % 256, rand() % 256, rand() % 256);
}

int main() {
    namedWindow("1", WINDOW_KEEPRATIO);
    hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
    pMOG2 = createBackgroundSubtractorMOG2(); //MOG2 approach

    VideoCapture *cap = new VideoCapture("/home/hya/workspace/reidentification/video/campus4-c0.avi");
    Mat gray, curImage;
    vector<Rect> foundLocations;
    vector<double> foundWeights;

    int frame_idx = 0;
    vector<Ptr<Human>> identified;

    Mat maskedCurImageMOG2;
    int morph_elem = 2;

    while (cap->isOpened()) {
        cap->read(curImage);
        if (curImage.empty()) {
            break;
        }
//        resize(curImage, curImage, Size(0,0), 2, 2, INTER_LANCZOS4);
        pMOG2->apply(curImage, fgMaskMOG2, 0);

        // Since MORPH_X : 2,3,4,5 and 6

        medianBlur(fgMaskMOG2, fgMaskMOG2, 2 * 2 + 1);

        int morph_size2 = 3;

        morphologyEx(fgMaskMOG2, fgMaskMOG2, 3,
                     getStructuringElement(morph_elem, Size(2 * morph_size2 + 1, 2 * morph_size2 + 1),
                                           Point(morph_size2, morph_size2)));


        maskedCurImageMOG2.release();
        curImage.copyTo(maskedCurImageMOG2, fgMaskMOG2);

        if (frame_idx % detect_interval == 0) {
            cvtColor(curImage, gray, COLOR_BGR2GRAY);

            hog.detectMultiScale(gray, foundLocations, foundWeights, hogParams.hitThreshold, hogParams.winStride,
                                 hogParams.padding,
                                 hogParams.scale, hogParams.finalThreshold, hogParams.useMeanShift);

            for (Rect rect: foundLocations) {
                Ptr<Human> human = new Human();
                Rect trimmed = trimRect(rect);
                human->descriptor.extractFeatures(curImage, trimmed, fgMaskMOG2);
                vector<Ptr<Human>>::iterator best = identified.end();
                double best_comparison = 0;
                for (vector<Ptr<Human>>::iterator it = identified.begin(); it != identified.end(); it++) {
                    double comparison = 1 - human->descriptor.compare(((*it)->descriptor));
                    if (comparison > COMPARISON_THRESHOLD && best_comparison < comparison) {
                        best_comparison = comparison;
                        best = it;
                    }
                }
                if(best != identified.end()) {
                    (*best)->point = Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
                    (*best)->descriptor = human->descriptor;
                } else {
                    human->id = id++;
                    human->point = Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
                    human->color = randColor();
                    identified.push_back(human);
                }
            }
        }

        draw_detections(maskedCurImageMOG2, foundLocations);
        draw_identified(maskedCurImageMOG2, identified);

        imshow("1", maskedCurImageMOG2);

        frame_idx++;

        char key = (char) waitKey(30);
        switch (key) {
            case 'q':
            case 'Q':
            case 27: //escape key
                return 0;
            default:
                break;
        }
    }

    delete cap;

    return 0;
}

Rect trimRect(Rect rect) {
    int pad_w = (int) (0.15 * rect.width);
    int pad_h = (int) (0.05 * rect.height);
    Point pt1(rect.x + pad_w, rect.y + pad_h);
    Point pt2(rect.x + rect.width - pad_w, rect.y + rect.height - pad_h);
    return Rect(pt1, pt2);
}
