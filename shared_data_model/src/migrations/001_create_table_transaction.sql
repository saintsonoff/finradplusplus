CREATE TYPE transaction_type AS ENUM ('WITHDRAWAL', 'DEPOSIT', 'TRANSFER', 'PAYMENT');
CREATE TYPE device_used AS ENUM ('MOBILE', 'ATM', 'POS', 'WEB');
CREATE TYPE payment_channel AS ENUM ('UPI', 'CARD', 'ACH', 'WIRE_TRANSFER');

CREATE TABLE IF NOT EXISTS transactions (
    id SERIAL PRIMARY KEY,
    transaction_id VARCHAR(255) UNIQUE NOT NULL,
    sender_account VARCHAR(255) NOT NULL,
    times_tamp TIMESTAMP NOT NULL,
    receiver_account VARCHAR(255) NOT NULL,
    amount NUMERIC NOT NULL,
    transaction_type transaction_type NOT NULL,
    merchant_category VARCHAR(255) NOT NULL,
    location VARCHAR(255) NOT NULL,
    device_used device_used NOT NULL,
    payment_channel payment_channel NOT NULL,
    ip_address VARCHAR(255) NOT NULL,
    device_hash VARCHAR(255) NOT NULL
);
