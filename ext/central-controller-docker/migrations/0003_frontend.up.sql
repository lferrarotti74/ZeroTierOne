ALTER TABLE networks_ctl
    ADD COLUMN frontend TEXT NOT NULL DEFAULT 'cv2';

ALTER TABLE network_memberships_ctl
    ADD COLUMN frontend TEXT NOT NULL DEFAULT 'cv2';