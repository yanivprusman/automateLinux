echo -e "${BLUE}theRealPath output:${NC}"
theRealPath | sed 's/^/\t/'
echo
echo -e "${BLUE}printTheRealPath output:${NC}"
printTheRealPath | sed 's/^/\t/'
