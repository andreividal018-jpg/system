#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <cmath>
#include <string>
#include <vector>
#include "sqlite3.h"

// ══════════════════════════════════════════════
//  DATABASE
// ══════════════════════════════════════════════
class Database {
public:
    Database() {}
    ~Database() { close(); }

    bool open(const std::string& path) {
        int rc = sqlite3_open_v2(path.c_str(), &db_,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << "\n";
            sqlite3_close(db_); db_ = nullptr; return false;
        }
        return true;
    }

    void close() { if (db_) { sqlite3_close(db_); db_ = nullptr; } }

    bool exec(const std::string& sql) {
        char* errmsg = nullptr;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL Error: " << errmsg << "\n";
            sqlite3_free(errmsg); return false;
        }
        return true;
    }

    sqlite3* get() const { return db_; }
    std::string lastError() const { return db_ ? sqlite3_errmsg(db_) : "No database open"; }

private:
    sqlite3* db_ = nullptr;
};

// ══════════════════════════════════════════════
//  UI
// ══════════════════════════════════════════════
namespace UI {

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printLine(char c = '-', int width = 72) {
    std::cout << std::string(width, c) << "\n";
}

void printBanner() {
    clearScreen();
    printLine('=');
    std::cout << std::setw(50) << std::right << "N003 BSIT TRACK\n";
    std::cout << std::setw(60) << std::right
              << "Student Attendance & Grade Management System\n";
    printLine('=');
}

void printHeader(const std::string& title) {
    printLine('-');
    int pad = (int)(72 - title.size()) / 2;
    std::cout << std::string(pad > 0 ? pad : 0, ' ') << title << "\n";
    printLine('-');
}

void pause() {
    std::cout << "\n  Press ENTER to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string inputString(const std::string& prompt, bool allowEmpty = false) {
    std::string val;
    while (true) {
        std::cout << prompt << ": ";
        std::getline(std::cin, val);
        val.erase(0, val.find_first_not_of(" \t\r\n"));
        val.erase(val.find_last_not_of(" \t\r\n") + 1);
        if (!val.empty() || allowEmpty) return val;
        std::cout << "  Input cannot be empty.\n";
    }
}

int inputInt(const std::string& prompt, int min = 0, int max = 9999) {
    while (true) {
        std::cout << prompt << " (" << min << "-" << max << "): ";
        std::string line; std::getline(std::cin, line);
        try { int v = std::stoi(line); if (v >= min && v <= max) return v; } catch (...) {}
        std::cout << "  Invalid input. Enter a number between " << min << " and " << max << ".\n";
    }
}

double inputDouble(const std::string& prompt, double min = 0.0, double max = 100.0) {
    while (true) {
        std::cout << prompt << " (" << min << "-" << max << "): ";
        std::string line; std::getline(std::cin, line);
        try { double v = std::stod(line); if (v >= min && v <= max) return v; } catch (...) {}
        std::cout << "  Invalid input.\n";
    }
}

std::string inputDate(const std::string& prompt) {
    while (true) {
        std::cout << prompt << " (YYYY-MM-DD): ";
        std::string val; std::getline(std::cin, val);
        if (val.size() == 10 && val[4] == '-' && val[7] == '-') return val;
        std::cout << "  Invalid format. Use YYYY-MM-DD.\n";
    }
}

std::string inputChoice(const std::string& prompt, const std::vector<std::string>& choices) {
    while (true) {
        std::cout << prompt << " [";
        for (int i = 0; i < (int)choices.size(); ++i) { if (i) std::cout << "/"; std::cout << choices[i]; }
        std::cout << "]: ";
        std::string val; std::getline(std::cin, val);
        for (auto& c : choices) {
            std::string lv = val, lc = c;
            std::transform(lv.begin(), lv.end(), lv.begin(), ::tolower);
            std::transform(lc.begin(), lc.end(), lc.begin(), ::tolower);
            if (lv == lc) return c;
        }
        std::cout << "  Please enter one of the listed options.\n";
    }
}

int menu(const std::string& title, const std::vector<std::string>& options) {
    printBanner();
    printHeader(title);
    for (int i = 0; i < (int)options.size(); ++i)
        std::cout << "  [" << (i + 1) << "] " << options[i] << "\n";
    std::cout << "  [0] Back / Exit\n";
    printLine();
    return inputInt("  Choice", 0, (int)options.size());
}

void success(const std::string& msg) { std::cout << "\n  [OK] "  << msg << "\n"; }
void error  (const std::string& msg) { std::cout << "\n  [ERR] " << msg << "\n"; }
void info   (const std::string& msg) { std::cout << "\n  [i] "   << msg << "\n"; }

void tableRow(const std::vector<std::string>& cells, const std::vector<int>& widths) {
    std::cout << "  |";
    for (int i = 0; i < (int)cells.size() && i < (int)widths.size(); ++i) {
        std::string cell = cells[i];
        if ((int)cell.size() > widths[i] - 1) cell = cell.substr(0, widths[i] - 2) + ".";
        std::cout << " " << std::left << std::setw(widths[i]) << cell << "|";
    }
    std::cout << "\n";
}

void tableHeader(const std::vector<std::string>& headers, const std::vector<int>& widths) {
    std::cout << "  +";
    for (int w : widths) std::cout << std::string(w + 2, '-') << "+";
    std::cout << "\n";
    tableRow(headers, widths);
    std::cout << "  +";
    for (int w : widths) std::cout << std::string(w + 2, '-') << "+";
    std::cout << "\n";
}

} // namespace UI

// ══════════════════════════════════════════════
//  MODELS
// ══════════════════════════════════════════════
struct Student {
    int id = 0;
    std::string studentNo, lastName, firstName, middleName;
    std::string section = "N003", track = "BSIT";
    std::string fullName() const { return lastName + ", " + firstName + " " + middleName; }
};

struct Subject {
    int id = 0; std::string code, name; int units = 3; std::string semester;
};

struct AttendanceSummary {
    std::string studentNo, studentName;
    int present = 0, absent = 0, late = 0, excused = 0, total = 0;
};

struct GradeRecord {
    int id = 0, studentId = 0, subjectId = 0;
    double prelim = 0, midterm = 0, prefinal = 0, finalScore = 0, finalGrade = 0;
    std::string remarks;
};

struct UserSession {
    int id = 0;
    std::string username, email, role;
    int studentId = 0;
    bool loggedIn = false;
};

// ══════════════════════════════════════════════
//  STUDENT MANAGER
// ══════════════════════════════════════════════
class StudentManager {
public:
    explicit StudentManager(Database& db) : db_(db) {}

    void initTable() {
        db_.exec(R"(
            CREATE TABLE IF NOT EXISTS students (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                student_no  TEXT NOT NULL UNIQUE,
                last_name   TEXT NOT NULL,
                first_name  TEXT NOT NULL,
                middle_name TEXT DEFAULT '',
                section     TEXT DEFAULT 'N003',
                track       TEXT DEFAULT 'BSIT'
            );
        )");
    }

    void addStudent() {
        UI::printBanner(); UI::printHeader("ADD NEW STUDENT");
        Student s;
        s.studentNo  = UI::inputString("  Student No");
        s.lastName   = UI::inputString("  Last Name");
        s.firstName  = UI::inputString("  First Name");
        s.middleName = UI::inputString("  Middle Name", true);
        std::string sql =
            "INSERT INTO students (student_no, last_name, first_name, middle_name, section, track) "
            "VALUES ('" + s.studentNo + "','" + s.lastName + "','" +
            s.firstName + "','" + s.middleName + "','N003','BSIT');";
        if (db_.exec(sql)) UI::success("Student added: " + s.fullName());
        else UI::error("Failed to add student. Student No may already exist.");
        UI::pause();
    }

    void editStudent() {
        UI::printBanner(); UI::printHeader("EDIT STUDENT");
        int sid = pickStudent(); if (sid <= 0) return;
        Student s = getStudentById(sid);
        std::cout << "  Editing: " << s.fullName() << " (" << s.studentNo << ")\n";
        std::cout << "  (Press ENTER to keep current value)\n\n";
        auto prompt = [](const std::string& field, const std::string& cur) -> std::string {
            std::cout << "  " << field << " [" << cur << "]: ";
            std::string v; std::getline(std::cin, v);
            v.erase(0, v.find_first_not_of(" \t")); v.erase(v.find_last_not_of(" \t\r\n") + 1);
            return v.empty() ? cur : v;
        };
        s.lastName   = prompt("Last Name",   s.lastName);
        s.firstName  = prompt("First Name",  s.firstName);
        s.middleName = prompt("Middle Name", s.middleName);
        std::string sql =
            "UPDATE students SET last_name='" + s.lastName +
            "', first_name='" + s.firstName +
            "', middle_name='" + s.middleName +
            "' WHERE id=" + std::to_string(sid) + ";";
        if (db_.exec(sql)) UI::success("Student updated."); else UI::error("Update failed.");
        UI::pause();
    }

    void deleteStudent() {
        UI::printBanner(); UI::printHeader("DELETE STUDENT");
        int sid = pickStudent(); if (sid <= 0) return;
        Student s = getStudentById(sid);
        std::cout << "\n  Are you sure you want to delete: " << s.fullName()
                  << "? This will remove ALL attendance and grade records.\n";
        std::string confirm = UI::inputChoice("  Confirm", {"Yes","No"});
        if (confirm != "Yes") { UI::info("Cancelled."); UI::pause(); return; }
        db_.exec("DELETE FROM attendance WHERE student_id=" + std::to_string(sid) + ";");
        db_.exec("DELETE FROM grades WHERE student_id=" + std::to_string(sid) + ";");
        if (db_.exec("DELETE FROM students WHERE id=" + std::to_string(sid) + ";"))
            UI::success("Student deleted."); else UI::error("Delete failed.");
        UI::pause();
    }

    void listStudents() {
        UI::printBanner(); UI::printHeader("STUDENT LIST - SECTION N003 BSIT");
        auto students = getAllStudents();
        if (students.empty()) { UI::info("No students enrolled yet."); UI::pause(); return; }
        printStudentTable(students);
        std::cout << "\n  Total: " << students.size() << " student(s)\n";
        UI::pause();
    }

    void searchStudent() {
        UI::printBanner(); UI::printHeader("SEARCH STUDENT");
        std::string kw = UI::inputString("  Keyword (name or student no)");
        std::string sql =
            "SELECT id, student_no, last_name, first_name, middle_name, section, track "
            "FROM students WHERE student_no LIKE '%" + kw + "%' "
            "OR last_name LIKE '%" + kw + "%' "
            "OR first_name LIKE '%" + kw + "%' ORDER BY last_name, first_name;";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr);
        std::vector<Student> results;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Student s;
            s.id         = sqlite3_column_int(stmt, 0);
            s.studentNo  = (const char*)sqlite3_column_text(stmt, 1);
            s.lastName   = (const char*)sqlite3_column_text(stmt, 2);
            s.firstName  = (const char*)sqlite3_column_text(stmt, 3);
            s.middleName = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
            results.push_back(s);
        }
        sqlite3_finalize(stmt);
        if (results.empty()) UI::info("No results found."); else printStudentTable(results);
        UI::pause();
    }

    std::vector<Student> getAllStudents() {
        std::vector<Student> list;
        const char* sql =
            "SELECT id, student_no, last_name, first_name, middle_name, section, track "
            "FROM students ORDER BY last_name, first_name;";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql, -1, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Student s;
            s.id         = sqlite3_column_int(stmt, 0);
            s.studentNo  = (const char*)sqlite3_column_text(stmt, 1);
            s.lastName   = (const char*)sqlite3_column_text(stmt, 2);
            s.firstName  = (const char*)sqlite3_column_text(stmt, 3);
            s.middleName = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
            list.push_back(s);
        }
        sqlite3_finalize(stmt);
        return list;
    }

    Student getStudentById(int id) {
        Student s;
        std::string sql =
            "SELECT id, student_no, last_name, first_name, middle_name FROM students WHERE id=" +
            std::to_string(id) + ";";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            s.id         = sqlite3_column_int(stmt, 0);
            s.studentNo  = (const char*)sqlite3_column_text(stmt, 1);
            s.lastName   = (const char*)sqlite3_column_text(stmt, 2);
            s.firstName  = (const char*)sqlite3_column_text(stmt, 3);
            s.middleName = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        }
        sqlite3_finalize(stmt);
        return s;
    }

    int pickStudent(const std::string& prompt = "Select Student") {
        auto students = getAllStudents();
        if (students.empty()) { UI::info("No students found."); return 0; }
        printStudentTable(students);
        std::cout << "\n";
        int choice = UI::inputInt("  " + prompt + " (Enter ID, 0 to cancel)", 0, 99999);
        if (choice == 0) return 0;
        for (auto& s : students) if (s.id == choice) return choice;
        UI::error("Student ID not found."); return 0;
    }

private:
    Database& db_;

    void printStudentTable(const std::vector<Student>& students) {
        std::vector<std::string> headers = {"ID","Student No","Name","Section","Track"};
        std::vector<int> widths = {5, 14, 28, 8, 6};
        UI::tableHeader(headers, widths);
        for (auto& s : students)
            UI::tableRow({std::to_string(s.id), s.studentNo, s.fullName(), s.section, s.track}, widths);
        std::cout << "  +";
        for (int w : widths) std::cout << std::string(w + 2, '-') << "+";
        std::cout << "\n";
    }
};

// ══════════════════════════════════════════════
//  SUBJECT MANAGER
// ══════════════════════════════════════════════
class SubjectManager {
public:
    explicit SubjectManager(Database& db) : db_(db) {}

    void initTable() {
        db_.exec(R"(
            CREATE TABLE IF NOT EXISTS subjects (
                id        INTEGER PRIMARY KEY AUTOINCREMENT,
                code      TEXT NOT NULL UNIQUE,
                name      TEXT NOT NULL,
                units     INTEGER DEFAULT 3,
                semester  TEXT DEFAULT '1st 2024-2025'
            );
        )");
    }

    void addSubject() {
        UI::printBanner(); UI::printHeader("ADD SUBJECT");
        Subject s;
        s.code     = UI::inputString("  Subject Code (e.g. IT101)");
        s.name     = UI::inputString("  Subject Name");
        s.units    = UI::inputInt("  Units", 1, 6);
        s.semester = UI::inputString("  Semester (e.g. 1st 2024-2025)");
        std::string sql =
            "INSERT INTO subjects (code, name, units, semester) VALUES ('" +
            s.code + "','" + s.name + "'," + std::to_string(s.units) + ",'" + s.semester + "');";
        if (db_.exec(sql)) UI::success("Subject added: " + s.code + " - " + s.name);
        else UI::error("Failed. Code may already exist.");
        UI::pause();
    }

    void editSubject() {
        UI::printBanner(); UI::printHeader("EDIT SUBJECT");
        int sid = pickSubject(); if (sid <= 0) return;
        Subject s = getSubjectById(sid);
        auto prompt = [](const std::string& field, const std::string& cur) -> std::string {
            std::cout << "  " << field << " [" << cur << "]: ";
            std::string v; std::getline(std::cin, v);
            v.erase(0, v.find_first_not_of(" \t")); v.erase(v.find_last_not_of(" \t\r\n") + 1);
            return v.empty() ? cur : v;
        };
        s.name     = prompt("Subject Name", s.name);
        s.semester = prompt("Semester",     s.semester);
        std::cout << "  Units [" << s.units << "] (0 to keep): ";
        std::string uline; std::getline(std::cin, uline);
        if (!uline.empty()) try { int u = std::stoi(uline); if (u >= 1 && u <= 6) s.units = u; } catch(...) {}
        std::string sql =
            "UPDATE subjects SET name='" + s.name + "', units=" + std::to_string(s.units) +
            ", semester='" + s.semester + "' WHERE id=" + std::to_string(sid) + ";";
        if (db_.exec(sql)) UI::success("Subject updated."); else UI::error("Update failed.");
        UI::pause();
    }

    void deleteSubject() {
        UI::printBanner(); UI::printHeader("DELETE SUBJECT");
        int sid = pickSubject(); if (sid <= 0) return;
        Subject s = getSubjectById(sid);
        std::cout << "\n  Delete subject: " << s.code << " - " << s.name
                  << "? This removes all related records.\n";
        std::string confirm = UI::inputChoice("  Confirm", {"Yes","No"});
        if (confirm != "Yes") { UI::info("Cancelled."); UI::pause(); return; }
        db_.exec("DELETE FROM attendance WHERE subject_id=" + std::to_string(sid) + ";");
        db_.exec("DELETE FROM grades WHERE subject_id=" + std::to_string(sid) + ";");
        if (db_.exec("DELETE FROM subjects WHERE id=" + std::to_string(sid) + ";"))
            UI::success("Subject deleted."); else UI::error("Delete failed.");
        UI::pause();
    }

    void listSubjects() {
        UI::printBanner(); UI::printHeader("SUBJECT LIST");
        auto subjects = getAllSubjects();
        if (subjects.empty()) { UI::info("No subjects yet."); UI::pause(); return; }
        printSubjectTable(subjects);
        UI::pause();
    }

    std::vector<Subject> getAllSubjects() {
        std::vector<Subject> list;
        const char* sql = "SELECT id, code, name, units, semester FROM subjects ORDER BY code;";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql, -1, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Subject s;
            s.id       = sqlite3_column_int(stmt, 0);
            s.code     = (const char*)sqlite3_column_text(stmt, 1);
            s.name     = (const char*)sqlite3_column_text(stmt, 2);
            s.units    = sqlite3_column_int(stmt, 3);
            s.semester = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
            list.push_back(s);
        }
        sqlite3_finalize(stmt);
        return list;
    }

    Subject getSubjectById(int id) {
        Subject s;
        std::string sql = "SELECT id, code, name, units, semester FROM subjects WHERE id=" +
                          std::to_string(id) + ";";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            s.id       = sqlite3_column_int(stmt, 0);
            s.code     = (const char*)sqlite3_column_text(stmt, 1);
            s.name     = (const char*)sqlite3_column_text(stmt, 2);
            s.units    = sqlite3_column_int(stmt, 3);
            s.semester = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        }
        sqlite3_finalize(stmt);
        return s;
    }

    int pickSubject(const std::string& prompt = "Select Subject") {
        auto subjects = getAllSubjects();
        if (subjects.empty()) { UI::info("No subjects found."); return 0; }
        printSubjectTable(subjects);
        std::cout << "\n";
        int choice = UI::inputInt("  " + prompt + " (Enter ID, 0 to cancel)", 0, 99999);
        if (choice == 0) return 0;
        for (auto& s : subjects) if (s.id == choice) return choice;
        UI::error("Subject ID not found."); return 0;
    }

private:
    Database& db_;

    void printSubjectTable(const std::vector<Subject>& subjects) {
        std::vector<std::string> headers = {"ID","Code","Subject Name","Units","Semester"};
        std::vector<int> widths = {4, 8, 30, 5, 16};
        UI::tableHeader(headers, widths);
        for (auto& s : subjects)
            UI::tableRow({std::to_string(s.id), s.code, s.name, std::to_string(s.units), s.semester}, widths);
        std::cout << "  +";
        for (int w : widths) std::cout << std::string(w + 2, '-') << "+";
        std::cout << "\n";
    }
};

// ══════════════════════════════════════════════
//  ATTENDANCE MANAGER
// ══════════════════════════════════════════════
class AttendanceManager {
public:
    AttendanceManager(Database& db, StudentManager& sm, SubjectManager& subm)
        : db_(db), sm_(sm), subm_(subm) {}

    void initTable() {
        db_.exec(R"(
            CREATE TABLE IF NOT EXISTS attendance (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                student_id  INTEGER NOT NULL,
                subject_id  INTEGER NOT NULL,
                date        TEXT NOT NULL,
                status      TEXT NOT NULL DEFAULT 'Present',
                remarks     TEXT DEFAULT '',
                FOREIGN KEY(student_id) REFERENCES students(id),
                FOREIGN KEY(subject_id) REFERENCES subjects(id),
                UNIQUE(student_id, subject_id, date)
            );
        )");
    }

    void takeAttendance() {
        UI::printBanner(); UI::printHeader("TAKE ATTENDANCE");
        int subjId = subm_.pickSubject("Select Subject for Attendance"); if (subjId <= 0) return;
        Subject subj = subm_.getSubjectById(subjId);
        std::string date = UI::inputDate("  Date");
        auto students = sm_.getAllStudents();
        if (students.empty()) { UI::info("No students enrolled."); UI::pause(); return; }
        UI::printLine();
        std::cout << "  Subject : " << subj.code << " - " << subj.name << "\n";
        std::cout << "  Date    : " << date << "\n";
        UI::printLine();
        std::cout << "  Status codes: P=Present  A=Absent  L=Late  E=Excused\n\n";
        db_.exec("BEGIN;");
        for (auto& s : students) {
            std::cout << "  " << std::left << std::setw(12) << s.studentNo
                      << " " << std::setw(28) << s.fullName();
            std::string status = UI::inputChoice("", {"P","A","L","E"});
            std::string sf = (status=="P") ? "Present" : (status=="A") ? "Absent" : (status=="L") ? "Late" : "Excused";
            db_.exec("INSERT OR REPLACE INTO attendance (student_id, subject_id, date, status) VALUES (" +
                std::to_string(s.id) + "," + std::to_string(subjId) + ",'" + date + "','" + sf + "');");
        }
        db_.exec("COMMIT;");
        UI::success("Attendance saved for " + date + "."); UI::pause();
    }

    void editAttendance() {
        UI::printBanner(); UI::printHeader("EDIT ATTENDANCE RECORD");
        int subjId = subm_.pickSubject("Select Subject"); if (subjId <= 0) return;
        int studId = sm_.pickStudent("Select Student"); if (studId <= 0) return;
        std::string date = UI::inputDate("  Date to edit");
        std::string qsql =
            "SELECT id, status FROM attendance WHERE student_id=" + std::to_string(studId) +
            " AND subject_id=" + std::to_string(subjId) + " AND date='" + date + "';";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), qsql.c_str(), -1, &stmt, nullptr);
        int recId = 0; std::string curStatus;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            recId = sqlite3_column_int(stmt, 0);
            curStatus = (const char*)sqlite3_column_text(stmt, 1);
        }
        sqlite3_finalize(stmt);
        if (recId == 0) { UI::error("No record found for that student/subject/date."); UI::pause(); return; }
        std::cout << "  Current status: " << curStatus << "\n";
        std::string newStatus = UI::inputChoice("  New Status", {"Present","Absent","Late","Excused"});
        if (db_.exec("UPDATE attendance SET status='" + newStatus + "' WHERE id=" + std::to_string(recId) + ";"))
            UI::success("Record updated."); else UI::error("Update failed.");
        UI::pause();
    }

    void viewAttendanceByDate() {
        UI::printBanner(); UI::printHeader("VIEW ATTENDANCE BY DATE");
        int subjId = subm_.pickSubject("Select Subject"); if (subjId <= 0) return;
        Subject subj = subm_.getSubjectById(subjId);
        std::string date = UI::inputDate("  Date");
        std::string sql =
            "SELECT s.student_no, s.last_name || ', ' || s.first_name AS name, a.status "
            "FROM attendance a JOIN students s ON a.student_id = s.id "
            "WHERE a.subject_id=" + std::to_string(subjId) + " AND a.date='" + date + "' "
            "ORDER BY s.last_name, s.first_name;";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr);
        std::cout << "\n  Subject: " << subj.code << " - " << subj.name << "  |  Date: " << date << "\n\n";
        std::vector<std::string> hdrs = {"Student No","Name","Status"};
        std::vector<int> widths = {14, 28, 10};
        UI::tableHeader(hdrs, widths);
        int cnt = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            UI::tableRow({(const char*)sqlite3_column_text(stmt,0),
                          (const char*)sqlite3_column_text(stmt,1),
                          (const char*)sqlite3_column_text(stmt,2)}, widths);
            ++cnt;
        }
        sqlite3_finalize(stmt);
        std::cout << "  +"; for (int w : widths) std::cout << std::string(w+2,'-') << "+"; std::cout << "\n";
        if (cnt == 0) UI::info("No records for that date.");
        UI::pause();
    }

    void viewStudentAttendance() {
        UI::printBanner(); UI::printHeader("STUDENT ATTENDANCE SUMMARY");
        int studId = sm_.pickStudent("Select Student"); if (studId <= 0) return;
        Student s = sm_.getStudentById(studId);
        printStudentAttendance(studId, s.fullName(), s.studentNo, false);
    }

    void viewStudentAttendanceById(int studentId) {
        Student s = sm_.getStudentById(studentId);
        UI::printBanner(); UI::printHeader("MY ATTENDANCE SUMMARY");
        printStudentAttendance(studentId, s.fullName(), s.studentNo, true);
    }

    void viewSummaryReport() {
        UI::printBanner(); UI::printHeader("CLASS ATTENDANCE SUMMARY REPORT");
        int subjId = subm_.pickSubject("Select Subject"); if (subjId <= 0) return;
        Subject subj = subm_.getSubjectById(subjId);
        std::string sql =
            "SELECT s.student_no, s.last_name || ', ' || s.first_name AS name, "
            "SUM(CASE WHEN a.status='Present' THEN 1 ELSE 0 END), "
            "SUM(CASE WHEN a.status='Absent'  THEN 1 ELSE 0 END), "
            "SUM(CASE WHEN a.status='Late'    THEN 1 ELSE 0 END), "
            "SUM(CASE WHEN a.status='Excused' THEN 1 ELSE 0 END), "
            "COUNT(*) FROM students s LEFT JOIN attendance a ON a.student_id = s.id AND a.subject_id=" +
            std::to_string(subjId) + " GROUP BY s.id ORDER BY s.last_name, s.first_name;";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr);
        std::cout << "\n  Subject: " << subj.code << " - " << subj.name << "\n\n";
        std::vector<std::string> hdrs = {"Student No","Name","Present","Absent","Late","Excused","Total","%Present"};
        std::vector<int> widths = {12, 26, 7, 6, 5, 7, 5, 8};
        UI::tableHeader(hdrs, widths);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int p=sqlite3_column_int(stmt,2), a=sqlite3_column_int(stmt,3);
            int l=sqlite3_column_int(stmt,4), e=sqlite3_column_int(stmt,5), t=sqlite3_column_int(stmt,6);
            double pct = (t > 0) ? (double)(p+l)/t*100.0 : 0.0;
            std::ostringstream ps; ps << std::fixed << std::setprecision(1) << pct << "%";
            UI::tableRow({(const char*)sqlite3_column_text(stmt,0),(const char*)sqlite3_column_text(stmt,1),
                std::to_string(p),std::to_string(a),std::to_string(l),std::to_string(e),std::to_string(t),ps.str()}, widths);
        }
        sqlite3_finalize(stmt);
        std::cout << "  +"; for (int w : widths) std::cout << std::string(w+2,'-') << "+"; std::cout << "\n";
        UI::pause();
    }

private:
    Database& db_; StudentManager& sm_; SubjectManager& subm_;

    void printStudentAttendance(int studentId, const std::string& name, const std::string& sno, bool isStudent) {
        std::string sql =
            "SELECT sub.code, sub.name, "
            "SUM(CASE WHEN a.status='Present' THEN 1 ELSE 0 END), "
            "SUM(CASE WHEN a.status='Absent'  THEN 1 ELSE 0 END), "
            "SUM(CASE WHEN a.status='Late'    THEN 1 ELSE 0 END), "
            "SUM(CASE WHEN a.status='Excused' THEN 1 ELSE 0 END), "
            "COUNT(*) FROM attendance a JOIN subjects sub ON a.subject_id = sub.id "
            "WHERE a.student_id=" + std::to_string(studentId) + " GROUP BY a.subject_id ORDER BY sub.code;";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr);
        std::cout << "\n  Student: " << name << " (" << sno << ")"
                  << (isStudent ? "  |  Section: N003 BSIT" : "") << "\n\n";
        std::vector<std::string> hdrs = {"Subject","Name","Present","Absent","Late","Excused","Total","%Attend"};
        std::vector<int> widths = {8, 22, 7, 6, 5, 7, 5, 8};
        UI::tableHeader(hdrs, widths);
        bool any = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int p=sqlite3_column_int(stmt,2),a=sqlite3_column_int(stmt,3);
            int l=sqlite3_column_int(stmt,4),e=sqlite3_column_int(stmt,5),t=sqlite3_column_int(stmt,6);
            double pct=(t>0)?(double)(p+l)/t*100.0:0.0;
            std::ostringstream ps; ps<<std::fixed<<std::setprecision(1)<<pct<<"%";
            UI::tableRow({(const char*)sqlite3_column_text(stmt,0),(const char*)sqlite3_column_text(stmt,1),
                std::to_string(p),std::to_string(a),std::to_string(l),std::to_string(e),std::to_string(t),ps.str()},widths);
            any = true;
        }
        sqlite3_finalize(stmt);
        std::cout << "  +"; for (int w : widths) std::cout << std::string(w+2,'-') << "+"; std::cout << "\n";
        if (!any) UI::info("No attendance records found.");
        UI::pause();
    }
};

// ══════════════════════════════════════════════
//  GRADE MANAGER
// ══════════════════════════════════════════════
class GradeManager {
public:
    GradeManager(Database& db, StudentManager& sm, SubjectManager& subm)
        : db_(db), sm_(sm), subm_(subm) {}

    void initTable() {
        db_.exec(R"(
            CREATE TABLE IF NOT EXISTS grades (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                student_id  INTEGER NOT NULL,
                subject_id  INTEGER NOT NULL,
                prelim      REAL DEFAULT 0,
                midterm     REAL DEFAULT 0,
                prefinal    REAL DEFAULT 0,
                final_grade REAL DEFAULT 0,
                final_raw   REAL DEFAULT 0,
                remarks     TEXT DEFAULT 'INC',
                FOREIGN KEY(student_id) REFERENCES students(id),
                FOREIGN KEY(subject_id) REFERENCES subjects(id),
                UNIQUE(student_id, subject_id)
            );
        )");
    }

    void encodeGrades() {
        UI::printBanner(); UI::printHeader("ENCODE / UPDATE GRADES");
        int subjId = subm_.pickSubject("Select Subject"); if (subjId <= 0) return;
        Subject subj = subm_.getSubjectById(subjId);
        int studId = sm_.pickStudent("Select Student"); if (studId <= 0) return;
        Student s = sm_.getStudentById(studId);
        GradeRecord gr; gr.studentId = studId; gr.subjectId = subjId;
        std::string qsql =
            "SELECT id, prelim, midterm, prefinal, final_raw FROM grades "
            "WHERE student_id=" + std::to_string(studId) + " AND subject_id=" + std::to_string(subjId) + ";";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), qsql.c_str(), -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            gr.id=sqlite3_column_int(stmt,0); gr.prelim=sqlite3_column_double(stmt,1);
            gr.midterm=sqlite3_column_double(stmt,2); gr.prefinal=sqlite3_column_double(stmt,3);
            gr.finalScore=sqlite3_column_double(stmt,4);
        }
        sqlite3_finalize(stmt);
        UI::printLine();
        std::cout << "  Student : " << s.fullName() << " (" << s.studentNo << ")\n";
        std::cout << "  Subject : " << subj.code << " - " << subj.name << "\n";
        UI::printLine();
        std::cout << "  Enter percentage grades (0-100). Current values shown in brackets.\n\n";
        auto pGrade = [](const std::string& lbl, double cur) -> double {
            std::cout << "  " << lbl << " [" << std::fixed << std::setprecision(2) << cur << "]: ";
            std::string line; std::getline(std::cin, line);
            line.erase(0, line.find_first_not_of(" \t")); line.erase(line.find_last_not_of(" \t\r\n")+1);
            if (line.empty()) return cur;
            try { double v=std::stod(line); if (v>=0&&v<=100) return v; } catch(...) {}
            std::cout << "  Invalid, keeping current.\n"; return cur;
        };
        gr.prelim     = pGrade("Prelim   (25%)", gr.prelim);
        gr.midterm    = pGrade("Midterm  (25%)", gr.midterm);
        gr.prefinal   = pGrade("Pre-Final(25%)", gr.prefinal);
        gr.finalScore = pGrade("Final    (25%)", gr.finalScore);
        double computed = (gr.prelim+gr.midterm+gr.prefinal+gr.finalScore)*0.25;
        std::string equiv = gradeToEquivalent(computed), remarks = gradeToRemarks(computed);
        std::cout << "\n  -----------------------------------------------\n";
        std::cout << "  Final Grade (raw) : " << std::fixed << std::setprecision(2) << computed << "%\n";
        std::cout << "  Equivalent        : " << equiv << "\n";
        std::cout << "  Remarks           : " << remarks << "\n";
        std::cout << "  -----------------------------------------------\n";
        if (UI::inputChoice("  Save this grade?", {"Yes","No"}) != "Yes") { UI::info("Not saved."); UI::pause(); return; }
        std::string sql;
        if (gr.id > 0) {
            sql = "UPDATE grades SET prelim=" + std::to_string(gr.prelim) +
                  ", midterm=" + std::to_string(gr.midterm) +
                  ", prefinal=" + std::to_string(gr.prefinal) +
                  ", final_raw=" + std::to_string(gr.finalScore) +
                  ", final_grade=" + std::to_string(computed) +
                  ", remarks='" + remarks + "' WHERE id=" + std::to_string(gr.id) + ";";
        } else {
            sql = "INSERT INTO grades (student_id, subject_id, prelim, midterm, prefinal, final_raw, final_grade, remarks) "
                  "VALUES (" + std::to_string(studId) + "," + std::to_string(subjId) + "," +
                  std::to_string(gr.prelim) + "," + std::to_string(gr.midterm) + "," +
                  std::to_string(gr.prefinal) + "," + std::to_string(gr.finalScore) + "," +
                  std::to_string(computed) + ",'" + remarks + "');";
        }
        if (db_.exec(sql)) UI::success("Grade saved."); else UI::error("Save failed.");
        UI::pause();
    }

    void viewStudentGrades() {
        UI::printBanner(); UI::printHeader("STUDENT GRADE REPORT");
        int studId = sm_.pickStudent("Select Student"); if (studId <= 0) return;
        Student s = sm_.getStudentById(studId);
        printGradeReport(studId, s.fullName(), s.studentNo);
    }

    void viewStudentGradesById(int studentId) {
        Student s = sm_.getStudentById(studentId);
        UI::printBanner(); UI::printHeader("MY GRADES");
        printGradeReport(studentId, s.fullName(), s.studentNo);
    }

    void viewSubjectGrades() {
        UI::printBanner(); UI::printHeader("SUBJECT GRADE REPORT");
        int subjId = subm_.pickSubject("Select Subject"); if (subjId <= 0) return;
        Subject subj = subm_.getSubjectById(subjId);
        std::string sql =
            "SELECT s.student_no, s.last_name || ', ' || s.first_name AS name, "
            "g.prelim, g.midterm, g.prefinal, g.final_raw, g.final_grade, g.remarks "
            "FROM students s LEFT JOIN grades g ON g.student_id = s.id AND g.subject_id=" +
            std::to_string(subjId) + " ORDER BY s.last_name, s.first_name;";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr);
        std::cout << "\n  Subject: " << subj.code << " - " << subj.name
                  << " (" << subj.units << " units)  |  " << subj.semester << "\n\n";
        std::vector<std::string> hdrs = {"Student No","Name","Prelim","Midterm","Pre-Fin","Final","Grade%","Equiv","Remarks"};
        std::vector<int> widths = {12, 26, 6, 7, 7, 6, 6, 5, 7};
        UI::tableHeader(hdrs, widths);
        int passed=0,failed=0,inc=0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string sno=(const char*)sqlite3_column_text(stmt,0), name=(const char*)sqlite3_column_text(stmt,1);
            bool hasGrade = sqlite3_column_text(stmt,7) != nullptr;
            if (hasGrade) {
                double p=sqlite3_column_double(stmt,2),m=sqlite3_column_double(stmt,3);
                double pf=sqlite3_column_double(stmt,4),f=sqlite3_column_double(stmt,5),fg=sqlite3_column_double(stmt,6);
                std::string rem=(const char*)sqlite3_column_text(stmt,7), eq=gradeToEquivalent(fg);
                std::ostringstream ps,ms,pfs,fs,fgs;
                ps<<std::fixed<<std::setprecision(1)<<p; ms<<std::fixed<<std::setprecision(1)<<m;
                pfs<<std::fixed<<std::setprecision(1)<<pf; fs<<std::fixed<<std::setprecision(1)<<f;
                fgs<<std::fixed<<std::setprecision(2)<<fg;
                UI::tableRow({sno,name,ps.str(),ms.str(),pfs.str(),fs.str(),fgs.str(),eq,rem}, widths);
                if (rem=="PASSED") ++passed; else ++failed;
            } else { UI::tableRow({sno,name,"-","-","-","-","-","-","INC"}, widths); ++inc; }
        }
        sqlite3_finalize(stmt);
        std::cout << "  +"; for (int w : widths) std::cout << std::string(w+2,'-') << "+"; std::cout << "\n";
        std::cout << "\n  Passed: " << passed << "  |  Failed: " << failed << "  |  Incomplete: " << inc << "\n";
        UI::pause();
    }

    void viewClassReport() {
        UI::printBanner(); UI::printHeader("FULL CLASS GRADE REPORT - N003 BSIT");
        auto students = sm_.getAllStudents(); auto subjects = subm_.getAllSubjects();
        if (students.empty() || subjects.empty()) { UI::info("No data to display."); UI::pause(); return; }
        for (auto& subj : subjects) {
            std::cout << "\n  " << subj.code << " - " << subj.name << "\n";
            std::vector<std::string> hdrs = {"Student No","Name","Final%","Equiv","Remarks"};
            std::vector<int> widths = {12, 28, 7, 5, 7};
            UI::tableHeader(hdrs, widths);
            for (auto& s : students) {
                std::string gsql = "SELECT final_grade, remarks FROM grades WHERE student_id=" +
                    std::to_string(s.id) + " AND subject_id=" + std::to_string(subj.id) + ";";
                sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), gsql.c_str(), -1, &stmt, nullptr);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    double fg=sqlite3_column_double(stmt,0);
                    std::string rem=(const char*)sqlite3_column_text(stmt,1);
                    std::ostringstream fgs; fgs<<std::fixed<<std::setprecision(2)<<fg;
                    UI::tableRow({s.studentNo,s.fullName(),fgs.str(),gradeToEquivalent(fg),rem}, widths);
                } else { UI::tableRow({s.studentNo,s.fullName(),"-","-","INC"}, widths); }
                sqlite3_finalize(stmt);
            }
            std::cout << "  +"; for (int w : widths) std::cout << std::string(w+2,'-') << "+"; std::cout << "\n";
        }
        UI::pause();
    }

private:
    Database& db_; StudentManager& sm_; SubjectManager& subm_;

    std::string gradeToEquivalent(double pct) {
        if (pct>=97) return "1.00"; if (pct>=94) return "1.25"; if (pct>=91) return "1.50";
        if (pct>=88) return "1.75"; if (pct>=85) return "2.00"; if (pct>=82) return "2.25";
        if (pct>=79) return "2.50"; if (pct>=76) return "2.75"; if (pct>=75) return "3.00";
        return "5.00";
    }
    std::string gradeToRemarks(double pct) { return (pct >= 75.0) ? "PASSED" : "FAILED"; }

    void printGradeReport(int studentId, const std::string& name, const std::string& sno) {
        std::string sql =
            "SELECT sub.code, sub.name, sub.units, "
            "g.prelim, g.midterm, g.prefinal, g.final_raw, g.final_grade, g.remarks "
            "FROM subjects sub LEFT JOIN grades g ON g.subject_id = sub.id AND g.student_id=" +
            std::to_string(studentId) + " ORDER BY sub.code;";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr);
        std::cout << "\n  Student: " << name << " (" << sno << ")  |  Section: N003 BSIT\n\n";
        std::vector<std::string> hdrs = {"Code","Subject","Units","Prelim","Midterm","Pre-Fin","Final","Grade%","Equiv","Remarks"};
        std::vector<int> widths = {7, 22, 5, 6, 7, 7, 6, 6, 5, 7};
        UI::tableHeader(hdrs, widths);
        double totalUnits=0, weightedSum=0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string code=(const char*)sqlite3_column_text(stmt,0), nm=(const char*)sqlite3_column_text(stmt,1);
            int units=sqlite3_column_int(stmt,2);
            bool hasGrade = sqlite3_column_text(stmt,8) != nullptr;
            if (hasGrade) {
                double p=sqlite3_column_double(stmt,3),m=sqlite3_column_double(stmt,4);
                double pf=sqlite3_column_double(stmt,5),f=sqlite3_column_double(stmt,6),fg=sqlite3_column_double(stmt,7);
                std::string rem=(const char*)sqlite3_column_text(stmt,8), eq=gradeToEquivalent(fg);
                std::ostringstream ps,ms,pfs,fs,fgs;
                ps<<std::fixed<<std::setprecision(1)<<p; ms<<std::fixed<<std::setprecision(1)<<m;
                pfs<<std::fixed<<std::setprecision(1)<<pf; fs<<std::fixed<<std::setprecision(1)<<f;
                fgs<<std::fixed<<std::setprecision(2)<<fg;
                totalUnits+=units; weightedSum+=std::stod(eq)*units;
                UI::tableRow({code,nm,std::to_string(units),ps.str(),ms.str(),pfs.str(),fs.str(),fgs.str(),eq,rem},widths);
            } else { UI::tableRow({code,nm,std::to_string(units),"-","-","-","-","-","-","INC"},widths); }
        }
        sqlite3_finalize(stmt);
        std::cout << "  +"; for (int w : widths) std::cout << std::string(w+2,'-') << "+"; std::cout << "\n";
        if (totalUnits > 0)
            std::cout << "\n  General Weighted Average (GWA): "
                      << std::fixed << std::setprecision(2) << weightedSum/totalUnits << "\n";
        UI::pause();
    }
};

// ══════════════════════════════════════════════
//  AUTH
// ══════════════════════════════════════════════
class Auth {
public:
    explicit Auth(Database& db) : db_(db) {}

    void initTable() {
        db_.exec(R"(
            CREATE TABLE IF NOT EXISTS users (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                username    TEXT NOT NULL UNIQUE,
                password    TEXT NOT NULL,
                email       TEXT DEFAULT '',
                role        TEXT NOT NULL CHECK(role IN ('admin','teacher','student')),
                studentId   INTEGER DEFAULT 0
            );
        )");
    }

    void seedAdmin() {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_.get(), "SELECT COUNT(*) FROM users WHERE role='admin';", -1, &stmt, nullptr);
        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (count == 0)
            db_.exec("INSERT OR IGNORE INTO users (username, password, email, role) "
                     "VALUES ('admin', 'admin123', 'admin@n003.edu', 'admin');");
    }

    UserSession signIn(const std::string& role) {
        UI::printBanner(); UI::printHeader("SIGN IN  [" + role + "]");
        std::string username = UI::inputString("  Username");
        std::string password = UI::inputString("  Password");
        std::string sql =
            "SELECT id, username, email, role, studentId FROM users "
            "WHERE username='" + username + "' AND password='" + password + "' AND role='" + role + "';";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr);
        UserSession session;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            session.id        = sqlite3_column_int(stmt, 0);
            session.username  = (const char*)sqlite3_column_text(stmt, 1);
            session.email     = sqlite3_column_text(stmt, 2) ? (const char*)sqlite3_column_text(stmt, 2) : "";
            session.role      = (const char*)sqlite3_column_text(stmt, 3);
            session.studentId = sqlite3_column_int(stmt, 4);
            session.loggedIn  = true;
            UI::success("Welcome, " + session.username + "! [" + session.role + "]");
        } else { UI::error("Invalid credentials or wrong role."); }
        sqlite3_finalize(stmt);
        UI::pause();
        return session;
    }

    bool signUp(const std::string& role) {
        UI::printBanner(); UI::printHeader("SIGN UP  [" + role + "]");
        std::string username = UI::inputString("  Username");
        sqlite3_stmt* stmt;
        std::string chk = "SELECT COUNT(*) FROM users WHERE username='" + username + "';";
        sqlite3_prepare_v2(db_.get(), chk.c_str(), -1, &stmt, nullptr);
        int cnt = 0; if (sqlite3_step(stmt) == SQLITE_ROW) cnt = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (cnt > 0) { UI::error("Username already taken."); UI::pause(); return false; }
        std::string password = UI::inputString("  Password");
        std::string email    = UI::inputString("  Email", true);
        std::string sql = "INSERT INTO users (username, password, email, role) VALUES ('" +
            username + "','" + password + "','" + email + "','" + role + "');";
        bool ok = db_.exec(sql);
        if (ok) UI::success("Account created! You can now sign in."); else UI::error("Registration failed.");
        UI::pause();
        return ok;
    }

private:
    Database& db_;
};

// ══════════════════════════════════════════════
//  DASHBOARDS & MAIN
// ══════════════════════════════════════════════
void teacherAdminDashboard(const UserSession& session,
    StudentManager& sm, SubjectManager& subm, AttendanceManager& am, GradeManager& gm)
{
    while (true) {
        int choice = UI::menu("DASHBOARD  [" + session.role + ": " + session.username + "]",
            {"Student Management","Subject Management","Attendance Management","Grade Management"});
        if (choice == 0) { UI::info("Logging out..."); break; }
        if (choice == 1) {
            while (true) {
                int sc = UI::menu("STUDENT MANAGEMENT",
                    {"Add Student","Edit Student","Delete Student","List All Students","Search Student"});
                if (sc==0) break;
                if (sc==1) sm.addStudent(); if (sc==2) sm.editStudent();
                if (sc==3) sm.deleteStudent(); if (sc==4) sm.listStudents(); if (sc==5) sm.searchStudent();
            }
        }
        if (choice == 2) {
            while (true) {
                int sc = UI::menu("SUBJECT MANAGEMENT",
                    {"Add Subject","Edit Subject","Delete Subject","List All Subjects"});
                if (sc==0) break;
                if (sc==1) subm.addSubject(); if (sc==2) subm.editSubject();
                if (sc==3) subm.deleteSubject(); if (sc==4) subm.listSubjects();
            }
        }
        if (choice == 3) {
            while (true) {
                int sc = UI::menu("ATTENDANCE MANAGEMENT",
                    {"Take Attendance (Whole Class)","Edit Attendance Record",
                     "View Attendance by Date","View Student Attendance Summary","View Class Attendance Report"});
                if (sc==0) break;
                if (sc==1) am.takeAttendance(); if (sc==2) am.editAttendance();
                if (sc==3) am.viewAttendanceByDate(); if (sc==4) am.viewStudentAttendance();
                if (sc==5) am.viewSummaryReport();
            }
        }
        if (choice == 4) {
            while (true) {
                int sc = UI::menu("GRADE MANAGEMENT",
                    {"Encode / Update Grades","View Student Grades","View Subject Grades","View Full Class Grade Report"});
                if (sc==0) break;
                if (sc==1) gm.encodeGrades(); if (sc==2) gm.viewStudentGrades();
                if (sc==3) gm.viewSubjectGrades(); if (sc==4) gm.viewClassReport();
            }
        }
    }
}

void studentDashboard(const UserSession& session,
    AttendanceManager& am, GradeManager& gm, Database& db)
{
    int sid = session.studentId;
    if (sid <= 0) {
        std::string sql = "SELECT id FROM students WHERE student_no='" + session.username + "';";
        sqlite3_stmt* stmt; sqlite3_prepare_v2(db.get(), sql.c_str(), -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) sid = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    while (true) {
        int choice = UI::menu("STUDENT PORTAL  [" + session.username + "]",
            {"View My Attendance","View My Grades"});
        if (choice == 0) { UI::info("Logging out..."); break; }
        if (sid <= 0) {
            UI::error("No student record linked to this account.");
            UI::info("Ask your teacher/admin to add your student record using your username as Student No.");
            continue;
        }
        if (choice == 1) am.viewStudentAttendanceById(sid);
        if (choice == 2) gm.viewStudentGradesById(sid);
    }
}

UserSession authMenu(Auth& auth, const std::string& role) {
    UserSession session;
    while (true) {
        int choice = UI::menu("ACCESS  [" + role + "]", {"Sign In","Sign Up"});
        if (choice == 0) break;
        if (choice == 1) { session = auth.signIn(role); if (session.loggedIn) return session; }
        if (choice == 2) auth.signUp(role);
    }
    return session;
}

int main() {
    Database db;
    if (!db.open("n003bsit.db")) { std::cerr << "Fatal: Cannot open database.\n"; return 1; }
    db.exec("PRAGMA foreign_keys = ON;");

    Auth              auth(db);
    StudentManager    sm(db);
    SubjectManager    subm(db);
    AttendanceManager am(db, sm, subm);
    GradeManager      gm(db, sm, subm);

    auth.initTable(); auth.seedAdmin();
    sm.initTable(); subm.initTable(); am.initTable(); gm.initTable();

    while (true) {
        int choice = UI::menu("N003 BSIT  |  ATTENDANCE & GRADE SYSTEM", {"Admin","Teacher","Student"});
        if (choice == 0) {
            UI::printBanner();
            std::cout << "\n  Thank you for using N003 BSIT System. Goodbye!\n\n";
            break;
        }
        std::string role;
        if (choice==1) role="admin"; if (choice==2) role="teacher"; if (choice==3) role="student";
        UserSession session = authMenu(auth, role);
        if (!session.loggedIn) continue;
        if (role == "admin" || role == "teacher")
            teacherAdminDashboard(session, sm, subm, am, gm);
        else
            studentDashboard(session, am, gm, db);
    }
    return 0;
}
