#include "SnakeEffect.hpp"
#include "led.hpp"

#include "pico/time.h"
#include <algorithm>
#include <cstdlib>

namespace ChristmasClock {

const uint8_t SnakeEffect::_offsets[4] = { 0, 8, 18, 26 };

namespace {
    const uint8_t seg0X[5] = { 1, 2, 3, 4, 5 };
    const uint8_t seg0Y[5] = { 0, 0, 0, 0, 0 };
    const uint8_t seg1X[5] = { 6, 6, 6, 6, 6 };
    const uint8_t seg1Y[5] = { 1, 2, 3, 4, 5 };
    const uint8_t seg2X[5] = { 6, 6, 6, 6, 6 };
    const uint8_t seg2Y[5] = { 7, 8, 9, 10, 11 };
    const uint8_t seg3X[5] = { 1, 2, 3, 4, 5 };
    const uint8_t seg3Y[5] = { 12, 12, 12, 12, 12 };
    const uint8_t seg4X[5] = { 0, 0, 0, 0, 0 };
    const uint8_t seg4Y[5] = { 7, 8, 9, 10, 11 };
    const uint8_t seg5X[5] = { 0, 0, 0, 0, 0 };
    const uint8_t seg5Y[5] = { 1, 2, 3, 4, 5 };
    const uint8_t seg6X[5] = { 1, 2, 3, 4, 5 };
    const uint8_t seg6Y[5] = { 6, 6, 6, 6, 6 };

    const uint8_t* segX[7] = { seg0X, seg1X, seg2X, seg3X, seg4X, seg5X, seg6X };
    const uint8_t* segY[7] = { seg0Y, seg1Y, seg2Y, seg3Y, seg4Y, seg5Y, seg6Y };

    const ColorGRBa PALETTE[6] = {
        ColorGRBa::RED, ColorGRBa::ORANGE, ColorGRBa::YELLOW,
        ColorGRBa::GREEN, ColorGRBa::CYAN, ColorGRBa::MAGENTA
    };
}

SnakeEffect::SnakeEffect(LED& led) :
    _led(led),
    _bmp(LED::SCREEN_WIDTH, LED::SCREEN_HIGHT)
{
    srand(static_cast<unsigned int>(time_us_64()));

    BuildGraph();

    for(auto& snake : _snakes){
        snake.hue = rand();
        Respawn(snake);
    }
}

void SnakeEffect::AddEdge(int a, int b){
    _graph[a].neighbors.push_back(b);
    _graph[b].neighbors.push_back(a);
}

void SnakeEffect::BuildGraph(){
    _graph.resize(35);
    for(int seg = 0; seg < 7; seg++){
        for(int i = 0; i < 5; i++){
            int index = seg *5 +i;
            _graph[index].x = segX[seg][i];
            _graph[index].y = segY[seg][i];
        }
    }

    // intra-segment chains
    for(int seg = 0; seg < 7; seg++){
        int base = seg *5;
        for(int i = 0; i < 4; i++){
            AddEdge(base +i, base +i +1);
        }
    }

    // corner/junction bridges between segments (same shape as the real digit outline)
    AddEdge(0, 25);  // top-left corner    (seg0 start  - seg5 start)
    AddEdge(4, 5);   // top-right corner   (seg0 end    - seg1 start)
    AddEdge(15, 24); // bottom-left corner (seg3 start  - seg4 end)
    AddEdge(19, 14); // bottom-right corner(seg3 end    - seg2 end)
    AddEdge(30, 29); // middle-left  (upper)(seg6 start - seg5 end)
    AddEdge(30, 20); // middle-left  (lower)(seg6 start - seg4 start)
    AddEdge(34, 9);  // middle-right (upper)(seg6 end   - seg1 end)
    AddEdge(34, 10); // middle-right (lower)(seg6 end   - seg2 start)
}

bool SnakeEffect::IsInBody(const Snake& snake, int node) const {
    return std::find(snake.body.begin(), snake.body.end(), (uint8_t)node) != snake.body.end();
}

ColorGRBa SnakeEffect::NextColor(int& hue){
    hue++;
    return PALETTE[(hue /3) % 6];
}

void SnakeEffect::Respawn(Snake& snake){
    snake.body.clear();
    snake.colors.clear();
    int start = rand() % _graph.size();
    snake.body.push_back(start);
    snake.colors.push_back(NextColor(snake.hue));
    PlaceFood(snake);
}

void SnakeEffect::PlaceFood(Snake& snake){
    for(int tries = 0; tries < 100; tries++){
        int candidate = rand() % _graph.size();
        if(!IsInBody(snake, candidate)){
            snake.food = candidate;
            return;
        }
    }
    Respawn(snake);
}

void SnakeEffect::Step(Snake& snake){
    const Node& head = _graph[snake.body.front()];

    std::vector<uint8_t> candidates;
    for(auto n : head.neighbors){
        if(!IsInBody(snake, n)) candidates.push_back(n);
    }

    if(candidates.empty()){
        Respawn(snake);
        return;
    }

    int next = candidates[rand() % candidates.size()];
    bool ate = (next == snake.food);

    snake.body.insert(snake.body.begin(), (uint8_t)next);
    snake.colors.insert(snake.colors.begin(), NextColor(snake.hue));

    if(ate){
        PlaceFood(snake);
    }else{
        snake.body.pop_back();
        snake.colors.pop_back();
    }
}

void SnakeEffect::Draw(const Snake& snake, int offsetX){
    for(size_t i = 0; i < snake.body.size(); i++){
        const Node& node = _graph[snake.body[i]];
        _bmp(node.x +offsetX, node.y) = snake.colors[i];
    }
    const Node& food = _graph[snake.food];
    _bmp(food.x +offsetX, food.y) = ColorGRBa::WHITE;
}

void SnakeEffect::Update(){
    for(int x = 0; x < LED::SCREEN_WIDTH; x++){
        for(int y = 0; y < LED::SCREEN_HIGHT; y++){
            _bmp(x, y) = ColorGRBa::BLACK;
        }
    }

    for(int n = 0; n < 4; n++){
        Step(_snakes[n]);
        Draw(_snakes[n], _offsets[n]);
    }

    _led.Update(_bmp);
}
}
