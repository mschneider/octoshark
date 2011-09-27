#include "ray.hpp"
#include "pictures.hpp"
#include "config.hpp"
#include <cmath>
#include <iostream>

namespace Constants {
    Ray* nullRay = (Ray*) 0;
}

int Ray::maximumStrokeAngle;
int Ray::maximumStrokeWidth;
int Ray::maximumStrokeWidthSquared;

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

bool PointOfInterest::isAt(const int x, const int y)
{
    return Pictures::canny.at<uchar>(y, x) != 0;
}

void Ray::printSteps()
{
    for(std::vector<cv::Point >::const_iterator i = this->steps.begin();  i != this->steps.end(); ++i) {
        std::cout<<i->x<<", "<<i->y<<std::endl;
    }
}

Ray::Ray(const PointOfInterest& start, const int sobelX, const int sobelY, const int shearingAngle, const Contour* const contour) : 
    start(start),
    sobelX(sobelX),
    sobelY(sobelY),
    shearingAngle(shearingAngle),
    //additionstheorem für sinus und cosinus
    slopeNotNormalized(
        sobelX * cos(degreeToRadiant(shearingAngle)) - sobelY * sin(degreeToRadiant(shearingAngle)),
        sobelY * cos(degreeToRadiant(shearingAngle)) + sobelX * sin(degreeToRadiant(shearingAngle))
    ),
    slopeX(slopeNotNormalized[0] / length(slopeNotNormalized)),
    slopeY(slopeNotNormalized[1] / length(slopeNotNormalized)),
    contour(contour)
    {}

void Ray::redraw() {
    std::vector<float> strokeWidths;
    strokeWidths.reserve(this->strokeWidth * 2);
    int actPosX = this->start.x;
    int actPosY = this->start.y;
    strokeWidths.push_back(Pictures::strokes.at<float>(actPosY, actPosX));
    for(std::vector<cv::Point>::const_iterator i = steps.begin(); i != steps.end(); ++i) {
        actPosX += i->x;
        actPosY += i->y;
        strokeWidths.push_back(Pictures::strokes.at<float>(actPosY, actPosX));
    }
    std::sort(strokeWidths.begin(), strokeWidths.end());
    this->strokeWidth = strokeWidths[strokeWidths.size()/2] * Config::variables["maximumStrokeWidth"];
    this->draw();
}

void Ray::draw() const
{
    int actPosX = this->start.x;
    int actPosY = this->start.y;
    this->drawPoint(actPosX, actPosY);
    for(std::vector<cv::Point>::const_iterator i = steps.begin(); i != steps.end(); ++i) {
        actPosX += i->x;
        actPosY += i->y;
        this->drawPoint(actPosX, actPosY);
    }
}

void Ray::drawPoint(const int x, const int y) const
{
    const float strokeValue = (float) this->strokeWidth / Ray::maximumStrokeWidth;
    if(Pictures::strokes.at<float>(y, x) > strokeValue) {
        Pictures::strokes.at<float>(y, x) = strokeValue;
    }
}

Ray* Ray::build()
{
    float goalX, goalY;
    goalX = stepX = goalY = stepY = 0;
    steps = std::vector<cv::Point >();
    steps.reserve(Ray::maximumStrokeWidth);
    while(furtherStepsArePossible()) {
        if(hitEdge()) {
            break;
        }
        if(!goalReached(goalX, stepX)) {
            takeStepX(goalX);
        } else if(!goalReached(goalY, stepY)) {
            takeStepY(goalY);
        }
        if(goalReached(goalX, stepX) && goalReached(goalY, stepY)) {
            goalX += slopeX;
            goalY += slopeY;
        }
    }
    // need to check wheter we left because loop condition is invalid or we breaked
    if(furtherStepsArePossible() && hitEdge() && betweenParallelEdges()) {
        strokeWidth = sqrt(stepX * stepX + stepY * stepY);
        return this;
    } else {
        //delete this;
        return Constants::nullRay;
    }
}

void Ray::takeStepX(const float goalX)
{
    const int goalSign = goalX > 0 ? 1 : -1;
    stepX += goalSign;
    steps.push_back(cv::Point(goalSign, 0));
}

void Ray::takeStepY(const float goalY)
{
    const int goalSign = goalY > 0 ? 1 : -1;
    stepY += goalSign;
    steps.push_back(cv::Point(0, goalSign));
}

bool Ray::betweenParallelEdges() const
{
    const int endX = currentPosX();
    const int endY = currentPosY();
    const short endSobelX = Pictures::sobelX.at<short>(endY, endX);
    const short endSobelY = Pictures::sobelY.at<short>(endY, endX);
    //still needed?
    if(endSobelX == 0 && endSobelY == 0) return false;
    float startAngle = computeAngle(this->sobelX, this->sobelY);
    float endAngle = computeAngle(endSobelX, endSobelY);
    if(startAngle < 0) startAngle += 180;
    if(endAngle < 0) endAngle += 180;
    const float difference1 = fabs(startAngle - endAngle);
    const float difference2 = 180 - std::max(startAngle, endAngle) + std::min(startAngle, endAngle);
    return std::min(difference1, difference2) <= Ray::maximumStrokeAngle;
}

int Ray::currentPosX() const
{
    return start.x + stepX;
}

int Ray::currentPosY() const
{
    return start.y + stepY;
}

bool Ray::goalReached(const float goal, const int step) const
{
    if(goal == 0)
        return true;
    if(goal > 0) {
        return step >= goal;
    } else {
        return step <= goal;
    }
}

bool Ray::furtherStepsArePossible() const
{
    const bool isNotTooLong = stepX * stepX + stepY * stepY < Ray::maximumStrokeWidthSquared;
    const bool isInsideX = currentPosX() > 0 && currentPosX() + 1 < Pictures::canny.cols;
    const bool isInsideY = currentPosY() > 0 && currentPosY() + 1 < Pictures::canny.rows;
    if(this->contour == NULL) {
        return isNotTooLong && isInsideX && isInsideY;
    } else {
        return isNotTooLong && isInsideX && isInsideY && this->contour->contains(cv::Point(currentPosX(), currentPosY()));
    }
}

bool Ray::hitEdge() const
{
    const bool farEnoughAway = abs(stepX) + abs(stepY) > 2;
    const bool overEdgePixel = Pictures::canny.at<uchar>(currentPosY(), currentPosX());
    return farEnoughAway && overEdgePixel;
    
}

float Ray::computeSlope(const float sobel) const 
{
    if(this->sobelX == 0)
        return sobel / fabs(this->sobelY);
    if(this->sobelY == 0)
        return sobel / fabs(this->sobelX);
    if(fabs(this->sobelX) >= fabs(this->sobelY))
        return sobel / fabs(this->sobelX);
    else
        return sobel / fabs(this->sobelY);
}

std::list<Ray*> Ray::buildRays(const std::vector<std::vector<Contour*> >& contourLimitMap)
{
    std::list<Ray*> rays;
    Ray::maximumStrokeAngle = Config::variables["maximumStrokeAngle"];
    Ray::maximumStrokeWidth = Config::variables["maximumStrokeWidth"];
    Ray::maximumStrokeWidthSquared = Ray::maximumStrokeWidth * Ray::maximumStrokeWidth;
    const int shearingAngles[] = {SHEARING_ANGLES};
    for(int y=0; y<Pictures::canny.rows; ++y) {
        for(int x=0; x<Pictures::canny.cols; ++x) {
            if(PointOfInterest::isAt(x,y)) {
                PointOfInterest poi(x, y);
                const int sobelX = Pictures::sobelX.at<short>(y, x);
                const int sobelY = Pictures::sobelY.at<short>(y, x);
                const Contour* contour = contourLimitMap[y][x];
                for(int i=0; i != sizeof(shearingAngles)/sizeof(int); ++i) {
                    Ray* forwards = new Ray(poi, sobelX, sobelY, shearingAngles[i], contour);
                    forwards = forwards->build();
                    if(forwards != NULL) 
                        rays.push_back(forwards);
                    Ray* backwards = new Ray(poi, -sobelX, -sobelY, shearingAngles[i], contour);
                    backwards = backwards->build();
                    if(backwards != NULL)
                        rays.push_back(backwards);
                }
            }
        }
    }
    return rays;
}

void Ray::drawRays(std::list<Ray*>& rays)
{
    for(std::list<Ray*>::iterator i = rays.begin(); i != rays.end(); ++i) {
        (*i)->draw();
    }
    for(std::list<Ray*>::iterator i = rays.begin(); i != rays.end(); ++i) {
        (*i)->redraw();
    }
}
