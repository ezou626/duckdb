#include "duckdb/common/numa_topology.hpp"

#ifdef DUCKDB_USE_NUMA
#include <numa.h>
#include <numaif.h>
#endif

#include <thread>
#include <algorithm>

namespace duckdb {

bool NUMATopology::initialized = false;
bool NUMATopology::numa_available = false;
idx_t NUMATopology::numa_node_count = 1;
idx_t NUMATopology::cpu_count = 0;
vector<vector<idx_t>> NUMATopology::cpus_per_node;

void NUMATopology::Initialize() {
	if (initialized) {
		return;
	}

#ifdef DUCKDB_USE_NUMA
	// Check if NUMA is available
	if (::numa_available() != -1) {
		NUMATopology::numa_available = true;
		int max_node = numa_max_node();
		if (max_node >= 0) {
			numa_node_count = NumericCast<idx_t>(max_node + 1);
		} else {
			numa_node_count = 1;
		}

		// Get total CPU count
		cpu_count = NumericCast<idx_t>(std::thread::hardware_concurrency());

		// Build mapping of CPUs to NUMA nodes
		cpus_per_node.clear();
		cpus_per_node.resize(numa_node_count);

		// Iterate through all CPUs and map them to NUMA nodes
		for (idx_t cpu_id = 0; cpu_id < cpu_count; cpu_id++) {
			int node_id = numa_node_of_cpu(NumericCast<int>(cpu_id));
			if (node_id >= 0 && NumericCast<idx_t>(node_id) < numa_node_count) {
				cpus_per_node[NumericCast<idx_t>(node_id)].push_back(cpu_id);
			}
		}

		// Sort CPUs within each node for consistent assignment
		for (auto &cpus : cpus_per_node) {
			std::sort(cpus.begin(), cpus.end());
		}
	} else {
		// NUMA library is not available or system doesn't support NUMA
		numa_available = false;
		numa_node_count = 1;
		cpu_count = NumericCast<idx_t>(std::thread::hardware_concurrency());
		cpus_per_node.clear();
		cpus_per_node.resize(1);
		// Add all CPUs to node 0
		for (idx_t cpu_id = 0; cpu_id < cpu_count; cpu_id++) {
			cpus_per_node[0].push_back(cpu_id);
		}
	}
#else
	// NUMA support not compiled in
	numa_available = false;
	numa_node_count = 1;
	cpu_count = NumericCast<idx_t>(std::thread::hardware_concurrency());
	cpus_per_node.clear();
	cpus_per_node.resize(1);
	// Add all CPUs to node 0
	for (idx_t cpu_id = 0; cpu_id < cpu_count; cpu_id++) {
		cpus_per_node[0].push_back(cpu_id);
	}
#endif

	initialized = true;
}

bool NUMATopology::IsNUMAAvailable() {
	if (!initialized) {
		Initialize();
	}
	return numa_available;
}

idx_t NUMATopology::GetNUMANodeCount() {
	if (!initialized) {
		Initialize();
	}
	return numa_node_count;
}

idx_t NUMATopology::GetNUMANodeForCPU(idx_t cpu_id) {
	if (!initialized) {
		Initialize();
	}

	if (!numa_available) {
		return 0;
	}

	if (cpu_id >= cpu_count) {
		return 0;
	}

#ifdef DUCKDB_USE_NUMA
	int node_id = numa_node_of_cpu(NumericCast<int>(cpu_id));
	if (node_id >= 0) {
		return NumericCast<idx_t>(node_id);
	}
#endif

	return 0;
}

vector<idx_t> NUMATopology::GetCPUsForNUMANode(idx_t node_id) {
	if (!initialized) {
		Initialize();
	}

	if (node_id >= numa_node_count) {
		return vector<idx_t>();
	}

	return cpus_per_node[node_id];
}

idx_t NUMATopology::GetNextCPUForNUMANode(idx_t node_id, idx_t thread_index) {
	if (!initialized) {
		Initialize();
	}

	if (node_id >= numa_node_count) {
		return 0;
	}

	const auto &cpus = cpus_per_node[node_id];
	if (cpus.empty()) {
		return 0;
	}

	// Round-robin assignment within the node
	return cpus[thread_index % cpus.size()];
}

idx_t NUMATopology::GetCPUCount() {
	if (!initialized) {
		Initialize();
	}
	return cpu_count;
}

} // namespace duckdb




