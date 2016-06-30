#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>
#include "dispatch.h"
/* ------------ */
#define OUTPUT_FILE "unittest"
/* ------------ */
typedef enum {
  TSTTASK_INIT = 0,
  TSTTASK_RUN,
  TSTTASK_CANCEL,
  TSTTASK_DONE,
  TSTTASK_MAX
} test_state;
/* ------------ */
typedef struct {
  test_state m_state;
  pthread_mutex_t m_runLock;
  pthread_mutex_t m_cancelLock;
  pthread_mutex_t m_endLock;
  pthread_cond_t m_runCond;
  pthread_cond_t m_cancelCond;
  pthread_cond_t m_endCond;
  int m_run;
  int m_end;
  int m_cancel;
  } test_context;
/* ------------ */
static int _runTests(const char *);
static void _paramTest_01(void);
static void _paramTest_02(void);
static void _paramTest_03(void);
static void _paramTest_04(void);
static void _paramTest_05(void);
static void _paramTest_06(void);
static void _paramTest_07(void);
static void _func_test_setUp(void);
static void _func_test_tearDown(void);
static void _funcTest_01(void);
static void _funcTest_02(void);
static void _funcTest_03(void);
static void _funcTest_04(void);
static void _funcTest_05(void);
static void _funcTest_06(void);
static void _funcTest_07(void);
static void _funcTest_08(void);
static void _funcTest_09(void);
static void _funcTest_10(void);
static void _funcTest_11(void);
static void _funcTest_12(void);
static void _funcTest_13(void);
static void _funcTest_14(void);
static void _funcTest_15(void);
static void _funcTest_16(void);
static void _funcTest_17(void);
static void _taskContextInit(test_context *);
static void _taskContextDestroy(test_context *);
static void _taskContextSignalRun(test_context *);
static void _taskContextWaitCancel(test_context *);
static void * _taskRun(dispatch_task, void *);
static void _taskFin(dispatch_task, void *, void *);
static void _taskCnl(dispatch_task, void *);
static void * _taskRunReschedule(dispatch_task, void *);
static void _taskFinReschedule(dispatch_task, void *, void *);
static void * _taskRunWait(dispatch_task, void *);
static void _taskFinWait(dispatch_task, void *, void *);
/* ------------ */
static CU_TestInfo param_tests[] = {
  { "Paramater Test 01", _paramTest_01 },
  { "Paramater Test 02", _paramTest_02 },
  { "Paramater Test 03", _paramTest_03 },
  { "Paramater Test 04", _paramTest_04 },
  { "Paramater Test 05", _paramTest_05 },
  { "Paramater Test 06", _paramTest_06 },
  { "Paramater Test 07", _paramTest_07 },  
  CU_TEST_INFO_NULL
};
/* ------------ */
static CU_TestInfo funct_tests[] = {
  { "Functionality Test 01", _funcTest_01 },
  { "Functionality Test 02", _funcTest_02 },
  { "Functionality Test 03", _funcTest_03 },
  { "Functionality Test 04", _funcTest_04 },
  { "Functionality Test 05", _funcTest_05 },
  { "Functionality Test 06", _funcTest_06 },
  { "Functionality Test 07", _funcTest_07 },
  { "Functionality Test 08", _funcTest_08 },
  { "Functionality Test 09", _funcTest_09 },
  { "Functionality Test 10", _funcTest_10 },
  { "Functionality Test 11", _funcTest_11 },
  { "Functionality Test 12", _funcTest_12 },
  { "Functionality Test 13", _funcTest_13 },
  { "Functionality Test 14", _funcTest_14 },
  { "Functionality Test 15", _funcTest_15 },
  { "Functionality Test 16", _funcTest_16 },
  { "Functionality Test 17", _funcTest_17 },  
  CU_TEST_INFO_NULL
};
/* ------------ */
static CU_SuiteInfo suites[] = {
  {
    "Paramater Tests",
    NULL,
    NULL,
    NULL,
    NULL,
    param_tests
  },
  {
    "Functionality Tests",
    NULL,
    NULL,
    _func_test_setUp,
    _func_test_tearDown,
    funct_tests
  },
  CU_SUITE_INFO_NULL
};
/* ------------ */

static void
_paramTest_01(void) {
  int testBuffer[4];
  CU_ASSERT(NULL == dispatch_init(NULL, NULL, NULL));
  CU_ASSERT(NULL == dispatch_init(NULL, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_init(NULL, _taskCnl, NULL));
  CU_ASSERT(NULL == dispatch_init(NULL, _taskCnl, &testBuffer[0]));  
}
/* ------------ */
static void
_paramTest_02(void) {
  int testBuffer[4];
  CU_ASSERT(NULL == dispatch_initEx(NULL, NULL, NULL, NULL));
  CU_ASSERT(NULL == dispatch_initEx(NULL, NULL, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(NULL, NULL, _taskFin, NULL));
  CU_ASSERT(NULL == dispatch_initEx(NULL, NULL, _taskFin, &testBuffer[0]));  
  CU_ASSERT(NULL == dispatch_initEx(NULL, _taskCnl, _taskFin, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(NULL, _taskCnl, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(NULL, _taskCnl, NULL, NULL));    
  CU_ASSERT(NULL == dispatch_initEx(_taskRun, NULL, NULL, NULL));
  CU_ASSERT(NULL == dispatch_initEx(_taskRun, NULL, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(_taskRun, _taskCnl, NULL, &testBuffer[0]));
  CU_ASSERT(NULL == dispatch_initEx(_taskRun, NULL, _taskFin, &testBuffer[0]));
}
/* ------------ */
static void
_paramTest_03(void) {
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_destroy(NULL));
}
/* ------------ */
static void
_paramTest_04(void) {
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_schedule(NULL));
}
/* ------------ */
static void
_paramTest_05(void) {
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_cancel(NULL));
}
/* ------------ */
static void
_paramTest_06(void) {
  dispatch_task testTasks[1];
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_wait(NULL, 0));
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_wait(NULL, 5));
  CU_ASSERT(DISPATCH_INVALID_OPERAND == dispatch_wait(testTasks, 0));
}
/* ------------ */
static void
_paramTest_07(void) {
  CU_ASSERT(NULL == dispatch_return(NULL));
}
/* ------------ */
static void
_func_test_setUp(void) {
  CU_ASSERT(DISPATCH_OK == dispatch_engine_init());
}
/* ------------ */
static void
_func_test_tearDown(void) {
  CU_ASSERT(DISPATCH_OK == dispatch_engine_destroy());
}
/* ------------ */
static void
_funcTest_01(void) {
  /* does nothing */
}
/* ------------ */
static void
_funcTest_02(void) {
  dispatch_task pTask;
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, NULL)));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
}
/* ------------ */
static void
_funcTest_03(void) {
  dispatch_task pTask;
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     NULL)));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
}
/* ------------ */
static void
_funcTest_04(void) {
  dispatch_task pTask;
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, NULL)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
}
/* ------------ */
static void
_funcTest_05(void) {
  dispatch_task pTask;
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     NULL)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
}
/* ------------ */
static void
_funcTest_06(void) {
  dispatch_task pTask;
  test_context ctx;
  /* init task */
  _taskContextInit(&ctx);
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  /* signal task to run */
  _taskContextSignalRun(&ctx);
  /* wait for completion */
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  /* verify results */
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(TSTTASK_RUN == ctx.m_state);
  /* destroy task */
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  _taskContextDestroy(&ctx);
}
/* ------------ */
static void
_funcTest_07(void) {
  dispatch_task pTask;
  test_context ctx;
  /* init task */
  _taskContextInit(&ctx);
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  /* signal task to run */
  _taskContextSignalRun(&ctx);  
  /* wait for completion */  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(&ctx == dispatch_return(pTask));
  /* verify results */  
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(1 == ctx.m_end);
  CU_ASSERT(TSTTASK_DONE == ctx.m_state);
  /* destroy task */  
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  _taskContextDestroy(&ctx);
}
/* ------------ */
static void
_funcTest_08(void) {
  int ndx;
  const int nTasks = (int)sysconf(_SC_NPROCESSORS_ONLN) * 2;
  dispatch_task * ppTask = malloc(sizeof(dispatch_task) * nTasks);
  test_context * pCtx = malloc(sizeof(test_context) * nTasks);
  /* init tasks */
  for (ndx = 0; ndx < nTasks; ++ndx) {
    _taskContextInit(&pCtx[ndx]);
    CU_ASSERT(NULL != (ppTask[ndx] = dispatch_init(_taskRun,
						   _taskCnl,
						   &pCtx[ndx])));    
  }
  /* run tasks */
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(DISPATCH_OK == dispatch_schedule(ppTask[ndx]));
    _taskContextSignalRun(&pCtx[ndx]);
  }  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(ppTask, nTasks));
  /* verify results */  
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(&pCtx[ndx] == dispatch_return(ppTask[ndx]));
    CU_ASSERT(1 == pCtx[ndx].m_run);
    CU_ASSERT(TSTTASK_RUN == pCtx[ndx].m_state);
  }
  /* destroy tasks */
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(DISPATCH_OK == dispatch_destroy(ppTask[ndx]));
    _taskContextDestroy(&pCtx[ndx]);
  }
  free(ppTask);
  free(pCtx);
}
/* ------------ */
static void
_funcTest_09(void) {
  int ndx;
  const int nTasks = (int)sysconf(_SC_NPROCESSORS_ONLN) * 2;
  dispatch_task * ppTask = malloc(sizeof(dispatch_task) * nTasks);
  test_context * pCtx = malloc(sizeof(test_context) * nTasks);
  /* init tasks */
  for (ndx = 0; ndx < nTasks; ++ndx) {
    _taskContextInit(&pCtx[ndx]);
    CU_ASSERT(NULL != (ppTask[ndx] =
		       dispatch_initEx(_taskRun,
				       _taskCnl,
				       _taskFin, &pCtx[ndx])));
  }  
  /* run tasks */
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(DISPATCH_OK == dispatch_schedule(ppTask[ndx]));
    _taskContextSignalRun(&pCtx[ndx]);
  }  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(ppTask, nTasks));
  /* verify results */
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(&pCtx[ndx] == dispatch_return(ppTask[ndx]));
    CU_ASSERT(1 == pCtx[ndx].m_run);
    CU_ASSERT(1 == pCtx[ndx].m_end);
    CU_ASSERT(TSTTASK_DONE == pCtx[ndx].m_state);
  }
  /* destroy tasks */
  for (ndx = 0; ndx < nTasks; ++ndx) {
    CU_ASSERT(DISPATCH_OK == dispatch_destroy(ppTask[ndx]));
    _taskContextDestroy(&pCtx[ndx]);
  }
  free(ppTask);
  free(pCtx);
}
/* ------------ */
static void
_funcTest_10(void) {
  dispatch_task pTask;
  test_context ctx;
  /* init task */
  _taskContextInit(&ctx);
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, &ctx)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  /* signal task to run */  
  _taskContextSignalRun(&ctx);
  /* wait for completion */  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  /* verify results */  
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(TSTTASK_RUN == ctx.m_state);
  /* reschedule task */
  ctx.m_state = TSTTASK_INIT;
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  _taskContextSignalRun(&ctx);
  /* wait for completion */
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  /* verify results */
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(2 == ctx.m_run);
  CU_ASSERT(TSTTASK_RUN == ctx.m_state);
  /* destroy task */
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  _taskContextDestroy(&ctx);
}
/* ------------ */
static void
_funcTest_11(void) {
  dispatch_task pTask;
  test_context ctx;
  /* init task */
  _taskContextInit(&ctx);
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     &ctx))); 
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  /* signal task to run */  
  _taskContextSignalRun(&ctx);
  /* wait for completion */  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  /* verify results */  
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(1 == ctx.m_end);
  CU_ASSERT(TSTTASK_DONE == ctx.m_state);
  /* reschedule task */
  ctx.m_state = TSTTASK_INIT;
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  _taskContextSignalRun(&ctx);
  /* wait for completion */
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  /* verify results */
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(2 == ctx.m_run);
  CU_ASSERT(2 == ctx.m_end);
  CU_ASSERT(TSTTASK_DONE == ctx.m_state);  
  /* destroy task */
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  _taskContextDestroy(&ctx);
}
/* ------------ */
static void
_funcTest_12(void) {
  dispatch_task pTask;
  test_context ctx;
  /* init task */
  _taskContextInit(&ctx);
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRunReschedule,
					     _taskCnl,
					     _taskFinReschedule,
					     &ctx)));
  /* run task */
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  _taskContextSignalRun(&ctx);  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  /* verify results */
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(1 == ctx.m_end);
  CU_ASSERT(TSTTASK_DONE == ctx.m_state);
  /* destroy task */
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  _taskContextDestroy(&ctx);  
}
/* ------------ */
static void
_funcTest_13(void) {
  dispatch_task pTask;
  test_context ctx;
  /* init task */
  _taskContextInit(&ctx);
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRunWait,
					     _taskCnl,
					     _taskFinWait,
					     &ctx)));
  /* run task */  
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  _taskContextSignalRun(&ctx);  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  /* verify results */
  CU_ASSERT(&ctx == dispatch_return(pTask));
  CU_ASSERT(1 == ctx.m_run);
  CU_ASSERT(1 == ctx.m_end);
  CU_ASSERT(TSTTASK_DONE == ctx.m_state);
  /* destroy task */
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  _taskContextDestroy(&ctx);
}
/* ------------ */
static void
_funcTest_14(void) {
  dispatch_task pTask;
  test_context ctx;
  /* init task */
  _taskContextInit(&ctx);
  CU_ASSERT(NULL != (pTask = dispatch_initEx(_taskRun,
					     _taskCnl,
					     _taskFin,
					     &ctx)));
  /* run task */    
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  pthread_yield();  /* wait for task to hit lock */
  dispatch_cancel(pTask);
  _taskContextSignalRun(&ctx);  
  _taskContextWaitCancel(&ctx);
  /* destroy task */  
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));  
  _taskContextDestroy(&ctx);
}
/* ------------ */
static void
_funcTest_15(void) {
  dispatch_task pTask;
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, NULL)));
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_cancel(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
}
/* ------------ */
static void
_funcTest_16(void) {
  dispatch_task pTask;
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, NULL)));
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_cancel(pTask));  
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
}
/* ------------ */
static void
_funcTest_17(void) {
  dispatch_task pTask;
  test_context ctx;
  /* init task */
  _taskContextInit(&ctx);  
  CU_ASSERT(NULL != (pTask = dispatch_init(_taskRun, _taskCnl, &ctx)));
  /* signal task to run */  
  CU_ASSERT(DISPATCH_OK == dispatch_schedule(pTask));
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_destroy(pTask));
  _taskContextSignalRun(&ctx);  
  CU_ASSERT(DISPATCH_OK == dispatch_wait(&pTask, 1));
  /* destroy task */  
  CU_ASSERT(DISPATCH_OK == dispatch_destroy(pTask));
  _taskContextDestroy(&ctx);
}
/* ------------ */
static void
_taskContextInit(test_context * pCtx) {
  test_context init = {
    TSTTASK_INIT,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    PTHREAD_COND_INITIALIZER,    
    0,
    0,
    0
  };
  *pCtx = init;
}
/* ------------ */
static void
_taskContextDestroy(test_context * pCtx) {
  pthread_mutex_destroy(&pCtx->m_runLock);
  pthread_mutex_destroy(&pCtx->m_cancelLock);
  pthread_mutex_destroy(&pCtx->m_endLock);
  pthread_cond_destroy(&pCtx->m_runCond);
  pthread_cond_destroy(&pCtx->m_cancelCond);
  pthread_cond_destroy(&pCtx->m_endCond);
}
/* ------------ */
static void
_taskContextSignalRun(test_context * pCtx) {
  pthread_mutex_lock(&pCtx->m_runLock);
  pCtx->m_state = TSTTASK_RUN;
  pthread_cond_signal(&pCtx->m_runCond);
  pthread_mutex_unlock(&pCtx->m_runLock);
}
/* ------------ */
static void
_taskContextWaitCancel(test_context * pCtx) {
  pthread_mutex_lock(&pCtx->m_cancelLock);
  while (TSTTASK_CANCEL != pCtx->m_state) {
    pthread_cond_wait(&pCtx->m_cancelCond, &pCtx->m_cancelLock);
  }
  pthread_mutex_unlock(&pCtx->m_cancelLock);
}
/* ------------ */
static void *
_taskRun(dispatch_task pTask, void * pContext) {
  test_context * pCtx = (test_context *)pContext;
  if (pCtx) {    
    /* wait for run signal */
    pthread_mutex_lock(&pCtx->m_runLock);
    while (TSTTASK_RUN != pCtx->m_state) {
      pthread_cond_wait(&pCtx->m_runCond, &pCtx->m_runLock);
    }
    pthread_mutex_unlock(&pCtx->m_runLock);
    /* update run counter */    
    pCtx->m_run++;
  }    
  return pCtx;
}
/* ------------ */
static void
_taskFin(const dispatch_task pTask, void * pContext, void * pReturn) {
  test_context * pCtx = (test_context *)pContext;
  if (pCtx) {
    /* update end counter */
    pCtx->m_end++;    
    /* signal task end */
    pthread_mutex_lock(&pCtx->m_endLock);
    pCtx->m_state = TSTTASK_DONE;
    pthread_cond_signal(&pCtx->m_endCond);
    pthread_mutex_unlock(&pCtx->m_endLock);
  }
}
/* ------------ */
static void
_taskCnl(const dispatch_task pTask, void * pContext) {
  test_context * pCtx = (test_context *)pContext;
  if (pCtx) {
    /* update cancel counter */
    pCtx->m_cancel++;
    /* signal task canceled */
    pthread_mutex_lock(&pCtx->m_cancelLock);
    pCtx->m_state = TSTTASK_CANCEL;
    pthread_cond_signal(&pCtx->m_cancelCond);
    pthread_mutex_unlock(&pCtx->m_cancelLock);
  }
}
/* ------------ */
static void *
_taskRunReschedule(dispatch_task pTask, void * pContext) {
  _taskRun(pTask, pContext);
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_schedule(pTask));
  return pContext;
}
/* ------------ */
static void
_taskFinReschedule(dispatch_task pTask, void * pContext, void * pReturn) {
  _taskFin(pTask, pContext, pReturn);
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_schedule(pTask));
}
/* ------------ */
static void *
_taskRunWait(dispatch_task pTask, void * pContext) {
  _taskRun(pTask, pContext);
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_wait(&pTask, 1));
  return pContext;
}
/* ------------ */
static void
_taskFinWait(dispatch_task pTask, void * pContext, void * pReturn) {
  _taskFin(pTask, pContext, pReturn);
  CU_ASSERT(DISPATCH_INVALID_OPERATION == dispatch_wait(&pTask, 1));
}
/* ------------ */
static int
_runTests(const char * pFileName) {
  int retVal = 0;
  CU_set_error_action(CUEA_IGNORE);
  /* setup tests */
  if (!(retVal = CU_initialize_registry())) {
    /* add tests cases */
    CU_register_suites(suites);
    /* setup output options */
    CU_set_output_filename(pFileName);
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_list_tests_to_file();
    /* run tests */
    CU_automated_run_tests();
    CU_basic_run_tests();
    /* shutdown */
    CU_cleanup_registry();
  }
  return retVal;  
}
/* ------------ */
int
main(void) {
  return _runTests(OUTPUT_FILE);
}
