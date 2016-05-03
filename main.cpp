#include <iostream>
#include "main.h"
#include "Human.h"

using namespace std;
using namespace cv;

int detect_interval = 5;
int id = 0;
HOGDescriptor hog;

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
    string window_name = "video | q or esc to quit";
    namedWindow(window_name, WINDOW_KEEPRATIO); //resizable window

    hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());

    VideoCapture *cap = new VideoCapture("/home/hya/workspace/reidentification/video/campus4-c0.avi");
    Mat gray, curImage;
    vector<Rect> foundLocations;
    vector<double> foundWeights;

    int frame_idx = 0;
    vector<Ptr<Human>> identified;

    Mat bgImage;
    Mat diffImage;
    Mat maskedCurImage;
    int morph_elem = 2;
    int morph_size = 3;

    while (cap->isOpened()) {
        cap->read(curImage);
        if (curImage.empty()) {
            break;
        }
        resize(curImage, curImage, Size(0,0), 2, 2, INTER_LANCZOS4);
        if (bgImage.empty()) {
            curImage.copyTo(bgImage);
        }
        absdiff(bgImage, curImage, diffImage);
        Mat mask = Mat::zeros(diffImage.rows, diffImage.cols, CV_8UC1);


        float threshold = 10.0f;
        float dist;
        for (int j = 0; j < diffImage.rows; ++j) {
            for (int i = 0; i < diffImage.cols; ++i) {
                cv::Vec3b pix = diffImage.at<cv::Vec3b>(j, i);

                dist = (pix[0] * pix[0] + pix[1] * pix[1] + pix[2] * pix[2]);
                dist = sqrt(dist);

                if (dist > threshold) {
                    mask.at<uchar>(j, i) = 255;
                }

            }
        }

        // Since MORPH_X : 2,3,4,5 and 6
        int operation = 2;

        Mat element = getStructuringElement(morph_elem, Size(2 * morph_size + 1, 2 * morph_size + 1),
                                            Point(morph_size, morph_size));

        /// Apply the specified morphology operation
        morphologyEx(mask, mask, operation, element);
        int dilation_size = 3;

        dilate(mask, mask, getStructuringElement(0,
                                                 Size(2 * dilation_size + 1, 2 * dilation_size + 1),
                                                 Point(dilation_size, dilation_size)));

        maskedCurImage.release();
        curImage.copyTo(maskedCurImage, mask);

        if (frame_idx % detect_interval == 0) {
            cvtColor(curImage, gray, COLOR_BGR2GRAY);

            hog.detectMultiScale(gray, foundLocations, foundWeights, hogParams.hitThreshold, hogParams.winStride,
                                 hogParams.padding,
                                 hogParams.scale, hogParams.finalThreshold, hogParams.useMeanShift);

            for (Rect rect: foundLocations) {
                Ptr<Human> human = new Human();
                Rect trimmed = trimRect(rect);
                human->descriptor.extractFeatures(curImage, trimmed, mask);
                bool matched = false;
                for (vector<Ptr<Human>>::iterator it = identified.begin(); it != identified.end(); it++) {
                    double comparison = 1 - human->descriptor.compare(((*it)->descriptor));
                    if (comparison > 0.6) {
                        matched = true;
                        (*it)->point = Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
                        break;
                    }
                }
                if (!matched) {
                    human->id = id++;
                    human->point = Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
                    human->color = randColor();
                    identified.push_back(human);
                }
            }
        }


//        draw_detections(curImage, foundLocations);
        draw_identified(curImage, identified);

        imshow(window_name, curImage);

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



