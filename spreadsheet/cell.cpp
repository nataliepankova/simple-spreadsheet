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
    impl_(std::make_unique<EmptyImpl>()) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        impl_ = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    }
    else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }

}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
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
