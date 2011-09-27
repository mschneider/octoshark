#include <vector>
#include <opencv/cv.h>

class Contour {
    bool wasDrawn;
    int size;
    std::vector<cv::Rect> subContours;
    bool mergeWith(const Contour& other);
    int minX() const;
    int maxX() const;
    bool operator<(const Contour& other) const;
    
public:    
    bool contains(const cv::Point_<int>& position) const;
    bool intersects(const cv::Rect& rect) const;
    bool intersects(const Contour& other) const;
    bool includes(const cv::Rect& rect) const;
    bool includes(const Contour& other) const;
    void drawOn(cv::Mat& output, const cv::Scalar boundingRectangleColor, const cv::Scalar subRectangleColor) const;
    void insertIntoLimitMap(std::vector<std::vector<Contour*> >& limitMap);
    
    Contour(const cv::Rect& firstElement);
    static const std::vector<std::vector<Contour*> > buildLimitMap(const cv::Mat_<uchar>& cannyImage);
    static std::vector<Contour*> collectContours(const cv::Mat_<uchar>& cannyImage);
    static void mergeOverlappingContours(std::vector<Contour*>& contours);
};