#pragma once

#include "../utils/shared.h"
#include <cstdint>
#include <memory>
#include <string>

namespace somedb {

enum CmpState { SQL_TRUE, SQL_FALSE };
struct PrimitiveValue;
struct PrimitiveType;

using PrimitiveTypeRef = std::shared_ptr<PrimitiveType>;

/*
 * Structs inheriting from PrimitiveType are structs representing the primitive types of the system (e.g booleans,
 * integers, varchars...)
 */
struct PrimitiveType {
    virtual ~PrimitiveType() = default;

    void assertTypesEqual(const PrimitiveValue &l, const PrimitiveValue &r) const;

    // Checks if left and right values are equal
    virtual CmpState equals(const PrimitiveValue &l, const PrimitiveValue &r) const = 0;

    // Checks if left and right values are not equal
    virtual CmpState notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const = 0;

    // Checks if left value is greater than right value
    virtual CmpState greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const = 0;

    // Checks if left value is lesser than right value
    virtual CmpState lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const = 0;

    // Basic mathematical functions on passed primitives
    virtual PrimitiveValue add(const PrimitiveValue &l, const PrimitiveValue &r) const;
    virtual PrimitiveValue subtract(const PrimitiveValue &l, const PrimitiveValue &r) const;
    virtual PrimitiveValue multiply(const PrimitiveValue &l, const PrimitiveValue &r) const;
    virtual PrimitiveValue divide(const PrimitiveValue &l, const PrimitiveValue &r) const;

    virtual std::string toString() const;
};

struct BooleanPrimitiveType : PrimitiveType {
    CmpState equals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
};

struct VarcharPrimitiveType : PrimitiveType {
    CmpState equals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
};

struct IntegerPrimitiveType : PrimitiveType {
    CmpState equals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;

    PrimitiveValue add(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    PrimitiveValue subtract(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    PrimitiveValue multiply(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    PrimitiveValue divide(const PrimitiveValue &l, const PrimitiveValue &r) const override;
};

struct DecimalPrimitiveType : PrimitiveType {
    CmpState equals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;

    PrimitiveValue add(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    PrimitiveValue subtract(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    PrimitiveValue multiply(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    PrimitiveValue divide(const PrimitiveValue &l, const PrimitiveValue &r) const override;
};

//--------------------------------------------------------------------------------------------------------------------------------
struct PrimitiveValue {
    PrimitiveTypeRef type;

    union {
        u8 boolean;
        i32 integer;
        double decimal;
        char *varchar;
    } value;

    u16 length; // for str type

    PrimitiveValue(PrimitiveTypeRef type, u8 b) : type(std::move(type)) { value = {.boolean = b}; };

    PrimitiveValue(PrimitiveTypeRef type, int32_t i) : type(std::move(type)) { value = {.integer = i}; };

    PrimitiveValue(PrimitiveTypeRef type, double d) : type(std::move(type)) { value = {.decimal = d}; };

    PrimitiveValue(PrimitiveTypeRef type, char *val, u16 vc) : type(std::move(type)) {
        value = {.varchar = val}, length = vc;
    };

    inline std::string toString() const { return type->toString(); };
};

} // namespace somedb