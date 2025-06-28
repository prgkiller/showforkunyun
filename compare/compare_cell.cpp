
#include <iostream>
#include <random>
#include "compare_cell.h"

CompareCell::CompareCell(const std::shared_ptr<Cell>& cell1, const std::shared_ptr<Cell>& cell2) {
    _cell1 = cell1;
    _cell2 = cell2;
    LoadData();
    // debug
    // printf("Now is comparing %s and %s.\n", cell1->GetName().c_str(), cell2->GetName().c_str());
    if (Debug::GetInstance().cellShow) {
        _cell1->Show();
        std::cout << std::endl;
        _cell2->Show();
        std::cout << std::endl;
    }
}

void CompareCell::LoadData() {
    auto Connect = [this](std::shared_ptr<DeviceElement> deviceElement, std::shared_ptr<WireElement> wireElement, PIN_MAGIC pinMagic) {
        deviceElement->_connectWireElements.emplace_back(wireElement, pinMagic);
        wireElement->_connectDeviceElements.emplace_back(deviceElement, pinMagic);
    };
    std::unordered_map<std::shared_ptr<Device>, std::shared_ptr<DeviceElement>> _devicesMap;
    auto LoadDevices = [&Connect, this](const std::shared_ptr<Cell>& cell) {
        const NETLIST_ID& id = cell->GetNetlist()->GetID();
        for (const auto& it : cell->GetDevices()) {
            const std::shared_ptr<Device>& device = it.second;
            const std::shared_ptr<DeviceElement>& deviceElement = std::make_shared<DeviceElement>(device, id);
            _deviceElements.emplace_back(deviceElement);
            // _deviceMap.emplace(device, deviceElement);
            for (const auto& itWire : device->GetConnectWires()) {
                const std::shared_ptr<Net>& wire = itWire.first;
                const PIN_MAGIC pinMagic = itWire.second;
                const std::shared_ptr<WireElement>& wireElement = _wires[wire];
                Connect(deviceElement, wireElement, pinMagic);
            }
        }
    };
    auto LoadWires = [this](const std::shared_ptr<Cell>& cell) {
        const NETLIST_ID& id = cell->GetNetlist()->GetID();
        for (const auto& it : cell->GetWires()) {
            const std::shared_ptr<Net>& wire = it.second;
            const std::shared_ptr<WireElement>& wireElement = std::make_shared<WireElement>(wire, id);
            _wires.emplace(wire, wireElement);
        }
    };
    _wires.reserve((_cell1->GetWires().size() + _cell2->GetWires().size()));
    LoadWires(_cell1); LoadWires(_cell2);
    _deviceElements.reserve((_cell1->GetDevices().size() + _cell2->GetDevices().size()));
    LoadDevices(_cell1);
    LoadDevices(_cell2);
}

COMPARE_CELL_RESULT CompareCell::Compare() {
    if (!AssignInitialBuckets()) {
        // debug
        // std::cout << "Initial Error" << std::endl;
        return COMPARE_CELL_FALSE;
    }
    static const bool debug = Debug::GetInstance().executeWLShow;
    std::string information;
    AUTOMORPHISM_GROUPS status = WeisfeilerLehman();
    switch (status) {
    case ITERATE_RIGHT_END:
        information = "Compare " + _cell1->GetName() + " and " + _cell2->GetName() + " result is true.";
        printf("%s\n", information.c_str());
        return COMPARE_CELL_TRUE;
    case ITERATE_RESULT_FALSE:
        information = "Compare " + _cell1->GetName() + " and " + _cell2->GetName() + " result is false.";
        printf("%s\n", information.c_str());
        return COMPARE_CELL_FALSE;
    default:
        if (debug) {
            information = "Compare " + _cell1->GetName() + " and " + _cell2->GetName() + " result is not false with " + std::to_string(status) + " automorphisms.";
            printf("%s\n", information.c_str());
        }
        break;
    }
    return ResolveAutomorphism();
}

bool CompareCell::AssignInitialBuckets() {
    auto BuildInitialDeviceColor = [](const std::shared_ptr<Device>& device) -> HASH_VALUE {
        std::hash<DEVICE_MODEL_NAME> DeviceModelHash; // set hash by deviceType and model name, might hash conflict, is better use uuid
        // return (device->GetDeviceType() ^ DeviceModelHash(device->GetModel())) % HASH_MOD_1;
        return (device->GetDeviceType() ^ DeviceModelHash(device->GetModel()));
    };
    auto BuildInitialWireColor = [](const std::shared_ptr<Net>& wire) -> HASH_VALUE {
        if (wire->GetPortIndex() == NOT_PORT) {
            return 999999 - (wire->GetConnectDevices()).size() * 2; // set hash by degree, is better use uuid prime number.
        } else {
            return 5000000 - (wire->GetConnectDevices()).size() * 2; // set hash by degree, is better use uuid prime number.
        }
    };
    for (const std::shared_ptr<DeviceElement>& deviceElement : _deviceElements) {
        deviceElement->newColor = BuildInitialDeviceColor(deviceElement->device);
    }
    for (const auto& it : _wires) {
        const std::shared_ptr<Net>& wire = it.first;
        const std::shared_ptr<WireElement>& wireElement = it.second;
        wireElement->newColor = BuildInitialWireColor(wire);
    }
    _lastBucketsId = 1;
    if (Debug::GetInstance().iterateShow) {
        std::cout << "iterate step 0" << std::endl;
    }
    AssignBuckets();
    return BucketsCheck() >= 0;
}

AUTOMORPHISM_GROUPS CompareCell::WeisfeilerLehman() {
    const Debug& debug = Debug::GetInstance();
    static uint32_t times = 0;
    if (debug.executeWLShow) {
        printf("execute WL algorithm %d times\n", ++times);
    }
    uint32_t iterateStep = 0;
    ITERATE_STATUS status = ITERATE_WILL_CONTINUE;
    while (status == ITERATE_WILL_CONTINUE) {
        ++iterateStep;
        if (debug.iterateShow) {
            std::cout << "iterate step " << iterateStep << std::endl;
        }
        status = Iterate();
    }
    return static_cast<AUTOMORPHISM_GROUPS>(status);
}

ITERATE_STATUS CompareCell::Iterate() {
    uint32_t oldDeviceBucketsNum = _deviceBuckets[_lastBucketsId].size();
    uint32_t oldWireBucketsNum = _wireBuckets[_lastBucketsId].size();
    // debug
    // std::cout << "Before Iterate, lastBucketId is " << static_cast<int16_t>(_lastBucketsId) << std::endl;
    IterateColor();
    AssignBuckets(); // set buckets by color
    // debug
    // std::cout << "After Iterate, lastBucketId is " << static_cast<int16_t>(_lastBucketsId) << std::endl;
    AUTOMORPHISM_GROUPS automorphisms = BucketsCheck();
    if (automorphisms == ITERATE_RESULT_FALSE) {
        return ITERATE_RESULT_FALSE;
    }
    if (_deviceBuckets[_lastBucketsId].size() == oldDeviceBucketsNum && _wireBuckets[_lastBucketsId].size() == oldWireBucketsNum) {
        return automorphisms;
    }
    return ITERATE_WILL_CONTINUE;
}

void CompareCell::AssignBuckets() {
    _lastBucketsId ^= 1;
    _deviceBuckets[_lastBucketsId].clear();
    _wireBuckets[_lastBucketsId].clear();
    AssignDeviceBuckets();
    AssignWireBuckets();
}

void CompareCell::AssignDeviceBuckets() {
    for (const std::shared_ptr<DeviceElement>& deviceElement : _deviceElements) {
        const auto& it = _deviceBuckets[_lastBucketsId].find(deviceElement);
        if (it == _deviceBuckets[_lastBucketsId].end()) {
            DeviceBucket newBucket;
            newBucket.oldColor = deviceElement->oldColor;
            newBucket.newColor = deviceElement->newColor;
            newBucket.degree = deviceElement->degree;
            newBucket.graphNodes.emplace_back(deviceElement);
            _deviceBuckets[_lastBucketsId].emplace(deviceElement, newBucket);
        } else {
            DeviceBucket& bucket = it->second;
            bucket.graphNodes.emplace_back(deviceElement);
        }
    }
}

void CompareCell::AssignWireBuckets() {
    for (const auto& itWire : _wires) {
        const std::shared_ptr<WireElement>& wireElement = itWire.second;
        const auto& it = _wireBuckets[_lastBucketsId].find(wireElement);
        if (it == _wireBuckets[_lastBucketsId].end()) {
            WireBucket newBucket;
            newBucket.oldColor = wireElement->oldColor;
            newBucket.newColor = wireElement->newColor;
            newBucket.degree = wireElement->degree;
            newBucket.graphNodes.emplace_back(wireElement);
            _wireBuckets[_lastBucketsId].emplace(wireElement, newBucket);
        } else {
            WireBucket& bucket = it->second;
            bucket.graphNodes.emplace_back(wireElement);
        }
    }
}

void CompareCell::IterateColor() {
    AssignNewDeviceColorToOld();
    AssignNewWireColorToOld();
    UpdateDevicesColor();
    UpdateWiresColor();
}

void CompareCell::AssignNewDeviceColorToOld() {
    std::unordered_map<std::shared_ptr<DeviceElement>, DeviceBucket, DeviceElementHash, DeviceElementEqual>& oldBuckets = _deviceBuckets[_lastBucketsId];
    for (const auto& it : oldBuckets) {
        const DeviceBucket& bucket = it.second;
        for (const std::shared_ptr<DeviceElement>& deviceElement : bucket.graphNodes) {
            deviceElement->oldColor = bucket.newColor; // is better to use uuid/guid, but now not done.
        }
    }
}

void CompareCell::AssignNewWireColorToOld() {
    std::unordered_map<std::shared_ptr<WireElement>, WireBucket, WireElementHash, WireElementEqual>& oldBuckets = _wireBuckets[_lastBucketsId];
    for (const auto& it : oldBuckets) {
        const WireBucket& bucket = it.second;
        for (const std::shared_ptr<WireElement>& wireElement : bucket.graphNodes) {
            wireElement->oldColor = bucket.newColor;
        }
    }
}

void CompareCell::UpdateDevicesColor() {
    for (const std::shared_ptr<DeviceElement>& deviceElement : _deviceElements) {
        deviceElement->newColor = GetDeviceNewColor(deviceElement);
    }
}

void CompareCell::UpdateWiresColor() {
    for (const auto& it : _wires) {
        const std::shared_ptr<WireElement>& wireElement = it.second;
        wireElement->newColor = GetWireNewColor(wireElement);
    }
}

HASH_VALUE CompareCell::GetDeviceNewColor(const std::shared_ptr<DeviceElement>& deviceElement) {
    HASH_VALUE result = deviceElement->oldColor;
    for (const auto& it : deviceElement->_connectWireElements) {
        const std::shared_ptr<WireElement>& wireElement = it.first; // is better use unique_ptr point to here rather than unordered_map
        const PIN_MAGIC pinMagic = it.second;
        result += wireElement->oldColor * pinMagic;
        // result += wireElement->oldColor * pinMagic % HASH_MOD_1; // customized hash function, is better to use more than 1 mod to avoid hash conflict.but here use only 1 mod.
        // result %= HASH_MOD_1;
    }
    return result;
}

HASH_VALUE CompareCell::GetWireNewColor(const std::shared_ptr<WireElement>& wireElement) {
    HASH_VALUE result = wireElement->oldColor;
    for (const auto& it : wireElement->_connectDeviceElements) {
        const std::shared_ptr<DeviceElement>& deviceElement = it.first.lock();
        const PIN_MAGIC pinMagic = it.second;
        result += deviceElement->oldColor * pinMagic;
        // result += deviceElement->oldColor * pinMagic % HASH_MOD_1;
        // result %= HASH_MOD_1;
    }
    return result;
}

AUTOMORPHISM_GROUPS CompareCell::BucketsCheck() {
    if (DeviceBucketsCheck(_deviceBuckets[_lastBucketsId]) == ITERATE_RESULT_FALSE) {
        return ITERATE_RESULT_FALSE;
    }
    if (WireBucketsCheck(_wireBuckets[_lastBucketsId]) == ITERATE_RESULT_FALSE) {
        return ITERATE_RESULT_FALSE;
    }
    return _automorphismDeviceBuckets.size() + _automorphismWireBuckets.size();
}

AUTOMORPHISM_GROUPS CompareCell::DeviceBucketsCheck(std::unordered_map<std::shared_ptr<DeviceElement>,
                                                    DeviceBucket, DeviceElementHash, DeviceElementEqual>& buckets) {
    _automorphismDeviceBuckets.clear();
    static const bool debug = Debug::GetInstance().iterateShow;
    AUTOMORPHISM_GROUPS automorphisms = 0;
    for (auto& itBucket : buckets) {
        auto& bucket = itBucket.second;
        if (DeviceBucketCheck(bucket) == ITERATE_RESULT_FALSE) {
            if (!debug) {
                return ITERATE_RESULT_FALSE;
            }
            automorphisms = ITERATE_RESULT_FALSE;
        }
    }
    return automorphisms == ITERATE_RESULT_FALSE ? ITERATE_RESULT_FALSE : _automorphismDeviceBuckets.size();
}

AUTOMORPHISM_GROUPS CompareCell::WireBucketsCheck(std::unordered_map<std::shared_ptr<WireElement>,
                                                  WireBucket, WireElementHash, WireElementEqual>& buckets) {
    _automorphismWireBuckets.clear();
    static const bool debug = Debug::GetInstance().iterateShow;
    AUTOMORPHISM_GROUPS automorphisms = 0;
    for (auto& itBucket : buckets) {
        auto& bucket = itBucket.second;
        if (WireBucketCheck(bucket) == ITERATE_RESULT_FALSE) {
            if (!debug) {
                return ITERATE_RESULT_FALSE;
            }
            automorphisms = ITERATE_RESULT_FALSE;
        }
    }
    return automorphisms == ITERATE_RESULT_FALSE ? ITERATE_RESULT_FALSE : _automorphismWireBuckets.size();
}

AUTOMORPHISM_GROUPS CompareCell::DeviceBucketCheck(DeviceBucket& bucket) {
    static const bool debug = Debug::GetInstance().iterateShow;
    if (debug) {
        std::cout << "device bucket {oldColor=" << bucket.oldColor << " newColor=" << bucket.newColor << "} :";
    }
    uint32_t id1 = 0, id2 = 0;
    for (const auto& deviceElement : bucket.graphNodes) {
        if (debug) {
            std::cout << ' ' << deviceElement->name << "(" << static_cast<int16_t>(deviceElement->netlistId) << ")";
        }
        deviceElement->netlistId == NETLIST_1 ? ++id1 : ++id2;
    }
    if (debug) {
        std::cout << std::endl;
    }
    if (id1 != id2) {
        return ITERATE_RESULT_FALSE;
    }
    if (id1 == 1) {
        return 0;
    }
    _automorphismDeviceBuckets.emplace_back(&bucket);
    return 1;
}

AUTOMORPHISM_GROUPS CompareCell::WireBucketCheck(WireBucket& bucket) {
    static const bool debug = Debug::GetInstance().iterateShow;
    if (debug) {
        std::cout << "wire bucket {oldColor=" << bucket.oldColor << " newColor=" << bucket.newColor << "} :";
    }
    uint32_t id1 = 0, id2 = 0;
    for (const auto& wireElement : bucket.graphNodes) {
        if (debug) {
            std::cout << ' ' << wireElement->name << "(" << static_cast<int16_t>(wireElement->netlistId) << ")";
        }
        wireElement->netlistId == NETLIST_1 ? ++id1 : ++id2;
    }
    if (debug) {
        std::cout << std::endl;
    }
    if (id1 != id2) {
        return ITERATE_RESULT_FALSE;
    }
    if (id1 == 1) {
        return 0;
    }
    _automorphismWireBuckets.emplace_back(&bucket);
    return 1;
}

COMPARE_CELL_RESULT CompareCell::ResolveAutomorphism() {
    AUTOMORPHISM_GROUPS result = 1;
    ResolveAutomorphismByProperty();
    result = WeisfeilerLehman();
    if (result == ITERATE_RESULT_FALSE) {
        return COMPARE_CELL_FALSE;
    } else if (result == ITERATE_RIGHT_END) {
        return COMPARE_CELL_TRUE;
    }

    ResolveAutomorphismByPin();
    result = WeisfeilerLehman();
    if (result == ITERATE_RESULT_FALSE) {
        return COMPARE_CELL_FALSE;
    } else if (result == ITERATE_RIGHT_END) {
        return COMPARE_CELL_TRUE;
    }

    while (result > 0) {
        ResolveAutomorphismForce();
        result = WeisfeilerLehman();
    }
    if (result == 0) {
        return COMPARE_CELL_TRUE;
    }
    return COMPARE_CELL_FALSE;
}

void CompareCell::ResolveAutomorphismByProperty() {
    for (const DeviceBucket* bucket: _automorphismDeviceBuckets) { // 遍历每个桶
        HASH_VALUE originColor = bucket->newColor; // 该桶本身的color
        for (auto it1 = bucket->graphNodes.begin(); it1 != bucket->graphNodes.end(); ++it1) {
            const std::shared_ptr<DeviceElement>& deviceElement1 = *it1;
            if (deviceElement1->newColor != originColor) {
                continue;
            }
            HASH_VALUE resetColor = Rand(); // 需要重置成的新color
            deviceElement1->newColor = resetColor;
            uint32_t id1 = 1, id2 = 0;
            const NETLIST_ID netlistId1 =  deviceElement1->netlistId;
            for (auto it2 = std::next(it1); it2 != bucket->graphNodes.end(); ++it2) {
                const std::shared_ptr<DeviceElement>& deviceElement2 = *it2;
                if (deviceElement2->newColor != originColor) {
                    continue;
                }
                if (deviceElement1->device->PropertyCompare(deviceElement2->device)) { // property same
                    deviceElement2->newColor = resetColor;
                    const NETLIST_ID netlistId2 = deviceElement2->netlistId;
                    netlistId1 == netlistId2? ++id1: ++id2;
                }
            }

            while (id2 > id1) {
                for (auto it2 = bucket->graphNodes.begin(); it2 != bucket->graphNodes.end(); ++it2) {
                    const std::shared_ptr<DeviceElement>& deviceElement2 = *it2;
                    if (deviceElement2->newColor == resetColor && deviceElement2->netlistId != netlistId1) {
                        deviceElement2->newColor = originColor;
                        if (-- id2 == id1) {
                            break;
                        }
                    }
                }
            }

            while (id1 > id2) {
                for (auto it2 = bucket->graphNodes.begin(); it2 != bucket->graphNodes.end(); ++it2) {
                    const std::shared_ptr<DeviceElement>& deviceElement2 = *it2;
                    if (deviceElement2->newColor == resetColor && deviceElement2->netlistId == netlistId1) {
                        deviceElement2->newColor = originColor;
                        if (-- id1 == id2) {
                            break;
                        }
                    }
                }
            }
        }
    }
    AssignBuckets();
}

void CompareCell::ResolveAutomorphismByPin() {

    AssignBuckets();
}

void CompareCell::ResolveAutomorphismForce() {
    DeviceBucket* dPrivileged = nullptr;
    for (DeviceBucket* bucket : _automorphismDeviceBuckets) {
        if (dPrivileged == nullptr) {
            dPrivileged = bucket;
        } else if (*bucket > *dPrivileged) {
            dPrivileged = bucket;
        }
    }
    WireBucket* wPrivileged = nullptr;
    for (WireBucket* bucket : _automorphismWireBuckets) {
        if (wPrivileged == nullptr) {
            wPrivileged = bucket;
        } else if (*bucket > *wPrivileged) {
            wPrivileged = bucket;
        }
    }
    if (wPrivileged == nullptr) {
        ResetDeviceBucketColor(*dPrivileged);
    } else if (dPrivileged == nullptr) {
        ResetWireBucketColor(*wPrivileged);
    } else if (*dPrivileged > *wPrivileged) {
        ResetDeviceBucketColor(*dPrivileged);
    } else {
        ResetWireBucketColor(*wPrivileged);
    }
    AssignBuckets();
}

void CompareCell::ResetDeviceBucketColor(DeviceBucket& bucket) {
    std::shared_ptr<DeviceElement> deviceElement1 = nullptr, deviceElement2 = nullptr;
    for (const std::shared_ptr<DeviceElement> deviceElement : bucket.graphNodes) {
        if (deviceElement->netlistId == NETLIST_1) {
            if (deviceElement1 == nullptr) {
                deviceElement1 = deviceElement;
                if (deviceElement2 != nullptr) {
                    break;
                }
            }
        } else {
            if (deviceElement2 == nullptr) {
                deviceElement2 = deviceElement;
                if (deviceElement1 != nullptr) {
                    break;
                }
            }
        }
    }
    deviceElement1->newColor = deviceElement2->newColor = rand();
    // deviceElement1->newColor = deviceElement2->newColor = rand() % HASH_MOD_1;
}

void CompareCell::ResetWireBucketColor(WireBucket& bucket) {
    std::shared_ptr<WireElement> wireElement1 = nullptr, wireElement2 = nullptr;
    for (const std::shared_ptr<WireElement> wireElement : bucket.graphNodes) {
        if (wireElement->netlistId == NETLIST_1) {
            if (wireElement1 == nullptr) {
                wireElement1 = wireElement;
                if (wireElement2 != nullptr) {
                    break;
                }
            }
        } else {
            if (wireElement2 == nullptr) {
                wireElement2 = wireElement;
                if (wireElement1 != nullptr) {
                    break;
                }
            }
        }
    }
    wireElement1->newColor = wireElement2->newColor = rand();
    // wireElement1->newColor = wireElement2->newColor = rand() % HASH_MOD_1;
}