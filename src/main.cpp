#include <sys/time.h>
#include "config.hpp"
#include "pictures.hpp"
#include "ray.hpp"
#include "component.hpp"
#include <iostream>
#include <fstream>
#include <set>
#include <opencv/highgui.h>

#define printTimeDiff(str, a, b) \
    timeval a; \
    gettimeofday(&a, NULL); \
    std::cout<<str<<" ran "<<a.tv_sec-b.tv_sec+(a.tv_usec-b.tv_usec)/1000000.0<<" seconds.\n"; \
    gettimeofday(&a, NULL);

#ifdef TEXT_OUTPUT
void initializeOutputFile(std::fstream& outputFileStream)
{
    outputFileStream.open(Config::outputFileName.c_str(), std::fstream::out | std::fstream::trunc);
    outputFileStream<<'#'<<Config::inputFileName<<std::endl;
}

void printToOutputFile(std::fstream& outputFileStream, const std::vector<LineCandidate*>& lineCandidates)
{
    for(std::vector<LineCandidate*>::const_iterator i = lineCandidates.begin(); i != lineCandidates.end(); ++i) {
        const cv::Rect currentBoundingBox = (*i)->getBoundingRect();
        outputFileStream<<currentBoundingBox.x<<','<<currentBoundingBox.y<<','<<currentBoundingBox.width<<','<<currentBoundingBox.height<<std::endl;
    }
}
#endif

#ifdef IMAGE_OUTPUT
void createExtractionImages(const std::vector<LineCandidate*>& lineCandidates)
{   //todo rotate right
    int x = 0;
    for(std::vector<LineCandidate*>::const_iterator i = lineCandidates.begin(); i != lineCandidates.end(); ++i) {
        // const cv::RotatedRect rotatedBoundingRect = (*i)->getRotatedBoundingRect();
        // const cv::Rect boundingRect = rotatedBoundingRect->boundingRect();
        // const cv::Point centerOffset(rotatedBoundingRect.center.x - boundingRect.x, rotatedBoundingRect.center.y - boundingRect.y);
        // const cv::Mat rotationMatrix = getRotationMatrix2D(centerOffset, rotatedBoundingRect.angle, 1.0);
        const cv::Rect boundingRect = (*i)->getBoundingRect();
        const cv::Mat extraction = cv::Mat(Pictures::original, boundingRect);// .clone();
        // cv::Mat rotatedExtraction;
        // cv::warpAffine(extraction, rotatedExtraction, rotationMatrix, rotatedBoundingRect.size);
        std::stringstream fileName;
        fileName<<IMAGE_OUTPUT<<x++<<".png";
        cv::imwrite(fileName.str(), extraction);
    }
}
#endif

int main(const int argc, const char** argv)
{
    timeval start;
    gettimeofday(&start, NULL);
    try {
        Config::initialize(argc, argv);
        Pictures::initialize();
    } catch (char* e) {
        std::cerr<<e<<std::endl<<"Aborting…\n";
        return -1;
    }
    printTimeDiff("init", initialize, start);

    const std::vector<std::vector<Contour*> > contourLimitMap = Contour::buildLimitMap(Pictures::canny);
    {
        std::set<Contour*> contoursSet;
        for(std::vector<std::vector<Contour*> >::const_iterator i = contourLimitMap.begin(); i != contourLimitMap.end(); ++i) {
            contoursSet.insert(i->begin(), i->end());
        }
        for(std::set<Contour*>::const_iterator i = contoursSet.begin(); i != contoursSet.end(); ++i) {
            if(*i) {
                (*i)->drawOn(Pictures::input, cv::Scalar(135,212,68), cv::Scalar(57,76,219));
            }
        }
    }
    printTimeDiff("contours", contours, initialize);

    std::list<Ray*> rays = Ray::buildRays(contourLimitMap); 
    printTimeDiff("buildRays", buildRays, contours);            

    Ray::drawRays(rays); 
    printTimeDiff("drawRays", drawRays, buildRays);

    const std::list<Component*> components = Component::findAll();
    std::set<const std::vector<Component*>*> equivalenceClasses = Component::collectEquivalenceClasses(components);
    std::vector<LetterCandidate*> letterCandidates = Component::identifyLetterCandidates(equivalenceClasses);
    printTimeDiff("identifyLetterCandidates", identifyLetterCandidates, drawRays);

    std::vector<LineCandidate*> lineCandidates = LetterCandidate::identifyLineCandidates(letterCandidates);
    printTimeDiff("identifyLineCandidates", identifyLineCandidates, identifyLetterCandidates);
    
    printTimeDiff("> the whole algorithm", end, start);
    
#ifdef TEXT_OUTPUT
    std::fstream outputFileStream;
    initializeOutputFile(outputFileStream);
    printToOutputFile(outputFileStream, lineCandidates);
    outputFileStream.close();
#endif
    
#ifdef IMAGE_OUTPUT
    createExtractionImages(lineCandidates);
#endif
    
#ifdef SHOW_PICTURES
    Pictures::show();
#endif
    return 0;
}
