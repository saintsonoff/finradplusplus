#!/usr/bin/env python3
"""
Load testing script for the fraud detection system.
Sends multiple transactions via HTTP POST to the API Gateway.
"""

import requests
import json
import sys
import time
import random
import threading
import argparse
import statistics
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timedelta

def generate_transaction(transaction_id):
    """Generate a random transaction with varying parameters."""
    # Base transaction data
    transaction_data = {
        "transaction_id": f"T{transaction_id:06d}",
        "timestamp": datetime.now().isoformat(),
        "sender_account": f"ACC{random.randint(100000, 999999)}",
        "receiver_account": f"ACC{random.randint(100000, 999999)}",
        "amount": round(random.uniform(1.0, 10000.0), 2),
        "currency": random.choice(["USD", "EUR", "GBP", "JPY", "CAD"]),
        "date": (datetime.now() - timedelta(days=random.randint(0, 30))).strftime("%Y-%m-%d"),
        "type": random.choice(["deposit", "withdrawal", "transfer", "payment"]),
        "transaction_type": random.choice(["deposit", "withdrawal", "transfer", "payment"]),
        "Merchant_category": random.choice(["utilities", "groceries", "entertainment", "transport", "healthcare", "retail"]),
        "location": random.choice(["Tokyo", "New York", "London", "Paris", "Berlin", "Sydney", "Moscow", "Beijing"]),
        "device_used": random.choice(["mobile", "atm", "pos", "web"]),
        "payment_channel": random.choice(["UPI", "card", "ACH", "wire_transfer"]),
        "ip_address": f"{random.randint(1,255)}.{random.randint(0,255)}.{random.randint(0,255)}.{random.randint(0,255)}",
        "device_hash": f"D{random.randint(10000000, 99999999)}"
    }
    return transaction_data

def send_single_transaction(transaction_data, url="http://localhost:8080/transaction", timeout=10):
    """Send a single transaction and return response data."""
    start_time = time.time()

    try:
        headers = {
            'Content-Type': 'application/json'
        }

        response = requests.post(url, json=transaction_data, headers=headers, timeout=timeout)
        response_time = time.time() - start_time

        return {
            'success': response.status_code == 200,
            'status_code': response.status_code,
            'response_time': response_time,
            'transaction_id': transaction_data['transaction_id'],
            'error': None if response.status_code == 200 else response.text
        }

    except requests.exceptions.RequestException as e:
        response_time = time.time() - start_time
        return {
            'success': False,
            'status_code': None,
            'response_time': response_time,
            'transaction_id': transaction_data['transaction_id'],
            'error': str(e)
        }
    except Exception as e:
        response_time = time.time() - start_time
        return {
            'success': False,
            'status_code': None,
            'response_time': response_time,
            'transaction_id': transaction_data['transaction_id'],
            'error': str(e)
        }

def run_load_test(num_transactions=100, concurrency=10, url="http://localhost:8080/transaction"):
    """Run load test with specified parameters."""
    print(f"🚀 Starting load test: {num_transactions} transactions with {concurrency} concurrent threads")
    print(f"Target URL: {url}")
    print("=" * 80)

    start_time = time.time()
    results = []

    with ThreadPoolExecutor(max_workers=concurrency) as executor:
        # Submit all tasks
        future_to_transaction = {
            executor.submit(send_single_transaction, generate_transaction(i), url): i
            for i in range(num_transactions)
        }

        # Process results as they complete
        completed = 0
        for future in as_completed(future_to_transaction):
            result = future.result()
            results.append(result)
            completed += 1

            # Progress update every 10 transactions
            if completed % 10 == 0 or completed == num_transactions:
                success_count = sum(1 for r in results if r['success'])
                print(f"Progress: {completed}/{num_transactions} transactions | Success: {success_count}/{completed}")

    total_time = time.time() - start_time

    # Calculate statistics
    successful_results = [r for r in results if r['success']]
    failed_results = [r for r in results if not r['success']]

    response_times = [r['response_time'] for r in results]

    stats = {
        'total_transactions': num_transactions,
        'successful_transactions': len(successful_results),
        'failed_transactions': len(failed_results),
        'total_time': total_time,
        'transactions_per_second': num_transactions / total_time if total_time > 0 else 0,
        'avg_response_time': statistics.mean(response_times) if response_times else 0,
        'min_response_time': min(response_times) if response_times else 0,
        'max_response_time': max(response_times) if response_times else 0,
        'median_response_time': statistics.median(response_times) if response_times else 0,
        'success_rate': len(successful_results) / num_transactions * 100 if num_transactions > 0 else 0
    }

    return stats, results

def print_results(stats, results):
    """Print detailed test results."""
    print("\n" + "=" * 80)
    print("📊 LOAD TEST RESULTS")
    print("=" * 80)

    print(f"Total Transactions: {stats['total_transactions']}")
    print(f"Successful: {stats['successful_transactions']} ({stats['success_rate']:.1f}%)")
    print(f"Failed: {stats['failed_transactions']}")
    print(f"Total Time: {stats['total_time']:.2f} seconds")
    print(f"Transactions/Second: {stats['transactions_per_second']:.2f} TPS")

    print(f"\n⏱️  Response Time Statistics:")
    print(f"  Average: {stats['avg_response_time']:.3f}s")
    print(f"  Median:  {stats['median_response_time']:.3f}s")
    print(f"  Min:     {stats['min_response_time']:.3f}s")
    print(f"  Max:     {stats['max_response_time']:.3f}s")

    # Show some failed transactions if any
    if stats['failed_transactions'] > 0:
        failed_results = [r for r in results if not r['success']]
        print(f"\n❌ Sample Failed Transactions:")
        for i, result in enumerate(failed_results[:5]):  # Show first 5 failures
            print(f"  {result['transaction_id']}: {result['error']}")

    print("\n" + "=" * 80)

def main():
    parser = argparse.ArgumentParser(description='Load test the fraud detection API')
    parser.add_argument('-n', '--num-transactions', type=int, default=100,
                       help='Number of transactions to send (default: 100)')
    parser.add_argument('-c', '--concurrency', type=int, default=10,
                       help='Number of concurrent threads (default: 10)')
    parser.add_argument('-u', '--url', type=str, default='http://localhost:8080/transaction',
                       help='API endpoint URL (default: http://localhost:8080/transaction)')
    parser.add_argument('--single', action='store_true',
                       help='Send a single transaction instead of load test')

    args = parser.parse_args()

    if args.single:
        # Single transaction mode
        transaction_data = generate_transaction(1)
        print("📤 Sending single transaction...")
        result = send_single_transaction(transaction_data, args.url)

        if result['success']:
            print("✅ Transaction sent successfully!")
            print(f"Response time: {result['response_time']:.3f}s")
        else:
            print(f"❌ Transaction failed: {result['error']}")
    else:
        # Load test mode
        stats, results = run_load_test(args.num_transactions, args.concurrency, args.url)
        print_results(stats, results)

if __name__ == "__main__":
    main()