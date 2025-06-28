#pragma once

#include <memory>
#include <string>
#include <list>
#include "../base/base.h"

class Cell;
class Netlist;
class Device;

class Net {
private:
    NET_NAME _name;
    std::weak_ptr<Cell> _cell;
    std::weak_ptr<Netlist> _netlist;

    PORT_INDEX _portIndex = NOT_PORT;
    std::list<std::pair<std::weak_ptr<Device>, PIN_MAGIC> > _connectDevices;
public:
    Net(const NET_NAME& name);
    Net(const Net&) = delete;
    Net& operator=(const Net&) = delete;

    NET_NAME GetName() const;

    std::shared_ptr<Cell> GetCell() const;
    void SetCell(const std::shared_ptr<Cell>& cell);

    std::shared_ptr<Netlist> GetNetlist() const;
    void SetNetlist(const std::shared_ptr<Netlist>& netlist);

    PORT_INDEX GetPortIndex() const;
    void SetPortIndex(const PORT_INDEX _portIndex);

    std::list<std::pair<std::weak_ptr<Device>, PIN_MAGIC> >& GetConnectDevices();
};