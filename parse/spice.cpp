#include "spice.h"

Spice::Spice()
{
    _netlist = std::make_shared<Netlist>();
}

std::shared_ptr<Netlist> Spice::GetNetlist() const
{
    return _netlist;
}

READ_STATE Spice::OpenReadAndParseSpice(const std::string& fileName, const CELL_NAME& topCellName)
{
    const Config& config = Config::GetInstance();
    if (!OpenFile(fileName)) {
        return NO_FILE;
    }

    size_t routePos = fileName.find('/');
    _fileName = (routePos == std::string::npos ? std::move(fileName) : std::move(fileName.substr(routePos + 1)));
    _mainCell = std::make_shared<Cell>(_fileName);
    _mainCell->SetNetlist(_netlist);

    READ_STATE readResult;
    if ((readResult = ReadSpice()) != READ_OK) {
        return readResult;
    }

    CloseFile();
    _netlist->_topCell = FindCell(topCellName);
    return _netlist->BuildHierarchyStructure() ? READ_OK : HIERARCHY_LOOP;
}

/* copy code */
bool Spice::EndFile() const
{
    return _nowFile->eof();
}

/* copy code */
bool Spice::OpenFile(const std::string& name)
{
    _nowFile = std::make_unique<std::fstream>(name);
    if (_nowFile == nullptr || !_nowFile->is_open()) {
        return false;
    }

    // reset scanner
    _lineNum = 0;
    _nextToken = "";
    _lineTokens.clear();
    _tokenIndex = -1;

    return true;
}

/* copy code */
void Spice::CloseFile()
{
    _nowFile->close();
}

/* write by myself except skip */
READ_STATE Spice::ReadSpice()
{
    _nowCell = _mainCell;
    READ_STATE readState = READ_OK;

    // parse file
    SkipToNextNotEmptyLine();

    while (!EndFile()) {
        if (EndFile()) {
            break;
        }

        if (MatchNoCase(_nextToken, ".SUBCKT")) {
            if ((readState = ReadSubckt()) != READ_OK) {
                break;
            }
        } else if (MatchNoCase(_nextToken, ".ENDS")) {
            if (_nowCell == _mainCell) {
                readState = ENDS_NO_MATCH_SUBCKT;
                SkipToFileEnds();
                break;
            }
            _nowCell = _mainCell;
            SkipToNextNotEmptyLine();
        } else if (MatchNoCase(_nextToken, ".PARAM")) {
            while (SkipToNextToken()) {}
        } else if (MatchNoCase(_nextToken, ".MODEL")) {
        } else if (MatchNoCase(_nextToken, ".GLOBAL")) {
            while (SkipToNextToken()) {}
        } else if (MatchNoCase(_nextToken, ".CONTROL")) {
            // ignore
            while (true) {
                SkipToNextLine();
                if (EndFile()) {
                    break;
                }
                if (MatchNoCase(_nextToken, ".ENDC")) {
                    break;
                }
            }
        } else if (toupper(_nextToken[0]) == 'M') {
            readState = ReadM();
            if (readState != READ_OK) {
                SkipToFileEnds();
                break;
            }
        } else if (toupper(_nextToken[0]) == 'X') {
            readState = ReadX();
            if (readState != READ_OK) {
                SkipToFileEnds();
                break;
            }
        } else if (MatchNoCase(_nextToken, ".END")) {
            SkipToNextNotEmptyLine();
        } else {
            // ignore other
            SkipToNextNotEmptyLine();
        }
    }

    if (readState == READ_OK) {
        if ((_netlist->QuotePointToCell(_mainCell)) != READ_OK) {
            return readState;
        }

        for (const std::pair<CELL_NAME, std::shared_ptr<Cell>>& it : _netlist->_cells) {
            const std::shared_ptr<Cell>& cell = it.second;
            if ((_netlist->QuotePointToCell(cell)) != READ_OK) {
                return readState;
            }
        }
    }

    return readState;
}

/* copy code */
void Spice::SkipToFileEnds()
{
    // wrong
    while (true) {
        SkipToNextNotEmptyLine();
        if (EndFile()) {
            break;
        }
        if (MatchNoCase(_nextToken, ".ENDS")) {
            _nowCell = _mainCell;
            break;
        }
    }
}

/* copy code */
void Spice::SkipToNextNotEmptyLine()
{
    bool flag = false;
    do {
        flag = SkipToNextLine();
    } while (!flag);
}

/* copy code */
bool Spice::SkipToNextLine()
{
    ++_lineNum;
    GetLineTokens();

    if (EndFile()) {
        return true;
    }

    if (_lineTokens.empty()) {
        return false;
    }

    if (_lineTokens[0][0] == '*') {
        return false; // empty line
    }

    if (_lineTokens.size() == 1 && _lineTokens[0] == "+") {
        return false; // only '+' is empty.
    }

    _tokenIndex = 0;
    _nextToken = _lineTokens[0];
    return true;
}

/* copy code */
bool Spice::SkipToNextToken()
{
    // skip to next token
    if (++_tokenIndex != _lineTokens.size()) {
        _nextToken = _lineTokens[_tokenIndex];
        return true;
    }

    if (_tokenIndex == _lineTokens.size()) {
        SkipToNextNotEmptyLine();
        if (EndFile()) {
            return false;
        }

        if (_lineTokens[0][0] == '+') {
            // no other after "+", skip to first.
            if (_lineTokens[0] == "+") {
                _tokenIndex = 1;
                _nextToken = std::move(_lineTokens[1]);
            } else {
                _tokenIndex = 0;
                _nextToken = std::move(_lineTokens[0].substr(1)); // extract the part after the "+"
            }

            return true;
        } else {
            // Next line is new
            return false;
        }
    }

    return false;
}

/* copy code */
void Spice::SkipToNextLineToken()
{
    if (_tokenIndex == 0) {
        return;
    }

    if (_tokenIndex == 1 && _lineTokens[0] == "+") {
        return;
    }

    SkipToNextNotEmptyLine();
}

void Spice::GetLineTokens()
{
    getline(*_nowFile, _line);

    size_t dollarPos = _line.find('$');
    if (dollarPos == std::string::npos) {
        // no '$'
        _lineTokens = std::move(SplitString(_line));
    } else {
        _lineTokens = std::move(SplitString(_line.substr(0, dollarPos)));
    }
}

READ_STATE Spice::ReadSubckt()
{
    if (!SkipToNextToken()) {
        // no name
        SkipToFileEnds();
        return SUBCKT_NO_NAME;
    }

    if (_nowCell != _mainCell) {
        // previous subckt not match
        SkipToFileEnds();
        return SUBCKT_NO_ENDS;
    }

    CELL_NAME cellName = std::move(_nextToken);
    _nowCell = _netlist->DefineCell(cellName);

    if (_nowCell == nullptr) {
        SkipToFileEnds();
        return SUBCKT_REDEFINE;
    }

    while (SkipToNextToken()) {
        WIRE_NAME portName = std::move(_nextToken);

        if (_nowCell->_portsMap.find(portName) != _nowCell->_portsMap.end()) {
            // redefine port name in the same subckt
            _nowCell = _mainCell;
            SkipToFileEnds();
            return SUBCKT_PORT_REDEFINE;
        }

        const std::shared_ptr<Port>& port = std::make_shared<Port>(portName);
        _nowCell->_portsMap[portName] = _nowCell->GetPorts().size(); // save in map
        _nowCell->GetPorts().push_back(port);                       // save in vector again
    }

    return READ_OK;
}

READ_STATE Spice::ReadM()
{
    DEVICE_NAME name = std::move(_nextToken);
    std::shared_ptr<Wire> drain, gate, source, bulk;

    if (!SkipToNextToken()) {
        return READ_MOSFET_ERROR;
    }
    drain = _nowCell->DefineWire(_nextToken);

    if (!SkipToNextToken()) {
        return READ_MOSFET_ERROR;
    }
    gate = _nowCell->DefineWire(_nextToken);

    if (!SkipToNextToken()) {
        return READ_MOSFET_ERROR;
    }
    source = _nowCell->DefineWire(_nextToken);

    if (!SkipToNextToken()) {
        return READ_MOSFET_ERROR;
    }
    bulk = _nowCell->DefineWire(_nextToken);

    if (!SkipToNextToken()) {
        return READ_MOSFET_ERROR;
    }
    DEVICE_MODEL_NAME model = std::move(_nextToken);

    const std::shared_ptr<Mosfet>& device = std::make_shared<Mosfet>(name);

    if (!_nowCell->AddDevice(device)) {
        return SUBCKT_DEVICE_REDEFINE;
    }

    device->SetCell(_nowCell);
    device->SetNetlist(_netlist);
    device->SetModel(model);

    device->AddConnectWire(drain, pinMagicTable[DEVICE_TYPE_MOSFET][0]);
    device->AddConnectWire(gate, pinMagicTable[DEVICE_TYPE_MOSFET][1]);
    device->AddConnectWire(source, pinMagicTable[DEVICE_TYPE_MOSFET][2]);
    device->AddConnectWire(bulk, pinMagicTable[DEVICE_TYPE_MOSFET][3]);

    while (SkipToNextToken()) {
        // parse parameter
        size_t equalPos = _nextToken.find_first_of("=");
        PROPERTY_NAME propertyName = _nextToken.substr(0, equalPos);
        std::string propertyValue = _nextToken.substr(equalPos + 1);
        device->SetPropertyValue(propertyName, propertyValue, _nowCell->GetParameters(), _mainCell->GetParameters());
    }

    return READ_OK;
}

READ_STATE Spice::ReadX()
{
    DEVICE_NAME name = std::move(_nextToken);
    const std::shared_ptr<Quote>& device = std::make_shared<Quote>(name);

    if (!_nowCell->AddDevice(device)) {
        return SUBCKT_DEVICE_REDEFINE;
    }

    _nowCell->GetQuotes().emplace_front(device);
    device->SetCell(_nowCell);
    device->SetNetlist(_netlist);

    while (SkipToNextToken()) {
        device->_tokens.emplace_back(_nextToken);
    }

    return READ_OK;
}

std::shared_ptr<Cell> Spice::FindCell(const CELL_NAME& name) const
{
    const std::shared_ptr<Cell>& cell = _netlist->FindCell(name);
    return cell == nullptr ? _mainCell : cell;
}
