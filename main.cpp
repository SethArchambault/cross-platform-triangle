void draw_arrow(const char * id, V2F32 pos, F32 width, F32 length, F32 turns, V3F32 color) {
    V2F32 p1;
    V2F32 p2;
    V2F32 p3;
    F32 degrees = turns * 360.0f;
    {
        F32 hypotenuse = width / 2;
        F32 radians = degrees * M_PI / 180;
        F32 adjacent = cos(radians) * hypotenuse;
        F32 opposite = sin(radians) * hypotenuse;

        // lower left
        p1 = {pos.x - adjacent, pos.y - opposite};
        // lower right
        p2 = {pos.x + adjacent, pos.y + opposite};
    }
    {
        F32 hypotenuse = length;
        F32 radians = degrees * M_PI / 180;
        F32 adjacent = cos(radians) * hypotenuse;
        F32 opposite = sin(radians) * hypotenuse;
        // upper center
        p3 = {pos.x + opposite, pos.y - adjacent};
    }
    platform_draw_triangle(id, p1, p2, p3, color);
}

S32 game_loop() {
    V3F32 color = {200.0, 0.0, 150.0};
    // TODO change this to a bulletproof array passing system that can be easily validated
    draw_arrow("basic_triangle", {100.0, 200.0}, 50.0, 70.0, 0.25, color);
    return 0;
}
