#include <cstdlib>
#include "config.hpp"
#include <fstream>

namespace Config {
    std::string inputFileName;
    std::string outputFileName;
    std::map<std::string, int> variables;
}

void Config::initialize(const int argc, const char** argv)
{
    Config::readConfigFile();
    if(argc != 2)
        throw "Wrong parameter count. Please supply a filename.";
    Config::inputFileName = argv[1];
#ifdef TEXT_OUTPUT    
    const size_t fileExtensionIndex = Config::inputFileName.rfind('.');
    if(fileExtensionIndex != std::string::npos) {
        Config::outputFileName = TEXT_OUTPUT;
        Config::outputFileName.append(Config::inputFileName.begin(), Config::inputFileName.begin()+fileExtensionIndex);
        Config::outputFileName.append(".txt");
    } else {
        Config::outputFileName = std::string(argv[1]).append(".txt");
    }
#endif
}

void Config::readConfigFile()
{
    std::string line;
    std::fstream configStream("config.ini", std::fstream::in);
    if(!configStream.is_open())
        throw "config.ini could not be opened.";
    while(!configStream.eof()) {
        getline(configStream, line);
        Config::variables.insert(std::make_pair(
            line.substr(0, line.find('=')),
            std::atoi(line.substr(line.find('=') + 1, line.length()).c_str())
        ));
    }
    configStream.close();
}
