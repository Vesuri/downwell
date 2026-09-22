set pagination off
target remote 127.0.0.1:2345
break downwellFrameLoopReady
continue
printf "DOWNWELL_FRAME_LOOP_READY\n"
detach
quit
