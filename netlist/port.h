#pragma once

#include <memory>
#include "../base/base.h"

class Net;
class Port {
private:
    WIRE_NAME _name;
    std::shared_ptr<Net> _wire;
    HASH_VALUE _label; // have label after compare true
public:
    explicit Port(const WIRE_NAME& name);
    Port(const Port&) = delete;
    Port& operator=(const Port&) = delete;

    std::shared_ptr<Net> GetWire() const;
    void SetWire(const std::shared_ptr<Net>& wire);
    HASH_VALUE GetLabel() const;
    void SetLabel(HASH_VALUE label);
};