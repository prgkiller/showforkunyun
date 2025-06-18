#include <fstream>
#include <memory>
#include "base.h"

std::vector<std::string> SplitString(const std::string& str) {
    std::vector<std::string> result;
    std::string token;
    std::istringstream strStream(str);
    while (strStream >> token) {
        result.emplace_back(token);
    }
    return result;
}

bool MatchNoCase(const std::string& str1, const std::string& str2) {
    if (str1.size() != str2.size()) {
        return false;
    }
    for (int i = 0; i < str1.size(); ++i) {
        if (tolower(str1[i]) != tolower(str2[i])) {
            return false;
        }
    }
    return true;
}

void SettingConfig(TestCase& testCase) {
    Config& config = Config::GetInstance();
    Debug& debug = Debug::GetInstance();

    config.file1 = std::move(testCase.file1);
    config.topCell1 = std::move(testCase.topCell1);
    config.file2 = std::move(testCase.file2);
    config.topCell2 = std::move(testCase.topCell2);
    config.outputFileName = testCase.outputFileName;
    config.caseInsensitive = testCase.caseInsensitive;
    config.hier = testCase.hier;
    config.autoMatch = testCase.autoMatch;
    config.multiThread = testCase.multiThread;
    config.tolerance = testCase.tolerance;

    debug.executeWLShow = testCase.executeWLShow;
    debug.iterateShow = testCase.iterateShow;
    debug.cellShow = testCase.cellShow;
}

void OutputToFileOrTerminal() {
    std::string& outputFileName = Config::GetInstance().outputFileName;
    if (!outputFileName.empty()) {
        std::ofstream outFile(outputFileName);
        outFile << OUT.str() << std::endl;
    } else {
        std::cout << OUT.str() << std::endl;
    }
}
