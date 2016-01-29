#ifndef WORLD_GEO_H_INCLUDED
#define WORLD_GEO_H_INCLUDED

#include <stdint.h>
#include "terrain.h"

struct PathNode
{
  float x, y;
  PathNode* next;

  PathNode(float _x, float _y, PathNode* _next): x(_x), y(_y), next(_next){}
  ~PathNode(){delete next;}

  PathNode* advance();

  void* operator new(size_t);
  void operator delete(void*);
};

namespace WorldGeo
{
  //these depend on terrain module
  constexpr size_t NMC_Plate_bitset_size = Terrain::blocked_map_max_size;
  using BlockedMapT = Terrain::BlockedMapT;

  PathNode* findPath(float, float, float, float);
  
  bool isBlocked(float, float);
  void pushIn(float&, float&, float, float);

  void setupNavMesh(int, int, BlockedMapT*);
  void deleteNavMesh();

  void displayNavMesh();
  void removeNavMesh();
}

#endif // WORLD_GEO_H_INCLUDED
