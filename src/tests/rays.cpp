#include "../ray.hpp"
#include <algorithm>
#include <iostream>
#include <cassert>
#include <cmath>

extern float computeAngle(const float, const float);

void testSlopeAngles() 
{
    PointOfInterest poi(0, 0);
    Contour contour(cv::Rect(0,0,1,1));
    const int sobelXs[] =    { 1, 1, 0, -1, -1,  -1,  0,  1 };
    const int sobelYs[] =    { 0, 1, 1,  1,  0,  -1, -1, -1 };
    const int shearAngles[] ={ 0, 0, 0,  0,  0,   0,  0,  0 };
    const int resultAngles[]={ 0,45,90,135,180,-135,-90,-45 };
    for(int i = 0; i != sizeof(sobelXs)/sizeof(int); ++i) {
        Ray ray(poi, sobelXs[i], sobelYs[i], shearAngles[i], &contour);
        assert(computeAngle(ray.slopeX, ray.slopeY) == resultAngles[i]);
    }
};

void testRayBuilding()
{

};

main() {
    testSlopeAngles();
    std::cout << "Everything fine!" << std::endl;
}