#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()){
        throw InvalidPositionException("Invalid Position!"s);
    }

    if (static_cast<int>(sheet_.size()) <= pos.row) {
        sheet_.resize(pos.row + 1);
    }

    if (static_cast<int>(sheet_[pos.row].size()) <= pos.col) {
        sheet_[pos.row].resize(pos.col + 1);
    }

    if (sheet_[pos.row][pos.col] == nullptr) {
        sheet_[pos.row][pos.col] = std::move(std::make_unique<Cell>(*this));
    }
    sheet_[pos.row][pos.col]->Set(text);

}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()){
        throw InvalidPositionException("Invalid Position!"s);
    }
    if (CellIsInitialised(pos)) {
        return sheet_[pos.row][pos.col].get();
    }
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()){
        throw InvalidPositionException("Invalid Position!"s);
    }
    if (CellIsInitialised(pos)) {
        return sheet_[pos.row][pos.col].get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()){
        throw InvalidPositionException("Invalid Position!"s);
    }
    if (CellIsInitialised(pos)) {
        if(GetConcreteCell(pos) != nullptr && GetConcreteCell(pos)->IsChildren()) {
            sheet_[pos.row][pos.col]->Clear();
        } else {
            sheet_[pos.row][pos.col] = nullptr;
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size size;

    for (int row = 0; row < static_cast<int>(sheet_.size()); ++row) {
        for (int col = 0; col < static_cast<int>(sheet_[row].size()); ++col) {
            if (sheet_[row][col] != nullptr && !sheet_[row][col]->GetText().empty()) {
                size.rows = std::max(size.rows, row + 1);
                size.cols = std::max(size.cols, col + 1);
            }
        }
    }
    return size;
}

std::ostream& operator<<(std::ostream& ost, const CellInterface::Value& value) {
    if (std::holds_alternative<std::string>(value)){
        ost << std::get<std::string>(value);
    } else if (std::holds_alternative<double>(value)) {
        ost << std::get<double>(value);
    } else {
        ost << std::get<FormulaError>(value);
    }
    return ost;
}

void Sheet::Print(std::ostream& output, std::function<CellInterface::Value(const CellInterface* cell)> cell_handler) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < std::min(size.rows, static_cast<int>(sheet_.size())); ++row){
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (col < static_cast<int>(sheet_.at(row).size()) && sheet_.at(row).at(col) != nullptr){
                output << cell_handler(this->GetCell({row, col}));
            }
        }
        output << '\n';
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    Print(output, [] (const CellInterface* cell){ return cell->GetValue(); });
}

void Sheet::PrintTexts(std::ostream& output) const {
    Print(output, [](const CellInterface* cell){ return cell->GetText(); });
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()){
        throw InvalidPositionException("Invalid Position!"s);
    }
    if (CellIsInitialised(pos)) {
        return sheet_[pos.row][pos.col] == nullptr ? nullptr : sheet_[pos.row][pos.col].get();
    }
    return nullptr;
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if (!pos.IsValid()){
        throw InvalidPositionException("Invalid Position!"s);
    }
    if (CellIsInitialised(pos)) {
        return sheet_[pos.row][pos.col] == nullptr ? nullptr : sheet_[pos.row][pos.col].get();
    }
    return nullptr;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

bool Sheet::CellIsInitialised(Position pos) const {
    return pos.row < static_cast<int>(sheet_.size()) && pos.col < static_cast<int>(sheet_[pos.row].size());
}