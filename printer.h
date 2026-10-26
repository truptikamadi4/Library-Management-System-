// printer.h - shared table printing helpers
#ifndef PRINTER_H
#define PRINTER_H

#include "library.h"

class Printer {
public:
    static std::string cut(const std::string& s, size_t n) { return s.size() <= n ? s : s.substr(0, n - 2) + ".."; }

    static void books(const std::vector<const Book*>& list) {
        if (list.empty()) { std::cout << "  No books found.\n"; return; }
        std::cout << "  " << std::left << std::setw(6) << "ID" << std::setw(42) << "Title" << std::setw(24) << "Author" << std::setw(18)
                  << "Category" << "Avail/Total\n";
        util::line('-', 100);
        for (auto* b : list)
            std::cout << "  " << std::setw(6) << b->id << std::setw(42) << cut(b->title, 40) << std::setw(24) << cut(b->author, 22)
                      << std::setw(18) << cut(b->category, 16) << b->availableQty << "/" << b->totalQty << "\n";
        std::cout << std::right;
        std::cout << "  " << list.size() << " book(s) listed.\n";
    }

    // list of active (not returned) issues with due dates and accrued fines
    static void issues(Database& db, Library& lib, const std::vector<const IssueRecord*>& list, bool showMember) {
        if (list.empty()) { std::cout << "  Nothing to show.\n"; return; }
        std::cout << "  " << std::left << std::setw(9) << "IssueID";
        if (showMember) std::cout << std::setw(8) << "Member";
        std::cout << std::setw(36) << "Book" << std::setw(12) << "Issued" << std::setw(12) << "Due" << std::setw(14) << "Status" << "Fine\n";
        util::line('-', showMember ? 100 : 92);
        for (auto* i : list) {
            Book* b = db.findBook(i->bookId);
            long long od = lib.overdueDays(*i);
            std::string status = i->returned() ? "Returned" : (od > 0 ? "OVERDUE " + std::to_string(od) + "d" : "On time");
            std::cout << "  " << std::setw(9) << i->id;
            if (showMember) std::cout << std::setw(8) << i->memberId;
            std::cout << std::setw(36) << cut(b ? b->title : "(deleted book)", 34) << std::setw(12) << i->issueDate << std::setw(12) << i->dueDate
                      << std::setw(14) << status << util::rs(lib.fineFor(*i)) << "\n";
        }
        std::cout << std::right;
    }
};

#endif
