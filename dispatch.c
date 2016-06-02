/*
 * Copyright (C) 2016 Ardavon Falls
 * This file is part of libdispatch
 *
 * Libdispatch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation, either version 3 of the 
 * License, or any later version.
 *
 * Libdispatch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "dispatch.h"
/* ------------ */
#define STACKSZ (64 * 1024)
#define WORKQUEUESZ (4)
#define THREAD_NAME "dispatch-%lu"
/* ------------ */
typedef enum {
  DISPATCH_INIT = 0,
  DISPATCH_SCHEDULED,
  DISPATCH_CANCELED,
  DISPATCH_RAN,
  DISPATCH_MAX
} task_state;
/* ------------ */
struct dispatch_task {
  dispatch_func m_run;
  dispatch_cnlCb m_cancel;
  dispatch_endCb m_end;
  pthread_mutex_t m_lock;
  pthread_mutex_t m_waitLock;
  pthread_cond_t m_wait;
  void * m_pCtx;
  void * m_pReturn;
  task_state m_state;
};
/* ------------ */
typedef struct {
  sem_t m_full;
  sem_t m_empty;
  unsigned int m_start;
  unsigned int m_end;
  dispatch_task m_queue[WORKQUEUESZ];
} dispatch_queue;
/* ------------ */
typedef struct {  
  size_t m_nThreads;
  pthread_mutex_t m_schdlLock;
  int m_quit;
  dispatch_queue * m_pQueues;  
  pthread_t m_threads[];
} dispatch_engine;
/* ------------ */
typedef struct {
  int * m_pQuit;
  dispatch_queue * m_pQueue;
} dispatch_threadCtx;
/* ------------ */
static dispatch_engine* gspTaskEngine;
/* ------------ */
static dispatch_error _initThreads(pthread_t[], const size_t);
static void _destroyThreads(pthread_t[], dispatch_queue[], const size_t);
static void * _threadProc(void *);
static dispatch_error _initQueues(dispatch_queue[], const size_t);
static void _destroyQueues(dispatch_queue[], const size_t);
static unsigned _minItemsQueues(dispatch_queue[], const size_t, unsigned);
static void _insertQueues(dispatch_queue[],
			  dispatch_task,
			  const size_t,
			  const unsigned);
static void _queueInsert(dispatch_queue *, dispatch_task);
static dispatch_task _queueRemove(dispatch_queue *);
static int _isCallingFromThreads(pthread_t[], const size_t, const pthread_t);
static void _cancelWaiting(dispatch_task[], unsigned, unsigned);
/* ------------ */
dispatch_error
dispatch_engine_init(void) {
  /* todo: add error handling */
  dispatch_error err = DISPATCH_OK;
  const unsigned nProcessors = (unsigned)sysconf(_SC_NPROCESSORS_ONLN);
  const size_t deStrcSize = sizeof(dispatch_engine);
  const size_t dqObjSize = sizeof(dispatch_queue) * nProcessors;
  const size_t deObjSize = deStrcSize + (sizeof(pthread_t) * nProcessors);
  
  if (!(gspTaskEngine = malloc(deObjSize))) {
    err = DISPATCH_MEMORY_ERR;
    goto dispatch_engine_init_err;
  }
  memset(gspTaskEngine, 0, deObjSize);
  gspTaskEngine->m_nThreads = nProcessors;
  if (pthread_mutex_init(&gspTaskEngine->m_schdlLock, NULL)) {
    err = DISPATCH_INTERNAL_ERR;
    goto dispatch_engine_init_err;    
  }
  if (!(gspTaskEngine->m_pQueues = malloc(dqObjSize))) {
    err = DISPATCH_MEMORY_ERR;
    goto dispatch_engine_init_err;    
  }
  memset(gspTaskEngine->m_pQueues, 0, dqObjSize);
  if ((err = _initQueues(gspTaskEngine->m_pQueues, nProcessors))) {
    goto dispatch_engine_init_err;
  }
  if ((err = _initThreads(gspTaskEngine->m_threads, nProcessors))) {
    goto dispatch_engine_init_err;    
  }
  return err;
 dispatch_engine_init_err:
  if (gspTaskEngine && gspTaskEngine->m_pQueues)
    free(gspTaskEngine->m_pQueues);
  if (gspTaskEngine)
    free(gspTaskEngine);
  return err;
}
/* ------------ */
dispatch_error
dispatch_engine_destroy(void) {
  if (gspTaskEngine) {
    gspTaskEngine->m_quit = -1;
    _destroyThreads(gspTaskEngine->m_threads,
		    gspTaskEngine->m_pQueues,
		    gspTaskEngine->m_nThreads);
    _destroyQueues(gspTaskEngine->m_pQueues, gspTaskEngine->m_nThreads);
    pthread_mutex_destroy(&gspTaskEngine->m_schdlLock);
    free(gspTaskEngine->m_pQueues);
    free(gspTaskEngine);
    return DISPATCH_OK;
  }
  return DISPATCH_INVALID_OPERATION;
}
/* ------------ */
dispatch_task
dispatch_init(dispatch_func pFunc, dispatch_cnlCb pCancel, void * pContext) {
  struct dispatch_task * pTask = malloc(sizeof(struct dispatch_task));
  if (!pTask || !pFunc || !pCancel) {
    goto dispatch_init_err;
  }
  memset(pTask, 0, sizeof(struct dispatch_task));
  pTask->m_run = pFunc;
  pTask->m_cancel = pCancel;
  pTask->m_pCtx = pContext;
  if (pthread_mutex_init(&pTask->m_lock, NULL)) {
    goto dispatch_init_err;
  }
  if (pthread_mutex_init(&pTask->m_waitLock, NULL)) {
    pthread_mutex_destroy(&pTask->m_lock);
    goto dispatch_init_err;
  }
  if (pthread_cond_init(&pTask->m_wait, NULL)) {
    pthread_mutex_destroy(&pTask->m_waitLock);
    pthread_mutex_destroy(&pTask->m_lock);
    goto dispatch_init_err;
  }
  return pTask;
 dispatch_init_err:
  if (pTask) free(pTask);
  return NULL;
}
/* ------------ */
dispatch_task
dispatch_initEx(dispatch_func pFunc,
		dispatch_cnlCb pCancel,
		dispatch_endCb pEnd,
		void * pContext) {
  struct dispatch_task * pTask = malloc(sizeof(struct dispatch_task));
  if (!pTask || !pFunc || !pCancel || !pEnd) {
    goto dispatch_initEx_err;
  }
  memset(pTask, 0, sizeof(struct dispatch_task));
  pTask->m_run = pFunc;
  pTask->m_cancel = pCancel;
  pTask->m_end = pEnd;
  pTask->m_pCtx = pContext;
  if (pthread_mutex_init(&pTask->m_lock, NULL)) {
    goto dispatch_initEx_err;
  }
  if (pthread_mutex_init(&pTask->m_waitLock, NULL)) {
    pthread_mutex_destroy(&pTask->m_lock);
    goto dispatch_initEx_err;
  }
  if (pthread_cond_init(&pTask->m_wait, NULL)) {
    pthread_mutex_destroy(&pTask->m_waitLock);
    pthread_mutex_destroy(&pTask->m_lock);
    goto dispatch_initEx_err;
  }
  return pTask;
 dispatch_initEx_err:
  if (pTask) free(pTask);
  return NULL; 
}
/* ------------ */
dispatch_error
dispatch_schedule(dispatch_task pTask) {
  dispatch_error retVal = DISPATCH_INVALID_OPERAND;
  if (pTask) {
    /* check that its not being called from inside a task */
    if (_isCallingFromThreads(gspTaskEngine->m_threads,
			      gspTaskEngine->m_nThreads,
			      pthread_self())) {
      retVal = DISPATCH_INVALID_OPERATION;
    } else {    
      switch (pTask->m_state) {
      case DISPATCH_INIT:
      case DISPATCH_CANCELED:
      case DISPATCH_RAN: {
	unsigned min = -1;
	pTask->m_state = DISPATCH_SCHEDULED;
	pthread_mutex_lock(&gspTaskEngine->m_schdlLock);      
	min = _minItemsQueues(gspTaskEngine->m_pQueues,
			      gspTaskEngine->m_nThreads,
			      min);
	_insertQueues(gspTaskEngine->m_pQueues,
		      pTask,
		      gspTaskEngine->m_nThreads,
		      min); 
	pthread_mutex_unlock(&gspTaskEngine->m_schdlLock);
	retVal = DISPATCH_OK;
	break;
      }
      default:
	retVal = DISPATCH_INVALID_OPERATION;
	break;
      }
    }
  }
  return retVal;
}
/* ------------ */
dispatch_error
dispatch_cancel(dispatch_task pTask) {
  dispatch_error retVal = DISPATCH_INVALID_OPERAND;
  if (pTask) {
    pthread_mutex_lock(&pTask->m_lock);
    switch (pTask->m_state) {
    case DISPATCH_SCHEDULED: {
      pTask->m_state = DISPATCH_CANCELED;
      retVal = DISPATCH_OK;
      break;
    }
    default:
      retVal = DISPATCH_INVALID_OPERATION;
    }
    pthread_mutex_unlock(&pTask->m_lock);    
  }
  return retVal;
}
/* ------------ */
dispatch_error
dispatch_wait(dispatch_task tasks[], size_t length) {
  /* verify arguments */
  if (NULL == tasks || 0 == length) {
    return DISPATCH_INVALID_OPERAND;
  }
  /* check that its not being called from inside a task */
  if (_isCallingFromThreads(gspTaskEngine->m_threads,
			    gspTaskEngine->m_nThreads,
			    pthread_self())) {
    return DISPATCH_INVALID_OPERATION;
  }    
  /* wait for jobs to finish */
  if (length > 0) {
    const unsigned index = length - 1;
    pthread_mutex_lock(&tasks[index]->m_waitLock);      
    while (DISPATCH_SCHEDULED == tasks[index]->m_state) {
      pthread_cond_wait(&tasks[index]->m_wait, &tasks[index]->m_waitLock);
    }
    pthread_mutex_unlock(&tasks[index]->m_waitLock);
    dispatch_wait(tasks, index);
  }
  return DISPATCH_OK;
}
/* ------------ */
void *
dispatch_return(dispatch_task pTask) {
  void * pRetVal = NULL;
  if (pTask) {
    pthread_mutex_lock(&pTask->m_lock);
    switch (pTask->m_state) {
    case DISPATCH_RAN: {
      pRetVal = pTask->m_pReturn;
      break;
    }
    default:
      break;
    }
    pthread_mutex_unlock(&pTask->m_lock);      
  }
  return pRetVal;  
}
/* ------------ */
dispatch_error
dispatch_destroy(dispatch_task pTask) {
  dispatch_error retVal = DISPATCH_INVALID_OPERAND;
  if (pTask) {
    pthread_mutex_lock(&pTask->m_lock);
    switch (pTask->m_state) {
    case DISPATCH_INIT:
    case DISPATCH_CANCELED:
    case DISPATCH_RAN:
      pthread_mutex_unlock(&pTask->m_lock);
      pthread_cond_destroy(&pTask->m_wait);
      pthread_mutex_destroy(&pTask->m_waitLock);
      pthread_mutex_destroy(&pTask->m_lock);      
      free(pTask);
      retVal = DISPATCH_OK;
      break;
    default:
      pthread_mutex_unlock(&pTask->m_lock);    	    
      retVal = DISPATCH_INVALID_OPERATION;
    }
  }
  return retVal;
}
/* ------------ */
static dispatch_error
_initThreads(pthread_t threads[], const size_t nThreads) {
  char threadName[32];
  pthread_attr_t attributes;
  if (pthread_attr_init(&attributes)) {
    return DISPATCH_INTERNAL_ERR;
  }
  if (pthread_attr_setstacksize(&attributes, STACKSZ)) {
    pthread_attr_destroy(&attributes);
    return DISPATCH_INTERNAL_ERR;    
  }
  if (nThreads < 1) {
    if (pthread_attr_destroy(&attributes)) {
      return DISPATCH_INTERNAL_ERR;
    }
  } else {
    cpu_set_t cpuset;
    const size_t index = nThreads - 1;
    dispatch_threadCtx * pTContext = malloc(sizeof(dispatch_threadCtx));
    if (!pTContext) {
      return DISPATCH_MEMORY_ERR;
    }
    pTContext->m_pQuit = &gspTaskEngine->m_quit;
    pTContext->m_pQueue= &gspTaskEngine->m_pQueues[index];    
    if (pthread_create(&threads[index],
		       &attributes,
		       _threadProc,
		       pTContext)) {
      free(pTContext);
      pthread_attr_destroy(&attributes);
      return DISPATCH_INTERNAL_ERR;
    }
    CPU_SET(index, &cpuset);
    snprintf(threadName, sizeof(threadName) - 1, THREAD_NAME, index);
    /* if name isn't set, not a deal killing issue */
    pthread_setname_np(threads[index], threadName);
    /* if affinity isn't set, not a deal killing issue */
    pthread_setaffinity_np(threads[index], sizeof(cpu_set_t), &cpuset);
    pthread_attr_destroy(&attributes);
    return _initThreads(threads, index);
  }
  return DISPATCH_OK;
}
/* ------------ */
static void
_destroyThreads(pthread_t threads[],
		dispatch_queue queues[],
		const size_t nThreads) {
  if (nThreads > 0) {
    void * pThreadVal;
    const size_t index = nThreads - 1;
    sem_post(&queues[index].m_full);
    pthread_join(threads[index], &pThreadVal);
    free(pThreadVal);
    _destroyThreads(threads, queues, index);
  }
}
/* ------------ */
static void * _threadProc(void * pUser) {
  dispatch_threadCtx * pCtx = (dispatch_threadCtx *)pUser;
  while(0 == *pCtx->m_pQuit) {
    dispatch_task pTask = _queueRemove(pCtx->m_pQueue);
    if (0 == *pCtx->m_pQuit) {
      /* make sure task is in proper state (not canceled, etc..) */    
      if (DISPATCH_SCHEDULED == pTask->m_state) {
	/* execute task */
	pTask->m_pReturn = pTask->m_run(pTask, pTask->m_pCtx);
	/* update task state */
	pthread_mutex_lock(&pTask->m_lock);
	if (DISPATCH_SCHEDULED == pTask->m_state) {
	  pTask->m_state = DISPATCH_RAN;
	}
	pthread_mutex_unlock(&pTask->m_lock);
      }
      /* invoke finished or canceled callback */
      if (pTask->m_end && DISPATCH_RAN == pTask->m_state) {
	pTask->m_end(pTask, pTask->m_pCtx, pTask->m_pReturn);
      } else if (pTask->m_cancel && DISPATCH_CANCELED == pTask->m_state) {
	pTask->m_cancel(pTask, pTask->m_pCtx);
      }
      /* unlock condition variable for wait */     
      pthread_mutex_lock(&pTask->m_waitLock);
      pthread_cond_signal(&pTask->m_wait);
      pthread_mutex_unlock(&pTask->m_waitLock);
    }
  }
  return pUser;
}
/* ------------ */
static dispatch_error
_initQueues(dispatch_queue queues[], const size_t nQueues) {
  if (nQueues > 0) {
    const size_t index = nQueues - 1;
    queues[index].m_start = queues[index].m_end = 0;
    if (sem_init(&queues[index].m_full, 0, 0) ||
	sem_init(&queues[index].m_empty, 0, WORKQUEUESZ)) {
      return DISPATCH_INTERNAL_ERR;
    }    
    return _initQueues(queues, index);
  }
  return DISPATCH_OK;
}
/* ------------ */
static void
_destroyQueues(dispatch_queue queues[], const size_t nQueues) {
  if (nQueues > 0) {
    const size_t index = nQueues - 1;
    dispatch_queue * pQueue = &queues[index];
    _cancelWaiting(pQueue->m_queue, pQueue->m_start, pQueue->m_end);
    sem_destroy(&pQueue->m_empty);
    sem_destroy(&pQueue->m_full);
    _destroyQueues(queues, index);
  }
}
/* ------------ */
static void
_queueInsert(dispatch_queue * pQueue, dispatch_task task) {
  sem_wait(&pQueue->m_empty);
  pQueue->m_queue[pQueue->m_end++ & (WORKQUEUESZ - 1)] = task;
  sem_post(&pQueue->m_full);
}
/* ------------ */
static dispatch_task
_queueRemove(dispatch_queue * pQueue) {
  dispatch_task pRetTask = NULL;
  sem_wait(&pQueue->m_full);
  pRetTask = pQueue->m_queue[pQueue->m_start++ & (WORKQUEUESZ - 1)];
  sem_post(&pQueue->m_empty);
  return pRetTask;
}
/* ------------ */
static unsigned
_minItemsQueues(dispatch_queue queues[], const size_t nQueues, unsigned min) {
  if (nQueues > 0) {
    const size_t index = nQueues - 1;
    const unsigned items = queues[index].m_end - queues[index].m_start;
    min = (min > items) ? items : min;
    return _minItemsQueues(queues, index, min);
  }
  return min;
}
/* ------------ */
static void
_insertQueues(dispatch_queue queues[],
	      dispatch_task task,
	      const size_t nQueues,
	      const unsigned min) {
  if (nQueues > 0) {
    const size_t index = nQueues - 1;
    const int items = queues[index].m_end - queues[index].m_start;
    if (min == items) {
      _queueInsert(&queues[index], task);
    } else {
      _insertQueues(queues, task, index, min);
    }
  }
}
/* ------------ */
static int
_isCallingFromThreads(pthread_t threads[],
		      const size_t nThreads,
		      const pthread_t threadId) {
  if (nThreads > 0) {
    const size_t index = nThreads - 1;    
    return ((threadId == threads[index]) |
	    _isCallingFromThreads(threads, index, threadId));
  }
  return 0;
}
/* ------------ */
static void
_cancelWaiting(dispatch_task dispatch[], unsigned start, unsigned end) {
  if (start < end) {
    dispatch_task pTask = dispatch[start & (WORKQUEUESZ - 1)];
    pthread_mutex_lock(&pTask->m_lock);
    pTask->m_state = DISPATCH_CANCELED;
    pthread_mutex_lock(&pTask->m_lock);
    if (pTask->m_cancel) {
      pTask->m_cancel(pTask, pTask->m_pCtx);
    }
    _cancelWaiting(dispatch, start + 1, end);
  }
}
