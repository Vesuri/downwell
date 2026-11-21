# Source after connecting, before executing the program. Debugger-only:
# catches transient AllocMem/FreeMem changes, including inside DOS file I/O.
# SDK layout assertions live in framework/AmigaMemory.cpp. The normal ELF
# need not retain a MemHeader debug type. Deliberately limited to one Chip
# region, as in the 1 MiB (ECS Agnus) and 512 KiB A500 configurations.
set $chip_exec = (struct ExecBase *) *(unsigned long *)4
set $chip_node = (unsigned char *) $chip_exec->MemList.lh_Head
set $chip_header = (unsigned char *)0
while *(unsigned long *)$chip_node
  printf "MEMORY_REGION attributes=%u range=%08lx..%08lx free_at_entry=%lu\n", *(unsigned short *)($chip_node+14), *(unsigned long *)($chip_node+20), *(unsigned long *)($chip_node+24), *(unsigned long *)($chip_node+28)
  if *(unsigned short *)($chip_node+14) & 2
    if $chip_header
      printf "CHIP_AUDIT_ERROR: multiple Chip regions\n"
      quit 1
    end
    set $chip_header = $chip_node
  end
  set $chip_node = (unsigned char *) *(unsigned long *)$chip_node
end
if !$chip_header
  printf "CHIP_AUDIT_ERROR: no Chip region\n"
  quit 1
end
set $chip_free = (unsigned long *)($chip_header+28)
set $chip_minimum = *$chip_free
set $chip_changes = 0
set $chip_files = 0
define downwell-chip-snapshot
  set $chip_chunk = (unsigned char *) *(unsigned long *)($chip_header+16)
  set $chip_sum = 0
  set $chip_largest = 0
  set $chip_chunks = 0
  while $chip_chunk
    set $chip_bytes = *(unsigned long *)($chip_chunk+4)
    if $chip_bytes < 8 || ($chip_bytes & 7) || $chip_chunks >= 65536
      printf "CHIP_AUDIT_ERROR: invalid free chunk\n"
      quit 1
    end
    set $chip_sum = $chip_sum + $chip_bytes
    if $chip_bytes > $chip_largest
      set $chip_largest = $chip_bytes
    end
    set $chip_next = (unsigned char *) *(unsigned long *)$chip_chunk
    if $chip_next && $chip_next < $chip_chunk + $chip_bytes
      printf "CHIP_AUDIT_ERROR: overlapping/unordered free chunks\n"
      quit 1
    end
    set $chip_chunk = $chip_next
    set $chip_chunks = $chip_chunks + 1
  end
  if $chip_sum != *$chip_free
    printf "CHIP_AUDIT_ERROR: free list not at a consistent checkpoint\n"
    quit 1
  end
  printf "CHIP_CHECKPOINT free=%lu largest=%lu minimum_free=%lu changes=%lu file_loads=%lu\n", *$chip_free, $chip_largest, $chip_minimum, $chip_changes, $chip_files
  # W2.5 sound banks (AmigaSoundBanks::Bank: 0 none, 1 run, 2 front,
  # 3 results, 4 clear) as this checkpoint finds them.
  printf "SOUND_BANKS ready=%lu resident=%lu scene=%lu chip=%lu loads=%lu failures=%lu\n", gDownwellSoundReady, gDownwellSoundResidentBank, gDownwellSoundSceneBank, gDownwellSoundChipBytes, gDownwellSoundBankLoads, gDownwellSoundBankFailures
end
downwell-chip-snapshot
watch -l *$chip_free
commands
  silent
  set $chip_changes = $chip_changes + 1
  if *$chip_free < $chip_minimum
    set $chip_minimum = *$chip_free
  end
  continue
end
break AmigaPlatform::loadFile
commands
  silent
  set $chip_files = $chip_files + 1
  downwell-chip-snapshot
  continue
end
