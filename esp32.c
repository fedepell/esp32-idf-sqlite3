/* From: https://chromium.googlesource.com/chromium/src.git/+/4.1.249.1050/third_party/sqlite/src/os_symbian.cc
 * https://github.com/spsoft/spmemvfs/tree/master/spmemvfs
 * http://www.sqlite.org/src/doc/trunk/src/test_demovfs.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfstrace.c
 * http://www.sqlite.org/src/doc/trunk/src/test_onefile.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfs.c
 * https://github.com/nodemcu/nodemcu-firmware/blob/master/app/sqlite3/esp8266.c
 **/

/**
 * Define this to enable some basic I/O statistics.
 * Useful for debugging only, do not use in production.
 */
#define DEBUG_IO_STATS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sqlite3.h>
#include <spi_flash_mmap.h>
#include <esp_system.h>
#include <esp_random.h>
#include <rom/ets_sys.h>
#include <sys/stat.h>

#undef dbg_printf
//#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_printf(...) 
#define CACHEBLOCKSZ 64
#define esp32_DEFAULT_MAXNAMESIZE 100

int esp32_Close(sqlite3_file*);
int esp32_Lock(sqlite3_file *, int);
int esp32_Unlock(sqlite3_file*, int);
int esp32_Sync(sqlite3_file*, int);
int esp32_Open(sqlite3_vfs*, const char *, sqlite3_file *, int, int*);
int esp32_Read(sqlite3_file*, void*, int, sqlite3_int64);
int esp32_Write(sqlite3_file*, const void*, int, sqlite3_int64);
int esp32_Truncate(sqlite3_file*, sqlite3_int64);
int esp32_Delete(sqlite3_vfs*, const char *, int);
int esp32_FileSize(sqlite3_file*, sqlite3_int64*);
int esp32_Access(sqlite3_vfs*, const char*, int, int*);
int esp32_FullPathname( sqlite3_vfs*, const char *, int, char*);
int esp32_CheckReservedLock(sqlite3_file*, int *);
int esp32_FileControl(sqlite3_file *, int, void*);
int esp32_SectorSize(sqlite3_file*);
int esp32_DeviceCharacteristics(sqlite3_file*);
void* esp32_DlOpen(sqlite3_vfs*, const char *);
void esp32_DlError(sqlite3_vfs*, int, char*);
void (*esp32_DlSym (sqlite3_vfs*, void*, const char*))(void);
void esp32_DlClose(sqlite3_vfs*, void*);
int esp32_Randomness(sqlite3_vfs*, int, char*);
int esp32_Sleep(sqlite3_vfs*, int);
int esp32_CurrentTime(sqlite3_vfs*, double*);

#ifdef DEBUG_IO_STATS
#include <esp_timer.h>

int esp32_Read_Stats(sqlite3_file*, void*, int, sqlite3_int64);
int esp32_Write_Stats(sqlite3_file*, const void*, int, sqlite3_int64);
int esp32_Sync_Stats(sqlite3_file*, int);
int esp32mem_Read_Stats(sqlite3_file*, void*, int, sqlite3_int64);
int esp32mem_Write_Stats(sqlite3_file*, const void*, int, sqlite3_int64);
#endif // DEBUG_IO_STATS

int esp32mem_Close(sqlite3_file*);
int esp32mem_Read(sqlite3_file*, void*, int, sqlite3_int64);
int esp32mem_Write(sqlite3_file*, const void*, int, sqlite3_int64);
int esp32mem_FileSize(sqlite3_file*, sqlite3_int64*);
int esp32mem_Sync(sqlite3_file*, int);

typedef struct st_linkedlist {
	uint16_t blockid;
	struct st_linkedlist *next;
	uint8_t data[CACHEBLOCKSZ];
} linkedlist_t, *pLinkedList_t;

typedef struct st_filecache {
	uint32_t size;
	linkedlist_t *list;
} filecache_t, *pFileCache_t;

typedef struct esp32_file {
	sqlite3_file base;
	FILE *fd;
	filecache_t *cache;
	const char *name;

	#ifdef DEBUG_IO_STATS
		struct {
			uint32_t wrt_cnt;
			uint32_t wrt_failed;
			int64_t  wrt_elapsed_us;
			uint32_t wrt_bytes;
			uint32_t wrt_bytes_max;
			uint32_t wrt_bytes_min;

			uint32_t read_cnt;
			uint32_t read_failed;
			int64_t  read_elapsed_us;
			uint32_t read_bytes;
			uint32_t read_bytes_max;
			uint32_t read_bytes_min;

			uint32_t r_mem_cnt;
			uint32_t r_mem_failed;
			int64_t  r_mem_elapsed_us;
			uint32_t r_mem_bytes;
			uint32_t r_mem_bytes_max;
			uint32_t r_mem_bytes_min;

			uint32_t w_mem_cnt;
			uint32_t w_mem_failed;
			int64_t  w_mem_elapsed_us;
			uint32_t w_mem_bytes;
			uint32_t w_mem_bytes_max;
			uint32_t w_mem_bytes_min;

			uint32_t sync_cnt;
			uint32_t sync_failed;
			int64_t  sync_elapsed_us;
		} stats;
	#endif // DEBUG_IO_STATS

} esp32_file;

sqlite3_vfs  esp32Vfs = {
	.iVersion          = 1,
	.szOsFile          = sizeof(esp32_file),
	.mxPathname        = 101,
	.pNext             = NULL,
	.zName             = "esp32",
	.pAppData          = 0,
	.xOpen             = esp32_Open,
	.xDelete           = esp32_Delete,
	.xAccess           = esp32_Access,
	.xFullPathname     = esp32_FullPathname,
	.xDlOpen           = esp32_DlOpen,
	.xDlError          = esp32_DlError,
	.xDlSym            = esp32_DlSym,
	.xDlClose          = esp32_DlClose,
	.xRandomness       = esp32_Randomness,
	.xSleep            = esp32_Sleep,
	.xCurrentTime      = esp32_CurrentTime,
	.xGetLastError     = 0,
	.xCurrentTimeInt64 = 0,
	.xSetSystemCall    = 0,
	.xGetSystemCall    = 0,
	.xNextSystemCall   = 0,
};

const sqlite3_io_methods esp32IoMethods = {
	.iVersion                 = 1,
	.xClose                   = esp32_Close,
	#ifdef DEBUG_IO_STATS
		.xRead                  = esp32_Read_Stats,
		.xWrite                 = esp32_Write_Stats,
		.xSync                  = esp32_Sync_Stats,
	#else
		.xRead                  = esp32_Read,
		.xWrite                 = esp32_Write,
		.xSync                  = esp32_Sync,
	#endif
	.xTruncate                = esp32_Truncate,
	.xFileSize                = esp32_FileSize,
	.xLock                    = esp32_Lock,
	.xUnlock                  = esp32_Unlock,
	.xCheckReservedLock       = esp32_CheckReservedLock,
	.xFileControl             = esp32_FileControl,
	.xSectorSize              = esp32_SectorSize,
	.xDeviceCharacteristics   = esp32_DeviceCharacteristics,
	.xShmMap                  = 0,
	.xShmLock                 = 0,
	.xShmBarrier              = 0,
	.xShmUnmap                = 0,
	.xFetch                   = 0,
	.xUnfetch                 = 0
};

const sqlite3_io_methods esp32MemMethods = {
	.iVersion                = 1,
	.xClose                   = esp32mem_Close,
	.xRead                    = esp32mem_Read,
	.xWrite                   = esp32mem_Write,
	.xTruncate                = esp32_Truncate,
	.xSync                    = esp32mem_Sync,
	.xFileSize                = esp32mem_FileSize,
	.xLock                    = esp32_Lock,
	.xUnlock                  = esp32_Unlock,
	.xCheckReservedLock       = esp32_CheckReservedLock,
	.xFileControl             = esp32_FileControl,
	.xSectorSize              = esp32_SectorSize,
	.xDeviceCharacteristics   = esp32_DeviceCharacteristics,
	.xShmMap                 = 0,
	.xShmLock                = 0,
	.xShmBarrier             = 0,
	.xShmUnmap               = 0,
	.xFetch                  = 0,
	.xUnfetch                = 0
};

uint32_t linkedlist_store (linkedlist_t **leaf, uint32_t offset, uint32_t len, const uint8_t *data) {
	const uint8_t blank[CACHEBLOCKSZ] = { 0 };
	uint16_t blockid = offset/CACHEBLOCKSZ;
	linkedlist_t *block;

	if (!memcmp(data, blank, CACHEBLOCKSZ))
		return len;

	block = *leaf;
	if (!block || ( block->blockid != blockid ) ) {
		block = (linkedlist_t *) sqlite3_malloc ( sizeof( linkedlist_t ) );
		if (!block)
			return SQLITE_NOMEM;

		memset (block->data, 0, CACHEBLOCKSZ);
		block->blockid = blockid;
	}

	if (!*leaf) {
		*leaf = block;
		block->next = NULL;
	} else if (block != *leaf) {
		if (block->blockid > (*leaf)->blockid) {
			block->next = (*leaf)->next;
			(*leaf)->next = block;
		} else {
			block->next = (*leaf);
			(*leaf) = block;
		}
	}

	memcpy (block->data + offset%CACHEBLOCKSZ, data, len);

	return len;
}

uint32_t filecache_pull (pFileCache_t cache, uint32_t offset, uint32_t len, uint8_t *data) {
	uint16_t i;
	float blocks;
	uint32_t r = 0;

	blocks = ( offset % CACHEBLOCKSZ + len ) / (float) CACHEBLOCKSZ;
	if (blocks == 0.0)
		return 0;
	if (!cache->list)
		return 0;

	if (( blocks - (int) blocks) > 0.0)
		blocks = blocks + 1.0;

	for (i = 0; i < (uint16_t) blocks; i++) {
		uint16_t round;
		float relablock;
		linkedlist_t *leaf;
		uint32_t relaoffset, relalen;
		uint8_t * reladata = (uint8_t*) data;

		relalen = len - r;

		reladata = reladata + r;
		relaoffset = offset + r;

		round = CACHEBLOCKSZ - relaoffset%CACHEBLOCKSZ;
		if (relalen > round) relalen = round;

		for (leaf = cache->list; leaf && leaf->next; leaf = leaf->next) {
			if ( ( leaf->next->blockid * CACHEBLOCKSZ ) > relaoffset )
				break;
		}

		relablock = relaoffset/((float)CACHEBLOCKSZ) - leaf->blockid;

		if ( ( relablock >= 0 ) && ( relablock < 1 ) )
			memcpy (data + r, leaf->data + (relaoffset % CACHEBLOCKSZ), relalen);

		r = r + relalen;
	}

	return 0;
}

uint32_t filecache_push (pFileCache_t cache, uint32_t offset, uint32_t len, const uint8_t *data) {
	uint16_t i;
	float blocks;
	uint32_t r = 0;
	uint8_t updateroot = 0x1;

	blocks = ( offset % CACHEBLOCKSZ + len ) / (float) CACHEBLOCKSZ;

	if (blocks == 0.0)
		return 0;

	if (( blocks - (int) blocks) > 0.0)
		blocks = blocks + 1.0;

	for (i = 0; i < (uint16_t) blocks; i++) {
		uint16_t round;
		uint32_t localr;
		linkedlist_t *leaf;
		uint32_t relaoffset, relalen;
		uint8_t * reladata = (uint8_t*) data;

		relalen = len - r;

		reladata = reladata + r;
		relaoffset = offset + r;

		round = CACHEBLOCKSZ - relaoffset%CACHEBLOCKSZ;
		if (relalen > round) relalen = round;

		for (leaf = cache->list; leaf && leaf->next; leaf = leaf->next) {
			if ( ( leaf->next->blockid * CACHEBLOCKSZ ) > relaoffset )
				break;
			updateroot = 0x0;
		}

		localr = linkedlist_store(&leaf, relaoffset, (relalen > CACHEBLOCKSZ) ? CACHEBLOCKSZ : relalen, reladata);
		if (localr == SQLITE_NOMEM)
			return SQLITE_NOMEM;

		r = r + localr;

		if (updateroot & 0x1)
			cache->list = leaf;
	}

	if (offset + len > cache->size)
		cache->size = offset + len;

	return r;
}

void filecache_free (pFileCache_t cache) {
	pLinkedList_t ll = cache->list, next;

	while (ll != NULL) {
		next = ll->next;
		sqlite3_free (ll);
		ll = next;
	}
}

int esp32mem_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
	int32_t ofst;
	esp32_file *file = (esp32_file*) id;
	ofst = (int32_t)(offset & 0x7FFFFFFF);

	filecache_pull (file->cache, ofst, amount, (uint8_t *) buffer);

	dbg_printf("esp32mem_Read: %s [%ld] [%d] OK\n", file->name, ofst, amount);
	return SQLITE_OK;
}

int esp32mem_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
	int32_t ofst;
	esp32_file *file = (esp32_file*) id;

	ofst = (int32_t)(offset & 0x7FFFFFFF);

	filecache_push (file->cache, ofst, amount, (const uint8_t *) buffer);

	dbg_printf("esp32mem_Write: %s [%ld] [%d] OK\n", file->name, ofst, amount);
	return SQLITE_OK;
}

int esp32mem_Sync(sqlite3_file *id, int flags)
{
	dbg_printf("esp32mem_Sync: OK\n");
	return  SQLITE_OK;
}

int esp32mem_FileSize(sqlite3_file *id, sqlite3_int64 *size)
{
	esp32_file *file = (esp32_file*) id;

	*size = 0LL | file->cache->size;
	dbg_printf("esp32mem_FileSize: %s [%ld] OK\n", file->name, file->cache->size);
	return SQLITE_OK;
}

int esp32_Open( sqlite3_vfs * vfs, const char * path, sqlite3_file * file, int flags, int * outflags )
{
	esp32_file *p = (esp32_file*) file;
	const char *mode = "r";

	if ( path == NULL ) return SQLITE_IOERR;
	dbg_printf("esp32_Open: 0o %s %s\n", path, mode);

	if( flags&SQLITE_OPEN_READWRITE || flags&SQLITE_OPEN_MAIN_JOURNAL ) {
		int result;
		if (SQLITE_OK != esp32_Access(vfs, path, flags, &result))
			return SQLITE_CANTOPEN;

		mode = (result == 1) ? "r+" : "w+";
	}

	dbg_printf("esp32_Open: 1o %s %s\n", path, mode);
	memset (p, 0, sizeof(esp32_file));

	p->name = path;

	if( flags&SQLITE_OPEN_MAIN_JOURNAL ) {
		p->fd = 0;
		p->cache = (filecache_t *) sqlite3_malloc(sizeof (filecache_t));
		if (! p->cache )
			return SQLITE_NOMEM;
		memset (p->cache, 0, sizeof(filecache_t));

		p->base.pMethods = &esp32MemMethods;
		dbg_printf("esp32_Open: 2o %s MEM OK\n", p->name);
		return SQLITE_OK;
	}

	p->fd = fopen(path, mode);
	if (!p->fd) {
		return SQLITE_CANTOPEN;
	}

	#ifdef DEBUG_IO_STATS
		p->stats.wrt_bytes_min  = 0xFFFFFFFF;
		p->stats.read_bytes_min = 0xFFFFFFFF;
	#endif // DEBUG_IO_STATS

	p->base.pMethods = &esp32IoMethods;
	dbg_printf("esp32_Open: 2o %s OK\n", p->name);
	return SQLITE_OK;
}

int esp32_Close(sqlite3_file *id)
{
	esp32_file *file = (esp32_file*) id;

	// Journal file doesn't have a file descriptor opened
	int rc = file->fd ? fclose(file->fd) : SQLITE_OK;
	dbg_printf("esp32_Close: %s %d\n", file->name, rc);

	if (file->cache) {
		dbg_printf("esp32_Close: Freeing cache\n");
		filecache_free(file->cache);
		sqlite3_free (file->cache);
	}
		float read_ratio = read_succ * 100 / (float) file->stats.read_cnt;

		uint32_t sync_succ = file->stats.sync_cnt - file->stats.sync_failed;
		float sync_ratio = sync_succ * 100 / (float) file->stats.sync_cnt;

		uint32_t r_mem_succ = file->stats.r_mem_cnt - file->stats.r_mem_failed;
		float r_mem_ratio = r_mem_succ * 100 / (float) file->stats.r_mem_cnt;

		uint32_t w_mem_succ = file->stats.w_mem_cnt - file->stats.w_mem_failed;
		float w_mem_ratio = w_mem_succ * 100 / (float) file->stats.w_mem_cnt;

		printf("esp32_Close: IO-STATS '%s'\n"
					 "  syncs      = %6.2f %% %4lu / %4lu - %9lld us (avg: %6lld us)\n"
					 "  writes     = %6.2f %% %4lu / %4lu - %9lld us (avg: %6lld us)\n"
					 "  reads      = %6.2f %% %4lu / %4lu - %9lld us (avg: %6lld us)\n"
					 "  mem writes = %6.2f %% %4lu / %4lu - %9lld us (avg: %6lld us)\n"
					 "  mem reads  = %6.2f %% %4lu / %4lu - %9lld us (avg: %6lld us)\n"
					 "  W Bytes    = Tot: %9lu B (min: %4lu, max: %4lu)\n"
					 "  R Bytes    = Tot: %9lu B (min: %4lu, max: %4lu)\n"
					 "  mem WB		 = Tot: %9lu B (min: %4lu)\n"
					 "  mem RB		 = Tot: %9lu B (min: %4lu)\n",
					 file->name,
					 sync_ratio, sync_succ,   file->stats.sync_cnt,       file->stats.sync_elapsed_us,  (file->stats.sync_elapsed_us  / file->stats.sync_cnt),
					 wrt_ratio,  wrt_succ,    file->stats.wrt_cnt,        file->stats.wrt_elapsed_us,   (file->stats.wrt_elapsed_us   / file->stats.wrt_cnt),
					 read_ratio, read_succ,   file->stats.read_cnt,       file->stats.read_elapsed_us,  (file->stats.read_elapsed_us  / file->stats.read_cnt),
					 w_mem_ratio, w_mem_succ, file->stats.w_mem_cnt,      file->stats.w_mem_elapsed_us, (file->stats.w_mem_elapsed_us / file->stats.w_mem_cnt),
					 r_mem_ratio, r_mem_succ, file->stats.r_mem_cnt,      file->stats.r_mem_elapsed_us, (file->stats.r_mem_elapsed_us / file->stats.r_mem_cnt),
					 file->stats.wrt_bytes,   file->stats.wrt_bytes_min,  file->stats.wrt_bytes_max,
					 file->stats.read_bytes,  file->stats.read_bytes_min, file->stats.read_bytes_max,
					 file->stats.w_mem_bytes, file->stats.w_mem_bytes_min,
					 file->stats.r_mem_bytes, file->stats.r_mem_bytes_min
					);
	#endif // DEBUG_IO_STATS

	file->name = NULL;
	return rc ? SQLITE_IOERR_CLOSE : SQLITE_OK;
}

int esp32_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
	size_t nRead;
	int32_t ofst, iofst;
	esp32_file *file = (esp32_file*) id;

	iofst = (int32_t)(offset & 0x7FFFFFFF);

	dbg_printf("esp32_Read: 1r %s %d %lld[%ld] \n", file->name, amount, offset, iofst);
	ofst = fseek(file->fd, iofst, SEEK_SET);
	if (ofst != 0) {
	    dbg_printf("esp32_Read: 2r %ld != %ld FAIL\n", ofst, iofst);
		memset(buffer, 0, amount);
		return SQLITE_IOERR_SHORT_READ /* SQLITE_IOERR_SEEK */;
	}

	nRead = fread(buffer, 1, amount, file->fd);
	if ( nRead == amount ) {
	    dbg_printf("esp32_Read: 3r %s %u %d OK\n", file->name, nRead, amount);
		return SQLITE_OK;
	} else if ( nRead < amount ) {
	    dbg_printf("esp32_Read: 3r %s %u %d FAIL\n", file->name, nRead, amount);
		memset((uint8_t *)(buffer) + nRead, 0, amount - nRead);
		return SQLITE_IOERR_SHORT_READ;
	}

	dbg_printf("esp32_Read: 4r %s FAIL\n", file->name);
	return SQLITE_IOERR_READ;
}

int esp32_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
	size_t nWrite;
	int32_t ofst, iofst;
	esp32_file *file = (esp32_file*) id;

	iofst = (int32_t)(offset & 0x7FFFFFFF);

	dbg_printf("esp32_Write: 1w %s %d %lld[%ld] \n", file->name, amount, offset, iofst);
	ofst = fseek(file->fd, iofst, SEEK_SET);
	if (ofst != 0) {
		return SQLITE_IOERR_SEEK;
	}

	nWrite = fwrite(buffer, 1, amount, file->fd);
	if ( nWrite != amount ) {
		dbg_printf("esp32_Write: 2w %s %u %d\n", file->name, nWrite, amount);
		return SQLITE_IOERR_WRITE;
	}

	dbg_printf("esp32_Write: 3w %s OK\n", file->name);
	return SQLITE_OK;
}

int esp32_Truncate(sqlite3_file *id, sqlite3_int64 bytes)
{
	//int fno = fileno(file->fd);
	//if (fno == -1)
	//	return SQLITE_IOERR_TRUNCATE;
	//if (ftruncate(fno, 0))
	//	return SQLITE_IOERR_TRUNCATE;

	dbg_printf("esp32_Truncate:\n");
	return SQLITE_OK;
}

int esp32_Delete( sqlite3_vfs * vfs, const char * path, int syncDir )
{
	int32_t rc = remove( path );
	if (rc)
		return SQLITE_IOERR_DELETE;

	dbg_printf("esp32_Delete: %s OK\n", path);
	return SQLITE_OK;
}

int esp32_FileSize(sqlite3_file *id, sqlite3_int64 *size)
{
	esp32_file *file = (esp32_file*) id;
	dbg_printf("esp32_FileSize: %s: ", file->name);
	struct stat st;
	int fno = fileno(file->fd);
	if (fno == -1)
		return SQLITE_IOERR_FSTAT;
	if (fstat(fno, &st))
		return SQLITE_IOERR_FSTAT;
    *size = st.st_size;
	dbg_printf(" %ld[%lld]\n", st.st_size, *size);
	return SQLITE_OK;
}

int esp32_Sync(sqlite3_file *id, int flags)
{
	esp32_file *file = (esp32_file*) id;

	int rc = fflush( file->fd );
        fsync(fileno(file->fd));
        dbg_printf("esp32_Sync( %s: ): %d \n",file->name, rc);

	return rc ? SQLITE_IOERR_FSYNC : SQLITE_OK;
}

int esp32_Access( sqlite3_vfs * vfs, const char * path, int flags, int * result )
{
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	int rc = stat( path, &st );
	*result = ( rc != -1 );

	dbg_printf("esp32_Access: %s %d %d %ld\n", path, *result, rc, st.st_size);
	return SQLITE_OK;
}

int esp32_FullPathname( sqlite3_vfs * vfs, const char * path, int len, char * fullpath )
{
	//structure stat does not have name.
	//struct stat st;
	//int32_t rc = stat( path, &st );
	//if ( rc == 0 ){
	//	strncpy( fullpath, st.name, len );
	//} else {
	//	strncpy( fullpath, path, len );
	//}

	// As now just copy the path
	strncpy( fullpath, path, len );
	fullpath[ len - 1 ] = '\0';

	dbg_printf("esp32_FullPathname: %s\n", fullpath);
	return SQLITE_OK;
}

int esp32_Lock(sqlite3_file *id, int lock_type)
{
	dbg_printf("esp32_Lock:Not locked\n");
	return SQLITE_OK;
}

int esp32_Unlock(sqlite3_file *id, int lock_type)
{
	dbg_printf("esp32_Unlock:\n");
	return SQLITE_OK;
}

int esp32_CheckReservedLock(sqlite3_file *id, int *result)
{
	*result = 0;
	dbg_printf("esp32_CheckReservedLock:\n");
	return SQLITE_OK;
}

int esp32_FileControl(sqlite3_file *id, int op, void *arg)
{
	dbg_printf("esp32_FileControl:\n");
	return SQLITE_OK;
}

int esp32_SectorSize(sqlite3_file *id)
{
	dbg_printf("esp32_SectorSize:\n");
	return SPI_FLASH_SEC_SIZE;
}

int esp32_DeviceCharacteristics(sqlite3_file *id)
{
	dbg_printf("esp32_DeviceCharacteristics:\n");
	return 0;
}

void * esp32_DlOpen( sqlite3_vfs * vfs, const char * path )
{
	dbg_printf("esp32_DlOpen:\n");
	return NULL;
}

void esp32_DlError( sqlite3_vfs * vfs, int len, char * errmsg )
{
	dbg_printf("esp32_DlError:\n");
	return;
}

void ( * esp32_DlSym ( sqlite3_vfs * vfs, void * handle, const char * symbol ) ) ( void )
{
	dbg_printf("esp32_DlSym:\n");
	return NULL;
}

void esp32_DlClose( sqlite3_vfs * vfs, void * handle )
{
	dbg_printf("esp32_DlClose:\n");
	return;
}

int esp32_Randomness( sqlite3_vfs * vfs, int len, char * buffer )
{
	long rdm;
	int sz = 1 + (len / sizeof(long));
	char a_rdm[sz * sizeof(long)];
	while (sz--) {
        rdm = esp_random();
		memcpy(a_rdm + sz * sizeof(long), &rdm, sizeof(long));
	}
	memcpy(buffer, a_rdm, len);
	dbg_printf("esp32_Randomness\n");
	return SQLITE_OK;
}

int esp32_Sleep( sqlite3_vfs * vfs, int microseconds )
{
	ets_delay_us(microseconds);
	dbg_printf("esp32_Sleep:\n");
	return SQLITE_OK;
}

int esp32_CurrentTime( sqlite3_vfs * vfs, double * result )
{
	time_t t = time(NULL);
	*result = t / 86400.0 + 2440587.5;
	// This is stubbed out until we have a working RTCTIME solution;
	// as it stood, this would always have returned the UNIX epoch.
	//*result = 2440587.5;
	dbg_printf("esp32_CurrentTime: %g\n", *result);
	return SQLITE_OK;
}

#ifdef DEBUG_IO_STATS
	int esp32_Read_Stats(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset) {
		esp32_file *file = (esp32_file*) id;
		file->stats.read_cnt++;

		if (file->stats.read_bytes_min > amount) {
			file->stats.read_bytes_min = amount;
		}
		if (file->stats.read_bytes_max < amount) {
			file->stats.read_bytes_max = amount;
		}

		int64_t start = esp_timer_get_time();
		int rc = esp32_Read(id, buffer, amount, offset);
		file->stats.read_elapsed_us += esp_timer_get_time() - start;

		if (rc == SQLITE_OK) {
			file->stats.read_bytes += amount;
		} else {
			file->stats.read_failed++;
		}
		return rc;
	}

	int esp32_Write_Stats(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset) {
		esp32_file *file = (esp32_file*) id;
		file->stats.wrt_cnt++;

		if (file->stats.wrt_bytes_min > amount) {
			file->stats.wrt_bytes_min = amount;
		}
		if (file->stats.wrt_bytes_max < amount) {
			file->stats.wrt_bytes_max = amount;
		}

		int64_t start = esp_timer_get_time();
		int rc = esp32_Write(id, buffer, amount, offset);
		file->stats.wrt_elapsed_us += esp_timer_get_time() - start;

		if (rc == SQLITE_OK) {
			file->stats.wrt_bytes += amount;
		} else {
			file->stats.wrt_failed++;
		}
		return rc;
	}

	int esp32_Sync_Stats(sqlite3_file *id, int flags) {
		esp32_file *file = (esp32_file*) id;
		file->stats.sync_cnt++;

		int start = esp_timer_get_time();
		int rc = esp32_Sync(id, flags);
		file->stats.sync_elapsed_us += esp_timer_get_time() - start;

		if (rc != SQLITE_OK) {
			file->stats.sync_failed++;
		}
		return rc;
	}

	int esp32mem_Read_Stats(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset) {
		esp32_file *file = (esp32_file*) id;
		file->stats.r_mem_cnt++;

		if (file->stats.r_mem_bytes_min > amount) {
			file->stats.r_mem_bytes_min = amount;
		}
		if (file->stats.r_mem_bytes_max < amount) {
			file->stats.r_mem_bytes_max = amount;
		}

		int64_t start = esp_timer_get_time();
		int rc = esp32mem_Read(id, buffer, amount, offset);
		file->stats.r_mem_elapsed_us += esp_timer_get_time() - start;

		if (rc == SQLITE_OK) {
			file->stats.r_mem_bytes += amount;
		} else {
			file->stats.r_mem_failed++;
		}
		return rc;
	}

	int esp32mem_Write_Stats(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset) {
		esp32_file *file = (esp32_file*) id;
		file->stats.w_mem_cnt++;

		if (file->stats.w_mem_bytes_min > amount) {
			file->stats.w_mem_bytes_min = amount;
		}
		if (file->stats.w_mem_bytes_max < amount) {
			file->stats.w_mem_bytes_max = amount;
		}

		int64_t start = esp_timer_get_time();
		int rc = esp32mem_Write(id, buffer, amount, offset);
		file->stats.w_mem_elapsed_us += esp_timer_get_time() - start;

		if (rc == SQLITE_OK) {
			file->stats.w_mem_bytes += amount;
		} else {
			file->stats.w_mem_failed++;
		}
		return rc;
	}
#endif // DEBUG_IO_STATS

int sqlite3_os_init(void){
  sqlite3_vfs_register(&esp32Vfs, 1);
  return SQLITE_OK;
}

int sqlite3_os_end(void){
  return SQLITE_OK;
}
