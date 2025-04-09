#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iomanip>
#include <stdexcept>
#include <pwd.h>


using namespace std;
namespace fs = std::filesystem;

const int MAX_UID = 65535;
vector<string> uid_to_login(MAX_UID, "");
int file_count = 0;

void uid_to_loginid_converter() {
    ifstream passwd_file("/etc/passwd");
    if (!passwd_file) {
        cerr << "Error opening /etc/passwd" << endl;
        exit(1);
    }
    string line;
    while (getline(passwd_file, line)) {
        istringstream iss(line);
        string username, dummy, uid_str;
        if (getline(iss, username, ':') &&
            getline(iss, dummy, ':') &&
            getline(iss, uid_str, ':')) {
            int uid = 0;
            try {
                uid = stoi(uid_str);
            } catch (const exception &e) {
                continue;
            }
            if (uid >= 0 && uid < MAX_UID) {
                uid_to_login[uid] = username;
            }
        }
    }
}

void traverse(const fs::path &directory, const string &ext) {
    if (!fs::exists(directory)) {
        cerr << "Error: directory " << directory << " does not exist." << endl;
        exit(1);
    }
    try {
        for (const auto &entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ("." + ext)) 
            {
                uintmax_t size = fs::file_size(entry.path());

                string owner = "unknown";
                struct stat file_stat;
                if (stat(entry.path().c_str(), &file_stat) == 0) {
                    uid_t uid = file_stat.st_uid;
                    if (uid < MAX_UID && !uid_to_login[uid].empty()) {
                        owner = uid_to_login[uid];
                    } else {
                        owner = to_string(uid);
                    }
                } 
                cout << setw(10) << ++file_count << ": " 
                     << setw(20) << owner << " " 
                     << setw(20) << size << " " 
                     << entry.path() << "\n";
            }
        }
    } catch (const exception &e) {
        cerr << "Error accessing directory: " << e.what() << endl;
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <directory> <file_extension>\n";
        exit(1);
    }
    uid_to_loginid_converter();
    cout << "NO        : OWNER               SIZE                 NAME\n";
    cout << "--        : -----               ----                 ----\n";

    traverse(argv[1], argv[2]);

    cout << "+++ " << file_count << " files match the extension " << argv[2] << "\n";
    return 0;
}
