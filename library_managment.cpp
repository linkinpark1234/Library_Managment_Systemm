#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <ctime>

using namespace std;

void finish_with_error(MYSQL *conn) {
    cerr << "MySQL error: " << mysql_error(conn) << "\n";
    mysql_close(conn);
    exit(EXIT_FAILURE);
}

void print_menu() {
    cout << "\n--- Library Management ---\n";
    cout << "1. Add Book\n";
    cout << "2. Remove Book\n";
    cout << "3. List All Books\n";
    cout << "4. Search Books (title/author/isbn)\n";
    cout << "5. Register Member\n";
    cout << "6. List Members\n";
    cout << "7. Issue Book\n";
    cout << "8. Return Book\n";
    cout << "9. List Issued Books\n";
    cout << "0. Exit\n";
    cout << "Choose: ";
}

void add_book(MYSQL *conn) {
    string title, author, isbn;
    int total;
    cout << "Title: "; getline(cin, title);
    cout << "Author: "; getline(cin, author);
    cout << "ISBN: "; getline(cin, isbn);
    cout << "Total copies: "; cin >> total; cin.ignore();

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *query = "INSERT INTO books(title, author, isbn, total_copies, available_copies) VALUES(?, ?, ?, ?, ?)";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) finish_with_error(conn);

    MYSQL_BIND bind[5];
    memset(bind, 0, sizeof(bind));

    // title
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)title.c_str();
    bind[0].buffer_length = title.length();

    // author
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (char*)author.c_str();
    bind[1].buffer_length = author.length();

    // isbn
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (char*)isbn.c_str();
    bind[2].buffer_length = isbn.length();

    // total_copies
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = (char*)&total;
    bind[3].is_unsigned = false;

    // available_copies
    int available = total;
    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = (char*)&available;
    bind[4].is_unsigned = false;

    if (mysql_stmt_bind_param(stmt, bind)) finish_with_error(conn);

    if (mysql_stmt_execute(stmt)) {
        cerr << "Failed to add book: " << mysql_stmt_error(stmt) << "\n";
    } else {
        cout << "Book added successfully.\n";
    }
    mysql_stmt_close(stmt);
}

void remove_book(MYSQL *conn) {
    int id;
    cout << "Book ID to remove: "; cin >> id; cin.ignore();

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *query = "DELETE FROM books WHERE id = ?";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) finish_with_error(conn);

    MYSQL_BIND bind;
    memset(&bind, 0, sizeof(bind));
    bind.buffer_type = MYSQL_TYPE_LONG;
    bind.buffer = (char*)&id;

    if (mysql_stmt_bind_param(stmt, &bind)) finish_with_error(conn);

    if (mysql_stmt_execute(stmt)) {
        cerr << "Failed to remove book: " << mysql_stmt_error(stmt) << "\n";
    } else {
        cout << "Book removed (if existed).\n";
    }
    mysql_stmt_close(stmt);
}

void list_books(MYSQL *conn) {
    if (mysql_query(conn, "SELECT id, title, author, isbn, total_copies, available_copies FROM books")) finish_with_error(conn);
    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        cout << "No books found.\n";
        return;
    }
    cout << left << setw(5) << "ID" << setw(30) << "Title" << setw(20) << "Author" << setw(18) << "ISBN" << setw(8) << "Total" << setw(10) << "Available" << "\n";
    cout << string(95, '-') << "\n";
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        cout << setw(5) << (row[0] ? row[0] : "") 
             << setw(30) << (row[1] ? row[1] : "")
             << setw(20) << (row[2] ? row[2] : "")
             << setw(18) << (row[3] ? row[3] : "")
             << setw(8) << (row[4] ? row[4] : "")
             << setw(10) << (row[5] ? row[5] : "") << "\n";
    }
    mysql_free_result(result);
}

void search_books(MYSQL *conn) {
    string term;
    cout << "Enter search term (title/author/isbn): "; getline(cin, term);
    // Use LIKE with escaped term
    string q = "SELECT id, title, author, isbn, total_copies, available_copies FROM books WHERE title LIKE CONCAT('%', ?, '%') OR author LIKE CONCAT('%', ?, '%') OR isbn LIKE CONCAT('%', ?, '%')";
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, q.c_str(), q.length())) finish_with_error(conn);

    MYSQL_BIND bind_in[3];
    memset(bind_in, 0, sizeof(bind_in));

    // Provide term three times; but prepared statements require separate binds each time
    // We'll bind three parameters to the same C-string buffer
    bind_in[0].buffer_type = MYSQL_TYPE_STRING;
    bind_in[0].buffer = (char*)term.c_str();
    bind_in[0].buffer_length = term.length();

    bind_in[1] = bind_in[0];
    bind_in[2] = bind_in[0];

    if (mysql_stmt_bind_param(stmt, bind_in)) finish_with_error(conn);

    if (mysql_stmt_execute(stmt)) {
        cerr << "Search failed: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return;
    }

    MYSQL_RES *meta = mysql_stmt_result_metadata(stmt);
    if (!meta) {
        cout << "No results.\n";
        mysql_stmt_close(stmt);
        return;
    }

    MYSQL_BIND bind_out[6];
    memset(bind_out, 0, sizeof(bind_out));
    char id_buf[20], title_buf[512], author_buf[256], isbn_buf[128];
    long total_copies;
    long available_copies;
    unsigned long len_id=0, len_title=0, len_author=0, len_isbn=0;

    bind_out[0].buffer_type = MYSQL_TYPE_STRING; bind_out[0].buffer = id_buf; bind_out[0].buffer_length = sizeof(id_buf); bind_out[0].length = &len_id;
    bind_out[1].buffer_type = MYSQL_TYPE_STRING; bind_out[1].buffer = title_buf; bind_out[1].buffer_length = sizeof(title_buf); bind_out[1].length = &len_title;
    bind_out[2].buffer_type = MYSQL_TYPE_STRING; bind_out[2].buffer = author_buf; bind_out[2].buffer_length = sizeof(author_buf); bind_out[2].length = &len_author;
    bind_out[3].buffer_type = MYSQL_TYPE_STRING; bind_out[3].buffer = isbn_buf; bind_out[3].buffer_length = sizeof(isbn_buf); bind_out[3].length = &len_isbn;
    bind_out[4].buffer_type = MYSQL_TYPE_LONG; bind_out[4].buffer = &total_copies;
    bind_out[5].buffer_type = MYSQL_TYPE_LONG; bind_out[5].buffer = &available_copies;

    if (mysql_stmt_bind_result(stmt, bind_out)) finish_with_error(conn);
    if (mysql_stmt_store_result(stmt)) finish_with_error(conn);

    cout << left << setw(5) << "ID" << setw(30) << "Title" << setw(20) << "Author" << setw(18) << "ISBN" << setw(8) << "Total" << setw(10) << "Available" << "\n";
    cout << string(95, '-') << "\n";
    while (mysql_stmt_fetch(stmt) == 0) {
        cout << setw(5) << (id_buf) 
             << setw(30) << (title_buf)
             << setw(20) << (author_buf)
             << setw(18) << (isbn_buf)
             << setw(8) << total_copies
             << setw(10) << available_copies << "\n";
    }

    mysql_free_result(meta);
    mysql_stmt_close(stmt);
}

void register_member(MYSQL *conn) {
    string name, email, phone;
    cout << "Name: "; getline(cin, name);
    cout << "Email: "; getline(cin, email);
    cout << "Phone: "; getline(cin, phone);

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *query = "INSERT INTO members(name, email, phone) VALUES(?, ?, ?)";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) finish_with_error(conn);

    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING; bind[0].buffer = (char*)name.c_str(); bind[0].buffer_length = name.length();
    bind[1].buffer_type = MYSQL_TYPE_STRING; bind[1].buffer = (char*)email.c_str(); bind[1].buffer_length = email.length();
    bind[2].buffer_type = MYSQL_TYPE_STRING; bind[2].buffer = (char*)phone.c_str(); bind[2].buffer_length = phone.length();

    if (mysql_stmt_bind_param(stmt, bind)) finish_with_error(conn);
    if (mysql_stmt_execute(stmt)) {
        cerr << "Failed to register member: " << mysql_stmt_error(stmt) << "\n";
    } else {
        cout << "Member registered successfully.\n";
    }
    mysql_stmt_close(stmt);
}

void list_members(MYSQL *conn) {
    if (mysql_query(conn, "SELECT id, name, email, phone FROM members")) finish_with_error(conn);
    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        cout << "No members found.\n";
        return;
    }
    cout << left << setw(5) << "ID" << setw(25) << "Name" << setw(30) << "Email" << setw(15) << "Phone" << "\n";
    cout << string(75, '-') << "\n";
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        cout << setw(5) << (row[0] ? row[0] : "")
             << setw(25) << (row[1] ? row[1] : "")
             << setw(30) << (row[2] ? row[2] : "")
             << setw(15) << (row[3] ? row[3] : "") << "\n";
    }
    mysql_free_result(result);
}

string today_str() {
    time_t t = time(nullptr);
    tm *lt = localtime(&t);
    char buf[20];
    strftime(buf, sizeof(buf), "%F", lt); // YYYY-MM-DD
    return string(buf);
}

void issue_book(MYSQL *conn) {
    int book_id, member_id;
    cout << "Book ID: "; cin >> book_id;
    cout << "Member ID: "; cin >> member_id; cin.ignore();

    // Check availability
    MYSQL_STMT *stmt_check = mysql_stmt_init(conn);
    const char *q_check = "SELECT available_copies FROM books WHERE id = ?";
    if (mysql_stmt_prepare(stmt_check, q_check, strlen(q_check))) finish_with_error(conn);
    MYSQL_BIND bind_check;
    memset(&bind_check,0,sizeof(bind_check));
    bind_check.buffer_type = MYSQL_TYPE_LONG;
    bind_check.buffer = (char*)&book_id;
    if (mysql_stmt_bind_param(stmt_check, &bind_check)) finish_with_error(conn);
    if (mysql_stmt_execute(stmt_check)) finish_with_error(conn);

    MYSQL_BIND result_bind;
    memset(&result_bind,0,sizeof(result_bind));
    int available;
    result_bind.buffer_type = MYSQL_TYPE_LONG;
    result_bind.buffer = &available;

    if (mysql_stmt_bind_result(stmt_check, &result_bind)) finish_with_error(conn);
    if (mysql_stmt_store_result(stmt_check)) finish_with_error(conn);

    if (mysql_stmt_fetch(stmt_check) != 0) {
        cout << "Book not found.\n";
        mysql_stmt_close(stmt_check);
        return;
    }
    mysql_stmt_close(stmt_check);

    if (available <= 0) {
        cout << "No copies available to issue.\n";
        return;
    }

    // Insert into issues
    MYSQL_STMT *stmt_issue = mysql_stmt_init(conn);
    const char *q_issue = "INSERT INTO issues(book_id, member_id, issue_date, returned) VALUES(?, ?, ?, 0)";
    if (mysql_stmt_prepare(stmt_issue, q_issue, strlen(q_issue))) finish_with_error(conn);

    string issue_date = today_str();
    MYSQL_BIND bind_issue[3];
    memset(bind_issue,0,sizeof(bind_issue));
    bind_issue[0].buffer_type = MYSQL_TYPE_LONG; bind_issue[0].buffer = (char*)&book_id;
    bind_issue[1].buffer_type = MYSQL_TYPE_LONG; bind_issue[1].buffer = (char*)&member_id;
    bind_issue[2].buffer_type = MYSQL_TYPE_STRING; bind_issue[2].buffer = (char*)issue_date.c_str(); bind_issue[2].buffer_length = issue_date.length();

    if (mysql_stmt_bind_param(stmt_issue, bind_issue)) finish_with_error(conn);
    if (mysql_stmt_execute(stmt_issue)) {
        cerr << "Failed to issue book: " << mysql_stmt_error(stmt_issue) << "\n";
        mysql_stmt_close(stmt_issue);
        return;
    }
    mysql_stmt_close(stmt_issue);

    // Update books.available_copies -= 1
    MYSQL_STMT *stmt_upd = mysql_stmt_init(conn);
    const char *q_upd = "UPDATE books SET available_copies = available_copies - 1 WHERE id = ?";
    if (mysql_stmt_prepare(stmt_upd, q_upd, strlen(q_upd))) finish_with_error(conn);
    MYSQL_BIND bind_upd; memset(&bind_upd,0,sizeof(bind_upd));
    bind_upd.buffer_type = MYSQL_TYPE_LONG; bind_upd.buffer = (char*)&book_id;
    if (mysql_stmt_bind_param(stmt_upd, &bind_upd)) finish_with_error(conn);
    if (mysql_stmt_execute(stmt_upd)) {
        cerr << "Failed to update book copies: " << mysql_stmt_error(stmt_upd) << "\n";
    } else {
        cout << "Book issued successfully.\n";
    }
    mysql_stmt_close(stmt_upd);
}

void return_book(MYSQL *conn) {
    int issue_id;
    cout << "Issue ID to return: "; cin >> issue_id; cin.ignore();

    // Check if issue exists and not already returned.
    MYSQL_STMT *stmt_check = mysql_stmt_init(conn);
    const char *q_check = "SELECT book_id, returned FROM issues WHERE id = ?";
    if (mysql_stmt_prepare(stmt_check, q_check, strlen(q_check))) finish_with_error(conn);

    MYSQL_BIND bind_check; memset(&bind_check,0,sizeof(bind_check));
    bind_check.buffer_type = MYSQL_TYPE_LONG; bind_check.buffer = (char*)&issue_id;
    if (mysql_stmt_bind_param(stmt_check, &bind_check)) finish_with_error(conn);
    if (mysql_stmt_execute(stmt_check)) finish_with_error(conn);

    MYSQL_BIND bind_res[2]; memset(bind_res,0,sizeof(bind_res));
    int book_id; my_bool returned_flag;
    bind_res[0].buffer_type = MYSQL_TYPE_LONG; bind_res[0].buffer = &book_id;
    bind_res[1].buffer_type = MYSQL_TYPE_TINY; bind_res[1].buffer = &returned_flag;

    if (mysql_stmt_bind_result(stmt_check, bind_res)) finish_with_error(conn);
    if (mysql_stmt_store_result(stmt_check)) finish_with_error(conn);

    if (mysql_stmt_fetch(stmt_check) != 0) {
        cout << "Issue record not found.\n";
        mysql_stmt_close(stmt_check);
        return;
    }
    mysql_stmt_close(stmt_check);

    if (returned_flag) {
        cout << "This book is already returned.\n";
        return;
    }

    // Update issues: set returned = 1, return_date = today
    MYSQL_STMT *stmt_ret = mysql_stmt_init(conn);
    const char *q_ret = "UPDATE issues SET returned = 1, return_date = ? WHERE id = ?";
    if (mysql_stmt_prepare(stmt_ret, q_ret, strlen(q_ret))) finish_with_error(conn);

    string ret_date = today_str();
    MYSQL_BIND bind_ret[2]; memset(bind_ret,0,sizeof(bind_ret));
    bind_ret[0].buffer_type = MYSQL_TYPE_STRING; bind_ret[0].buffer = (char*)ret_date.c_str(); bind_ret[0].buffer_length = ret_date.length();
    bind_ret[1].buffer_type = MYSQL_TYPE_LONG; bind_ret[1].buffer = (char*)&issue_id;

    if (mysql_stmt_bind_param(stmt_ret, bind_ret)) finish_with_error(conn);
    if (mysql_stmt_execute(stmt_ret)) {
        cerr << "Failed to mark returned: " << mysql_stmt_error(stmt_ret) << "\n";
        mysql_stmt_close(stmt_ret);
        return;
    }
    mysql_stmt_close(stmt_ret);

    // Update books.available_copies += 1
    MYSQL_STMT *stmt_upd = mysql_stmt_init(conn);
    const char *q_upd = "UPDATE books SET available_copies = available_copies + 1 WHERE id = ?";
    if (mysql_stmt_prepare(stmt_upd, q_upd, strlen(q_upd))) finish_with_error(conn);
    MYSQL_BIND bind_upd; memset(&bind_upd,0,sizeof(bind_upd));
    bind_upd.buffer_type = MYSQL_TYPE_LONG; bind_upd.buffer = (char*)&book_id;
    if (mysql_stmt_bind_param(stmt_upd, &bind_upd)) finish_with_error(conn);
    if (mysql_stmt_execute(stmt_upd)) {
        cerr << "Failed to update book copies: " << mysql_stmt_error(stmt_upd) << "\n";
    } else {
        cout << "Book returned successfully.\n";
    }
    mysql_stmt_close(stmt_upd);
}

void list_issued(MYSQL *conn) {
    const char *q = 
    "SELECT i.id, b.title, m.name, i.issue_date, i.return_date, i.returned "
    "FROM issues i "
    "JOIN books b ON i.book_id = b.id "
    "JOIN members m ON i.member_id = m.id "
    "ORDER BY i.issue_date DESC";
    if (mysql_query(conn, q)) finish_with_error(conn);
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) { cout << "No issued records.\n"; return; }
    cout << left << setw(5) << "ID" << setw(30) << "Book" << setw(20) << "Member" << setw(12) << "Issued" << setw(12) << "Returned" << setw(8) << "Status" << "\n";
    cout << string(95, '-') << "\n";
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        cout << setw(5) << (row[0] ? row[0] : "")
             << setw(30) << (row[1] ? row[1] : "")
             << setw(20) << (row[2] ? row[2] : "")
             << setw(12) << (row[3] ? row[3] : "")
             << setw(12) << (row[4] ? row[4] : "N/A")
             << setw(8) << ((row[5] && string(row[5]) == "1") ? "Yes" : "No") << "\n";
    }
    mysql_free_result(res);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        cerr << "Usage: " << argv[0] << " <host> <user> <password> <database>\n";
        return 1;
    }

    const char *host = argv[1];
    const char *user = argv[2];
    const char *pass = argv[3];
    const char *db   = argv[4];

    MYSQL *conn = mysql_init(nullptr);
    if (!conn) {
        cerr << "mysql_init() failed\n";
        return EXIT_FAILURE;
    }

    if (!mysql_real_connect(conn, host, user, pass, db, 0, NULL, 0)) {
        cerr << "mysql_real_connect() failed: " << mysql_error(conn) << "\n";
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    while (true) {
        print_menu();
        int choice;
        if (!(cin >> choice)) { cin.clear(); cin.ignore(); continue; }
        cin.ignore();
        switch (choice) {
            case 1: add_book(conn); break;
            case 2: remove_book(conn); break;
            case 3: list_books(conn); break;
            case 4: search_books(conn); break;
            case 5: register_member(conn); break;
            case 6: list_members(conn); break;
            case 7: issue_book(conn); break;
            case 8: return_book(conn); break;
            case 9: list_issued(conn); break;
            case 0: cout << "Exiting...\n"; mysql_close(conn); return 0;
            default: cout << "Invalid choice\n";
        }
    }

    mysql_close(conn);
    return 0;
}