#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <CommonCrypto/CommonDigest.h>
#include <zlib.h>
#include <bits/stdc++.h>
#include <chrono>

namespace fs = std::filesystem;

std::string hashFile(const std::string& filePath, bool writeToObject) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file " << filePath << std::endl;
        return "";
    }

    CC_SHA1_CTX shaContext;
    CC_SHA1_Init(&shaContext);

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        CC_SHA1_Update(&shaContext, reinterpret_cast<unsigned char*>(buffer), file.gcount());
    }
    CC_SHA1_Update(&shaContext, reinterpret_cast<unsigned char*>(buffer), file.gcount());

    unsigned char hash[CC_SHA1_DIGEST_LENGTH];
    CC_SHA1_Final(hash, &shaContext);

    std::ostringstream oss;
    for (int i = 0; i < CC_SHA1_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    std::string sha1Hash = oss.str();

    if (writeToObject) {
        std::string objectPath = ".mygit/objects/" + sha1Hash;
        std::ofstream objectFile(objectPath, std::ios::binary);
        if (!objectFile) {
            std::cerr << "Error: Failed to create object file." << std::endl;
            return "";
        }

        file.clear();
        file.seekg(0, std::ios::beg);
        std::ostringstream uncompressedData;
        uncompressedData << file.rdbuf();
        std::string uncompressedStr = uncompressedData.str();

        uLongf compressedSize = compressBound(uncompressedStr.size());
        std::vector<char> compressedData(compressedSize);

        if (compress(reinterpret_cast<Bytef*>(compressedData.data()), &compressedSize,
                     reinterpret_cast<const Bytef*>(uncompressedStr.data()), uncompressedStr.size()) != Z_OK) {
            std::cerr << "Error: Compression failed." << std::endl;
            return "";
        }

        objectFile.write(compressedData.data(), compressedSize);
        objectFile.close();
    }

    return sha1Hash;
}

void displayCommitDetails(const std::string& commitSha) {
    std::string commitPath = ".mygit/commits/" + commitSha;
    std::ifstream commitFile(commitPath);
    if (!commitFile) {
        std::cerr << "Error: Commit file not found for SHA " << commitSha << std::endl;
        return;
    }

    std::cout << "Commit: " << commitSha << std::endl;

    std::string line;
    while (std::getline(commitFile, line)) {
        if (line.find("parent:") == 0) {
            std::cout << "Parent: " << line.substr(8) << std::endl;
        } else if (line.find("message:") == 0) {
            std::cout << "Message: " << line.substr(9) << std::endl;
        } else if (line.find("timestamp:") == 0) {
            std::cout << "Timestamp: " << line.substr(11) << std::endl;
        }
    }

    std::cout << "Committer: User <user@example.com>" << std::endl;
    std::cout << std::endl;
}

void logHistory() {
    std::string headPath = ".mygit/HEAD";
    std::ifstream headFile(headPath);
    std::string currentCommitSha;
    if (!headFile || !std::getline(headFile, currentCommitSha)) {
        std::cerr << "Error: No commit history found." << std::endl;
        return;
    }

    while (!currentCommitSha.empty()) {
        displayCommitDetails(currentCommitSha);

        std::string commitPath = ".mygit/commits/" + currentCommitSha;
        std::ifstream commitFile(commitPath);
        std::string line;
        currentCommitSha.clear();
        while (std::getline(commitFile, line)) {
            if (line.find("parent:") == 0) {
                currentCommitSha = line.substr(8);
                break;
            }
        }
    }
}

std::string writeTree(const std::string& directory = ".") {
    CC_SHA1_CTX shaContext;
    CC_SHA1_Init(&shaContext);

    std::ostringstream treeContent;

    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (entry.is_directory() || entry.path().string().find(".mygit") != std::string::npos) {
            continue;
        }

        std::string relativePath = fs::relative(entry.path(), directory).string();
        std::string fileSha = hashFile(entry.path().string(), true);

        treeContent << "blob " << fileSha << " " << relativePath << "\n";
    }

    std::string treeData = treeContent.str();
    CC_SHA1_Update(&shaContext, reinterpret_cast<const unsigned char*>(treeData.c_str()), treeData.size());

    unsigned char hash[CC_SHA1_DIGEST_LENGTH];
    CC_SHA1_Final(hash, &shaContext);

    std::ostringstream oss;
    for (int i = 0; i < CC_SHA1_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    std::string treeSha1Hash = oss.str();

    std::string objectPath = ".mygit/objects/" + treeSha1Hash;
    std::ofstream objectFile(objectPath, std::ios::binary);
    if (!objectFile) {
        std::cerr << "Error: Failed to create tree object file." << std::endl;
        return "";
    }

    objectFile << "tree\n" << treeData;
    objectFile.close();

    std::cout << treeSha1Hash << std::endl;
    return treeSha1Hash;
}

void commitChanges(const std::string& message = "Default commit message") {
    const std::string indexFilePath = ".mygit/index";
    std::ifstream indexFile(indexFilePath);
    if (!indexFile) {
        std::cerr << "Error: No staged files to commit." << std::endl;
        return;
    }

    std::string treeSha = writeTree();

    std::ostringstream commitContent;
    commitContent << "commit\n";
    commitContent << "tree: " << treeSha << "\n";
    commitContent << "message: " << message << "\n";
    
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    commitContent << "timestamp: " << std::ctime(&now);

    std::string parentCommitPath = ".mygit/HEAD";
    std::ifstream parentFile(parentCommitPath);
    std::string parentHash;
    if (parentFile && std::getline(parentFile, parentHash)) {
        commitContent << "parent: " << parentHash << "\n";
    }
    parentFile.close();

    CC_SHA1_CTX shaContext;
    CC_SHA1_Init(&shaContext);
    CC_SHA1_Update(&shaContext, reinterpret_cast<const unsigned char*>(commitContent.str().c_str()), commitContent.str().size());

    unsigned char hash[CC_SHA1_DIGEST_LENGTH];
    CC_SHA1_Final(hash, &shaContext);
    std::ostringstream oss;
    for (int i = 0; i < CC_SHA1_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    std::string commitSha = oss.str();

    std::string commitPath = ".mygit/commits/" + commitSha;
    fs::create_directories(".mygit/commits");
    std::ofstream commitFile(commitPath);
    if (!commitFile) {
        std::cerr << "Error: Unable to write commit." << std::endl;
        return;
    }
    commitFile << commitContent.str();
    commitFile.close();

    std::ofstream headFile(".mygit/HEAD");
    headFile << commitSha;
    headFile.close();

    std::cout << commitSha << std::endl;
}


void initializeRepository() {
    const std::string mygitDir = ".mygit";

    try {
        if (!fs::exists(mygitDir)) {
            fs::create_directory(mygitDir);
            std::cout << "Initialized empty MyGit repository in ./.mygit" << std::endl;
        } else {
            std::cerr << "Error: Repository already exists." << std::endl;
            return;
        }

        fs::create_directory(mygitDir + "/objects");
        fs::create_directory(mygitDir + "/refs");

        std::ofstream headFile(mygitDir + "/HEAD");
        if (headFile) {
            headFile << "ref: refs/heads/master" << std::endl;
            headFile.close();
        } else {
            std::cerr << "Error: Failed to create HEAD file." << std::endl;
            return;
        }

    } catch (const fs::filesystem_error &e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void catFile(const std::string& flag, const std::string& fileSha) {
    std::string objectPath = ".mygit/objects/" + fileSha;
    std::ifstream objectFile(objectPath, std::ios::binary);
    if (!objectFile) {
        std::cerr << "Error: Object not found for SHA " << fileSha << std::endl;
        return;
    }

    if (flag == "-p") {
        std::ostringstream compressedData;
        compressedData << objectFile.rdbuf();
        std::string compressedStr = compressedData.str();

        uLongf uncompressedSize = compressedStr.size() * 4;
        std::string uncompressedData(uncompressedSize, '\0');
        if (uncompress(reinterpret_cast<Bytef*>(&uncompressedData[0]), &uncompressedSize, reinterpret_cast<const Bytef*>(compressedStr.data()), compressedStr.size()) != Z_OK) {
            std::cerr << "Error: Decompression failed." << std::endl;
            return;
        }
        uncompressedData.resize(uncompressedSize);
        std::cout << uncompressedData << std::endl;
    } else if (flag == "-s") {
        objectFile.seekg(0, std::ios::end);
        std::cout << "File size: " << objectFile.tellg() << " bytes" << std::endl;
    } else if (flag == "-t") {
        std::string line;
        std::getline(objectFile, line);
        if (line.rfind("blob", 0) == 0) {
            std::cout << "Type: blob" << std::endl;
        } else if (line.rfind("tree", 0) == 0) {
            std::cout << "Type: tree" << std::endl;
        } else {
            std::cerr << "Error: Unknown object type." << std::endl;
        }
    } else {
        std::cerr << "Error: Unknown flag " << flag << std::endl;
    }
}


void listTree(const std::string& treeSha, bool nameOnly) {
    std::string objectPath = ".mygit/objects/" + treeSha;
    std::ifstream objectFile(objectPath, std::ios::binary);
    if (!objectFile) {
        std::cerr << "Error: Tree object not found for SHA " << treeSha << std::endl;
        return;
    }

    std::string line;
    std::getline(objectFile, line);
    if (line != "tree") {
        std::cerr << "Error: Object is not a tree." << std::endl;
        return;
    }

    while (std::getline(objectFile, line)) {
        std::istringstream lineStream(line);
        std::string type, sha, name;
        lineStream >> type >> sha >> std::ws;
        std::getline(lineStream, name);

        if (nameOnly) {
            std::cout << name << std::endl;
        } else {
            std::cout << "100644 " << type << " " << sha << "\t" << name << std::endl;
        }
    }
}

void addFiles(const std::vector<std::string>& files) {
    const std::string indexFilePath = ".mygit/index";
    std::ofstream indexFile(indexFilePath, std::ios::app);
    if (!indexFile) {
        std::cerr << "Error: Unable to open index file for writing." << std::endl;
        return;
    }

    for (const auto& path : files) {
        if (!fs::exists(path)) {
            std::cerr << "Error: File " << path << " does not exist." << std::endl;
            continue;
        }

        if (fs::is_directory(path)) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (!entry.is_directory()) {
                    std::string fileSha = hashFile(entry.path().string(), true);
                    if (!fileSha.empty()) {
                        indexFile << entry.path().string() << " " << fileSha << "\n";
                        std::cout << "Staged: " << entry.path().string() << std::endl;
                    }
                }
            }
        } else {
            std::string fileSha = hashFile(path, true);
            if (!fileSha.empty()) {
                indexFile << path << " " << fileSha << "\n";
                std::cout << "Staged: " << path << std::endl;
            }
        }
    }
    indexFile.close();
}

void restoreTree(const std::string& treeSha, const std::string& directory = ".") {
    std::string treePath = ".mygit/objects/" + treeSha;
    std::ifstream treeFile(treePath);
    if (!treeFile) {
        std::cerr << "Error: Tree object not found for SHA " << treeSha << std::endl;
        return;
    }

    std::string line;
    while (std::getline(treeFile, line)) {
        std::istringstream lineStream(line);
        std::string type, fileSha, relativePath;
        lineStream >> type >> fileSha >> std::ws;
        std::getline(lineStream, relativePath);

        std::string targetPath = directory + "/" + relativePath;

        if (type == "blob") {
            std::string objectPath = ".mygit/objects/" + fileSha;
            std::ifstream objectFile(objectPath, std::ios::binary);
            if (!objectFile) {
                std::cerr << "Error: Object file not found for SHA " << fileSha << std::endl;
                continue;
            }

            // Read compressed data
            std::ostringstream compressedDataStream;
            compressedDataStream << objectFile.rdbuf();
            std::string compressedStr = compressedDataStream.str();

            // Initial estimate for uncompressed size
            uLongf uncompressedSize = compressedStr.size() * 4;
            std::vector<char> uncompressedData(uncompressedSize);

            int decompressResult = uncompress(reinterpret_cast<Bytef*>(uncompressedData.data()), &uncompressedSize,
                                              reinterpret_cast<const Bytef*>(compressedStr.data()), compressedStr.size());

            // If the buffer is too small, increase it until decompression is successful
            while (decompressResult == Z_BUF_ERROR) {
                uncompressedSize *= 2;  // Double the buffer size
                uncompressedData.resize(uncompressedSize);
                decompressResult = uncompress(reinterpret_cast<Bytef*>(uncompressedData.data()), &uncompressedSize,
                                              reinterpret_cast<const Bytef*>(compressedStr.data()), compressedStr.size());
            }

            if (decompressResult != Z_OK) {
                std::cerr << "Error: Failed to decompress object for file " << relativePath
                          << " with error code: " << decompressResult << std::endl;
                continue;
            }

            // Write decompressed data to the target path
            fs::create_directories(fs::path(targetPath).parent_path());
            std::ofstream outFile(targetPath, std::ios::binary);
            if (!outFile) {
                std::cerr << "Error: Unable to write to target file " << targetPath << std::endl;
                continue;
            }
            outFile.write(uncompressedData.data(), uncompressedSize);
            if (outFile.fail()) {
                std::cerr << "Error: Writing decompressed data to file " << targetPath << " failed." << std::endl;
            }
        }
    }
}

void checkoutCommit(const std::string& commitSha) {
    std::string commitPath = ".mygit/commits/" + commitSha;
    std::ifstream commitFile(commitPath);
    if (!commitFile) {
        std::cerr << "Error: Commit not found for SHA " << commitSha << std::endl;
        return;
    }

    std::string line, treeSha;
    while (std::getline(commitFile, line)) {
        if (line.find("tree:") == 0) {
            treeSha = line.substr(6);
            break;
        }
    }

    if (treeSha.empty()) {
        std::cerr << "Error: Tree SHA not found in commit." << std::endl;
        return;
    }

    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.path().filename() != ".mygit") {
            fs::remove_all(entry);
        }
    }

    restoreTree(treeSha);

    std::ofstream headFile(".mygit/HEAD");
    headFile << commitSha;
    std::cout << "Checked out commit: " << commitSha << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string command = argv[1];
        if (command == "init") {
            initializeRepository();
        } else if (command == "hash-object" && argc >= 3) {
            bool writeToObject = (argc == 4 && std::string(argv[2]) == "-w");
            std::string filePath = writeToObject ? argv[3] : argv[2];
            std::string sha1Hash = hashFile(filePath, writeToObject);
            if (!sha1Hash.empty()) {
                std::cout << sha1Hash << std::endl;
            }
        } else if (command == "cat-file" && argc == 4) {
            std::string flag = argv[2];
            std::string fileSha = argv[3];
            catFile(flag, fileSha);
        } else if (command == "write-tree") {
            writeTree();
        } else if (command == "ls-tree" && argc >= 3) {
            bool nameOnly = (argc == 4 && std::string(argv[2]) == "--name-only");
            std::string treeSha = nameOnly ? argv[3] : argv[2];
            listTree(treeSha, nameOnly);
        } else if (command == "add") {
            std::vector<std::string> files;
            if (argc == 3 && std::string(argv[2]) == ".") {
                files.push_back(".");
            } else {
                for (int i = 2; i < argc; ++i) {
                    files.push_back(argv[i]);
                }
            }
            addFiles(files);
        } else if (command == "commit") {
            std::string commitMessage = "Default commit message";
            if (argc == 4 && std::string(argv[2]) == "-m") {
                commitMessage = argv[3];
            }
            commitChanges(commitMessage);
        } 
        else if(command == "log") {
            logHistory();
        }
        else if (command == "checkout" && argc == 3) {
            std::string commitSha = argv[2];
            checkoutCommit(commitSha);
        } 
        else {
            std::cerr << "Usage: ./mygit <command> [options]" << std::endl;
        }
    } else {
        std::cerr << "Usage: ./mygit <command> [options]" << std::endl;
    }
    return 0;
}
