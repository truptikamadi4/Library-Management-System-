// admin_ui.h - Librarian / admin dashboard: books, members, circulation, fines, reports
#ifndef ADMIN_UI_H
#define ADMIN_UI_H

#include <fstream>
#include <map>

#include "member_ui.h"

class AdminUI {
public:
    AdminUI(Database& db, Library& lib, MemberUI& members) : db_(db), lib_(lib), memberUi_(members) {}

    static bool login() {
        util::title("LIBRARIAN / ADMIN LOGIN");
        std::string u = util::readLine("Username : ");
        std::string p = util::readLine("Password : ");
        if (u == "admin" && p == "admin123") return true;
        std::cout << "  ! Invalid admin credentials.\n";
        return false;
    }

    void menu() {
        while (true) {
            util::title("ADMIN DASHBOARD  |  Library date: " + db_.today() + (db_.simulatedDate.empty() ? "" : " (simulated)"));
            std::cout << "  1. Book management\n  2. Member management\n  3. Issue / return / circulation\n"
                         "  4. Statistics & report\n  5. Set simulated date (demo of overdue fines)\n  6. Logout\n";
            int c = util::readInt("\n  Choose: ", 1, 6);
            switch (c) {
                case 1: bookMenu(); break;
                case 2: memberMenu(); break;
                case 3: circulationMenu(); break;
                case 4: dashboard(true); break;
                case 5: setDate(); break;
                case 6: std::cout << "  Admin logged out.\n"; return;
            }
        }
    }

private:
    Database& db_;
    Library& lib_;
    MemberUI& memberUi_;

    // ------------------------------------------------------------ books
    void bookMenu() {
        while (true) {
            util::title("BOOK MANAGEMENT");
            std::cout << "  1. Add new book\n  2. Update book details\n  3. Delete book\n  4. View all books\n  5. Search books\n"
                         "  6. Check stock of a book\n  7. Back\n";
            int c = util::readInt("\n  Choose: ", 1, 7);
            if (c == 7) return;
            if (c == 1) addBook();
            else if (c == 2) updateBook();
            else if (c == 3) deleteBook();
            else if (c == 4) Printer::books(lib_.allBooksSorted());
            else if (c == 5) memberUi_.searchMenu();
            else stock();
        }
    }

    void addBook() {
        util::title("ADD BOOK");
        std::string t = util::readNonEmpty("Title    : ");
        std::string a = util::readNonEmpty("Author   : ");
        std::string cat = util::readNonEmpty("Category : ");
        int q = util::readInt("Quantity (1-500): ", 1, 500);
        Book& b = lib_.addBook(t, a, cat, q);
        std::cout << "  Book added with ID " << b.id << ".\n";
    }

    void updateBook() {
        util::title("UPDATE BOOK");
        int id = util::readInt("Book ID: ", 0, 2000000000);
        Book* b = db_.findBook(id);
        if (!b) { std::cout << "  ! Book not found.\n"; return; }
        std::cout << "  Current: " << b->title << " | " << b->author << " | " << b->category << " | total copies " << b->totalQty << "\n";
        std::cout << "  (press Enter to keep the current value)\n";
        std::string t = util::sanitize(util::readLine("New title    : "));
        std::string a = util::sanitize(util::readLine("New author   : "));
        std::string c = util::sanitize(util::readLine("New category : "));
        int q = b->totalQty;
        std::string qs = util::readLine("New total copies: ");
        if (!qs.empty()) {
            if (!util::isDigits(qs) || qs.size() > 4) { std::cout << "  ! Invalid quantity - nothing changed.\n"; return; }
            q = std::stoi(qs);
        }
        Result r = lib_.updateBook(id, t, a, c, q);
        std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
    }

    void deleteBook() {
        util::title("DELETE BOOK");
        int id = util::readInt("Book ID: ", 0, 2000000000);
        Book* b = db_.findBook(id);
        if (!b) { std::cout << "  ! Book not found.\n"; return; }
        if (!util::confirm("  Delete \"" + b->title + "\"?")) return;
        Result r = lib_.deleteBook(id);
        std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
    }

    void stock() {
        int id = util::readInt("Book ID: ", 0, 2000000000);
        Book* b = db_.findBook(id);
        if (!b) { std::cout << "  ! Book not found.\n"; return; }
        int waiting = 0, ready = 0;
        for (auto& r : db_.reservations) {
            if (r.bookId != id) continue;
            if (r.status == "Waiting") waiting++;
            if (r.status == "Ready") ready++;
        }
        std::cout << "  \"" << b->title << "\" by " << b->author << "\n  Total copies: " << b->totalQty << "   On shelf: " << b->availableQty
                  << "   Issued: " << lib_.copiesOut(*b) << "\n  Reservations waiting: " << waiting << "   held for pickup: " << ready << "\n";
    }

    // ------------------------------------------------------------ members
    void memberMenu() {
        while (true) {
            util::title("MEMBER MANAGEMENT");
            std::cout << "  1. Register new member\n  2. View all members\n  3. Update member details\n  4. Delete member\n  5. Member record\n  6. Back\n";
            int c = util::readInt("\n  Choose: ", 1, 6);
            if (c == 6) return;
            if (c == 1) {
                std::string n = util::readNonEmpty("Name    : ");
                std::string ct = util::readNonEmpty("Contact : ");
                std::string pw = util::readLine("Initial password (min 4, Enter = member1234): ");
                if (pw.size() < 4 || pw.find(',') != std::string::npos) pw = "member1234";
                Member& m = lib_.registerMember(n, ct, pw);
                std::cout << "  Registered. Member ID: " << m.id << " (password: " << pw << ")\n";
            } else if (c == 2) {
                listMembers();
            } else if (c == 3) {
                Member* m = db_.findMember(util::readLine("Member ID: "));
                if (!m) { std::cout << "  ! Member not found.\n"; continue; }
                std::cout << "  Current: " << m->name << " | " << m->contact << "  (Enter = keep)\n";
                std::string n = util::sanitize(util::readLine("New name    : "));
                std::string ct = util::sanitize(util::readLine("New contact : "));
                if (!n.empty()) m->name = n;
                if (!ct.empty()) m->contact = ct;
                db_.save();
                std::cout << "  Member updated.\n";
            } else if (c == 4) {
                std::string id = util::readLine("Member ID to delete: ");
                Member* m = db_.findMember(id);
                if (!m) { std::cout << "  ! Member not found.\n"; continue; }
                if (!util::confirm("  Delete " + m->name + " (" + m->id + ")?")) continue;
                Result r = lib_.deleteMember(id);
                std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
            } else {
                Member* m = db_.findMember(util::readLine("Member ID: "));
                if (!m) { std::cout << "  ! Member not found.\n"; continue; }
                std::cout << "  " << m->id << " | " << m->name << " | " << m->contact << " | joined " << m->joinedAt << "\n  Books issued: "
                          << lib_.issuedCount(m->id) << "   Unpaid fines: " << util::rs(lib_.unpaidSettledFines(m->id)) << "   Accruing: "
                          << util::rs(lib_.accruedFines(m->id)) << "\n";
                std::vector<const IssueRecord*> all;
                for (auto& i : db_.issues)
                    if (i.memberId == m->id) all.push_back(&i);
                Printer::issues(db_, lib_, all, false);
            }
        }
    }

    void listMembers() {
        util::title("ALL MEMBERS");
        if (db_.members.empty()) { std::cout << "  No members registered yet.\n"; return; }
        std::cout << "  " << std::left << std::setw(8) << "ID" << std::setw(22) << "Name" << std::setw(26) << "Contact" << std::setw(12) << "Joined"
                  << std::setw(8) << "Issued" << "Fines due\n";
        util::line('-', 90);
        for (auto& m : db_.members)
            std::cout << "  " << std::setw(8) << m.id << std::setw(22) << Printer::cut(m.name, 20) << std::setw(26) << Printer::cut(m.contact, 24)
                      << std::setw(12) << m.joinedAt << std::setw(8) << lib_.issuedCount(m.id)
                      << util::rs(lib_.unpaidSettledFines(m.id) + lib_.accruedFines(m.id)) << "\n";
        std::cout << std::right;
    }

    // ------------------------------------------------------------ circulation
    void circulationMenu() {
        while (true) {
            util::title("CIRCULATION");
            std::cout << "  1. Issue a book to a member\n  2. Return a book\n  3. All books currently issued\n  4. Overdue books\n"
                         "  5. Collect a member's unpaid fines\n  6. View reservations\n  7. Back\n";
            int c = util::readInt("\n  Choose: ", 1, 7);
            if (c == 7) return;
            if (c == 1) {
                std::string mid = util::readLine("Member ID: ");
                if (!db_.findMember(mid)) { std::cout << "  ! Member not found.\n"; continue; }
                int bid = util::readInt("Book ID  : ", 0, 2000000000);
                Result r = lib_.issueBook(mid, bid);
                std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
            } else if (c == 2) {
                returnFlow();
            } else if (c == 3) {
                std::vector<const IssueRecord*> v;
                for (auto& i : db_.issues) if (!i.returned()) v.push_back(&i);
                Printer::issues(db_, lib_, v, true);
            } else if (c == 4) {
                Printer::issues(db_, lib_, lib_.overdueList(), true);
            } else if (c == 5) {
                Member* m = db_.findMember(util::readLine("Member ID: "));
                if (!m) { std::cout << "  ! Member not found.\n"; continue; }
                long long due = lib_.unpaidSettledFines(m->id);
                if (due == 0) { std::cout << "  " << m->name << " has no unpaid fines on returned books.\n"; continue; }
                if (util::confirm("  Collect " + util::rs(due) + " from " + m->name + "?"))
                    std::cout << "  Collected " << util::rs(lib_.payFines(m->id)) << ".\n";
            } else {
                reservationList();
            }
        }
    }

    void returnFlow() {
        std::string mid = util::readLine("Member ID: ");
        Member* m = db_.findMember(mid);
        if (!m) { std::cout << "  ! Member not found.\n"; return; }
        auto mine = lib_.activeIssuesOf(m->id);
        if (mine.empty()) { std::cout << "  " << m->name << " has no books to return.\n"; return; }
        std::vector<const IssueRecord*> view(mine.begin(), mine.end());
        Printer::issues(db_, lib_, view, false);
        int id = util::readInt("  IssueID to return (0 = back): ", 0, 2000000000);
        if (id == 0) return;
        bool belongs = false;
        for (auto* i : mine) if (i->id == id) belongs = true;
        if (!belongs) { std::cout << "  ! That issue ID does not belong to this member.\n"; return; }
        long long fine = lib_.fineFor(*db_.findIssue(id));
        bool collect = false;
        if (fine > 0) {
            std::cout << "  Overdue fine: " << util::rs(fine) << "\n";
            collect = util::confirm("  Collect the fine now?");
        }
        Result r = lib_.returnBook(id, collect);
        std::cout << (r.ok ? "  " : "  ! ") << r.msg << "\n";
    }

    void reservationList() {
        util::title("ACTIVE RESERVATIONS");
        bool any = false;
        for (auto& r : db_.reservations) {
            if (r.status != "Waiting" && r.status != "Ready") continue;
            any = true;
            Book* b = db_.findBook(r.bookId);
            std::cout << "  #" << r.id << " | " << r.memberId << " | " << (b ? b->title : "?") << " | " << r.status << " | since " << r.date
                      << " | position " << lib_.queuePosition(r) << "\n";
        }
        if (!any) std::cout << "  No active reservations.\n";
    }

    // ------------------------------------------------------------ dashboard + report
    void dashboard(bool writeFile) {
        util::title("STATISTICS & REPORT");
        int titles = static_cast<int>(db_.books.size()), copies = 0, onShelf = 0;
        for (auto& b : db_.books) { copies += b.totalQty; onShelf += b.availableQty; }
        int active = 0, overdue = 0;
        long long pending = 0, collected = 0;
        std::map<int, int> popularity;
        for (auto& i : db_.issues) {
            popularity[i.bookId]++;
            if (!i.returned()) {
                active++;
                if (lib_.overdueDays(i) > 0) overdue++;
                pending += lib_.fineFor(i);
            } else if (i.fine > 0) {
                (i.finePaid ? collected : pending) += i.fine;
            }
        }
        int resv = 0;
        for (auto& r : db_.reservations) if (r.status == "Waiting" || r.status == "Ready") resv++;

        std::ostringstream os;
        os << "  Library date        : " << db_.today() << "\n";
        os << "  Book titles         : " << titles << "  (" << copies << " copies, " << onShelf << " on the shelf)\n";
        os << "  Registered members  : " << db_.members.size() << "\n";
        os << "  Books issued now    : " << active << "   (overdue: " << overdue << ")\n";
        os << "  Total issues so far : " << db_.issues.size() << "\n";
        os << "  Active reservations : " << resv << "\n";
        os << "  Fines collected     : " << util::rs(collected) << "\n";
        os << "  Fines pending       : " << util::rs(pending) << "\n";
        os << "  Most issued books   :\n";
        std::vector<std::pair<int, int>> top(popularity.begin(), popularity.end());
        std::sort(top.begin(), top.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) { return a.second > b.second; });
        if (top.empty()) os << "    (no issues yet)\n";
        for (size_t k = 0; k < top.size() && k < 5; k++) {
            Book* b = db_.findBook(top[k].first);
            os << "    " << top[k].second << " x " << (b ? b->title : "(deleted book)") << "\n";
        }
        auto od = lib_.overdueList();
        os << "  Overdue list        : " << (od.empty() ? "none" : "") << "\n";
        for (auto* i : od) {
            Book* b = db_.findBook(i->bookId);
            os << "    " << i->memberId << " - " << (b ? b->title : "?") << " (due " << i->dueDate << ", " << lib_.overdueDays(*i) << " day(s) late, fine "
               << util::rs(lib_.fineFor(*i)) << ")\n";
        }
        std::cout << os.str();
        if (writeFile) {
            std::ofstream("report.txt") << "LIBRARY REPORT\n" << os.str();
            std::cout << "  (Report saved to report.txt)\n";
        }
    }

    void setDate() {
        util::title("SIMULATED DATE");
        std::cout << "  Current library date: " << db_.today() << (db_.simulatedDate.empty() ? " (real clock)" : " (simulated)") << "\n"
                  << "  Use this to test overdue fines, e.g. move the date 20 days ahead.\n";
        std::string s = util::readLine("  Enter date YYYY-MM-DD (Enter = back to real date): ");
        if (s.empty()) { db_.simulatedDate.clear(); db_.save(); std::cout << "  Using the real date again.\n"; return; }
        if (!util::dates::parse(s)) { std::cout << "  ! Invalid date.\n"; return; }
        db_.simulatedDate = s;
        db_.save();
        std::cout << "  Library date set to " << s << ".\n";
    }
};

#endif
