#ifndef DOWNWELL_CORE_INSTALLED_TABLES_H
#define DOWNWELL_CORE_INSTALLED_TABLES_H
// WI.3: builds that define DOWNWELL_INSTALLED_TABLES (the Amiga) read the
// tables derived from the player's files from the installed Downwell.tables
// (tools/install-table-headers.py): each table is then a bound pointer with
// a name_count constant instead of a linked array. Other builds link them.
#include <cstddef>
#ifdef DOWNWELL_INSTALLED_TABLES
#define DOWNWELL_TABLE_COUNT(table) (table##_count)
#define DOWNWELL_TABLE_OBJECT(table) (*(table))
#else
#define DOWNWELL_TABLE_COUNT(table) (sizeof(table) / sizeof((table)[0]))
#define DOWNWELL_TABLE_OBJECT(table) (table)
#endif
#endif
