set pagination off
target remote 127.0.0.1:2345
break downwellSplashFrameReady
continue
dump binary memory .run/splash-frame.bin gDownwellSplashFrame gDownwellSplashFrame+12496
printf "DOWNWELL_SPLASH_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
