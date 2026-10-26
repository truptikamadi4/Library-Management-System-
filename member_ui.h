// member_ui.h - Member Management + self service menu (login, search, issue, return, reserve, fines)
#ifndef MEMBER_UI_H
#define MEMBER_UI_H

#include "printer.h"

class MemberUI {
public:
    MemberUI(Database& db, Library& lib) : db_(db), lib_(lib) {}

    // ------------------------------------------------------------ registration / login
    void registerSelf() {
        util::title("MEMBER REGISTRATION");
        std::string name = util::readNonEmpty("Full name         : ");
        std::string contact = util::readNonEmpty("Contact (phone/email): ");
        std::string pw = readPassword("Password (min 4)  : ");
        Member& m = lib_.registerMember(name, contact, pw);
        std::cout << "\n  Registration successful! Your Member ID is " << m.id << " - use it to login.\n";
    }

    Member* login() {
        util::title("MEMBER LOGIN");
        std::string id = util::readLine("Member ID : ");
        std::string pw = util::readLine("Password  : ");
        Member* m = db_.findMember(id);
        if (!m || m->passHash != util::hashPassword(pw)) {
            std::cout << "  ! Invalid member ID or password.\n";
            return nullptr;
        }
        std::cout << "\n  Welcome, " << m->name << "!\n";
        return m;
    }

    void menu(Member& m) {
        while (true) {
            util::title("MEMBER MENU - " + m.name + " (" + m.id + ")");
            notifications(m);
            std::cout << "  1. Search books           5. My issued books\n"
                         "  2. Available books        6. Reserve a book / my reservations\n"
                         "  3. Issue a book           7. My fines\n"
                         "  4. Return a book          8. Change password   9. Logout\n";
            int c = util::readInt("\n  Choose: ", 1, 9);
            switch (c) {
                case 1: searchMenu(); break;
                case 2: Printer::books(lib_.availableBooks()); break;
                case 3: issue(m); break;
                case 4: giveBack(m); break;
                case 5: myIssues(m); break;
                case 6: reservations(m); break;
                case 7: fines(m); break;
                case 8: changePassword(m); break;
                case 9: std::cout << "  Logged out. Happy reading!\n"; return;
            }
        }
    }

    // Shared by the librarian menu
    void searchMenu() {
        util::title("SEARCH BOOKS");
        std::cout << "  1. By title   2. By author   3. By category   4. Back\n";
        int c = util::readInt("  Choose: ", 1, 4);
        if (c == 4) return;
        std::string q = util::readNonEmpty("  Search text: ");
        auto f = c == 1 ? Library::Field::Title : (c == 2 ? Library::Field::Author : Library::Field::Category);
        Printer::books(lib_.search(f, q));
    }

private:
    Database& db_;
    Library& lib_;

    static std::string readPassword(const std::string& prompt) {
        while (true) {
            std::string p = util::readLine(prompt);
            if (p.size() >= 4 && p.find(',') == std::string::npos) return p;
            std::cout << "  ! Password must have at least 4 characters (no commas).\n";
        }
    }

    void notifications(Member& m) {
        for (auto& r : db_.reservations)
            if (r.memberId == m.id && r.status == "Ready") {
                Book* b = db_.findBook(r.bookId);
                std::cout << "  >> Your reserved book \"" << (b ? b->title : "?") << "\" is READY - issue it from menu option 3.\n";
            }
        int od = 0;
        for (auto* i : lib_.activeIssuesOf(m.id))
            if (lib_.overdueDays(*i) > 0) od++;
        if (od) std::cout << "  >> " << od << " of your books are OVERDUE (fine " << util::rs(Library::FINE_PER_DAY) << "/day). Please return them.\n";
        if (lib_.unpaidSettledFines(m.id) > 0) std::cout << "  >> Unpaid fines: " << util::rs(lib_.unpaidSettledFines(m.id)) << " (menu option 7).\n";
    }

    void issue(Member& m) {
        util::title("ISSUE A BOOK");
        std::cout << "  You currently hold " << lib_.issuedCount(m.id) << " of " << Library::MAX_BOOKS << " allowed books. Loan period: "
                  << Library::LOAN_DAYS << " days.\n";
        int id = util::readInt("  Enter Book ID (0 = back): ", 0, 2000000000);
        if (id == 0) return;
        Result r = lib_.issueBook(m.id, id);
        std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
    }

    void giveBack(Member& m) {
        util::title("RETURN A BOOK");
        auto mine = lib_.activeIssuesOf(m.id);
        if (mine.empty()) { std::cout << "  You have no books to return.\n"; return; }
        std::vector<const IssueRecord*> view(mine.begin(), mine.end());
        Printer::issues(db_, lib_, view, false);
        int id = util::readInt("  Enter IssueID to return (0 = back): ", 0, 2000000000);
        if (id == 0) return;
        bool mineOk = false;
        for (auto* i : mine) if (i->id == id) mineOk = true;
        if (!mineOk) { std::cout << "  ! That issue ID is not in your list.\n"; return; }
        long long fine = lib_.fineFor(*db_.findIssue(id));
        bool pay = false;
        if (fine > 0) {
            std::cout << "  This book is overdue. Fine to pay: " << util::rs(fine) << "\n";
            pay = util::confirm("  Pay the fine now?");
        }
        Result r = lib_.returnBook(id, pay);
        std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
    }

    void myIssues(Member& m) {
        util::title("MY ISSUED BOOKS");
        auto mine = lib_.activeIssuesOf(m.id);
        std::vector<const IssueRecord*> view(mine.begin(), mine.end());
        Printer::issues(db_, lib_, view, false);
        if (util::confirm("\n  Show my full history (including returned books)?")) {
            std::vector<const IssueRecord*> all;
            for (auto& i : db_.issues)
                if (i.memberId == m.id) all.push_back(&i);
            Printer::issues(db_, lib_, all, false);
        }
    }

    void reservations(Member& m) {
        util::title("RESERVATIONS");
        bool any = false;
        for (auto& r : db_.reservations) {
            if (r.memberId != m.id || (r.status != "Waiting" && r.status != "Ready")) continue;
            any = true;
            Book* b = db_.findBook(r.bookId);
            std::cout << "  Reservation #" << r.id << " | " << (b ? b->title : "?") << " | " << r.status << " | queue position " << lib_.queuePosition(r) << "\n";
        }
        if (!any) std::cout << "  You have no active reservations.\n";
        std::cout << "\n  1. Reserve a book   2. Cancel a reservation   3. Back\n";
        int c = util::readInt("  Choose: ", 1, 3);
        if (c == 1) {
            int id = util::readInt("  Book ID to reserve: ", 0, 2000000000);
            Result r = lib_.reserve(m.id, id);
            std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
        } else if (c == 2) {
            int id = util::readInt("  Reservation ID to cancel: ", 0, 2000000000);
            Result r = lib_.cancelReservation(id, m.id);
            std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
        }
    }

    void fines(Member& m) {
        util::title("MY FINES");
        long long settled = lib_.unpaidSettledFines(m.id), accrued = lib_.accruedFines(m.id);
        std::cout << "  Fine rate                        : " << util::rs(Library::FINE_PER_DAY) << " per overdue day\n"
                  << "  Unpaid fines (returned books)     : " << util::rs(settled) << "\n"
                  << "  Accruing on books not yet returned: " << util::rs(accrued) << "\n";
        if (settled > 0 && util::confirm("\n  Pay " + util::rs(settled) + " now?")) {
            long long p = lib_.payFines(m.id);
            std::cout << "  Payment of " << util::rs(p) << " received. Thank you!\n";
        }
    }

    void changePassword(Member& m) {
        util::title("CHANGE PASSWORD");
        if (util::hashPassword(util::readLine("Current password: ")) != m.passHash) { std::cout << "  ! Wrong current password.\n"; return; }
        m.passHash = util::hashPassword(readPassword("New password: "));
        db_.save();
        std::cout << "  Password changed.\n";
    }
};

#endif
