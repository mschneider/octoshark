#include "config.hpp"
#include "pictures.hpp"
#include "component.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>

namespace Constants {
    Component* nullComponent = (Component*) 0;
}

inline cv::Vec3i sumOfScalars(cv::Vec3i& a, cv::Vec3b& b)
{
	a[0] += b[0];
	a[1] += b[1];
	a[2] += b[2];
	return a;
}

inline cv::Vec3i operator/ (cv::Vec3i& a, int divident)
{
	a[0] /= divident;
	a[1] /= divident;
	a[2] /= divident;
	return a;
}

std::vector<std::vector<Component*> > Component::map;
// ungünstig, da dadurch nicht im caller initialisiert werden kann
std::list<Component*> Component::list;
float Component::groupingThreshold;

std::list<Component*> Component::findAll()
{    
    map = std::vector<std::vector<Component*> >(
        Pictures::strokes.rows,
        std::vector<Component*>(
            Pictures::strokes.cols,
            Constants::nullComponent));
    list = std::list<Component*>();
    groupingThreshold = (float) Config::variables["groupingThreshold1"] / Config::variables["groupingThreshold2"];
    for(int y=0; y<Pictures::strokes.rows; ++y) {
        for(int x=0; x<Pictures::strokes.cols; ++x) {
            ConnectionTestRegion region(x, y);
            region.connectAdjacentComponents();
            Component* current = region.calculateComponent();
            if(current != Constants::nullComponent)
               current->updateBounds(x, y);
            map[y][x] = current;
        }
    }
    return list;
}

std::set<const std::vector<Component*>*> Component::collectEquivalenceClasses(const std::list<Component*>& components)
{
    std::set<const std::vector<Component*>*> equivalenceClasses;
    for(std::list<Component*>::const_iterator i = components.begin(); i != components.end(); ++i) {
        equivalenceClasses.insert((*i)->getEquivalenceClass());
    }
    return equivalenceClasses;
}

std::vector<LetterCandidate*> Component::identifyLetterCandidates(const std::set<const std::vector<Component*>*>& equivalenceClasses)
{
    std::vector<LetterCandidate*> result;
    result.reserve(equivalenceClasses.size());
    const double maximumStrokeWidth = Config::variables["maximumStrokeWidth"]; //bad
    const double maximumStrokeVariance = Config::variables["maximumStrokeVariance"];
    cv::Vec3i letterColor = cv::Vec3i(0,0,0);
    for(std::set<const std::vector<Component*> * >::iterator j = equivalenceClasses.begin(); j != equivalenceClasses.end(); ++j) {
        int minX, minY, maxX, maxY;
        double strokeWidthSum = 0;
        int pixelCount = 0;
        std::list<float> strokeWidths = std::list<float>();
        minX = minY = 9999999;
        maxX = maxY = -1;
        for(std::vector<Component*>::const_iterator i = (*j)->begin(); i != (*j)->end(); ++i) {
            if((*i)->getMinX() < minX) minX = (*i)->getMinX();
            if((*i)->getMinY() < minY) minY = (*i)->getMinY();
            if((*i)->getMaxX() > maxX) maxX = (*i)->getMaxX();
            if((*i)->getMaxY() > maxY) maxY = (*i)->getMaxY();
            strokeWidthSum += (*i)->getStrokeWidthSum();
            pixelCount += (*i)->getPixelCount();
            strokeWidths.splice(strokeWidths.begin(), (*i)->strokeWidths);
          
            for(std::list<cv::Point>::iterator k = (*i)->coordinates.begin(); k != (*i)->coordinates.end(); ++k) {
            	letterColor=sumOfScalars(letterColor, Pictures::original.at<cv::Vec3b>(k->y, k->x));
            }
        }
        const double averageStrokeWidth = (strokeWidthSum / pixelCount) * maximumStrokeWidth;
        cv::Vec3i averageLetterColor = letterColor / pixelCount;
        double variance = 0;
        for(std::list<float>::iterator i = strokeWidths.begin(); i != strokeWidths.end(); ++i) {
            const double difference = averageStrokeWidth - maximumStrokeWidth * (*i);
            variance += difference * difference;
        }
        variance = variance / pixelCount;
        const double width = maxX-minX+1;
        const double height = maxY-minY+1;
        if ( height > 8 
            // && height < 10*averageStrokeWidth 
            // && height > 2*averageStrokeWidth
            // && width < 10*averageStrokeWidth
            // && variance <= maximumStrokeVariance
        ) {
#ifdef DRAW_COMPONENTS
            cv::rectangle(Pictures::original, cv::Rect(minX, minY, width, height), cv::Scalar(0,0,255));
#endif
            result.push_back(new LetterCandidate(averageStrokeWidth, averageLetterColor, pixelCount, cv::Rect(minX, minY, width, height)));
    	}
    }
    return result;
}

Component::Component(const ConnectionTestRegion& region) :
    minX(region.x),
    maxX(region.x),
    minY(region.y),
    maxY(region.y),
    pixelCount(1),
    strokeWidthSum(region.current)
{
    this->equivalenceClass = new std::vector<Component*>();
    this->equivalenceClass->push_back(this);
    this->strokeWidths = std::list<float>();
    this->strokeWidths.push_back(region.current);
    this->coordinates = std::list<cv::Point>();
    this->coordinates.push_back(cv::Point(region.x, region.y));
    list.push_back(this);
}

void Component::addPixel(const ConnectionTestRegion& region)
{    
    this->strokeWidths.push_back(region.current);
    this->strokeWidthSum += region.current;
    this->coordinates.push_back(cv::Point(region.x, region.y));
    this->pixelCount++;
}

void Component::updateBounds(const int x, const int y)
{
    if(x<this->minX) this->minX = x;
    if(y<this->minY) this->minY = y;
    if(x>this->maxX) this->maxX = x;
    if(y>this->maxY) this->maxY = y;
}

void Component::connectWith(Component& other)
{
    std::vector<Component*> *otherEquivalenceClass = other.equivalenceClass;
    if(otherEquivalenceClass->size() > this->equivalenceClass->size())
        return other.connectWith(*this);
    this->equivalenceClass->reserve(this->equivalenceClass->size() + otherEquivalenceClass->size());
    this->equivalenceClass->insert(this->equivalenceClass->end(), otherEquivalenceClass->begin(), otherEquivalenceClass->end());
    for(std::vector<Component*>::iterator i = otherEquivalenceClass->begin(); i != otherEquivalenceClass->end(); ++i)
        (*i)->equivalenceClass = this->equivalenceClass;
    // delete otherEquivalenceClass;
}

bool Component::isConnectedWith(const Component& other) const
{
    return (this == &other) || (this->equivalenceClass == other.equivalenceClass);
}

ConnectionTestRegion::ConnectionTestRegion(const int x, const int y) :
    x(x),
    y(y),
    current(Pictures::strokes.at<float>(y, x)),
    left(x>0                                    ? Pictures::strokes.at<float>(y, x-1)   : Constants::strokeBackground),
    topLeft(x>0 && y>0                          ? Pictures::strokes.at<float>(y-1, x-1) : Constants::strokeBackground),
    top(y>0                                     ? Pictures::strokes.at<float>(y-1, x)   : Constants::strokeBackground),
    topRight(y>0 && x<Pictures::strokes.cols-1  ? Pictures::strokes.at<float>(y-1, x+1) : Constants::strokeBackground),
    leftComponent(x>0                                   ? Component::map[y][x-1]   : Constants::nullComponent),
    topLeftComponent(x>0 && y>0                         ? Component::map[y-1][x-1] : Constants::nullComponent),
    topComponent(y>0                                    ? Component::map[y-1][x]   : Constants::nullComponent),
    topRightComponent(y>0 && x<Pictures::strokes.cols-1 ? Component::map[y-1][x+1] : Constants::nullComponent)
    {}

/*
    Mögliche Fälle sind:
    l&ol, l&o, l&or
    ol&o, ol&or
    o&or 
    Achtung: div by Zero
*/
void ConnectionTestRegion::connectAdjacentComponents()
{
    const float strokeWidths[] = {left, topLeft, top, topRight};
    Component* components[] = {leftComponent, topLeftComponent, topComponent, topRightComponent};
    for(int i=0; i != sizeof(components)/sizeof(Component*); ++i) {
        for(int j=i+1; j != sizeof(components)/sizeof(Component*); ++j) {
            // being aware of nullComponents
            if(components[i]&&components[j]&&!components[i]->isConnectedWith(*components[j])) {
                const float ratio = strokeWidths[i]>strokeWidths[j] ? strokeWidths[i]/strokeWidths[j] : strokeWidths[j]/strokeWidths[i];
                if(ratio < Component::groupingThreshold) {
                    components[i]->connectWith(*components[j]);
                }
            }
        }
    }
}

/*
    Achtung: div by Zero
*/
Component* ConnectionTestRegion::calculateComponent()
{
    if(current == Constants::strokeBackground)
        return Constants::nullComponent;
    const float leftRatio = left>current ? left/current : current/left;
    const float topLeftRatio = topLeft>current ? topLeft/current : current/topLeft;
    const float topRatio = top>current ? top/current : current/top;
    const float topRightRatio = topRight>current ? topRight/current : current/topRight;
    const float ratios[] = {leftRatio, topLeftRatio, topRatio, topRightRatio};
    const float minimumRatio = *std::min_element(ratios, ratios+4);
    if(minimumRatio < Component::groupingThreshold) {
        Component* calculatedComponent = NULL;
        if(minimumRatio == leftRatio)
            calculatedComponent = leftComponent;
        if(minimumRatio == topLeftRatio)
            calculatedComponent = topLeftComponent;
        if(minimumRatio == topRatio)
            calculatedComponent = topComponent;
        if(minimumRatio == topRightRatio)
            calculatedComponent = topRightComponent;
        assert(calculatedComponent != NULL);// "minimumRatio should still be contained in ratios."
        calculatedComponent->addPixel(*this);
        return calculatedComponent;
    }
    return new Component(*this);
}
