tail -f ../log/log.txt | awk '
{
    # Shift lines up
    for(i=1; i<5; i++) lines[i] = lines[i+1]
    lines[5] = $0

    # Only start printing once buffer is full
    if(NR >= 5) {
        # Move cursor up 5 lines
        printf "\033[5A"

        # Print 5 lines
        for(i=1; i<=5; i++) printf "\r\033[K%s\n", lines[i]

        fflush()
    }
}'
