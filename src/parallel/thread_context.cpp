#include "duckdb/parallel/thread_context.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/logging/logger.hpp"
#include "duckdb/main/database.hpp"
#include "duckdb/logging/log_manager.hpp"
#include "duckdb/common/numa_topology.hpp"
#include "duckdb/common/allocator.hpp"
#include "duckdb/parallel/task_scheduler.hpp"

namespace duckdb {

ThreadContext::ThreadContext(ClientContext &context) : profiler(context) {
	LoggingContext log_context(LogContextScope::THREAD);

	log_context.connection_id = context.GetConnectionId();
	if (context.transaction.HasActiveTransaction()) {
		log_context.transaction_id = context.transaction.ActiveTransaction().global_transaction_id;
		auto query_id = context.transaction.GetActiveQuery();
		if (query_id == DConstants::INVALID_INDEX) {
			log_context.query_id = optional_idx();
		} else {
			log_context.query_id = query_id;
		}
	}

	log_context.thread_id = TaskScheduler::GetEstimatedCPUId();
	logger = LogManager::Get(context).CreateLogger(log_context, true);

	// Initialize NUMA topology and determine which NUMA node this thread is on
	NUMATopology::Initialize();
	auto &config = DBConfig::GetConfig(context);
	if (config.options.enable_numa && NUMATopology::IsNUMAAvailable()) {
		idx_t cpu_id = TaskScheduler::GetEstimatedCPUId();
		numa_node_id = NUMATopology::GetNUMANodeForCPU(cpu_id);
		
		// Set the NUMA node for this thread in the allocator
		Allocator::SetThreadNUMANode(numa_node_id);
		
		// Set the failure threshold from config
		Allocator::SetNUMAFailureThreshold(config.options.numa_failure_threshold);
	} else {
		// NUMA is disabled or not available, use node 0
		numa_node_id = 0;
		Allocator::SetThreadNUMANode(0);
	}
}

ThreadContext::~ThreadContext() {
}

} // namespace duckdb
