#pragma once

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdint.h>
#include <sstream>
#include <map>
#include "express.h"

typedef std::string CELL_NAME;
typedef std::string DEVICE_NAME;
typedef std::string WIRE_NAME;
typedef std::string DEVICE_MODEL_NAME;
typedef std::string PROPERTY_NAME;
typedef std::variant<double, std::string> PROPERTY_VALUE;

typedef int8_t READ_STATE;
constexpr READ_STATE NO_FILE = -1; // can't find route
constexpr READ_STATE READ_OK = 1;
constexpr READ_STATE SUBCKT_NO_NAME = 2; // no name after ".subckt"
constexpr READ_STATE SUBCKT_NO_ENDS = 3; // no ".ends" match with ".subckt"
constexpr READ_STATE SUBCKT_REDEFINE = 4; // same subckt name
constexpr READ_STATE SUBCKT_PORT_REDEFINE = 5; // same port name in the same subckt
constexpr READ_STATE SUBCKT_DEVICE_REDEFINE = 6; // device redefine in the same subckt
constexpr READ_STATE ENDS_NO_MATCH_SUBCKT = 11; // no ".ends" match with ".ends"
constexpr READ_STATE READ_MOSFET_ERROR = 21; // read mosfet error
constexpr READ_STATE QUOTE_CANT_FIND_CELL = 31; // quote can't find cell
constexpr READ_STATE QUOTE_PORT_NUMBER_ERROR = 42; // quote find cell but the number of ports is not matched.
constexpr READ_STATE HIERARCHY_LOOP = -128; // hierarchy structure has loop

typedef int32_t PORT_INDEX;
constexpr PORT_INDEX NOT_PORT = -1;

typedef uint64_t HASH_VALUE;
constexpr HASH_VALUE HASH_MOD_1 = 1000000007ll;
constexpr HASH_VALUE HASH_MOD_2 = 1000000009ll;

/* All the large numbers here are prime numbers. They were directly transplanted here. It would be best to use some other prime numbers */
typedef HASH_VALUE DEVICE_TYPE;
constexpr DEVICE_TYPE DEVICE_TYPE_MOSFET = 452375449ll;
constexpr DEVICE_TYPE DEVICE_TYPE_QUOTE = 807303071ll;

typedef HASH_VALUE PIN_MAGIC;
constexpr PIN_MAGIC PIN_MAGIC_NO_DEFINE = 0;
constexpr PIN_MAGIC PIN_MAGIC_M_1 = 581880311ll;
constexpr PIN_MAGIC PIN_MAGIC_M_2 = 772136249ll;
constexpr PIN_MAGIC PIN_MAGIC_M_3 = PIN_MAGIC_M_1;
constexpr PIN_MAGIC PIN_MAGIC_M_4 = 928917673ll;

typedef uint8_t NETLIST_ID;
constexpr NETLIST_ID NETLIST_1 = 0;
constexpr NETLIST_ID NETLIST_2 = 1;

typedef int8_t COMPARE_NETLIST_RESULT;
constexpr COMPARE_NETLIST_RESULT COMPARE_NETLIST_TRUE = 1;
constexpr COMPARE_NETLIST_RESULT COMPARE_NETLIST_FALSE = 0;

typedef int8_t COMPARE_CELL_RESULT;
constexpr COMPARE_CELL_RESULT COMPARE_CELL_TRUE = 1;
constexpr COMPARE_CELL_RESULT COMPARE_CELL_FALSE = 0;

typedef int32_t ITERATE_STATUS; // >0  automorphism
constexpr ITERATE_STATUS ITERATE_RESULT_FALSE = -1;
constexpr ITERATE_STATUS ITERATE_WILL_CONTINUE = -2;
constexpr ITERATE_STATUS ITERATE_RIGHT_END = 0;

typedef int32_t AUTOMORPHISM_GROUPS;
// <0 return ITERATE_RESULT_FALSE

static std::ostringstream OUT;
void OutputToFileOrTerminal();

static std::map<DEVICE_TYPE, std::vector<PIN_MAGIC> > pinMagicTable = {
    {DEVICE_TYPE_MOSFET, {PIN_MAGIC_M_1, PIN_MAGIC_M_2, PIN_MAGIC_M_3, PIN_MAGIC_M_4}}
};

static std::map<PIN_MAGIC, std::string> pinNameTable = {
    {PIN_MAGIC_NO_DEFINE, {"pending quote"}},
    {PIN_MAGIC_M_1, "M_pin1/3"}, {PIN_MAGIC_M_2, "M_pin2"}, {PIN_MAGIC_M_3, "M_pin1/3"}, {PIN_MAGIC_M_4, "M_pin4"}
};

inline std::string GetPinName(PIN_MAGIC pinMagic) {
    return !pinNameTable[pinMagic].empty()? pinNameTable[pinMagic]: std::to_string(pinMagic);
}

std::vector<std::string> SplitString(const std::string& str);

bool MatchNoCase(const std::string& str1, const std::string& str2);

struct TestCase {
    // config
    std::string file1; std::string topCell1; std::string file2; std::string topCell2; std::string outputFileName;
    bool caseInsensitive; bool hier; bool autoMatch; bool multiThread; double tolerance;

    // debug
    bool executeWLShow = 1; bool iterateShow = 0; bool cellShow = 0;
};

static TestCase testCase[] = {
    { // 0
        "hier1.spice", "", "hier2.spice", "", "",
        false, true, false, false, 0.0,
        true, true, false
    },
    { // 1
        "layout.sp", "RAS1024X16", "layout2.sp", "RAS1024X16", "case1_res",
        false, true, false, false, 0.0,
        true, false, false
    },
    { // 2
        "layout.sp", "RAS1024X16", "layout2.sp", "RAS1024X16", "case2_res",
        false, true, false, true, 0.0,
        true, false, false
    },
    { // 3
        "layout.sp", "SOP_DC_X128Y8_620", "layout2.sp", "SOP_DC_X128Y8_620", "case3_res",
        false, true, false, false, 0.0,
        true, false, false
    },
    { // 4
        "layout.sp", "Logic_leafcell_X128Y8_VHS", "layout2.sp", "Logic_leafcell_X128Y8_VHS", "case4_res",
        false, true, false, false, 0.0,
        true, false, false
    },
    { // 5
        "layout.sp", "LEAF_XDEC4_VHSSRAM", "layout2.sp", "LEAF_XDEC4_VHSSRAM", "case5_res",
        false, true, false, false, 0.0,
        true, false, false
    },
    { // 6
        "layout.sp", "Logic_leafcell_X128Y8_VHS", "layout2.sp", "Logic_leafcell_X128Y8_VHS", "case6_res",
        false, true, false, false, 0.0,
        true, false, false
    }
};

HASH_VALUE Rand();
void SettingConfig(TestCase& testCase);