#pragma once

#include <vector>
#include <string>

namespace termcontrol {

class Buffer {
public:
    Buffer(int rows, int cols);
    
    // Buffer manipulation
    void write(const std::string& str, int row, int col);
    void clear();
    void resize(int rows, int cols);
    
    // Access
    char at(int row, int col) const;
    std::string getLine(int row) const;
    
    // Properties
    int getRows() const { return rows; }
    int getCols() const { return cols; }

private:
    int rows;
    int cols;
    std::vector<std::vector<char>> data;
};

} // namespace termcontrol