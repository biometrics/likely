/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2013 Joshua C. Klontz                                           *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "likely/runtime.h"

using namespace std;

// Parallel synchronization
static condition_variable worker;
static mutex work;
static atomic<bool> *workers = NULL;
static size_t numWorkers = 0;

// Parallel data
static likely_thunk currentThunk = NULL;
static void *thunkArgs = NULL;
static size_t thunkSize = 0;

static void executeWorker(size_t id)
{
    // There are hardware_concurrency-1 helper threads and the main thread with id = 0
    const size_t step = (thunkSize+numWorkers-1)/numWorkers;
    const size_t start = id * step;
    const size_t stop = min((id+1)*step, thunkSize);
    if (start >= stop) return;
    currentThunk(thunkArgs, start, stop);
}

static void workerThread(size_t id)
{
    while (true) {
        {
            unique_lock<mutex> lock(work);
            workers[id] = false;
            while (!workers[id])
                worker.wait(lock);
        }

        executeWorker(id);
    }
}

void likely_fork(likely_thunk thunk, void *args, size_t size)
{
    static mutex forkLock;
    lock_guard<mutex> lockFork(forkLock);

    currentThunk = thunk;
    thunkArgs = args;
    thunkSize = size;

    {
        unique_lock<mutex> lock(work);
        for (size_t i = 1; i < numWorkers; i++) {
            assert(!workers[i]);
            workers[i] = true;
        }
    }

    worker.notify_all();
    executeWorker(0);

    for (size_t i = 1; i < numWorkers; i++)
        while (workers[i]) {} // Wait for the worker to finish
}

bool likely_initialize_multicore()
{
    numWorkers = max((int)thread::hardware_concurrency(), 1);
    if (numWorkers == 1)
        return false;

    // Spin up the worker threads
    workers = new atomic<bool>[numWorkers];
    for (size_t i=1; i<numWorkers; i++) {
        workers[i] = true;
        thread(workerThread, i).detach();
        while (workers[i]) {} // Wait for the worker to initialize
    }

    return true;
}
