#!/bin/sh
for test in symlinks/echo1.sh symlinks/echo2.sh symlinks/echo_chain.sh \
    symlinks/dir1_link symlinks/dir1_link/echo.sh symlinks/parent_link \
    symlinks_back/echo1.sh "" "." "./" ".." "../" "../../" "../../../" \
    "../../../utilities/./theRealPath/../theRealPath/./symlinks/../dir1/./echo.sh"; do
    case "$test" in
        "") desc="No arguments (current directory)" ;;
        ".") desc="Current directory (.)" ;;
        "./") desc="Current directory (./)" ;;
        "..") desc="Parent directory (..)" ;;
        "../") desc="Parent directory (../)" ;;
        "../../") desc="Multiple parent traversal (../../)" ;;
        "../../../") desc="Triple parent traversal (../../../)" ;;
        "../../../utilities/./theRealPath/../theRealPath/./symlinks/../dir1/./echo.sh")
            desc="Complex path ($test)" ;;
        *) desc="Path: $test" ;;
    esac
    echo "Test: $desc"
    theRealPath "$test"
    echo
done

echo "\nTest 3: Chain of symlinks"
theRealPath symlinks/echo_chain.sh

echo "\nTest 4: Directory symlink"
theRealPath symlinks/dir1_link

echo "\nTest 5: Directory symlink with a file inside"
theRealPath symlinks/dir1_link/echo.sh

echo "\nTest 6: Parent directory symlink"
theRealPath symlinks/parent_link

echo "\nTest 7: Circular symlink reference"
theRealPath symlinks_back/echo1.sh

echo "\nTest 8: No arguments (current directory)"
theRealPath

echo "\nTest 9: Current directory (.)"
theRealPath .

echo "\nTest 10: Current directory (./)"
theRealPath ./

echo "\nTest 11: Parent directory (..)"
theRealPath ..

echo "\nTest 12: Parent directory (../)"
theRealPath ../

echo "\nTest 13: Multiple parent traversal (../../)"
theRealPath ../../

echo "\nTest 14: Triple parent traversal (../../../)"
theRealPath ../../../

echo "\nTest 15: Complex path (../../../utilities/./theRealPath/../theRealPath/./symlinks/../dir1/./echo.sh)"
theRealPath ../../../utilities/./theRealPath/../theRealPath/./symlinks/../dir1/./echo.sh