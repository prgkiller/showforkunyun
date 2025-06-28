#include "cell.h"

Cell::Cell(const CELL_NAME& name) : _name(name) {
    _inDegree = 0;
    _outDegree = 0;
}

CELL_NAME Cell::GetName() const {
    return _name;
}

void Cell::SetNetlist(const std::shared_ptr<Netlist>& netlist) {
    _netlist = netlist;
}

std::shared_ptr<Netlist> Cell::GetNetlist() {
    return _netlist.lock();
}

std::vector<std::shared_ptr<Port>>& Cell::GetPorts() {
    return _ports;
}

std::forward_list<std::shared_ptr<Quote>>& Cell::GetQuotes() {
    return _quotes;
}

std::unordered_map<DEVICE_NAME, std::shared_ptr<Device>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& Cell::GetDevices() {
    return _devices;
}

std::unordered_map<WIRE_NAME, std::shared_ptr<Net>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& Cell::GetWires() {
    return _wires;
}

std::unordered_map<PARAMETER_NAME, PARAMETER_VALUE, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& Cell::GetParameters() {
    return _parameters;
}

std::shared_ptr<Device> Cell::FindDevice(const DEVICE_NAME& name) const {
    const auto& it = _devices.find(name);
    if (it == _devices.end()) {
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<Net> Cell::FindWire(const WIRE_NAME& name) const {
    const auto& it = _wires.find(name);
    if (it == _wires.end()) {
        return nullptr;
    }
    return it->second;
}

bool Cell::AddDevice(const std::shared_ptr<Device>& device) {
    // 如果设备名已存在则返回 false，防止重定义
    const auto& it = _devices.find(device->GetName());
    if (it != _devices.end()) {
        return false;
    }
    _devices[device->GetName()] = device;
    return true;
}

std::shared_ptr<Net> Cell::DefineWire(const WIRE_NAME& wireName) {
    // 如果未定义则新建，已定义则返回已有实例
    const auto& it = _wires.find(wireName);
    if (it != _wires.end()) {
        return it->second;
    }

    const std::shared_ptr<Net>& wire = std::make_shared<Net>(wireName);
    wire->SetCell(shared_from_this());
    wire->SetNetlist(_netlist.lock());
    _wires[wireName] = wire;

    // 检查是否为端口
    const auto& itPort = _portsMap.find(wireName);
    if (itPort != _portsMap.end()) {
        const PORT_INDEX portIndex = itPort->second;
        wire->SetPortIndex(portIndex);

        const std::shared_ptr<Port>& port = _ports[portIndex];
        port->SetWire(wire);
    }

    return wire;
}

void Cell::SetParameterValue(const PARAMETER_NAME& parameterName, const std::string& str) {
    _parameters[parameterName] = str;
}

// // 共享指针版本的 CellElement，暂时注释
// std::shared_ptr<CompareNetlist::CellElement> Cell::GetCellElement() const {
//     return _cellElement.lock();
// }

// 调试输出 Cell 内容
void Cell::Show() {
    std::cout << GetName() << ":" << std::endl;

    for (const auto& it : GetDevices()) {
        const std::shared_ptr<Device>& device = it.second;
        std::cout << device->GetName() << ":";

        if (device->GetDeviceType() != DEVICE_TYPE_QUOTE) {
            for (const auto& it2 : device->GetConnectWires()) {
                const std::shared_ptr<Net>& wire = it2.first;
                const PIN_MAGIC pinMagic = it2.second;
                std::cout << " " << wire->GetName() << "(" << GetPinName(pinMagic) << ")";
            }

            const auto properties = std::move(device->GetProperties());
            for (const auto& item : properties) {
                std::cout << "," << item.first << "=";
                if (const double* dPtr = std::get_if<double>(&item.second)) {
                    std::cout << *dPtr;
                } else if (const std::string* strPtr = std::get_if<std::string>(&item.second)) {
                    std::cout << *strPtr;
                }
            }

            std::cout << std::endl;
        } else {
            const std::shared_ptr<Quote>& quote = std::dynamic_pointer_cast<Quote>(device);
            for (const std::shared_ptr<Net>& wire : quote->_pendingWires) {
                std::cout << " " << wire->GetName();
            }
            std::cout << " [quote(" << quote->GetQuoteCell()->GetName() << ")]" << std::endl;
        }
    }

    for (const auto& it : GetWires()) {
        const std::shared_ptr<Net>& wire = it.second;
        std::cout << wire->GetName() << ":";

        for (const auto& it2 : wire->GetConnectDevices()) {
            const std::shared_ptr<Device>& device = it2.first.lock();
            const PIN_MAGIC pinMagic = it2.second;
            std::cout << " " << device->GetName() << "(" << GetPinName(pinMagic) << ")";
        }

        std::cout << std::endl;
    }
}
