#pragma once
#include "device.h"

class Quote: public Device {
private:
    std::weak_ptr<Cell> _quoteCell;
public:
    Quote();
    explicit Quote(const DEVICE_NAME& name);
    Quote(const Quote&) = delete;
    Quote& operator= (const Quote&) = delete;

    std::vector<std::string> _tokens;
    std::vector<std::shared_ptr<Wire>> _pendingWires;

    std::shared_ptr<Cell> GetQuoteCell() const;
    void SetQuoteCell(const std::shared_ptr<Cell>& cell);

    void SetPropertyValue(const PROPERTY_NAME& propertyName, const std::string& expression,
        std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& localParam,
        std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& globalParam) override;
    std::unordered_map<PROPERTY_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> GetProperties() override;
    // bool PropertyCompare(const std::shared_ptr<Device>& another) override;
    std::shared_ptr<Device> CopyDevice(const std::shared_ptr<Cell>& parentCell, const CELL_NAME& parentDeviceName) override;
};