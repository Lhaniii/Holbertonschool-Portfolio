CREATE DATABASE devlink;
USE devlink;

CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(100) NOT NULL,
    email VARCHAR(100) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    role ENUM('freelance', 'client') NOT NULL
);

CREATE TABLE freelances (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    age INT,
    skills TEXT,
    rating FLOAT DEFAULT 0,
    email_contact VARCHAR(100),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE clients (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE ratings (
    id INT AUTO_INCREMENT PRIMARY KEY,
    freelance_id INT,
    client_id INT,
    rating FLOAT NOT NULL,
    comment TEXT,
    FOREIGN KEY (freelance_id) REFERENCES freelances(id) ON DELETE CASCADE,
    FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE
);

CREATE TABLE skills (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL UNIQUE
);

CREATE TABLE freelance_skills (
    freelance_id INT,
    skill_id INT,
    PRIMARY KEY (freelance_id, skill_id),
    FOREIGN KEY (freelance_id) REFERENCES freelances(id) ON DELETE CASCADE,
    FOREIGN KEY (skill_id) REFERENCES skills(id) ON DELETE CASCADE
);
