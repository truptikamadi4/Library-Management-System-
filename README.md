# Library Management System (C++)

Console-based library system built for **Sqrock IT Solutions - Internship Project Phase 1, Task 4**.
The librarian manages books and members; members search, issue, return and reserve books.
Stock is updated automatically, due dates are tracked and late returns are fined.

## Build and run

Requires a C++17 compiler (g++ / clang++ / MinGW).

```bash
make run          # builds and starts the app
# or manually
g++ -std=c++17 -O2 -o library main.cpp
./library
```

Data is created automatically in `data/` (CSV files). 12 sample books are added on the first run. Delete `data/` to reset.

### Logins

| Role | Login | Password |
|------|-------|----------|
| Member | member ID (`M1001`, `M1002`...) - register from the main menu or ask the librarian | your choice |
| Librarian / admin | `admin` | `admin123` |

## Features

**Required**
- **Book management**: add, update, delete, view all books (ID, title, author, category, quantity)
- **Member management**: register, update, delete, view (ID, name, contact, issued books count)
- **Issue and return**: stock changes automatically, issued books are tracked, issue / due / return dates are recorded, check available stock
- **Search**: by title, author or category (case-insensitive, partial match), list available books
- **Fine calculation**: Rs. 2 per overdue day, overdue list, fines accrue on unreturned books

**Rules enforced**
- Loan period 14 days, maximum 3 books per member, no duplicate copy of the same book
- A member with unpaid fines cannot borrow until they pay
- A book cannot be deleted while copies are issued; total copies cannot drop below copies issued; a member cannot be deleted with books or fines outstanding

**Bonus (all implemented except GUI)**
- Login authentication (members and librarian, hashed passwords)
- Admin dashboard: titles / copies / on shelf, members, issued and overdue books, fines collected and pending, most issued books
- Fine management: pay at the counter or from the member menu
- Book reservation queue: if no copy is free a member reserves it; when a copy is returned the first waiting member is notified and the copy is held for them
- Report generation (`report.txt`)
- "Simulated date" in the admin menu so overdue fines can be demonstrated without waiting 14 days

## Project structure

| File | Module |
|------|--------|
| `main.cpp` | Top-level menu |
| `utils.h` | Validation, input helpers, date arithmetic, hashing |
| `models.h` | Entities: `Book`, `Member`, `IssueRecord`, `Reservation` |
| `database.h` | **File Handling Module** (CSV load / save, sample data, simulated clock) |
| `library.h` | **Book, Member, Issue/Return and Search logic** in the `Library` class |
| `printer.h` | Table printing helpers |
| `member_ui.h` | Member registration / login and self-service menu |
| `admin_ui.h` | Librarian menus and dashboard |

## C++ concepts used

OOP (classes, encapsulation, service layer separate from UI), STL (`vector`, `map`, `sort` with lambdas, `string`, `pair`), file handling (CSV with `ifstream` / `ofstream`), date arithmetic (day-number conversion, no external library), pointers and references, input validation, constants (`constexpr`).

## Data files

`data/books.csv`, `data/members.csv`, `data/issues.csv`, `data/reservations.csv`, `data/settings.txt`, `report.txt`.

## Sample run

`sample_output.txt` shows registration, issuing up to the 3-book limit, a reservation being queued and then fulfilled, jumping the date forward to create overdue books, fine collection, the block on borrowing with unpaid fines, and the dashboard.

## Possible improvements

Book renewal, barcode / ISBN lookup, email reminders, SQL database, GUI (Qt / wxWidgets).
