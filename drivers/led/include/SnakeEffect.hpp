#pragma once

#include "bitmap.hpp"
#include "ColorGRBa.hpp"
#include <cstdint>
#include <vector>

namespace ChristmasClock {

class LED;

// Autoplaying "Snake", confined to the physically lit LEDs (same constraint as MatrixEffect -
// the board is not a dense matrix, only the seven-segment strokes are populated). Each digit's
// 7 segments (35 pixels) form one connected graph shaped exactly like the digit's own outline
// plus its middle bar (the 6 outer segments form a loop, the middle segment bridges into both
// sides of it) - one independent, randomly self-steering snake runs on each of the 4 digits at
// once. A snake eats a randomly placed food pixel, grows by one pixel per pixel eaten, and its
// head color cycles through a palette over time, so the body leaves a shifting-color trail.
class SnakeEffect {
public:
    SnakeEffect(LED& led);

    // Call periodically (e.g. once per ~200ms tick) to advance and redraw all 4 snakes.
    void Update();

private:
    struct Node {
        uint8_t x;
        uint8_t y;
        std::vector<uint8_t> neighbors;
    };

    struct Snake {
        std::vector<uint8_t> body;      // node indices into _graph, front = head
        std::vector<ColorGRBa> colors;  // one color per body entry, same order as body
        int food;                        // node index of the current food pixel
        int hue;                         // drives NextColor()
    };

    LED& _led;
    Bitmap _bmp;

    std::vector<Node> _graph;
    Snake _snakes[4];

    static const uint8_t _offsets[4];

    void BuildGraph();
    void AddEdge(int a, int b);
    bool IsInBody(const Snake& snake, int node) const;

    ColorGRBa NextColor(int& hue);
    void Respawn(Snake& snake);
    void PlaceFood(Snake& snake);
    void Step(Snake& snake);
    void Draw(const Snake& snake, int offsetX);
};
}
