// Library Management System - Sqrock IT Solutions Internship, Project Phase 1, Task 4
// Concepts: OOP (classes, encapsulation, service layer), STL (vector, map, sort), file handling (CSV),
// date arithmetic, input validation, hashed passwords.

#include "admin_ui.h"
#include "database.h"
#include "library.h"
#include "member_ui.h"

int main() {
    Database db("data");
    db.load();
    Library lib(db);
    MemberUI members(db, lib);
    AdminUI admin(db, lib, members);

    try {
        while (true) {
            util::title("LIBRARY MANAGEMENT SYSTEM");
            std::cout << "  1. Member login\n  2. Register as a member\n  3. Search books (no login)\n  4. Librarian / admin login\n  5. Exit\n";
            int c = util::readInt("\n  Choose: ", 1, 5);
            switch (c) {
                case 1:
                    if (Member* m = members.login()) members.menu(*m);
                    break;
                case 2: members.registerSelf(); break;
                case 3: members.searchMenu(); break;
                case 4:
                    if (AdminUI::login()) admin.menu();
                    break;
                case 5:
                    db.save();
                    std::cout << "\n  Thank you for using the Library Management System. Goodbye!\n";
                    return 0;
            }
        }
    } catch (const EndOfInput&) {
        db.save();
        std::cout << "\n  (Input closed - data saved. Goodbye!)\n";
    }
    return 0;
}
