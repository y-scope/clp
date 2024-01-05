#include <boost/filesystem.hpp>

int main() {
    boost::filesystem::path path_prefix_to_remove;
    std::string path;
    std::vector<std::string> empty_directory_paths;
    std::string path_without_prefix;
    boost::filesystem::is_directory(path);
    boost::filesystem::is_empty(path);
    boost::filesystem::recursive_directory_iterator iter(path, boost::filesystem::symlink_option::recurse);
    boost::filesystem::recursive_directory_iterator end;
    boost::filesystem::is_directory(iter->path());
    boost::filesystem::is_empty(iter->path());

    return 0;
}
