#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

// ===== Empty cell impl ======

Cell::Value Cell::EmptyImpl::GetValue() const {
    return 0.0;
}

std::string Cell::EmptyImpl::GetText() const {
    using namespace std::literals;
    return ""s;
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
    return {};
}

// ===== Text cell impl =====

Cell::TextImpl::TextImpl(std::string text)
    : text_(std::move(text)) {}

Cell::Value Cell::TextImpl::GetValue() const {
    if (text_[0] == ESCAPE_SIGN) {
        return text_.substr(1);
    }
    else {
        return text_;
    }
}

std::string Cell::TextImpl::GetText() const {
    return text_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
    return {};
}

// ===== Formula cell impl =====
Cell::FormulaImpl::FormulaImpl(std::string text, const Sheet& sheet)
    : formula_(ParseFormula(text.substr(1))),
      sheet_(sheet){}

Cell::Value Cell::FormulaImpl::GetValue() const {
    auto value = formula_->Evaluate(sheet_);
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    else {
        return std::get<FormulaError>(value);
    }
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

// ==== Cell methods ====

Cell::Cell(Sheet& sheet, Position pos)
    : sheet_(sheet),
    pos_(pos),
    impl_(std::make_unique<EmptyImpl>()) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    using namespace std::literals;
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        impl_ = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    }
    else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
    std::vector<Position> referenced_cells = GetReferencedCells();

    if (!referenced_cells.empty()) {
        if (HasCyclicDependencies(referenced_cells)) {
            throw CircularDependencyException("Cell formula has circular dependencies"s);
        }
        // tell referenced cells they have a new dependant
        AddUpperRefToCells(referenced_cells);
    }
    // invalidate cache of dependant cells
    if (!upper_references_.empty()) {
        for (const Position& cell : upper_references_) {
            Cell* cell_data = sheet_.GetConcreteCell(cell);
            cell_data->ClearCache();
        }
    }

}

void Cell::Clear() {
    using namespace std::literals;
    Set(""s);
}

Cell::Value Cell::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = impl_->GetValue();
    }
    return *cache_;
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::ClearCache() const {
    cache_.reset();
}

bool Cell::IsReferenced() const {
    return !GetReferencedCells().empty();
}

void Cell::AddUpperRefToCells(std::vector<Position> referenced_cells) {
    for (const Position& pos : referenced_cells) {
        Cell* cell_data = sheet_.GetConcreteCell(pos);
        // create referenced cell if it doesn't exist
        if (!cell_data) {
            sheet_.SetCell(pos, "");
            cell_data = sheet_.GetConcreteCell(pos);
        }
        // add curret cell pos as upper reference
        cell_data->GetUpperReferences().insert(pos_);
    }
}

bool Cell::HasCyclicDependencies(const std::vector<Position> references_down) const {
    std::set<Position> cells_to_check = { pos_ };
    return HasCyclicDependencies(cells_to_check, references_down);
}

bool Cell::HasCyclicDependencies(std::set<Position>& cells_to_check, const std::vector<Position>& references_down) const {
    for (const Position& ref : references_down) {
        // try insert ref cell to set
        if (!cells_to_check.insert(ref).second) {
            return true;
        }
        auto ref_data = sheet_.GetCell(ref);
        if (ref_data) {
            if (HasCyclicDependencies(cells_to_check, ref_data->GetReferencedCells())) {
                return true;
            }
        }
    }
    return false;
}
