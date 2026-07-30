ALTER TABLE sso_expiry RENAME COLUMN member_id TO device_id;
CREATE INDEX IF NOT EXISTS sso_expiry_network_member_ix ON public.sso_expiry (network_id, device_id);

CREATE TABLE IF NOT EXISTS oidc_config (
	client_id TEXT NOT NULL,
	linked_id TEXT NOT NULL,
	issuer TEXT NOT NULL,
	authorization_endpoint TEXT NOT NULL,
	sso_impl_version BIGINT NOT NULL DEFAULT 1,
	provider TEXT NOT NULL DEFAULT 'default',
	PRIMARY KEY (client_id, linked_id)
);