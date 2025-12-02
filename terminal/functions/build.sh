
b(){
    ./build.sh "$@"
}
export -f b

m(){
    build/main "$@"
}
export -f m
