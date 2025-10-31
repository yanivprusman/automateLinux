#include "buffer.hpp"
#include <algorithm>
#include <vector>
#include <string>

namespace termcontrol {

Buffer::Buffer(int rows, int cols) : rows(rows), cols(cols) {
    using std::vector;
    data.resize(rows, vector<char>(cols, ' '));
}

void Buffer::write(const std::string& str, int row, int col) {
    if (row < 0 || row >= rows || col < 0) return;
    
    for (std::size_t i = 0; i < str.length() && col < cols; i++, col++) {
        data[row][col] = str[i];
    }
}

void Buffer::clear() {
    for (auto& row : data) {
        std::fill(row.begin(), row.end(), ' ');
    }
}

void Buffer::resize(int new_rows, int new_cols) {
    data.resize(new_rows);
    for (auto& row : data) {
        row.resize(new_cols, ' ');
    }
    rows = new_rows;
    cols = new_cols;
}

char Buffer::at(int row, int col) const {
    if (row < 0 || row >= rows || col < 0 || col >= cols) {
        return ' ';
    }
    return data[row][col];
}

std::string Buffer::getLine(int row) const {
    if (row < 0 || row >= rows) {
        return std::string();
    }
    return std::string(data[row].begin(), data[row].end());
}

} // namespace termcontrol