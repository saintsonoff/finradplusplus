CREATE TYPE status_enum AS ENUM ('ERROR', 'NOT_FRAUD', 'FRAUD');

CREATE TABLE IF NOT EXISTS rule_results (
    id BIGSERIAL PRIMARY KEY,
    profile_uuid UUID NOT NULL,
    profile_name VARCHAR NOT NULL,
    config_uuid UUID NOT NULL,
    config_name VARCHAR NOT NULL,
    status status_enum NOT NULL, 
    transaction_id VARCHAR NOT NULL,
    description TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
);
