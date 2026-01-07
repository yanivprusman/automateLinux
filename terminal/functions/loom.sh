startLoomServerAndClient(){
    cd ~/coding/loom/server/build && \
    ./loom-server 4001 && \
    cd ~/coding/loom/client \
    npm run dev
}