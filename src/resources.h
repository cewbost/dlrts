#ifndef RESOURCES_H_INCLUDED
#define RESOURCES_H_INCLUDED

#include <stdint.h>
#include <vector>
#include <utility>

#include <horde3d.h>

#ifdef RESOURCES_CPP

namespace
{
  const std::string _resources[] =
  {
    "-g",
    "_hmaptile_oh",
    "_hmaptile",
    "_flattile_oh",
    "_flattile",
    "_cube",
    "_navmesh",
    "-m",
    "overlay.xml",
    "font_mat.xml",
    "water.xml",
    "h_map_general.xml",
    "h_map.xml",
    "h_map_top.xml",
    "h_map_overlay.xml",
    "h_map_overlay_1.xml",
    "h_map_overlay_2.xml",
    "unit_hilite.xml",
    "model.xml",
    "dragbox.xml",
    "navmesh.xml",
    "deferred.xml",
    "-c",
    "h_map.glsl",
    "-s",
    "water.shader",
    "gui.shader",
    "text.shader",
    "model.shader",
    "hmap_overlay.shader",
    "dragbox.shader",
    "navmesh.shader",
    "deferred.shader",
    "terrain.shader",
    "-t",
    "_heightmap",
    "_visionmap",
    "tile_hilight.png",
    "click_hilight.png",
    "cobblestone.png",
    "ground.png",
    "cobblestone_nmap.png",
    "grass_nmap.png",
    "waves_nmap.png",
    "gui.png",
    "font.png",
    "-p",
    "general_p.xml",
    "\0"
  };

  const std::string _res_directories[10] =
  {
    "\0",
    "\0",
    "content/geometry/",
    "\0",
    "content/materials/",
    "content/shaders/common/",
    "content/shaders/",
    "content/textures/",
    "\0",
    "content/pipelines/"
  };
}

#endif // RESOURCES_CPP

namespace Resources
{
  void registerAll();

  int load(int t, const char* name, bool overwrite = true);
  //void generateProcResources();

  int loadAll();
  void unloadAll();

  //procedural generation functions
  std::pair<int, int> generateGrid(H3DRes, double, int);
  std::pair<int, int> generateGridOH(H3DRes, double, int);
  std::pair<int, int> generateCube(H3DRes, double);

  //extra functions
    //arguments: (resource handle, width, height, initial value)
  void loadEmptyTexture(H3DRes, uint16_t, uint16_t, uint32_t = 0);
  void writeBWBitmap(uint16_t, uint16_t, const char*);
  void create2DMesh(H3DRes, std::vector<std::pair<float, float>>&,
                    std::vector<int>&, int&, int&);
}

#endif // RESOURCES_H_INCLUDED
