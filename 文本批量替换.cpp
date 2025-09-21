#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <regex>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

// 列出当前目录下的所有txt文件
std::vector<fs::path> list_txt_files() {
    std::vector<fs::path> txt_files;

    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            txt_files.push_back(entry.path());
        }
    }

    if (txt_files.empty()) {
        std::cout << "当前目录下没有找到txt文件" << std::endl;
        return txt_files;
    }

    std::cout << "找到以下txt文件:" << std::endl;
    for (size_t i = 0; i < txt_files.size(); ++i) {
        std::cout << (i + 1) << ". " << txt_files[i].filename().string() << std::endl;
    }

    return txt_files;
}

// 让用户选择一个文件
fs::path select_file(const std::string& prompt, const std::vector<fs::path>& file_list) {
    while (true) {
        std::cout << prompt;
        std::string choice;
        std::getline(std::cin, choice);

        if (choice.empty()) {
            return fs::path();
        }

        try {
            size_t index = std::stoul(choice) - 1;
            if (index < file_list.size()) {
                return file_list[index];
            } else {
                std::cout << "请输入1到" << file_list.size() << "之间的数字" << std::endl;
            }
        } catch (const std::exception&) {
            std::cout << "请输入有效的数字" << std::endl;
        }
    }
}

// 读取文件内容并返回行列表
std::vector<std::string> read_file(const fs::path& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout << "错误：文件 " << filename << " 未找到" << std::endl;
        return lines;
    }

    try {
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
    } catch (const std::exception& e) {
        std::cout << "读取文件 " << filename << " 时出错: " << e.what() << std::endl;
        lines.clear();
    }

    return lines;
}

// 将内容写入文件
bool write_file(const fs::path& filename, const std::vector<std::string>& content) {
    try {
        // 创建备份文件
        if (fs::exists(filename)) {
            fs::path backup_name = filename;
            backup_name += ".bak";

            if (fs::exists(backup_name)) {
                fs::remove(backup_name);
            }

            fs::rename(filename, backup_name);
            std::cout << "已创建备份: " << backup_name.filename().string() << std::endl;
        }

        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cout << "无法打开文件 " << filename << " 进行写入" << std::endl;
            return false;
        }

        for (size_t i = 0; i < content.size(); ++i) {
            file << content[i];
            if (i < content.size() - 1) {
                file << "\n";
            }
        }

        std::cout << "成功写入文件: " << filename.filename().string() << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "写入文件 " << filename << " 时出错: " << e.what() << std::endl;
        return false;
    }
}

// 从行列表中提取键值对
std::map<std::string, std::string> extract_key_value_pairs(const std::vector<std::string>& lines) {
    std::map<std::string, std::string> key_value_pairs;
    std::regex pattern(R"((\w+)=(.*))");

    for (const auto& line : lines) {
        std::smatch match;
        if (std::regex_match(line, match, pattern)) {
            key_value_pairs[match[1].str()] = match[2].str();
        }
    }

    return key_value_pairs;
}

// 处理文件并进行替换操作
bool process_files(const fs::path& write_file_path, const fs::path& compare_file_path, const fs::path& replace_file_path) {
    // 读取文件
    auto write_lines = read_file(write_file_path);
    auto compare_lines = read_file(compare_file_path);
    auto replace_lines = read_file(replace_file_path);

    if (write_lines.empty() || compare_lines.empty() || replace_lines.empty()) {
        return false;
    }

    // 提取键值对
    auto write_dict = extract_key_value_pairs(write_lines);
    auto compare_dict = extract_key_value_pairs(compare_lines);
    auto replace_dict = extract_key_value_pairs(replace_lines);

    // 找出两个文件中都存在的键，并且值相同的键
    std::set<std::string> common_keys;
    for (const auto& pair : compare_dict) {
        const auto& key = pair.first;
        if (write_dict.count(key) && compare_dict[key] == write_dict[key]) {
            common_keys.insert(key);
        }
    }

    std::cout << "找到" << common_keys.size() << "个键和值都相同的键" << std::endl;

    if (common_keys.empty()) {
        std::cout << "没有找到可以替换的键，请检查文件内容" << std::endl;
        return false;
    }

    // 确认操作
    std::cout << "是否继续?(y/n):";
    std::string confirm;
    std::getline(std::cin, confirm);

    if (confirm != "y" && confirm != "Y") {
        std::cout << "操作已取消" << std::endl;
        return false;
    }

    // 处理要写入的文件的每一行
    std::vector<std::string> processed_lines;
    size_t replaced_count = 0;
    std::vector<std::string> not_replaced_keys;
    std::regex pattern(R"((\w+)=(.*))");

    for (const auto& line : write_lines) {
        std::smatch match;
        if (std::regex_match(line, match, pattern)) {
            std::string key = match[1].str();
            std::string value = match[2].str();

            // 如果键在共同键中（键和值都相同）并且在替换字典中存在，则进行替换
            if (common_keys.find(key) != common_keys.end() &&
                replace_dict.count(key)) {
                processed_lines.push_back(key + "=" + replace_dict.at(key));
                replaced_count++;
            } else {
                processed_lines.push_back(line);
                if (common_keys.find(key) != common_keys.end() &&
                    !replace_dict.count(key)) {
                    not_replaced_keys.push_back(key);
                }
            }
        } else {
            // 如果不是键值对格式，保持原样
            processed_lines.push_back(line);
        }
    }

    // 将处理后的内容写入文件
    if (write_file(write_file_path, processed_lines)) {
        std::cout << "成功替换了" << replaced_count << "个键值对" << std::endl;

        // 打印未替换的键
        if (!not_replaced_keys.empty()) {
            std::cout << not_replaced_keys.size() << "个键未替换" << std::endl;
            for (const auto& key : not_replaced_keys) {
                std::cout << key << std::endl;
            }
        }

        return true;
    } else {
        return false;
    }
}

// 等待用户退出
void wait_for_exit() {
    std::cout << "\n按回车键退出...";
    std::cin.get();
}

int main() {
    try {
        std::cout << "===批量替换工具===" << std::endl;

        // 列出所有txt文件
        auto txt_files = list_txt_files();
        if (txt_files.empty()) {
            wait_for_exit();
            return 1;
        }

        // 选择要写入的文件
        std::cout << "\n选择要写入的文件(将直接修改此文件):" << std::endl;
        auto write_file_path = select_file("请输入数字选择:", txt_files);
        if (write_file_path.empty()) {
            wait_for_exit();
            return 1;
        }

        // 选择要对比的文件
        std::cout << "\n选择要对比的文件:" << std::endl;
        auto compare_file_path = select_file("请输入数字选择:", txt_files);
        if (compare_file_path.empty()) {
            wait_for_exit();
            return 1;
        }

        // 选择要替换文本的文件
        std::cout << "\n选择要替换文本的文件:" << std::endl;
        auto replace_file_path = select_file("请输入数字选择:", txt_files);
        if (replace_file_path.empty()) {
            wait_for_exit();
            return 1;
        }

        // 确认选择
        std::cout << "\n您选择了:" << std::endl;
        std::cout << "要写入的文件:" << write_file_path.filename().string() << std::endl;
        std::cout << "要对比的文件:" << compare_file_path.filename().string() << std::endl;
        std::cout << "要替换文本的文件:" << replace_file_path.filename().string() << std::endl;

        std::cout << "是否继续?(y/n):";
        std::string confirm;
        std::getline(std::cin, confirm);

        if (confirm != "y" && confirm != "Y") {
            std::cout << "操作已取消" << std::endl;
            wait_for_exit();
            return 1;
        }

        // 处理文件
        std::cout << "\n开始处理文件..." << std::endl;
        if (process_files(write_file_path, compare_file_path, replace_file_path)) {
            std::cout << "处理完成!" << std::endl;
        } else {
            std::cout << "处理失败!" << std::endl;
            wait_for_exit();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cout << "发生错误:" << e.what() << std::endl;
        wait_for_exit();
        return 1;
    }

    wait_for_exit();
    return 0;
}
