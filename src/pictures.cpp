#include "pictures.hpp"
#include "config.hpp"
#include <opencv/highgui.h>
#include <iostream>

namespace Constants {
    const float strokeBackground = 10.0;
}

namespace Pictures {
	cv::Mat_<cv::Vec3b> original;
    cv::Mat_<uchar> input;
    cv::Mat_<short> sobelX;
    cv::Mat_<short> sobelY;
    cv::Mat_<uchar> canny;
    cv::Mat_<float> strokes;
}

void Pictures::initialize()
{
	original = cv::imread(Config::inputFileName, 1);
	if(original.cols == 0 || original.rows == 0)
        throw "Could not read inputfile.";
    input = cv::imread(Config::inputFileName, 0);
    cv::Sobel(input, sobelX, CV_16S, 1, 0, Config::variables["apertureSize"], 1, 0, cv::BORDER_REPLICATE);
    cv::Sobel(input, sobelY, CV_16S, 0, 1, Config::variables["apertureSize"], 1, 0, cv::BORDER_REPLICATE);
    cv::Canny(input, canny, Config::variables["cannyThreshold1"], Config::variables["cannyThreshold2"],
                                Config::variables["apertureSize"], Config::variables["accurateCanny"]);
    strokes = cv::Mat(input.size(), CV_32FC1, cv::Scalar(Constants::strokeBackground));
}

void Pictures::save()
{
    cv::imwrite("canny.png", Pictures::canny);
}

void Pictures::show()
{
    cv::namedWindow("canny", CV_WINDOW_AUTOSIZE);
    cv::imshow("canny", Pictures::canny);
    cv::namedWindow("strokes", CV_WINDOW_AUTOSIZE);
    cv::imshow("strokes", Pictures::strokes);
    cv::namedWindow("original", CV_WINDOW_AUTOSIZE);
    cv::imshow("original", Pictures::original);
    cv::namedWindow("input", CV_WINDOW_AUTOSIZE);
    cv::imshow("input", Pictures::input);
    cv::waitKey();
}
