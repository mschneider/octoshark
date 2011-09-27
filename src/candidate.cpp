#include "candidate.hpp"
#include "pictures.hpp"
#include <set>
#include <cmath>
#include <iostream>

namespace Constants {
    const int xDirection = 1;
    const int yDirection = 2;
    const unsigned int minimumLineSize = 2;
}

inline bool orderLetterCandidatesX(const LetterCandidate* const a, const LetterCandidate* const b)
{
	return a->boundingRect.x < b->boundingRect.x;
}

inline bool orderLetterCandidatesY(const LetterCandidate* const a, const LetterCandidate* const b)
{
	return a->boundingRect.y < b->boundingRect.y;
}

inline bool orderLetterCandidatesCenter(const LetterCandidate* const a, const LetterCandidate* const b)
{
    return a->center.x < b->center.x;
}

inline double normalizedAzimuthOf(cv::Point distance)
{
    const double azimuth = atan2(distance.y, distance.x);
    return azimuth < 0 ? azimuth + M_PI : azimuth;
}

LetterCandidateConnection::LetterCandidateConnection(LetterCandidate* source, LetterCandidate* destination, int direction) :
    source(source),
    destination(destination)
{
    this->value = 3 * abs(source->center.x - destination->center.x) + 7 * abs(source->center.y - destination->center.y);
}

bool LetterCandidateConnection::operator<(const LetterCandidateConnection& other) const
{
    return this->value < other.value;
}

LetterCandidateGroup::LetterCandidateGroup(LetterCandidate* firstElement) :
    hasBeenSelected(false)
{
    candidates = std::vector<LetterCandidate*>();
    candidates.push_back(firstElement);
}

bool LetterCandidateGroup::checkAzimuthAgainst(const double otherAzimuth) const
{
    const double difference1 = fabs(this->azimuth - otherAzimuth);
    const double difference2 = M_PI - std::max(this->azimuth, otherAzimuth) + std::min(this->azimuth, otherAzimuth);
    const double azimuthDifference = std::min(difference1, difference2);
    return azimuthDifference < M_PI_4/2;
}

bool LetterCandidateGroup::canMergeWith(const LetterCandidateGroup* const other) const
{
    if(this->candidates.size() == 1 && other->candidates.size() == 1)
        return true;
    if(this->candidates.size() == 1)
        return other->canMergeWith(this);
    if(other->candidates.size() > 1) //implicit: this->candidates.size() > 1
        return this->checkAzimuthAgainst(other->azimuth);
    //implicit: this->candidates.size() > 1 && other->candidates.size() = 1
    //implicit: this->candidates is sorted by orderLetterCandidatesCenter
    const cv::Point distanceToFront = this->candidates.front()->center - other->candidates.front()->center;
    const cv::Point distanceToBack = this->candidates.back()->center - other->candidates.front()->center;
    return this->checkAzimuthAgainst(normalizedAzimuthOf(distanceToFront)) && this->checkAzimuthAgainst(normalizedAzimuthOf(distanceToBack));
}

void LetterCandidateGroup::mergeWith(LetterCandidateGroup* other)
{
    if(other->candidates.size() > this->candidates.size())
        return other->mergeWith(this);
    this->candidates.reserve(this->candidates.size() + other->candidates.size());
    this->candidates.insert(this->candidates.end(), other->candidates.begin(), other->candidates.end());
    for(std::vector<LetterCandidate*>::iterator i = other->candidates.begin(); i != other->candidates.end(); ++i)
        (*i)->group = this;
    //delete other;
    std::sort(this->candidates.begin(), this->candidates.end(), orderLetterCandidatesCenter);
    this->azimuth = normalizedAzimuthOf(this->candidates.back()->center - this->candidates.front()->center);
}

LineCandidate* LetterCandidateGroup::buildLineCandidate()
{
    if(this->hasBeenSelected) return NULL;
    if(this->candidates.size() < Constants::minimumLineSize) return NULL;
    this->hasBeenSelected = true;
    return new LineCandidate(this);
}

LineCandidate::LineCandidate(LetterCandidateGroup* group) :
    group(group)
{
    this->boundingRect = group->candidates.front()->boundingRect;
    for(std::vector<LetterCandidate*>::iterator i = group->candidates.begin(); i != group->candidates.end(); ++i) {
        this->boundingRect |= (*i)->boundingRect;
    }
}

cv::RotatedRect LineCandidate::getRotatedBoundingRect() const
{
    std::vector<cv::Point> letterBoundingPoints;
    letterBoundingPoints.reserve(4*group->candidates.size());
    for(std::vector<LetterCandidate*>::const_iterator i = group->candidates.begin(); i != group->candidates.end(); ++i) {
        const cv::Rect& rect = (*i)->boundingRect;
        letterBoundingPoints.push_back(cv::Point(rect.x, rect.y));
        letterBoundingPoints.push_back(cv::Point(rect.x+rect.width-1, rect.y));
        letterBoundingPoints.push_back(cv::Point(rect.x+rect.width-1, rect.y+rect.height-1));
        letterBoundingPoints.push_back(cv::Point(rect.x, rect.y+rect.height-1));
    }
   cv::RotatedRect result = cv::minAreaRect(cv::Mat(letterBoundingPoints));
   return result;
}

std::vector<LineCandidate*> LineCandidate::selectCandidates(std::vector<LetterCandidate*>& letterCandidates, std::vector<LetterCandidateConnection>& connectionsX, std::vector<LetterCandidateConnection>& connectionsY)
{
    std::vector<LineCandidate*> result;
    std::vector<LetterCandidateConnection> connections;
    connections.reserve(connectionsX.size() + connectionsY.size());
    connections.insert(connections.end(), connectionsX.begin(), connectionsX.end());
    connections.insert(connections.end(), connectionsY.begin(), connectionsY.end());
    std::sort(connections.begin(), connections.end());
    for(std::vector<LetterCandidateConnection>::const_iterator i = connections.begin(); i != connections.end(); ++i) {       
        i->source->tryToConnectWith(*i->destination);
    }
    for(std::vector<LetterCandidate*>::const_iterator i = letterCandidates.begin(); i != letterCandidates.end(); ++i) {
        LineCandidate* const newCandidate = (*i)->group->buildLineCandidate();
        if(newCandidate != NULL) {
            result.push_back(newCandidate);
        }
    }
    return result;
}

LetterCandidate::LetterCandidate(const float averageStrokeWidth, const cv::Vec3i averageColor, int numberOfPixels, const cv::Rect boundingRect) :
    averageStrokeWidth(averageStrokeWidth),
    averageColor(averageColor),
    numberOfPixels(numberOfPixels),
    boundingRect(boundingRect),
    center(((boundingRect.br()-boundingRect.tl()) * 0.5) + boundingRect.tl()),
    hasOutgoingConnection(false),
    hasIncomingConnection(false)
{
    group = new LetterCandidateGroup(this);
}

bool LetterCandidate::hasSimilarStrokeWidth(const LetterCandidate& other) const
{
    const float strokeWidthRatio = std::max(this->averageStrokeWidth, other.averageStrokeWidth) / std::min(this->averageStrokeWidth, other.averageStrokeWidth);
    return strokeWidthRatio < 2;
}

bool LetterCandidate::hasSimilarProportions(const LetterCandidate& other) const
{
    const float heightRatio = std::max(this->boundingRect.height, other.boundingRect.height) / std::min(this->boundingRect.height, other.boundingRect.height);
    return heightRatio < 2;  
} 

bool LetterCandidate::hasSimilarColor(const LetterCandidate& other) const
{
    const cv::Vec3i ownColor = this->averageColor;
    const cv::Vec3i otherColor = other.averageColor;
    return abs(ownColor[0]-otherColor[0])+abs(ownColor[1]-otherColor[1])+abs(ownColor[2]-otherColor[2]) < 250;
}

bool LetterCandidate::isInConnectivitySectorOf(const LetterCandidate& other, const int direction) const
{
    switch(direction) {
        case Constants::xDirection:
#ifdef HORIZONTAL_LINES_WITH_FRUSTUM
            return 1 * this->center.x - other.center.x > 3 * (abs(other.center.y - this->center.y) - this->boundingRect.height / 2);
#endif
#ifdef HORIZONTAL_LINES_WITH_CONE
            return 2 * this->center.x - other.center.x > HORIZONTAL_LINES_WITH_CONE * (abs(other.center.y - this->center.y) - this->boundingRect.height / 2);
#endif
            return this->center.x - other.center.x > abs(other.center.y - this->center.y);
        case Constants::yDirection:
            return this->center.y - other.center.y > abs(this->center.x - other.center.x);
        default:
            assert(false);
            return false;
    }
}

void LetterCandidate::tryToConnectWith(LetterCandidate& other)
{
#ifdef DRAW_LETTER_CONNECTIONS    
    cv::line(Pictures::original, this->center, other.center, cv::Scalar(0,200,220), 1, CV_AA);
#endif
    if((!this->hasOutgoingConnection) && (!other.hasIncomingConnection)) {
        if(this->group->canMergeWith(other.group)) {
#ifdef DRAW_LETTER_GROUPS    
            cv::line(Pictures::original, this->center, other.center, cv::Scalar(0,200,40), 2, CV_AA);
#endif
            this->group->mergeWith(other.group);
            this->hasOutgoingConnection = other.hasIncomingConnection = true;
        }
    }
}


bool LetterCandidate::exceedsRangeOfByDirection(const LetterCandidate& other, const int direction) const
{
    switch(direction) {
        case Constants::xDirection: {
            const int dx = this->boundingRect.tl().x - other.boundingRect.br().x;
            return dx > this->averageStrokeWidth*5;
        }
        case Constants::yDirection: {
            const int dy = this->boundingRect.tl().y - other.boundingRect.br().y;
            return dy > this->averageStrokeWidth*5;
        }
        default:
            assert(false);
            return false;
    }    
}

void LetterCandidate::sortByDirection(std::vector<LetterCandidate*>& letterCandidates, const int direction)
{
    switch(direction) {
        case Constants::xDirection:
    	    std::sort(letterCandidates.begin(), letterCandidates.end(), orderLetterCandidatesX);
            return;
        case Constants::yDirection:
    	    std::sort(letterCandidates.begin(), letterCandidates.end(), orderLetterCandidatesY);
            return;
        default:
            assert(false);
            return;
    }
}

std::vector<LetterCandidateConnection> LetterCandidate::computeNeighbourhood(std::vector<LetterCandidate*>& letterCandidates, const int direction)
{
    std::vector<LetterCandidateConnection> connections;
#if defined(HORIZONTAL_LINES_WITH_FRUSTUM) || defined (HORIZONTAL_LINES_WITH_FRUSTUM_WITH_CONE)
    if(direction == Constants::yDirection)
        return connections;
#endif
    connections.reserve(letterCandidates.size()); // mal quadrat ausprobieren
    LetterCandidate::sortByDirection(letterCandidates, direction);
    for(std::vector<LetterCandidate*>::const_iterator i = letterCandidates.begin(); i != letterCandidates.end(); ++i) {
        std::vector<LetterCandidate*>::const_iterator j = i;
        ++j;
        for(;j != letterCandidates.end(); ++j) {
            if((*j)->exceedsRangeOfByDirection(**i, direction)) break;
            if((*j)->isInConnectivitySectorOf(**i, direction)
                && (*i)->hasSimilarStrokeWidth(**j)
                && (*i)->hasSimilarProportions(**j)
                && (*i)->hasSimilarColor(**j)
            ){
                connections.push_back(LetterCandidateConnection(*i, *j, direction));
            }
        }
    }
    return connections;
}

std::vector<LineCandidate*> LetterCandidate::identifyLineCandidates(std::vector<LetterCandidate*>& letterCandidates)
{
    std::vector<LetterCandidateConnection> connectionsX = LetterCandidate::computeNeighbourhood(letterCandidates, Constants::xDirection);
    std::vector<LetterCandidateConnection> connectionsY = LetterCandidate::computeNeighbourhood(letterCandidates, Constants::yDirection);
    std::vector<LineCandidate*> result = LineCandidate::selectCandidates(letterCandidates, connectionsX, connectionsY);
    return result;
}
