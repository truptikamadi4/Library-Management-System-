// library.h - Core library rules: books, members, issue/return, fines, reservations, search
#ifndef LIBRARY_H
#define LIBRARY_H

#include <algorithm>

#include "database.h"
#include "utils.h"

struct Result {
    bool ok = false;
    std::string msg;
};

class Library {
public:
    static constexpr int LOAN_DAYS = 14;         // books are issued for 14 days
    static constexpr int MAX_BOOKS = 3;          // per member
    static constexpr long long FINE_PER_DAY = 200;  // Rs. 2.00 per overdue day (paise)

    explicit Library(Database& db) : db_(db) {}

    // ------------------------------------------------------------------ members
    Member& registerMember(const std::string& name, const std::string& contact, const std::string& password) {
        Member m;
        m.id = "M" + std::to_string(db_.nextMemberNo++);
        m.name = name;
        m.contact = contact;
        m.passHash = util::hashPassword(password);
        m.joinedAt = db_.today();
        db_.members.push_back(m);
        db_.save();
        return db_.members.back();
    }

    int issuedCount(const std::string& memberId) const {
        int n = 0;
        for (auto& i : db_.issues)
            if (i.memberId == memberId && !i.returned()) n++;
        return n;
    }

    Result deleteMember(const std::string& memberId) {
        Member* m = db_.findMember(memberId);
        if (!m) return fail("Member not found.");
        if (issuedCount(m->id) > 0) return fail("Member still has issued books. Return them first.");
        if (unpaidSettledFines(m->id) > 0 || accruedFines(m->id) > 0) return fail("Member has unpaid fines.");
        for (auto& r : db_.reservations)
            if (r.memberId == m->id && (r.status == "Waiting" || r.status == "Ready")) r.status = "Cancelled";
        std::string name = m->name;
        for (size_t i = 0; i < db_.members.size(); i++)
            if (db_.members[i].id == memberId || util::toLower(db_.members[i].id) == util::toLower(memberId)) {
                db_.members.erase(db_.members.begin() + i);
                break;
            }
        for (auto& b : db_.books) promote(b.id);
        db_.save();
        return ok("Member " + name + " deleted.");
    }

    // ------------------------------------------------------------------ books
    Book& addBook(const std::string& title, const std::string& author, const std::string& category, int qty) {
        Book b;
        b.id = db_.nextBookId++;
        b.title = title;
        b.author = author;
        b.category = category;
        b.totalQty = b.availableQty = qty;
        db_.books.push_back(b);
        db_.save();
        return db_.books.back();
    }

    int copiesOut(const Book& b) const { return b.totalQty - b.availableQty; }

    Result updateBook(int id, const std::string& title, const std::string& author, const std::string& category, int newTotal) {
        Book* b = db_.findBook(id);
        if (!b) return fail("Book not found.");
        int out = copiesOut(*b);
        if (newTotal < out) return fail("Total copies cannot be less than the " + std::to_string(out) + " copies currently issued.");
        if (!title.empty()) b->title = title;
        if (!author.empty()) b->author = author;
        if (!category.empty()) b->category = category;
        b->totalQty = newTotal;
        b->availableQty = newTotal - out;
        promote(id);
        db_.save();
        return ok("Book updated.");
    }

    Result deleteBook(int id) {
        Book* b = db_.findBook(id);
        if (!b) return fail("Book not found.");
        if (copiesOut(*b) > 0) return fail(std::to_string(copiesOut(*b)) + " copy/copies are still issued. Delete is allowed only after they are returned.");
        for (auto& r : db_.reservations)
            if (r.bookId == id && (r.status == "Waiting" || r.status == "Ready")) r.status = "Cancelled";
        std::string t = b->title;
        for (size_t i = 0; i < db_.books.size(); i++)
            if (db_.books[i].id == id) { db_.books.erase(db_.books.begin() + i); break; }
        db_.save();
        return ok("Book \"" + t + "\" deleted.");
    }

    // ------------------------------------------------------------------ search
    enum class Field { Title, Author, Category };

    std::vector<const Book*> search(Field f, const std::string& query) const {
        std::vector<const Book*> out;
        std::string q = util::toLower(query);
        for (auto& b : db_.books) {
            const std::string& hay = f == Field::Title ? b.title : (f == Field::Author ? b.author : b.category);
            if (util::toLower(hay).find(q) != std::string::npos) out.push_back(&b);
        }
        return out;
    }

    std::vector<const Book*> availableBooks() const {
        std::vector<const Book*> out;
        for (auto& b : db_.books)
            if (b.availableQty > 0) out.push_back(&b);
        return out;
    }

    std::vector<const Book*> allBooksSorted() const {
        std::vector<const Book*> out;
        for (auto& b : db_.books) out.push_back(&b);
        std::sort(out.begin(), out.end(), [](const Book* a, const Book* b) { return util::toLower(a->title) < util::toLower(b->title); });
        return out;
    }

    // ------------------------------------------------------------------ issue / return
    // copies a given member could actually take right now (copies held for other members' reservations are excluded)
    int copiesFor(const Book& b, const std::string& memberId) const {
        int heldForOthers = 0;
        for (auto& r : db_.reservations)
            if (r.bookId == b.id && r.status == "Ready" && util::toLower(r.memberId) != util::toLower(memberId)) heldForOthers++;
        return b.availableQty - heldForOthers;
    }

    Result issueBook(const std::string& memberId, int bookId) {
        Member* m = db_.findMember(memberId);
        if (!m) return fail("Member not found.");
        Book* b = db_.findBook(bookId);
        if (!b) return fail("Book ID " + std::to_string(bookId) + " not found.");
        if (unpaidSettledFines(m->id) > 0)
            return fail("Member has unpaid fines of " + util::rs(unpaidSettledFines(m->id)) + ". Clear them before issuing new books.");
        if (issuedCount(m->id) >= MAX_BOOKS) return fail("Limit reached: a member can hold at most " + std::to_string(MAX_BOOKS) + " books.");
        for (auto& i : db_.issues)
            if (i.memberId == m->id && i.bookId == bookId && !i.returned()) return fail("This member already has a copy of this book.");
        if (copiesFor(*b, m->id) <= 0) {
            if (b->availableQty > 0) return fail("The remaining copy is on hold for another member's reservation.");
            return fail("No copies available right now. You can reserve this book.");
        }

        for (auto& r : db_.reservations)
            if (r.bookId == bookId && r.memberId == m->id && (r.status == "Waiting" || r.status == "Ready")) r.status = "Fulfilled";

        IssueRecord rec;
        rec.id = db_.nextIssueId++;
        rec.bookId = bookId;
        rec.memberId = m->id;
        rec.issueDate = db_.today();
        rec.dueDate = util::dates::fromDays(util::dates::days(rec.issueDate) + LOAN_DAYS);
        db_.issues.push_back(rec);
        b->availableQty--;
        db_.save();
        return ok("Issued \"" + b->title + "\" to " + m->name + ". Due date: " + rec.dueDate + ". Stock left: " + std::to_string(b->availableQty) + ".");
    }

    long long overdueDays(const IssueRecord& i) const {
        std::string end = i.returned() ? i.returnDate : db_.today();
        long long d = util::dates::days(end) - util::dates::days(i.dueDate);
        return d > 0 ? d : 0;
    }

    long long fineFor(const IssueRecord& i) const { return i.returned() ? i.fine : overdueDays(i) * FINE_PER_DAY; }

    // collectNow = fine is paid at the counter while returning
    Result returnBook(int issueId, bool collectNow) {
        IssueRecord* i = db_.findIssue(issueId);
        if (!i) return fail("Issue record not found.");
        if (i->returned()) return fail("This book has already been returned.");
        Book* b = db_.findBook(i->bookId);
        i->returnDate = db_.today();
        i->fine = overdueDays(*i) * FINE_PER_DAY;
        i->finePaid = i->fine == 0 || collectNow;
        if (b) {
            b->availableQty++;
            promote(b->id);
        }
        db_.save();
        std::string msg = "Returned \"" + (b ? b->title : std::string("(deleted book)")) + "\".";
        if (i->fine > 0)
            msg += " Late by " + std::to_string(overdueDays(*i)) + " day(s) - fine " + util::rs(i->fine) + (i->finePaid ? " (collected)." : " (UNPAID - must be cleared before the next issue).");
        else
            msg += " Returned on time - no fine.";
        return ok(msg);
    }

    // ------------------------------------------------------------------ fines
    long long unpaidSettledFines(const std::string& memberId) const {
        long long s = 0;
        for (auto& i : db_.issues)
            if (i.memberId == memberId && i.returned() && i.fine > 0 && !i.finePaid) s += i.fine;
        return s;
    }

    long long accruedFines(const std::string& memberId) const {
        long long s = 0;
        for (auto& i : db_.issues)
            if (i.memberId == memberId && !i.returned()) s += fineFor(i);
        return s;
    }

    long long payFines(const std::string& memberId) {
        long long s = 0;
        for (auto& i : db_.issues)
            if (i.memberId == memberId && i.returned() && i.fine > 0 && !i.finePaid) {
                i.finePaid = true;
                s += i.fine;
            }
        db_.save();
        return s;
    }

    // ------------------------------------------------------------------ reservations
    Result reserve(const std::string& memberId, int bookId) {
        Member* m = db_.findMember(memberId);
        Book* b = db_.findBook(bookId);
        if (!m || !b) return fail("Member or book not found.");
        for (auto& i : db_.issues)
            if (i.memberId == m->id && i.bookId == bookId && !i.returned()) return fail("You already have this book issued.");
        for (auto& r : db_.reservations)
            if (r.bookId == bookId && r.memberId == m->id && (r.status == "Waiting" || r.status == "Ready"))
                return fail("You already have an active reservation for this book.");
        if (copiesFor(*b, m->id) > 0) return fail("Copies are available - you can issue this book directly.");
        Reservation r;
        r.id = db_.nextReservationId++;
        r.bookId = bookId;
        r.memberId = m->id;
        r.date = db_.today();
        db_.reservations.push_back(r);
        db_.save();
        return ok("Reserved \"" + b->title + "\". You will be notified (queue position " + std::to_string(queuePosition(r)) + ").");
    }

    int queuePosition(const Reservation& r) const {
        int pos = 0;
        for (auto& x : db_.reservations)
            if (x.bookId == r.bookId && (x.status == "Waiting" || x.status == "Ready")) {
                pos++;
                if (x.id == r.id) break;
            }
        return pos;
    }

    Result cancelReservation(int id, const std::string& memberId) {
        for (auto& r : db_.reservations)
            if (r.id == id && util::toLower(r.memberId) == util::toLower(memberId) && (r.status == "Waiting" || r.status == "Ready")) {
                r.status = "Cancelled";
                promote(r.bookId);
                db_.save();
                return ok("Reservation #" + std::to_string(id) + " cancelled.");
            }
        return fail("No active reservation with that ID.");
    }

    // Marks waiting reservations as Ready when copies are free
    void promote(int bookId) {
        Book* b = db_.findBook(bookId);
        if (!b) return;
        int ready = 0;
        for (auto& r : db_.reservations)
            if (r.bookId == bookId && r.status == "Ready") ready++;
        int free = b->availableQty - ready;
        for (auto& r : db_.reservations) {
            if (free <= 0) break;
            if (r.bookId == bookId && r.status == "Waiting") {
                r.status = "Ready";
                free--;
            }
        }
    }

    // ------------------------------------------------------------------ queries
    std::vector<IssueRecord*> activeIssuesOf(const std::string& memberId) {
        std::vector<IssueRecord*> out;
        for (auto& i : db_.issues)
            if (util::toLower(i.memberId) == util::toLower(memberId) && !i.returned()) out.push_back(&i);
        return out;
    }

    std::vector<const IssueRecord*> overdueList() const {
        std::vector<const IssueRecord*> out;
        for (auto& i : db_.issues)
            if (!i.returned() && overdueDays(i) > 0) out.push_back(&i);
        return out;
    }

private:
    Database& db_;
    static Result fail(const std::string& m) { return {false, m}; }
    static Result ok(const std::string& m) { return {true, m}; }
};

#endif
