#include <queue>
#include "compare_netlist.h"

CompareNetlist::CompareNetlist(std::shared_ptr<Netlist>& netlist1, std::shared_ptr<Netlist>& netlist2)
{
    _netlist1 = netlist1;
    _netlist2 = netlist2;
    _netlist1->SetID(NETLIST_1);
    _netlist2->SetID(NETLIST_2);  // 修复拼写错误：NETLIET_2 -> NETLIST_2
    LoadData();
}

void CompareNetlist::LoadData()
{
    LoadCells(_netlist1, _cells1);
    LoadCells(_netlist2, _cells2);
}

void CompareNetlist::LoadCells(const std::shared_ptr<Netlist>& netlist,
                               std::unordered_map<std::shared_ptr<Cell>, std::shared_ptr<CellElement> >& cells)
{
    cells.reserve(netlist->_validCells.size() << 2);  // The number of preset buckets, with a load factor of 25%
    for (const std::shared_ptr<Cell>& cell : netlist->_validCells) {
        const std::shared_ptr<CellElement>& cellElement = std::make_shared<CellElement>(cell);
        cells.emplace(cell, cellElement);
    }
}

COMPARE_NETLIST_RESULT CompareNetlist::Compare()
{
    const Config& config = Config::GetInstance();
    return config.hier ? HierarchyCompare() : FullFlattenCompare();
}

void CompareNetlist::BuildTargetCell()
{
    const std::shared_ptr<Cell>& topCell1 = _netlist1->GetTopCell();
    const std::shared_ptr<Cell>& topCell2 = _netlist2->GetTopCell();
    const std::shared_ptr<CellElement>& topCellElement1 = GetCellELement(topCell1);
    const std::shared_ptr<CellElement>& topCellElement2 = GetCellELement(topCell2);
    topCellElement1->targetCell = topCellElement2;
    topCellElement2->targetCell = topCellElement1;

    // rule file can customize target, now not write.
    for (const auto& it : _cells1) {
        const std::shared_ptr<Cell>& cell1 = it.first;
        const std::shared_ptr<CellElement>& cellElement1 = it.second;
        if (cellElement1->targetCell.expired()) {
            const CELL_NAME& cellName = cell1->GetName();
            const std::shared_ptr<Cell>& cell2 = _netlist2->FindCell(cellName);
            if (cell2 && _netlist2->_validCells.count(cell2)) {
                const std::shared_ptr<CellElement>& cellElement2 = GetCellELement(cell2);
                cellElement1->targetCell = cellElement2;
                cellElement2->targetCell = cellElement1;
            }
        }
    }
}

COMPARE_NETLIST_RESULT CompareNetlist::FullFlattenCompare()
{
    const std::shared_ptr<Cell>& topCell1 = _netlist1->GetTopCell();
    const std::shared_ptr<Cell>& topCell2 = _netlist2->GetTopCell();
    const std::shared_ptr<CellElement>& topCellElement1 = GetCellELement(topCell1);
    const std::shared_ptr<CellElement>& topCellElement2 = GetCellELement(topCell2);

    AtomizeCell(topCellElement1);
    AtomizeCell(topCellElement2);

    // debug
    // topCell1->Show();
    // topCell2->Show();

    const std::unique_ptr<CompareCell> compareCell = std::make_unique<CompareCell>(topCell1, topCell2);
    const COMPARE_CELL_RESULT compareCellResult = compareCell->Compare();
    return compareCellResult == COMPARE_CELL_TRUE ? COMPARE_NETLIST_TRUE : COMPARE_NETLIST_FALSE;
}

COMPARE_NETLIST_RESULT CompareNetlist::HierarchyCompare()
{
    BuildTargetCell();
    Config& config = Config::GetInstance();
    return config.multiThread ? MultiThreadHierarchyCompare() : OneThreadHierarchyCompare();
}

COMPARE_NETLIST_RESULT CompareNetlist::OneThreadHierarchyCompare()
{
    std::queue<std::shared_ptr<CellElement>> compareQueue;
    for (const auto& itCellElement : _cells1) {
        const std::shared_ptr<CellElement>& cellElement = itCellElement.second;
        if (cellElement->outDegree == 0) {
            compareQueue.push(cellElement);
        }
    }

    COMPARE_CELL_RESULT compareCellResult;
    while (!compareQueue.empty()) {
        const std::shared_ptr<CellElement> cellElement1 = compareQueue.front();  // can't use const&, because it will be popped
        compareQueue.pop();

        AtomizeCell(cellElement1);

        const std::shared_ptr<Cell>& cell1 = cellElement1->cell;
        for (const std::weak_ptr<Cell>& parentCell : cell1->_parents) {
            const std::shared_ptr<CellElement>& parentElement = GetCellELement(parentCell.lock());
            if (--parentElement->outDegree == 0) {
                compareQueue.push(parentElement);
            }
        }

        const std::shared_ptr<CellElement>& cellElement2 = cellElement1->targetCell.lock();
        if (cellElement2 == nullptr) {
            continue;
        }

        // debug
        // std::cout << "AtomizeCell2(" << cellElement1->cell->GetName() << ")" << std::endl;
        AtomizeCell(cellElement2);

        std::unique_ptr<CompareCell> compareCell = std::make_unique<CompareCell>(cellElement1->cell, cellElement2->cell);
        compareCellResult = compareCell->Compare();
        if (compareCellResult == COMPARE_CELL_TRUE) {
            DealCompareCellsTrue(std::move(compareCell), cellElement1, cellElement2);
        }
    }

    return compareCellResult == COMPARE_CELL_TRUE ? COMPARE_NETLIST_TRUE : COMPARE_NETLIST_FALSE;
}

COMPARE_NETLIST_RESULT CompareNetlist::MultiThreadHierarchyCompare()
{
    std::queue<std::shared_ptr<CellElement>> compareQueue;
    auto DecreaseParentsOutDegree = [&compareQueue, this](const std::shared_ptr<CellElement>& cellElement) {
        const std::shared_ptr<Cell>& cell = cellElement->cell;
        for (const std::weak_ptr<Cell>& parentCell : cell->_parents) {
            const std::shared_ptr<CellElement>& parentElement = GetCellELement(parentCell.lock());
            if (--parentElement->outDegree == 0) {  // outDegree is atomic
                std::lock_guard<std::mutex> lock(queueMutex);  // queue's push is not thread safe.
                compareQueue.push(parentElement);
            }
        }
    };

    auto Compare = [&DecreaseParentsOutDegree, this](const std::shared_ptr<CellElement>& cellElement1,
                                                    const std::shared_ptr<CellElement>& cellElement2) -> void {
        AtomizeCell(cellElement2);
        std::unique_ptr<CompareCell> compareCell = std::make_unique<CompareCell>(cellElement1->cell, cellElement2->cell);
        COMPARE_CELL_RESULT compareCellResult = compareCell->Compare();
        if (compareCellResult == COMPARE_CELL_TRUE) {
            DealCompareCellsTrue(std::move(compareCell), cellElement1, cellElement2);
        }
        DecreaseParentsOutDegree(cellElement1);
        cvQueueNotEmpty.notify_one();
    };

    for (const auto& itCellElement : _cells1) {
        const std::shared_ptr<CellElement>& cellElement = itCellElement.second;
        if (cellElement->outDegree == 0) {
            compareQueue.push(cellElement);
        }
    }

    const std::shared_ptr<CellElement>& topCellElement1 = GetCellELement(_netlist1->GetTopCell());
    bool compareOver = false;

    while (!compareOver) {
        if (compareQueue.empty()) {  // until compareQueue is not empty
            std::unique_lock<std::mutex> lock(mtx);
            cvQueueNotEmpty.wait(lock, [&compareQueue] { return !compareQueue.empty(); });
        }

        const std::shared_ptr<CellElement> cellElement1 = compareQueue.front();
        compareQueue.pop();

        AtomizeCell(cellElement1);
        const std::shared_ptr<CellElement>& cellElement2 = cellElement1->targetCell.lock();

        // debug
        // printf("Now is visiting %s\n", cellElement1->cell->GetName().c_str());

        if (cellElement2 == nullptr) {
            DecreaseParentsOutDegree(cellElement1);
            continue;
        }

        std::thread compareThread(Compare, cellElement1, cellElement2);
        if (cellElement1 != topCellElement1) {
            compareThread.detach();
        } else {
            // topCell-> block main thread
            compareThread.join();
            compareOver = true;
        }
    }

    return topCellElement1->matched.expired() ? COMPARE_CELL_FALSE : COMPARE_CELL_TRUE;
}

void CompareNetlist::AtomizeCell(const std::shared_ptr<CellElement>& cellElement)
{
    std::lock_guard<std::mutex> atomizeLock(cellElement->atomizeMutex);
    if (cellElement->flattened) {
        return;
    }

    const std::shared_ptr<Cell>& cell = cellElement->cell;
    for (const std::pair<std::shared_ptr<Cell>, std::vector<std::shared_ptr<Quote>>>& it : cell->_sons) {
        const std::shared_ptr<Cell>& sonCell = it.first;
        const std::vector<std::shared_ptr<Quote>>& quotes = it.second;

        if (sonCell->GetDevices().size() == 0) {  // sonCell has no device
            for (const std::shared_ptr<Quote>& quote : quotes) {
                cell->GetDevices().erase(quote->GetName());
            }
            continue;
        }

        const std::shared_ptr<CellElement>& sonCellElement = GetCellELement(sonCell);  // is better use a pointer point to here, rather than use unordered_map

        if (sonCellElement->matched.expired()) {  // sonCellElement->matche.lock() == nullptr
            AtomizeCell(sonCellElement);
            for (const std::shared_ptr<Quote>& quote : quotes) {
                FlattenOneQuote(cellElement, quote);
            }
        } else {
            for (const std::shared_ptr<Quote>& quote : quotes) {
                QuoteToBeDevice(quote);
            }
        }
    }

    // clear nets which are not connect device.
    for (const auto& it : cell->GetNets()) {
        const std::shared_ptr<Net>& net = it.second;
        if (net->GetConnectDevices().empty()) {
            cell->GetNets().erase(net->GetName());
        }
    }

    // debug
    // std::cout << "Flatten(" << cellElement->cell->GetName() << ") is over" << std::endl;
    cellElement->flattened = true;
}

void CompareNetlist::FlattenOneQuote(const std::shared_ptr<CellElement>& cellElement, std::shared_ptr<Quote> quote)
{
    // debug
    // std::cout << "FlattenOneQuote(" << cellElement->cell->GetName() << ", " << quote->GetName() << ")" << std::endl;

    const std::shared_ptr<Cell>& parentCell = cellElement->cell;  // up cell
    const std::shared_ptr<Cell>& sonCell = quote->GetQuoteCell();  // down cell

    std::unordered_map<std::shared_ptr<Device>, std::shared_ptr<Device>> copiedDevices;  // key:son, value:parent
    std::unordered_map<std::shared_ptr<Net>, std::shared_ptr<Net>> copiedNets;  // key:son, value:parent

    // copy down devices to up
    for (const auto& it : sonCell->GetDevices()) {
        const std::shared_ptr<Device>& sonDevice = it.second;
        // it will be named "X../..:X../..:X..", the backward is not regard the outer quote is from which cell.
        const DEVICE_NAME& parentDeviceName = quote->GetName() + '/' + sonCell->GetName() + ':' + sonDevice->GetName();
        const std::shared_ptr<Device>& parentDevice = sonDevice->CopyDevice(parentCell, parentDeviceName);
        parentCell->AddDevice(parentDevice);
        copiedDevices[sonDevice] = parentDevice;
    }

    for (const auto& itNet : sonCell->GetNets()) {
        const std::shared_ptr<Net>& sonNet = itNet.second;
        // std::cout << "When flatten(" << parentCell->GetName() << "," << quote->GetName() << "), sonNet " << sonNet->GetName() << " is check now." << std::endl;
        std::shared_ptr<Net> parentNet = nullptr;
        const PORT_INDEX portIndex = sonNet->GetPortIndex();
        if (portIndex == NOT_PORT) {
            // copy net
            const WIRE_NAME& parentNetName = quote->GetName() + '/' + sonCell->GetName() + ':' + sonNet->GetName();
            parentNet = parentCell->DefineNet(parentNetName);
        } else {
            parentNet = quote->_pendingNets[portIndex];  // find parent net
            // merge
            for (const auto& itDevice : sonNet->GetConnectDevices()) {
                const std::shared_ptr<Device>& sonDevice = itDevice.first.lock();
                const PIN_MAGIC pinMagic = itDevice.second;
                const std::shared_ptr<Device>& parentDevice = copiedDevices[sonDevice];
                // parentNet->AddConnectDevice(parentDevice, pinMagic);
            }
        }
        copiedNets[sonNet] = parentNet;
    }

    // connect devices and nets.
    for (const auto& itPairs : copiedDevices) {
        const std::shared_ptr<Device>& sonDevice = itPairs.first;
        const std::shared_ptr<Device>& parentDevice = itPairs.second;
        for (const auto& itNet : sonDevice->GetConnectNets()) {
            const std::shared_ptr<Net>& sonNet = itNet.first;
            const PIN_MAGIC pinMagic = itNet.second;
            const std::shared_ptr<Net>& parentNet = copiedNets[sonNet];
            parentDevice->AddConnectNet(parentNet, pinMagic);
        }
    }

    // delete quote after flatten
    parentCell->GetDevices().erase(quote->GetName());
}

void CompareNetlist::QuoteToBeDevice(const std::shared_ptr<Quote>& quote) const
{
    const std::shared_ptr<Cell>& quoteCell = quote->GetQuoteCell();
    const std::shared_ptr<CellElement>& quoteCellElement = GetCellELement(quoteCell);

    // debug
    // std::cout << "QuoteToBeDevice(" << quote->GetName() << "), quoteCell = " << quoteCell->GetName() << " quoteCellElement address = " << quoteCellElement << std::endl;

    quote->SetModel(quoteCellElement->label);

    for (size_t index = 0; index < quote->_pendingNets.size(); ++index) {
        const std::shared_ptr<Net>& net = quote->_pendingNets[index];
        const PIN_MAGIC& pinMagic = quoteCell->GetPorts()[index]->GetLabel();
        quote->AddConnectNet(net, pinMagic);
    }
}

// Imperfect code
void CompareNetlist::DealCompareCellsTrue(const std::unique_ptr<CompareCell>& compareCell,
                                         const std::shared_ptr<CellElement>& cellElement1,
                                         const std::shared_ptr<CellElement>& cellElement2)
{
    cellElement1->matched = cellElement2;
    cellElement2->matched = cellElement1;

    // After setting the label for the cell, I'll just write one randomly for now
    auto BuildCellLabel = [&compareCell](const std::shared_ptr<CellElement>& cellElement) -> DEVICE_MODEL_NAME {
        const uint8_t lastBucketsId = compareCell->_lastBucketsId;
        HASH_VALUE result = 0;
        for (const auto& itDevice : compareCell->_deviceBuckets[lastBucketsId]) {
            const std::shared_ptr<CompareCell::DeviceElement>& deviceElement = itDevice.first;
            // result += deviceElement->oldColor * deviceElement->newColor % HASH_MOD_1;
            result += deviceElement->oldColor * deviceElement->newColor;
            // result %= HASH_MOD_1;
        }
        for (const auto& itNet : compareCell->_netBuckets[lastBucketsId]) {
            const std::shared_ptr<CompareCell::NetElement>& netElement = itNet.first;
            // result += netElement->oldColor * netElement->newColor % HASH_MOD_1;
            result += netElement->oldColor * netElement->newColor;
            // result %= HASH_MOD_1;
        }
        return std::to_string(result);
    };

    auto SetPortsLabel = [&compareCell](const std::shared_ptr<CellElement>& cellElement) -> void {
        const std::shared_ptr<Cell>& cell = cellElement->cell;
        for (const auto& port : cell->GetPorts()) {
            const std::shared_ptr<Net>& net = port->GetNet();
            if (net != nullptr) {
                const std::shared_ptr<CompareCell::NetElement>& netElement = compareCell->_nets[net];
                // port->SetLabel(netElement->oldColor * netElement->newColor % HASH_MOD_1);
                port->SetLabel(netElement->oldColor * netElement->newColor);
            } else {
                port->SetLabel(static_cast<HASH_VALUE>(0));  // open circuit
            }
        }
    };

    // determine the label of cell
    cellElement1->label = BuildCellLabel(cellElement1);
    cellElement2->label = BuildCellLabel(cellElement2);

    // deal the label of port
    SetPortsLabel(cellElement1);
    SetPortsLabel(cellElement2);
}

std::shared_ptr<CompareNetlist::CellElement> CompareNetlist::GetCellELement(const std::shared_ptr<Cell>& cell) const
{
    // might not necessary, use a pointer point to CellElement is better.
    const NETLIST_ID& id = cell->GetNetlist()->GetID();
    const std::unordered_map<std::shared_ptr<Cell>, std::shared_ptr<CellElement> >& cells = id == NETLIST_1 ? _cells1 : _cells2;
    return cells.find(cell)->second;
}
