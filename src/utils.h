#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <horde3d.h>

#include <stdint.h>
#include <cstddef>
#include <cassert>
#include <cstdio>

#include <utility>

#include "mathutils.h"

namespace Utils
{

///random utilities
template<class T, class I>
constexpr uintptr_t offset(I T::* p)
{return (uintptr_t)&(((T*)nullptr)->*p) - (uintptr_t)nullptr;}

template<class T, class Y, class I>
constexpr uintptr_t offset(I Y::* p)
{return (uintptr_t)&(((T*)nullptr)->*p) - (uintptr_t)nullptr;}

///Horde3d utility functions
void pickRay(H3DNode, float, float, float*, float*,
                      float*, float*, float*, float*);
void pickRayNormalized(H3DNode, float, float, float*, float*,
                                float*, float*, float*, float*);

/***********************************
**    FIXED POINT NUMBER CLASS    **
***********************************/
struct __SET_NUMERATOR_DUMMY{} extern set_numerator;


template<int F, class T>
class fixedp_t
{
private:
  struct __HelperType
  {
    T num;
    __HelperType(T t): num(t){}
  };

  fixedp_t<F, T>(__HelperType t){num = t.num;}

public:
  T num;

  //constructors
  constexpr fixedp_t<F, T>(){}
  constexpr fixedp_t<F, T>(T v): num(v * F){}
  constexpr fixedp_t<F, T>(T v, __SET_NUMERATOR_DUMMY &m): num(v){}
  constexpr fixedp_t<F, T>(double v): num((T)(v * F)){}
  constexpr fixedp_t<F, T>(const fixedp_t<F, T> &v): num(v.num){}

  //typecasting
  template<int G, class Y>
  explicit fixedp_t<F, T>(const fixedp_t<G, Y>&v){num = ((T)v.num * F) / G;}
  //assignment operators
  fixedp_t<F, T>&operator=(T v){num = v * F; return *this;}
  fixedp_t<F, T>&operator=(double v){num = v * F; return *this;}
  fixedp_t<F, T>&operator=(fixedp_t<F, T>v){num = v.num; return *this;}
  //arithmetic operators
  fixedp_t<F, T>operator+(fixedp_t<F, T>v)const
  {return fixedp_t<F, T>(__HelperType(num + v.num));}
  fixedp_t<F, T>operator-(fixedp_t<F, T>v)const
  {return fixedp_t<F, T>(__HelperType(num - v.num));}
  fixedp_t<F, T>operator*(fixedp_t<F, T>v)const
  {return fixedp_t<F, T>(__HelperType(num * v.num / F));}
  fixedp_t<F, T>operator/(fixedp_t<F, T>v)const
  {return fixedp_t<F, T>(__HelperType(num * F / v.num));}
  
  template<class G> fixedp_t<F, T>operator+(G v)const
  {return *this + fixedp_t<F, T>(v);}
  template<class G> fixedp_t<F, T>operator-(G v)const
  {return *this - fixedp_t<F, T>(v);}
  template<class G> fixedp_t<F, T>operator*(G v)const
  {return *this * fixedp_t<F, T>(v);}
  template<class G> fixedp_t<F, T>operator/(G v)const
  {return *this / fixedp_t<F, T>(v);}

  //compound operators
  fixedp_t<F, T>&operator+=(fixedp_t<F, T>v)
  {num += v.num; return *this;}
  fixedp_t<F, T>&operator-=(fixedp_t<F, T>v)
  {num -= v.num; return *this;}
  fixedp_t<F, T>&operator*=(fixedp_t<F, T>v)
  {num *= v.num; num /= F; return *this;}
  fixedp_t<F, T>&operator/=(fixedp_t<F, T>v)
  {num *= F; num /= v.num; return *this;}

  template<class G> fixedp_t<F, T>&operator+=(G v)
  {return *this += fixedp_t<F, T>(v);}
  template<class G> fixedp_t<F, T>&operator-=(G v)
  {return *this -= fixedp_t<F, T>(v);}
  template<class G> fixedp_t<F, T>&operator*=(G v)
  {return *this *= fixedp_t<F, T>(v);}
  template<class G> fixedp_t<F, T>&operator/=(G v)
  {return *this /= fixedp_t<F, T>(v);}
  
  //comparative operators
  bool operator==(fixedp_t<F, T> v)const{return num == v.num;}
  bool operator!=(fixedp_t<F, T> v)const{return num != v.num;}
  bool operator>(fixedp_t<F, T> v)const{return num > v.num;}
  bool operator<(fixedp_t<F, T> v)const{return num < v.num;}
  bool operator>=(fixedp_t<F, T> v)const{return num >= v.num;}
  bool operator<=(fixedp_t<F, T> v)const{return num <= v.num;}
  //typecasts
  operator int()const{return num / F;}
  operator float()const{return (float)num / F;}
  operator double()const{return (double)num / F;}
  operator bool()const{return (bool)num;}

  //other functions
  fixedp_t<F, T> abs(){return fixedp_t<F, T>(__HelperType(std::abs(num)));}
};

///typedefs
typedef fixedp_t<10, int16_t>       fixed16d_t;
typedef fixedp_t<100, int16_t>      fixed16c_t;
typedef fixedp_t<1000, int16_t>     fixed16m_t;
typedef fixedp_t<1000000, int16_t>  fixed16i_t;
typedef fixedp_t<0x10, int16_t>     fixed16n_t;
typedef fixedp_t<0x100, int16_t>    fixed16b_t;
typedef fixedp_t<0x10000, int16_t>  fixed16w_t;

typedef fixedp_t<10, int32_t>       fixed32d_t;
typedef fixedp_t<100, int32_t>      fixed32c_t;
typedef fixedp_t<1000, int32_t>     fixed32m_t;
typedef fixedp_t<1000000, int32_t>  fixed32i_t;
typedef fixedp_t<0x10, int32_t>     fixed32n_t;
typedef fixedp_t<0x100, int32_t>    fixed32b_t;
typedef fixedp_t<0x10000, int32_t>  fixed32w_t;

typedef fixedp_t<10, int64_t>       fixed64d_t;
typedef fixedp_t<100, int64_t>      fixed64c_t;
typedef fixedp_t<1000, int64_t>     fixed64m_t;
typedef fixedp_t<1000000, int64_t>  fixed64i_t;
typedef fixedp_t<0x10, int64_t>     fixed64n_t;
typedef fixedp_t<0x100, int64_t>    fixed64b_t;
typedef fixedp_t<0x10000, int64_t>  fixed64w_t;


/*********************************
**    TEMPLATE VECTOR CLASS     **
*********************************/


template <class T>
class Vec3_t
{
public:
  T x, y, z;

  Vec3_t(){}
  Vec3_t(T _x, T _y, T _z): x(_x), y(_y), z(_z){}
  Vec3_t<T>(const Vec3_t<T> &v): x(v.x), y(v.y), z(v.z){}

  template<class O>Vec3_t<T>(O other)
  {x = std::get<0>(other); y = std::get<1>(other); z = std::get<2>(other);}

  //assignment/compound operators
  Vec3_t<T>& operator=(Vec3_t<T> vec) 
  {x = vec.x; y = vec.y; z = vec.z; return *this;}
  Vec3_t<T>& operator+=(Vec3_t<T> vec)
  {x += vec.x; y += vec.y; z += vec.z; return *this;}
  Vec3_t<T>& operator-=(Vec3_t<T> vec)
  {x -= vec.x; y -= vec.y; z -= vec.z; return *this;}
  Vec3_t<T>& operator*=(T op){x *= op; y *= op; z *= op; return *this;}
  Vec3_t<T>& operator/=(T op){x /= op; y /= op; z /= op; return *this;}
  template<class O> Vec3_t<T>& operator*=(O op)
  {x *= (T)op; y *= (T)op; z *= (T)op; return *this;}
  template<class O> Vec3_t<T>& operator/=(O op)
  {x /= (T)op; y /= (T)op; z /= (T)op; return *this;}

  template<class O>Vec3_t<T>& operator=(O other)
  {x = std::get<0>(other); y = std::get<1>(other);
  z = std::get<2>(other); return *this;}

  //arithmetic operators
  Vec3_t<T> operator+(Vec3_t<T> vec)const
  {return Vec3_t<T>(x + vec.x, y + vec.y, z + vec.z);}
  Vec3_t<T> operator-(Vec3_t<T> vec)const
  {return Vec3_t<T>(x - vec.x, y - vec.y, z - vec.z);}
  Vec3_t<T> operator*(T op)const
  {return Vec3_t<T>(x * op, y * op, z * op);}
  Vec3_t<T> operator/(T op)const
  {return Vec3_t<T>(x / op, y / op, z / op);}
  template<class O> Vec3_t<T> operator*(O op)const
  {return Vec3_t<T>(x * (T)op, y * (T)op, z * (T)op);}
  template<class O> Vec3_t<T> operator/(O op)const
  {return Vec3_t<T>(x / (T)op, y / (T)op, z / (T)op);}
  
  //relational operators
  bool operator==(Vec3_t<T> vec)const
  {return x == vec.x && y == vec.y && z == vec.z;}
  bool operator!=(Vec3_t<T> vec)const
  {return x != vec.x || y != vec.y || z != vec.z;}

  T magnitude()const
  {return (T)std::sqrt((float)(x * x) + (float)(y * y) + (float)(z * z));}
  T magnitudeSquared()const
  {return x * x + y * y + z * z;}
  
  Vec3_t<T> normalize()
  {
    auto mag = magnitude();
    if(mag == (T)0)
      return *this;
    else *this / mag;
  }
};

template <class T>
class Vec2_t
{
  public:
  T x, y;

  Vec2_t(){}
  Vec2_t(T _x, T _y): x(_x), y(_y){}
  Vec2_t<T>(const Vec2_t<T> &v): x(v.x), y(v.y){}

  template<class O>Vec2_t<T>(O other)
  {x = std::get<0>(other); y = std::get<1>(other);}

  //assignment/compount operators
  Vec2_t<T>& operator=(Vec2_t<T> vec)
  {x = vec.x; y = vec.y; return *this;}
  Vec2_t<T>& operator+=(Vec2_t<T> vec)
  {x += vec.x; y += vec.y; return *this;}
  Vec2_t<T>& operator-=(Vec2_t<T> vec)
  {x -= vec.x; y -= vec.y; return *this;}
  Vec2_t<T>& operator*=(T op)
  {x *= op; y *= op; return *this;}
  Vec2_t<T>& operator/=(T op)
  {x /= op; y /= op; return *this;}
  template<class O> Vec2_t<T>& operator*=(O op)
  {x *= (T)op; y *= (T)op; return *this;}
  template<class O> Vec2_t<T>& operator/=(O op)
  {x /= (T)op; y /= (T)op; return *this;}

  template<class O> Vec2_t<T>& operator=(O other)
  {x = std::get<0>(other); y = std::get<1>(other); return *this;}

  //arithmetic operators
  Vec2_t<T> operator+(Vec2_t<T> vec)
  {return Vec2_t<T>(x + vec.x, y + vec.y);}
  Vec2_t<T> operator-(Vec2_t<T> vec)
  {return Vec2_t<T>(x - vec.x, y - vec.y);}
  Vec2_t<T> operator*(T op){return Vec2_t<T>(x * op, y * op);}
  Vec2_t<T> operator/(T op){return Vec2_t<T>(x / op, y / op);}
  template<class O> Vec2_t<T> operator*(O op)
  {return Vec2_t<T>(x * (T)op, y * (T)op);}
  template<class O> Vec2_t<T> operator/(O op)
  {return Vec2_t<T>(x / (T)op, y / (T)op);}
  
  //relational operators
  bool operator==(Vec2_t<T> vec)const
  {return x == vec.x && y == vec.y;}
  bool operator!=(Vec2_t<T> vec)const
  {return x != vec.x || y != vec.y;}

  T magnitude()const
  {return (T)std::sqrt((float)(x * x) + (float)(y * y));}
  T magnitudeSquared()const
  {return x * x + y * y;}
  
  Vec2_t<T> normalize()
  {
    auto mag = magnitude();
    if(mag == (T)0)
      return *this;
    else return (*this / mag);
  }
};


/*********************************
**    ARENA ALLOCATOR CLASS     **
*********************************/

template<unsigned BlockSize, unsigned ChunkSize, unsigned NumChunks = 64>
class ArenaAllocator
{
private:

  union Block
  {
    Block* next;
    char padding[BlockSize];
    //T t;
  };

  Block* _memory[NumChunks];
  Block* _next;

public:

  void* getAdress()
  {
    if(_next == nullptr)
      reserve();

    Block* block = _next;
    _next = _next->next;
    return (void*)block;
  }

  void* getAdress(size_t size)
  {
    if(_next == nullptr)
      reserve();

    Block* block = _next;
    _next = _next->next;
    return (void*)block;
  }

  void freeAdress(void* block)
  {
    ((Block*)block)->next = _next;
    _next = (Block*)block;
  }

  void reserve(uint8_t num_chunks = 1)
  {
    for(auto& chunk: _memory)
    {
      if(chunk == nullptr)
      {
        chunk = new Block[ChunkSize];

        for(unsigned m = 0; m < ChunkSize - 1; ++m)
          chunk[m].next = &chunk[m + 1];

        chunk[ChunkSize - 1].next = _next;
        _next = chunk;
        break;
      }
    }
    if(num_chunks != 1) reserve(num_chunks - 1);
  }

  void deallocate()
  {
    _next = nullptr;

    for(auto& chunk: _memory)
    {
      delete chunk;
      chunk = nullptr;
    }
  }

  ArenaAllocator()
  {
    _next = nullptr;
    for(auto& c: _memory)
      c = nullptr;
  }
  ~ArenaAllocator()
  {
    this->deallocate();
  }
};

/*********************************
**    INTRUSIVE LINKED LIST     **
*********************************/

struct IntrusiveNode
{
  IntrusiveNode* next;
  IntrusiveNode* previous;

  void linkTo(IntrusiveNode& other)
  {
    previous = &other;
    next = other.next;
    next->previous = this;
    previous->next = this;
  }
  void unlink()
  {
    next->previous = previous;
    previous->next = next;
  }

  IntrusiveNode(){}
  ~IntrusiveNode(){unlink();}
};

template<class T>
struct IntrusiveList
{
private:

  uintptr_t offset;

  struct iterator
  {
    IntrusiveNode* ptr;
    uintptr_t offset;

    iterator(IntrusiveNode* start, uintptr_t ofs)
            : ptr(start), offset(ofs){}
    ~iterator(){}

    bool operator!=(const iterator& other) const
    {
      return ptr != other.ptr;
    }
    T* operator*() const
    {
      return (T*)((char*)ptr - offset);
    }
    const iterator& operator++()
    {
      ptr = ptr->next;
      return *this;
    }
  };

public:

  IntrusiveNode head;

  iterator begin()
  {return iterator(head.next, offset);}
  iterator end()
  {return iterator(&head, offset);}

  void push_back(T& obj)
  {
    IntrusiveNode* node = (IntrusiveNode*)(((char*)&obj) + offset);
    node->linkTo(*head.previous);
  }
  void push_front(T& obj)
  {
    IntrusiveNode* node = (IntrusiveNode*)(((char*)&obj) + offset);
    node->linkTo(head);
  }
  void remove(T& obj)
  {
    IntrusiveNode* node = (IntrusiveNode*)(((char*)&obj) + offset);
    node->unlink();
  }
  void pop_back()
  {
    head.previous->unlink();
  }
  void pop_front()
  {
    head.next->unlink();
  }
  T* front() const
  {
    return (T*)((char*)*head.next - offset);
  }
  T* back() const
  {
    return (T*)((char*)*head.previous - offset);
  }
  void clear()
  {
    head.next = &head;
    head.previous = &head;
  }
  bool empty()
  {
    return head.next == &head;
  }
  int size()
  {
    int ret = 0;
    for(auto it = this->begin(); it != this->end(); ++it)
      ++ret;
    return ret;
  }

  template<class Y>
  IntrusiveList(IntrusiveNode Y::* o)
  {
    offset = Utils::offset<T, Y>(o);
    head.next = &head;
    head.previous = &head;
  }
  ~IntrusiveList(){}
};

/**********************************
**    REFERENCE COUNTED TRAIT    **
**********************************/

///RefCountedStd

class RefCountedStd
{
private:

  uint16_t _refcount;
  uint16_t _weakrefcount;
  void* _memptr;

public:

  RefCountedStd(): _refcount(0), _weakrefcount(0){}
  virtual ~RefCountedStd(){}

  void* operator new(size_t size)
  {
    return ::operator new(size);
  }
  void operator delete(void* p)
  {
    ::operator delete(p);
  }

  void incRefCount(){++_refcount;}
  void decRefCount()
  {
    --_refcount;
    if(_refcount == 0)
    {
      this->~RefCountedStd();
      if(_weakrefcount == 0)
        this->operator delete(_memptr);
    }
  }
  void incWeakRefCount(){++_weakrefcount;}
  void decWeakRefCount()
  {
    --_weakrefcount;
    if(_refcount + _weakrefcount == 0)
      this->operator delete(_memptr);
  }

  uint32_t getRefCount(){return _refcount;}
  uint32_t getWeakRefCount(){return _weakrefcount;}

  void setMemoryLocation(void* p){_memptr = p;}
};

///RefCounted (template class)

template<class A, A* allocator>
class RefCounted
{
private:

  uint16_t _refcount;
  uint16_t _weakrefcount;
  void* _memptr;

public:

  RefCounted(): _refcount(0), _weakrefcount(0){}
  virtual ~RefCounted(){}

  void* operator new(size_t size)
  {
    return allocator->getAdress(size);
  }
  void operator delete(void* p)
  {
    allocator->freeAdress(p);;
  }

  void incRefCount(){++_refcount;}
  void decRefCount()
  {
    --_refcount;
    if(_refcount == 0)
    {
      this->~RefCounted();
      if(_weakrefcount == 0)
        this->operator delete(_memptr);
    }
  }
  void incWeakRefCount(){++_weakrefcount;}
  void decWeakRefCount()
  {
    --_weakrefcount;
    if(_refcount + _weakrefcount == 0)
      this->operator delete(_memptr);
  }

  uint32_t getRefCount(){return _refcount;}
  uint32_t getWeakRefCount(){return _weakrefcount;}
  void setMemoryLocation(void* p){_memptr = p;}
};

///helper function

template<class T, class... A>
inline T* makeRefCounted(A... a)
{
  void* p = T::operator new(sizeof(T));
  T* t = ::new(p) T(a...);
  t->setMemoryLocation(p);
  return t;
}

///counted_ptr

template<class T>
class counted_ptr
{
private:
  T* _ptr;

public:
  //counstructors
  counted_ptr()
  {
    _ptr = nullptr;
  }
  counted_ptr(const counted_ptr<T>& other)
  {
    _ptr = other.get();
    if(_ptr) _ptr->incRefCount();
  }
  counted_ptr(counted_ptr<T>&& other)
  {
    _ptr = other._ptr;
    other._ptr = nullptr;
  }
  counted_ptr(T* other)
  {
    _ptr = other;
    if(_ptr) _ptr->incRefCount();
  }
  counted_ptr(std::nullptr_t null)
  {
    _ptr = nullptr;
  }
  template<class Y>
  counted_ptr(Y* other)
  {
    _ptr = static_cast<T*>(other);
    if(_ptr) _ptr->incRefCount();
  }
  template<class Y>
  counted_ptr(const counted_ptr<Y>& other)
  {
    _ptr = static_cast<T*>(other.get());
    if(_ptr) _ptr->incRefCount();
  }

  //destructor
  ~counted_ptr()
  {
    if(_ptr) _ptr->decRefCount();
  }

  //assignment
  template<class Y>
  counted_ptr<T>& operator=(Y* other)
  {
    if(_ptr) _ptr->decRefCount();
    _ptr = (T*)other;
    if(_ptr) _ptr->incRefCount();
    return *this;
  }
  template<class Y>
  counted_ptr<T>& operator=(const counted_ptr<Y>& other)
  {
    if(_ptr) _ptr->decRefCount();
    _ptr = (T*)other.get();
    if(_ptr) _ptr->incRefCount();
    return *this;
  }
  counted_ptr<T>& operator=(counted_ptr<T>&& other)
  {
    if(_ptr) _ptr->decRefCount();
    _ptr = other._ptr;
    other._ptr = nullptr;
    return *this;
  }
  counted_ptr<T>& operator=(std::nullptr_t)
  {
    if(_ptr) _ptr->decRefCount();
    _ptr = nullptr;
    return *this;
  }

  //dereference
  T& operator*()
  {
    return *_ptr;
  }
  T* operator->()
  {
    return _ptr;
  }

  //comparison
  bool operator==(const counted_ptr<T>& other) const
  {
    return _ptr == other._ptr;
  }
  bool operator!=(const counted_ptr<T>& other) const
  {
    return _ptr != other._ptr;
  }
  bool operator==(T* other) const
  {
    return _ptr == other;
  }
  bool operator!=(T* other) const
  {
    return _ptr != other;
  }
  bool operator==(std::nullptr_t) const
  {
    return _ptr == nullptr;
  }
  bool operator!=(std::nullptr_t) const
  {
    return _ptr != nullptr;
  }

  //typecasting
  template<class Y>
  operator Y() const
  {
    return (Y)_ptr;
  }
  operator bool() const
  {
    return _ptr != nullptr;
  }

  //other functions
  T* get() const
  {
    return _ptr;
  }
  void swap(counted_ptr<T>& other)
  {
    T* temp = other._ptr;
    other._ptr = _ptr;
    _ptr = temp;
  }

  uint32_t getRefCount()
  {if(_ptr) return _ptr->getRefCount(); else return 0;}
};

///counted_weak_ptr

template<class T>
class counted_weak_ptr
{
private:
  T* _ptr;

public:
  //counstructors
  counted_weak_ptr()
  {
    _ptr = nullptr;
  }
  counted_weak_ptr(const counted_ptr<T>& other)
  {
    _ptr = other.get();
    if(_ptr) _ptr->incWeakRefCount();
  }
  counted_weak_ptr(const counted_weak_ptr<T>& other)
  {
    _ptr = other.get();
    if(_ptr) _ptr->incWeakRefCount();
  }
  counted_weak_ptr(counted_weak_ptr<T>&& other)
  {
    _ptr = other._ptr;
    other.ptr = nullptr;
  }
  counted_weak_ptr(T* other)
  {
    _ptr = other;
    if(_ptr) _ptr->incWeakRefCount();
  }
  counted_weak_ptr(std::nullptr_t null)
  {
    _ptr = nullptr;
  }
  template<class Y>
  counted_weak_ptr(Y* other)
  {
    _ptr = static_cast<T*>(other);
    if(_ptr) _ptr->incWeakRefCount();
  }
  template<class Y>
  counted_weak_ptr(const counted_weak_ptr<Y>& other)
  {
    _ptr = static_cast<T*>(other.get());
    if(_ptr) _ptr->incWeakRefCount();
  }
  template<class Y>
  counted_weak_ptr(const counted_ptr<Y>& other)
  {
    _ptr = static_cast<T*>(other.get());
    if(_ptr) _ptr->incWeakRefCount();
  }

  //destructor
  ~counted_weak_ptr()
  {
    if(_ptr) _ptr->decWeakRefCount();
  }

  //assignment
  counted_weak_ptr<T>& operator=(counted_ptr<T>& other)
  {
    if(_ptr) _ptr->decWeakRefCount();
    _ptr = other.get();
    if(_ptr) _ptr->incWeakRefCount();
    return *this;
  }
  counted_weak_ptr<T>& operator=(counted_weak_ptr<T>& other)
  {
    if(_ptr) _ptr->decWeakRefCount();
    _ptr = other;
    if(_ptr) _ptr->incWeakRefCount();
    return *this;
  }
  counted_weak_ptr<T>& operator=(T* other)
  {
    if(_ptr) _ptr->decWeakRefCount();
    _ptr = other;
    if(_ptr) _ptr->incWeakRefCount();
    return *this;
  }
  counted_weak_ptr<T>& operator=(std::nullptr_t)
  {
    if(_ptr) _ptr->decWeakRefCount();
    _ptr = nullptr;
    return *this;
  }

  //comparison
  bool operator==(const counted_weak_ptr<T>& other)
  {
    return _ptr == other._ptr;
  }
  bool operator!=(const counted_weak_ptr<T>& other)
  {
    return _ptr != other._ptr;
  }

  //other functions
  void swap(counted_weak_ptr<T>& other)
  {
    T* temp = other._ptr;
    other._ptr = _ptr;
    _ptr = temp;
  }
  T* get()
  {
    if(!expired()) return _ptr;
    else return nullptr;
  }

  uint32_t getRefCount()
  {if(_ptr) return _ptr->getRefCount(); else return 0;}
  bool expired()
  {if(_ptr) return _ptr->getRefCount() == 0; else return true;}

  operator bool() const{return expired();}
};


/**********************************
**           QUAD TREE           **
**********************************/

template<class Storage, unsigned Num, Storage NotFound, unsigned MaxLevel,
bool(*Intersects)(Storage, float, float, float, float),
//args(left, right, top, bottom)
bool(*Contains)(Storage, float, float),
void*(*AllocFunc)(size_t) = operator new,
void(*DeallocFunc)(void*) = operator delete>
class QuadTree
{
  struct Node
  {
    enum
    {
      divided = 0x01,
      maxout  = 0x02
    };

    float left, right, top, bottom;
    uint16_t entries;
    uint8_t level;
    uint8_t status;
    //bool divided;
    //bool maxout;
    union
    {
      struct
      {
        Node* left_top;
        Node* right_top;
        Node* left_bottom;
        Node* right_bottom;
      };
      struct
      {
        Storage* bulk_storage;
        int bulk_size;
      };
      Storage storage[Num];
    };

    Node(): left(0.), right(0.), top(0.), bottom(0.)
    {
      level = 0;
      status = 0;
      entries = 0;
      for(auto& s: storage)
        s = NotFound;
    }
    Node(float l, float r, float t, float b, int e):
    left(l), right(r), top(t), bottom(b), level(e)
    {
      status = 0;
      entries = 0;
      for(auto& s: storage)
        s = NotFound;
    }
    ~Node()
    {
      if(status & divided)
      {
        delete left_top;
        delete right_top;
        delete left_bottom;
        delete right_bottom;
      }
      else if(status & maxout)
        delete[] bulk_storage;
    }

    void setDimensions(float l, float r, float t, float b)
    {
      left = l;
      right = r;
      top = t;
      bottom = b;
    }

    void divide()
    {
      Storage temp[Num];
      for(unsigned n = 0; n < Num; ++n)
        temp[n] = storage[n];

      if(level == MaxLevel)
      {
        bulk_size = Num * 2;
        bulk_storage = new Storage[Num * 2];
        for(unsigned n = 0; n < Num; ++n)
        {
          bulk_storage[n] = temp[n];
          bulk_storage[n + Num] = NotFound;
        }
        status |= maxout;
        return;
      }

      status |= divided;

      //if(right - left < .01) *(int*)nullptr = 0;

      float x_center = (left + right) / 2;
      float y_center = (top + bottom) / 2;
      left_top =
        new Node(left, x_center, top, y_center, level + 1);
      right_top =
        new Node(x_center, right, top, y_center, level + 1);
      left_bottom =
        new Node(left, x_center, y_center, bottom, level + 1);
      right_bottom =
        new Node(x_center, right, y_center, bottom, level + 1);

      for(auto s: temp)
      {
        if(Intersects(s, left, x_center, top, y_center))
          left_top->insert(s);
        if(Intersects(s, x_center, right, top, y_center))
          right_top->insert(s);
        if(Intersects(s, left, x_center, y_center, bottom))
          left_bottom->insert(s);
        if(Intersects(s, x_center, right, y_center, bottom))
          right_bottom->insert(s);
      }
    }

    void downSize()
    {
      Storage temp_storage[Num];
      Storage st;
      float x_center = (left + right) / 2;
      float y_center = (top + bottom) / 2;
      for(int n = 0; n < entries; ++n)
      {
        st = this->retrieve_any();
        if(Intersects(st, left, x_center, top, y_center))
          left_top->remove(st);
        if(Intersects(st, x_center, right, top, y_center))
          right_top->remove(st);
        if(Intersects(st, left, x_center, y_center, bottom))
          left_bottom->remove(st);
        if(Intersects(st, x_center, right, y_center, bottom))
          right_bottom->remove(st);
        temp_storage[n] = st;
      }
      delete left_top;
      delete right_top;
      delete left_bottom;
      delete right_bottom;

      for(int n = 0; n < entries; ++n)
      {
        storage[n] = temp_storage[n];
      }
      status = 0;
    }

    void insert(Storage st)
    {
      ++entries;

      if(status & divided)
      {
        float x_center = (left + right) / 2;
        float y_center = (top + bottom) / 2;

        if(Intersects(st, left, x_center, top, y_center))
          left_top->insert(st);
        if(Intersects(st, x_center, right, top, y_center))
          right_top->insert(st);
        if(Intersects(st, left, x_center, y_center, bottom))
          left_bottom->insert(st);
        if(Intersects(st, x_center, right, y_center, bottom))
          right_bottom->insert(st);
      }
      else if(status & maxout)
      {
        int n;
        for(n = 0; n < bulk_size; ++n)
        {
          if(bulk_storage[n] == NotFound)
          {
            bulk_storage[n] = st;
            break;
          }
        }
        if(n == bulk_size)
        {
          Storage* temp = new Storage[bulk_size * 2];
          for(int m = 0; m < bulk_size; ++m)
          {
            temp[m] = bulk_storage[m];
            temp[m + bulk_size] = NotFound;
          }
          temp[n] = st;
          bulk_size *= 2;
          delete[] bulk_storage;
          bulk_storage = temp;
        }
      }
      else
      {
        for(auto& s: storage)
        {
          if(s == NotFound)
          {
            s = st;
            return;
          }
        }

        this->divide();

        float x_center = (left + right) / 2;
        float y_center = (top + bottom) / 2;

        if(Intersects(st, left, x_center, top, y_center))
          left_top->insert(st);
        if(Intersects(st, x_center, right, top, y_center))
          right_top->insert(st);
        if(Intersects(st, left, x_center, y_center, bottom))
          left_bottom->insert(st);
        if(Intersects(st, x_center, right, y_center, bottom))
          right_bottom->insert(st);
      }
    }

    void remove(Storage st)
    {
      --entries;

      if(status & divided)
      {
        float x_center = (left + right) / 2;
        float y_center = (top + bottom) / 2;

        if(Intersects(st, left, x_center, top, y_center))
          left_top->remove(st);
        if(Intersects(st, x_center, right, top, y_center))
          right_top->remove(st);
        if(Intersects(st, left, x_center, y_center, bottom))
          left_bottom->remove(st);
        if(Intersects(st, x_center, right, y_center, bottom))
          right_bottom->remove(st);

        if(entries <= Num) this->downSize();
      }
      else if(status & maxout)
      {
        int n;
        for(n = 0; n < bulk_size; ++n)
        {
          if(bulk_storage[n] == st)
          {
            bulk_storage[n] = NotFound;
            ++n;
            break;
          }
        }
        while(n < bulk_size)
        {
          bulk_storage[n - 1] = bulk_storage[n];
        }
      }
      else
      {
        int n;
        for(n = 0; n < Num; ++n)
        {
          if(storage[n] == st)
          {
            storage[n] = NotFound;
            ++n;
            break;
          }
        }
        while(n < Num)
        {
          storage[n - 1] = storage[n];
        }
      }
    }

    Storage retrieve(float x, float y)
    {
      if(status & divided)
      {
        if(x < (left + right) / 2)
        {
          if(y < (top + bottom) / 2)
            return left_bottom->retrieve(x, y);
          else return left_top->retrieve(x, y);
        }
        else
        {
          if(y < (top + bottom) / 2)
            return right_bottom->retrieve(x, y);
          else return right_top->retrieve(x, y);
        }
      }
      else if(status & maxout)
      {
        for(int n = 0; n < bulk_size; ++n)
        {
          if(bulk_storage[n] != NotFound 
            && Contains(bulk_storage[n], x, y))
            return bulk_storage[n];
        }
        return NotFound;
      }
      else
      {
        for(auto s: storage)
        {
          if(s != NotFound && Contains(s, x, y))
            return s;
        }
        return NotFound;
      }
    }

    Storage retrieve_any(float xmin, float xmax, float ymax, float ymin)
    {
      xmin = xmin < left? left : xmin;
      xmax = xmax > right? right : xmax;
      ymin = ymin < bottom? bottom : ymin;
      ymax = ymax > top? top : ymax;

      if(xmin >= xmax || ymin >= ymax) return NotFound;

      if(status & divided)
      {
        Storage st;
        st = left_top->retrieve_any(xmin, xmax, ymax, ymin);
        if(st != NotFound) return st;
        st = right_bottom->retrieve_any(xmin, xmax, ymax, ymin);
        if(st != NotFound) return st;
        st = left_top->retrieve_any(xmin, xmax, ymax, ymin);
        if(st != NotFound) return st;
        st = right_bottom->retrieve_any(xmin, xmax, ymax, ymin);
        return st;
      }
      else if(status & maxout)
      {
        for(int n = 0; n < bulk_size / 2; ++n)
        {
          if(Intersects(bulk_storage[n], xmin, xmax, ymax, ymin))
            return bulk_storage[n];
          if(bulk_storage[n + bulk_size / 2] != NotFound &&
            Intersects(bulk_storage[n + bulk_size / 2],
                      xmin, xmax, ymax, ymin))
            return bulk_storage[n];
        }
      }
      else
      {
        for(Storage st: storage)
        {
          if(Intersects(st, xmin, xmax, ymax, ymin)) return st;
        }
      }

      return NotFound;
    }

    Storage retrieve_any()
    {
      if(status & divided)
      {
        Storage st;
        st = left_top->retrieve_any();
        if(st != NotFound) return st;
        st = right_bottom->retrieve_any();
        if(st != NotFound) return st;
        st = left_top->retrieve_any();
        if(st != NotFound) return st;
        st = right_bottom->retrieve_any();
        return st;
      }
      else if(status & maxout)
      {
        if(entries) return bulk_storage[0];
        else return NotFound;
      }
      else
      {
        if(entries) return storage[0];
        else return NotFound;
      }
    }

    bool found(float xmin, float xmax, float ymax, float ymin)
    {
      xmin = xmin < left? left : xmin;
      xmax = xmax > right? right : xmax;
      ymin = ymin < bottom? bottom : ymin;
      ymax = ymax > top? top : ymax;

      if(xmin >= xmax || ymin >= ymax) return false;
      if(xmin == left && xmax == right && ymin == bottom && ymax == top)
        return entries;

      if(status & divided)
      {
        if(left_top->found(xmin, xmax, ymax, ymin)) return true;
        if(right_top->found(xmin, xmax, ymax, ymin)) return true;
        if(left_bottom->found(xmin, xmax, ymax, ymin)) return true;
        if(right_bottom->found(xmin, xmax, ymax, ymin)) return true;
      }
      else if(status & maxout)
      {
        for(int n = 0; n < bulk_size / 2; ++n)
        {
          if(Intersects(bulk_storage[n], xmin, xmax, ymax, ymin))
            return true;
          if(bulk_storage[n + bulk_size / 2] != NotFound &&
            Intersects(bulk_storage[n + bulk_size / 2],
            xmin, xmax, ymax, ymin)) return true;
        }
      }
      else
      {
        for(auto st: storage)
        {
          if(st == NotFound) break;
          if(Intersects(st, xmin, xmax, ymax, ymin)) return true;
        }
      }

      return false;
    }

    void* operator new(size_t s)
    {
      return AllocFunc(s);
    }
    void operator delete(void* p)
    {
      DeallocFunc(p);
    }
  };

  Node* _initial_nodes;
  int _width, _height;
  float _size;

public:

  static constexpr size_t node_size = sizeof(Node);

  QuadTree(): _initial_nodes(nullptr){}
  ~QuadTree(){delete[] _initial_nodes;}

  void create(float size, int w, int h)
  {
    _width = w;
    _height = h;
    _size = size;

    _initial_nodes = new Node[w * h];
    for(int x = 0; x < w; ++x)
    for(int y = 0; y < h; ++y)
      _initial_nodes[x + y * w].setDimensions(x * size,
        (x + 1) * size, (y + 1) * size, y * size);
  }
  void destroy()
  {
    delete[] _initial_nodes;
    _initial_nodes = nullptr;
  }

  void insert(Storage st)
  {
    for(int n = 0; n < _width * _height; ++n)
    {
      if(Intersects(st, _initial_nodes[n].left, _initial_nodes[n].right,
                    _initial_nodes[n].top, _initial_nodes[n].bottom))
        _initial_nodes[n].insert(st);
    }
  }

  void remove(Storage st)
  {
    for(int n = 0; n < _width * _height; ++n)
    {
      if(Intersects(st, _initial_nodes[n].left, _initial_nodes[n].right,
                    _initial_nodes[n].top, _initial_nodes[n].bottom))
        _initial_nodes[n].remove(st);
    }
  }

  Storage retrieve(float x, float y)
  {
    return _initial_nodes
    [(int)(x / _size) + (int)(y / _size) * _width].retrieve(x, y);
  }

  Storage retrieve(float x, float y, float range, float inc)
  {
    assert(range > 0.);
    assert(inc > 0.);
    assert(range > inc);
    //Storage st;
    //st = retrieve(x, y);
    //if(st != NotFound) return st;
    //std::printf("bad retrieve.\n");

    int search_l, search_r, search_b, search_t;

    search_l = (int)((x - range) / _size);
    search_b = (int)((y - range) / _size);
    search_r = (int)((x + range) / _size);
    search_t = (int)((y + range) / _size);

    if(search_l < 0) search_l = 0;
    if(search_b < 0) search_b = 0;
    if(search_r >= _width) search_r = _width - 1;
    if(search_t >= _height) search_t = _height - 1;

    for(float f = inc; f <= range; f += inc)
    {
      for(int w = search_l; w <= search_r; ++w)
      for(int h = search_b; h <= search_t; ++h)
      {
        if(_initial_nodes[w + h * _width].found(
          x - f, x + f, y + f, y - f))
          return _initial_nodes[w + h * _width].retrieve_any(
                                  x - f, x + f, y + f, y - f);
        /*{
          printf("found at dist: %f\n", f);
          return NotFound;
        }*/
      }
    }
    return NotFound;
  }

  //overloads
  QuadTree<Storage, Num, NotFound, MaxLevel,
  Intersects, Contains, AllocFunc, DeallocFunc>&
  operator<<(Storage st)
  {
    insert(st);
    return *this;
  }
};


///dummy type for node_size constexpr
template<class Storage, int Num>
class QuadTreeDummy
{
  struct Node
  {
    float left, right, top, bottom;
    bool divided;
    //uint8_t level;
    union
    {
      struct
      {
        Node* left_top;
        Node* right_top;
        Node* left_bottom;
        Node* right_bottom;
      };
      Storage storage[Num];
    };
  };

public:
  static constexpr size_t node_size = sizeof(Node);
};


/**********************************
**            BIT MAP            **
**********************************/

template<class T>
class BitMap
{
protected:

  int _width;
  int _height;
  
  T* _map;
  
public:

  BitMap(int w, int h)
  {
    _width = w;
    _height = h;
    _map = new T[w * h];
  }
  ~BitMap()
  {
    delete[] _map;
  }
  
  T& operator()(int x, int y)
  {
    return _map[x + y * _width];
  }
  //bilinear interpolation
  T operator()(double x, double y)
  {
    int _x, _y;
    T sample[4];
    _x = (int)x;
    _y = (int)y;
    x -= (double)_x;
    y -= (double)_y;
    
    sample[0] = _map[_x + _y * _width];
    sample[1] = _map[(_x + 1) + _y * _width];
    sample[2] = _map[_x + (_y + 1) * _width];
    sample[3] = _map[(_x + 1) + (_y + 1) * _width];
    
    sample[0] = sample[1] * x + sample[0] * (1. - x);
    sample[2] = sample[3] * x + sample[2] * (1. - x);
    
    return sample[2] * y + sample[0] * (1. - y);
  }
  
  //other functions
  void clear(T cl)
  {
    for(int n = 0; n < _width * _height; ++n)
      _map[n] = cl;
  }
  
  T* getData()
  {
    return _map;
  }
  
  void getResolution(int* w, int* h)
  {
    *w = _width;
    *h = _height;
  }
  
  void copy(BitMap<T>& other)
  {
    for(int n = 0; n < _width * _height; ++n)
      _map[n] = other._map[n];
  }
  
  void blit(BitMap<T>& other,
  int this_x, int this_y, int other_x, int other_y, int size_x, int size_y)
  {
    T* own_map_p = _map + (this_x + this_y * _width);
    T* other_map_p = other._map + (other_x + other_y * other._width);
    for(int y = 0; y < size_y; ++y)
    {
      for(int x = 0; x < size_x; ++x)
      {
        *own_map_p = *other_map_p;
        ++own_map_p;
        ++other_map_p;
      }
      own_map_p -= size_x;
      own_map_p += _width;
      other_map_p -= size_x;
      other_map_p += other._width;
    }
  }
  
  //higher order functions
  template<class F>
  void apply(F func)
  {
    for(int n = 0; n < _width * _height; ++n)
      _map[n] = func(_map[n]);
  }
  template<class F>
  void apply(BitMap<T>& other, F func)
  {
    for(int n = 0; n < _width * _height; ++n)
      _map[n] = func(_map[n], other._map[n]);
  }
  
  template<class F>
  void for_each(F func)
  {
    for(int n = 0; n < _width * _height; ++n)
      func(_map[n]);
  }
  
  template<class F>
  void blit(BitMap<T>& other,
  int this_x, int this_y, int other_x, int other_y, int size_x, int size_y,
  F func)
  {
    T* own_map_p = _map + (this_x + this_y * _width);
    T* other_map_p = other._map + (other_x + other_y * other._width);
    for(int y = 0; y < size_y; ++y)
    {
      for(int x = 0; x < size_x; ++x)
      {
        *own_map_p = func(*other_map_p);
        ++own_map_p;
        ++other_map_p;
      }
      own_map_p -= size_x;
      own_map_p += _width;
      other_map_p -= size_x;
      other_map_p += other._width;
    }
  }
};

/**********************************
**   SIGNED DISTANCE FIELD MAP   **
**********************************/

template<class T, class F, F(*DistFunc)(F, F)>
class SDFMap: public BitMap<T>
{
protected:

  struct __Field
  {
    F point_x;
    F point_y;
    F point_size;
    F opacity;
  };
  
  __Field* _dist_map;
  
  /*static F _distSq(F x1, F y1, F x2, F y2)
  {
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
  }*/
  
  void _replaceIfLess(int x, int y, F x2, F y2, F size)
  {
    F n_opacity = size - DistFunc(x2 - F(x), y2 - F(y));
    if(_dist_map[x + y * this->_width].opacity < n_opacity)
    {
      _dist_map[x + y * this->_width].point_x = x2;
      _dist_map[x + y * this->_width].point_y = y2;
      _dist_map[x + y * this->_width].point_size = size;
      _dist_map[x + y * this->_width].opacity = n_opacity;
    }
  }

public:

  SDFMap(int w, int h): BitMap<T>(w, h)
  {
    _dist_map = new __Field[w * h];
  }
  ~SDFMap()
  {
    delete[] _dist_map;
  }
  
  void clearDistances()
  {
    for(int n = 0; n < this->_width * this->_height; ++n)
    {
      _dist_map[n].opacity = F(0);
    }
  }
  
  void insertPoint(F x, F y, F size)
  {
    int _x = (int)(x);// + F(0.5));
    int _y = (int)(y);// + F(0.5));
    
    _replaceIfLess(_x, _y, x, y, size);
    _replaceIfLess(_x, _y + 1, x, y, size);
    _replaceIfLess(_x + 1, _y, x, y, size);
    _replaceIfLess(_x + 1, _y + 1, x, y, size);
  }
  
  void generateDistances()
  {
    F px, py, ps;
    
    //using dead recconing algorithm
    //first direction
    for(int y = 0; y < this->_height - 1; ++y)
    {
      if(_dist_map[y * this->_width].opacity > F(0))
      {
        px = _dist_map[y * this->_width].point_x;
        py = _dist_map[y * this->_width].point_y;
        ps = _dist_map[y * this->_width].point_size;
        
        _replaceIfLess(1, y, px, py, ps);
        _replaceIfLess(1, y + 1, px, py, ps);
        _replaceIfLess(0, y + 1, px, py, ps);
      }
      
      for(int x = 1; x < this->_width - 1; ++x)
      {
        if(_dist_map[x + y * this->_width].opacity > F(0))
        {
          px = _dist_map[x + y * this->_width].point_x;
          py = _dist_map[x + y * this->_width].point_y;
          ps = _dist_map[x + y * this->_width].point_size;
          _replaceIfLess(x + 1, y, px, py, ps);
          _replaceIfLess(x + 1, y + 1, px, py, ps);
          _replaceIfLess(x, y + 1, px, py, ps);
          _replaceIfLess(x - 1, y + 1, px, py, ps);
        }
      }
      
      if(_dist_map[this->_width - 1 + y * this->_height].opacity > F(0))
      {
        px = _dist_map[this->_width - 1 + y * this->_height].point_x;
        py = _dist_map[this->_width - 1 + y * this->_height].point_y;
        ps = _dist_map[this->_width - 1 + y * this->_height].point_size;
        
        _replaceIfLess(this->_width - 1, y + 1, px, py, ps);
        _replaceIfLess(this->_width - 2, y + 1, px, py, ps);
      }
    }
    for(int x = 0; x < this->_width - 1; ++x)
    {
      if(_dist_map[x + (this->_height - 1) * this->_width].opacity > F(0))
      {
        px = _dist_map[x + (this->_height - 1) * this->_width].point_x;
        py = _dist_map[x + (this->_height - 1) * this->_width].point_y;
        ps = _dist_map[x + (this->_height - 1) * this->_width].point_size;
        _replaceIfLess(x + 1, this->_height - 1, px, py, ps);
      }
    }
    //other direction
    for(int y = this->_height - 1; y > 0; --y)
    {
      if(_dist_map[this->_width - 1 + y * this->_width].opacity > F(0))
      {
        px = _dist_map[this->_width - 1 + y * this->_width].point_x;
        py = _dist_map[this->_width - 1 + y * this->_width].point_y;
        ps = _dist_map[this->_width - 1 + y * this->_width].point_size;
        
        _replaceIfLess(this->_width - 2, y, px, py, ps);
        _replaceIfLess(this->_width - 2, y - 1, px, py, ps);
        _replaceIfLess(this->_width - 1, y - 1, px, py, ps);
      }
      
      for(int x = this->_width - 2; x > 0; --x)
      {
        if(_dist_map[x + y * this->_width].opacity > F(0))
        {
          px = _dist_map[x + y * this->_width].point_x;
          py = _dist_map[x + y * this->_width].point_y;
          ps = _dist_map[x + y * this->_width].point_size;
          
          _replaceIfLess(x - 1, y, px, py, ps);
          _replaceIfLess(x - 1, y - 1, px, py, ps);
          _replaceIfLess(x, y - 1, px, py, ps);
          _replaceIfLess(x + 1, y - 1, px, py, ps);
        }
      }
      
      if(_dist_map[y * this->_width].opacity > F(0))
      {
        px = _dist_map[y * this->_width].point_x;
        py = _dist_map[y * this->_width].point_y;
        ps = _dist_map[y * this->_width].point_size;
        
        _replaceIfLess(0, y - 1, px, py, ps);
        _replaceIfLess(1, y - 1, px, py, ps);
      }
    }
    for(int x = this->_width - 1; x > 0; --x)
    {
      if(_dist_map[x].opacity > F(0))
      {
        px = _dist_map[x].point_x;
        py = _dist_map[x].point_y;
        ps = _dist_map[x].point_size;
        
        _replaceIfLess(x - 1, 0, px, py, ps);
      }
    }
  }
  
  template<class L>
  void generateBitMap(L func)
  {
    for(int n = 0; n < this->_width * this->_height; ++n)
      this->_map[n] = func(_dist_map[n].opacity);
  }
};


/**********************************
**      CACHE FRIENDLY HEAP      **
**********************************/

template<class T>
class CFHeap
{
  T* storage;
  int capacity;
  int _size;
  
  void allocate_more()
  {
    capacity <<= 1;
    capacity += 1;
    T* temp = static_cast<T*>(::operator new(capacity * sizeof(T)));
    for(int i = 0; i < _size; ++i)
    {
      new(temp + i) T(std::move(storage[i]));
    }
    delete[] storage;
    storage = temp;
  }
  void up_swap()
  {
    int idx = _size;
    while(idx != 0)
    {
      int next_idx = ((idx + 1) >> 1) - 1;
      if(storage[idx] > storage[next_idx])
      {
        std::swap(storage[idx], storage[next_idx]);
        idx = next_idx;
      }
      else break;
    }
  }
  void down_swap()
  {
    int idx = 0;
    do
    {
      int next_idx = ((idx + 1) << 1) - 1;
      if(next_idx >= _size) break;
      if(next_idx + 1 != _size && storage[next_idx] < storage[next_idx + 1])
        ++next_idx;
      if(storage[idx] < storage[next_idx])
      {
        std::swap(storage[idx], storage[next_idx]);
        idx = next_idx;
      }
      else break;
    }while(true);
  }
  
public:
  CFHeap()
  {
    storage = static_cast<T*>(::operator new(15 * sizeof(T)));
    capacity = 15;
    _size = 0;
  }
  CFHeap(const CFHeap<T>& other)
  {
    capacity = other.capacity;
    _size = other._size;
    storage = static_cast<T*>(::operator new(capacity * sizeof(T)));
    for(int i = 0; i < _size; ++i)
      new(storage + i) T(other.storage[i]);
  }
  CFHeap(CFHeap<T>&& other)
  {
    capacity = other.capacity;
    _size = other.size;
    storage = other.storage;
    other.storage = nullptr;
    other._size = 0;
  }
  ~CFHeap()
  {
    for(int i = 0; i < _size; ++i)
      storage[i].~T();
    ::operator delete(storage);
  }
  
  CFHeap<T>& operator=(const CFHeap<T>& other)
  {
    for(int i = 0; i < _size; ++i)
      storage[i].~T();
    ::operator delete(storage);
    capacity = other.capacity;
    _size = other._size;
    storage = static_cast<T*>(::operator new(capacity * sizeof(T)));
    for(int i = 0; i < _size; ++i)
      new(storage + i) T(other.storage[i]);
    return *this;
  }
  CFHeap<T>& operator=(CFHeap<T>&& other)
  {
    for(int i = 0; i < _size; ++i)
      storage[i].~T();
    ::operator delete(storage);
    capacity = other.capacity;
    _size = other._size;
    storage = other.storage;
    other.storage = nullptr;
    other._size = 0;
  }
  
  void push(const T& element)
  {
    if(_size == capacity)
      allocate_more();
    new(storage + _size) T(element);
    up_swap();
    ++_size;
  }
  void push(T&& element)
  {
    if(_size == capacity)
      allocate_more();
    new(storage + _size) T(element);
    up_swap();
    ++size;
  }
  void pop()
  {
    storage->~T();
    new(storage) T(std::move(storage[_size - 1]));
    storage[_size - 1].~T();
    --_size;
    down_swap();
  }
  template<class... Args>
  void emplace(Args&&... args)
  {
    if(_size == capacity)
      allocate_more();
    new(storage) T(std::forward<Args>(args)...);
    up_swap();
    ++_size;
  }
  void swap(CFHeap<T>& other)
  {
    int cap = capacity;
    int si = _size;
    auto st = storage;
    capacity = other.capacity;
    _size = other._size;
    storage = other.storage;
    other.capacity = cap;
    other._size = si;
    other.storage = st;
  }

  T& top()
  {
    return *storage;
  }
  
  bool empty()
  {
    return _size == 0;
  }
  int size()
  {
    return _size;
  }
};

}; //namespace utils

#endif