#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>
#include <memory>

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

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;
    void Print(std::ostream& output, std::function<CellInterface::Value(const CellInterface* cell)> cell_handler) const;
    bool CellIsInitialised (Position pos) const;
};