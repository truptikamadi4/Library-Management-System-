// database.h - File Handling Module (CSV persistence, sample data, simulated clock)
#ifndef DATABASE_H
#define DATABASE_H

#include <fstream>
#include <sys/stat.h>

#include "models.h"
#include "utils.h"

class Database {
public:
    std::vector<Book> books;
    std::vector<Member> members;
    std::vector<IssueRecord> issues;
    std::vector<Reservation> reservations;
    int nextBookId = 101;
    int nextMemberNo = 1001;
    int nextIssueId = 1;
    int nextReservationId = 1;
    std::string simulatedDate;  // empty = use the real date

    explicit Database(const std::string& dir = "data") : dir_(dir) {}

    void load() {
        ::mkdir(dir_.c_str(), 0755);
        loadBooks();
        loadMembers();
        loadIssues();
        loadReservations();
        loadSettings();
    }

    void save() {
        saveBooks();
        saveMembers();
        saveIssues();
        saveReservations();
        saveSettings();
    }

    // The library "today" - real date unless the admin set a simulated date for demos
    std::string today() const { return simulatedDate.empty() ? util::today() : simulatedDate; }

    Book* findBook(int id) {
        for (auto& b : books)
            if (b.id == id) return &b;
        return nullptr;
    }
    Member* findMember(const std::string& id) {
        std::string want = util::toLower(id);
        for (auto& m : members)
            if (util::toLower(m.id) == want) return &m;
        return nullptr;
    }
    IssueRecord* findIssue(int id) {
        for (auto& i : issues)
            if (i.id == id) return &i;
        return nullptr;
    }

private:
    std::string dir_;
    std::string path(const std::string& f) const { return dir_ + "/" + f; }

    static std::string f(const std::vector<std::string>& r, size_t i) { return i < r.size() ? r[i] : ""; }
    static long long ll(const std::string& s) {
        try { return s.empty() ? 0 : std::stoll(s); } catch (...) { return 0; }
    }

    static bool rows(const std::string& p, std::vector<std::vector<std::string>>& out) {
        std::ifstream in(p);
        if (!in) return false;
        std::string ln;
        bool first = true;
        while (std::getline(in, ln)) {
            if (first) { first = false; continue; }
            if (util::trim(ln).empty()) continue;
            out.push_back(util::split(ln, ','));
        }
        return true;
    }

    // ---- books (seeded with sample data on first run) ----
    void seedBooks() {
        struct S { const char *t, *a, *c; int q; };
        const S data[] = {{"The C++ Programming Language", "Bjarne Stroustrup", "Programming", 3},
                          {"Let Us C", "Yashavant Kanetkar", "Programming", 4},
                          {"Clean Code", "Robert C. Martin", "Programming", 2},
                          {"Introduction to Algorithms", "Thomas H. Cormen", "Computer Science", 2},
                          {"Database System Concepts", "Abraham Silberschatz", "Computer Science", 3},
                          {"Operating System Concepts", "Abraham Silberschatz", "Computer Science", 2},
                          {"Computer Networks", "Andrew S. Tanenbaum", "Computer Science", 2},
                          {"Python Crash Course", "Eric Matthes", "Programming", 3},
                          {"Digital Logic and Computer Design", "M. Morris Mano", "Electronics", 2},
                          {"Discrete Mathematics and Its Applications", "Kenneth H. Rosen", "Mathematics", 2},
                          {"Wings of Fire", "A. P. J. Abdul Kalam", "Biography", 3},
                          {"The Alchemist", "Paulo Coelho", "Fiction", 1}};
        for (auto& s : data) {
            Book b;
            b.id = nextBookId++;
            b.title = s.t; b.author = s.a; b.category = s.c;
            b.totalQty = b.availableQty = s.q;
            books.push_back(b);
        }
    }
    void loadBooks() {
        books.clear();
        std::vector<std::vector<std::string>> r;
        if (!rows(path("books.csv"), r)) {
            seedBooks();
            saveBooks();
            return;
        }
        for (auto& x : r) {
            Book b;
            b.id = static_cast<int>(ll(f(x, 0))); b.title = f(x, 1); b.author = f(x, 2); b.category = f(x, 3);
            b.totalQty = static_cast<int>(ll(f(x, 4))); b.availableQty = static_cast<int>(ll(f(x, 5)));
            books.push_back(b);
            nextBookId = std::max(nextBookId, b.id + 1);
        }
    }
    void saveBooks() {
        std::ofstream o(path("books.csv"));
        o << "id,title,author,category,totalQty,availableQty\n";
        for (auto& b : books) o << b.id << ',' << b.title << ',' << b.author << ',' << b.category << ',' << b.totalQty << ',' << b.availableQty << '\n';
    }

    // ---- members ----
    void loadMembers() {
        members.clear();
        std::vector<std::vector<std::string>> r;
        if (!rows(path("members.csv"), r)) return;
        for (auto& x : r) {
            Member m;
            m.id = f(x, 0); m.name = f(x, 1); m.contact = f(x, 2); m.passHash = f(x, 3); m.joinedAt = f(x, 4);
            members.push_back(m);
            if (m.id.size() > 1 && util::isDigits(m.id.substr(1))) nextMemberNo = std::max(nextMemberNo, std::stoi(m.id.substr(1)) + 1);
        }
    }
    void saveMembers() {
        std::ofstream o(path("members.csv"));
        o << "id,name,contact,passHash,joinedAt\n";
        for (auto& m : members) o << m.id << ',' << m.name << ',' << m.contact << ',' << m.passHash << ',' << m.joinedAt << '\n';
    }

    // ---- issue / return records ----
    void loadIssues() {
        issues.clear();
        std::vector<std::vector<std::string>> r;
        if (!rows(path("issues.csv"), r)) return;
        for (auto& x : r) {
            IssueRecord i;
            i.id = static_cast<int>(ll(f(x, 0))); i.bookId = static_cast<int>(ll(f(x, 1))); i.memberId = f(x, 2);
            i.issueDate = f(x, 3); i.dueDate = f(x, 4); i.returnDate = f(x, 5); i.fine = ll(f(x, 6)); i.finePaid = f(x, 7) == "1";
            issues.push_back(i);
            nextIssueId = std::max(nextIssueId, i.id + 1);
        }
    }
    void saveIssues() {
        std::ofstream o(path("issues.csv"));
        o << "id,bookId,memberId,issueDate,dueDate,returnDate,fine,finePaid\n";
        for (auto& i : issues)
            o << i.id << ',' << i.bookId << ',' << i.memberId << ',' << i.issueDate << ',' << i.dueDate << ',' << i.returnDate << ','
              << i.fine << ',' << (i.finePaid ? 1 : 0) << '\n';
    }

    // ---- reservations ----
    void loadReservations() {
        reservations.clear();
        std::vector<std::vector<std::string>> r;
        if (!rows(path("reservations.csv"), r)) return;
        for (auto& x : r) {
            Reservation v;
            v.id = static_cast<int>(ll(f(x, 0))); v.bookId = static_cast<int>(ll(f(x, 1))); v.memberId = f(x, 2);
            v.date = f(x, 3); v.status = f(x, 4);
            reservations.push_back(v);
            nextReservationId = std::max(nextReservationId, v.id + 1);
        }
    }
    void saveReservations() {
        std::ofstream o(path("reservations.csv"));
        o << "id,bookId,memberId,date,status\n";
        for (auto& v : reservations) o << v.id << ',' << v.bookId << ',' << v.memberId << ',' << v.date << ',' << v.status << '\n';
    }

    // ---- settings (simulated date) ----
    void loadSettings() {
        simulatedDate.clear();
        std::ifstream in(path("settings.txt"));
        std::string ln;
        if (in && std::getline(in, ln)) {
            ln = util::trim(ln);
            if (util::dates::parse(ln)) simulatedDate = ln;
        }
    }
    void saveSettings() {
        std::ofstream o(path("settings.txt"));
        o << simulatedDate << '\n';
    }
};

#endif
