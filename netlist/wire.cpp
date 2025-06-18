#include <list>
#include "wire.h"

Wire::Wire(const WIRE_NAME& name) : _name(name) {
    _portIndex = NOT_PORT;
}

WIRE_NAME Wire::GetName() const {
    return _name;
}

std::shared_ptr<Cell> Wire::GetCell() const {
    return _cell.lock();
}

void Wire::SetCell(const std::shared_ptr<Cell>& cell) {
    _cell = cell;
}

std::shared_ptr<Netlist> Wire::GetNetlist() const {
    return _netlist.lock();
}

void Wire::SetNetlist(const std::shared_ptr<Netlist>& netlist) {
    _netlist = netlist;
}

PORT_INDEX Wire::GetPortIndex() const {
    return _portIndex;
}

void Wire::SetPortIndex(const PORT_INDEX portIndex) {
    _portIndex = portIndex;
}

std::list<std::pair<std::weak_ptr<Device>, PIN_MAGIC>>& Wire::GetConnectDevices() {
    return _connectDevices;
}
