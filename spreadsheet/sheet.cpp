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

void Sheet::ValidatePosition(Position pos) {
    using namespace std::literals;
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position"s);
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    ValidatePosition(pos);
    std::unique_ptr<Cell> new_cell = CreateCell(pos, text);
    EmplaceCell(pos, new_cell);

}

const CellInterface* Sheet::GetCell(Position pos) const {
    ValidatePosition(pos);
    if (pos.row >= int(sheet_.size()) || pos.col >= int(sheet_[pos.row].size())) {
        return nullptr;
    }
    return sheet_[pos.row][pos.col].get();
}
CellInterface* Sheet::GetCell(Position pos) {
    ValidatePosition(pos);
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
    ValidatePosition(pos);
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

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
