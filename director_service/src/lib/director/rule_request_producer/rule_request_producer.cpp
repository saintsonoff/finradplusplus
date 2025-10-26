#include "rule_request_producer.hpp"


// stdcpp
#include <utility>
#include <string_view>

// userver
#include <userver/kafka/producer.hpp>
#include <userver/kafka/exceptions.hpp>

// self
#include <transaction/transaction.pb.h>
#include <rules/profile.pb.h>
#include <rules/rule_request.pb.h>

namespace director_service {


std::pair<size_t, RuleRequestProducer::SendStatus> RuleRequestProducer::operator()(
  const std::string& topic,
  const userver::kafka::Producer& producer,
  profile::Profile& profile,
  transaction::Transaction& transaction
) const {
    size_t sended_count = 0;

    auto send_request = [
        &, key = transaction.transaction_id() + profile.uuid(),
        total_rule_count = std::size(profile.rules())](
      transaction::Transaction& transaction,
      rules::RuleConfig& config,
      size_t number
    ) {
        try {
            auto request = rules::RuleRequest{};
            request.set_profile_uuid(profile.name());
            request.set_profile_name(profile.uuid());
            request.set_allocated_rule(&config);
            request.set_allocated_transaction(&transaction);
            request.set_number(number);
            request.set_total_rule_count(total_rule_count);

            auto message = request.SerializeAsString();

            if (message.size() == 0) {
                return SendStatus::kErrorSerializationNonRetryable;
            }

            producer.Send(topic, key, message);
            return SendStatus::kSuccess;
        } catch (const userver::kafka::SendException& ex) {
            return ex.IsRetryable() ? SendStatus::kErrorRetryable : SendStatus::kErrorNonRetryable;
        }
    };

    for (auto config : profile.rules()) {
        auto send_status = send_request(transaction, config, sended_count);

        ++sended_count;
        if (send_status != SendStatus::kSuccess) {
            return {sended_count, send_status};
        }
    }

    return {sended_count, SendStatus::kSuccess};
}


} // namespace director_service