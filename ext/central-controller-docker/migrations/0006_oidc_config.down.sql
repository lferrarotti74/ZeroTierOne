DROP TABLE IF EXISTS oidc_config;
ALTER TABLE sso_expiry RENAME COLUMN device_id TO member_id;
DROP INDEX IF EXISTS sso_expiry_network_member_ix;
