DROP TABLE `VERSION`;

CREATE TABLE SCHEMA_CHANGES(
   ID integer PRIMARY KEY NOT NULL,
   VERSION_NUMBER integer NOT NULL,
   SCRIPT_NAME varchar(255) NOT NULL,
   TIMESTAMP_UTC_APPLIED varchar(19) NOT NULL);

INSERT INTO SCHEMA_CHANGES(VERSION_NUMBER, SCRIPT_NAME, TIMESTAMP_UTC_APPLIED) VALUES(3, 'updatesqliteepisodes3.sql', datetime());
