# Pattern Finder

A C++ project for pattern discovery in graphs, supporting configurable parameters and efficient processing of large datasets.

---

## 🚀 Getting Started

This guide will help you clone, build, and run the project using **VS Code** and **SSH with GitHub**.

---

## 📥 1. Clone the Repository

Open a terminal and run:

```bash
git clone git@github.com:TalCohenYosef/pattern_finder.git
cd pattern_finder
```

---

## 💻 2. Open in VS Code

```bash
code .
```

Or manually:

* Open VS Code
* Click **File → Open Folder**
* Select the `pattern_finder` folder

---

## � 3. Setup Python Virtual Environment (Optional)

For development tools and utilities:

```bash
# Create virtual environment
python3 -m venv venv

# Activate environment
source venv/bin/activate  # On Linux/Mac
# or
venv\Scripts\activate     # On Windows

# Install development tools (if needed)
pip install -r requirements.txt  # Create this file if needed
```

---

## 📦 4. Install Dependencies

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y build-essential make pkg-config libboost-program-options-dev libboost-system-dev
```

### RHEL / CentOS / Fedora

```bash
sudo yum install -y gcc-c++ make pkgconfig boost-devel
```

### University Server (No sudo required)

The project is configured to work on university servers with limited permissions:

```bash
# Uses system-installed boost libraries
# No additional dependencies required
```

---

## ⚙️ 5. Build the Project

**Simple build using Makefile:**

```bash
make clean
make
```

**Alternative: Manual compilation**

```bash
g++ -std=c++17 -O2 -g -Wall -Wextra -Wpedantic -Iinclude -I/usr/local/anaconda3/include -o pattern_finder src/*.cpp -lboost_program_options
```

After compilation, an executable will be created:

```bash
./pattern_finder
```

---

## ▶️ 6. Run the Project

The program uses command-line arguments:

| Argument     | Description                    |
| ------------ | ------------------------------ |
| `--path`     | Path to input graphs folder    |
| `--alive`    | Alive threshold (double)       |
| `--directed` | Use directed graphs (optional) |

### Example:

```bash
./pattern_finder --path inputs_si --alive 0.7
```

With directed graphs:

```bash
./pattern_finder --path inputs_si --alive 0.7 --directed
```

---

## 🔁 7. Working with Git (VS Code)

### Pull latest changes

* Open **Source Control**
* Click `...` → **Pull**

### Commit & Push

1. Make changes
2. Click `+` to stage files
3. Write commit message
4. Click **Commit**
5. Click **Push / Sync Changes**

---

## 🌿 8. Working with Branches

View all branches:

```bash
git branch -a
```

Create a local branch from remote:

```bash
git switch -c dev origin/changed_json_library
```

Push a new branch:

```bash
git push -u origin your-branch-name
```

---

## 🛠️ Troubleshooting

### SSH issues

```bash
ssh -T git@github.com
```

Expected:

```
Hi TalCohenYosef!
```

---

### Build errors

**Common issues and solutions:**

1. **Boost not found**: Make sure boost is installed or update the include path in Makefile
2. **Permission denied**: The project is designed to work without sudo on university servers
3. **Missing headers**: Check that boost headers are in `/usr/local/anaconda3/include`

**Clean build:**

```bash
make clean
make
```

---

## 📁 Project Structure

```
pattern_finder/
│── src/
│── include/
│── inputs_si/
│── Makefile          # Simple build system
│── CMakeLists.txt    # Original CMake (legacy)
│── README.md
│── .gitignore        # Ignores venv/, build/, etc.
```

---

## 💡 Notes

* Uses **Makefile** for simple builds (no CMake required)
* Uses **boost property_tree** for JSON parsing (no json-c dependency)
* Designed for handling large graph datasets
* Supports both directed and undirected graphs
* Compatible with university servers with limited permissions

---

## 👩‍💻 Author

Tal Cohen Yosef

---

## ⭐ Contributing

Contributions, issues, and suggestions are welcome!
