#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

FormulaError::FormulaError(Category category)
    : category_(category)
{}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    if (category_ == FormulaError::Category::Arithmetic) {
        return "#ARITHM!"sv;
    }
    else if (category_ == FormulaError::Category::Value) {
        return "#VALUE!"sv;
    }
    else {
        return "REF!"sv;
    }
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression)
            try : ast_(ParseFormulaAST(expression))
        {

        }
        catch (const std::exception& ex) {
            throw FormulaException(ex.what());
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                std::function<double(Position)> func = [&sheet](Position pos) {
                    if (!pos.IsValid()) {
                        throw FormulaError(FormulaError::Category::Ref);
                    }
                    const CellInterface* cell = sheet.GetCell(pos);
                    double result = 0.0;
                    if (cell) {
                        auto value = cell->GetValue();
                        if (std::holds_alternative<std::string>(value)) {
                            try {
                                size_t size;
                                std::string str_value = std::get<std::string>(value);
                                result = std::stod(str_value, &size);
                                if (size != str_value.size()) {
                                    throw FormulaError(FormulaError::Category::Value);
                                }
                            }
                            catch (...) {
                                throw FormulaError(FormulaError::Category::Value);
                            }
                        }
                        else if (std::holds_alternative<double>(value)) {
                            result = std::get<double>(value);
                        }
                        else {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                    }

                    return result;
                    };

                return ast_.Execute(func);
            }
            catch (const FormulaError& fe) {
                return fe;
            }
        }
        std::string GetExpression() const override {
            std::ostringstream output;
            ast_.PrintFormula(output);
            return output.str();
        }
        std::vector<Position> GetReferencedCells() const override {
            // the list is already sorted
            const std::forward_list<Position> cells = ast_.GetCells();
            std::vector<Position> referenced_cells(cells.begin(), cells.end());
            DeleteDuplicates(referenced_cells);
            return referenced_cells;
        }

    private:
        FormulaAST ast_;

        static void DeleteDuplicates(std::vector<Position>& cells) {
            auto it = std::unique(cells.begin(), cells.end());
            cells.resize(std::distance(cells.begin(), it));
        }
    };


}  // namespace


std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
