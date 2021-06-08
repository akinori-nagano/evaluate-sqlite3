#include <iostream>

#include "constants.h"
#include "TestUtility.h"
#include "Test001.h"

static TestMainStatus __case_insert_or_update(TestUtility *testUtil, const int id, Table001Kind updateKind);
static TestMainStatus __case_delete_and_insert(TestUtility *testUtil, const int id);
static TestMainStatus __case_select(TestUtility *testUtil, const int id);
#if 0
static void __dumpTable001(Table001 *p);
#endif


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
	StartTransaction(SQLITE3_DEFERRED);

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
		if (testUtil->diffTable001(&t, p)) {
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

	StartTransaction(SQLITE3_DEFERRED);

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
		if (testUtil->diffTable001(&t, p)) {
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
		if (testUtil->diffTable001(&t, p)) {
			st = TestMainStatus::Error;
			goto END;
		}
	}

	__ASSERT("Found recored. (id=%d)", id);

END:
	__INFO("end.");
	return st;
}

#if 0
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
#endif
