#include "test_map.h"

#define _ 0
uint8_t test_map_walls[] =
{
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,7,7,7,7,7,7,7,
  4,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,7,_,_,_,_,_,_,7,
  4,_,1,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,7,
  4,_,2,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,7,
  4,_,3,_,_,_,_,_,_,_,_,_,_,_,_,_,7,_,_,_,_,_,_,7,
  4,_,4,_,_,_,_,5,5,5,5,5,5,5,5,5,7,7,_,7,7,7,7,7,
  4,_,5,_,_,_,_,5,_,5,_,5,_,5,_,5,7,_,_,_,7,7,7,1,
  4,_,6,_,_,_,_,5,_,_,_,_,_,_,_,5,7,_,_,_,_,_,_,8,
  4,_,7,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,7,7,7,1,
  4,_,8,_,_,_,_,5,_,_,_,_,_,_,_,5,7,_,_,_,_,_,_,8,
  4,_,_,_,_,_,_,5,_,_,_,_,_,_,_,5,7,_,_,_,7,7,7,1,
  4,_,_,_,_,_,_,5,5,5,5,_,5,5,5,5,7,7,7,7,7,7,7,1,
  6,6,6,6,6,6,6,6,6,6,6,_,6,6,6,6,6,6,6,6,6,6,6,6,
  8,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,4,
  6,6,6,6,6,6,_,6,6,6,6,_,6,6,6,6,6,6,6,6,6,6,6,6,
  4,4,4,4,4,4,_,4,4,4,6,_,6,2,2,2,2,2,2,2,3,3,3,3,
  4,_,_,_,_,_,_,_,_,4,6,_,6,2,_,_,_,_,_,2,_,_,_,2,
  4,_,_,_,_,_,_,_,_,_,_,_,6,2,_,_,5,_,_,2,_,_,_,2,
  4,_,_,_,_,_,_,_,_,4,6,_,6,2,_,_,_,_,_,2,2,_,2,2,
  4,_,6,_,6,_,_,_,_,4,6,_,_,_,_,_,5,_,_,_,_,_,_,2,
  4,_,_,5,_,_,_,_,_,4,6,_,6,2,_,_,_,_,_,2,2,_,2,2,
  4,_,6,_,6,_,_,_,_,4,6,_,6,2,_,_,5,_,_,2,_,_,_,2,
  4,_,_,_,_,_,_,_,_,4,6,_,6,2,_,_,_,_,_,2,_,_,_,2,
  4,4,4,4,4,4,4,4,4,4,1,1,1,2,2,2,2,2,2,3,3,3,3,3
};
#undef _

map_t test_map = {
    .map_width = 24,
    .map_height = 24,
    .wall_map = test_map_walls,
};

map_t* get_test_map()
{
    return &test_map;
}