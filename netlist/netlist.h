#pragma once

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include "../netlist/cell.h"

class Spice;
class Netlist: public std::enable_shared_from_this<Netlist> {
    struct Error {
        int32_t errorLine;
        std::string errorInformation;
    };
private:
    friend class Spice;
    friend class Layout;
    friend class CompareNetlist;
private:
    Error _error;

    NETLIST_ID _id;
    std::shared_ptr<Cell> _topCell;
    std::unordered_map<CELL_NAME, std::shared_ptr<Cell>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> _cells;
    std::unordered_set<std::shared_ptr<Cell> > _validCells;
private:
    std::shared_ptr<Cell> FindCell(const CELL_NAME& name) const;
    std::shared_ptr<Cell> DefineCell(const CELL_NAME& cellName);
    READ_STATE QuotePointToCell(const std::shared_ptr<Cell>& cell);
    bool BuildHierarchyStructure();

    static void BuildDependencyRelationShip(const std::shared_ptr<Cell>& parent, const std::shared_ptr<Quote>& quote);
    bool JudgeLoop() const;
public:
    Netlist& operator= (const Netlist&) = delete;

    void SetID(const NETLIST_ID& id);
    NETLIST_ID GetID() const;

    std::shared_ptr<Cell> GetTopCell() const;

    std::string OutputError() const;

    // test
    void Show();
    void ShowHierarchyStructure() const;
};