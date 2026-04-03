# UE5 Threading Guidelines

> Distilled from a talk by **Alex Stevens** (Senior Engineer, Gameloft; former Epic Games evangelist) at Unreal Fest.  
> Covers threading systems available in UE5, when to use each, primitives, thread safety, and a real-world multithreaded save system walkthrough.

---

## Why Threading?

CPUs have stopped getting faster (clock speed is plateaued around 5 GHz) and are going wider instead. Games with many independent units, background I/O, or expensive per-frame work will saturate a single core. Threading lets you offload work so the game thread and render thread stay unblocked.

**The most important rule:** threading is an *optimization*. Profile first. Solve the problem on a single thread, then parallelize only where measurements show a bottleneck.

---

## Threading Systems in UE5

UE5 provides several threading systems at different levels of abstraction. Use the highest-level system that fits your needs — avoid reinventing what the engine already provides.

### 1. Runnable Threads (`FRunnable`)

Low-level engine threads. The game thread, render thread, and RHI thread are all runnable threads.

**Use for:** long-lifetime or engine-lifetime jobs (e.g. an audio thread that polls continuously).  
**Do not use for:** short-lived tasks — thread creation costs ~500 µs at the OS level. Spawning 50 threads for a one-shot optimization task wastes several milliseconds before any work begins.

```cpp
class FMyWorker : public FRunnable
{
    virtual uint32 Run() override
    {
        // Do work here. Return 0 for success.
        return 0;
    }
};

FRunnableThread* Thread = FRunnableThread::Create(new FMyWorker(), TEXT("MyWorker"));
// Later:
Thread->Kill();   // Optional — thread stops when Run() returns
delete Thread;    // Always clean up to avoid leaks
```

---

### 2. Thread Pool (`GThreadPool` / `FQueuedThreadPool`)

Pre-allocated pool of threads that sit idle and pick up queued work. Eliminates the per-task thread creation cost.

The engine exposes a global singleton `GThreadPool` sized to the number of logical cores on the system.

**Use for:** short-lived, fire-and-forget tasks where you don't need game/render thread targeting.

```cpp
class FMyTask : public IQueuedWork
{
    virtual void DoThreadedWork() override { /* work */ }
    virtual void Abandon() override {}
};

GThreadPool->AddQueuedWork(new FMyTask());
```

You can also create your own scoped pool (`FQueuedThreadPool::Allocate()`) with a fixed concurrency limit.

---

### 3. Async / Promises / Futures (`Async.h`)

Convenience wrappers over the thread pool and task graph. Include `Async/Async.h`.

```cpp
// Run on thread pool, get a future back
TFuture<int32> Future = Async(EAsyncExecution::ThreadPool, []()
{
    return DoExpensiveWork();
});

int32 Result = Future.Get(); // Blocks until complete
```

**`ParallelFor`** — distributes loop iterations across the thread pool. Blocks the calling thread until all iterations complete.

```cpp
ParallelFor(MyArray.Num(), [&](int32 Index)
{
    ProcessItem(MyArray[Index]); // Must be thread-safe
});
```

**`AsyncTask` with thread targeting** — dispatches onto a named engine thread (game thread, render thread, etc.):

```cpp
AsyncTask(ENamedThreads::GameThread, []()
{
    // Runs on game thread — safe to touch UObjects
});
```

---

### 4. Task Graph (`FTaskGraphInterface`)

The backbone of the engine's concurrency. The game thread itself drains a lock-free task queue between ticks.

Key property: tasks form a **directed acyclic graph (DAG)** — each task can declare prerequisites, enabling fan-out/fan-in patterns without manual synchronization.

```cpp
// Create an init task (runs on any thread)
FGraphEventRef InitTask = FFunctionGraphTask::CreateAndDispatchWhenReady([]()
{
    DoInit();
}, TStatId(), nullptr, ENamedThreads::AnyThread);

// Chain a game-thread task that depends on init
FGraphEventRef GameTask = FFunctionGraphTask::CreateAndDispatchWhenReady([]()
{
    DoGameThreadWork(); // Safe: runs on game thread
}, TStatId(), InitTask, ENamedThreads::GameThread);

// Chain a render-thread task also depending on init
FGraphEventRef RenderTask = FFunctionGraphTask::CreateAndDispatchWhenReady([]()
{
    DoRenderWork();
}, TStatId(), InitTask, ENamedThreads::RenderThread);

// Final task that waits for both branches to complete
FGraphEventArray Prerequisites = { GameTask, RenderTask };
FFunctionGraphTask::CreateAndDispatchWhenReady([]()
{
    OnAllDone();
}, TStatId(), &Prerequisites, ENamedThreads::GameThread);
```

**When to use:** whenever you need explicit dependencies between tasks, or need to target named engine threads.

---

### 5. Task System (`UE::Tasks`) — UE 5.1+

A more ergonomic successor to Task Graph. Still partially in progress (as of UE 5.x) but the engine's save system already uses it. Expected to supersede Task Graph long-term.

```cpp
#include "Tasks/Task.h"

UE::Tasks::TTask<void> Task1 = UE::Tasks::Launch(UE_SOURCE_LOCATION, []()
{
    DoWork();
});

// Chain with dependency
UE::Tasks::TTask<void> Task2 = UE::Tasks::Launch(UE_SOURCE_LOCATION, []()
{
    DoMoreWork();
}, Task1); // Runs after Task1
```

**`FPipe`** — serializes tasks through a named pipe (FIFO ordering). Useful for systems like save/load where operations must not interleave:

```cpp
UE::Tasks::FPipe SavePipe{ UE_SOURCE_LOCATION };

SavePipe.Launch(UE_SOURCE_LOCATION, []() { DoSave(); });
SavePipe.Launch(UE_SOURCE_LOCATION, []() { DoAnotherSave(); }); // Always runs after first
```

**Limitation:** Task System does not natively target named engine threads (game thread, render thread). Use `AsyncTask(ENamedThreads::GameThread, ...)` or sync events (below) as a bridge.

**Insights integration:** enable the task channel in Unreal Insights to see task dependency arrows — far clearer than Task Graph's front-end.

---

## Synchronization Primitives

### Atomics (`TAtomic<T>`)

Lock-free shared state across threads. Near-zero cost on modern CPUs. Use for counters, flags, and indices shared between threads.

```cpp
TAtomic<int32> CompletedCount{ 0 };
CompletedCount.IncrementExchange(); // Thread-safe increment
```

### Mutex (`FCriticalSection`)

OS-level lock. When contended, the waiting thread yields to the OS scheduler (context switch). Use when lock hold time is long enough that spinning would waste CPU.

```cpp
FCriticalSection Mutex;
{
    FScopeLock Lock(&Mutex);
    SharedData.Add(Item);
}
```

Avoid holding mutexes across frame boundaries or during expensive operations.

### Spin Lock (`FSpinLock`)

Busy-waits instead of yielding. Lower latency than mutex for very short critical sections. Wastes CPU if the lock is contended for more than a few microseconds.

### Synchronization Events (`FEvent`)

Signal/wait primitives for coordinating across threads. The engine provides a pool — always return events to the pool after use.

```cpp
FEvent* Event = FPlatformProcess::GetSynchEventFromPool(false);

// On worker thread:
DoWork();
Event->Trigger(); // Signal completion

// On waiting thread:
bool bCompleted = Event->Wait(500); // Wait up to 500ms; returns false on timeout

FPlatformProcess::ReturnSynchEventToPool(Event);
```

Sync events can be used as task dependencies in the Task System, bridging callback-based async APIs (e.g. `UGameplayStatics::LoadStreamLevel`) into task graphs.

### Lock-Free Lists (`TLockFreePointerListUnordered`)

Linked list built on atomics. Push from any thread, pop from any thread — no locks, no race conditions.

**Pattern:** worker threads push game-thread-only work items onto the list; the game thread drains it each tick.

```cpp
TLockFreePointerListUnordered<FMyWorkItem, PLATFORM_CACHE_LINE_SIZE> WorkQueue;

// Any thread:
WorkQueue.Push(new FMyWorkItem(Data));

// Game thread each tick:
while (FMyWorkItem* Item = WorkQueue.Pop())
{
    Item->Execute(); // Safe: game thread only
    delete Item;
}
```

---

## Thread Safety in UE5

**UObjects are not thread-safe by default.** The garbage collector, property system, and most engine APIs assume game-thread access.

### Safe patterns

| Operation | Thread-safe? | Notes |
|---|---|---|
| Reading a `UObject`'s UPROPERTY while game thread is blocked | Yes | You hold exclusive access |
| `FSerializeScriptProperties` on a self-contained object | Yes | Does not traverse other UObjects |
| `GetActorTransform()` while game thread is blocked | Yes | Read-only, no side effects |
| `SetActorTransform()` | **No** | Can fire overlap events — game thread only |
| Spawning actors | **No** | Game thread only |
| `NewObject<T>()` | **No** | Game thread only |
| Blueprint VM execution | **No** | Game thread only |

### The game-thread queue pattern

When parallel tasks need to perform game-thread-only operations, collect them in a lock-free queue and drain the queue from the game thread:

```cpp
// Worker thread produces game-thread work:
GameThreadQueue.Push(MakeLambda([Actor]()
{
    Actor->SetActorTransform(NewTransform); // Safe: runs on game thread
}));

// Game thread drains the queue (called while workers are running):
while (auto* Task = GameThreadQueue.Pop())
{
    (*Task)();
    delete Task;
}
```

This gives workers the appearance of game-thread access without locking the world.

### Blueprint thread safety

Blueprints are **not thread-safe**. Do not call Blueprint functions or fire Blueprint events from worker threads. Use the game-thread queue pattern above to dispatch Blueprint-facing work back to the game thread.

---

## Worked Example: Multithreaded Save System

This demonstrates composing the systems above into a production save/load pipeline. The design reduced a 150 ms single-threaded save to ~15 ms game-thread cost (75 ms wall time across all threads) — roughly 10× improvement.

### Architecture

```
Game Thread
│
├── [Task] Decompress data          (any thread)
│         └── [Task] Parse header   (any thread, depends on decompress)
│                   └── [Event] Map load complete  (triggered by level load delegate)
│                             └── [Task] Initialize actors  (game thread — spawning)
│                                       └── [ParallelFor] Serialize actors  (N threads)
│                                                 └── game-thread queue for non-safe ops
│                                                         └── [Task] Merge data  (any thread)
│                                                                   └── [Task] Compress + write
```

### Key design decisions

**Each actor gets its own serializer buffer.** Actors serialize into isolated structs with no shared state, eliminating the need for locks during the parallel serialization step. A single-threaded merge step combines them afterward.

**The game thread is never blocked waiting.** When the game thread needs to stay available (e.g. during map loading), it drains the game-thread work queue instead of sleeping. This keeps the frame running.

**Level loading uses a sync event bridge.** `LoadStreamLevel` does not integrate with Task System, so its completion delegate triggers an `FEvent`, which is then used as a task prerequisite. This pattern applies to any callback-based async API.

**Actor spawning remains on the game thread.** There is no workaround — spawning requires game-thread context. If your game avoids dynamic spawning (objects placed at map load, not runtime-spawned), you can eliminate this bottleneck entirely.

### Concurrency limiter

When dispatching many tasks against a bounded thread pool, use a concurrency limiter to avoid flooding the pool with more tasks than there are workers:

```cpp
// Rather than queuing 10,000 tasks simultaneously,
// feed a bounded work queue that self-throttles to worker count.
FPipelineWorkQueue Queue{ GThreadPool->GetNumThreads() };
for (FActorData& Actor : AllActors)
{
    Queue.AddJob([&Actor]() { Actor.Serialize(); });
}
Queue.WaitCompletion();
```

---

## Quick Reference: Which System to Use?

| Scenario | Recommended |
|---|---|
| Engine-lifetime background thread (audio, networking) | `FRunnable` |
| One-shot background task, no result needed | `GThreadPool` / `IQueuedWork` |
| One-shot task with a result | `Async<T>()` + `TFuture` |
| Parallel loop over independent data | `ParallelFor` |
| Task with dependencies on other tasks | `UE::Tasks::Launch` (Task System) |
| Task that must target game/render thread | `FTaskGraphInterface` or `AsyncTask(ENamedThreads::...)` |
| Serialized queue of operations (save/load, I/O) | `UE::Tasks::FPipe` |
| Bridging a callback-based API into a task graph | `FEvent` as task prerequisite |
| Lock-free producer/consumer across threads | `TLockFreePointerListUnordered` |

---

## Rules of Thumb

1. **Profile before parallelizing.** Threading adds complexity. Prove you need it first.
2. **Use what the engine provides.** Look in the engine before writing synchronization primitives — they already exist.
3. **Design for no locks.** Structure data so each thread owns what it touches. Locks introduce contention and potential deadlocks.
4. **Game thread owns UObjects.** Any access to `UObject` state from a worker thread requires explicit proof of safety or the game-thread queue pattern.
5. **Always clean up.** `FRunnable` threads, `FEvent` pool objects, and task references must be explicitly released.
6. **Instrument with Insights.** `SCOPED_NAMED_EVENT` and the task channel give you per-task visibility. Enable `stat namedEvents` at runtime for full scope data.
