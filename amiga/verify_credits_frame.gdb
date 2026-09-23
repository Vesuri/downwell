set pagination off
target remote 127.0.0.1:2345
break downwellCreditsFrameReady
continue
# Audit the actual published program, not only its source raster.
set $i = 0
set $seen = 0
set $p1 = 0
set $p2 = 0
while $i < 400 && gDownwellCreditsCopper[$i] != 0xffff
    set $reg = gDownwellCreditsCopper[$i]
    set $value = gDownwellCreditsCopper[$i+1]
    if $reg == 0xe0
        set $p1 = $p1 | ($value << 16)
        set $seen = $seen | 1
    end
    if $reg == 0xe2
        set $p1 = $p1 | $value
        set $seen = $seen | 2
    end
    if $reg == 0xe4
        set $p2 = $p2 | ($value << 16)
        set $seen = $seen | 4
    end
    if $reg == 0xe6
        set $p2 = $p2 | $value
        set $seen = $seen | 8
    end
    if $reg == 0x108 && $value == 4
        set $seen = $seen | 16
    end
    if $reg == 0x10a && $value == 4
        set $seen = $seen | 32
    end
    if $reg == 0x92 && $value == 0x30
        set $seen = $seen | 64
    end
    if $reg == 0x94 && $value == 0xd8
        set $seen = $seen | 128
    end
    if $reg == 0x100 && $value == 0x2001
        set $seen = $seen | 256
    end
    set $i = $i + 2
end
if $i >= 400 || $seen != 511 || $p1 != (unsigned long)gDownwellCreditsFrame+2 || $p2 != (unsigned long)gDownwellCreditsFrame+13632+2
    printf "Invalid credits DMA setup: seen=%lu BPL1=%lx BPL2=%lx\n", $seen, $p1, $p2
    quit 1
end
printf "CREDITS_DMA_OK: both planes, 44-byte fetch, 48-byte stride\n"
dump binary memory .run/credits-frame.bin gDownwellCreditsFrame gDownwellCreditsFrame+27264
printf "DOWNWELL_CREDITS_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
