#include <opencv/cv.h>

class LetterCandidate;
class LineCandidate;

class LetterCandidateConnection {
    friend class LineCandidate;
    double value;
    LetterCandidate* source;
    LetterCandidate* destination;
public:
    bool operator<(const LetterCandidateConnection& other) const;
    LetterCandidateConnection(LetterCandidate* source, LetterCandidate* destination, int direction);
};

class LetterCandidateGroup {
    friend class LineCandidate; //zugriff auf candidates
    std::vector<LetterCandidate*> candidates;
    double azimuth;
    bool hasBeenSelected;
public:
    const std::vector<LetterCandidate*> getCandidates() const { return candidates; };
    double getAzimuth() const { return azimuth; };
    bool checkAzimuthAgainst(const double otherAzimuth) const;
    bool canMergeWith(const LetterCandidateGroup* const other) const;
    void mergeWith(LetterCandidateGroup* other);
    LineCandidate* buildLineCandidate();
    LetterCandidateGroup(LetterCandidate* firstElement);
};

class LineCandidate {
    cv::Rect boundingRect;
    LetterCandidateGroup* group;
public:
    cv::RotatedRect getRotatedBoundingRect() const;
    const cv::Rect getBoundingRect() const { return boundingRect; };
    const LetterCandidateGroup* const getGroup() const { return group; };
    LineCandidate(LetterCandidateGroup* group);
    static std::vector<LineCandidate*> selectCandidates(std::vector<LetterCandidate*>& letterCandidates, std::vector<LetterCandidateConnection>& connectionsX, std::vector<LetterCandidateConnection>& connectionsY);
};

class LetterCandidate {
    friend class LetterCandidateGroup;
    friend class LineCandidate; //zugriff auf group
    bool hasOutgoingConnection;
    bool hasIncomingConnection;
    LetterCandidateGroup* group;
public:    
    const float averageStrokeWidth;
    const int numberOfPixels;
    const cv::Vec3i averageColor;
    const cv::Rect boundingRect;
    const cv::Point center;
    bool hasSimilarStrokeWidth(const LetterCandidate& other) const;
    bool hasSimilarProportions(const LetterCandidate& other) const;
    bool hasSimilarColor(const LetterCandidate& other) const;
    bool exceedsRangeOfByDirection(const LetterCandidate& other, const int direction) const;
    bool isInConnectivitySectorOf(const LetterCandidate& other, const int direction) const;
    void tryToConnectWith(LetterCandidate& other);
    
    LetterCandidate(const float averageStrokeWidth, const cv::Vec3i averageColor, int numberOfPixels, const cv::Rect boundingRect);
    static void sortByDirection(std::vector<LetterCandidate*>& letterCandidates, const int direction);
    static std::vector<LetterCandidateConnection> computeNeighbourhood(std::vector<LetterCandidate*>& letterCandidates, const int direction);
    static std::vector<LineCandidate*> identifyLineCandidates(std::vector<LetterCandidate*>& letterCandidates);
};



