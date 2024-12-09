#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>
#include <set>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);

    bool HasCyclicDependencies(Position pos, const std::vector<Position> references_down) const;


private:
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;
    std::map<Position, std::set<Position, Comp>, Comp> upper_references_;

    std::unique_ptr<Cell> CreateCell(Position pos, std::string text);
    void EmplaceCell(Position pos, std::unique_ptr<Cell>& new_cell);
    void AddUpperRefToCells(Position cell, std::vector<Position> referenced_cells);
    bool HasCyclicDependencies(std::set<Position>& cells_to_check, const std::vector<Position>& references_down) const;
};
