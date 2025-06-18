#pragma once
#include <string>
#include <fstream>

class Config {
public:
    std::string file1 = "layout.sp";
    std::string topCell1 = "SOP_DC_X128Y8_620"; // if can't find cell name， use main cell default
    std::string file2 = "layout2.sp";
    std::string topCell2 = "SOP_DC_X128Y8_620"; // if can't find cell name， use main cell default
    std::string outputFileName = "";

    bool caseInsensitive = false;
    bool hier = 1;
    bool autoMatch = false; // Temporarily unavailable
    bool multiThread = 1;
    double tolerance = 1e-6;

    static Config& GetInstance() {
        static Config instance;
        return instance;
    }
};

class Debug {
public:
    bool executeWLShow = 1;
    bool iterateShow = 1;
    bool cellShow = 0;

    static Debug& GetInstance() {
        static Debug instance;
        return instance;
    }
};