#include <queue>
#include "netlist.h"

void Netlist::SetID(const NETLIST_ID& id) {
    _id = id;
}

NETLIST_ID Netlist::GetID() const {
    return _id;
}

std::shared_ptr<Cell> Netlist::GetTopCell() const {
    return _topCell;
}

std::string Netlist::OutputError() const {
    return _error.errorInformation;
}

READ_STATE Netlist::QuotePointToCell(const std::shared_ptr<Cell>& cell) {
    for (const std::shared_ptr<Quote>& quote : cell->GetQuotes()) {
        bool quoteFindCell = false;
        for (const std::string& token : quote->_tokens) {
            const std::shared_ptr<Cell>& finCell = FindCell(token);
            if (finCell == nullptr) {
                const std::shared_ptr<Net>& net = cell->DefineNet(token);
                quote->_pendingNets.push_back(net);
            } else {
                quoteFindCell = true;
                if (finCell->GetPorts().size() != quote->_pendingNets.size()) {
                    return QUOTE_PORT_NUMBER_ERROR;
                }
                quote->SetQuoteCell(finCell);
                break;
            }
        }
        if (!quoteFindCell) {
            // _error.errorInformation = quote->GetName() + " can't find cell.";
            return QUOTE_CANT_FIND_CELL;
        }
        quote->_tokens.clear();  // free some space
    }
    return READ_OK;
}

bool Netlist::BuildHierarchyStructure() {
    std::queue<std::shared_ptr<Cell>> cellQueue;
    cellQueue.push(_topCell);

    while (!cellQueue.empty()) {
        std::shared_ptr<Cell> parent = cellQueue.front();
        cellQueue.pop();

        if (_validCells.count(parent)) {
            continue;
        }
        _validCells.insert(parent);

        for (const std::shared_ptr<Quote>& quote : parent->GetQuotes()) {
            const std::shared_ptr<Cell>& son = quote->GetQuoteCell();
            cellQueue.push(son);
            BuildDependencyRelationShip(parent, quote);
        }
    }

    // debug
    // std::cout << _validCells.size() << std::endl;
    // ShowHierarchyStructure();

    return JudgeLoop();
}

void Netlist::BuildDependencyRelationShip(const std::shared_ptr<Cell>& parent, const std::shared_ptr<Quote>& quote) {
    const std::shared_ptr<Cell>& son = quote->GetQuoteCell();
    auto edge = (parent->_sons).find(son);

    if (edge == (parent->_sons).end()) {
        son->_parents.emplace_back(parent);
        parent->_sons.emplace(son, std::vector<std::shared_ptr<Quote>>{quote});
        ++parent->_outDegree;
        ++son->_inDegree;
    } else {
        edge->second.emplace_back(quote);
    }
}

bool Netlist::JudgeLoop() const {
    std::queue<std::shared_ptr<Cell>> cellQueue;
    std::unordered_set<std::shared_ptr<Cell>> visited;

    cellQueue.push(_topCell);

    while (!cellQueue.empty()) {
        std::shared_ptr<Cell> parent = cellQueue.front();
        cellQueue.pop();

        if (visited.count(parent)) {
            continue;
        }
        visited.emplace(parent);

        for (const auto& it : parent->_sons) {
            const std::shared_ptr<Cell>& son = it.first;
            if (--son->_inDegree == 0) {
                cellQueue.push(son);
            }
        }
    }

    // debug
    // std::cout << "valid cells(" << _validCells.size() << "):";
    // for (const shared_ptr<Cell>& validCell: _validCells) {
    //     std::cout << " " << validCell->GetName();
    // }
    // std::cout << "\nvisited cells(" << visited.size() << "):";
    // for (const shared_ptr<Cell>& visitedCell: visited) {
    //     std::cout << " " << visitedCell->GetName();
    // }
    // std::cout << std::endl;

    return _validCells.size() == visited.size();
}

std::shared_ptr<Cell> Netlist::FindCell(const CELL_NAME& name) const {
    const auto& it = _cells.find(name);
    if (it == _cells.end()) {
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<Cell> Netlist::DefineCell(const CELL_NAME& cellName) {
    std::shared_ptr<Cell> cell = FindCell(cellName);
    if (cell != nullptr) {
        return nullptr;
    }
    cell = std::make_shared<Cell>(cellName);
    cell->SetNetlist(shared_from_this());
    _cells[cell->GetName()] = cell;
    return cell;
}

// test
void Netlist::Show() {
    // 显示单个 Cell 的信息
    auto ShowCell = [](const std::shared_ptr<Cell>& cell) {
        std::cout << cell->GetName() << ":" << std::endl;
        for (const auto& it : cell->GetDevices()) {
            const std::shared_ptr<Device>& device = it.second;
            std::cout << device->GetName() << ":";

            if (device->GetDeviceType() != DEVICE_TYPE_QUOTE) {
                for (const auto& it2 : device->GetConnectNets()) {
                    const std::shared_ptr<Net>& net = it2.first;
                    const PIN_MAGIC pinMagic = it2.second;
                    std::cout << " " << net->GetName() << "(" << GetPinName(pinMagic) << ")";
                }
                std::cout << std::endl;
            } else {
                const std::shared_ptr<Quote> quote = std::dynamic_pointer_cast<Quote>(device);
                for (const std::shared_ptr<Net>& net : quote->_pendingNets) {
                    std::cout << " " << net->GetName();
                }
                std::cout << " [quote(" << quote->GetQuoteCell()->GetName() << ")]" << std::endl;
            }
        }

        for (const auto& it : cell->GetNets()) {
            const std::shared_ptr<Net>& net = it.second;
            std::cout << net->GetName() << ":";
            for (const auto& it2 : net->GetConnectDevices()) {
                const std::shared_ptr<Device>& device = it2.first.lock();
                const PIN_MAGIC pinMagic = it2.second;
                std::cout << " " << device->GetName() << "(" << GetPinName(pinMagic) << ")";
            }
            std::cout << std::endl;
        }
    };

    // 显示层级关系
    ShowHierarchyStructure();
    std::cout << std::endl;

    for (const std::shared_ptr<Cell>& cell : _validCells) {
        ShowCell(cell);
    }
}

void Netlist::ShowHierarchyStructure() const {
    for (const std::shared_ptr<Cell>& cell : _validCells) {
        std::cout << "==== " << cell->GetName()
                  << " ==== indegree:" << cell->_inDegree
                  << " outdegree:" << cell->_outDegree << std::endl;

        std::cout << "parent:";
        for (const std::weak_ptr<Cell>& parent : cell->_parents) {
            std::cout << " " << parent.lock()->GetName();
        }
        std::cout << std::endl;

        std::cout << "son:";
        for (const auto& it : cell->_sons) {
            const std::shared_ptr<Cell>& son = it.first;
            const std::vector<std::shared_ptr<Quote>>& quotes = it.second;

            std::cout << " " << son->GetName() << "(";
            std::cout << quotes[0]->GetName();

            for (size_t i = 1; i < quotes.size(); ++i) {
                std::cout << ',' << quotes[i]->GetName();
            }
            std::cout << ")";
        }
        std::cout << std::endl;
    }
}
