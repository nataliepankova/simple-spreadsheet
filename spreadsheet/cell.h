#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet, Position pos);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    void ClearCache() const;
    bool IsReferenced() const;

private:

    class Impl {
    public:
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual ~Impl() = default;
    };
    class EmptyImpl : public Impl {
    public:
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
    };
    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string text);
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
    private:
        std::string text_;
    };
    class FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(std::string text, const Sheet& sheet);
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
    private:
        std::unique_ptr<FormulaInterface> formula_;
        const Sheet& sheet_;
    };

    Sheet& sheet_;
    Position pos_;
    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cache_;
    std::set<Position, Comp> upper_references_;

    std::set<Position, Comp>& GetUpperReferences() {
        return upper_references_;
    }
    void AddUpperRefToCells(std::vector<Position> referenced_cells);
    bool HasCyclicDependencies(const std::vector<Position> references_down) const;
    bool HasCyclicDependencies(std::set<Position>& cells_to_check, const std::vector<Position>& references_down) const;

};
