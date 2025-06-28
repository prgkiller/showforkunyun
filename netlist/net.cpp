#include <list>
#include "net.h"

Net::Net(const NET_NAME& name) : _name(name) {
    _portIndex = NOT_PORT;
}

NET_NAME Net::GetName() const {
    return _name;
}

std::shared_ptr<Cell> Net::GetCell() const {
    return _cell.lock();
}

void Net::SetCell(const std::shared_ptr<Cell>& cell) {
    _cell = cell;
}

std::shared_ptr<Netlist> Net::GetNetlist() const {
    return _netlist.lock();
}

void Net::SetNetlist(const std::shared_ptr<Netlist>& netlist) {
    _netlist = netlist;
}

PORT_INDEX Net::GetPortIndex() const {
    return _portIndex;
}

void Net::SetPortIndex(const PORT_INDEX portIndex) {
    _portIndex = portIndex;
}

std::list<std::pair<std::weak_ptr<Device>, PIN_MAGIC>>& Net::GetConnectDevices() {
    return _connectDevices;
}
