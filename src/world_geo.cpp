
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <cstring>
#include <ctime>
#include <csignal>

#include <queue>
#include <bitset>
#include <utility>
#include <vector>
#include <algorithm>
#include <initializer_list>
#include <list>
#include <complex>
#include <memory>

#include "utils.h"

#include "resources.h"
#include "terrain.h"

#include "world_geo.h"


/**
    Navmesh construction and organisation utilities for terrain module
*/


///this stuff requires terrain module
using namespace WorldGeo;

using NavMeshVert = std::pair<float, float>;

namespace
{
  //NavMeshVert operators and mathematical functions
  inline NavMeshVert operator+(NavMeshVert a, NavMeshVert b)
  {
    return NavMeshVert(a.first + b.first, a.second + b.second);
  }
  inline NavMeshVert operator-(NavMeshVert a, NavMeshVert b)
  {
    return NavMeshVert(a.first - b.first, a.second - b.second);
  }
  inline NavMeshVert operator*(NavMeshVert a, int b)
  {
    return NavMeshVert(a.first * b, a.second * b);
  }
  inline NavMeshVert operator/(NavMeshVert a, int b)
  {
    return NavMeshVert(a.first / b, a.second / b);
  }
  inline NavMeshVert operator*(NavMeshVert a, double b)
  {
    return NavMeshVert(a.first * b, a.second * b);
  }
  inline NavMeshVert operator/(NavMeshVert a, double b)
  {
    return NavMeshVert(a.first / b, a.second / b);
  }
  inline NavMeshVert _turn90d(NavMeshVert v)
  {
    return NavMeshVert(v.second, -v.first);
  }
  inline double _dotProduct(NavMeshVert a, NavMeshVert b)
  {
    return a.first * b.first + a.second * b.second;
  }
  inline double _distance(NavMeshVert a, NavMeshVert b)
  {
    NavMeshVert helper = a - b;
    return std::sqrt(helper.first * helper.first
                    + helper.second * helper.second);
  }
  /*inline double _vertLength(NavMeshVert v)
  {
    return std::sqrt(v.first * v.first + v.second * v.second);
  }*/

  //triangle class
  struct __NavMeshTriangle
  {
    /*
      cons stores the connected triangle indices in the following order:
          cons[0]: connected with indices[0, 1]
          cons[1]: connected with indices[1, 2]
          cons[2]: connected with indices[2, 0]
    */
    int indices[3];
    int cons[3];
    short dists[3];
    float center_x, center_y;

    __NavMeshTriangle()
    {
      for(auto& i: indices)   i = 0;
      for(auto& c: cons)      c = -1;
    }
    __NavMeshTriangle(int a, int b, int c)
    {
      indices[0] = a;
      indices[1] = b;
      indices[2] = c;
      for(auto& c: cons) c = -1;
    }

    void addToIndices(int n)
    {
      indices[0] += n;
      indices[1] += n;
      indices[2] += n;
    }
    void addToCons(int n)
    {
      for(auto& i: cons)
      {
        if(i != -1) i += n;
      }
    }

    /*void setConnection(int n)
    {
      for(auto& i: cons)
      {
        assert(i != n);
        if(i == -1)
        {
          i = n;
          return;
        }
      }
      assert(false);
    }*/

    int connections()
    {
      int x = 0;
      for(auto i: cons)
      {
        if(i != -1)
          ++x;
      }
      return x;
    }

    bool hasVert(int v) const
    {
      for(int vert: indices)
      {
        if(v == vert)
          return true;
      }
      return false;
    }
    
    bool hasVerts(int v1, int v2) const
    {
      int falses = 0;
      for(int vert: indices)
      {
        if(vert != v1 && vert != v2)
        {
          if(falses == 0) falses = 1;
          else return false;
        }
      }
      return true;
    }

    int getCon(int c) const
    {
      if(c == cons[0]) return 0;
      if(c == cons[1]) return 1;
      if(c == cons[2]) return 2;
      return -1;
    }

    friend bool hasMutualSide(const __NavMeshTriangle& a,
                              const __NavMeshTriangle& b)
    {
      int sides = 0;
      for(auto i: a.indices)
      {
        for(auto j: b.indices)
        {
          if(i == j)
          {
            ++sides;
            break;
          }
        }
      }
      assert(sides <= 2);
      return sides == 2;
    }

    friend bool tryToConnect(__NavMeshTriangle& a, __NavMeshTriangle& b,
                            int a_idx, int b_idx)
    {
      //assert(hasMutualSide(a, b));
      if(!hasMutualSide(a, b))
        return false;
      if(a.hasVert(b.indices[0]))
      {
        if(a.hasVert(b.indices[1]))
          b.cons[0] = a_idx;
        else b.cons[2] = a_idx;
      }
      else b.cons[1] = a_idx;
      if(b.hasVert(a.indices[0]))
      {
        if(b.hasVert(a.indices[1]))
          a.cons[0] = b_idx;
        else a.cons[2] = b_idx;
      }
      else a.cons[1] = b_idx;
      return true;
    }
  };

  //struct __NavMesh

  std::vector<NavMeshVert>          _navmesh_verts;
  std::vector<std::pair<int, int>>  _navmesh_vert_cons;
  std::vector<__NavMeshTriangle>    _navmesh_triangles;
  //std::vector<int>            _navmesh_indices;
  BlockedMapT*                      _blocked_map            = nullptr;
  
  //plate stuff
  int*                              _plate_ofsets           = nullptr;
  int*                              _plate_vert_ofsets      = nullptr;
  int                               _num_plates;
  
  inline int _getPlate(int tri)
  {
    int approx = (tri * _num_plates) / _navmesh_triangles.size();
    
    if(tri < _plate_ofsets[approx])
      do --approx; while(tri < _plate_ofsets[approx]);
    else if(tri >= _plate_ofsets[approx + 1])
      do; while(tri >= _plate_ofsets[++approx + 1]);
  
    return approx;
  }

  //__NavMeshTriangle* _navmesh_triangles = nullptr;

  ///NavMeshTriangle QuadTree

  inline bool _isInside(NavMeshVert p1, NavMeshVert p2,
                        NavMeshVert p3, NavMeshVert point)
  {
    double d = _dotProduct(_turn90d(p2 - p1), point - p1);
    if(d * _dotProduct(_turn90d(p3 - p2), point - p2) < 0.) return false;
    if(d * _dotProduct(_turn90d(p1 - p3), point - p3) < 0.) return false;
    return true;
  }

  bool _contains(int idx, float x, float y)
  {
    auto tri = &_navmesh_triangles[idx];
    NavMeshVert p1 = _navmesh_verts[tri->indices[0]];
    NavMeshVert p2 = _navmesh_verts[tri->indices[1]];
    NavMeshVert p3 = _navmesh_verts[tri->indices[2]];
    return _isInside(p1, p2, p3, NavMeshVert(x, y));
  }

  bool _intersects(int idx, float left, float right, float top, float bottom)
  {
    auto tri = &_navmesh_triangles[idx];

    NavMeshVert p1 = _navmesh_verts[tri->indices[0]];
    NavMeshVert p2 = _navmesh_verts[tri->indices[1]];
    NavMeshVert p3 = _navmesh_verts[tri->indices[2]];

    double min_x, max_x, min_y, max_y;

    min_x = p1.first < p2.first? p1.first : p2.first;
    min_x = min_x < p3.first? min_x : p3.first;
    max_x = p1.first > p2.first? p1.first : p2.first;
    max_x = max_x > p3.first? max_x : p3.first;
    min_y = p1.second < p2.second? p1.second : p2.second;
    min_y = min_y < p3.second? min_y : p3.second;
    max_y = p1.second > p2.second? p1.second : p2.second;
    max_y = max_y > p3.second? max_y : p3.second;

    if(min_x >= right) return false;
    if(max_x <= left) return false;
    if(min_y >= top) return false;
    if(max_y <= bottom) return false;

    double d;
    d = _dotProduct(_turn90d(p2 - p1), p3 - p1);
    do
    {
      if(d * _dotProduct(_turn90d(p2 - p1),
        NavMeshVert(left, top) - p1) >= 0.) break;
      if(d * _dotProduct(_turn90d(p2 - p1),
        NavMeshVert(right, top) - p1) >= 0.) break;
      if(d * _dotProduct(_turn90d(p2 - p1),
        NavMeshVert(left, bottom) - p1) >= 0.) break;
      if(d * _dotProduct(_turn90d(p2 - p1),
        NavMeshVert(right, bottom) - p1) >= 0.) break;
      return false;
    }while(false);
    d = _dotProduct(_turn90d(p3 - p2), p1 - p2);
    do
    {
      if(d * _dotProduct(_turn90d(p3 - p2),
        NavMeshVert(left, top) - p2) > 0.) break;
      if(d * _dotProduct(_turn90d(p3 - p2),
        NavMeshVert(right, top) - p2) > 0.) break;
      if(d * _dotProduct(_turn90d(p3 - p2),
        NavMeshVert(left, bottom) - p2) > 0.) break;
      if(d * _dotProduct(_turn90d(p3 - p2),
        NavMeshVert(right, bottom) - p2) > 0.) break;
      return false;
    }while(false);
    d = _dotProduct(_turn90d(p1 - p3), p2 - p3);
    do
    {
      if(d * _dotProduct(_turn90d(p1 - p3),
        NavMeshVert(left, top) - p3) > 0.) break;
      if(d * _dotProduct(_turn90d(p1 - p3),
        NavMeshVert(right, top) - p3) > 0.) break;
      if(d * _dotProduct(_turn90d(p1 - p3),
        NavMeshVert(left, bottom) - p3) > 0.) break;
      if(d * _dotProduct(_turn90d(p1 - p3),
        NavMeshVert(right, bottom) - p3) > 0.) break;
      return false;
    }while(false);

    return true;
  }

  Utils::ArenaAllocator
  <Utils::QuadTreeDummy<int, 8>::node_size, 64> _nmtqt_allocator;

  //global functions for allocating shit
  void* _nmtqt_allocate(size_t s)
  {
    return _nmtqt_allocator.getAdress();
  }
  void _nmtqt_deallocate(void* p)
  {
    _nmtqt_allocator.freeAdress(p);
  }

  Utils::QuadTree<int, 8, -1, 8, _intersects, _contains,
  _nmtqt_allocate, _nmtqt_deallocate> _nav_mesh_triangle_tree;
  bool _nav_mesh_triangle_tree_inited = false;

  uint16_t _mapsize_w;
  uint16_t _mapsize_h;
  //uint8_t* _obstacle_map = nullptr;
  //uint8_t* _area_map = nullptr;

  //__Tile* _path_map = nullptr;

  H3DRes _navmesh_geo = 0;
  H3DRes _navmesh_mat = 0;
  H3DNode _navmesh_node = 0;

  Utils::ArenaAllocator<sizeof(PathNode), 512> _pathnode_allocator;
}



class NMC_Plate: public BlockedMapT
{
  typedef std::pair<float, float> __Coords;
  struct __CoordsAngles: __Coords
  {
    int first_con, second_con;

    __CoordsAngles(__Coords c): __Coords(c){}
    __CoordsAngles(const __CoordsAngles& other): __Coords(other)
    {
      first_con = other.first_con;
      second_con = other.second_con;
    }
  };

  ///structure for storing vertex connections
  struct __VertCon
  {
    __VertCon* connections[8];
    __VertCon* more;

    void connect_ow(__VertCon& other)
    {
      for(auto& c: connections)
      {
        if(c == nullptr)
        {
          c = &other;
          return;
        }
      }
      if(!more) more = new __VertCon;
      more->connect_ow(other);
    }
    void disconnect_ow(__VertCon& other)
    {
      for(auto& c: connections)
      {
        if(c == &other)
        {
          c = nullptr;
          return;
        }
      }
      more->disconnect_ow(other);
    }

    void connect(__VertCon& other)
    {
      other.connect_ow(*this);
      this->connect_ow(other);
    }

    void disconnect(__VertCon& other)
    {
      other.disconnect_ow(*this);
      this->disconnect_ow(other);
    }

    void disconnectAll(__VertCon* real_this)
    {
      for(auto& c: connections)
      {
        if(c == nullptr) continue;
        c->disconnect_ow(*real_this);

        c =  nullptr;
      }
      if(more) more->disconnectAll(real_this);
    }

    void disconnectAll()
    {disconnectAll(this);}

    void listConnections(std::vector<intptr_t>& list)
    {
      for(auto c: connections)
      {
        if(c) list.push_back((intptr_t)c);
      }
      if(more) more->listConnections(list);
    }

    __VertCon()
    {
      for(auto& c: connections)
        c = nullptr;
      more = nullptr;
    }
    ~__VertCon()
    {
      for(auto& c: connections)
      {
        if(c) c->disconnect_ow(*this);
      }
      delete more;
    }

    friend bool isConnected(const __VertCon& a, const __VertCon& b)
    {
      const __VertCon* ptr = &a;
      do
      {
        for(auto c: ptr->connections)
        {
          if(c == &b)
            return true;
        }
        ptr = ptr->more;
      }while(ptr);
      return false;
    }
    
    friend __VertCon* getCommonConnection
        (const __VertCon& a, const __VertCon& b, const __VertCon& c)
    {
      const __VertCon* a_ptr = &a;
      do
      {
        for(auto con: a_ptr->connections)
        {
          if(con != nullptr)
          {
            if(con == &c)
              continue;
            else if(isConnected(*con, b)) return con;
          }
        }
        a_ptr = a_ptr->more;
      }while(a_ptr);
      return nullptr;
    }
  };

  std::pair<uint16_t, uint16_t> _main_plate;
  std::vector<std::pair<uint16_t, uint16_t>> _holes;
  mutable std::vector<__CoordsAngles> _vertices;
  mutable std::vector<__NavMeshTriangle> _triangles;
  unsigned _width;

  public:

  NMC_Plate(unsigned w): _width(w)
  {this->reset();}

  void setStart(uint16_t x, uint16_t y)
  {_main_plate = {x, y};}
  void setHole(uint16_t x, uint16_t y)
  {_holes.push_back({x, y});}

  void listVertices(int x, int y, bool is_hole)
  {
    //list vertices
    std::vector<__Coords> helper;

    int seeker_x = x;
    int seeker_y = y;

    constexpr int ce_north  = 0;
    constexpr int ce_east   = 1;
    constexpr int ce_south  = 2;
    constexpr int ce_west   = 3;

    int current_n = 0;

    int first_vert = _vertices.size();

    uint16_t direction = ce_east;
    uint16_t last_dir = ce_east;

    NavMeshVert break_vert, comp_vert, current_vert;

    //setup init state
    break_vert = {seeker_x, seeker_y};
    comp_vert = {0., 0.};
    current_vert = {1., 0.};
    _vertices.push_back(break_vert);

    ++seeker_x;
    if(!(*this)[seeker_x + seeker_y * _width])
      direction = ce_south;
    else if((*this)[seeker_x + (seeker_y - 1) * _width])
      direction = ce_north;
    last_dir = direction;

    do
    {
      ++current_n;

      //update seeker
      switch(last_dir)
      {
      case ce_north:
        --seeker_y;
        if(!(*this)[seeker_x + (seeker_y - 1) * _width])
          direction = ce_east;
        else if((*this)[seeker_x - 1 + (seeker_y - 1) * _width])
          direction = ce_west;
        break;
      case ce_east:
        ++seeker_x;
        if(!(*this)[seeker_x + seeker_y * _width])
          direction = ce_south;
        else if((*this)[seeker_x + (seeker_y - 1) * _width])
          direction = ce_north;
        break;
      case ce_south:
        ++seeker_y;
        if(!(*this)[seeker_x - 1 + seeker_y * _width])
          direction = ce_west;
        else if((*this)[seeker_x + seeker_y * _width])
          direction = ce_east;
        break;
      case ce_west:
        --seeker_x;
        if(!(*this)[seeker_x - 1 + (seeker_y - 1) * _width])
          direction = ce_north;
        else if((*this)[seeker_x - 1 + seeker_y * _width])
          direction = ce_south;
        break;
      }

      comp_vert = (_turn90d(current_vert) + comp_vert * (current_n - 1)) 
                  / current_n;
      current_vert = {seeker_x - break_vert.first,
                      seeker_y - break_vert.second};

      if(current_n > 3 && std::fabs(_dotProduct(comp_vert, current_vert)) > 4.)
      {
        break_vert = {seeker_x, seeker_y};
        switch(last_dir)
        {
        case ce_north:
          break_vert.second += 1.;
          break;
        case ce_east:
          break_vert.first -= 1.;
          break;
        case ce_south:
          break_vert.second -= 1.;
          break;
        case ce_west:
          break_vert.first += 1.;
          break;
        }

        _vertices.push_back(break_vert);
        current_vert = {seeker_x - break_vert.first,
                        seeker_y - break_vert.second};
        comp_vert = {0., 0.};
        current_n = 0;
      }

      last_dir = direction;

    }while(seeker_x != x || seeker_y != y);


    int last_vert = _vertices.size() - 1;

    for(int n = first_vert; n < last_vert; ++n)
    {
      _vertices[n].second_con = n + 1;
      _vertices[n + 1].first_con = n;
    }
    
    _vertices[last_vert].second_con = first_vert;
    _vertices[first_vert].first_con = last_vert;
  }

  void listVertices()
  {
    listVertices(_main_plate.first, _main_plate.second, false);
    for(auto coord: _holes)
    {
      listVertices(coord.first, coord.second, true);
    }
  }
  
  //helper function for constraints
  void retriangulate_h(
      std::vector<int>::iterator b,
      std::vector<int>::iterator e,
      __VertCon* con_array)
  {
    if(e - b <= 2) return; 
    
    //printf("_vertices.size: %i, *b: %i, *e: %i\n",
    //        _vertices.size(), *b, *e);
    
    --e;
    
    std::complex<double> main_line, sec_line;
    main_line = {_vertices[*e].first - _vertices[*b].first,
                _vertices[*e].second - _vertices[*b].second};
    std::vector<int>::iterator t = b + 1;
    std::vector<int>::iterator l = t;
    sec_line = {_vertices[*t].first - _vertices[*b].first,
                _vertices[*t].second - _vertices[*b].second};
    double angle1 = std::arg(sec_line / main_line);
    double angle2;
    if(angle1 > 0.)
    {
      for(++t; t != e; ++t)
      {
        sec_line = {_vertices[*t].first - _vertices[*b].first,
                    _vertices[*t].second - _vertices[*b].second};
        angle2 = std::arg(sec_line / main_line);
        if(angle2 < angle1)
        {
          if(t - l != 1)
          {
            con_array[*l].connect(con_array[*t]);
            retriangulate_h(l, e, con_array);
          }
          con_array[*b].connect(con_array[*t]);
          l = t;
        }
      }
      if(e - l != 1)
      {
        con_array[*l].connect(con_array[*e]);
        retriangulate_h(l, e, con_array);
      }
    }
    else
    {
      for(++t; t != e; ++t)
      {
        sec_line = {_vertices[*t].first - _vertices[*b].first,
                    _vertices[*t].second - _vertices[*b].second};
        angle2 = std::arg(sec_line / main_line);
        if(angle2 > angle1)
        {
          if(t - l != 1)
          {
            con_array[*l].connect(con_array[*t]);
            retriangulate_h(l, e, con_array);
          }
          con_array[*b].connect(con_array[*t]);
          l = t;
        }
      }
      if(e - l != 1)
      {
        con_array[*l].connect(con_array[*e]);
        retriangulate_h(l, e, con_array);
      }
    }
  }

  //method for generating delaunay triangulation
  void triangulate()
  {
    /*
      _vertices: vertices sorted by x-coordinate
      sub_seq: array of sub-sequence indices
      num_sub_seq: number of sub-sequences
      index_list: list of triangle indices,
      initialized with num_sub_seq place-holders
      place_holder_list: array of place-holder iterators

      to fix:
        nothing, but if something fails check
        the initial triangulation code.
    */

    /*
      possible optimization:
        candidate lists might not always have to be recalculated
    */

    /*
      this lambda calculates if a point lies outside the 
      circumcircle of a triangle
      arguments:
        a, b, c: points of the triangle, in counterclockwise order
        d: point to check
      note: returns true when all points lie on the same line
    */

    auto isDelaunay = [](__Coords a, __Coords b, __Coords c, __Coords d)
                        ->bool
    {
      double aa, ba, ca, ab, bb, cb, ac, bc, cc;
      aa = a.first - d.first;
      ba = a.second - d.second;
      ca = a.first * a.first + a.second * a.second
          - d.first * d.first - d.second * d.second;
      ab = b.first - d.first;
      bb = b.second - d.second;
      cb = b.first * b.first + b.second * b.second
          - d.first * d.first - d.second * d.second;
      ac = c.first - d.first;
      bc = c.second - d.second;
      cc = c.first * c.first + c.second * c.second
          - d.first * d.first - d.second * d.second;

      return (aa*bb*cc + ba*cb*ac + ca*ab*bc
              - ca*bb*ac - ba*ab*cc - aa*cb*bc) <= float(0);
    };

    ///nother lambda for calculating angle coefficients
    auto getAngleCoef = [](__Coords left, __Coords right)->float
    {return (float)(right.second - left.second)
      / (float)(right.first - left.first);};

    constexpr float theta = .000001;

    if(_vertices.size() < 3) return;

    std::vector<intptr_t> current_v_cons;
    using Candidate = std::pair<double, int>;
    std::vector<Candidate> current_v_cands;

    //this might be redundant
    current_v_cands.reserve(12);
    current_v_cons.reserve(12);
    
    //horrible sort
    
    {
      int* idx_list = new int[_vertices.size()];
      
      for(unsigned n = 0; n < _vertices.size(); ++n)
        idx_list[n] = n;
      
      std::sort(idx_list, idx_list + _vertices.size(),
      [this](int i1, int i2)->bool
      {
        if(this->_vertices[i1].first < this->_vertices[i2].first)
          return true;
        else if(this->_vertices[i1].first > this->_vertices[i2].first)
          return false;
        else return
          (this->_vertices[i1].second < this->_vertices[i2].second);
      });
      
      decltype(_vertices) new_vertices;
      new_vertices.reserve(_vertices.size());
      
      int* idx_list_rev = new int[_vertices.size()];
      for(unsigned n = 0; n < _vertices.size(); ++n)
        idx_list_rev[idx_list[n]] = n;
      
      for(unsigned n = 0; n < _vertices.size(); ++n)
      {
        new_vertices.push_back(_vertices[idx_list[n]]);
        
        new_vertices.back().first_con =
        idx_list_rev[new_vertices.back().first_con];
        new_vertices.back().second_con =
        idx_list_rev[new_vertices.back().second_con];
      }
      
      _vertices = std::move(new_vertices);
      
      delete[] idx_list;
      delete[] idx_list_rev;
    }

    __VertCon* vert_cons = new __VertCon[_vertices.size()];

    ///initial triangulation
    //needlessly difficult
    //printf("Initial triangulation.\n");
    unsigned num_sub_seq = 0;
    int* sub_seq = new int[_vertices.size() / 2 + 1];
    {
      int first = -1;
      unsigned current;

      for(current = 0; current < _vertices.size(); ++current)
      {
        if(first == -1)
        {
          sub_seq[num_sub_seq] = current;
          ++num_sub_seq;
          first = current;
        }
        else if(current - first == 3)
        {
          if(_vertices[current - 1].first == _vertices[current].first)
          {
            vert_cons[first].connect(vert_cons[first + 1]);
            current -= 2;
          }
          else
          {
            if(getAngleCoef(_vertices[first], _vertices[first + 1])
            == getAngleCoef(_vertices[first], _vertices[first + 2]))
            {
              vert_cons[first].connect(vert_cons[first + 1]);
              vert_cons[first + 1].connect(vert_cons[first + 2]);
            }
            else
            {
              vert_cons[first].connect(vert_cons[first + 1]);
              vert_cons[first + 1].connect(vert_cons[first + 2]);
              vert_cons[first].connect(vert_cons[first + 2]);
            }

            --current;
          }
          first = - 1;
        }
        else if(current - first == 2)
        {
          if(_vertices[first + 1].first == _vertices[current].first)
          {
            do
            {
              ++current;
            }while(current < _vertices.size()
                  && _vertices[first + 1].first
                  == _vertices[current].first);
            --current;
            for(unsigned m = first + 1; m < current; ++m)
            {
              vert_cons[first].connect(vert_cons[m]);
              vert_cons[m].connect(vert_cons[m + 1]);
            }
            vert_cons[first].connect(vert_cons[current]);
            first = -1;
          }
        }
        else if(_vertices[first].first == _vertices[current].first)
        {
          do
          {
            ++current;
          }while(current < _vertices.size()
                && _vertices[first].first
                == _vertices[current].first);

          if(current == _vertices.size())
          {
            --current;
            for(unsigned m = first; m < current; ++m)
              vert_cons[m].connect(vert_cons[m + 1]);
          }
          else
          {
            for(unsigned m = first; m < current - 1; ++m)
            {
              vert_cons[m].connect(vert_cons[m + 1]);
              vert_cons[m].connect(vert_cons[current]);
            }
            vert_cons[current - 1].connect(vert_cons[current]);
          }
          first = -1;
        }
      }

      switch(current - first)
      {
      case 3:
        if(getAngleCoef(_vertices[first], _vertices[first + 1]) ==
          getAngleCoef(_vertices[first], _vertices[first + 2]))
        {
          vert_cons[first].connect(vert_cons[first + 1]);
          vert_cons[first + 1].connect(vert_cons[first + 2]);
        }
        else
        {
          vert_cons[first].connect(vert_cons[first + 1]);
          vert_cons[first + 1].connect(vert_cons[first + 2]);
          vert_cons[first].connect(vert_cons[first + 2]);
        }

        break;
      case 2:
        vert_cons[first].connect(vert_cons[first + 1]);

        break;
      default: break;
      }

      sub_seq[num_sub_seq] = _vertices.size();
    }

    ///main triangulation loop
    int left, middle, right;
    int low_l, low_r;
    int l_cand, r_cand;
    //std::vector<decltype(index_list.begin())> connected_triangles;
    //__Coords temp;

    for(unsigned n = 2; (n >> 1) < num_sub_seq; n <<= 1)
    {
      for(unsigned m = 0; m < num_sub_seq; m += n)
      {
        if(m + n / 2 >= num_sub_seq) break;
        left = sub_seq[m];
        middle = sub_seq[m + n / 2];
        right = m + n >= num_sub_seq? 
          sub_seq[num_sub_seq] : sub_seq[m + n];

        ///find lowest
        low_l = left; low_r = middle;
        for(int v = left + 1; v < middle; ++v)
        {
          if(_vertices[low_l].second >= _vertices[v].second)
            low_l = v;
        }
        for(int v = middle + 1; v < right; ++v)
        {
          if(_vertices[low_r].second > _vertices[v].second)
            low_r = v;
        }

        {
          float temp = getAngleCoef(_vertices[low_l], _vertices[low_r]);
          float temp2;

          //positive angle
          if(temp > 0.)
          {
            int old_low_l;
            do
            {
              old_low_l = low_l;
              for(int v = low_r + 1; v < right; ++v)
              {
                temp2 = getAngleCoef(_vertices[low_l],
                                    _vertices[v]);
                if(temp2 < temp)
                {
                  low_r = v;
                  temp = temp2;
                }
              }
              for(int v = low_l + 1; v < middle; ++v)
              {
                temp2 = getAngleCoef(_vertices[v],
                      _vertices[low_r]);
                if(temp2 >= temp)
                {
                  low_l = v;
                  temp = temp2;
                }
              }
            }while(old_low_l != low_l);
          }
          //negative angle
          else if(temp < 0.)
          {
            int old_low_r;
            do
            {
              old_low_r = low_r;
              for(int v = low_l - 1; v >= left; --v)
              {
                temp2 = getAngleCoef(_vertices[v],
                                    _vertices[low_r]);
                if(temp2 > temp)
                {
                  low_l = v;
                  temp = temp2;
                }
              }
              for(int v = low_r - 1; v >= middle; --v)
              {
                temp2 = getAngleCoef(_vertices[low_l],
                                    _vertices[v]);
                if(temp2 <= temp)
                {
                  low_r = v;
                  temp = temp2;
                }
              }
            }while(old_low_r != low_r);
          }
        }

        ///sewing loop
        do
        {

          ///find candidates
          std::complex<double> cpx;

          /*
            keep in mind:
              the candidates are listed and then culled,
              removing all candidates with an angle >180deg.
              might be that that requirement only aplies
              to the first candidate in any given comparison.
          */

          //left candidates
          cpx = {_vertices[low_r].first - _vertices[low_l].first,
              _vertices[low_r].second - _vertices[low_l].second};
          current_v_cons.clear();
          current_v_cands.clear();
          vert_cons[low_l].listConnections(current_v_cons);
          for(auto i: current_v_cons)
          {
            auto j = i - (intptr_t)vert_cons;
            j /= sizeof(__VertCon);
            std::complex<double> comp(
              _vertices[j].first - _vertices[low_l].first,
              _vertices[j].second - _vertices[low_l].second);
            current_v_cands.push_back({std::arg(comp / cpx), j});
          }
          current_v_cands.erase(std::remove_if(
            current_v_cands.begin(), current_v_cands.end(),
            [middle](Candidate& cand) -> bool
            {return cand.first < .0
              || cand.first > Utils::Math::Pi - theta
              || cand.second >= middle;}),
            current_v_cands.end());
          ///this is here for testing
          #ifndef NDEBUG
          for(auto it_1 = current_v_cands.begin();
            it_1 != current_v_cands.end(); ++it_1)
          {
            auto it_2 = it_1;
            for(++it_2; it_2 != current_v_cands.end(); ++it_2)
            {
              if(it_1->first == it_2->first)
                *(int*)nullptr = 0;
            }
          }
#endif
          std::sort(current_v_cands.begin(), current_v_cands.end(),
          [](const Candidate& a, const Candidate& b)
          {
            //if(a.first == b.first)
            //  printf("GGG.\n");
            return a.first < b.first;
          });

          if(current_v_cands.size() == 0)
            l_cand = -1;
          else
          {
            auto it2 = current_v_cands.begin();
            auto it1 = it2++;
            l_cand = it1->second;
            while(it2 != current_v_cands.end())
            {
              if(isDelaunay(_vertices[low_l],
                _vertices[low_r],
                _vertices[it1->second],
                _vertices[it2->second]))
                break;
              vert_cons[low_l].disconnect(vert_cons[it1->second]);
              it1 = it2++;
              l_cand = it1->second;
            }
          }

          //right candidates
          cpx = -cpx;
          current_v_cons.clear();
          current_v_cands.clear();
          vert_cons[low_r].listConnections(current_v_cons);
          for(auto i: current_v_cons)
          {
            intptr_t j = i - (intptr_t)vert_cons;
            j /= sizeof(__VertCon);
            std::complex<double> comp(
              _vertices[j].first - _vertices[low_r].first,
              _vertices[j].second - _vertices[low_r].second);
            current_v_cands.push_back(Candidate(
              -std::arg(comp / cpx), j));
          }
          current_v_cands.erase(std::remove_if(
            current_v_cands.begin(), current_v_cands.end(),
            [middle](Candidate& cand) -> bool
            {return cand.first < 0.
              || cand.first > Utils::Math::Pi - theta
              || cand.second < middle;}),
            current_v_cands.end());
          ///this is here for testing
#ifndef NDEBUG
          for(auto it_1 = current_v_cands.begin(); it_1
            != current_v_cands.end(); ++it_1)
          {
            auto it_2 = it_1;
            for(++it_2; it_2 != current_v_cands.end(); ++it_2)
            {
              if(it_1->first == it_2->first)
                *(int*)nullptr = 0;
            }
          }
#endif
          std::sort(current_v_cands.begin(), current_v_cands.end(),
          [](const Candidate& a, const Candidate& b)
          {
            //if(a.first == b.first)
            //  printf("HHHl\n");
            return a.first < b.first;
          });

          if(current_v_cands.size() == 0)
            r_cand = -1;
          else
          {
            auto it2 = current_v_cands.begin();
            auto it1 = it2++;
            r_cand = it1->second;
            while(it2 != current_v_cands.end())
            {
              if(isDelaunay(_vertices[low_l],
                            _vertices[low_r],
                            _vertices[it1->second],
                            _vertices[it2->second]))
                break;
              vert_cons[low_r].disconnect(vert_cons[it1->second]);
              it1 = it2++;
              r_cand = it1->second;
            }
          }

          ///sew together
          vert_cons[low_l].connect(vert_cons[low_r]);
          //printf("connected %i and %i\n", low_l, low_r);
          if(l_cand != -1)
          {
            if(r_cand != -1)
            {
              if(isDelaunay(_vertices[low_l], _vertices[low_r],
                            _vertices[l_cand], _vertices[r_cand]))
                low_l = l_cand;
              else low_r = r_cand;
            }
            else low_l = l_cand;
          }
          else if(r_cand != -1)
            low_r = r_cand;
          else break;

        }while(true);//end sewing loop

      }
    }
    
    //insert constraints
    /*
      possible corner case when contraint of an inner plate crosses
      constraints of outer plate
    */
    
    for(unsigned curr = 0; curr < _vertices.size(); ++curr)
    {
      current_v_cons.clear();
      vert_cons[curr].listConnections(current_v_cons);
      
      bool constrained = false;
      int targ = _vertices[curr].first_con;
      
      for(auto i: current_v_cons)
      {
        intptr_t j = i - (intptr_t)vert_cons;
        j /= sizeof(__VertCon);
        
        if(j == targ)
        {
          constrained = true;
          break;
        }
      }
      if(!constrained)
      {
        std::vector<int> left_side, right_side;
        std::complex<double> main_comp = {
          _vertices[targ].first - _vertices[curr].first,
          _vertices[targ].second - _vertices[curr].second};
        
        //find first two cons
        int l_con, r_con;
        
        l_con = r_con = -1;
        
        double l_angle = 10.;
        double r_angle = -10.;
        for(auto i: current_v_cons)
        {
          intptr_t j = i - (intptr_t)vert_cons;
          j /= sizeof(__VertCon);
          
          double temp_d = std::arg(std::complex<double>(
            _vertices[j].first - _vertices[curr].first,
            _vertices[j].second - _vertices[curr].second)
            / main_comp);
          if(temp_d > 0.)
          {
            if(temp_d < l_angle)
            {
              l_angle = temp_d;
              l_con = j;
            }
          }
          else
          {
            if(temp_d > r_angle)
            {
              r_angle = temp_d;
              r_con = j;
            }
          }
        }
        left_side.push_back(l_con);
        right_side.push_back(r_con);
        
        /*printf("l_con = %i; r_con = %i;\n", l_con, r_con);
        printf("targ = %i\n", targ);
        
        if(isConnected(vert_cons[l_con], vert_cons[r_con]))
          printf("not connected.\n");*/
        
        //list cons
        __VertCon *next_con, *last_con;
        last_con = &vert_cons[curr];
        while((next_con = getCommonConnection(vert_cons[l_con],
              vert_cons[r_con], *last_con)) != vert_cons + targ)
        {
          intptr_t j = (intptr_t)next_con - (intptr_t)vert_cons;
          j /= sizeof(__VertCon);
          
          if(std::arg(std::complex<double>(
            _vertices[j].first - _vertices[curr].first,
            _vertices[j].second - _vertices[curr].second)
            / main_comp) > 0.)
          {
            last_con = &vert_cons[l_con];
            l_con = j;
            left_side.push_back(l_con);
          }
          else
          {
            last_con = &vert_cons[r_con];
            r_con = j;
            right_side.push_back(r_con);
          }
        }
        
        //disconnect all
        //this is for debugging but should be left
#ifndef NDEBUG
        for(auto l: left_side)
        for(auto r: right_side)
        {
          if(_vertices[l].first_con == r
          || _vertices[r].second_con == r)
          {
            printf("::\n:: Error %s:%i constraint disconnected.\n::\n",
                    __FILE__, __LINE__);
            abort();
          }
        }
        /*
          if this causes an error it means that a triangulation
          constraint has hen disconnected to make room for another one.
          IOW there are crossing contraints in the graph.
          (should preferably be fixed in listVertices function that
          calculates constraints.)
        */
#endif
                
        for(auto l: left_side)
        for(auto r: right_side)
        if(isConnected(vert_cons[l], vert_cons[r]))
            vert_cons[l].disconnect(vert_cons[r]);
        
        //push curr and targ into containers (retriangulate_h expects it)
        left_side.insert(left_side.begin(), curr);
        right_side.insert(right_side.begin(), curr);
        left_side.push_back(targ);
        right_side.push_back(targ);
        
        //new triangulation (ayy lmao)
        vert_cons[curr].connect(vert_cons[targ]);
        retriangulate_h(left_side.begin(), left_side.end(), vert_cons);
        retriangulate_h(right_side.begin(), right_side.end(), vert_cons);
      }
    }
        
    //remove invalid connections
    for(unsigned n = 0; n < _vertices.size(); ++n)
    {
      current_v_cons.clear();
      vert_cons[n].listConnections(current_v_cons);
      
      double first_angle, second_angle;
      
      //lol second_con -> first_angle, first_con -> second_angle
      //lrn2code
      int temp_con = _vertices[n].second_con;
      first_angle = std::arg(std::complex<double>{
        _vertices[temp_con].first - _vertices[n].first,
        _vertices[temp_con].second - _vertices[n].second});
      temp_con = _vertices[n].first_con;
      second_angle = std::arg(std::complex<double>{
        _vertices[temp_con].first - _vertices[n].first,
        _vertices[temp_con].second - _vertices[n].second});
      
      if(first_angle > second_angle)
      {
        for(auto i: current_v_cons)
        {
          intptr_t j = i - (intptr_t)vert_cons;
          j /= sizeof(__VertCon);
          
          double angle = std::arg(std::complex<double>{
            _vertices[j].first - _vertices[n].first,
            _vertices[j].second - _vertices[n].second});
          
          if(angle < first_angle && angle > second_angle)
            vert_cons[n].disconnect(vert_cons[j]);
        }
      }
      else
      {
        for(auto i: current_v_cons)
        {
          intptr_t j = i - (intptr_t)vert_cons;
          j /= sizeof(__VertCon);
          
          double angle = std::arg(std::complex<double>{
            _vertices[j].first - _vertices[n].first,
            _vertices[j].second - _vertices[n].second});
          
          if(angle < first_angle || angle > second_angle)
            vert_cons[n].disconnect(vert_cons[j]);
        }
      }
    }

    ///list triangles
    _triangles.clear();

    for(unsigned n = 0; n < _vertices.size(); ++n)
    {
      current_v_cons.clear();
      current_v_cands.clear();
      vert_cons[n].listConnections(current_v_cons);
      for(auto i: current_v_cons)
      {
        intptr_t j = i - (intptr_t)vert_cons; j /= sizeof(__VertCon);
        std::complex<double> comp(
          _vertices[j].first - _vertices[n].first,
          _vertices[j].second - _vertices[n].second);
        current_v_cands.push_back(Candidate(std::arg(comp), j));
      }
      std::sort(current_v_cands.begin(), current_v_cands.end(),
                [](const Candidate& a, const Candidate& b)
                {return a.first < b.first;});
      for(int m = 0; m < (int)current_v_cands.size() - 1; ++m)
      {
        if(isConnected(vert_cons[current_v_cands[m].second],
          vert_cons[current_v_cands[m + 1].second]))
        {
          auto v1 = current_v_cands[m].second;
          auto v2 = current_v_cands[m + 1].second;
          std::complex<double> cmpx1 = {
            _vertices[v1].first - _vertices[n].first,
            _vertices[v1].second - _vertices[n].second};
          std::complex<double> cmpx2 = {
            _vertices[v2].first - _vertices[n].first,
            _vertices[v2].second - _vertices[n].second};
          if(std::arg(cmpx1 / cmpx2) > 0.)
            _triangles.push_back({(int)n, v1, v2});
          else _triangles.push_back({(int)n, v2, v1});
        }
      }
      vert_cons[n].disconnectAll();
    }

    ///find triangle connections
    for(unsigned n = 0; n < _triangles.size(); ++n)
    {
      int connects = _triangles[n].connections();
      if(connects == 3) break;
      for(unsigned m = n + 1; m < _triangles.size(); ++m)
      {
        if(tryToConnect(_triangles[n], _triangles[m], n, m))
        {
          //_triangles[n].setConnection(m);
          //_triangles[m].setConnection(n);
          if(++connects == 3)
            break;
        }
      }
    }

  delete[] sub_seq;
  delete[] vert_cons;
  }

  void clear(){_vertices.clear(); _triangles.clear();}
  /*void push_back_v(__Coords c)
  {
    _vertices.push_back(__CoordsAngles(c));
  }*/

  //iterators
  typename std::vector<__CoordsAngles>::iterator vBegin()const
  {return _vertices.begin();}
  typename std::vector<__CoordsAngles>::iterator vEnd()const
  {return _vertices.end();}
  std::vector<__NavMeshTriangle>::iterator tBegin()const
  {return _triangles.begin();}
  std::vector<__NavMeshTriangle>::iterator tEnd()const
  {return _triangles.end();}
};

//end navmesh plate


///PathNode memory allocation overloads
PathNode* PathNode::advance()
{
  auto ret = next;
  next = nullptr;
  delete this;
  return ret;
}

//int pn_counter = 0;

void* PathNode::operator new(size_t size)
{
  //++pn_counter;
  return _pathnode_allocator.getAdress();
}

void PathNode::operator delete(void* adress)
{
  //--pn_counter;
  _pathnode_allocator.freeAdress(adress);
}

namespace
{
  constexpr int __pts_per_unit = 100;

  //uint8_t* _closed_map = nullptr;

  struct __Status
  {
    enum
    {
      unused  = 0,
      open    = 1,
      closed  = 2
    };

    int status;
    int travel_len;
    int prev;

    void setAll(int s, int t, int p)
    {
      status = s;
      travel_len = t;
      prev = p;
    }
  };

  __Status* _status_map = nullptr;

  struct __OpenTriangle
  {
    int triangle;
    int heuristic;
    __OpenTriangle(int t, int h): triangle(t), heuristic(h){}
    bool operator<(const __OpenTriangle& other) const
    {
      return heuristic > other.heuristic;
    }
  };

  /*bool operator<(__OpenTriangle& a, __OpenTriangle& b)
  {
    return a.heuristic < b.heuristic;
  }*/

  std::priority_queue<__OpenTriangle> _open_heap;
  
  template<class T>
  class __SquaredDistFunctor
  {
    T _x_comp, _y_comp;
    
  public:
  
    __SquaredDistFunctor(T x, T y): _x_comp(x), _y_comp(y){}
    T operator()(T x, T y)
    {
      T x_diff = _x_comp - x;
      T y_diff = _y_comp - y;
      return x_diff * x_diff + y_diff * y_diff;
    }
    
    T operator()(const std::pair<T, T>& v)
    {
      return this->operator()(v.first, v.second);
    }
    T operator()(const T* v)
    {
      return this->operator()(v[0], v[1]);
    }
  };
}

///
namespace WorldGeo
{
  /*
    the following code implements the A* algorithm
  */

  PathNode* findPath(float x_start, float y_start, float x_dest, float y_dest)
  {
    /*
      possible optimizations:
        Paths between crossroads can be precalculated.
        Finding of nearest destination triangles in case of miss can be mapped.
        triangles along plate edges can be precalculated
    */
    
    auto dest_dist = [x_dest, y_dest](float x, float y)
    {
      x -= x_dest;
      y -= y_dest;
      return sqrt(x * x + y * y) * __pts_per_unit;
    };

    int from = 0;
    int to = 0;
    int current = 0;
    int travel_len = 0;

    if((from = _nav_mesh_triangle_tree.retrieve(x_start, y_start)) == -1)
    {
#ifndef NDEBUG
      printf("findPath: start point not found in nav_mesh_triangle_tree.\n"
            "%s: %i\n", __FILE__, __LINE__);
#endif
      return nullptr;
    }
    int from_plate = _getPlate(from);
    if((to = _nav_mesh_triangle_tree.retrieve(x_dest, y_dest)) == -1 ||
        from_plate != _getPlate(to))
    {
      //helper functor
      /*auto get_squared_dist = [x_dest, y_dest](__NavMeshTriangle& tri)->float
      {
        float x_diff = tri.center_x - x_dest;
        float y_diff = tri.center_y - y_dest;
        return x_diff * x_diff + y_diff * y_diff;
      };*/
      __SquaredDistFunctor<decltype(NavMeshVert::first)>
        get_squared_dist(x_dest, y_dest);
      
      /*
        note:
        equation for finding nearest point on a line to nother point
          -l = ((x1 - x2)(x2 - x0) + (y1 - y2)(y2 - y0))/
                      ((x1 - x2)^2 + (y1 - y2)^2)
        where:
          x0 = point to compare to
          x1 = first point on line
          x2 = second point on line
          l = ratio of nearest point from x1 to x2
        
        also note: this is not tested
      */
      
      //find nearest triangle
      /*to = _plate_ofsets[from_plate];
      float squared_dist = get_squared_dist(_navmesh_triangles[to]);
      for(int curr_idx = _plate_ofsets[from_plate] + 1;
          curr_idx != _plate_ofsets[from_plate + 1];
          ++curr_idx)
      {
        float curr_sq_dist = get_squared_dist(_navmesh_triangles[curr_idx]);
        if(curr_sq_dist < squared_dist)
        {
          squared_dist = curr_sq_dist;
          to = curr_idx;
        }
      }*/
      
      //find nearest vert
      int v_temp = _plate_vert_ofsets[from_plate];
      float squared_dist = get_squared_dist(_navmesh_verts[v_temp]);
      for(int temp_idx = _plate_vert_ofsets[from_plate] + 1;
          temp_idx != _plate_vert_ofsets[from_plate + 1];
          ++temp_idx)
      {
        float temp_sq_dist = get_squared_dist(_navmesh_verts[temp_idx]);
        if(temp_sq_dist < squared_dist)
        {
          squared_dist = temp_sq_dist;
          v_temp = temp_idx;
        }
      }
      
      //find nearest triangle
      {
        int v_temp1 = _navmesh_vert_cons[v_temp].first;
        int v_temp2 = _navmesh_vert_cons[v_temp].second;
      
        std::complex<double> main_vert, first_vert, second_vert;
        main_vert = {x_dest - _navmesh_verts[v_temp].first,
                      y_dest - _navmesh_verts[v_temp].second};
        first_vert = 
          {_navmesh_verts[v_temp1].first - _navmesh_verts[v_temp].first,
          _navmesh_verts[v_temp1].second - _navmesh_verts[v_temp].second};
        second_vert = 
          {_navmesh_verts[v_temp2].first - _navmesh_verts[v_temp].first,
          _navmesh_verts[v_temp2].second - _navmesh_verts[v_temp].second};
        
        if(std::abs(std::arg(main_vert / first_vert)) <
            std::abs(std::arg(main_vert / second_vert)))
          to = std::find_if(
            _navmesh_triangles.begin() + _plate_ofsets[from_plate],
            _navmesh_triangles.begin() + _plate_ofsets[from_plate + 1],
            [=](const __NavMeshTriangle& tri)
            {return tri.hasVerts(v_temp, v_temp1);})
            - _navmesh_triangles.begin();
          else to = std::find_if(
            _navmesh_triangles.begin() + _plate_ofsets[from_plate],
            _navmesh_triangles.begin() + _plate_ofsets[from_plate + 1],
            [=](const __NavMeshTriangle& tri)
            {return tri.hasVerts(v_temp, v_temp2);})
            - _navmesh_triangles.begin();
      }
      
      float center_x = _navmesh_triangles[to].center_x;
      float center_y = _navmesh_triangles[to].center_y;
      
      do
      {
        x_dest = (x_dest * 4 + center_x) / 5;
        y_dest = (y_dest * 4 + center_y) / 5;
      }while(!_contains(to, x_dest, y_dest));
    }
    

    if(from == to)
      return new PathNode(x_dest, y_dest, nullptr);

    //memset(_status_map, __Status::unused, _navmesh_triangles.size());
    for(unsigned n = 0; n < _navmesh_triangles.size(); ++n)
      _status_map[n].status = __Status::unused;

    //list first candidates
    _status_map[from].setAll(__Status::closed, 0, -1);

    {
      int i;
      if((i = _navmesh_triangles[from].cons[0]) != -1)
      {
        travel_len = (_navmesh_triangles[from].dists[0] 
                    + _navmesh_triangles[from].dists[2]) / 2;
        _status_map[i].setAll(__Status::open, travel_len, from);
        _open_heap.push(__OpenTriangle(i,
        dest_dist(_navmesh_triangles[i].center_x,
                    _navmesh_triangles[from].center_y) + travel_len));
      }
      if((i = _navmesh_triangles[from].cons[1]) != -1)
      {
        travel_len = (_navmesh_triangles[from].dists[1]
                    + _navmesh_triangles[from].dists[0]) / 2;
        _status_map[i].setAll(__Status::open, travel_len, from);
        _open_heap.push(__OpenTriangle(i,
        dest_dist(_navmesh_triangles[i].center_x,
                    _navmesh_triangles[from].center_y) + travel_len));
      }
      if((i = _navmesh_triangles[from].cons[2]) != -1)
      {
        travel_len = (_navmesh_triangles[from].dists[2]
                    + _navmesh_triangles[from].dists[1]) / 2;
        _status_map[i].setAll(__Status::open, travel_len, from);
        _open_heap.push(__OpenTriangle(i,
        dest_dist(_navmesh_triangles[i].center_x,
                    _navmesh_triangles[from].center_y) + travel_len));
      }
    }

    while(!_open_heap.empty())
    {
      current = _open_heap.top().triangle;
      _open_heap.pop();
      if(current == to) break;

      _status_map[current].status = __Status::closed;

      //push new connections unto open_heap
      auto& tri = _navmesh_triangles[current];
      int prev_con;
      int temp_con1, temp_con2;
      if(tri.cons[0] == _status_map[current].prev) prev_con = 0;
      else if(tri.cons[1] == _status_map[current].prev) prev_con = 1;
      else prev_con = 2;

      temp_con1 = tri.cons[(prev_con + 1) % 3];
      temp_con2 = tri.cons[(prev_con + 2) % 3];

      //check for closed triangles tangents
      if(temp_con1 != -1
        && _status_map[temp_con1].status == __Status::closed)
      {
        int sec_prev_con = _status_map[temp_con1].prev;
        auto& temp_tri = _navmesh_triangles[temp_con1];
        if(sec_prev_con == temp_tri.cons[0]) sec_prev_con = 0;
        else if(sec_prev_con == temp_tri.cons[1]) sec_prev_con = 1;
        else sec_prev_con = 2;

        travel_len = _status_map[temp_con1].travel_len;
        if(current == temp_tri.cons[(sec_prev_con + 1) % 3])
          travel_len += temp_tri.dists[(sec_prev_con + 1) % 3];
        else travel_len += temp_tri.dists[(sec_prev_con + 2) % 3];

        if(travel_len < _status_map[current].travel_len)
        {
          _status_map[current].travel_len = travel_len;
          _status_map[current].prev = temp_con1;
        }

        temp_con1 = -1;
      }
      if(temp_con2 != -1
        && _status_map[temp_con2].status == __Status::closed)
      {
        int sec_prev_con = _status_map[temp_con1].prev;
        auto& temp_tri = _navmesh_triangles[temp_con2];
        if(sec_prev_con == temp_tri.cons[0]) sec_prev_con = 0;
        else if(sec_prev_con == temp_tri.cons[1]) sec_prev_con = 1;
        else sec_prev_con = 2;

        travel_len = _status_map[temp_con2].travel_len;
        if(current == temp_tri.cons[(sec_prev_con + 1) % 3])
          travel_len += temp_tri.dists[(sec_prev_con + 1) % 3];
        else travel_len += temp_tri.dists[(sec_prev_con + 2) % 3];

        if(travel_len < _status_map[current].travel_len)
        {
          _status_map[current].travel_len = travel_len;
          _status_map[current].prev = temp_con2;
        }

        temp_con2 = -1;
      }

      //push new open triangles
      if(temp_con1 != -1 
        && _status_map[temp_con1].status == __Status::unused)
      {
        travel_len = tri.dists[prev_con]
                    + _status_map[current].travel_len;
        _status_map[temp_con1].setAll(
          __Status::open, travel_len, current);
        _open_heap.push(__OpenTriangle(
          temp_con1, dest_dist(tri.center_x, tri.center_y)
          + travel_len));
      }
      if(temp_con2 != -1 
        && _status_map[temp_con2].status == __Status::unused)
      {
        travel_len = tri.dists[(prev_con + 2) % 3]
                    + _status_map[current].travel_len;
        _status_map[temp_con2].setAll(
          __Status::open, travel_len, current);
        _open_heap.push(__OpenTriangle(
          temp_con2, dest_dist(tri.center_x, tri.center_y)
          + travel_len));
      }
    }

    while(!_open_heap.empty()) _open_heap.pop();

    //list path nodes using a funnel algorithm
    PathNode* current_node = new PathNode(x_dest, y_dest, nullptr);

    static std::vector<std::pair<int, int>> funnel;
    __NavMeshTriangle* triangle;

    //insert vertex index pairs into funnel
    funnel.clear();
    do
    {
      triangle = &_navmesh_triangles[current];
      current = _status_map[current].prev;
      int con = triangle->getCon(current);
      funnel.push_back({triangle->indices[con],
        triangle->indices[(con + 1) % 3]});
    }while(current != from);

    std::complex<double> l_angle, r_angle, t_angle;
    float current_x, current_y;
    int l_break, r_break, l_vert, r_vert;

    current_x = x_dest;
    current_y = y_dest;

    l_break = r_break = current = 0;
    l_vert = funnel.at(0).first;
    r_vert = funnel.at(0).second;

    l_angle = {_navmesh_verts[l_vert].first - current_x,
                _navmesh_verts[l_vert].second - current_y};
    r_angle = {_navmesh_verts[r_vert].first - current_x,
                _navmesh_verts[r_vert].second - current_y};

    ++current;

    do
    {
      while((unsigned)current < funnel.size())
      {
        if(funnel.at(current).first == funnel.at(current - 1).first)
        //turn left
        {
          t_angle = {_navmesh_verts[funnel.at(current).second].first
            - current_x,
            _navmesh_verts[funnel.at(current).second].second
            - current_y};

          if(std::arg(t_angle / l_angle) >= 0.) //break on left
          {
            current = r_break = l_break;
            l_vert = funnel.at(current).first;
            r_vert = funnel.at(current).second;

            current_x = _navmesh_verts[l_vert].first;
            current_y = _navmesh_verts[l_vert].second;
            float tempf =
            (std::fabs(current_x
                    - _navmesh_verts[r_vert].first) +
            std::fabs(current_y
                    - _navmesh_verts[r_vert].second)) * 4;
            current_x +=
            (_navmesh_verts[r_vert].first - current_x) / tempf;
            current_y +=
            (_navmesh_verts[r_vert].second - current_y) / tempf;

            l_angle = {_navmesh_verts[l_vert].first - current_x,
                        _navmesh_verts[l_vert].second - current_y};
            r_angle = -l_angle;

            current_node =
            new PathNode(current_x, current_y, current_node);
          }
          else if(std::arg(t_angle / r_angle) >= 0.)
          {
            r_angle = t_angle;
            r_break = current;
            if(l_break + 1 == current) l_break = current;
          }
        }
        else //turn right
        {
          t_angle =
          {_navmesh_verts[funnel.at(current).first].first
              - current_x,
          _navmesh_verts[funnel.at(current).first].second
          - current_y};

          if(std::arg(t_angle / r_angle) <= 0.) //break on right
          {
            current = l_break = r_break;
            l_vert = funnel.at(current).first;
            r_vert = funnel.at(current).second;

            current_x = _navmesh_verts[r_vert].first;
            current_y = _navmesh_verts[r_vert].second;
            float tempf =
            (std::fabs(current_x
                        - _navmesh_verts[l_vert].first) +
            std::fabs(current_y
                        - _navmesh_verts[l_vert].second)) * 4;
            current_x +=
            (_navmesh_verts[l_vert].first - current_x) / tempf;
            current_y +=
            (_navmesh_verts[l_vert].second - current_y) / tempf;

            r_angle = {_navmesh_verts[r_vert].first - current_x,
                        _navmesh_verts[r_vert].second - current_y};
            l_angle = -r_angle;

            current_node =
            new PathNode(current_x, current_y, current_node);
          }
          else if(std::arg(t_angle / l_angle) <= 0.)
          {
            l_angle = t_angle;
            l_break = current;
            if(r_break + 1 == current) r_break = current;
          }
        }

        ++current;
      }

      //final condition
      t_angle = {x_start - current_x, y_start - current_y};
      if(std::arg(t_angle / l_angle) >= 0.) //break left
      {
        current = r_break = l_break;
        l_vert = funnel.at(current).first;
        r_vert = funnel.at(current).second;

        current_x = _navmesh_verts[l_vert].first;
        current_y = _navmesh_verts[l_vert].second;
        float tempf =
        (std::fabs(current_x - _navmesh_verts[r_vert].first) +
        std::fabs(current_y - _navmesh_verts[r_vert].second)) * 4;
        current_x += 
            (_navmesh_verts[r_vert].first - current_x) / tempf;
        current_y +=
            (_navmesh_verts[r_vert].second - current_y) / tempf;

        l_angle = {_navmesh_verts[l_vert].first - current_x,
                    _navmesh_verts[l_vert].second - current_y};
        r_angle = -l_angle;

        current_node = new PathNode(current_x, current_y, current_node);
        ++current;
      }
      else if(std::arg(t_angle / r_angle) <= 0.) //break right
      {
        current = l_break = r_break;
        l_vert = funnel.at(current).first;
        r_vert = funnel.at(current).second;

        current_x = _navmesh_verts[r_vert].first;
        current_y = _navmesh_verts[r_vert].second;
        float tempf =
        (std::fabs(current_x - _navmesh_verts[l_vert].first) +
        std::fabs(current_y - _navmesh_verts[l_vert].second)) * 4;
        current_x +=
        (_navmesh_verts[l_vert].first - current_x) / tempf;
        current_y +=
        (_navmesh_verts[l_vert].second - current_y) / tempf;

        r_angle = {_navmesh_verts[r_vert].first - current_x,
                    _navmesh_verts[r_vert].second - current_y};
        l_angle = -r_angle;

        current_node = new PathNode(current_x, current_y, current_node);
        ++current;
      }
      else
      {
        current_node = new PathNode(current_x, current_y, current_node);
        break;
      }

    }while(true);

    return current_node;
  }
  
  //collision detection
  bool isBlocked(float x, float y)
  {
    if(!_nav_mesh_triangle_tree_inited) return false;
    return _nav_mesh_triangle_tree.retrieve(x, y) == -1;
  }
  
  void pushIn(float& x, float& y, float last_x, float last_y)
  {
    __SquaredDistFunctor<decltype(NavMeshVert::first)> dist_func(x, y);
  
    int tri_i = _nav_mesh_triangle_tree.retrieve(last_x, last_y);
    
#ifndef NDEBUG
    if(tri_i == -1)
    {
      printf("invalid last coordinates in WorldGeo::pushIn\n%s: %i\n",
              __FILE__, __LINE__);
      return;
    }
#endif

    //triangle bounds
    __NavMeshTriangle& tri = _navmesh_triangles[tri_i];
    NavMeshVert v1 = _navmesh_verts[tri.indices[0]];
    NavMeshVert v2 = _navmesh_verts[tri.indices[1]];
    NavMeshVert v3 = _navmesh_verts[tri.indices[2]];
    
    //biasing (this could prevent chraches)
    /*constexpr double bias = 10.;
    v1.first += (tri.center_x - v1.first) / bias;
    v1.second += (tri.center_y - v1.second) / bias;
    v2.first += (tri.center_x - v2.first) / bias;
    v2.second += (tri.center_y - v2.second) / bias;
    v3.first += (tri.center_x - v3.first) / bias;
    v3.second += (tri.center_y - v3.second) / bias;*/
    
    NavMeshVert point = {x, y};
    int corner = 0;
    double dist_sq, ratio;
    
    do
    {
      if(_dotProduct(v2 - v1, _turn90d(point - v1)) > 0.)
      {
        dist_sq = _dotProduct(v2 - v1, v2 - v1);
        ratio = _dotProduct(v2 - v1, point - v1) / dist_sq;
        
        if(ratio < 0.)
          corner = 1;
        else if(ratio > 1.)
          corner = 2;
        else
        {
          point = v2 * ratio + v1 * (1. - ratio);
          break;
        }
      }
      if(corner != 1 && _dotProduct(v3 - v2, _turn90d(point - v2)) > 0.)
      {
        dist_sq = _dotProduct(v3 - v2, v3 - v2);
        ratio = _dotProduct(v3 - v2, point - v2) / dist_sq;
        
        if(ratio < 0.)
          corner = 2;
        else if(ratio > 1.)
          corner = 3;
        else
        {
          point = v3 * ratio + v2 * (1. - ratio);
          break;
        }
      }
      if(corner != 2 && _dotProduct(v1 - v3, _turn90d(point - v3)) > 0.)
      {
        dist_sq = _dotProduct(v1 - v3, v1 - v3);
        ratio = _dotProduct(v1 - v3, point - v3) / dist_sq;
        
        if(ratio < 0.)
          corner = 3;
        else if(ratio > 1.)
          corner = 1;
        else
        {
          point = v1 * ratio + v3 * (1. - ratio);
          break;
        }
      }
      
      switch(corner)
      {
      case 1:
        point = v1;
        break;
      case 2:
        point = v2;
        break;
      case 3:
        point = v3;
        break;
#ifndef NDEBUG
      default:
        printf("switch fell through in WorldGeo::pushIn\n%s: %i\n",
                __FILE__, __LINE__);
        break;
#endif
      }
    }while(false);
    
    //printf("last_point: %f : %f\n", last_x, last_y);
    //printf("next_point: %f : %f\n", x, y);
    //printf("middle: %f : %f\n", point.first, point.second);
    
    x = point.first;
    y = point.second;
  }


  inline void __constructNavMeshHelper(BlockedMapT* helper)
  {
    //make a square of tiles around helper_total
    for(int n = 0; n < _mapsize_w; ++n)
    {
      helper->set(n);
      helper->set(n + (_mapsize_h - 1) * _mapsize_w);
    }
    for(int n = 0; n < _mapsize_h; ++n)
    {
      helper->set(n * _mapsize_w);
      helper->set(_mapsize_w - 1 + n * _mapsize_w);
    }
  }

  void setupNavMesh(int width, int height, BlockedMapT* blocked_map)
  {
    //clock_t cl = clock();

    using u16pair = std::pair<uint16_t, uint16_t>;
    using PlateType = NMC_Plate;

    _mapsize_w = width;
    _mapsize_h = height;
    _blocked_map = blocked_map;

    std::unique_ptr<BlockedMapT> helper(new BlockedMapT);

    std::stack<u16pair> open_stack;
    std::vector<NavMeshVert> navmesh_helper;

    std::vector<PlateType*> plates;

    _navmesh_verts.clear();
    _navmesh_vert_cons.clear();
    _navmesh_triangles.clear();
    delete[] _status_map;

    //list plates
    *helper = *_blocked_map;
    __constructNavMeshHelper(helper.get());

    for(int y = 1; y < _mapsize_h - 1; ++y)
    for(int x = 1; x < _mapsize_w - 1; ++x)
    {
      if(!helper->test(x + y * _mapsize_w))
      {
        //fill shit
        PlateType* plate = new PlateType(_mapsize_w);
        open_stack.push(u16pair(x, y));

        uint16_t samp_x, samp_y;

        do
        {
          samp_x = open_stack.top().first;
          samp_y = open_stack.top().second;
          open_stack.pop();

          if(!helper->test(samp_x + samp_y * _mapsize_w))
          {
            helper->set(samp_x + samp_y * _mapsize_w);
            plate->set(samp_x + samp_y * _mapsize_w);
            open_stack.push(u16pair(samp_x - 1, samp_y));
            open_stack.push(u16pair(samp_x + 1, samp_y));
            open_stack.push(u16pair(samp_x, samp_y - 1));
            open_stack.push(u16pair(samp_x, samp_y + 1));
          }
        }while(!open_stack.empty());
        plate->setStart(x, y);
        plates.push_back(plate);
      }
    }

    //add holes
    *helper = ~(*_blocked_map);
    __constructNavMeshHelper(helper.get());

    for(int y = 1; y < _mapsize_h - 1; ++y)
    for(int x = 1; x < _mapsize_w - 1; ++x)
    {
      if(!helper->test(x + y * _mapsize_w))
      {
        //fill shit
        uint16_t samp_x, samp_y;
        bool not_hole = false;

        open_stack.push(u16pair(x, y));

        do
        {
          samp_x = open_stack.top().first;
          samp_y = open_stack.top().second;
          open_stack.pop();

          if(!not_hole && (samp_x == 0
              || samp_x == (_mapsize_w - 1) ||
              samp_y == 0
              || samp_y == (_mapsize_h - 1)))
            not_hole = true;

          if(!helper->test(samp_x + samp_y * _mapsize_w))
          {
            helper->set(samp_x + samp_y * _mapsize_w);
            open_stack.push(u16pair(samp_x - 1, samp_y));
            open_stack.push(u16pair(samp_x + 1, samp_y));
            open_stack.push(u16pair(samp_x, samp_y - 1));
            open_stack.push(u16pair(samp_x, samp_y + 1));
          }
        }while(!open_stack.empty());

        if(not_hole)
          continue;

        for(auto ptr: plates)
        {
          if(ptr->operator[](x - 1 + (y - 1) * _mapsize_w))
          {
            ptr->setHole(x, y);
            break;
          }
        }
      }
    }

    helper = nullptr;
    
    _num_plates = 0;
    _plate_ofsets = new int[plates.size() + 1];
    _plate_vert_ofsets = new int[plates.size() + 1];

    for(auto ptr: plates)
    {
      ptr->listVertices();
      ptr->triangulate();
      int num_verts = _navmesh_verts.size();
      int num_triangles = _navmesh_triangles.size();
      
      _plate_ofsets[_num_plates] = num_triangles;
      _plate_vert_ofsets[_num_plates] = num_verts;
      ++_num_plates;
      
      /*_navmesh_verts.insert(_navmesh_verts.end(), 
                          ptr->vBegin(), ptr->vEnd());*/
      
      for(auto it = ptr->vBegin(); it != ptr->vEnd(); ++it)
      {
        _navmesh_verts.push_back(*it);
        _navmesh_vert_cons.push_back(
          {it->first_con + num_verts, it->second_con + num_verts});
      }
      
      _navmesh_triangles.insert(_navmesh_triangles.end(),
                              ptr->tBegin(), ptr->tEnd());
      if(num_verts == 0)
        continue;
      std::for_each(_navmesh_triangles.begin() + num_triangles,
                      _navmesh_triangles.end(),
                    [=](__NavMeshTriangle& tri)
                    {tri.addToIndices(num_verts);
                      tri.addToCons(num_triangles);});
      ptr->clear();
    }
    
    _plate_ofsets[_num_plates] = _navmesh_triangles.size();
    _plate_vert_ofsets[_num_plates] = _navmesh_verts.size();

    std::for_each(_navmesh_verts.begin(),
      _navmesh_verts.end(), [](NavMeshVert& v)
        {v.first /= 8; v.second /= 8;});

    //finalize navmesh triangles
    for(auto& tri: _navmesh_triangles)
    {
      //reorder connections

      //calculate internal distances
      NavMeshVert con0, con1, con2;
      NavMeshVert vert0, vert1, vert2;
      vert0 = _navmesh_verts[tri.indices[0]];
      vert1 = _navmesh_verts[tri.indices[1]];
      vert2 = _navmesh_verts[tri.indices[2]];
      con0 = (vert0 + vert1) / 2;
      con1 = (vert1 + vert2) / 2;
      con2 = (vert2 + vert0) / 2;
      tri.dists[0] = __pts_per_unit * _distance(con0, con1);
      tri.dists[1] = __pts_per_unit * _distance(con1, con2);
      tri.dists[2] = __pts_per_unit * _distance(con2, con0);
      vert0 = (con0 + con1 + con2) / 3;
      tri.center_x = vert0.first;
      tri.center_y = vert0.second;
    }

    //setup quadtree
    if(_nav_mesh_triangle_tree_inited)
      _nav_mesh_triangle_tree.destroy();
    _nav_mesh_triangle_tree.create(16.,
      (_mapsize_w + 127) / 128, (_mapsize_h + 127) / 128);
    for(unsigned n = 0; n < _navmesh_triangles.size(); ++n)
      _nav_mesh_triangle_tree.insert(n);
    _nav_mesh_triangle_tree_inited = true;

    _status_map = new __Status[_navmesh_triangles.size()];
  }

  void deleteNavMesh()
  {
    _navmesh_verts.clear();
    _navmesh_triangles.clear();
    _blocked_map = nullptr;
    delete[] _status_map;
    _status_map = nullptr;
    delete[] _plate_ofsets;
    _plate_ofsets = nullptr;
    delete[] _plate_vert_ofsets;
    _plate_vert_ofsets = nullptr;
    _nav_mesh_triangle_tree.destroy();
    _nav_mesh_triangle_tree_inited = false;
  }

  void displayNavMesh()
  {
    _navmesh_mat = h3dFindResource(H3DResTypes::Material, "navmesh.xml");
    _navmesh_geo = h3dFindResource(H3DResTypes::Geometry, "_navmesh");

    int vert_n, idx_n;

    if(_navmesh_geo)
      h3dUnloadResource(_navmesh_geo);

    if(_navmesh_verts.size() == 0)
      return;

    std::vector<int> navmesh_indices;
    for(auto& tri: _navmesh_triangles)
    {
      navmesh_indices.push_back(tri.indices[0]);
      navmesh_indices.push_back(tri.indices[1]);
      navmesh_indices.push_back(tri.indices[2]);
    }

    Resources::create2DMesh(_navmesh_geo, _navmesh_verts,
                            navmesh_indices, vert_n, idx_n);

    _navmesh_node = h3dAddModelNode(H3DRootNode, "", _navmesh_geo);
    H3DNode mesh = h3dAddMeshNode(_navmesh_node, "", _navmesh_mat,
                                  0, idx_n, 0, vert_n - 1);

    h3dSetNodeTransform(_navmesh_node, 0., 0., 0., 0., 0., 0., 1., 1., 1.);
    h3dSetNodeTransform(mesh, 0., 0., 0., 0., 0., 0., 1., 1., 1.);
  }

  void removeNavMesh()
  {
    if(_navmesh_node)h3dRemoveNode(_navmesh_node);
    _navmesh_node = 0;
  }
}
