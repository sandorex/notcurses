#include "demo.h"

// planes as of unicode 13.0:
//  0: BMP
//  1: SMP
//  2: SIP
//  3: TIP
// 14: SSPP
// 15: SPUAP
// 16: SPUBP

// works on a scrolling plane
static int
allglyphs(struct notcurses* nc, struct ncplane* column){
  const int PERIOD = 4;
  const int valid_planes[] = {
    0, 1, 2, 3, 14, 15, 16, -1
  };
  int period = 0;
  for(const int* plane = valid_planes ; *plane >= 0 ; ++plane){
    for(long int c = 0 ; c < 0x10000l ; ++c){
      wchar_t w[2] = { *plane * 0x10000l + c, L'\0' };
      if(wcswidth(w, UINT_MAX) >= 1){
        if(ncplane_putwegc(column, w, NULL) < 0){
          return -1;
        }
        if(++period % PERIOD == 0){
          DEMO_RENDER(nc);
          period = 0;
        }
      }
    }
  }
  DEMO_RENDER(nc);
  return 0;
}

// render all the glyphs worth rendering
int allglyphs_demo(struct notcurses* nc){
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_erase(n);
  int width = 40;
  if(width > dimx - 8){
    if((width = dimx - 8) <= 0){
      return -1;
    }
  }
  int height = 40;
  if(height > dimy - 3){
    if((height = dimy - 3) <= 0){
      return -1;
    }
  }
  struct ncplane* column = ncplane_new(nc, height, width,
                                       (dimy - height) / 2 + 1,
                                       (dimx - width) / 2, NULL);
  if(column == NULL){
    return -1;
  }
  ncplane_set_scrolling(column, true);
  int r = allglyphs(nc, column);
  ncplane_destroy(column);
  return r;
}
