#pragma once

#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "compare_cell.h"

class CompareNetlist {
    private:
        friend class CompareCell;
    private:
        struct CellElement;
        struct SimilarCell {
            std::weak_ptr<CellElement> cell;
            double similarity;
            bool operator< (const SimilarCell& another) {
                return similarity < another.similarity;
            }
        };
        struct CellElement {
            std::shared_ptr<Cell> cell;
            std::atomic<uint32_t> outDegree;
            bool flattened;
            std::priority_queue<SimilarCell> similarCells;
            std::weak_ptr<CellElement> targetCell;
            std::weak_ptr<CellElement> matched;
            DEVICE_MODEL_NAME label;
            std::mutex atomizeMutex;
            CellElement(const std::shared_ptr<Cell>& cell_): cell(cell_) {
                outDegree = cell_->_outDegree;
                flattened = false;
            }
        };
    private:
        std::mutex mtx;
        std::mutex queueMutex;
        std::condition_variable cvQueueNotEmpty;
        std::shared_ptr<Netlist> _netlist1, _netlist2;
        std::unordered_map<std::shared_ptr<Cell>, std::shared_ptr<CellElement> > _cells1, _cells2;
    private:
        void LoadData();
        void LoadCells(const std::shared_ptr<Netlist>& netlist, std::unordered_map<std::shared_ptr<Cell>, std::shared_ptr<CellElement> >& cells);
        void BuildTargetCell();
        COMPARE_NETLIST_RESULT FullFlattenCompare();
        COMPARE_NETLIST_RESULT HierarchyCompare();
        COMPARE_NETLIST_RESULT OneThreadHierarchyCompare();
        COMPARE_NETLIST_RESULT MultiThreadHierarchyCompare();
        void AtomizeCell(const std::shared_ptr<CellElement>& cellElement); // flatten all quotes
        void FlattenOneQuote(const std::shared_ptr<CellElement>& cellElement, std::shared_ptr<Quote> quote);
        void QuoteToBeDevice(const std::shared_ptr<Quote>& quote) const;
        void DealCompareCellsTrue(const std::unique_ptr<CompareCell>& compareCell,
                    const std::shared_ptr<CellElement>& cellElement1, const std::shared_ptr<CellElement>& cellElement2);

        std::shared_ptr<CellElement> GetCellELement(const std::shared_ptr<Cell>& cell) const;
    public:
        CompareNetlist(std::shared_ptr<Netlist>& netlist1, std::shared_ptr<Netlist>& netlist2);
        COMPARE_NETLIST_RESULT Compare();
};