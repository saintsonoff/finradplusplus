#include <iostream>

#include <transaction.pb.h>

int main() {
    transaction::Transaction t;
    std::cout << "fst main\n";
    std::cout << "fst main\n" << t.transaction_id();
    return 0;
}