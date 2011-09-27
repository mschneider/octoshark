#include <list>
#include <opencv/cv.h>
#include "contour.hpp"

inline float computeAngle(const float dx, const float dy)
{    
    if(dx == 0 && dy == 0)
        return 0;
    return atan2(dy,dx)*180/M_PI;    
}

inline float degreeToRadiant(const float degree)
{
    return M_PI*degree/180;
}

inline float length(cv::Vec2f vector) {
    return hypot(vector[0], vector[1]);
}

class Ray;

class PointOfInterest : public cv::Point {
public:
    PointOfInterest(int x, int y) : cv::Point(x, y) {};
    static bool isAt(const int x,const int y);
};

class Ray {
    //static tbb::concurrent_deque<Ray*> knownRays;
    const PointOfInterest start;
    const int sobelX;
    const int sobelY;
    const int shearingAngle;
    const cv::Vec2f slopeNotNormalized;
    const Contour* contour;
    std::vector<cv::Point > steps;
    int stepX;
    int stepY;
    int strokeWidth;
    void takeStepX(const float);
    void takeStepY(const float);
    float computeSlope(const float) const;
    bool furtherStepsArePossible() const;
    bool hitEdge() const;
    int currentPosX() const;
    int currentPosY() const;
    bool goalReached(const float goal, const int step) const;
    bool betweenParallelEdges() const;
    void drawPoint(const int x, const int y) const;
    void printSteps();
    static int maximumStrokeAngle;
    static int maximumStrokeWidth;
    static int maximumStrokeWidthSquared;
public:    
    const float slopeX;
    const float slopeY;
    Ray* build();
    void draw() const;
    void redraw();
    static std::list<Ray*> buildRays(const std::vector<std::vector<Contour*> >&);
    static void drawRays(std::list<Ray*>&);
    Ray(const PointOfInterest& start,
        const int sobelX,
        const int sobelY,
        const int shearingAngle,
        const Contour* const contour
    );
};
