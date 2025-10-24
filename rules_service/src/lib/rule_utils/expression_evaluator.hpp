#pragma once

#include <rules/rule_config.pb.h>
#include <transaction/transaction.pb.h>
#include <variant>
#include <string>
#include <stdexcept>

namespace rule_utils {

using ExpressionValue = std::variant<std::string, float, int32_t, bool>;

class FieldExtractor {
public:
    static ExpressionValue GetFieldValue(
        const transaction::Transaction& transaction, 
        rules::FieldReference::FieldType field) {
        switch (field) {
            case rules::FieldReference::TRANSACTION_ID:
                return transaction.transaction_id();
            case rules::FieldReference::SENDER_ACCOUNT:
                return transaction.sender_account();
            case rules::FieldReference::RECEIVER_ACCOUNT:
                return transaction.receiver_account();
            case rules::FieldReference::AMOUNT:
                return transaction.amount();
            case rules::FieldReference::TIMESTAMP:
                return transaction.timestamp();
            case rules::FieldReference::TRANSACTION_TYPE:
                return transaction.transaction_type();
            case rules::FieldReference::MERCHANT_CATEGORY:
                return transaction.merchant_category();
            case rules::FieldReference::LOCATION:
                return transaction.location();
            case rules::FieldReference::DEVICE_USED:
                return transaction.device_used();
            case rules::FieldReference::PAYMENT_CHANNEL:
                return transaction.payment_channel();
            case rules::FieldReference::IP_ADDRESS:
                return transaction.ip_address();
            case rules::FieldReference::DEVICE_HASH:
                return transaction.device_hash();
            default:
                throw std::runtime_error("Unknown field type");
        }
    }
};

class LiteralExtractor {
public:
    static ExpressionValue GetLiteralValue(const rules::LiteralValue& literal) {
        switch (literal.value_case()) {
            case rules::LiteralValue::kStringValue:
                return literal.string_value();
            case rules::LiteralValue::kFloatValue:
                return literal.float_value();
            case rules::LiteralValue::kIntValue:
                return literal.int_value();
            case rules::LiteralValue::kBoolValue:
                return literal.bool_value();
            default:
                throw std::runtime_error("Unknown literal type");
        }
    }
};

class ComparisonEvaluator {
public:
    static bool Evaluate(
        const ExpressionValue& left,
        const ExpressionValue& right,
        rules::ComparisonOperation::Operator op) {
        return std::visit([op](auto&& l, auto&& r) -> bool {
            using L = std::decay_t<decltype(l)>;
            using R = std::decay_t<decltype(r)>;
            if constexpr ((std::is_same_v<L, float> || std::is_same_v<L, int32_t>) &&
                         (std::is_same_v<R, float> || std::is_same_v<R, int32_t>)) {
                return CompareNumeric(
                    std::is_same_v<L, float> ? l : static_cast<float>(l),
                    std::is_same_v<R, float> ? r : static_cast<float>(r),
                    op
                );
            }
            else if constexpr (std::is_same_v<L, std::string> && std::is_same_v<R, std::string>) {
                return CompareString(l, r, op);
            }
            else if constexpr (std::is_same_v<L, bool> && std::is_same_v<R, bool>) {
                return CompareBoolean(l, r, op);
            }
            else {
                throw std::runtime_error("Type mismatch in comparison");
            }
        }, left, right);
    }

private:
    static bool CompareNumeric(float left, float right, rules::ComparisonOperation::Operator op) {
        switch (op) {
            case rules::ComparisonOperation::EQUAL:
                return left == right;
            case rules::ComparisonOperation::NOT_EQUAL:
                return left != right;
            case rules::ComparisonOperation::GREATER_THAN:
                return left > right;
            case rules::ComparisonOperation::GREATER_THAN_OR_EQUAL:
                return left >= right;
            case rules::ComparisonOperation::LESS_THAN:
                return left < right;
            case rules::ComparisonOperation::LESS_THAN_OR_EQUAL:
                return left <= right;
            default:
                throw std::runtime_error("Invalid operator for numeric comparison");
        }
    }

    static bool CompareString(
        const std::string& left, 
        const std::string& right,
        rules::ComparisonOperation::Operator op) {
        switch (op) {
            case rules::ComparisonOperation::EQUAL:
                return left == right;
            case rules::ComparisonOperation::NOT_EQUAL:
                return left != right;
            case rules::ComparisonOperation::LIKE:
                return left.find(right) != std::string::npos;
            default:
                throw std::runtime_error("Invalid operator for string comparison");
        }
    }

    static bool CompareBoolean(bool left, bool right, rules::ComparisonOperation::Operator op) {
        switch (op) {
            case rules::ComparisonOperation::EQUAL:
                return left == right;
            case rules::ComparisonOperation::NOT_EQUAL:
                return left != right;
            default:
                throw std::runtime_error("Invalid operator for boolean comparison");
        }
    }
};

inline float ExtractNumericValue(const ExpressionValue& value) {
    if (std::holds_alternative<float>(value)) {
        return std::get<float>(value);
    } else if (std::holds_alternative<int32_t>(value)) {
        return static_cast<float>(std::get<int32_t>(value));
    }
    throw std::runtime_error("Value is not numeric");
}

}
