#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <stack>
#include <string>
#include <optional>

using namespace std::literals;

Cell::Cell(Sheet& sheet)
: impl_(std::move(std::make_unique<EmptyImpl>()))
, sheet_(sheet) {
}

Cell::~Cell() = default;

struct CellCurrentStateBackup {
    //Структура для хранения текущего состояния ячейки
    CellCurrentStateBackup(std::unordered_set<Cell*>& children_cells,
                           std::optional<CellInterface::Value> cache,
                           std::vector<Position>& referenced_cells)
    : temp_children_cells(std::move(children_cells))
    , temp_cache(cache)
    , temp_referenced_cells(std::move(referenced_cells))
    {}

    std::unordered_set<Cell*> temp_children_cells;
    std::optional<CellInterface::Value> temp_cache;
    std::vector<Position> temp_referenced_cells;

    void RestoreFromBackup(std::unordered_set<Cell*>& children_cells,
                 std::optional<CellInterface::Value>& cache,
                 std::vector<Position>& referenced_cells) {
        children_cells = std::move(temp_children_cells);
        cache = temp_cache;
        referenced_cells = std::move(temp_referenced_cells);
    }
};

void Cell::Set(std::string text) {
    if (text.empty()) {
        Clear();
        return;
    }
    if (text.at(0) == '=' && text.size() > 1){

        CellCurrentStateBackup backup(children_cells_, cache_, referenced_cells_);

        auto temp_impl = std::make_unique<FormulaImpl>(this, text);

        for (Position pos : referenced_cells_){
            if (!CellExist(pos)) {
                sheet_.SetCell(pos, ""s);
            }
            children_cells_.insert(sheet_.GetConcreteCell(pos));
        }

        if (HasCycle()) {
            backup.RestoreFromBackup(children_cells_, cache_, referenced_cells_);
            throw CircularDependencyException("Circular!");
        } else {
            for (Cell* cell : backup.temp_children_cells) {
                auto it = cell->parent_cells_.find(this);
                if (it != cell->parent_cells_.end()){
                    cell->parent_cells_.erase(it);
                }
            }
            for (Cell* cell : children_cells_) {
                cell->parent_cells_.insert(this);
            }
            impl_ = std::move(temp_impl);
            CacheDestroy();
        }
    } else {
        Clear();
        impl_ = std::move(std::make_unique<TextImpl>(text));
    }
}

void Cell::Clear() {
    impl_ = std::move(std::make_unique<EmptyImpl>());
    referenced_cells_.clear();
    for (Cell* children_cell : children_cells_) {
        auto it = children_cell->parent_cells_.find(this);
        if (it != children_cell->parent_cells_.end()){
            children_cell->parent_cells_.erase(it);
        }
    }
    children_cells_.clear();
    CacheDestroy();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return referenced_cells_;
}

Cell::Impl::Impl(Cell* cell)
: cell_(cell)
{}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

Cell::Value Cell::EmptyImpl::GetValue() const {
    return ""s;
}

std::string Cell::EmptyImpl::GetText() const {
    return ""s;
}


Cell::TextImpl::TextImpl(const std::string& text)
: text_ (text)
{
    if (text.at(0) == '\'') {
        value_ = text.substr(1, text.size());
    } else {
        value_ = text;
    }
}

Cell::Value Cell::TextImpl::GetValue() const {
    return value_;
}

std::string Cell::TextImpl::GetText() const {
    return text_;
}


Cell::FormulaImpl::FormulaImpl(Cell* cell, const std::string& text)
: Cell::Impl(cell)
, formula_(ParseFormula(text.substr(1, text.size())))
{
    text_ = '=' + formula_->GetExpression();
    cell_->referenced_cells_ = formula_->GetReferencedCells();
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!cell_->cache_.has_value()) {
        std::variant<double, FormulaError> value = formula_->Evaluate(cell_->sheet_);
        if (std::holds_alternative<double>(value)) {
            cell_->cache_ = std::get<double>(value);
        } else {
            cell_->cache_ = std::get<FormulaError>(value);
        }
    }
    return cell_->cache_.value();
}


std::string Cell::FormulaImpl::GetText() const {
    return text_;
}

bool Cell::CellExist(Position pos) const {
    return sheet_.GetConcreteCell(pos) == nullptr ? false : true;
}

void Cell::CacheDestroy() const{
    cache_.reset();
    for (Cell* parent_cell : parent_cells_) {
        if (parent_cell->cache_.has_value()) {
            parent_cell->CacheDestroy();
        }
    }
}

bool Cell::HasCycle() const {
    std::stack<const Cell*> stack;
    std::unordered_set<const Cell*> visited;
    for (const Cell* child : children_cells_) {
        stack.push(child);
    }
    while (!stack.empty()) {
        const Cell* current = stack.top();
        stack.pop();
        if (current == this) {
            return true;
        }
        if (visited.find(current) != visited.end()) {
            continue;
        }
        visited.insert(current);
        for (const Cell* child : current->children_cells_) {
            stack.push(child);
        }
    }
    return false;
}

bool Cell::IsChildren() const {
    return !parent_cells_.empty();
}
