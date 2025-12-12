//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/numa_topology.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/common.hpp"
#include "duckdb/common/vector.hpp"

namespace duckdb {

//! NUMA topology detection and management
class NUMATopology {
public:
	//! Initialize NUMA topology (should be called once at startup)
	DUCKDB_API static void Initialize();

	//! Check if NUMA is available on this system
	DUCKDB_API static bool IsNUMAAvailable();

	//! Get the number of NUMA nodes (returns 1 if NUMA is not available)
	DUCKDB_API static idx_t GetNUMANodeCount();

	//! Get the NUMA node ID for a given CPU ID
	//! Returns 0 if NUMA is not available or CPU ID is invalid
	DUCKDB_API static idx_t GetNUMANodeForCPU(idx_t cpu_id);

	//! Get all CPU IDs for a specific NUMA node
	//! Returns empty vector if node_id is invalid or NUMA is not available
	DUCKDB_API static vector<idx_t> GetCPUsForNUMANode(idx_t node_id);

	//! Get the next CPU ID for a thread on the specified NUMA node
	//! This is used for round-robin assignment of threads to CPUs within a node
	DUCKDB_API static idx_t GetNextCPUForNUMANode(idx_t node_id, idx_t thread_index);

	//! Get the total number of CPUs in the system
	DUCKDB_API static idx_t GetCPUCount();

	//! Get an alternate NUMA node (for migration when current node has failures)
	//! Returns a different node than the current one, or the same node if only one exists
	DUCKDB_API static idx_t GetAlternateNUMANode(idx_t current_node);

private:
	static bool initialized;
	static bool numa_available;
	static idx_t numa_node_count;
	static idx_t cpu_count;
	static vector<vector<idx_t>> cpus_per_node; // CPUs grouped by NUMA node
};

} // namespace duckdb




