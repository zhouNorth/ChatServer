-- Apply to a NEW database selected by the caller. Does not reset existing data.
CREATE TABLE IF NOT EXISTS user (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    password VARCHAR(255) NOT NULL,
    state ENUM('offline','online') NOT NULL DEFAULT 'offline'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS friend (
    userid INT NOT NULL,
    friendid INT NOT NULL,
    PRIMARY KEY(userid,friendid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS all_group (
    id INT PRIMARY KEY AUTO_INCREMENT,
    groupname VARCHAR(100) NOT NULL,
    groupdesc VARCHAR(255) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS groupuser (
    userid INT NOT NULL,
    groupid INT NOT NULL,
    grouprole VARCHAR(20) NOT NULL,
    PRIMARY KEY(userid,groupid),
    INDEX(groupid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS offlinemsg (
    userid INT NOT NULL,
    message MEDIUMTEXT NOT NULL,
    INDEX(userid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
