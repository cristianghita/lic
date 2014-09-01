//
//  util.hpp
//  LineExtraction
//
//  Created by cristi on 8/5/14.
//  Copyright (c) 2014 cristi. All rights reserved.
//

#ifndef LineExtraction_util_hpp
#define LineExtraction_util_hpp

#include "Common.hpp"



static const size_t GetCPUCount(){
  size_t cpucount = 1;
#if defined(__APPLE__) || defined(__MACH__)
  const char* request = "hw.logicalcpu";
  size_t len = sizeof(cpucount);
  if ( sysctlbyname(request, &cpucount, &len, NULL, 0) < 0){
    return 2;
  }
#elif defined(__ANDROID__)
  
#endif
  return cpucount;
}

static const size_t GetCacheLineSize(){
  size_t cpucachelinesize = 32;
#if defined(__APPLE__) || defined(__MACH__)
  size_t len = sizeof(cpucachelinesize);
  const char* request = "hw.cachelinesize";
  if ( sysctlbyname(request, &cpucachelinesize, &len, NULL, 0) < 0){
    LOGERROR("Error getting cache line size...")
  }
#endif
  return cpucachelinesize;
}

/// TODO queue
template<class D, class... Args>
struct parallel_process{
  typedef function<void(Args...)>  t_workerFunction;
  typedef function<bool()>          t_stopFunction;
  explicit parallel_process(t_workerFunction wf,
                            t_stopFunction sf):
  _wf(wf),
  _sf(sf){
    threadVector = shared_ptr<vector<thread>>(new vector<thread>);
  }
  
  ~parallel_process(){
    join_all();
  }
  
  bool start(const size_t threads,
             Args... args){
    /// TODO running
    if (threads > GetCPUCount()) return false;
    this->threads = threads;
    this->activeFunction = [&](shared_ptr<vector<thread>>){
      while(!_sf()){
        _wf(args...);
      }
      stop();
      stoppedThreads.fetch_add(1,memory_order_relaxed);
    };
    
    for (int i=0;i < threads;i++){
      if (!shouldStop()){
        thread t(this->activeFunction,threadVector);
        //t.detach();
        threadVector->push_back(thread());
        threadVector->back().swap(t);
        cout << " STARTED THREAD " << endl;
      }
    }
    return true;
  }
  
  template<class T>
  std::cv_status wait(T t){
    if (threads == 0){
      return cv_status::timeout;
    }
    unique_lock<mutex> ul(waitMutex);
    cv_status ret = waitCondition.wait_for(ul, t);
    while(stoppedThreads.load(memory_order_relaxed)!=threads);
    return ret;
  }
  
  void stop(){
    stopped = true;
    //// wait until stopped
    waitCondition.notify_all();
  }
  
protected:
  void join_all(){
    for (size_t i=0; i<threadVector->size();i++){
      if (threadVector->at(i).joinable()){
        try{
          threadVector->at(i).join();
        }catch(const system_error& err){
          //cout << err.what() << endl;
        }
      }
    }
  }
  
  bool shouldStop(){
    lock_guard<mutex> lock(stoppedMutex);
    return stopped;
  }
  shared_ptr<vector<thread>>                  threadVector;
  condition_variable                          waitCondition;
  mutex                                       waitMutex;
  mutex                                       stoppedMutex;
  bool                                        stopped;
  size_t                                      threads;
  atomic<size_t>                              stoppedThreads;
  function<void(shared_ptr<vector<thread>>)>  activeFunction;
  t_workerFunction                            _wf;
  t_stopFunction                              _sf;
};

template<class D>
struct matrix_parallel_process : public parallel_process<matrix_parallel_process<D>>{
  
  struct RowState : public D::RowState{
    size_t column;
    size_t row;
  };
  
  virtual void Row(){
    _rowmutex.lock();
    size_t row = _processedRow;
    _processedRow++;
    _rowmutex.unlock();
    if (row >= _rows) return;
    RowState state;
    state.row = row;
    for (state.column = 0; state.column < _columns; state.column++) {
      Kernel(&state);
    }
  }
  
  virtual void Kernel(RowState*) = 0;
  
  bool run(const size_t threads){
    if (threads <= 1) {
      while(!done()){
        Row();
      }
      return true;
    }
    this->start(threads);
    return true;
  }
  
  bool done(){
    lock_guard<mutex> lock(_rowmutex);
    return _processedRow > _rows;
  }
  
  explicit matrix_parallel_process(const size_t rows,
                                   const size_t columns):
  parallel_process<matrix_parallel_process<D>>(bind(&matrix_parallel_process<D>::Row,ref(*this)),
                                               bind(&matrix_parallel_process<D>::done,ref(*this))),
  _rows(rows),
  _columns(columns){
    _processedRow = 0;
  }
private:
  mutex             _rowmutex;
  const size_t      _rows;
  int               _processedRow;
  const size_t      _columns;
};


#endif
