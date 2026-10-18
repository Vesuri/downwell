set pagination off
set confirm off
target remote 127.0.0.1:2345
set $encounter_backend = 1
source verify_results_restart.gdb
