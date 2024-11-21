// ==================
// DEBUG OPTIONS
// ==================
#if defined(NDEBUG)
  #undef SQLITE_DEBUG
  #undef SQLITE_TEST
  #define SQLITE_OMIT_EXPLAIN                1
  #define SQLITE_OMIT_COMPILEOPTION_DIAGS    1
#else
  #define SQLITE_DEBUG                       1
  #define SQLITE_TEST                        1
#endif // NDEBUG
// ==================

#define YYSTACKDEPTH                        20
#define SQLITE_THREADSAFE                    0
#define SQLITE_MUTEX_APPDEF                  1
#define SQLITE_SMALL_STACK                   1
#define SQLITE_DISABLE_LFS                   1
#define SQLITE_DISABLE_DIRSYNC               1
#define SQLITE_DISABLE_FTS3_UNICODE          1
#define SQLITE_DISABLE_FTS4_DEFERRED         1
#define SQLITE_LIKE_DOESNT_MATCH_BLOBS       1
#define SQLITE_DEFAULT_CACHE_SIZE          -32
#define SQLITE_DEFAULT_MEMSTATUS             0
#define SQLITE_DEFAULT_MMAP_SIZE             0
#define SQLITE_DEFAULT_LOCKING_MODE          1 // EXCLUSIVE Allows WAL also on non-shm system
#define SQLITE_DEFAULT_PAGE_SIZE          4096
#define SQLITE_DEFAULT_PCACHE_INITSZ         4
#define SQLITE_MAX_DEFAULT_PAGE_SIZE      4096
#define SQLITE_POWERSAFE_OVERWRITE           0
#define SQLITE_SORTER_PMASZ                  4
#define SQLITE_MAX_EXPR_DEPTH                0
#define SQLITE_OMIT_AUTHORIZATION            1
#define SQLITE_OMIT_AUTOINIT                 1
#define SQLITE_OMIT_AUTOMATIC_INDEX          1
#define SQLITE_OMIT_AUTORESET                1
#define SQLITE_OMIT_AUTOVACUUM               1
#define SQLITE_OMIT_BETWEEN_OPTIMIZATION     1
#define SQLITE_OMIT_BLOB_LITERAL             1
#define SQLITE_OMIT_CHECK                    1
#define SQLITE_OMIT_DEPRECATED               1
#define SQLITE_OMIT_FOREIGN_KEY              1
#define SQLITE_OMIT_GET_TABLE                1
#define SQLITE_OMIT_INCRBLOB                 1
#define SQLITE_OMIT_INTEGRITY_CHECK          1
#define SQLITE_OMIT_LIKE_OPTIMIZATION        1
#define SQLITE_OMIT_LOAD_EXTENSION           1
#define SQLITE_OMIT_LOCALTIME                1
#define SQLITE_OMIT_LOOKASIDE                1
#define SQLITE_OMIT_PROGRESS_CALLBACK        1
#define SQLITE_OMIT_QUICKBALANCE             1
#define SQLITE_OMIT_SHARED_CACHE             1
#define SQLITE_OMIT_TCL_VARIABLE             1
#define SQLITE_OMIT_TEMPDB                   1
#define SQLITE_OMIT_TRACE                    1
#define SQLITE_OMIT_UTF16                    1
#define SQLITE_OMIT_XFER_OPT                 1

#define SQLITE_HAVE_ISNAN                    1
#define SQLITE_STRICT_SUBTYPE                1
#define SQLITE_OMIT_JSON                     1
#define SQLITE_OMIT_COMPLETE                 1
#define SQLITE_OMIT_DESERIALIZE              1
#define SQLITE_DISABLE_PAGECACHE_OVERFLOW_STATS 1
#define SQLITE_ENABLE_NULL_TRIM              1
#define SQLITE_DQS                           0
#define SQLITE_DEFAULT_SYNCHRONOUS           1 // Without WAL better 2
#define SQLITE_DEFAULT_WAL_SYNCHRONOUS       1
#define SQLITE_DEFAULT_JOURNAL_SIZE_LIMIT    65536
#define SQLITE_ENABLE_SNAPSHOT               1
#define SQLITE_DEFAULT_WAL_AUTOCHECKPOINT    100

// ESP32 Specific configuration for SQLite.
// Under an ARCHDEFINE to be able to compile
// on different archs (unix) to make testing easier.
#ifdef ESP32
  // #define SQLITE_ENABLE_8_3_NAMES              2 ///< Some FS like fat without long names can't handle normal '-journal' names
  #define SQLITE_4_BYTE_ALIGNED_MALLOC       1
  #define SQLITE_OS_OTHER                    1
#endif // ESP32
