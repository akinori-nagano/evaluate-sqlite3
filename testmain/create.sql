
CREATE TABLE t_test_001 (
	id int  PRIMARY KEY  NOT NULL,
	kind integer,
	contents_id TEXT NOT NULL,
	contents_code varchar(53) NOT NULL,
	terminal_value blob,
	hashed_id char(64) NOT NULL  UNIQUE,
	updated_date datetime,
	updated_at timestamp
);

CREATE TABLE t_test_002 (
	contents_code varchar(53) PRIMARY KEY NOT NULL,
	hashed_id char(64) NOT NULL  UNIQUE,
	id int  NOT NULL,
	secret integer NOT NULL default 0,
	read_datetime timestamp,
	delete_status integer NOT NULL default 0,
	updated_at timestamp
);
CREATE INDEX i_t_test_002 on t_test_002(id);
CREATE UNIQUE INDEX u_t_test_002 on t_test_002(secret, read_datetime, delete_status);

CREATE TABLE tmp_t_test_002 (
	contents_code varchar(53) PRIMARY KEY NOT NULL,
	hashed_id char(64) NOT NULL  UNIQUE,
	id int  NOT NULL,
	secret integer NOT NULL default 0,
	read_datetime timestamp,
	delete_status integer NOT NULL default 0,
	updated_at timestamp
);
CREATE INDEX i_tmp_t_test_002 on tmp_t_test_002(id);
CREATE UNIQUE INDEX u_tmp_t_test_002 on tmp_t_test_002(secret, read_datetime, delete_status);

CREATE VIEW v_test_002 AS
SELECT * from tmp_t_test_002 tt2
UNION ALL
SELECT * from t_test_002 t
where not exists (
    select * from tmp_t_test_002 t2
    where
    t2.contents_code = t.contents_code
);
