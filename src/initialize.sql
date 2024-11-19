DROP DATABASE IF EXISTS db;
CREATE DATABASE db;
USE db;

CREATE TABLE session
(
    id VARCHAR(40) PRIMARY KEY
);

CREATE TABLE locations
(
    id CHAR PRIMARY KEY,
    x  INT,
    y  INT
);

CREATE PROCEDURE Reset_DB()
BEGIN
    DELETE FROM session WHERE 1;
    DELETE FROM locations WHERE 1;
END;

select * from locations;