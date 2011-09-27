#include "contour.hpp"
#include <iostream>

namespace Constants {
    const unsigned int contourSizeLimit = 10;
}

Contour::Contour(const cv::Rect& firstElement) :
    wasDrawn(false)
{
    this->subContours = std::vector<cv::Rect>();
    this->subContours.reserve(Constants::contourSizeLimit);
    this->subContours.push_back(firstElement);
    this->size = firstElement.width * firstElement.height;
}

bool Contour::includes(const cv::Rect& rect) const
{
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i != subContours.end(); ++i) {
        if(i->contains(rect.tl()) && i->contains(rect.br())) {
            return true;
        }
    }
    return false;
}


bool Contour::includes(const Contour& other) const
{
    for(std::vector<cv::Rect>::const_iterator i = other.subContours.begin(); i != other.subContours.end(); ++i) {
        if(!this->includes(*i)) {
            return false;
        }
    }
    return true;    
}

bool Contour::contains(const cv::Point_<int>& position) const
{
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i != subContours.end(); ++i) {
        if(i->contains(position)) {
            return true;
        }
    }
    return false;
}

bool Contour::intersects(const cv::Rect& rect) const
{
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i != subContours.end(); ++i) {
        const cv::Rect intersection = (*i) & rect;
        if((intersection.width != 0) || (intersection.height != 0)) {
            return true;
        }
    }
    return false;
}

bool Contour::intersects(const Contour& other) const
{
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i != subContours.end(); ++i) {
        if(other.intersects(*i)) {
            return true;
        }
    }
    return false;
}

int Contour::maxX() const
{
    int maxX = subContours.begin()->x + subContours.begin()->width;
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i != subContours.end(); ++i) {
        if (i->x + i->width > maxX) {
            maxX = i->x + i->width;
        }
    }
    return maxX;
}

int Contour::minX() const
{
    int minX = subContours.begin()->x;
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i != subContours.end(); ++i) {
        if (i->x < minX) {
            minX = i->x;
        }
    }
    return minX;
}

bool Contour::operator<(const Contour& other) const
{
    return this->minX() < other.minX();
}

bool Contour::mergeWith(const Contour& other)
{
    if(this->subContours.size() + other.subContours.size() > Constants::contourSizeLimit) {
        return false;
    }
    this->subContours.insert(this->subContours.end(), other.subContours.begin(), other.subContours.end());
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i != subContours.end(); ++i) {
        this->size += i->width * i->height;
    }
    return true;
}

void Contour::insertIntoLimitMap(std::vector<std::vector<Contour*> >& limitMap)
{
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i != subContours.end(); ++i) {
        for(int y = i->y; y != i->y+i->height; ++y) {
            for(int x = i->x; x != i->x+i->width; ++x) {
                if(limitMap[y][x]==NULL || this->size < limitMap[y][x]->size) {
                    limitMap[y][x] = this;
                }
            }
        }
    }
}

void Contour::drawOn(cv::Mat& output, const cv::Scalar boundingRectangleColor, const cv::Scalar subRectangleColor) const
{
    cv::Rect boundingRectangle = *subContours.begin();
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i!= subContours.end(); ++i) {
        boundingRectangle |= *i;
    }
    cv::rectangle(output, boundingRectangle, boundingRectangleColor, 2);
    for(std::vector<cv::Rect>::const_iterator i = subContours.begin(); i!= subContours.end(); ++i) {
        cv::rectangle(output, *i, subRectangleColor);
    }
}

std::vector<Contour*> Contour::collectContours(const cv::Mat_<uchar>& cannyImage)
{    
    cv::Mat_<uchar> cannyCopy = cannyImage.clone();
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(cannyCopy, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
    std::vector<Contour*> result;
    result.reserve(contours.size());
    for(std::vector<std::vector<cv::Point> >::const_iterator i = contours.begin(); i != contours.end(); ++i) {
        cv::Rect boundingRect = cv::Rect(cv::boundingRect(cv::Mat(*i)));
        result.push_back(new Contour(boundingRect));
    }
    return result;
}

void Contour::mergeOverlappingContours(std::vector<Contour*>& contours)
{    
    std::sort(contours.begin(), contours.end());
    for(std::vector<Contour*>::iterator i = contours.begin(); i != contours.end(); ++i) {
        std::vector<Contour*>::iterator j = i;
        ++j;
        for(const int iMaxX = (*i)->maxX(); j != contours.end() && (*j)->minX() <= iMaxX; ++j) {
            if(*i==*j) continue;
            if((*i)->intersects(**j) && (!(*i)->includes(**j)) && (!(*j)->includes(**i))) {
                if((*i)->mergeWith(**j)) {
                    //delete *j;
                    *j = *i;   
                }
            }
        }
    }
}

const std::vector<std::vector<Contour*> > Contour::buildLimitMap(const cv::Mat_<uchar>& cannyImage)
{
    std::vector<std::vector<Contour*> > result(cannyImage.rows, std::vector<Contour*>(cannyImage.cols, (Contour*)NULL));
#ifdef NO_CONTOURS
    return result;
#endif
    std::vector<Contour*> contours = Contour::collectContours(cannyImage);
    Contour::mergeOverlappingContours(contours);
    for(std::vector<Contour*>::iterator i = contours.begin(); i != contours.end(); ++i) {
        if(!(*i)->wasDrawn) {
            (*i)->insertIntoLimitMap(result);
            (*i)->wasDrawn = true;
        }
    }
    return result;
}
