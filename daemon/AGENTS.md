inside classes and functions no empty lines.
do not add comments, but do not delete existing comments.
keep indenting code like:
std::string getTtyFromPid(pid_t pid) {
    std::string fdPath = "/proc/" + std::to_string(pid) + "/fd";
}
Adding Commands:
- Add `#define COMMAND_NAME "commandName"` to `include/common.h`
- Add `CommandSignature(COMMAND_NAME, {required, args})` to `COMMAND_REGISTRY[]` in `include/common.h`
- Create `CmdResult handleName(const json& command)` handler function in `src/mainCommand.cpp`
- Add `{COMMAND_NAME, handleName}` to `COMMAND_HANDLERS[]` array in `src/mainCommand.cpp`
- Add command to HELP_MESSAGE in `src/mainCommand.cpp`
- Update `commands` variable and case statement in `terminal/completions/daemon.bash`