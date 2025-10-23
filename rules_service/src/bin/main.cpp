#include <iostream>

#include "transaction.pb.h"
#include "rule_interface/IRule.hpp"
#include "ml_rule/ml_rule.hpp"
#include "rule_config.pb.h"
#include "rule_factory/rule_factory.hpp"

int main() {
    std::cout << "OK\n" ;
    transaction::Transaction trans;
    trans.set_amount(1000);
    rules::RuleConfig lol;
    ::rules::MLRule* kek = new rules::MLRule;
    lol.set_allocated_ml_rule(kek);
    MlRuleAnalyzer lolkek(lol);
    std::cout << trans.amount();
}
