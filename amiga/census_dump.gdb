# Sourced by `CENSUS=1 bash amiga/profile.sh MODE` just before the ready
# marker: read the W3.5 per-update work census once, after the measured run.
# tools/amiga-census-report.py decodes .run/census.bin with this line.
printf "CENSUS count=%lu capacity=%lu ticks_per_field=%lu\n", gDownwellCensusCount, gDownwellCensusCapacity, gDownwellCensusTicksPerField
dump binary memory .run/census.bin &gDownwellCensus[0] &gDownwellCensus[gDownwellCensusCapacity]
