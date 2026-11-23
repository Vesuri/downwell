set pagination off
set confirm off
target remote 127.0.0.1:2345
# CENSUS=1 profile.sh entry|transition: the probes routes as FPSCOUNT census
# builds. No stop until the profile freeze; then read the census ring once.
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1
    printf "Census route did not reach the profile freeze\n"
    quit 1
end
printf "PROFILE census route: target_scene=%lu frames=%lu,%lu,%lu,%lu\n", gDownwellProfileTargetScene, gDownwellProfileFrames[0], gDownwellProfileFrames[1], gDownwellProfileFrames[2], gDownwellProfileFrames[3]
source census_dump.gdb
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
