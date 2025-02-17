#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <functional>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe){
    output << fe.ToString();
    return output;
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression)
    try : ast_(ParseFormulaAST(expression)) {
    } catch (...) {
        throw FormulaException("WRONG FORMULA!"s);
    }

    Value Evaluate(const SheetInterface& sheet) const override {

        const std::function<double(Position)> evaluating_func = [&sheet](const Position pos)->double {
            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }
            const CellInterface* cell = sheet.GetCell(pos);
            if (cell == nullptr) {
                return 0;
            }
            CellInterface::Value value = cell->GetValue();
            if (std::holds_alternative<std::string>(value)) {
                std::string string_value = std::get<std::string>(value);
                if (string_value.empty()){
                    return 0;
                }
                double double_value;
                size_t it;
                try {
                    double_value = std::stod(string_value, &it);
                } catch (...) {
                    throw FormulaError(FormulaError::Category::Value);
                }
                if (it <  string_value.length()){
                    throw FormulaError(FormulaError::Category::Value);
                }
                return double_value;
            }
            return std::get<double>(value);
        };

        try {
            return ast_.Execute(evaluating_func);
        } catch (const FormulaError& err) {
            return err;
        }
    }

    std::string GetExpression() const override {
        std::stringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const {
        std::set<Position> pos = {ast_.GetCells().begin(), ast_.GetCells().end()};
        return {pos.begin(), pos.end()};
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}