// models.h - entity classes for the Library Management System
#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>

struct Book {
    int id = 0;
    std::string title;
    std::string author;
    std::string category;
    int totalQty = 0;
    int availableQty = 0;  // copies currently on the shelf
};

struct Member {
    std::string id;  // e.g. M1001
    std::string name;
    std::string contact;
    std::string passHash;
    std::string joinedAt;
};

struct IssueRecord {
    int id = 0;
    int bookId = 0;
    std::string memberId;
    std::string issueDate;
    std::string dueDate;
    std::string returnDate;  // empty while the book is still out
    long long fine = 0;      // paise, fixed when the book is returned
    bool finePaid = false;

    bool returned() const { return !returnDate.empty(); }
};

struct Reservation {
    int id = 0;
    int bookId = 0;
    std::string memberId;
    std::string date;
    std::string status = "Waiting";  // Waiting -> Ready -> Fulfilled / Cancelled
};

#endif
