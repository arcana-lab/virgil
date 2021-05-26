#pragma once

/**
 * ARCHITECTURE is a collection of classes used to describe any particular
 *machine. Relationships between object and certain properties of each object
 *can be queried. In summary, this is a graph with types of edges for types of
 *relationships. These classes are meant to be optmized for query efficiency,
 *not build efficiency.
 **/

#include <cstddef>
#include <cstdint>
#include <queue>
#include <unordered_set>
#include <vector>

#define MARC_DELETE_COPY(x)                 \
  /** Delete automatic copy constructor **/ \
  x(const x&) = delete;                     \
  /** Delete automatic assignment op **/    \
  x& operator=(const x&) = delete

typedef std::size_t pu_strength_t;
typedef std::size_t pu_id_t;
typedef std::size_t core_id_t;
typedef std::size_t numa_id_t;

namespace MARC {
class ThreadPool;
class NumaNode;
class PU;
class Core;
class Socket;

class NumaNode {
public:
  NumaNode();

private:
  /// Unique identifier
  numa_id_t id_;
};

class Cache {
public:
  MARC_DELETE_COPY(Cache);

  Cache();

  /// @param pu PU to associate with this cache
  void associate_pu(PU* pu);

  /// @param other The cache to associate as this cache's lower cache. Also adds
  /// this cache to other's higher caches
  void associate_lower_cache(Cache* other);

  /// @return All PUs associated with this cache
  const std::vector<PU*>& get_associated_pus() const;

  /// @return The next-lower cache
  const Cache* get_lower_cache() const;

  /// @return The next-higher caches
  const std::vector<Cache*> get_higher_caches() const;

private:
  /// All PUs associated with this Cache
  std::vector<PU*> associated_pus_fast_;

  /// All caches which draw from this one, e.g. for L2 this would be any
  /// associated L1 caches.
  std::vector<Cache*> higher_caches_;

  /// The cache that this cache uses, e.g. for L1 this would be L2.
  Cache* lower_cache_;
};

/// A PU is a logical processor. Distinct from a Core (see hyperthreading).
/// A PU has a notion of computational power, which may vary when other PUs on
/// the system are running.
class PU {
public:
  MARC_DELETE_COPY(PU);

  /// @param id ID coresponding to the physical core as in cpuset
  /// @param isolated_strength Relative power of this PU when nothing else is
  /// running
  PU(pu_id_t id, pu_strength_t isolated_strength);

  /// @return Unique identifier for this PU
  pu_id_t get_id() const;

  /// @return Power of this PU assuming it is running in isolation
  pu_strength_t get_power() const;

  /// @param pus Set of unique IDs corresponding to other PUs that are
  /// currently running
  /// @return Power of this PU given context that some other PUs are hot
  pu_strength_t get_power(const std::vector<const PU*>& other_pus) const;

  /// @return Core that this PU is part of
  const Core* get_core() const;

private:
  /// Unique identifier
  pu_id_t id_;

  /// Raw, relative power of the PU when nothing else is running
  pu_strength_t isolated_strength_;

  /// Core this pu belongs to
  Core* core_;
};

/// A representation of a core on a processor. Distinct from a PU (see
/// hyperthreading). A Core has one or more PUs, belongs to exactly one Socket,
/// and has its own cache.
class Core {
public:
  MARC_DELETE_COPY(Core);

  /// @param numa_node NUMA node this core belongs to
  /// @param pus List of PUs on this core
  /// @param l1_cache L1 cache
  /// @param l2_cache L2 cache
  /// @param l3_cache L3 cache
  Core(NumaNode* numa_node, std::vector<PU*>& pus, Cache* l1_cache,
       Cache* l2_cache, Cache* l3_cache);

  ~Core();

  /// @return set of PUs on this core
  const std::vector<PU*>& get_pus() const;

  /// @return this core's NUMA node
  const NumaNode* get_numa_node() const;

private:
  /// Unique identifier for this core
  core_id_t id_;

  /// Representation of cores on this numa node.
  NumaNode* numa_node_;

  /// PUs on this core. We are responsible for freeing them.
  std::vector<PU*> pus_;

  /// L1 cache
  Cache* l1_cache_;

  /// L2 cache
  Cache* l2_cache_;

  /// L3 cache
  Cache* l3_cache_;

  /// Socket this core belongs to
  Socket* socket_;
};

class Socket {
public:
  MARC_DELETE_COPY(Socket);

  Socket(const std::vector<Core*>& cores);

  /// @return The cores on this socket
  const std::vector<Core*>& get_cores() const;

private:
  /// Cores on this socket. We are responsible for freeing them.
  std::vector<Core*> cores_;

  /// Caches on this socket. We are responsible for freeing them.
  std::vector<Cache*> caches_;
};

class Architecture {
public:
  MARC_DELETE_COPY(Architecture);

  /// Construct a new architecture from properties
  Architecture();

  /// Destructor
  ~Architecture();

  /// @param pu_id The ID of the PU to get strength for
  /// @return The static PU strength
  pu_strength_t get_pu_strength(std::size_t pu_id) const;

  /// @return Sockets on this system
  const std::vector<Socket*>& get_sockets() const;

  /// @return Cores on this system
  const std::vector<PU*>& get_pus() const;

  /// @return The number of PUs
  std::size_t num_pus() const;

  /// @return The number of cores
  std::size_t num_cores() const;

  /// TODO: remove!!! for testing only
  pu_strength_t max_pu_strength;

private:
  void count_cores_and_pus_();

  /// Sockets on this system. We are responsible for freeing them.
  std::vector<Socket*> sockets_;

  /// Numa nodes on this system. We are responsible for freeing them.
  std::vector<NumaNode*> numa_nodes_;

  /// Static number of pus
  std::size_t num_pus_;

  /// Static number of cores
  std::size_t num_cores_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Implementation below here */
////////////////////////////////////////////////////////////////////////////////////////////////////////////

NumaNode::NumaNode() {}

void Cache::associate_lower_cache(Cache* other) {
  this->lower_cache_ = other;
  for (auto* existing_higher_cache : other->higher_caches_) {
    if (existing_higher_cache == this) {
      return;
    }
  }
  other->higher_caches_.push_back(this);
}

PU::PU(pu_id_t id, pu_strength_t isolated_strength)
    : id_(id)
    , isolated_strength_(isolated_strength) {}

pu_id_t PU::get_id() const { return id_; }

pu_strength_t PU::get_power() const { return isolated_strength_; }

Core::Core(NumaNode* numa_node, std::vector<PU*>& pus, Cache* l1_cache,
           Cache* l2_cache, Cache* l3_cache)
    : numa_node_(numa_node)
    , pus_(pus)
    , l1_cache_(l1_cache)
    , l2_cache_(l2_cache)
    , l3_cache_(l3_cache) {}

Core::~Core() {
  for (auto* pu : pus_) {
    delete pu;
  }
}

const std::vector<PU*>& Core::get_pus() const { return this->pus_; }

Socket::Socket(const std::vector<Core*>& cores)
    : cores_(cores) {}

const std::vector<Core*>& Socket::get_cores() const { return this->cores_; }

Architecture::Architecture() {

  max_pu_strength = 100000;
  PU* pu_a0 = new PU(24, 100000);
  PU* pu_a1 = new PU(26, 70000);

  NumaNode* numa_node = new NumaNode();
  std::vector<PU*> pus = {pu_a0, pu_a1};
  Core* core = new Core(numa_node, pus, nullptr, nullptr, nullptr);
  Socket* socket = new Socket({core});

  sockets_ = {socket};
  numa_nodes_ = {numa_node};

  count_cores_and_pus_(); /* Must be called after setting up the heirarchy
                              above */
}

Architecture::~Architecture() {
  for (auto* socket : sockets_) {
    delete socket;
  }
  for (auto* numa_node : numa_nodes_) {
    delete numa_node;
  }
}

void Architecture::count_cores_and_pus_() {
  num_pus_ = 0;
  num_cores_ = 0;
  for (const auto& socket : sockets_) {
    for (const auto* core : socket->get_cores()) {
      num_cores_ += 1;
      num_pus_ += core->get_pus().size();
    }
  }
}

const std::vector<PU*>& Architecture::get_pus() const {
  /// TODO: refactor caching to class level, possibly at construction
  static std::vector<PU*> all_pus = {};
  if (all_pus.empty()) {
    // Assuming that Socket -> Core -> PU is a tree relationship
    for (auto* socket : sockets_) {
      for (auto* core : socket->get_cores()) {
        for (auto* pu : core->get_pus()) {
          all_pus.push_back(pu);
        }
      }
    }
  }
  return all_pus;
}

std::size_t Architecture::num_pus() const { return num_pus_; }

std::size_t Architecture::num_cores() const { return num_cores_; }

} // namespace MARC

#undef MARC_DELETE_COPY