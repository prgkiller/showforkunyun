#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../net.h"
#include "../../base/base.h"

class Cell;
class Netlist;
class Device: public std::enable_shared_from_this<Device> {
protected:
    DEVICE_NAME _name;
    DEVICE_TYPE _deviceType;
    DEVICE_MODEL_NAME _model;
    std::weak_ptr<Netlist> _netlist;
    std::weak_ptr<Cell> _cell;
    std::vector<std::pair<std::shared_ptr<Net>, PIN_MAGIC> > _connectNets;

public:
    Device() = default;
    Device(const DEVICE_NAME& _name);
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    virtual ~Device() = default;

    DEVICE_NAME GetName() const;

    DEVICE_TYPE GetDeviceType() const;
    void SetDeviceType(const DEVICE_TYPE& type);

    DEVICE_MODEL_NAME GetModel() const;
    void SetModel(const DEVICE_MODEL_NAME& model);

    std::shared_ptr<Netlist> GetNetlist() const;
    void SetNetlist(const std::shared_ptr<Netlist>& netlist);

    std::shared_ptr<Cell> GetCell() const;
    void SetCell(const std::shared_ptr<Cell>& cell);

    std::vector<std::pair<std::shared_ptr<Net>, PIN_MAGIC> >& GetConnectNets();
    void AddConnectNet(const std::shared_ptr<Net>& net, const PIN_MAGIC& pinMagic);

    virtual void SetPropertyValue(const PROPERTY_NAME& propertyName, const std::string& expression,
        std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& localParam,
        std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& globalParam) = 0;
    virtual std::unordered_map<PROPERTY_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> GetProperties() = 0;
    virtual bool PropertyCompare(const std::shared_ptr<Device>& another);
    virtual std::shared_ptr<Device> CopyDevice(const std::shared_ptr<Cell>& parentCell, const CELL_NAME& parentDeviceName) = 0;
};