#include "device.h"

Device::Device(const DEVICE_NAME& name) : _name(name) {}

DEVICE_NAME Device::GetName() const {
    return _name;
}

DEVICE_TYPE Device::GetDeviceType() const {
    return _deviceType;
}

void Device::SetDeviceType(const DEVICE_TYPE& type) {
    _deviceType = type;
}

DEVICE_MODEL_NAME Device::GetModel() const {
    return _model;
}

void Device::SetModel(const DEVICE_MODEL_NAME& model) {
    _model = model;
}

std::shared_ptr<Netlist> Device::GetNetlist() const {
    return _netlist.lock();
}

void Device::SetNetlist(const std::shared_ptr<Netlist>& netlist) {
    _netlist = netlist;
}

std::shared_ptr<Cell> Device::GetCell() const {
    return _cell.lock();
}

void Device::SetCell(const std::shared_ptr<Cell>& cell) {
    _cell = cell;
}

std::vector<std::pair<std::shared_ptr<Net>, PIN_MAGIC>>& Device::GetConnectWires() {
    return _connectWires;
}

void Device::AddConnectWire(const std::shared_ptr<Net>& wire, const PIN_MAGIC& pinMagic) {
    _connectWires.emplace_back(std::make_pair(wire, pinMagic));
    wire->GetConnectDevices().emplace_front(std::make_pair(shared_from_this(), pinMagic));
}

bool Device::PropertyCompare(const std::shared_ptr<Device>& another) {
    return true;
}

// 留作派生类重载使用
// std::shared_ptr<Device> Device::CopyDevice(
//     const std::shared_ptr<Cell>& parentCell,
//     const CELL_NAME& parentDeviceName
// ) {
//     const std::shared_ptr<Device>& parentDevice = std::make_shared<Device>(parentDeviceName);
//     parentDevice->SetCell(parentCell);
//     parentDevice->SetNetlist(_netlist.lock());
//     parentDevice->SetModel(_model);
//     return parentDevice;
// }
