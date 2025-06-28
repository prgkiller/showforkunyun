#include "port.h"

Port::Port(const NET_NAME& name)
    : _name(name), _net(nullptr) {
}

std::shared_ptr<Net> Port::GetNet() const {
    return _net;
}

void Port::SetNet(const std::shared_ptr<Net>& net) {
    _net = net;
}

HASH_VALUE Port::GetLabel() const {
    return _label;
}

void Port::SetLabel(const HASH_VALUE label) {
    _label = label;
}
