DROP INDEX `CATEGORIES__UUID` ON CATEGORIES;
CREATE INDEX `CATEGORIES__UUID` ON CATEGORIES (UUID(190));

DROP INDEX `CATEGORIES__MIME` ON CATEGORIES;                                                
CREATE INDEX `CATEGORIES__MIME` ON CATEGORIES (MIME(190)); 

DROP TABLE `VERSION`;

CREATE TABLE `SCHEMA_CHANGES`(
   `ID` int NOT NULL AUTO_INCREMENT,
   `VERSION_NUMBER` int NOT NULL,
   `SCRIPT_NAME` varchar(255) NOT NULL,
   `TIMESTAMP_UTC_APPLIED` varchar(19) NOT NULL,
   PRIMARY KEY (ID)
);

INSERT INTO `SCHEMA_CHANGES` (`VERSION_NUMBER`, `SCRIPT_NAME`, `TIMESTAMP_UTC_APPLIED`)
VALUES ('1', 'updatecategory1.sql', UTC_TIMESTAMP());
