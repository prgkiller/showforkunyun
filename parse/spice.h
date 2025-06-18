#pragma once

#include <fstream>
#include "../netlist/netlist.h"

class Netlist;
class Spice {
private:
    std::shared_ptr<Netlist> _netlist;

    std::unique_ptr<std::fstream> _nowFile;
    std::string _fileName;
    std::shared_ptr<Cell> _mainCell;
    std::shared_ptr<Cell> _nowCell;
    std::string _nextToken;
    uint32_t _lineNum;
    std::string _line;
    std::vector<std::string> _lineTokens;
    size_t _tokenIndex;
private:
    /* copy code begin */
    bool EndFile() const;
    bool OpenFile(const std::string& name);
    void CloseFile();

    READ_STATE ReadSpice();

    READ_STATE ReadSubckt();
    // bool ReadQ();
    READ_STATE ReadM();
    // bool ReadC();
    // bool ReadR();
    // bool ReadD();
    // bool ReadL();
    READ_STATE ReadX();
    /* copy code end */

    /* copy code begin */
    void SkipToFileEnds();
    void SkipToNextNotEmptyLine();
    bool SkipToNextLine();
    bool SkipToNextToken();
    void SkipToNextLineToken();
    void GetLineTokens();
    /* copy code end */

    std::shared_ptr<Cell> FindCell(const CELL_NAME& name) const;
public:
    Spice();
    std::shared_ptr<Netlist> GetNetlist() const;
    READ_STATE OpenReadAndParseSpice(const std::string& fileName, const CELL_NAME& topCellName); // copy code
};