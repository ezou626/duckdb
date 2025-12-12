//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/parallel/thread_context.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/main/query_profiler.hpp"

namespace duckdb {
class ClientContext;
class Logger;

//! The ThreadContext holds thread-local info for parallel usage
class ThreadContext {
public:
	explicit ThreadContext(ClientContext &context);
	~ThreadContext();

	//! The operator profiler for the individual thread context
	OperatorProfiler profiler;
	unique_ptr<Logger> logger;
	//! The NUMA node ID this thread is assigned to (0 if NUMA is not available)
	idx_t numa_node_id = 0;
};

} // namespace duckdb
