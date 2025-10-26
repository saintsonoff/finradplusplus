# Benchmark Scripts

This directory contains scripts for testing the fraud detection system.

## send_transaction.py

A comprehensive load testing script for the fraud detection API Gateway.

### Features

- **Single Transaction Mode**: Send one transaction for basic testing
- **Load Testing Mode**: Send multiple transactions with configurable concurrency
- **Performance Metrics**: TPS (transactions per second), response times, success rates
- **Random Data Generation**: Creates varied transaction data for realistic testing
- **Concurrent Execution**: Uses threading for high-throughput testing

### Prerequisites

Create and activate a virtual environment, then install dependencies:
```bash
# From project root
python -m venv venv
source venv/bin/activate
pip install -r benchmark/requirements.txt
```

### Usage

Make sure the Docker services are running:
```bash
docker-compose up -d
```

#### Single Transaction Test
```bash
python3 send_transaction.py --single
```

#### Load Testing
```bash
# Basic load test (100 transactions, 10 concurrent)
python3 send_transaction.py

# Custom load test (1000 transactions, 50 concurrent)
python3 send_transaction.py -n 1000 -c 50

# High load test (10000 transactions, 100 concurrent)
python3 send_transaction.py -n 10000 -c 100
```

### Command Line Options

- `-n, --num-transactions`: Number of transactions to send (default: 100)
- `-c, --concurrency`: Number of concurrent threads (default: 10)
- `-u, --url`: API endpoint URL (default: http://localhost:8080/transaction)
- `--single`: Send a single transaction instead of load test

### Sample Output

```
🚀 Starting load test: 100 transactions with 10 concurrent threads
Target URL: http://localhost:8080/transaction
================================================================================
Progress: 10/100 transactions | Success: 10/10
Progress: 20/100 transactions | Success: 20/20
...
Progress: 100/100 transactions | Success: 100/100

================================================================================
📊 LOAD TEST RESULTS
================================================================================
Total Transactions: 100
Successful: 100 (100.0%)
Failed: 0
Total Time: 12.34 seconds
Transactions/Second: 8.10 TPS

⏱️  Response Time Statistics:
  Average: 1.234s
  Median:  1.156s
  Min:     0.987s
  Max:     2.345s
```

### Transaction Data Fields

Each generated transaction includes:
- `transaction_id`: Unique sequential identifier (T000001, T000002, etc.)
- `timestamp`: Current ISO timestamp
- `sender_account`: Random account number
- `receiver_account`: Random account number
- `amount`: Random amount (1.0 - 10000.0)
- `currency`: Random currency (USD, EUR, GBP, JPY, CAD)
- `date`: Random date within last 30 days
- `type`: Random transaction type (deposit, withdrawal, transfer, payment)
- `transaction_type`: Same as type
- `Merchant_category`: Random category (utilities, groceries, etc.)
- `location`: Random city
- `device_used`: Random device (mobile, desktop, tablet)
- `payment_channel`: Random payment method
- `ip_address`: Random IP address
- `device_hash`: Random device hash

### Expected Response

On success, the API Gateway should return a 200 status code. The transaction will be processed through the fraud detection pipeline:
1. API Gateway receives the transaction
2. Transaction is sent to Kafka topic "Request"
3. Rules service processes the transaction using ML models and rule engine
4. Results are sent back via Kafka topic "Response"
5. Results can be monitored using the test clients in `test_clients/` directory