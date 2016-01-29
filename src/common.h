#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <utility>

#include <horde3d.h>

namespace Common
{
  struct MeshFactory
  {
    H3DRes res;
    union
    {
      struct
      {
        int verts;
        int indices;
      };
      std::pair<int, int> vni_pair;
    };
    template<class... T>
    MeshFactory(
      const char* name,
      std::pair<int, int>(*generator)(H3DRes, T...),
      T... params)
    {
      res = h3dFindResource(H3DResTypes::Geometry, name);
      vni_pair = generator(res, params...);
    }
    
    MeshFactory(){}
    MeshFactory(H3DRes n, std::pair<int, int> p): res(n), vni_pair(p){}
    MeshFactory(H3DRes n, int v, int i): res(n), verts(v), indices(i){}
    
    H3DNode gen(H3DNode mother, const char* name, H3DRes mat)
    {
      return h3dAddMeshNode(mother, name, mat, 0, indices, 0, verts - 1);
    }
    H3DNode operator()(H3DNode mother, const char* name, H3DRes mat)
    {
      return gen(mother, name, mat);
    }
    
    template<class... T>
    void build(T... params)
    {
      new(this) MeshFactory(params...);
    }
  };
}

#endif
