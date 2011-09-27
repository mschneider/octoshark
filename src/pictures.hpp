#include <opencv/cv.h>

namespace Pictures {   
    void initialize();
    void save();
    void show();
    
    extern cv::Mat_<cv::Vec3b> original;
    extern cv::Mat_<uchar> input;
    extern cv::Mat_<short> sobelX;
    extern cv::Mat_<short> sobelY;
    extern cv::Mat_<uchar> canny;
    extern cv::Mat_<float> strokes;
}

namespace Constants {
    extern const float strokeBackground;
}
