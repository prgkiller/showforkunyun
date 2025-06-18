#include "quote.h"

Quote::Quote() {
    _deviceType = DEVICE_TYPE_QUOTE;
}

Quote::Quote(const DEVICE_NAME& name) {
    _name = name;
    _deviceType = DEVICE_TYPE_QUOTE;
}

std::shared_ptr<Cell> Quote::GetQuoteCell() const {
    return _quoteCell.lock();
}

void Quote::SetQuoteCell(const std::shared_ptr<Cell>& cell) {
    _quoteCell = cell;
}

void Quote::SetPropertyValue(const PROPERTY_NAME& propertyName, const std::string& expression,
    std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& localParam,
    std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& globalParam
) {
    // quote has no properties
}

std::unordered_map<PROPERTY_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> Quote::GetProperties() {
    std::unordered_map<PROPERTY_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> result;
    return result;
}

std::shared_ptr<Device> Quote::CopyDevice(const std::shared_ptr<Cell>& parentCell, const CELL_NAME& parentDeviceName) {
    const std::shared_ptr<Quote>& parentDevice = std::make_shared<Quote>(parentDeviceName);
    parentDevice->SetCell(parentCell);
    parentDevice->SetNetlist(_netlist.lock());
    parentDevice->SetModel(_model);
    parentDevice->SetQuoteCell(_quoteCell.lock());
    return parentDevice;
}
