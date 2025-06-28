#pragma once

#include <vector>
#include <list>
#include <forward_list>
#include <string>
#include <unordered_map>
#include "device/mosfet.h"
#include "device/quote.h"
#include "port.h"

class Cell: public std::enable_shared_from_this<Cell> {
    private:
        friend class CompareNetlist;
    private:
        CELL_NAME _name;
        std::weak_ptr<Netlist> _netlist;

        std::vector<std::shared_ptr<Port> > _ports;
        std::forward_list<std::shared_ptr<Quote> > _quotes;
        std::unordered_map<DEVICE_NAME, std::shared_ptr<Device>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> _devices;
        std::unordered_map<WIRE_NAME, std::shared_ptr<Net>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> _nets;
        std::unordered_map<PARAMETER_NAME, PARAMETER_VALUE, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> _parameters;

        // weak_ptr<CompareNetlist::CellElement> _cellElement;
    public:
        uint32_t _inDegree, _outDegree;
        std::unordered_map<std::shared_ptr<Cell>, std::vector<std::shared_ptr<Quote> > > _sons; // record sons and quotes
        std::vector<std::weak_ptr<Cell> > _parents; // record parents
        // after parse can be abandon
        std::unordered_map<WIRE_NAME, PORT_INDEX, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> _portsMap;
        // shared_ptr<CompareNetlist::CellElement> GetCellElement() const;
    public:
        Cell(const CELL_NAME& name);
        Cell(const Cell&) = delete;
        Cell& operator= (const Cell&) = delete;

        CELL_NAME GetName() const;

        void SetNetlist(const std::shared_ptr<Netlist>& netlist);
        std::shared_ptr<Netlist> GetNetlist();

        std::vector<std::shared_ptr<Port> >& GetPorts();

        std::forward_list<std::shared_ptr<Quote> >& GetQuotes();

        std::unordered_map<DEVICE_NAME, std::shared_ptr<Device>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& GetDevices();
        std::unordered_map<WIRE_NAME, std::shared_ptr<Net>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& GetNets();
        std::unordered_map<PARAMETER_NAME, PARAMETER_VALUE, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& GetParameters();

        std::shared_ptr<Device> FindDevice(const DEVICE_NAME& name) const;
        std::shared_ptr<Net> FindNet(const WIRE_NAME& name) const;

        bool AddDevice(const std::shared_ptr<Device>& device);
        std::shared_ptr<Net> DefineNet(const WIRE_NAME& net);

        void SetParameterValue(const PARAMETER_NAME& parameterName, const std::string& str);

        void Show();
};