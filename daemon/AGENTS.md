inside classes and functions no empty lines.
do not add comments, but do not delete existing comments.
keep indenting code like:
std::string getTtyFromPid(pid_t pid) {
    std::string fdPath = "/proc/" + std::to_string(pid) + "/fd";
}
instead of spawing processes do coproc like in 
initCoproc in functions.sh