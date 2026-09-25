set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellShopLiveReady
continue
if gDownwellShopLiveCoverage != 255 || gDownwellShopLiveFailures != 0 || gDownwellShopLiveFrames < 13 || gDownwellShopLivePixelFailures != 0
    printf "Live shop failed: coverage=%lu failures=%lu frames=%lu pixel_failures=%lu\n", gDownwellShopLiveCoverage, gDownwellShopLiveFailures, gDownwellShopLiveFrames, gDownwellShopLivePixelFailures
    quit 1
end
if gDownwellStackGuardReady != 1 || gDownwellLiveGenerationFailed != 0
    printf "Live shop initialization/generation failed\n"
    quit 1
end
set $guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $i = 0
while $i < 64
    if $guard[$i] != 0x57ac6a4d
        printf "Live shop exhausted stack margin\n"
        quit 1
    end
    set $i = $i + 1
end
printf "Live shop: coverage=%lu, %lu retained/scalar frames, zero state/pixel failures, stack intact\n", gDownwellShopLiveCoverage, gDownwellShopLiveFrames
printf "DOWNWELL_GAMEPLAY_SHOP_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
