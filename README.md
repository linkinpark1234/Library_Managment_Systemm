Library Management System (C++ + MySQL)
---------------------------------------
Description:
A simple console-based Library Management System. Features include:
- Add / Remove books
- List / Search books
- Register members and list members
- Issue books and record returns
- Track issued books with issue/return dates

Tech stack:
- C++ (single-file example)
- MySQL (schema in library_db.sql)
- Uses MySQL C API (libmysqlclient)

Setup:
1. Install MySQL server and client libraries.
   - On Ubuntu: sudo apt update && sudo apt install mysql-server libmysqlclient-dev
2. Create the database and seed sample data:
   - mysql -u root -p < library_db.sql
3. Build:
   - g++ -std=c++17 library_management.cpp -o library_management -lmysqlclient
   - (If linker fails, ensure mysqlclient dev libs are installed and library path is visible.)
4. Run:
   - ./library_management localhost root your_mysql_password library_db

Notes for resume:
- Implemented a full-stack-like system (C++ backend interacting with MySQL DB).
- Demonstrates knowledge of SQL schema design, prepared statements, CRUD operations, and basic application flow.
- Repo contains source code, SQL schema, and run instructions.

Optional improvements 
- Wrap logic into C++ classes (Book, Member, Issue, DBConnector).
- Use MySQL Connector/C++ to leverage C++-style API.
- Add a web frontend using Flask (Python) or a REST API to practice full-stack integration.
- Add unit tests and input validation/error handling for production-readiness.
