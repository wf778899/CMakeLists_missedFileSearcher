﻿#include <iostream>

#include <filesystem>
#include <fstream>

#include <map>
#include <list>
#include <set>
#include <regex>
#include <future>

void printUsage()
{
    std::cout << "Usage:\n  cmake-missed-files-searcher path/to/project\n";
}

std::list<std::filesystem::path> gatherCMakeLists(std::filesystem::path const& workingDir)
{
    std::filesystem::recursive_directory_iterator root_recursive_it(workingDir);
    std::list<std::filesystem::path> cmake_root_paths;

    // Находим рекурсивно пути до всех CMakeLists.txt, начиная с текущей директории
    for (; root_recursive_it != std::filesystem::recursive_directory_iterator(); ++root_recursive_it)
    {
        if (root_recursive_it->path().filename() == "CMakeLists.txt")
            cmake_root_paths.push_back(root_recursive_it->path());
    }
    return cmake_root_paths;
}

std::list<std::filesystem::path> gatherSourceFilesFromDir(const std::filesystem::path& cmakePath)
{
    const std::set<std::string>& srcExtensions{ ".cpp", ".h", ".cc", ".mm", ".proto", ".rc" }; // TODO: move to config
    std::filesystem::recursive_directory_iterator recursive_dir_it(cmakePath.parent_path());

    std::list<std::filesystem::path> file_names;

    // Находим все названия .h и .cpp файлов в данной директории, включая подпапки. Заносим их в список (по нему будем проверять содержимое текущего CMakeLists.txt).
    for (; recursive_dir_it != std::filesystem::recursive_directory_iterator(); ++recursive_dir_it)
    {
        if (srcExtensions.find(recursive_dir_it->path().extension().string()) != srcExtensions.end()) {
            file_names.push_back(std::filesystem::relative(recursive_dir_it->path(), cmakePath.parent_path()));
        }
    }

    return file_names;
}

std::map<std::filesystem::path, std::list<std::filesystem::path>> findAbsensSourceFiles(std::list<std::filesystem::path> const& cmake_root_paths)
{
    std::map<std::filesystem::path, std::list<std::filesystem::path>> absent_fileNames;
    std::mutex resultMutex;
    std::vector<std::future<void>> waitList;  waitList.reserve(cmake_root_paths.size() * 50);

    // Сканируем каждую директорию, содержащую CMakeLists.txt
    for (const auto& cmakePath : cmake_root_paths)
    {
        const auto file_names = gatherSourceFilesFromDir(cmakePath);

        // Открываем текущий CMakeLists.txt.
        std::ifstream CMakeLists(cmakePath);
        if (!CMakeLists.is_open()) {
            std::cout << "Can't open the file " << cmakePath << std::endl;
            continue;
        }

        // Тупо копируем содержимое файла в строку
        const auto file_content = std::make_shared<std::string>((std::istreambuf_iterator<char>(CMakeLists)),
            std::istreambuf_iterator<char>());

        if (file_content->empty())
        {
            std::cout << "CMakeLists.txt is empty somewhat." << std::endl;
            continue;
        }

        // Ищем в полученном содержимом названия всех .h- и .cpp-файлов, которые содержит данная директория (включая субдиректории). Если что-то отсутствует - записываем что.
        for (const auto& fname : file_names)
        {
            waitList.push_back( std::async([&resultMutex, &absent_fileNames, fname, file_content, cmakePath]() {
                const std::regex filePattern(
                    "[\\t\\n\\r\\s]+" + fname.generic_string() + "[\\t\\n\\r\\s]+"
                );

                if (!std::regex_search(*file_content, filePattern) )
                {
                    std::lock_guard<std::mutex> lock(resultMutex);
                    absent_fileNames[cmakePath.parent_path()].push_back(fname.generic_string());
                }
            }) );
        }
    }

    return absent_fileNames;
}

int main(int argc, const char* argv[])
{
    std::filesystem::path workingDir = std::filesystem::current_path();
    if (argc == 2) {
        workingDir = std::filesystem::path(argv[1]);
    }
    else if (argc > 2) {
        printUsage();
        return -1;
    }

    const auto start = std::chrono::steady_clock::now();

    const auto absent_fileNames = findAbsensSourceFiles(
        gatherCMakeLists(workingDir));

    const auto finish = std::chrono::steady_clock::now();

    if (absent_fileNames.empty())
    {
        std::cout << "All files are present in all CMakeLists.txt or no CMakeLists.txt has been found." << std::endl;
        return 0;
    }

    for (const auto& pair : absent_fileNames) {
        std::string path = pair.first.generic_string();
        std::cout << pair.first.generic_string() << std::endl;

        for (const auto& fname : pair.second)
        {
            std::cout << '\t' << fname << std::endl;
        }
    }

    std::cout << "\nWorked " << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "mks\n";

    return 0;
}
