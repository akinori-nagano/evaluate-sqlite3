#ifdef FOR_PLATFORM_WINDOWS
#include <windows.h>
#include <sysinfoapi.h>
#endif

#include <iostream>
#include "TestUtility.h"
#include "hashes.h"

#if 1
#define __INFO(fmt, ...)  this->debugOut("INFO",  __FUNCTION__, fmt, ##__VA_ARGS__)
#define __ERROR(fmt, ...) this->debugOut("ERROR", __FUNCTION__, fmt, ##__VA_ARGS__)
#define __FATAL(fmt, ...) this->debugOut("FATAL", __FUNCTION__, fmt, ##__VA_ARGS__)
#else
#define __INFO(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define __ERROR(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define __FATAL(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

#define R_CHECK() { \
	if (res) { \
		__FATAL("%s: Index-%d is failed. (res=%d)", __FUNCTION__, idx, res); \
		status = TestMainStatus::Error; \
		goto END; \
	} \
	idx++; \
}

Table001 TestData001[TEST_DATA1_ID_MAX_COUNT][TEST_DATA_KIND_MAX_COUNT];
Table002 TestData002[TEST_DATA2_ID_MAX_COUNT];

////////////////////////////////////////////////////////////////////////////////
// extern

static void __create_test_data(void);
static TestMainStatus __create_test_data_one_001(int id_index, int kind_index, int count);
static TestMainStatus __create_test_data_one_002(int id_index, int count);

void
TestInitialize(void)
{
	__create_test_data();
}

Table001 *
GetTest001Data(const int id, Table001Kind kind)
{
	if ((id - 1) >= TEST_DATA1_ID_MAX_COUNT) {
		return NULL;
	}
	return &(TestData001[id - 1][kind - 1]);
}

Table002 *
GetTest002Data(const int id)
{
	for (int i = 0; i < TEST_DATA2_ID_MAX_COUNT; i++) {
		if (TestData002[i].id == id) {
			return &(TestData002[i]);
		}
	}
	return NULL;
}

void
__create_test_data(void)
{
	int hashesCount = 0;

	for (int i = 0; i < TEST_DATA1_ID_MAX_COUNT; i++) {
		for (int j = 0; j < TEST_DATA_KIND_MAX_COUNT; j++) {
			if (__create_test_data_one_001(i, j, hashesCount) != TestMainStatus::Ok) {
				printf("FatalError: index size over. (%d, %d)\n", i, j);
				break;
			}
			hashesCount++;
		}
	}

	for (int i = 0; i < TEST_DATA2_ID_MAX_COUNT; i++) {
		if (__create_test_data_one_002(i, hashesCount) != TestMainStatus::Ok) {
			printf("FatalError: index size over. (%d)\n", i);
			break;
		}
		hashesCount++;
	}

	printf("==== TestData001\n");
	for (int i = 0; i < TEST_DATA1_ID_MAX_COUNT; i++) {
		for (int j = 0; j < TEST_DATA_KIND_MAX_COUNT; j++) {
			printf("%d:%d:%s:\n", TestData001[i][j].id, TestData001[i][j].kind, TestData001[i][j].hashed_id);
		}
	}
	printf("==== TestData002\n");
	for (int i = 0; i < TEST_DATA2_ID_MAX_COUNT; i++) {
		printf("%d:%s:\n", TestData002[i].id, TestData002[i].hashed_id);
	}
}

TestMainStatus
__create_test_data_one_001(int id_index, int kind_index, int count)
{
	if (count >= HashValuesMaxCount) {
		return TestMainStatus::Error;
	}

	const char *h = HashValues[count];

	Table001 *p = &(TestData001[id_index][kind_index]);
	memset(p, 0x00, sizeof(Table001));
	p->id = id_index + 1;
	p->kind = kind_index + 1;
	snprintf(p->contents_id, sizeof(p->contents_id), "CONTENTS_ID_%s", h);
	snprintf(p->contents_code, sizeof(p->contents_code), "CONTENTS_CODE_%s", h);
	snprintf(p->hashed_id, sizeof(p->hashed_id), "%s", h);
	for (int i = 0; i < p->id; i++) {
		strncat((char *)p->terminal_value, h, sizeof(p->terminal_value) - 1);
	}
	p->terminal_value_sz = strlen((char *)p->terminal_value);

	return TestMainStatus::Ok;
}

TestMainStatus
__create_test_data_one_002(int id_index, int count)
{
	if (count >= HashValuesMaxCount) {
		return TestMainStatus::Error;
	}

	const char *h = HashValues[count];

	Table002 *p = &(TestData002[id_index]);
	memset(p, 0x00, sizeof(Table002));
	p->id = id_index + 1;

	snprintf(p->contents_code, sizeof(p->contents_code), "CONTENTS_CODE_%s", h);
	snprintf(p->hashed_id, sizeof(p->hashed_id), "%s", h);
	p->secret = 0;
	snprintf(p->read_datetime, sizeof(p->read_datetime), "%s", h);
	p->delete_status = 0;

	return TestMainStatus::Ok;
}

////////////////////////////////////////////////////////////////////////////////
// public

TestUtility::TestUtility(FILE *__fpDebug, long __threadID) :
	// public
		fpDebug(__fpDebug),
		threadID(__threadID),
		db(NULL)
{
	;
}

TestUtility::~TestUtility()
{
	;
}

long
TestUtility::SystemErapsedCount(void)
{
#ifdef FOR_PLATFORM_WINDOWS
	DWORD r = GetTickCount();
	return (long)r;
#else
	struct timespec ts;
	long theTick = 0U;
	clock_gettime( CLOCK_REALTIME, &ts );
	theTick  = ts.tv_nsec / 1000000;
	theTick += ts.tv_sec * 1000;
	return theTick;
#endif
}

void
TestUtility::debugOut(const char *tag, const char *fn, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

	long sec = SystemErapsedCount();
	fprintf(this->fpDebug, "%ld:%s:T%ld:%s: %s\n", sec, tag, this->threadID, fn, buf);
	fflush(this->fpDebug);
}

int
TestUtility::IsThreadsafe(void)
{
	return sqlite3_threadsafe();
}

TestMainStatus
TestUtility::openDB(const char *tag, const char *dbPath)
{
	int res = sqlite3_open_v2(dbPath, &(this->db),
			// SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_SHAREDCACHE,
			SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX,
			NULL);
	if (SQLITE_OK != res) {
		__FATAL("%s: sqlite3_open_v2 failed. (res=%d) [%s]", tag, res, dbPath);
		return TestMainStatus::Error;
	}
	__INFO("%s: open db. (%p)", tag, this->db);
	res = sqlite3_busy_timeout(this->db, 60 * 1000);
	if (SQLITE_OK != res) {
		__FATAL("%s: sqlite3_busy_timeout failed. (res=%d)", tag, res);
		return TestMainStatus::Error;
	}
	return TestMainStatus::Ok;
}

void
TestUtility::closeDB(const char *tag)
{
	__INFO("%s: close db. (%p)", tag, this->db);
	sqlite3_close_v2(this->db);
	this->db = NULL;
}

TestMainStatus
TestUtility::startTransaction(const char *tag)
{
	__INFO("%s: start transaction. (%p)", tag, this->db);

	TestMainStatus status = TestMainStatus::Ok;
	const char *sql = "BEGIN;";

#if 1
	sqlite3_stmt *stmt = __sqlite3_prepare(sql);

	if (__sqlite3_step(stmt, tag, 0, false) != SQLITE_OK) {
		__FATAL("%s: __sqlite3_step failed.", tag);
		status = TestMainStatus::Error;
		goto END;
	}

END:
	sqlite3_finalize(stmt);
 	return status;
#else
	int ret = sqlite3_exec(this->db, sql, NULL, NULL, NULL);
	__INFO("@@@ begin exec ret %d", ret);
	status = (ret == SQLITE_OK) ? TestMainStatus::Ok : TestMainStatus::Error;
 	return status;
#endif
}

TestMainStatus
TestUtility::endTransaction(const char *tag, TestMainStatus commitStatus)
{
	__INFO("%s: end transaction. [%s] (%p)", tag, (commitStatus == TestMainStatus::Ok) ? "COMMIT" : "ROLLBACK", this->db);

	TestMainStatus status = TestMainStatus::Ok;
	const char *sql = (commitStatus == TestMainStatus::Ok) ? "COMMIT;" : "ROLLBACK;";

#if 1
	sqlite3_stmt *stmt = __sqlite3_prepare(sql);

	if (__sqlite3_step(stmt, tag, 0, true) != SQLITE_OK) {
		__FATAL("%s: __sqlite3_step failed.", tag);
		status = TestMainStatus::Error;
		goto END;
	}

END:
	sqlite3_finalize(stmt);
	return status;
#else
	int ret = sqlite3_exec(this->db, sql, NULL, NULL, NULL);
	__INFO("@@@ commit exec ret %d. (%s)", ret, sql);
	status = (ret == SQLITE_OK) ? TestMainStatus::Ok : TestMainStatus::Error;
	return status;
#endif
}

void
TestUtility::deleteAll(void)
{
	__INFO("delete all rows in all tables. (%p)", this->db);

	const char *SQLS[] = {
		"DELETE FROM t_test_001;",
		"DELETE FROM t_test_002;",
		"DELETE FROM tmp_t_test_002;",
	};
	const int SQLS_MAX = sizeof(SQLS) / sizeof(char *);

	for (int i=0; i<SQLS_MAX; i++) {
		const char *sql = SQLS[i];
		int ret = sqlite3_exec(this->db, sql, NULL, NULL, NULL);
		if (ret != SQLITE_OK) {
			__FATAL("@@@ begin exec ret %d. [%s]", ret, sql);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// public Table001

TestMainStatus
TestUtility::insertTable001(Table001 *table)
{
	TestMainStatus status = TestMainStatus::Ok;

	__INFO("%s: start (id=%d)", __FUNCTION__, table->id);
	const char *sql = "INSERT INTO t_test_001" \
					   " (id, kind, contents_id, contents_code, hashed_id, terminal_value, updated_date, updated_at)" \
					   " VALUES(?, ?, ?, ?, ?, ?, DATE(CURRENT_TIMESTAMP), DATETIME('now', 'localtime'));";

	sqlite3_stmt *stmt = __sqlite3_prepare(sql);

	int res = 0;
	int idx = 1;
	res = sqlite3_bind_int(stmt, idx, table->id);
	R_CHECK();
	res = sqlite3_bind_int(stmt, idx, table->kind);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->contents_id, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->contents_code, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->hashed_id, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_blob(stmt, idx, table->terminal_value, table->terminal_value_sz, SQLITE_STATIC);
	R_CHECK();

	res = __sqlite3_step(stmt, NULL, table->id, false);
	if (res != SQLITE_OK) {
		if (res == SQLITE_BUSY) {
			__ERROR("__sqlite3_step busy. (%d)", table->id);
			status = TestMainStatus::Redoing;
		} else {
			__FATAL("__sqlite3_step failed. (%d)", table->id);
			status = TestMainStatus::Error;
		}
		goto END;
	}

END:
	sqlite3_finalize(stmt);
	__INFO("%s: end (id=%d)(status=%d)", __FUNCTION__, table->id, status);
	return status;
}

TestMainStatus
TestUtility::updateTable001(Table001 *table)
{
	TestMainStatus status = TestMainStatus::Ok;

    __INFO("%s: start (id=%d)", __FUNCTION__, table->id);
	const char *sql = "UPDATE t_test_001 SET" \
					   " kind = ?, contents_id = ?, contents_code = ?, hashed_id = ?, terminal_value = ?," \
					   " updated_date = DATE(CURRENT_TIMESTAMP), updated_at = DATETIME('now', 'localtime')" \
					   " WHERE id = ?;";

	sqlite3_stmt *stmt = __sqlite3_prepare(sql);

	int res = 0;
	int idx = 1;
	res = sqlite3_bind_int(stmt, idx, table->kind);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->contents_id, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->contents_code, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->hashed_id, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_blob(stmt, idx, table->terminal_value, table->terminal_value_sz, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_int(stmt, idx, table->id);
	R_CHECK();

	res = __sqlite3_step(stmt, NULL, table->id, false);
	if (res != SQLITE_OK) {
		if (res == SQLITE_BUSY) {
			__ERROR("__sqlite3_step busy. (%d)", table->id);
			status = TestMainStatus::Redoing;
		} else {
			__FATAL("__sqlite3_step failed. (%d)", table->id);
			status = TestMainStatus::Error;
		}
		goto END;
	}

END:
	sqlite3_finalize(stmt);
    __INFO("%s: end (id=%d)(status=%d)", __FUNCTION__, table->id, status);
	return status;
}

TestMainStatus
TestUtility::selectTable001(const int id, Table001 *output)
{
	TestMainStatus status = TestMainStatus::Ok;

    __INFO("%s: start (id=%d)", __FUNCTION__, id);
	char sql[1024];
	snprintf(sql, sizeof(sql), "SELECT" \
		" id, kind, contents_id, contents_code, hashed_id, terminal_value, updated_date, updated_at" \
		" FROM t_test_001 WHERE id = %d;", id);

	sqlite3_stmt *stmt = __sqlite3_prepare(sql);
	int res = sqlite3_step(stmt);
	if (res == SQLITE_DONE) {
		__INFO("sqlite3_step is not found. (res=%d) (id=%d)", res, id);
		status = TestMainStatus::NotFound;
		goto END;
	}
	if (res != SQLITE_ROW) {
		__ERROR("sqlite3_step is failed. (res=%d) (id=%d)", res, id);
		status = TestMainStatus::Error;
		goto END;
	}

	{
		int idx = 0;
		memset(output, 0x00, sizeof(Table001));
		output->id = sqlite3_column_int(stmt, idx++);
		output->kind = sqlite3_column_int(stmt, idx++);

		__sqlite3_column_text(stmt, idx, output->contents_id, sizeof(output->contents_id));
		idx++;
		__sqlite3_column_text(stmt, idx, output->contents_code, sizeof(output->contents_code));
		idx++;
		__sqlite3_column_text(stmt, idx, output->hashed_id, sizeof(output->hashed_id));
		idx++;

		output->terminal_value_sz = sqlite3_column_bytes(stmt, idx);
		if (output->terminal_value_sz != 0) {
			unsigned char *b = (unsigned char *)sqlite3_column_blob(stmt, idx);
			if (b) {
				memcpy(output->terminal_value, b, output->terminal_value_sz);
			}
		}
		idx++;

		__sqlite3_column_text(stmt, idx, output->updated_date, sizeof(output->updated_date));
		idx++;
		__sqlite3_column_text(stmt, idx, output->updated_at, sizeof(output->updated_at));
		idx++;
	}

END:
	sqlite3_finalize(stmt);
    __INFO("%s: end (id=%d)(status=%d)", __FUNCTION__, id, status);
	return status;
}

TestMainStatus
TestUtility::deleteTable001(const int id)
{
	TestMainStatus status = TestMainStatus::Ok;
	int res = SQLITE_DONE;

    __INFO("%s: start (id=%d)", __FUNCTION__, id);
	char sql[1024];
	snprintf(sql, sizeof(sql), "DELETE FROM t_test_001 WHERE id = %d;", id);
	sqlite3_stmt *stmt = __sqlite3_prepare(sql);

	if (__sqlite3_step(stmt, NULL, id, false) != SQLITE_OK) {
		__ERROR("__sqlite3_step failed. (id=%d) (res=%d)", id, res);
		status = res == SQLITE_BUSY ? TestMainStatus::Redoing : TestMainStatus::Error;
		goto END;
	}

END:
	sqlite3_finalize(stmt);
    __INFO("%s: end (id=%d)(status=%d)", __FUNCTION__, id, status);
	return status;
}

////////////////////////////////////////////////////////////////////////////////
// public Table002

TestMainStatus
TestUtility::insertTable002(Table002 *table, bool isTemp)
{
	TestMainStatus status = TestMainStatus::Ok;

    __INFO("%s: start (id=%d)", __FUNCTION__, table->id);
	char sql[1024];
	snprintf(sql, sizeof(sql),
			"INSERT INTO %s" \
			" (contents_code, hashed_id, id, secret, read_datetime, delete_status, updated_at)" \
			" VALUES (?, ?, ?, ?, ?, ?, DATETIME('now', 'localtime'));",
			isTemp ? "tmp_t_test_002" : "t_test_002");

	sqlite3_stmt *stmt = __sqlite3_prepare(sql);

	int res = 0;
	int idx = 1;
	res = sqlite3_bind_text(stmt, idx, table->contents_code, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->hashed_id, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_int(stmt, idx, table->id);
	R_CHECK();
	res = sqlite3_bind_int(stmt, idx, table->secret);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->read_datetime, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_int(stmt, idx, table->delete_status);
	R_CHECK();

	res = __sqlite3_step(stmt, table->contents_code, 0, false);
	if (res != SQLITE_OK) {
		if (res == SQLITE_BUSY) {
			__ERROR("__sqlite3_step busy. (%d)", table->id);
			status = TestMainStatus::Redoing;
		} else {
			__FATAL("__sqlite3_step failed. (%d)", table->id);
			status = TestMainStatus::Error;
		}
		goto END;
	}

END:
	sqlite3_finalize(stmt);
    __INFO("%s: end (id=%d)(status=%d)", __FUNCTION__, table->id, status);
	return status;
}

TestMainStatus
TestUtility::updateTable002(Table002 *table, bool isTemp)
{
	TestMainStatus status = TestMainStatus::Ok;

    __INFO("%s: start (id=%d)", __FUNCTION__, table->id);
	char sql[1024];
	snprintf(sql, sizeof(sql),
			"UPDATE %s SET" \
			" hashed_id = ?, id = ?, secret = ?, read_datetime = ?, delete_status = ?," \
			" updated_at = DATETIME('now', 'localtime')" \
			" WHERE contents_code = ?;",
			isTemp ? "tmp_t_test_002" : "t_test_002");

	sqlite3_stmt *stmt = __sqlite3_prepare(sql);

	int res = 0;
	int idx = 1;
	res = sqlite3_bind_text(stmt, idx, table->hashed_id, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_int(stmt, idx, table->id);
	R_CHECK();
	res = sqlite3_bind_int(stmt, idx, table->secret);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->read_datetime, -1, SQLITE_STATIC);
	R_CHECK();
	res = sqlite3_bind_int(stmt, idx, table->delete_status);
	R_CHECK();
	res = sqlite3_bind_text(stmt, idx, table->contents_code, -1, SQLITE_STATIC);
	R_CHECK();

	res = __sqlite3_step(stmt, table->contents_code, 0, false);
	if (res != SQLITE_OK) {
		if (res == SQLITE_BUSY) {
			__ERROR("__sqlite3_step busy. (id=%d)(cc=%s)", table->id, table->contents_code);
			status = TestMainStatus::Redoing;
		} else {
			__FATAL("__sqlite3_step failed. (id=%d)(cc=%s)", table->id, table->contents_code);
			status = TestMainStatus::Error;
		}
		goto END;
	}

END:
	sqlite3_finalize(stmt);
    __INFO("%s: end (id=%d)(status=%d)", __FUNCTION__, table->id, status);
	return status;
}

TestMainStatus
TestUtility::selectTable002(const char *contents_code, bool isTemp, Table002 *output)
{
    __INFO("%s: start (cc=%s)", __FUNCTION__, contents_code);
	char sql[1024];
	snprintf(sql, sizeof(sql), "SELECT" \
			" contents_code, hashed_id, id, secret, read_datetime, delete_status, updated_at" \
			" FROM %s WHERE contents_code = '%s';",
			isTemp ? "tmp_t_test_002" : "t_test_002", contents_code);

	TestMainStatus status = this->__selectTable002UniqueRow(sql, output);
	if (status == TestMainStatus::Ok) {
		__INFO("%s: end (id=%d)(cc=%s)(status=%d)", __FUNCTION__, output->id, contents_code, status);
	} else {
		__INFO("%s: end (cc=%s)(status=%d)", __FUNCTION__, contents_code, status);
	}
	return status;
}

TestMainStatus
TestUtility::selectTable002Byid(const int id, bool isTemp, Table002 *output)
{
    __INFO("%s: start (id=%d)", __FUNCTION__, id);
	char sql[1024];
	snprintf(sql, sizeof(sql), "SELECT" \
			" contents_code, hashed_id, id, secret, read_datetime, delete_status, updated_at" \
			" FROM %s WHERE id = '%d';",
			isTemp ? "tmp_t_test_002" : "t_test_002", id);

	TestMainStatus status = this->__selectTable002UniqueRow(sql, output);
    __INFO("%s: end (id=%d)(status=%d)", __FUNCTION__, id, status);
	return status;
}

TestMainStatus
TestUtility::selectAllView(Table002 **output, int *n_output)
{
	TestMainStatus status = TestMainStatus::Ok;
	*output = NULL;
	*n_output = 0;

    __INFO("%s: start", __FUNCTION__);
	const char *sql = "SELECT" \
					   " contents_code, hashed_id, id, secret, read_datetime, delete_status, updated_at" \
					   " FROM v_test_002 ORDER BY id ASC;";

	sqlite3_stmt *stmt = __sqlite3_prepare(sql);

	Table002 *p = NULL;
	int n_p = 0;
	while (true) {
		int res = sqlite3_step(stmt);
		if (res == SQLITE_DONE) {
			goto END;
		}
		if (res != SQLITE_ROW) {
			__ERROR("sqlite3_step is failed. (res=%d) (n_p=%d)", res, n_p);
			status = TestMainStatus::Error;
			goto END;
		}

		Table002 data;
		memset(&data, 0x00, sizeof(Table002));
		__copySqlite3ColumnTable002(stmt, &data);
		if (p) {
			Table002 *p2 = (Table002 *)realloc(p, sizeof(Table002) * (n_p + 1));
			p = p2;
		} else {
			p = (Table002 *)malloc(sizeof(Table002) * (n_p + 1));
		}
		__INFO("@@@@@@@@ CC=[%d][%s]", n_p, data.contents_code);
		memcpy(&(p[n_p]), &data, sizeof(Table002));
		n_p++;
	}

END:
	sqlite3_finalize(stmt);

	if (n_p == 0) {
		__INFO("sqlite3_step is not found.");
		if (p) {
			free(p);
		}
		return TestMainStatus::NotFound;
	}
	*output = p;
	*n_output = n_p;
    __INFO("%s: end (status=%d)", __FUNCTION__, status);
	return status;
}

TestMainStatus
TestUtility::selectViewById(const int id, Table002 *output)
{
    __INFO("%s: start (id=%d)", __FUNCTION__, id);
	char sql[1024];
	snprintf(sql, sizeof(sql), "SELECT" \
			" contents_code, hashed_id, id, secret, read_datetime, delete_status, updated_at" \
			" FROM v_test_002 WHERE id = %d ORDER BY id ASC;", id);

	TestMainStatus status = this->__selectTable002UniqueRow(sql, output);
    __INFO("%s: end (id=%d)(status=%d)", __FUNCTION__, id, status);
	return status;
}

TestMainStatus
TestUtility::deleteTable002(const int id, bool isTemp)
{
	TestMainStatus status = TestMainStatus::Ok;
	int res = SQLITE_DONE;

    __INFO("%s: start (id=%d)(%s)", __FUNCTION__, id, isTemp?"temp":"none");
	char sql[1024];
	snprintf(sql, sizeof(sql), "DELETE FROM %s WHERE id = %d;",
			isTemp ? "tmp_t_test_002" : "t_test_002", id);
	sqlite3_stmt *stmt = __sqlite3_prepare(sql);

	if ((res = __sqlite3_step(stmt, NULL, id, false)) != SQLITE_OK) {
		__ERROR("__sqlite3_step failed. (id=%d) (res=%d)", id, res);
		status = res == SQLITE_BUSY ? TestMainStatus::Redoing : TestMainStatus::Error;
		goto END;
	}

END:
	sqlite3_finalize(stmt);
    __INFO("%s: end (id=%d)(%s)(status=%d)", __FUNCTION__, id, isTemp?"temp":"none", status);
	return status;
}

////////////////////////////////////////////////////////////////////////////////
// private

TestMainStatus
TestUtility::__selectTable002UniqueRow(const char *sql, Table002 *output)
{
	TestMainStatus status = TestMainStatus::Ok;
	memset(output, 0x00, sizeof(Table002));

	sqlite3_stmt *stmt = __sqlite3_prepare(sql);
	int res = sqlite3_step(stmt);
	if (res == SQLITE_DONE) {
		__INFO("sqlite3_step is not found. (res=%d)", res);
		status = TestMainStatus::NotFound;
		goto END;
	}
	if (res != SQLITE_ROW) {
		__ERROR("sqlite3_step is failed. (res=%d)", res);
		status = TestMainStatus::Error;
		goto END;
	}

	__copySqlite3ColumnTable002(stmt, output);

END:
	sqlite3_finalize(stmt);
	return status;
}

void
TestUtility::__copySqlite3ColumnTable002(sqlite3_stmt *stmt, Table002 *output)
{
	int idx = 0;

	__sqlite3_column_text(stmt, idx, output->contents_code, sizeof(output->contents_code));
	idx++;
	__sqlite3_column_text(stmt, idx, output->hashed_id, sizeof(output->hashed_id));
	idx++;

	output->id = sqlite3_column_int(stmt, idx++);
	output->secret = sqlite3_column_int(stmt, idx++);

	__sqlite3_column_text(stmt, idx, output->read_datetime, sizeof(output->read_datetime));
	idx++;

	output->delete_status = sqlite3_column_int(stmt, idx++);

	__sqlite3_column_text(stmt, idx, output->updated_at, sizeof(output->updated_at));
	idx++;
}

sqlite3_stmt *
TestUtility::__sqlite3_prepare(const char *sql)
{
	sqlite3_stmt *stmt = NULL;
	int res = sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
	if (res != SQLITE_OK) {
		__ERROR("sqlite3_prepare_v2: (res=%d) [%s]", res, sql);
		return NULL;
	}
	res = sqlite3_reset(stmt);
	if (res != SQLITE_OK) {
		__INFO("sqlite3_reset: (res=%d)", res);
		return NULL;
	}
	return stmt;
}

int
TestUtility::__sqlite3_step(sqlite3_stmt *stmt, const char *sTag, const int iTag, bool isRetry)
{
	int res = SQLITE_DONE;

	if (isRetry) {
		while (true) {
			res = sqlite3_step(stmt);
			if (res != SQLITE_BUSY) {
				break;
			}
			__INFO("Sleeping. (res=%d)", res);
			mySleep(1);
		}
	} else {
		res = sqlite3_step(stmt);
	}
	if (res == SQLITE_OK || res == SQLITE_DONE) {
		return SQLITE_OK;
	}
	if (res != SQLITE_ROW) {
		if (sTag) {
			__ERROR("sqlite3_step is failed. (res=%d) (retry=%d) (cc=%s)", res, isRetry, sTag);
		} else {
			__ERROR("sqlite3_step is failed. (res=%d) (retry=%d) (id=%d)", res, isRetry, iTag);
		}
		__ERROR("%s", sqlite3_errmsg(this->db));
	}
	return res;
}

TestMainStatus
TestUtility::__sqlite3_column_text(sqlite3_stmt *stmt, const int index, char *dst, size_t dst_sz)
{
	const unsigned char *p = sqlite3_column_text(stmt, index);
	if (p) {
		// char buf[1024];
		// size_t sz = sizeof(buf);
		// snprintf(buf, sz, "CONTENTS_ID_%s", p);
        //
		// snprintf(buf , dst_sz, "%s", p);
		strncpy(dst, (char *)p, dst_sz - 1);
		return TestMainStatus::Ok;
	}
	memset(dst, 0x00, dst_sz);
	return TestMainStatus::Error;
}
