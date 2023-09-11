#include "../../include/sql/primitive.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace somedb {

//--------------------------------------------------------------------------------------------------------------------------------
std::string PrimitiveType::toString() const { return typeid(this).name(); };

void PrimitiveType::assertTypesEqual(const PrimitiveValue &l, const PrimitiveValue &) const {
    assert(l.type->toString() == this->toString());
};

PrimitiveValue PrimitiveType::add(const PrimitiveValue &, const PrimitiveValue &) const {
    throw std::runtime_error("Add function doesn't exist for type " + std::string(typeid(*this).name()));
}

PrimitiveValue PrimitiveType::subtract(const PrimitiveValue &, const PrimitiveValue &) const {
    throw std::runtime_error("Subtract function doesn't exist for type " + std::string(typeid(*this).name()));
}

PrimitiveValue PrimitiveType::multiply(const PrimitiveValue &, const PrimitiveValue &) const {
    throw std::runtime_error("Multiply function doesn't exist for type " + std::string(typeid(*this).name()));
}

PrimitiveValue PrimitiveType::divide(const PrimitiveValue &, const PrimitiveValue &) const {
    throw std::runtime_error("Divide function doesn't exist for type " + std::string(typeid(*this).name()));
}

//--------------------------------------------------------------------------------------------------------------------------------
CmpState BooleanPrimitiveType::equals(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.boolean == r.value.boolean ? SQL_TRUE : SQL_FALSE;
};

CmpState BooleanPrimitiveType::notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.boolean != r.value.boolean ? SQL_TRUE : SQL_FALSE;
};

CmpState BooleanPrimitiveType::greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.boolean == true && r.value.boolean == false ? SQL_TRUE : SQL_FALSE;
};

CmpState BooleanPrimitiveType::lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.boolean == false && r.value.boolean == true ? SQL_TRUE : SQL_FALSE;
};

//--------------------------------------------------------------------------------------------------------------------------------
CmpState VarcharPrimitiveType::equals(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.varchar == r.value.varchar ? SQL_TRUE : SQL_FALSE;
};

CmpState VarcharPrimitiveType::notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.varchar != r.value.varchar ? SQL_TRUE : SQL_FALSE;
};

CmpState VarcharPrimitiveType::greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.varchar > r.value.varchar ? SQL_TRUE : SQL_FALSE;
};

CmpState VarcharPrimitiveType::lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.varchar < r.value.varchar ? SQL_TRUE : SQL_FALSE;
};

//--------------------------------------------------------------------------------------------------------------------------------
CmpState IntegerPrimitiveType::equals(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.integer == r.value.integer ? SQL_TRUE : SQL_FALSE;
};

CmpState IntegerPrimitiveType::notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.integer != r.value.integer ? SQL_TRUE : SQL_FALSE;
};

CmpState IntegerPrimitiveType::greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.integer > r.value.integer ? SQL_TRUE : SQL_FALSE;
};

CmpState IntegerPrimitiveType::lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.integer < r.value.integer ? SQL_TRUE : SQL_FALSE;
};

PrimitiveValue IntegerPrimitiveType::add(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);

    i32 added = l.value.integer + r.value.integer;
    auto type = std::make_shared<IntegerPrimitiveType>();
    return PrimitiveValue(type, added);
};

PrimitiveValue IntegerPrimitiveType::subtract(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);

    i32 added = l.value.integer - r.value.integer;
    auto type = std::make_shared<IntegerPrimitiveType>();
    return PrimitiveValue(type, added);
};

PrimitiveValue IntegerPrimitiveType::multiply(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);

    i32 added = l.value.integer * r.value.integer;
    auto type = std::make_shared<IntegerPrimitiveType>();
    return PrimitiveValue(type, added);
};

PrimitiveValue IntegerPrimitiveType::divide(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    if (r.value.integer == 0) {
        throw std::runtime_error("An integer value (" + std::to_string(l.value.integer) +
                                 ") may not be divided by zero");
    }

    i32 added = l.value.integer / r.value.integer;
    auto type = std::make_shared<IntegerPrimitiveType>();
    return PrimitiveValue(type, added);
};

//--------------------------------------------------------------------------------------------------------------------------------
CmpState DecimalPrimitiveType::equals(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.decimal == r.value.decimal ? SQL_TRUE : SQL_FALSE;
};

CmpState DecimalPrimitiveType::notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.decimal != r.value.decimal ? SQL_TRUE : SQL_FALSE;
};

CmpState DecimalPrimitiveType::greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.decimal > r.value.decimal ? SQL_TRUE : SQL_FALSE;
};

CmpState DecimalPrimitiveType::lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    return l.value.decimal < r.value.decimal ? SQL_TRUE : SQL_FALSE;
};

PrimitiveValue DecimalPrimitiveType::add(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);

    double added = l.value.decimal + r.value.decimal;
    auto type = std::make_shared<DecimalPrimitiveType>();
    return PrimitiveValue(type, added);
};

PrimitiveValue DecimalPrimitiveType::subtract(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);

    double added = l.value.decimal - r.value.decimal;
    auto type = std::make_shared<DecimalPrimitiveType>();
    return PrimitiveValue(type, added);
};

PrimitiveValue DecimalPrimitiveType::multiply(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);

    double added = l.value.decimal * r.value.decimal;
    auto type = std::make_shared<DecimalPrimitiveType>();
    return PrimitiveValue(type, added);
};

PrimitiveValue DecimalPrimitiveType::divide(const PrimitiveValue &l, const PrimitiveValue &r) const {
    PrimitiveType::assertTypesEqual(l, r);
    if (r.value.decimal == 0) {
        throw std::runtime_error("A decimal value (" + std::to_string(l.value.decimal) +
                                 ") may not be divided by zero");
    }

    double added = l.value.decimal / r.value.decimal;
    auto type = std::make_shared<DecimalPrimitiveType>();
    return PrimitiveValue(type, added);
};

//--------------------------------------------------------------------------------------------------------------------------------
} // namespace somedb