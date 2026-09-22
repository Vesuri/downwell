set pagination off
target remote 127.0.0.1:2345
break downwellStartupReady
continue
printf "DOWNWELL_STARTUP_READY\n"
detach
quit
