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



DELIMITER !!

CREATE PROCEDURE Reset_DB()
BEGIN
    DELETE FROM session WHERE 1;
    DELETE FROM locations WHERE 1;
END !!

DELIMITER ;





select * from locations;
call Reset_DB();