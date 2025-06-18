#include "port.h"

Port::Port(const WIRE_NAME& name)
    : _name(name), _wire(nullptr) {
}

std::shared_ptr<Wire> Port::GetWire() const {
    return _wire;
}

void Port::SetWire(const std::shared_ptr<Wire>& wire) {
    _wire = wire;
}

HASH_VALUE Port::GetLabel() const {
    return _label;
}

void Port::SetLabel(const HASH_VALUE label) {
    _label = label;
}
