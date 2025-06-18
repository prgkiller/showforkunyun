#include <cmath>
#include "mosfet.h"

Mosfet::Mosfet(const DEVICE_NAME& name) {
    _name = name;
    _deviceType = DEVICE_TYPE_MOSFET;
}

void Mosfet::SetPropertyValue(const PROPERTY_NAME& propertyName, const std::string& expression,
        std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& localParam,
        std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& globalParam) {
    Expression exp;
    std::variant<double, std::string> expResult = exp.SolveExpression(expression, localParam, globalParam);

    if (MatchNoCase(propertyName, "w")) {
        const double* wPtr = std::get_if<double>(&expResult);
        _w = (wPtr == nullptr) ? 0.0 : *wPtr;
    } else if (MatchNoCase(propertyName, "l")) {
        const double* lPtr = std::get_if<double>(&expResult);
        _l = (lPtr == nullptr) ? 0.0 : *lPtr;
    }
}

std::unordered_map<PROPERTY_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> Mosfet::GetProperties() {
    std::unordered_map<PROPERTY_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> result;
    result.emplace("w", _w);
    result.emplace("l", _l);
    return result;
}

bool Mosfet::PropertyCompare(const std::shared_ptr<Device>& another) {
    if (another->GetDeviceType() != DEVICE_TYPE_MOSFET) {
        return false;
    }

    const std::shared_ptr<Mosfet> device = std::dynamic_pointer_cast<Mosfet>(another);
    double tolerance = Config::GetInstance().tolerance;

    return std::fabs(_w - device->_w) <= tolerance && std::fabs(_l - device->_l) <= tolerance;
}

std::shared_ptr<Device> Mosfet::CopyDevice( const std::shared_ptr<Cell>& parentCell, const CELL_NAME& parentDeviceName) {
    const std::shared_ptr<Mosfet>& parentDevice = std::make_shared<Mosfet>(parentDeviceName);
    parentDevice->SetCell(parentCell);
    parentDevice->SetNetlist(_netlist.lock());
    parentDevice->SetModel(_model);
    parentDevice->_w = _w;
    parentDevice->_l = _l;
    // other parameters should be copied if needed
    return parentDevice;
}
