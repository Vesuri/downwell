set pagination off
target remote 127.0.0.1:2345
break downwellControlsFrameReady
continue
# Static 160-pixel window must show source x=0, not crop an early fetch word.
set $i = 0
set $seen = 0
while $i < 400 && gDownwellControlsCopper[$i] != 0xffff
    set $reg = gDownwellControlsCopper[$i]
    set $value = gDownwellControlsCopper[$i+1]
    if $reg == 0x92 && $value == 0x60
        set $seen = $seen | 1
    end
    if $reg == 0x94 && $value == 0xa8
        set $seen = $seen | 2
    end
    if $reg == 0x102 && $value == 0
        set $seen = $seen | 4
    end
    if $reg == 0x108 && $value == 2
        set $seen = $seen | 8
    end
    if $reg == 0x10a && $value == 2
        set $seen = $seen | 16
    end
    if $reg == 0x8e && $value == 0x12d1
        set $seen = $seen | 32
    end
    if $reg == 0x90 && $value == 0x2e71
        set $seen = $seen | 64
    end
    set $i = $i + 2
end
if $i >= 400 || $seen != 127
    printf "Invalid static controls fetch/window setup: %lu\n", $seen
    quit 1
end
printf "CONTROLS_DMA_OK: 160 visible pixels, 20-byte fetch, 22-byte stride\n"
dump binary memory .run/controls-frame.bin gDownwellControlsFrame gDownwellControlsFrame+12496
printf "DOWNWELL_CONTROLS_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
