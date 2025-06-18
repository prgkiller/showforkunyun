#include <iostream>
#include <memory>
#include <string>
#include <chrono>

#include "compare/compare_netlist.h"

READ_STATE ReadOneFile(std::shared_ptr<Netlist>& netlist, const std::string& fileRoute, const std::string& topCellName)
{
    Config& config = Config::GetInstance();
    std::unique_ptr<Spice> spice = std::make_unique<Spice>();
    READ_STATE readState = spice->OpenReadAndParseSpice(fileRoute, topCellName);

    switch (readState) {
        case NO_FILE:
            std::cout << "Error, can't open file \"" << fileRoute << "\"" << std::endl;
            break;
        case READ_OK:
            netlist = spice->GetNetlist();
            // debug
            // std::cout << "==========netlist show==========" << std::endl;
            // netlist->Show();
            // std::cout << "========netlist show end========" << std::endl;
            break;
        case SUBCKT_NO_NAME:
            break;
        case SUBCKT_NO_ENDS:
            break;
        case QUOTE_CANT_FIND_CELL:
            std::cout << netlist->OutputError() << std::endl;
            break;
        case QUOTE_PORT_NUMBER_ERROR:
            break;
        case HIERARCHY_LOOP:
            std::cout << "HAVE LOOP!" << std::endl;
            break;
        default:
            std::cout << static_cast<int16_t>(readState) << std::endl;
            break;
    }

    return readState;
}

READ_STATE ReadTwoFiles(std::shared_ptr<Netlist>& netlist1, std::shared_ptr<Netlist>& netlist2)
{
    const Config& config = Config::GetInstance();
    READ_STATE readState;

    if ((readState = ReadOneFile(netlist1, config.file1, config.topCell1)) != READ_OK) {
        return readState;
    }

    return ReadOneFile(netlist2, config.file2, config.topCell2);
}

int main()
{
    SettingConfig(testCase[2]);
    Config& config = Config::GetInstance();

    auto start_clock = std::chrono::high_resolution_clock::now();

    std::shared_ptr<Netlist> netlist1 = nullptr, netlist2 = nullptr;

    if (ReadTwoFiles(netlist1, netlist2) != READ_OK) {
        // OutputToFileOrTerminal();
        return 0;
    }

    CompareNetlist cmp(netlist1, netlist2);
    COMPARE_NETLIST_RESULT result = cmp.Compare();

    // debug
    if (result == COMPARE_NETLIST_TRUE) {
        std::cout << "Compare True" << std::endl;
    } else if (result == COMPARE_NETLIST_FALSE) {
        std::cout << "Compare False" << std::endl;
    }

    // OutputToFileOrTerminal();
    auto end_clock = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_clock - start_clock);
    std::cout << "Run time " << duration.count() * 0.000001 << "s." << std::endl;

    return 0;
}
