// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef CEPH_TIMER_H
#define CEPH_TIMER_H

#include "Cond.h"
#include "Mutex.h"

class CephContext;
class Context;
class SafeTimerThread;

/* 类SafeTimer实现了定时器的功能 */
class SafeTimer
{
  CephContext *cct;
  Mutex& lock;
  Cond cond;
  bool safe_callbacks;  //是否是safe_callback

  friend class SafeTimerThread;  //定时器执行线程
  SafeTimerThread *thread;

  void timer_thread();    //定时器任务执行
  void _shutdown();

  std::multimap<utime_t, Context*> schedule;  //目标时间和定时任务执行函数Context
  std::map<Context*, std::multimap<utime_t, Context*>::iterator> events;
  //定时任务<-->定时任务在schedule中的位置映射
  bool stopping;  //是否停止

  void dump(const char *caller = 0) const;

public:
  // This class isn't supposed to be copied
  SafeTimer(const SafeTimer&) = delete;
  SafeTimer& operator=(const SafeTimer&) = delete;

  /* Safe callbacks determines whether callbacks are called with the lock
   * held.
   *
   * safe_callbacks = true (default option) guarantees that a cancelled
   * event's callback will never be called.
   *
   * Under some circumstances, holding the lock can cause lock cycles.
   * If you are able to relax requirements on cancelled callbacks, then
   * setting safe_callbacks = false eliminates the lock cycle issue.
   * */
  SafeTimer(CephContext *cct, Mutex &l, bool safe_callbacks=true);
  virtual ~SafeTimer();

  /* Call with the event_lock UNLOCKED.
   *
   * Cancel all events and stop the timer thread.
   *
   * If there are any events that still have to run, they will need to take
   * the event_lock first. */
  void init();
  void shutdown();

  /* Schedule an event in the future
   * Call with the event_lock LOCKED */
  Context* add_event_after(double seconds, Context *callback);
  Context* add_event_at(utime_t when, Context *callback);  //添加定时器人去

  /* Cancel an event.
   * Call with the event_lock LOCKED
   *
   * Returns true if the callback was cancelled.
   * Returns false if you never addded the callback in the first place.
   */
  bool cancel_event(Context *callback);    //取消定时器任务

  /* Cancel all events.
   * Call with the event_lock LOCKED
   *
   * When this function returns, all events have been cancelled, and there are no
   * more in progress.
   */
  void cancel_all_events();

};

#endif
