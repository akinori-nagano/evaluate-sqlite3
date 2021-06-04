#include <iostream>

#include "constants.h"
#include "Test001.h"

static TestMainStatus __case_insert_or_update(TestUtility *testUtil, const int id, Table001Kind updateKind);
static TestMainStatus __case_delete_and_insert(TestUtility *testUtil, const int id);
static TestMainStatus __case_select(TestUtility *testUtil, const int id);
static void __dumpTable001(Table001 *p);
static bool __diffTable001(TestUtility *testUtil, Table001 *s, Table001 *d);


TestMainStatus
runTest001_01(TestUtility *testUtil, int testCount)
{
	{
		time_t t = (unsigned int)time(NULL);
		unsigned int n = (unsigned int)((t % 50000) + testUtil->threadID);
		__INFO("srand(%u)", n);
		srand(n);
	}

	for (int i = 0; i < testCount; i++) {
		long r = random();
		const int forkNumber = (int)(r % 3);

		r = random();
		const int id = (int)(r % TEST_DATA1_ID_MAX_COUNT) + 1;

		r = random();
		const int updateKind = (int)(r % TEST_DATA_UPDATE_KIND_MAX_COUNT) + 1;

		__INFO("@@@ %d: fn: %d: id: %d, kind: %d", i, forkNumber, id, updateKind);
		while (true) {
			TestMainStatus st = TestMainStatus::Error;
			if (forkNumber == 0) {
				st = __case_insert_or_update(testUtil, id, (Table001Kind)updateKind);
			} else if (forkNumber == 1) {
				st = __case_delete_and_insert(testUtil, id);
			} else if (forkNumber == 2) {
				st = __case_select(testUtil, id);
			}
			if (st == TestMainStatus::Redoing) {
				__INFO("Redo the process. SQLITE_BUSY has been acquired.");
				continue;
			}

			if (st == TestMainStatus::Ok) {
				break;
			} else if (st == TestMainStatus::Redoing) {
				__INFO("Redo the process. SQLITE_BUSY has been acquired.");
				continue;
			} else {
				return st;
			}
			myMicroSleep(2);
		}
	}
	return TestMainStatus::Ok;
}

TestMainStatus
__case_insert_or_update(TestUtility *testUtil, const int id, Table001Kind updateKind)
{
	Table001 *p = NULL;
	TestMainStatus st = TestMainStatus::Error;
	bool isInsert = true;

	__INFO("start.");
	StartTransaction();

	// select してupdateかinsertか決める
	{
		Table001 t;
		TestMainStatus st = testUtil->selectTable001(id, &t);
		isInsert = (st != TestMainStatus::Ok);
	}
	// テストデータ作成
	if (isInsert) {
		p = GetTest001Data(id, Table001Kind::Insert);
		if (p == NULL) {
			__FATAL("cannot get testdata. %d, Insert.", id);
		}
	} else {
		p = GetTest001Data(id, updateKind);
		if (p == NULL) {
			__FATAL("cannot get testdata. %d, %d.", id, updateKind);
		}
	}
	if (p == NULL) {
		goto END;
	}
	// update or insert
	if (isInsert) {
		st = testUtil->insertTable001(p);
	} else {
		st = testUtil->updateTable001(p);
	}
	if (st != TestMainStatus::Ok) {
		__FATAL("insert or update failed.");
		goto END;
	}
	// 検証
	{
		Table001 t;
		st = testUtil->selectTable001(id, &t);
		if (st != TestMainStatus::Ok) {
			__FATAL("select failed.");
			goto END;
		}
		if (__diffTable001(testUtil, &t, p)) {
			st = TestMainStatus::Error;
			goto END;
		}
	}

	if (isInsert) {
		__ASSERT("Insert success. (id=%d)", id);
	} else {
		__ASSERT("Update success. (id=%d) (updateKind=%d)", id, updateKind);
	}

END:
	EndTransaction();
	__INFO("end.");
	return st;
}

TestMainStatus
__case_delete_and_insert(TestUtility *testUtil, const int id)
{
	__INFO("start.");

	// テストデータ作成
	Table001 *p = NULL;
	{
		p = GetTest001Data(id, Table001Kind::Insert);
		if (p == NULL) {
			__FATAL("cannot get testdata. %d.", id);
			return TestMainStatus::Error;
		}
	}

	StartTransaction();

	bool isDeleted = false;
	TestMainStatus st = testUtil->deleteTable001(id);
	if (st != TestMainStatus::Ok) {
		isDeleted = true;
	}

	st = testUtil->insertTable001(p);
	if (st != TestMainStatus::Ok) {
		__FATAL("insert failed. %d.", id);
		goto END;
	}

	// 検証
	{
		Table001 t;
		st = testUtil->selectTable001(id, &t);
		if (st != TestMainStatus::Ok) {
			__FATAL("select failed. %d.", id);
			goto END;
		}
		if (__diffTable001(testUtil, &t, p)) {
			st = TestMainStatus::Error;
			goto END;
		}
	}

	if (isDeleted) {
		__ASSERT("Not found record, Insert success. (id=%d)", id);
	} else {
		__ASSERT("Delete and Insert success. (id=%d)", id);
	}

END:
	EndTransaction();
	__INFO("end.");
	return st;
}

TestMainStatus
__case_select(TestUtility *testUtil, const int id)
{
	__INFO("start.");

	Table001 t;
	TestMainStatus st = testUtil->selectTable001(id, &t);
	if (st == TestMainStatus::NotFound) {
		__ASSERT("Not found recored. (id=%d)", id);
		st = TestMainStatus::Ok;
		goto END;
	}
	if (st != TestMainStatus::Ok) {
		__FATAL("select failed. %d.", id);
		goto END;
	}

	{
		Table001 *p = NULL;
		p = GetTest001Data(id, (Table001Kind)t.kind);
		if (p == NULL) {
			__FATAL("cannot get testdata. %d, %d.", id, t.kind);
			goto END;
		}
		if (__diffTable001(testUtil, &t, p)) {
			st = TestMainStatus::Error;
			goto END;
		}
	}

	__ASSERT("Found recored. (id=%d)", id);

END:
	__INFO("end.");
	return st;
}

void
__dumpTable001(Table001 *p)
{
	std::cout << "++++++++++ " << __FUNCTION__ << ": id: " << p->id << ", kind: " << p->kind << std::endl;
	std::cout << "cid: " << p->contents_id << std::endl;
	std::cout << "ccd: " << p->contents_code << std::endl;
	std::cout << "hsh: " << p->hashed_id << std::endl;
	std::cout << p->terminal_value_sz << ": [" << p->terminal_value << "]" << std::endl;
	std::cout << p->updated_date << std::endl;
	std::cout << p->updated_at << std::endl;
	std::cout << "----------" << std::endl;
}

bool
__diffTable001(TestUtility *testUtil, Table001 *s, Table001 *d)
{
	bool hasDiff = false;

	if (s->id != d->id) {
		__FATAL("DIFF: id: %d, %d.", s->id, d->id);
		hasDiff = true;
	}
	if (s->kind != d->kind) {
		__FATAL("DIFF: kind: %d, %d.", s->kind, d->kind);
		hasDiff = true;
	}
	if (strcmp(s->contents_id, d->contents_id)) {
		__FATAL("DIFF: contents_id: [%s], [%s].", s->contents_id, d->contents_id);
		hasDiff = true;
	}
	if (strcmp(s->contents_code, d->contents_code)) {
		__FATAL("DIFF: contents_code: [%s], [%s].", s->contents_code, d->contents_code);
		hasDiff = true;
	}
	if (strcmp(s->hashed_id, d->hashed_id)) {
		__FATAL("DIFF: hashed_id: [%s], [%s].", s->hashed_id, d->hashed_id);
		hasDiff = true;
	}
	if (s->terminal_value_sz != d->terminal_value_sz) {
		__FATAL("DIFF: terminal_value_sz: %d, %d.", s->terminal_value_sz, d->terminal_value_sz);
		hasDiff = true;
	} else {
		if (s->terminal_value_sz == 0) {
			__FATAL("DIFF: terminal_value: is null.");
			hasDiff = true;
		} else {
			if (memcmp(s->terminal_value, d->terminal_value, s->terminal_value_sz)) {
				__FATAL("DIFF: terminal_value: [%s], [%s].", s->terminal_value, d->terminal_value);
				hasDiff = true;
			}
		}
	}
	return hasDiff;
}
