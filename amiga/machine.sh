# Machine selection for run.sh, verify_startup.sh and verify_replay.sh.
# Source it after $FSUAE_COMMON, then call downwell_machine before
# fsuae_options. MACHINE (or VERIFY_MACHINE) is one of:
#   a1200       the target: unexpanded A1200 (68EC020 14 MHz, AGA, 2 MB Chip,
#               no Fast RAM, Kickstart 3.1);
#   a1200-fast  the same with 8 MB Fast RAM: proves that only DMA data is
#               allocated in Chip;
#   a500        A500 with 1 MiB Chip (ECS 8372A Agnus) and 8 MiB Fast;
#   a500-512k   A500 with 512 KiB Chip (OCS) and 8 MiB Fast.
# The A1200 machines use the shared AMIGA_SETUP presets, which also select
# the A1200 Kickstart and override the launch script's chipset argument.
# The default becomes a1200 once the game fits it (WR).
DOWNWELL_DEFAULT_MACHINE=a500

downwell_machine() {
  MACHINE="${MACHINE:-${VERIFY_MACHINE:-$DOWNWELL_DEFAULT_MACHINE}}"
  case "$MACHINE" in
    a1200|a1200-fast)
      AMIGA_SETUP="$MACHINE" MACHINE_CHIPSET=aga ;;
    a500)
      AMIGA_MODEL=A500 CHIP_KB=1024 FAST_KB=8192 MACHINE_CHIPSET=ecs_agnus ;;
    a500-512k)
      AMIGA_MODEL=A500 CHIP_KB=512 FAST_KB=8192 MACHINE_CHIPSET=ocs ;;
    *)
      echo "MACHINE must be a1200, a1200-fast, a500 or a500-512k" >&2
      return 1 ;;
  esac
  export MACHINE
  [ -z "${AMIGA_SETUP:-}" ] || export AMIGA_SETUP
}

downwell_machine_description() {
  case "$MACHINE" in
    a1200) echo "unexpanded A1200 (2 MB Chip, no Fast)" ;;
    a1200-fast) echo "A1200 (2 MB Chip, 8 MB Fast)" ;;
    a500) echo "A500 (1 MiB Chip, 8 MiB Fast)" ;;
    a500-512k) echo "A500 (512 KiB Chip, 8 MiB Fast)" ;;
  esac
}
