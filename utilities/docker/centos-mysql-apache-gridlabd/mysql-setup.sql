CREATE USER 'gridlabd'@'localhost' IDENTIFIED BY 'gridlabd';
GRANT ALL PRIVILEGES ON *.* TO 'gridlabd'@'localhost' WITH GRANT OPTION;
FLUSH PRIVILEGES;
CREATE USER 'gridlabd_ro'@'%' IDENTIFIED BY 'gridlabd';
GRANT SELECT ON *.* TO 'gridlabd_ro'@'%';
FLUSH PRIVILEGES;