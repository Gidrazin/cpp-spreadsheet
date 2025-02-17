#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>
#include <optional>

class Sheet;

class Cell : public CellInterface {

public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    bool IsChildren() const;

private:

    class Impl {
    public:
        Impl() = default;
        Impl(Cell* cell);
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
    protected:
        Cell* cell_ = nullptr;
    };

    class EmptyImpl final : public Impl {
    public:
        Value GetValue() const override;
        std::string GetText() const override;
    };

    class TextImpl final : public Impl {
    public:
        explicit TextImpl(const std::string& text);
        Value GetValue() const override;
        std::string GetText() const override;
    private:
        std::string value_;
        std::string text_;
    };

    class FormulaImpl final : public Impl {
    public:
        explicit FormulaImpl(Cell* cell, const std::string& text);
        Value GetValue() const override;
        std::string GetText() const override;
    private:
        std::unique_ptr<FormulaInterface> formula_;
        std::string text_;
        
    };

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::unordered_set<Cell*> parent_cells_; //Родительские ячейки, которые в своих формулах содержат данную ячейку
    std::unordered_set<Cell*> children_cells_; //Дочерние ячейки, которые содержатся в формуле данной ячейки
    std::vector<Position> referenced_cells_; // Позиции дочерних ячеек
    mutable std::optional<Value> cache_; // Кэш значения ячейки

    bool HasCycle() const; // Функция для выявления циклической ссылки
    bool CellExist(Position pos) const; //Проверяет, существует ли экземпляр класса Cell по данной позиции
    void CacheDestroy() const; //Каскадно инвалидирует кэш в родительских ячейках
};