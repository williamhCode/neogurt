#include <hb-cplusplus.hh>
#include <hb-ft.h>

int main() {
  FT_Library library;
  FT_Init_FreeType(&library);

  hb::unique_ptr<hb_buffer_t> buf{hb_buffer_create()};

}
