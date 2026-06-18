CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    mfa_secret VARCHAR(32) DEFAULT NULL,
    mfa_enabled BOOLEAN DEFAULT FALSE,
    email_verified BOOLEAN DEFAULT FALSE,
    verification_token VARCHAR(255),
    failed_login_attempts INT DEFAULT 0,
    lockout_until TIMESTAMP WITH TIME ZONE NULL,
    password_reset_token VARCHAR(255),
    password_reset_expires TIMESTAMP WITH TIME ZONE,
    password_change_token VARCHAR(255),
    pending_password_hash VARCHAR(255),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Index for lightning-fast user lookups (as per README)
CREATE INDEX idx_users_email ON users(email);

CREATE TABLE transactions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    sender_id UUID REFERENCES users(id),
    receiver_id UUID REFERENCES users(id) NOT NULL,
    amount DECIMAL(15, 2) NOT NULL CHECK (amount > 0),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Composite indexes on (sender_id, created_at) and (receiver_id, created_at) to instantly fetch and paginate transaction histories without full table scans
CREATE INDEX idx_transactions_sender ON transactions(sender_id, created_at DESC);
CREATE INDEX idx_transactions_receiver ON transactions(receiver_id, created_at DESC);
