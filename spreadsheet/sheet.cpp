#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() = default;

std::unique_ptr<Cell> Sheet::CreateCell(Position pos, std::string text) {
    std::unique_ptr<Cell> new_cell = std::make_unique<Cell>(*this, pos);
    new_cell->Set(text);
    return new_cell;
}

void Sheet::EmplaceCell(Position pos, std::unique_ptr<Cell>& new_cell) {
    sheet_.resize(std::max(pos.row + 1, int(sheet_.size())));
    sheet_[pos.row].resize(std::max(pos.col + 1, int(sheet_[pos.row].size())));

    auto& ptr_to_cell = sheet_[pos.row][pos.col];
    ptr_to_cell.reset(new_cell.release());
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    std::unique_ptr<Cell> new_cell = CreateCell(pos, text);

    std::vector<Position> referenced_cells = new_cell->GetReferencedCells();

    if (!referenced_cells.empty()) {
        if (HasCyclicDependencies(pos, referenced_cells)) {
            throw CircularDependencyException("Cell formula has circular dependencies"s);
        }
        // tell referenced cells they have a new dependant
        AddUpperRefToCells(pos, referenced_cells);
    }
    // invalidate cache of dependant cells
    if (upper_references_.count(pos) > 0) {
        for (const Position& cell : upper_references_.at(pos)) {
            Cell* cell_data = GetConcreteCell(cell);
            cell_data->ClearCache();
        }
    }
    EmplaceCell(pos, new_cell);

}

void Sheet::AddUpperRefToCells(Position cell, std::vector<Position> referenced_cells) {
    for (const Position& pos : referenced_cells) {
        auto cell_data = GetCell(pos);
        // create referenced cell if it doesn't exist
        if (!cell_data) {
            SetCell(pos, "");
        }
        upper_references_[pos].insert(cell);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (pos.row >= int(sheet_.size()) || pos.col >= int(sheet_[pos.row].size())) {
        return nullptr;
    }
    return sheet_[pos.row][pos.col].get();
}
CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (pos.row >= int(sheet_.size()) || pos.col >= int(sheet_[pos.row].size())) {
        return nullptr;
    }
    return sheet_[pos.row][pos.col].get();
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    return dynamic_cast<const Cell*>(GetCell(pos));
}
Cell* Sheet::GetConcreteCell(Position pos) {
    return dynamic_cast<Cell*>(GetCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (pos.row < int(sheet_.size()) && pos.col < int(sheet_[pos.row].size()) && sheet_[pos.row][pos.col]) {
        sheet_[pos.row][pos.col]->Clear();
        sheet_[pos.row][pos.col].reset();
    }
}

Size Sheet::GetPrintableSize() const {
    Size result;
    for (int row = 0; row < int(sheet_.size()); ++row) {
        for (int col = int(sheet_[row].size()) - 1; col >= 0; --col) {
            if (!sheet_[row][col] || sheet_[row][col]->GetText().empty()) {
                continue;
            }
            result.rows = std::max(result.rows, row + 1);
            result.cols = std::max(result.cols, col + 1);
            break;
        }
    }
    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size printable_size = GetPrintableSize();
    for (int row = 0; row < printable_size.rows; ++row) {
        for (int col = 0; col < printable_size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (col >= int(sheet_[row].size()) || !sheet_[row][col]) {
                continue;
            }
            std::visit(
                [&](const auto& x) {
                    output << x;
                },
                sheet_[row][col]->GetValue());

        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    Size printable_size = GetPrintableSize();
    for (int row = 0; row < printable_size.rows; ++row) {
        for (int col = 0; col < printable_size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (col >= int(sheet_[row].size()) || !sheet_[row][col]) {
                continue;
            }
            output << sheet_[row][col]->GetText();
        }
        output << '\n';
    }
}

bool Sheet::HasCyclicDependencies(Position pos, const std::vector<Position> references_down) const {
    std::set<Position> cells_to_check = { pos };
    return HasCyclicDependencies(cells_to_check, references_down);
}

bool Sheet::HasCyclicDependencies(std::set<Position>& cells_to_check, const std::vector<Position>& references_down) const {
    for (const Position& ref : references_down) {
        // try insert ref cell to set
        if (!cells_to_check.insert(ref).second) {
            return true;
        }
        auto ref_data = GetCell(ref);
        if (ref_data) {
            if (HasCyclicDependencies(cells_to_check, ref_data->GetReferencedCells())) {
                return true;
            }
        }
    }
    return false;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
