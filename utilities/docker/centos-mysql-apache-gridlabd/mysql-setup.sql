SET GLOBAL validate_password_policy=low;

CREATE USER gridlabd_a@localhost IDENTIFIED BY 'gridlabd';
GRANT ALL PRIVILEGES ON *.* TO gridlabd_a@localhost WITH GRANT OPTION;
FLUSH PRIVILEGES;

CREATE USER gridlabd@localhost IDENTIFIED BY 'gridlabd';
GRANT ALTER ON *.* TO gridlabd@localhost WITH GRANT OPTION;
GRANT CREATE ON *.* TO gridlabd@localhost WITH GRANT OPTION;
GRANT DROP ON *.* TO gridlabd@localhost WITH GRANT OPTION;
GRANT SELECT ON *.* TO gridlabd@localhost WITH GRANT OPTION;
GRANT UPDATE ON *.* TO gridlabd@localhost WITH GRANT OPTION;
GRANT INSERT ON *.* TO gridlabd@localhost WITH GRANT OPTION;
FLUSH PRIVILEGES;

CREATE USER gridlabd_ro@'%' IDENTIFIED BY 'gridlabd';
GRANT SELECT ON *.* TO gridlabd_ro@'%';
FLUSH PRIVILEGES;