#define RESOURCES_CPP

#include <horde3d.h>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cassert>

#include <memory>

#include "mathutils.h"

#include "app.h"
#include "interface.h"
#include "entities.h"

#include "resources.h"

namespace Resources
{
  ///procedural generation functions
  std::pair<int, int> generateGridOH(H3DRes r, double size, int div)
  {
    /*
      this function actually generates a grid with overhang and a stray vertex
      for expanding the AABB box
    */

    int num_vertices;
    int num_indices;
    int total_size;
    
    char* data_p;

    union
    {
      unsigned int ui[3];
      signed short us[6];
      float uf[3];
      char uc[12];
    };
    
    div += 2;

    num_vertices = (div + 1) * (div + 1);
    num_indices = div * div * 6 - 24;
    
    std::unique_ptr<double[]> divisions(new double[div + 1]);
    divisions[0] = 0.0;
    divisions[1] = 0.0;
    divisions[2] = size / (double)div;
    for(int n = 3; n < div - 1; ++n)
    {
      divisions[n] = divisions[2] * (n - 1);
    }
    divisions[div - 1] = size;
    divisions[div] = size;

    total_size = 0;
    total_size += 20;                         //header
    total_size += 8 + 12 * (num_vertices + 1);//vetrex stream
    total_size += 8 + 6 * (num_vertices + 1); //normal stream
    total_size += 8 + 8 * (num_vertices + 1); //texcoord stream
    total_size += 4 + 4 * num_indices;        //index stream
    total_size += 4;                          //morph targets
    
    std::unique_ptr<char[]> temp_data(new char[total_size]);
    data_p = temp_data.get();

    ///generate data:

    //header
    std::strncpy(uc, "H3DG", 4);    //magic string
    ui[1] = 5;                      //version number
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    ui[0] = 0;                //number of joints
    ui[1] = 3;                //number of streams
    ui[2] = num_vertices + 1; //number of vertices
    std::memcpy(data_p, uc, 12);
    data_p += 12;

    //vertex stream
    ui[0] = 0;
    ui[1] = 12;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    for(int n = 0; n < num_vertices; ++n)
    {
      uf[0] = divisions[n % (div + 1)];
      uf[2] = divisions[n / (div + 1)];
      if(n % (div + 1) == 0 || n % (div + 1) == div ||
        n < div + 1 || n >= div * (div + 1))
        uf[1] = -0.1;
      else uf[1] = 0.0;

      std::memcpy(data_p, uc, 12);
      data_p += 12;
    }
    
    uf[0] = 0.0; uf[1] = 5.0; uf[2] = 0.0;
    std::memcpy(data_p, uc, 12);
    data_p += 12;

    //normal stream
    ui[0] = 1;
    ui[1] = 6;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    for(int n = 0; n < num_vertices; ++n)
    {
      if(n % (div + 1) == 0 || n % (div + 1) == div ||
        n < div + 1 || n >= div * (div + 1))
      {
        us[1] = 0;
        if(n % (div + 1) == 0)
          us[0] = 0x8000;
        else if(n % (div + 1) == div)
          us[0] = 0x7fff;
        if(n < div + 1)
          us[2] = 0x8000;
        else if(n >= div * (div + 1))
          us[2] = 0x7fff;
      }
      else
      {
        us[0] = 0;
        us[1] = 0x7fff;
        us[2] = 0;
      }
      
      std::memcpy(data_p, uc, 6);
      data_p += 6;
    }

    us[0] = 0; us[1] = 0x7fff; us[2] = 0;
    std::memcpy(data_p, uc, 6);
    data_p += 6;

    //texcoord stream
    ui[0] = 6;
    ui[1] = 8;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    for(int n = 0; n < num_vertices; ++n)
    {
      uf[0] = divisions[n % (div + 1)] / size;
      uf[1] = divisions[n / (div + 1)] / size;
      std::memcpy(data_p, uc, 8);
      data_p += 8;
    }

    uf[0] = 0.0; uf[1] = 0.0;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    //indices
    ui[0] = num_indices;
    std::memcpy(data_p, uc, 4);
    data_p += 4;
    
    auto add_indices = [&ui, div, &uc, &data_p](int n)
    {
      ui[0] = n + div + 1 + n / div;
      ui[1] = n + div + 2 + n / div;
      ui[2] = n + 1 + n / div;
      std::memcpy(data_p, uc, 12);
      data_p += 12;

      ui[0] = n + div + 1 + n / div;
      ui[1] = n + 1 + n / div;
      ui[2] = n + n / div;
      std::memcpy(data_p, uc, 12);
      data_p += 12;
    };
    
    for(int n = 1; n < div - 1; ++n)
      add_indices(n);
    for(int n = div; n < div * (div - 1); ++n)
      add_indices(n);
    for(int n = div * (div - 1) + 1; n < div * div - 1; ++n)
      add_indices(n);

    //morph targets
    ui[0] = 0;
    std::memcpy(data_p, uc, 4);
    data_p += 4;

    ///data generated

    h3dUnloadResource(r);
    h3dLoadResource(r, temp_data.get(), total_size);

    data_p = nullptr;

    return std::make_pair(num_vertices + 1, num_indices);
  }
  
  std::pair<int, int> generateGrid(H3DRes r, double size, int div)
  {
    /*
      this function actually generates a grid with overhang and a stray vertex
      for expanding the AABB box
    */

    int num_vertices;
    int num_indices;
    int total_size;
    
    char* data_p;

    union
    {
      unsigned int ui[3];
      signed short us[6];
      float uf[3];
      char uc[12];
    };
    
    num_vertices = (div + 1) * (div + 1);
    num_indices = div * div * 6;
    
    std::unique_ptr<double[]> divisions(new double[div + 1]);
    divisions[0] = 0.0;
    divisions[1] = size / (double)div;
    for(int n = 2; n < div; ++n)
    {
      divisions[n] = divisions[1] * n;
    }
    divisions[div] = size;

    total_size = 0;
    total_size += 20;                         //header
    total_size += 8 + 12 * (num_vertices + 1);//vetrex stream
    total_size += 8 + 6 * (num_vertices + 1); //normal stream
    total_size += 8 + 8 * (num_vertices + 1); //texcoord stream
    total_size += 4 + 4 * num_indices;        //index stream
    total_size += 4;                          //morph targets
    
    std::unique_ptr<char[]> temp_data(new char[total_size]);
    data_p = temp_data.get();

    ///generate data:

    //header
    std::strncpy(uc, "H3DG", 4);    //magic string
    ui[1] = 5;                      //version number
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    ui[0] = 0;                //number of joints
    ui[1] = 3;                //number of streams
    ui[2] = num_vertices + 1; //number of vertices
    std::memcpy(data_p, uc, 12);
    data_p += 12;

    //vertex stream
    ui[0] = 0;
    ui[1] = 12;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    for(int n = 0; n < num_vertices; ++n)
    {
      uf[0] = divisions[n % (div + 1)];
      uf[1] = 0.0;
      uf[2] = divisions[n / (div + 1)];

      std::memcpy(data_p, uc, 12);
      data_p += 12;
    }
    
    uf[0] = 0.0; uf[1] = 5.0; uf[2] = 0.0;
    std::memcpy(data_p, uc, 12);
    data_p += 12;

    //normal stream
    ui[0] = 1;
    ui[1] = 6;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    for(int n = 0; n < num_vertices; ++n)
    {
      us[0] = 0;
      us[1] = 0x7fff;
      us[2] = 0;
      
      std::memcpy(data_p, uc, 6);
      data_p += 6;
    }

    us[0] = 0; us[1] = 0x7fff; us[2] = 0;
    std::memcpy(data_p, uc, 6);
    data_p += 6;

    //texcoord stream
    ui[0] = 6;
    ui[1] = 8;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    for(int n = 0; n < num_vertices; ++n)
    {
      uf[0] = divisions[n % (div + 1)] / size;
      uf[1] = divisions[n / (div + 1)] / size;
      std::memcpy(data_p, uc, 8);
      data_p += 8;
    }

    uf[0] = 0.0; uf[1] = 0.0;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    //indices
    ui[0] = num_indices;
    std::memcpy(data_p, uc, 4);
    data_p += 4;

    for(int n = 0; n < num_indices / 6; ++n)
    {
      ui[0] = n + div + 1 + n / div;
      ui[1] = n + div + 2 + n / div;
      ui[2] = n + 1 + n / div;
      std::memcpy(data_p, uc, 12);
      data_p += 12;

      ui[0] = n + div + 1 + n / div;
      ui[1] = n + 1 + n / div;
      ui[2] = n + n / div;
      std::memcpy(data_p, uc, 12);
      data_p += 12;
    }

    //morph targets
    ui[0] = 0;
    std::memcpy(data_p, uc, 4);
    data_p += 4;

    ///data generated

    h3dUnloadResource(r);
    h3dLoadResource(r, temp_data.get(), total_size);

    data_p = nullptr;

    return std::make_pair(num_vertices + 1, num_indices);
  }

  std::pair<int, int> generateCube(H3DRes r, double size)
  {
    uint32_t num_vertices;
    uint32_t num_indices;
    uint32_t total_size;

    char* data_p;

    union
    {
      unsigned int ui[3];
      signed short us[6];
      float uf[3];
      char uc[12];
    };

    num_vertices = 24;  //6*4
    num_indices = 36;   //6*6

    total_size = 0;
    total_size += 20;                       //header
    total_size += 8 + 12 * num_vertices;    //vetrex stream
    total_size += 8 + 6 * num_vertices;     //normal stream
    total_size += 8 + 8 * num_vertices;     //texcoord stream
    total_size += 4 + 4 * num_indices;      //index stream
    total_size += 4;                        //morph targets

    std::unique_ptr<char[]> temp_data(new char[total_size]);
    data_p = temp_data.get();

    ///generate data:

    //header
    std::strncpy(uc, "H3DG", 4);    //magic string
    ui[1] = 5;                      //versoin number
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    ui[0] = 0;                      //number of joints
    ui[1] = 3;                      //number of streams
    ui[2] = num_vertices;           //number of vertices
    std::memcpy(data_p, uc, 12);
    data_p += 12;

    //vertex stream
    ui[0] = 0;
    ui[1] = 12;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    for(uint32_t n = 0; n < num_vertices; ++n)
    {
      uf[n / 8]               = size * (((n / 4) % 2)? 1 : -1);
      uf[((n / 8) + 1) % 3]   = size * (((n / 2) % 2)? 1 : -1);
      uf[((n / 8) + 2) % 3]   = size * ((((n + 1) / 2) % 2)? 1 : -1);

      std::memcpy(data_p, uc, 12);
      data_p += 12;
    }

    //normal stream
    ui[0] = 1;
    ui[1] = 6;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    for(uint32_t n = 0; n < num_vertices; n += 4)
    {
      us[0] = 0;
      us[1] = 0;
      us[2] = 0;

      us[n / 8] = 0x7fff * (((n / 4) % 2)? 1 : -1);

      //it loops over four vertices at a time
      std::memcpy(data_p, uc, 6);
      data_p += 6;
      std::memcpy(data_p, uc, 6);
      data_p += 6;
      std::memcpy(data_p, uc, 6);
      data_p += 6;
      std::memcpy(data_p, uc, 6);
      data_p += 6;
    }

    //texcoord stream
    ui[0] = 6;
    ui[1] = 8;
    std::memcpy(data_p, uc, 8);
    data_p += 8;

    for(uint32_t n = 0; n < num_vertices; ++n)
    {
      uf[0]   = ((n / 2) % 2)? 0. : 1.;
      uf[1]   = (((n + 1) / 2) % 2)? 0. : 1.;

      std::memcpy(data_p, uc, 8);
      data_p += 8;
    }

    //indices
    ui[0] = num_indices;
    std::memcpy(data_p, uc, 4);
    data_p += 4;

    for(uint32_t n = 0; n < num_indices / 6; ++n)
    {
      ui[0] = n * 4;
      ui[1] = n * 4 + 1;
      ui[2] = n * 4 + 2;
      std::memcpy(data_p, uc, 12);
      data_p += 12;

      ui[0] = n * 4 + 2;
      ui[1] = n * 4 + 3;
      ui[2] = n * 4;
      std::memcpy(data_p, uc, 12);
      data_p += 12;
    }

    //morph targets
    ui[0] = 0;
    std::memcpy(data_p, uc, 4);
    data_p += 4;

    ///data generated

    h3dUnloadResource(r);
    h3dLoadResource(r, temp_data.get(), total_size);

    data_p = nullptr;

    return std::make_pair(num_vertices, num_indices);
  }

  ///general functions
  void registerAll()
  {
    int l = 0;
    int n = 0;
    int type = 0;
    int flags = 0;
    l = _resources[n].length();
    while(l != 0)
    {
      if(l >= 2 && _resources[n][0] == '-')
      {
        //switch type
        switch(_resources[n][1])
        {
                 case 'g':
          type = H3DResTypes::Geometry; flags = 0;
          break; case 'm':
          type = H3DResTypes::Material; flags = 0;
          break; case 'c':
          type = H3DResTypes::Code; flags = 0;
          break; case 's':
          type = H3DResTypes::Shader; flags = 0;
          break; case 't':
          type = H3DResTypes::Texture;
          flags = H3DResFlags::NoTexMipmaps;
          break; case 'p':
          type = H3DResTypes::Pipeline; flags = 0;
          break;
          default: break;
        }
      }

      else
      {
        h3dAddResource(type, _resources[n].c_str(), flags);
      }

      ++n;
      l = _resources[n].length();
    }
  }

  int load(int t, const char* name, bool overwrite)
  {
    H3DRes res = h3dFindResource(t, name);
    if(res == 0) return 2;

    if(h3dIsResLoaded(res))
    {
      if(overwrite)
        h3dUnloadResource(res);
      else
        return 0;
    }

    if(name[0] == '_')
    {
      h3dLoadResource(res, nullptr, 0);
      return 0;
    }

    std::string path;
    std::FILE* f;
    char* data = nullptr;
    int size = 0;

    path = "";
    path += AppCtrl::app_path;
    path += _res_directories[h3dGetResType(res)];
    path += name;

    f = fopen(path.c_str(), "rb");
    if(f == nullptr)
    {
      std::printf("Unable to open file: %s.\n", path.c_str());
      h3dLoadResource(res, nullptr, 0);
      return 1;
    }

    std::fseek(f, 0, SEEK_END);
    size = (int)std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    data = new char[size];
    std::fread(data, 1, size, f);

    //printf("%i\n", size);

    if(!h3dLoadResource(res, data, size))
    {
      h3dUnloadResource(res);
      h3dLoadResource(res, nullptr, 0);
    }

    std::fclose(f);
    delete[] data;

    return 0;
  }

  int loadAll()
  {
    H3DRes res;
    std::string path;
    std::FILE* f;
    char* data = nullptr;
    int size = 0;

    for(res = h3dQueryUnloadedResource(0); res != 0;
        res = h3dQueryUnloadedResource(0))
    {
      if(h3dGetResName(res)[0] == '_')
      {
        h3dLoadResource(res, nullptr, 0);
        AppCtrl::dumpMessages();
        continue;
      }

      path = "";
      path += AppCtrl::app_path;
      path += _res_directories[h3dGetResType(res)];
      path += h3dGetResName(res);

      f = fopen(path.c_str(), "rb");
      if(f == nullptr)
      {
        std::printf("Unable to open file: %s.\n", path.c_str());
        h3dLoadResource(res, nullptr, 0);
        return 1;
      }

      std::fseek(f, 0, SEEK_END);
      size = (int)std::ftell(f);
      std::fseek(f, 0, SEEK_SET);

      data = new char[size];
      std::fread(data, 1, size, f);

      //printf("%i\n", size);

      if(!h3dLoadResource(res, data, size))
      {
        h3dUnloadResource(res);
        h3dLoadResource(res, nullptr, 0);
      }

      std::fclose(f);
      delete[] data;

      AppCtrl::dumpMessages();
    }

    Interface::loadFiles();

    AppCtrl::dumpMessages();

    return 0;
  }

  void unloadAll()
  {
    if(!(AppCtrl::flags & AppCtrl::H3DINIT_FLAG)) return;

    H3DRes r = 0;

    while(true)
    {
      r = h3dGetNextResource(H3DResTypes::Undefined, r);

      if(r == 0)
        break;
      else if(h3dIsResLoaded(r))
        h3dUnloadResource(r);
    }

    return;
  }


  void loadEmptyTexture(H3DRes res, uint16_t x, uint16_t y, uint32_t set)
  {
    if(h3dIsResLoaded(res))
      h3dUnloadResource(res);

    char *data, *d_ref;
    uint32_t size = x * y;

    data = new char[size * 4 + 54];
    d_ref = data;

    union
    {
      char ub[4];
      uint32_t ul;
      uint16_t us[2];
    };

    //bmp header
    strncpy(ub, "BM", 2);   //magic number
    memcpy(d_ref, ub, 2); d_ref += 2;

    ul = size * 4 + 54;     //file size
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 0;                 //unused
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 54;                //offset of pixel array
    memcpy(d_ref, ub, 4); d_ref += 4;

    //dib header
    ul = 40;                //size of dib header
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = x;                 //image width
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = y;                 //image height
    memcpy(d_ref, ub, 4); d_ref += 4;
    us[0] = 1; us[1] = 32;  //number of color planes/color depth
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 0;                 //pixel array compression used
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = size * 4;          //size of pixel array
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 1000;              //pixels/meter
    memcpy(d_ref, ub, 4); d_ref += 4;
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 0;                 //palette size
    memcpy(d_ref, ub, 4); d_ref += 4;
    memcpy(d_ref, ub, 4); d_ref += 4;

    //pixel array
    ul = set;

    for(uint32_t n = 0; n < size; ++n)
    {
      memcpy(d_ref, ub, 4);
      d_ref += 4;
    }

    h3dLoadResource(res, data, size * 4 + 54);

    delete[] data;

    return;
  }


  void writeBWBitmap(uint16_t w, uint16_t h, const char* data)
  {
    char *d_ref;
    uint32_t size = w * h;

    uint32_t _buff_size = size * 4 + 54;

    char *buffer = new char[_buff_size];
    d_ref = buffer;

    union
    {
      char ub[4];
      uint16_t us[2];
      uint32_t ul;
    };

    //bmp header
    strncpy(ub, "BM", 2);   //magic number
    memcpy(d_ref, ub, 2); d_ref += 2;

    ul = size * 4 + 54;     //file size
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 0;                 //unused
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 54;                //offset of pixel array
    memcpy(d_ref, ub, 4); d_ref += 4;

    //dib header
    ul = 40;                //size of dib header
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = w;                 //image width
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = h;                 //image height
    memcpy(d_ref, ub, 4); d_ref += 4;
    us[0] = 1; us[1] = 32;  //number of color planes/color depth
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 0;                 //pixel array compression used
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = size * 4;          //size of pixel array
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 1000;              //pixels/meter
    memcpy(d_ref, ub, 4); d_ref += 4;
    memcpy(d_ref, ub, 4); d_ref += 4;
    ul = 0;                 //palette size
    memcpy(d_ref, ub, 4); d_ref += 4;
    memcpy(d_ref, ub, 4); d_ref += 4;

    //pixel array

    ub[3] = 255;

    for(uint32_t n = 0; n < size; ++n)
    {
      ub[0] = ub[1] = ub[2] = data[n];
      memcpy(d_ref, ub, 4);
      d_ref += 4;
    }

    std::string path = AppCtrl::app_path;
    path += "colmapout.bmp";
    FILE* file = fopen(path.c_str(), "wb");

    if(file == nullptr)
      printf("Unable to write file \"%s\"\n", path.c_str());
    else
    {
      fwrite(buffer, 1, _buff_size, file);
      fclose(file);
    }

    delete[] buffer;
  }

  void create2DMesh(H3DRes res,
                    std::vector<std::pair<float, float>>& vertices,
                    std::vector<int>& indices,
                    int& vert_n, int& idx_n)
  {
    assert(indices.size() % 3 == 0);

    int res_size = 44 + indices.size() * 22;

    char* buffer = new char[res_size];
    char* writer = buffer;

    union
    {
      float   uf[3];
      int     ui[3];
      short   us[6];
      char    uc[12];
    };

    //header + num_joints
    strcpy(uc, "H3DG");
    ui[1] = 5;
    ui[2] = 0;
    memcpy(writer, uc, 12);
    writer += 12;

    //vertex streams & count
    ui[0] = 2;
    ui[1] = indices.size();
    memcpy(writer, uc, 8);
    writer += 8;

    //vertex positions
    ui[0] = 0;
    ui[1] = 12;
    memcpy(writer, uc, 8);
    writer += 8;

    for(auto idx: indices)
    {
      uf[0] = vertices[idx].first;
      uf[2] = vertices[idx].second;
      uf[1] = 0.;
      memcpy(writer, uc, 12);
      writer += 12;
    }

    //vertex normals
    ui[0] = 1;
    ui[1] = 6;
    memcpy(writer, uc, 8);
    writer += 8;

    for(unsigned n = 0; n < indices.size(); n += 3)
    {
      us[0] = 0x7fff;
      us[1] = 0x7fff;
      us[2] = 0;
      memcpy(writer, uc, 6);
      writer += 6;

      us[0] = 0;
      us[1] = 0x7fff;
      us[2] = 0x7fff;
      memcpy(writer, uc, 6);
      writer += 6;

      us[0] = 0x7fff;
      us[1] = 0;
      us[2] = 0x7fff;
      memcpy(writer, uc, 6);
      writer += 6;
    }

    //indices
    ui[0] = indices.size();
    memcpy(writer, uc, 4);
    writer += 4;

    for(unsigned n = 0; n < indices.size(); ++n)
    {
      ui[0] = n;
      memcpy(writer, uc, 4);
      writer += 4;
    }

    //morph targets
    ui[0] = 0;
    memcpy(writer, uc, 4);
    writer += 4;

    h3dLoadResource(res, buffer, res_size);

    vert_n = indices.size();
    idx_n = indices.size();

    delete[] buffer;
  }

}
