DROP DATABASE IF EXISTS library_db;
CREATE DATABASE library_db;
USE library_db;

-- Books table
CREATE TABLE books (
  id INT AUTO_INCREMENT PRIMARY KEY,
  title VARCHAR(255) NOT NULL,
  author VARCHAR(255),
  isbn VARCHAR(50) UNIQUE,
  total_copies INT NOT NULL DEFAULT 1,
  available_copies INT NOT NULL DEFAULT 1
);

-- Members table
CREATE TABLE members (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  email VARCHAR(255) UNIQUE,
  phone VARCHAR(50)
);

-- Issues table
CREATE TABLE issues (
  id INT AUTO_INCREMENT PRIMARY KEY,
  book_id INT NOT NULL,
  member_id INT NOT NULL,
  issue_date DATE NOT NULL,
  return_date DATE,
  returned BOOLEAN DEFAULT FALSE,
  FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE,
  FOREIGN KEY (member_id) REFERENCES members(id) ON DELETE CASCADE
);

-- Sample data
INSERT INTO books (title, author, isbn, total_copies, available_copies) VALUES
('The C++ Programming Language', 'Bjarne Stroustrup', '9780321563842', 3, 3),
('Clean Code', 'Robert C. Martin', '9780132350884', 2, 2);

INSERT INTO members (name, email, phone) VALUES
('Alice Kumar', 'alice@example.com', '9876543210'),
('Ravi Sharma', 'ravi@example.com', '9123456780');