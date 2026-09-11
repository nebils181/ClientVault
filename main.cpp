#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <cctype>
#include <functional>

using namespace std;

// ============================================================
//  Config
// ============================================================
static const char DELIM = '|';                 // field separator in data files
static const string CLIENTS_FILE  = "clients.txt";
static const string ACCOUNTS_FILE = "accounts.txt";
static const int    MAX_LOGIN_ATTEMPTS = 3;

// ============================================================
//  Small input / string helpers
// ============================================================

// Strip characters that would break the delimited file format.
string sanitize(string s) {
    s.erase(remove(s.begin(), s.end(), DELIM), s.end());
    s.erase(remove(s.begin(), s.end(), '\n'), s.end());
    s.erase(remove(s.begin(), s.end(), '\r'), s.end());
    return s;
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)tolower(c); });
    return s;
}

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Reads a whole line, sanitized. Retries (silently) until non-empty if requireNonEmpty.
string readLine(const string& prompt, bool requireNonEmpty = false) {
    string val;
    while (true) {
        cout << prompt;
        getline(cin, val);
        val = sanitize(val);
        if (!requireNonEmpty || !val.empty()) return val;
        cout << "This field can't be empty.\n";
    }
}

// Reads a single character choice (first non-space char of the line).
char readChar(const string& prompt) {
    string s = readLine(prompt);
    for (char c : s) if (!isspace((unsigned char)c)) return c;
    return '\0';
}

int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        int val;
        if (cin >> val) { clearInput(); return val; }
        cout << "Please enter a valid whole number.\n";
        clearInput();
    }
}

bool confirm(const string& prompt) {
    return toLower(readLine(prompt)) == "y";
}

// Very small non-cryptographic hash so plaintext passwords are at least not
// stored on disk. NOTE: this is NOT secure enough for production use — a
// real system should use a salted password hash such as bcrypt/argon2.
size_t hashPassword(const string& password) {
    return hash<string>{}(password);
}

// ============================================================
//  Account (login) management
// ============================================================
struct Account {
    string username;
    size_t passwordHash;
};

class AccountManager {
public:
    AccountManager() { load(); }

    bool usernameExists(const string& username) const {
        return find_if(accounts.begin(), accounts.end(), [&](const Account& a) {
            return a.username == username;
        }) != accounts.end();
    }

    bool createAccount(const string& username, const string& password) {
        if (username.empty() || password.empty()) return false;
        if (usernameExists(username)) return false;
        accounts.push_back({ username, hashPassword(password) });
        save();
        return true;
    }

    bool authenticate(const string& username, const string& password) const {
        size_t h = hashPassword(password);
        for (const auto& a : accounts)
            if (a.username == username && a.passwordHash == h) return true;
        return false;
    }

private:
    vector<Account> accounts;

    void load() {
        ifstream file(ACCOUNTS_FILE);
        if (!file) return;
        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            size_t pos = line.find(DELIM);
            if (pos == string::npos) continue;
            Account a;
            a.username = line.substr(0, pos);
            try {
                a.passwordHash = stoull(line.substr(pos + 1));
            } catch (...) {
                continue; // skip corrupted line rather than crash
            }
            accounts.push_back(a);
        }
    }

    void save() const {
        ofstream file(ACCOUNTS_FILE, ios::trunc);
        for (const auto& a : accounts) file << a.username << DELIM << a.passwordHash << "\n";
    }
};

// ============================================================
//  Client record
// ============================================================
struct Client {
    string fullname;
    string gender;
    string passport;
    string country;
    string city;
    bool   married = false;
    string relativeName;
    string relativePhone;
    bool   employed = false;

    string toLine() const {
        ostringstream oss;
        oss << fullname << DELIM << gender << DELIM << passport << DELIM
            << country << DELIM << city << DELIM << (married ? 1 : 0) << DELIM
            << relativeName << DELIM << relativePhone << DELIM << (employed ? 1 : 0);
        return oss.str();
    }

    static bool fromLine(const string& line, Client& out) {
        vector<string> f;
        stringstream ss(line);
        string field;
        while (getline(ss, field, DELIM)) f.push_back(field);
        // account for a possible empty trailing field (e.g. line ends in '|')
        if (!line.empty() && line.back() == DELIM) f.push_back("");
        if (f.size() < 9) return false;

        out.fullname      = f[0];
        out.gender        = f[1];
        out.passport      = f[2];
        out.country       = f[3];
        out.city          = f[4];
        out.married       = (f[5] == "1");
        out.relativeName  = f[6];
        out.relativePhone = f[7];
        out.employed      = (f[8] == "1");
        return true;
    }
};

// ============================================================
//  Client management
// ============================================================
class ClientManager {
public:
    ClientManager() { load(); }

    bool passportExists(const string& passport) const {
        return find_if(clients.begin(), clients.end(), [&](const Client& c) {
            return c.passport == passport;
        }) != clients.end();
    }

    bool empty() const { return clients.empty(); }
    int  count() const { return (int)clients.size(); }

    void addClientInteractive() {
        Client c;
        c.fullname = readLine("Full name: ", true);
        c.gender   = readLine("Gender: ");
        while (true) {
            c.passport = readLine("Passport number: ", true);
            if (passportExists(c.passport)) {
                cout << "A client with this passport number already exists.\n";
                continue;
            }
            break;
        }
        cout << "Address:\n";
        c.country = readLine("  Country: ");
        c.city    = readLine("  City: ");
        c.married = confirm("Married? (y/n): ");
        cout << "Relative information:\n";
        c.relativeName  = readLine("  Full name: ");
        c.relativePhone = readLine("  Phone number: ");
        c.employed = confirm("Employed? (y/n): ");

        clients.push_back(c);
        cout << "Client added.\n";
    }

    void addLoop() {
        do {
            addClientInteractive();
        } while (confirm("Add another client? (y/n): "));
        save();
    }

    void display() const {
        if (clients.empty()) { cout << "No records to display.\n"; return; }

        const int idxW = 5, nameW = 20, midW = 14, phoneW = 16;
        cout << left
             << setw(idxW)   << "#"
             << setw(nameW)  << "Full Name"
             << setw(midW)   << "Gender"
             << setw(nameW)  << "Passport"
             << setw(midW)   << "Country"
             << setw(midW)   << "City"
             << setw(midW)   << "Marital"
             << setw(nameW)  << "Relative"
             << setw(phoneW) << "Rel. Phone"
             << setw(midW)   << "Employment"
             << "\n";
        cout << string(idxW + nameW * 3 + midW * 4 + phoneW, '-') << "\n";

        for (size_t i = 0; i < clients.size(); ++i) {
            const Client& c = clients[i];
            cout << left
                 << setw(idxW)   << i + 1
                 << setw(nameW)  << c.fullname
                 << setw(midW)   << c.gender
                 << setw(nameW)  << c.passport
                 << setw(midW)   << c.country
                 << setw(midW)   << c.city
                 << setw(midW)   << (c.married ? "Married" : "Single")
                 << setw(nameW)  << c.relativeName
                 << setw(phoneW) << c.relativePhone
                 << setw(midW)   << (c.employed ? "Employed" : "Unemployed")
                 << "\n";
        }
    }

    void printRecord(int i) const {
        const Client& c = clients[i];
        cout << "\n--- Record #" << i + 1 << " ---\n"
             << "Name:        " << c.fullname << "\n"
             << "Gender:      " << c.gender << "\n"
             << "Passport:    " << c.passport << "\n"
             << "Country:     " << c.country << "\n"
             << "City:        " << c.city << "\n"
             << "Marital:     " << (c.married ? "Married" : "Single") << "\n"
             << "Relative:    " << c.relativeName << " (" << c.relativePhone << ")\n"
             << "Employment:  " << (c.employed ? "Employed" : "Unemployed") << "\n";
    }

    void search() const {
        if (clients.empty()) { cout << "No records to search.\n"; return; }
        cout << "1. Search by name\n2. Search by passport\n";
        char ch = readChar("Choice: ");

        vector<int> matches;
        if (ch == '1') {
            string term = toLower(readLine("Enter (part of) the name: "));
            for (size_t i = 0; i < clients.size(); ++i)
                if (toLower(clients[i].fullname).find(term) != string::npos)
                    matches.push_back((int)i);
        } else if (ch == '2') {
            string term = readLine("Enter passport number: ");
            for (size_t i = 0; i < clients.size(); ++i)
                if (clients[i].passport == term) matches.push_back((int)i);
        } else {
            cout << "Invalid choice.\n";
            return;
        }

        if (matches.empty()) cout << "No matching records found.\n";
        else for (int i : matches) printRecord(i);
    }

    bool validIndex(int idx) const { return idx >= 0 && idx < (int)clients.size(); }

    void removeLoop() {
        if (clients.empty()) { cout << "There are no clients to remove.\n"; return; }
        while (true) {
            display();
            int idx = readInt("Enter the index of the client to remove (0 to cancel): ") - 1;
            if (idx == -1) break;
            if (!validIndex(idx)) { cout << "Invalid index.\n"; continue; }

            if (confirm("Remove \"" + clients[idx].fullname + "\"? (y/n): ")) {
                clients.erase(clients.begin() + idx);
                save();
                cout << "Client removed.\n";
            }
            if (clients.empty() || !confirm("Remove another client? (y/n): ")) break;
        }
    }

    void updateLoop() {
        if (clients.empty()) { cout << "There are no clients to update.\n"; return; }
        display();
        int idx = readInt("Enter the index of the client to update: ") - 1;
        if (!validIndex(idx)) { cout << "Invalid index.\n"; return; }
        Client& c = clients[idx];

        cout << "1. Full name\n2. Gender\n3. Passport number\n4. Address\n"
             << "5. Marital status\n6. Relative information\n7. Employment status\n"
             << "*. Cancel\n";
        char ch = readChar("Choice: ");

        switch (ch) {
            case '1':
                c.fullname = readLine("New full name: ", true);
                break;
            case '2':
                c.gender = readLine("New gender: ");
                break;
            case '3': {
                string p = readLine("New passport number: ", true);
                if (p != c.passport && passportExists(p))
                    cout << "That passport number is already in use — not changed.\n";
                else
                    c.passport = p;
                break;
            }
            case '4': {
                char sub = readChar("1. Country\n2. City\nChoice: ");
                if (sub == '1')      c.country = readLine("New country: ");
                else if (sub == '2') c.city    = readLine("New city: ");
                else { cout << "Invalid choice.\n"; return; }
                break;
            }
            case '5':
                c.married = confirm("Married? (y/n): ");
                break;
            case '6': {
                char sub = readChar("1. Relative name\n2. Relative phone\nChoice: ");
                if (sub == '1')      c.relativeName  = readLine("New relative name: ");
                else if (sub == '2') c.relativePhone = readLine("New relative phone: ");
                else { cout << "Invalid choice.\n"; return; }
                break;
            }
            case '7':
                c.employed = confirm("Employed? (y/n): ");
                break;
            case '*':
                cout << "Cancelled.\n";
                return;
            default:
                cout << "Invalid choice.\n";
                return;
        }

        save();
        cout << "Client updated.\n";
        printRecord(idx);
    }

    void save() const {
        ofstream file(CLIENTS_FILE, ios::trunc);
        for (const auto& c : clients) file << c.toLine() << "\n";
    }

private:
    vector<Client> clients;

    void load() {
        clients.clear();
        ifstream file(CLIENTS_FILE);
        if (!file) return;
        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            Client c;
            if (Client::fromLine(line, c)) clients.push_back(c);
        }
    }
};

// ============================================================
//  Menus
// ============================================================
void mainMenu() {
    ClientManager manager;
    cout << "\nWelcome! You have " << manager.count() << " client record(s) on file.\n";

    while (true) {
        cout << "\n1. Display records\n"
             << "2. Add record\n"
             << "3. Search records\n"
             << "4. Remove record\n"
             << "5. Update record\n"
             << "*. Log out\n";
        char choice = readChar("Enter the option you want to execute: ");

        switch (choice) {
            case '1': manager.display();  break;
            case '2': manager.addLoop();  break;
            case '3': manager.search();   break;
            case '4': manager.removeLoop(); break;
            case '5': manager.updateLoop(); break;
            case '*':
                cout << "Logging out...\n";
                return;
            default:
                cout << "!! Invalid input !! Please try again.\n";
        }
    }
}

void handleCreateAccount(AccountManager& accounts) {
    string username = readLine("Choose a username: ", true);
    string password = readLine("Choose a password: ", true);
    if (accounts.createAccount(username, password))
        cout << "Account created successfully!\n";
    else
        cout << "That username is taken, or the input was invalid. Try again.\n";
}

// Returns true if login succeeded.
bool handleLogin(AccountManager& accounts) {
    string username = readLine("Username: ");
    string password = readLine("Password: ");
    if (accounts.authenticate(username, password)) {
        cout << "Login successful.\n";
        mainMenu();
        return true;
    }
    cout << "Invalid username or password!\n";
    return false;
}

int firstPage() {
    AccountManager accounts;
    int attempts = 0;

    while (true) {
        cout << "\nWelcome to the Task Manager.\n"
             << "C - Create a new account\n"
             << "L - Log in\n"
             << "Q - Quit\n";
        char choice = readChar("Choice: ");

        switch (tolower((unsigned char)choice)) {
            case 'c':
                handleCreateAccount(accounts);
                break;
            case 'l':
                if (!handleLogin(accounts)) {
                    attempts++;
                    if (attempts >= MAX_LOGIN_ATTEMPTS) {
                        cout << "Too many failed attempts. Exiting.\n";
                        return 0;
                    }
                } else {
                    attempts = 0;
                }
                break;
            case 'q':
                cout << "Goodbye!\n";
                return 0;
            default:
                cout << "!! Invalid input !!\n";
        }
    }
}

int main() {
    return firstPage();
}
