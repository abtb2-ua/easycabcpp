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

CREATE TABLE customers (
    id CHAR PRIMARY KEY,
    x  INT,
    y  INT,
    destination CHAR DEFAULT NULL,

    CONSTRAINT fk_destination
        FOREIGN KEY (destination)
        REFERENCES locations(id)
);

CREATE TABLE taxis
(
    id INT PRIMARY KEY,
    x  INT DEFAULT 0,
    y  INT DEFAULT 0,

    # Indicates whether the taxi is connected to the server
    connected BOOLEAN DEFAULT TRUE,
    # Indicates whether the taxi is available to move
    can_move BOOLEAN DEFAULT TRUE,
    # Indicates whether there's any order to move assigned to the taxi
    should_move BOOLEAN DEFAULT FALSE,

    service CHAR DEFAULT NULL,
    carrying BOOLEAN DEFAULT FALSE,

    CONSTRAINT fk_service
        FOREIGN KEY (service)
        REFERENCES customers(id)
);


DELIMITER !!

CREATE PROCEDURE Reset_DB()
BEGIN
    DELETE FROM session WHERE 1;
    DELETE FROM locations WHERE 1;
END !!

DELIMITER ;


SELECT taxis.connected FROM taxis WHERE taxis.id = 1;