#pragma once

#include "../disk/heapfile.h"
#include "../utils/shared.h"
#include <cstdint>
#include <memory>
#include <string>

namespace somedb {

enum CmpState { SQL_TRUE, SQL_FALSE };
struct PrimitiveValue;
struct PrimitiveType;

//--------------------------------------------------------------------------------------------------------------------------------
using PrimitiveTypeRef = std::shared_ptr<PrimitiveType>;

/*
 * Structs inheriting from PrimitiveType are structs representing the primitive types of the system (e.g booleans,
 * integers, varchars...)
 */
struct PrimitiveType {
    virtual ~PrimitiveType() = default;

    // Asserts that the two types being evaluated against each other are of the same type
    void assertTypesEqual(const PrimitiveValue &l, const PrimitiveValue &r) const;

    // Returns space occupied by a value of a concrete data type
    virtual u32 getSize() const = 0;

    // Deserializes a value from a raw byte buffer
    virtual PrimitiveValue deserialize(u8 *buf) const = 0;

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
    u32 getSize() const override;
    PrimitiveValue deserialize(u8 *buf) const override;
    CmpState equals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
};

struct VarcharPrimitiveType : PrimitiveType {
    u32 getSize() const override;
    PrimitiveValue deserialize(u8 *buf) const override;
    CmpState equals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState notEquals(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState greaterThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
    CmpState lessThan(const PrimitiveValue &l, const PrimitiveValue &r) const override;
};

struct IntegerPrimitiveType : PrimitiveType {
    u32 getSize() const override;
    PrimitiveValue deserialize(u8 *buf) const override;
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
    u32 getSize() const override;
    PrimitiveValue deserialize(u8 *buf) const override;
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

    ColumnValue value;

    u16 length; // for str type

    PrimitiveValue(){};

    PrimitiveValue(PrimitiveTypeRef type, bool b) : type(std::move(type)) { value = {.boolean = b}; };

    PrimitiveValue(PrimitiveTypeRef type, int32_t i) : type(std::move(type)) { value = {.integer = i}; };

    PrimitiveValue(PrimitiveTypeRef type, double d) : type(std::move(type)) { value = {.decimal = d}; };

    PrimitiveValue(PrimitiveTypeRef type, char *val, u16 vc) : type(std::move(type)) {
        value = {.string = val}, length = vc;
    };

    inline std::string toString() const { return type->toString(); };
};

//--------------------------------------------------------------------------------------------------------------------------------

} // namespace somedb