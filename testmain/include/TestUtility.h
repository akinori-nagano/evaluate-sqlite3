#ifndef __TESTMAIN__TESTUTILITY_H__
#define __TESTMAIN__TESTUTILITY_H__

#ifdef __cplusplus
extern "C" {
#endif

	#include <string.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <time.h>

#ifndef FOR_PLATFORM_WINDOWS
	#include <unistd.h>
#endif

	#include "sqlite3.h"

#ifdef __cplusplus
}
#endif

#if defined(FOR_PLATFORM_WINDOWS)
	#define snprintf _snprintf_s
	#define random   rand
	#define mySleep(__SEC__)       Sleep((__SEC__) * 1000)
	#define myMicroSleep(__MSEC__) Sleep(__MSEC__)
#elif defined(FOR_PLATFORM_IOS) || defined(FOR_PLATFORM_ANDROID)
	#define mySleep(__SEC__)       usleep((__SEC__) * 1000)
	#define myMicroSleep(__MSEC__) usleep(__MSEC__)
#else
	#define mySleep(__SEC__)	sleep((__SEC__))
#endif

typedef enum {
	Ok = 0,
	Error,
	Redoing,
	NotFound,
} TestMainStatus;

typedef enum {
	Insert = 1,
	Update1 = 2,
	Update2 = 3,
	Update3 = 4,
	Table001KindMax = 5,
} Table001Kind;

#define TEST_DATA1_ID_MAX_COUNT          (3)
#define TEST_DATA_KIND_MAX_COUNT        ((int)Table001Kind::Table001KindMax - 1)
#define TEST_DATA_UPDATE_KIND_MAX_COUNT ((int)Table001Kind::Table001KindMax - 2)

#define TEST_DATA2_ID_MAX_COUNT          (20)

// TABLE t_test_001
typedef struct {
	// id INT
	int id;
	// kind INTEGER
	int kind;
	// contents_id TEXT
	char contents_id[1024];
	// contents_code VARCHAR(53)
	char contents_code[53 + 1];
	// hashed_id char(64)
	char hashed_id[64 + 1];
	// blob
	unsigned char terminal_value[1024];
	size_t terminal_value_sz;
	// updated_date datetime, '2017-12-05'
	char updated_date[32];
	// updated_at timestamp, '2017-12-05 03:34:56'
	char updated_at[64];
} Table001;

// TABLE t_test_002
typedef struct {
	// contents_code VARCHAR(53)
	char contents_code[53 + 1];
	// hashed_id char(64)
	char hashed_id[64 + 1];
	// id INT
	int id;
	// secret INTEGER
	int secret;
	// read_datetime timestamp, '2017-12-05 03:34:56'
	char read_datetime[64];
	// delete_status INTEGER
	int delete_status;
	// updated_at timestamp, '2017-12-05 03:34:56'
	char updated_at[64];
} Table002;


extern void TestInitialize(void);
extern Table001 *GetTest001Data(const int index, Table001Kind kind);
extern Table002 *GetTest002Data(const int id);


class TestUtility
{
	public:
		FILE *fpDebug;
		long threadID;
		sqlite3 *db;

		TestUtility(FILE *tti, long threadID);
		~TestUtility();

		static int IsThreadsafe(void);

		long SystemErapsedCount(void);
		void debugOut(const char *tag, const char *fn, const char *fmt, ...);

		TestMainStatus openDB(const char *tag, const char *dbPath);
		void closeDB(const char *tag);
		TestMainStatus startTransaction(const char *tag);
		TestMainStatus endTransaction(const char *tag, TestMainStatus status);
		void deleteAll(void);

		TestMainStatus insertTable001(Table001 *table);
		TestMainStatus updateTable001(Table001 *table);
		TestMainStatus selectTable001(const int id, Table001 *output);
		TestMainStatus deleteTable001(const int id);

		TestMainStatus insertTable002(Table002 *table, bool isTemp);
		TestMainStatus updateTable002(Table002 *table, bool isTemp);
		TestMainStatus selectTable002(const char *contents_code, bool isTemp, Table002 *output);
		TestMainStatus selectTable002Byid(const int id, bool isTemp, Table002 *output);
		TestMainStatus deleteTable002(const int id, bool isTemp);

		TestMainStatus selectAllView(Table002 **output, int *n_output);
		TestMainStatus selectViewById(const int id, Table002 *output);

	private:
		TestMainStatus __selectTable002UniqueRow(const char *sql, Table002 *output);
		void __copySqlite3ColumnTable002(sqlite3_stmt *stmt, Table002 *output);

		sqlite3_stmt * __sqlite3_prepare(const char *sql);
		int __sqlite3_step(sqlite3_stmt *stmt, const char *sTag, const int iTag, bool isRetry);
		TestMainStatus __sqlite3_column_text(sqlite3_stmt *stmt, const int index, char *dst, size_t dst_sz);
};

#endif
