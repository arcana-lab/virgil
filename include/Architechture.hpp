#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

typedef std::size_t cpu_strength_t;
typedef std::size_t cpu_id_t;
typedef std::size_t core_id_t;
typedef std::size_t numa_id_t;

namespace MARC {
class ThreadPool;
class NumaNode;

class NumaNode {
private:
  /// Unique identifier
  numa_id_t id_;
};

/// A CPU is a logical processor. Distinct from a Core (see hyperthreading).
/// A CPU has a notion of computational power, which may vary when other CPUs on
/// the system are running.
class CPU {
public:
  /// @return Unique identifier for this CPU
  cpu_id_t get_id() const;

  /// @return Power of this CPU assuming it is running in isolation
  cpu_strength_t get_power() const;

  /// @param cpus Set of unique IDs corresponding to other CPUs that are
  /// currently running
  /// @return Power of this CPU given context that some other CPUs are hot
  cpu_strength_t get_power(const std::vector<const Core*>& other_cpus) const;

  /// @return Core that this CPU is part of
  const Core* get_core() const;

private:
  /// Unique identifier
  cpu_id_t id_;

  /// Core this cpu belongs to
  Core* core_;
};

/// A representation of a core on a processor. Distinct from a CPU (see
/// hyperthreading). A Core has one or more CPUs, belongs to exactly one Socket,
/// and has its own cache.
class Core {
public:
  /// @param socket Socket this core belongs to
  /// @param numa_node NUMA node this core belongs to
  /// @param cpus List of CPUs on this core
  /// @param l1i_cache_size_bytes L1i cache size in bytes
  /// @param l1d_cache_size_bytes L1d cache size in bytes
  /// @param l2_cache_size_bytes L2 cache size in bytes
  Core(Socket* socket, NumaNode* numa_node, std::vector<CPU>&& cpus,
       std::size_t l1i_cache_size_bytes, std::size_t l1d_cache_size_bytes,
       std::size_t l2_cache_size_bytes);

  /// @return size of the L1i cache in bytes
  std::size_t get_l1i_cache_size_bytes() const;

  /// @return size of the L1d cache in bytes
  std::size_t get_l1d_cache_size_bytes() const;

  /// @return size of the L2 cache in bytes
  std::size_t get_l2_cache_size_bytes() const;

  /// @return size of the L3 cache in bytes
  std::size_t get_l3_cache_size_bytes() const;

  /// @return set of CPUs on this core
  const std::vector<CPU>& get_cpus() const;

  /// @return this core's NUMA node
  const NumaNode* get_numa_node() const;

private:
  /// Unique identifier for this core
  core_id_t id_;

  /// Representation of cores on this numa node.
  NumaNode* numa_node_;

  /// CPUs on this core (i.e. hyperthreads)
  std::vector<CPU> cpus_;

  /// L1i cache size in bytes
  std::size_t l1i_cache_size_bytes_;

  /// L1d cache size in bytes
  std::size_t l1d_cache_size_bytes_;

  /// L2 cache size in bytes
  std::size_t l2_cache_size_bytes_;

  /// Socket this core belongs to
  Socket* socket_;
};

class Socket {
public:
  /// @return The cores on this socket
  const std::vector<Core>& get_cores() const;

  /// @return size of the L3 cache in bytes
  std::size_t get_l3_cache_size_bytes() const;

private:
  std::vector<Core> cores_;

  std::size_t llc_cache_size_;
};

/// TODO: add a way to differentiate related cores i.e. hyperthreads (hwloc?)
/// /project/zythos/zythos.git
class Architechture {
public:
  /// Construct a new architecture from properties
  Architechture();

  /// @param cpu_id The ID of the CPU to get strength for
  /// @return The static CPU strength
  cpu_strength_t get_cpu_strength(std::size_t cpu_id) const;

  /// @return The number of CPUs
  std::size_t num_cpus() const;

  /// @return The number of cores
  std::size_t num_cores() const;

private:
  void count_cores_and_cpus_();
  std::vector<Socket> sockets_;

  std::vector<NumaNode> numa_nodes_;

  /// Static number of cpus
  std::size_t num_cpus_;

  /// Static number of cores
  std::size_t num_cores_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Implementation below here */
////////////////////////////////////////////////////////////////////////////////////////////////////////////

Core::Core(Socket* socket, NumaNode* numa_node, std::vector<CPU>&& cpus,
           std::size_t l1i_cache_size_bytes, std::size_t l1d_cache_size_bytes,
           std::size_t l2_cache_size_bytes)
    : socket_(socket)
    , numa_node_(numa_node)
    , cpus_(cpus)
    , l1i_cache_size_bytes_(l1i_cache_size_bytes)
    , l1d_cache_size_bytes_(l1d_cache_size_bytes)
    , l2_cache_size_bytes_(l2_cache_size_bytes)

{}

std::size_t Core::get_l3_cache_size_bytes() const {
  return socket_->get_l3_cache_size_bytes();
}

std::size_t Socket::get_l3_cache_size_bytes() const { return llc_cache_size_; }

Architechture::Architechture() {
  count_cores_and_cpus_(); /* Must be called after setting up the heirarchy
                              above */
}

void Architechture::count_cores_and_cpus_() {
  num_cpus_ = 0;
  num_cores_ = 0;
  for (const auto& socket : sockets_) {
    for (const auto& core : socket.get_cores()) {
      num_cores_ += 1;
      for (const auto& cpu : core.get_cpus()) {
        num_cpus_ += 1;
      }
    }
  }
}

std::size_t Architechture::num_cpus() const { return num_cpus_; }

std::size_t Architechture::num_cores() const { return num_cores_; }

} // namespace MARC