set pagination off
set confirm off
target remote 127.0.0.1:2345
break main
continue
# The minimal wrapper has no C frame; use its actual 68000 return address.
set $entry_return = *(unsigned long *)$sp
tbreak *$entry_return
continue
if $d0 != 20
    printf "Undersized stack returned %lu instead of 20\n", $d0
    quit 1
end
printf "DOWNWELL_SMALL_STACK_REJECTED\n"
shell touch .run/gdb-ready.marker
detach
quit
