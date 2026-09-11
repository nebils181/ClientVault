# ClientVault

A command-line client-record management system written in modern C++.
Users authenticate with a username/password, then can create, search,
update, and remove client records, with everything persisted to disk
between sessions.

## Features

- 🔐 Account creation & login (passwords are hashed before being stored)
- 📇 Add, view, search, update, and remove client records
- 🔎 Search by name (partial match) or exact passport number
- 💾 Simple delimited-file persistence — no external database required
- 🛡️ Input validation and duplicate-passport checks
- 🧱 Clean class-based design (`AccountManager`, `ClientManager`)

## Getting started

### Build
```bash
g++ -std=c++17 -Wall -Wextra -O2 -o task_manager main.cpp
```

### Run
```bash
./task_manager
```

You'll be prompted to create an account or log in, then dropped into
the main menu to manage client records.

## Data storage

- `accounts.txt` — usernames and hashed passwords
- `clients.txt` — client records, one per line, `|`-delimited

Both files are created automatically in the working directory on first run.

## Notes / limitations

- Password hashing here is a simple non-cryptographic hash meant to avoid
  storing plaintext — **not** suitable for production auth. For real use,
  swap in a salted hash like bcrypt or argon2.
- Data files are plain text with no encryption; don't store sensitive
  real-world data with this as-is.

## License

MIT (or your preferred license)
