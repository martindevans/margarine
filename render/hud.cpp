#include "hud.h"

using namespace picosystem;

void draw_battery_indicator()
{
    uint8_t level = battery();

    const int width = 24;
    const int height = 12;
    const int margin_top = 3;
    const int margin_right = 4;

    blend(COPY);
    pen(13, 13, 14);

    rect(240 - width - margin_right, margin_top, width, height);
    rect(240 - margin_right, margin_top + 3, 2, height - 6);

    int w = level / 5;
    frect(240 - width - margin_right + 2, margin_top + 2, w, height - 4);
}