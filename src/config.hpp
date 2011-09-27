#include <map>
#include <string>

namespace Config {
    void initialize(const int, const char**);
    void readConfigFile();
    
    extern std::string inputFileName;
    extern std::string outputFileName;
    extern std::map<std::string, int> variables;
}
