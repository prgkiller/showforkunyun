cmake_minimum_required(VERSION 3.28)
project(lvs)

set(CMAKE_CXX_STANDARD 20)

add_executable(lvs lvs_main.cpp
        parse/spice.cpp
        parse/spice.h
        parse/layout.cpp
        parse/layout.h
        netlist/netlist.cpp
        netlist/netlist.h
        netlist/device/device.cpp
        netlist/device/device.h
        netlist/wire.cpp
        netlist/wire.h
        netlist/device/quote.cpp
        netlist/device/quote.h
        netlist/device/mosfet.cpp
        netlist/device/mosfet.h
        netlist/cell.cpp
        netlist/cell.h
        compare/compare_netlist.cpp
        compare/compare_netlist.h
        compare/compare_cell.cpp
        compare/compare_cell.h
        config/config.h
        netlist/port.cpp
        netlist/port.h
        base/base.cpp
        base/base.h
        base/express.cpp
        base/express.h
)
