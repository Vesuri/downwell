set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellProfileReady
continue
printf "AMMO frames=%lu comparisons=%lu prepared=%lu pixel_failures=%lu (instrumented; not timing)\n", gDownwellProfileFrames[3],gDownwellAmmoChecks,gDownwellAmmoFactoredUses,gDownwellAmmoFailures
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[3] != gDownwellProfileTargetFrames || gDownwellAmmoChecks < 100 || gDownwellAmmoFactoredUses < 100 || gDownwellAmmoFailures != 0
    printf "AMMO comparison failed or coverage incomplete\n"
    quit 1
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
