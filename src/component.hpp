#include <list>
#include <set>
#include <vector>
#include "candidate.hpp"

class ConnectionTestRegion;

class Component {
    friend class ConnectionTestRegion;
    
    static std::vector<std::vector<Component*> > map;
    static std::list<Component*> list;
    static float groupingThreshold;

    std::vector<Component*>* equivalenceClass;
    double strokeWidthSum;
    int minX, maxX, minY, maxY, pixelCount;
    void updateBounds(const int x, const int y);
    
public:
    const double getStrokeWidthSum() { return strokeWidthSum; };
    const int getMinX() { return minX; };
    const int getMaxX() { return maxX; };
    const int getMinY() { return minY; };
    const int getMaxY() { return maxY; };
    const int getPixelCount() { return pixelCount; };
    const std::vector<Component*>* getEquivalenceClass() const { return equivalenceClass; };
    std::list<float> strokeWidths;
    std::list<cv::Point> coordinates;
    static const std::vector<std::vector<Component*> > getMap() { return map; };
    
    static std::list<Component*> findAll();
    static std::set<const std::vector<Component*>*> collectEquivalenceClasses(const std::list<Component*>&);
    static std::vector<LetterCandidate*> identifyLetterCandidates(const std::set<const std::vector<Component*>*>&);
    Component(const ConnectionTestRegion&);
    
    void addPixel(const ConnectionTestRegion&);
    void connectWith(Component&);
    bool isConnectedWith(const Component&) const;
};

class ConnectionTestRegion {
    Component * const leftComponent, * const topLeftComponent, * const topComponent, * const topRightComponent;
public:
    const int x, y;
    const float current, left, topLeft, top, topRight;
    ConnectionTestRegion(const int x, const int y);
    
    void connectAdjacentComponents();
    Component* calculateComponent();
};
