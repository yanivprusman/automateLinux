#pragma once

namespace termcontrol {

class Region {
public:
    Region(int top, int left, int bottom, int right);
    
    // Region properties
    int getTop() const { return top; }
    int getLeft() const { return left; }
    int getBottom() const { return bottom; }
    int getRight() const { return right; }
    
    // Region operations
    bool contains(int row, int col) const;
    void scroll(int lines);
    void clear();

private:
    int top;
    int left;
    int bottom;
    int right;
};