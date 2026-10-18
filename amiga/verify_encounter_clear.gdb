set pagination off
set confirm off
target remote 127.0.0.1:2345
set $encounter_clear = 1
source verify_clear.gdb
