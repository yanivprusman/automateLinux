sudo evsieve --input /dev/input/event6 grab \
    --map btn:forward:1 key:1:1 key:2:1 \
    --map btn:forward:0 key:1:0 key:2:0 \
    --output