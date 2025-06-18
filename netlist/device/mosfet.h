#pragma once
#include "device.h"

class Mosfet: public Device {
private:
    double _w;
    double _l;
public:
    explicit Mosfet(const DEVICE_NAME& name);
    Mosfet(const Mosfet&) = delete;
    Mosfet& operator=(const Mosfet&) = delete;
    void SetPropertyValue(const PROPERTY_NAME& propertyName, const std::string& expression,
        std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& localParam,
        std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& globalParam) override;
    std::unordered_map<PROPERTY_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> GetProperties() override;
    bool PropertyCompare(const std::shared_ptr<Device>& another) override;
    std::shared_ptr<Device> CopyDevice(const std::shared_ptr<Cell>& parentCell, const CELL_NAME& parentDeviceName) override;
};