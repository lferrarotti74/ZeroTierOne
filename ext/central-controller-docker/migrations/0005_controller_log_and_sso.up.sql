CREATE TABLE IF NOT EXISTS controller_log (
	controller_id CHAR(10) NOT NULL REFERENCES controllers_ctl(id) ON DELETE CASCADE,
	check_time TIMESTAMP WITH TIME ZONE NOT NULL,
	load_factor REAL,
	PRIMARY KEY (controller_id, check_time)
);
CREATE INDEX IF NOT EXISTS ctl_check_time_ix ON public.controller_log USING btree (check_time);
CREATE INDEX IF NOT EXISTS ctl_id_ix ON public.controller_log USING btree (controller_id);

CREATE TABLE IF NOT EXISTS sso_expiry (
	nonce TEXT PRIMARY KEY,
	nonce_expiration TIMESTAMP WITH TIME ZONE NOT NULL,
	network_id CHARACTER(16) NOT NULL,
	member_id CHARACTER(10) NOT NULL,
	creation_time TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT (current_timestamp AT TIME ZONE 'UTC'),
	email TEXT,
	authentication_expiry_time TIMESTAMP WITH TIME ZONE,
	FOREIGN KEY (network_id, member_id) REFERENCES network_memberships_ctl(network_id, device_id) ON DELETE CASCADE
);
