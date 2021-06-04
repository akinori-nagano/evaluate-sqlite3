#ifdef FOR_PLATFORM_WINDOWS
#include <windows.h>
#include <sysinfoapi.h>
#endif

#include <iostream>

#include "constants.h"
#include "Test002.h"

static TestMainStatus __insert_or_update(TestUtility *testUtil, Table002 *table002, bool isTemp);
static TestMainStatus __delete_and_insert(TestUtility *testUtil, Table002 *table002, bool isTemp);

static void __dumpTable002(TestUtility *testUtil, Table002 *s, Table002 *d);
static bool __diffTable002(TestUtility *testUtil, Table002 *s, Table002 *d);


TestMainStatus
runTest002_01(TestUtility *testUtil, int testCount)
{
	// 検証用
	Table002 viewVerificationData[] = {
		{ "CONTENTS_CODE_AA6ED9E0F26A6EBA784AAE8267DF1951", "AA6ED9E0F26A6EBA784AAE8267DF1951", 1, 0, "2017-12-08 03:34:56", 0, "" },
		{ "CONTENTS_CODE_367764329430DB34BE92FD14A7A770EE", "367764329430DB34BE92FD14A7A770EE", 2, 1, "2017-12-06 03:34:56", 0, "" },
		{ "CONTENTS_CODE_8C9EB686BF3EB5BD83D9373EADF6504B", "8C9EB686BF3EB5BD83D9373EADF6504B", 3, 0, "2017-12-07 03:34:56", 1, "" },
	};
	Table002 tableVerificationData[] = {
		{ "CONTENTS_CODE_AA6ED9E0F26A6EBA784AAE8267DF1951", "AA6ED9E0F26A6EBA784AAE8267DF1951", 1, 1, "2017-12-05 03:34:56", 1, "" },
		viewVerificationData[1],
	};
	Table002 tmptableVerificationData[] = {
		viewVerificationData[0],
		viewVerificationData[2],
	};

	/*
	 * data1、data2 は t_test_002 へ
	 * data3、data4 は tmp_t_test_002 へ
	 * data1とdata3のcontents_codeは同一
	 */
	Table002 *data1 = &(tableVerificationData[0]);
	Table002 *data2 = &(tableVerificationData[1]);
	Table002 *data3 = &(tmptableVerificationData[0]);
	Table002 *data4 = &(tmptableVerificationData[1]);

#define LocalEndTransaction { \
	if (doingTransaction) { \
		EndTransaction(); \
		doingTransaction = false; \
	} \
}
	bool doingTransaction = false;
	TestMainStatus st = TestMainStatus::Ok;
	for (int i = 0; i < testCount; i++) {
		while (true) {
			st = TestMainStatus::Ok;
			LocalEndTransaction;
			StartTransaction();
			doingTransaction = true;
			__INFO("START:%d:", i + 1);

			if (st == TestMainStatus::Ok) {
				if ((st = __insert_or_update(testUtil, data1, false))) {
					__ERROR("__insert_or_update failed. (st=%d)(id=%d)", st, data1->id);
				}
			}
			if (st == TestMainStatus::Ok) {
				if ((st = __insert_or_update(testUtil, data2, false))) {
					__ERROR("__insert_or_update failed. (st=%d)(id=%d)", st, data2->id);
				}
			}
			if (st == TestMainStatus::Ok) {
				if ((st = __insert_or_update(testUtil, data3, true))) {
					__ERROR("__insert_or_update temp failed. (st=%d)(id=%d)", st, data3->id);
				}
			}
			if (st == TestMainStatus::Ok) {
				if ((st = __insert_or_update(testUtil, data4, true))) {
					__ERROR("__insert_or_update temp failed. (st=%d)(id=%d)", st, data4->id);
				}
			}
			if (st == TestMainStatus::Redoing) {
				__INFO("Redo the process. SQLITE_BUSY has been acquired.");
				continue;
			}
			break;
		}
		if (st != TestMainStatus::Ok) {
			__FATAL("insert Table002 failed. (st=%d)", st);
			goto END;
		}

		__INFO("### CHECK 1");
		{
			Table002 *t = NULL;
			int n_t;
			st = testUtil->selectAllView(&t, &n_t);
			if (st) {
				__FATAL("selectAllView failed. (res=%d)", st);
				if (t) { free(t); }
				goto END;
			}
			if (n_t != 3 || t == NULL) {
				__FATAL("invalid select data. (count=%d)(t=%s)", n_t, (t == NULL)?"null":"notnull");
				if (t) { free(t); }
				goto END;
			}
			for (int j = 0; j < n_t; j++) {
				Table002 *s = &(t[j]);
				Table002 *d = &(viewVerificationData[j]);
				// __INFO("CC[%s]", s->hashed_id);
				if (__diffTable002(testUtil, s, d)) {
					__FATAL("diff select data. (number=%d)", j);
					if (t) { free(t); }
					goto END;
				}
			}
			if (t) { free(t); }
		}

		__INFO("### CHECK 2");
		{
			Table002 t;
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {
					bool isTemp = (j != 0);

					int id;
					if (k == 0) {
						id = 1;
					} else {
						id = isTemp ? 3 : 2;
					}

					if (testUtil->selectTable002Byid(id, isTemp, &t)) {
						__FATAL("invalid select data. (isTemp=%d)(id=%d)", isTemp, id);
						goto END;
					}
					Table002 *d = isTemp ? &(tmptableVerificationData[k]) : &(tableVerificationData[k]);
					if (__diffTable002(testUtil, &t, d)) {
						__FATAL("diff select data. (isTemp=%d)", isTemp);
						goto END;
					}
				}
			}
		}

		__ASSERT("test%d is ok", i + 1);
		myMicroSleep(2);
	}

END:
	TestMainStatus tempSt = st;
	st = TestMainStatus::Ok;
	LocalEndTransaction;
	return tempSt;

	// LocalEndTransaction;
	// return st;
#undef LocalEndTransaction
}

TestMainStatus
runTest002_02(TestUtility *testUtil, int testCount)
{
	{
		time_t t = (unsigned int)time(NULL);
		unsigned int n = (unsigned int)((t % 50000) + testUtil->threadID);
		__INFO("srand(%u)", n);
		srand(n);
	}

	TestMainStatus st = TestMainStatus::Ok;
	for (int i = 0; i < testCount; i++) {
		long r = random();
		const int isTemp = (int)(r % 2);

		int deleteId = 0;
		{
			int isOdd = random() % 2;
			if (isTemp) {
				deleteId = isOdd ? 3 : 1;
			} else {
				deleteId = isOdd ? 2 : 1;
			}
		}
		__INFO("START%d: delete (isTemp=%d)(id=%d)", i+1, isTemp, deleteId);

		while (true) {
			st = TestMainStatus::Ok;
			StartTransaction();

			// 事前に全4レコードをランダムにセレクト、失敗かどうかは気にしない
			{
				Table002 t;
				TestMainStatus st2;
				long v = ((int)random() % (0xf + 1));
				if (v & 0x01) {
					if ((st2 = testUtil->selectTable002Byid(1, false, &t)) != TestMainStatus::Ok) {
						__ERROR("selectTable002Byid failed. (st=%d)(id=1)", st2);
					}
				}
				if (v & 0x02) {
					if ((st2 = testUtil->selectTable002Byid(2, false, &t)) != TestMainStatus::Ok) {
						__ERROR("selectTable002Byid failed. (st=%d)(id=2)", st2);
					}
				}
				if (v & 0x04) {
					if ((st2 = testUtil->selectTable002Byid(1, true, &t)) != TestMainStatus::Ok) {
						__ERROR("selectTable002Byid failed. (st=%d)(temp id=1)", st2);
					}
				}
				if (v & 0x08) {
					if ((st2 = testUtil->selectTable002Byid(3, true, &t)) != TestMainStatus::Ok) {
						__ERROR("selectTable002Byid failed. (st=%d)(temp id=3)", st2);
					}
				}
			}

			st = testUtil->deleteTable002(deleteId, isTemp);
			if (st == TestMainStatus::Redoing) {
				__INFO("Redo the process. SQLITE_BUSY has been acquired.");
				EndTransaction();
				continue;
			}
			break;
		}
		EndTransaction();

		if (st) {
			__FATAL("deleteTable002 failed. (st=%d)(id=%d)", st, deleteId);
			return st;
		}

		__ASSERT("test%d is ok", i + 1);
		myMicroSleep(2);
	}

	return st;
}

TestMainStatus
__insert_or_update(TestUtility *testUtil, Table002 *table002, bool isTemp)
{
	__INFO("start.");

	// select してupdateかinsertか決める
	bool isInsert;
	{
		Table002 t;
		TestMainStatus st = testUtil->selectTable002(table002->contents_code, isTemp, &t);
		isInsert = (st != TestMainStatus::Ok);
	}
	// update or insert
	TestMainStatus st;
	if (isInsert) {
		st = testUtil->insertTable002(table002, isTemp);
	} else {
		st = testUtil->updateTable002(table002, isTemp);
	}
	if (st != TestMainStatus::Ok) {
		__FATAL("insert or update failed.");
		goto END;
	}
	// 検証
	{
		Table002 t;
		st = testUtil->selectTable002(table002->contents_code, isTemp, &t);
		if (st != TestMainStatus::Ok) {
			__FATAL("select failed.");
			goto END;
		}
		if (__diffTable002(testUtil, &t, table002)) {
			st = TestMainStatus::Error;
			goto END;
		}
	}

	if (isInsert) {
		__INFO("Insert success. (id=%d)", table002->id);
	} else {
		__INFO("Update success. (id=%d)", table002->id);
	}

END:
	__INFO("end. (st=%d)", st);
	return st;
}

TestMainStatus
__delete_and_insert(TestUtility *testUtil, Table002 *table002, bool isTemp)
{
	__INFO("start.");

	bool isDeleted = false;
	TestMainStatus st = testUtil->deleteTable002(table002->id, isTemp);
	if (st != TestMainStatus::Ok) {
		isDeleted = true;
	}

	st = testUtil->insertTable002(table002, isTemp);
	if (st != TestMainStatus::Ok) {
		__FATAL("insert failed. %d.", table002->id);
		goto END;
	}

	// 検証
	{
		Table002 t;
		st = testUtil->selectTable002(table002->contents_code, isTemp, &t);
		if (st != TestMainStatus::Ok) {
			__FATAL("select failed.");
			goto END;
		}
		if (__diffTable002(testUtil, &t, table002)) {
			st = TestMainStatus::Error;
			goto END;
		}
	}

	if (isDeleted) {
		__INFO("Not found record, Insert success. (id=%d)", table002->id);
	} else {
		__INFO("Delete and Insert success. (id=%d)", table002->id);
	}

END:
	EndTransaction();
	__INFO("end.");
	return st;
}

void
__dumpTable002(TestUtility *testUtil, Table002 *s, Table002 *d)
{
	__INFO("contents_code: [%s], [%s].", s->contents_code, d->contents_code);
	__INFO("    hashed_id: [%s], [%s].", s->hashed_id, d->hashed_id);
	__INFO("           id: %d, %d.", s->id, d->id);
	__INFO("       secret: %d, %d.", s->secret, d->secret);
	__INFO("read_datetime: [%s], [%s].", s->read_datetime, d->read_datetime);
	__INFO("delete_status: %d, %d.", s->delete_status, d->delete_status);
}

bool
__diffTable002(TestUtility *testUtil, Table002 *s, Table002 *d)
{
	bool hasDiff = false;

	if (strcmp(s->contents_code, d->contents_code)) {
		__FATAL("DIFF: contents_code: [%s], [%s].", s->contents_code, d->contents_code);
		hasDiff = true;
	}
	if (strcmp(s->hashed_id, d->hashed_id)) {
		__FATAL("DIFF: hashed_id: [%s], [%s].", s->hashed_id, d->hashed_id);
		hasDiff = true;
	}
	if (s->id != d->id) {
		__FATAL("DIFF: id: %d, %d.", s->id, d->id);
		hasDiff = true;
	}
	if (s->secret != d->secret) {
		__FATAL("DIFF: secret: %d, %d.", s->secret, d->secret);
		hasDiff = true;
	}
	if (strcmp(s->read_datetime, d->read_datetime)) {
		__FATAL("DIFF: read_datetime: [%s], [%s].", s->read_datetime, d->read_datetime);
		hasDiff = true;
	}
	if (s->delete_status != d->delete_status) {
		__FATAL("DIFF: delete_status: %d, %d.", s->delete_status, d->delete_status);
		hasDiff = true;
	}
	if (hasDiff) {
		__dumpTable002(testUtil, s, d);
	}
	return hasDiff;
}
