#pragma once

#include <vector>

struct Pos
{
    char x;
    char y;
    Pos() : x(0), y(0) {}
    Pos(char _x, char _y) : x(_x), y(_y) {}
    Pos(const Pos& pos) : x(pos.x), y(pos.y) {}
    Pos operator+(const Pos& offset)
    {
        Pos pos;
        pos.x=x+offset.x;
        pos.y=y+offset.y;
        return pos;
    }
    bool operator==(const Pos& dst)
    {
        return x=dst.x&&y==dst.y;
    }

    int Distance(const Pos& dst)
    {
        char dx = dst.x - x;
        char dy = dst.y - y;
        return (abs(dx) + abs(dy) + abs(dx - dy)) / 2;
    }

    // 2,2 range 1 = 1,1 1,2 2,1 2,3, 3,2 3,3 
    void GetNearest(std::vector<Pos> vec, char range)
    {
        if (range == 0)
        {
            vec.push_back(Pos(x,y));
            return;
        }
        // left
        for (char i=x-range,seq=0;i<=x;++i,++seq)
        {
            char yr = y+range+seq;
            for (char j=y-range;j<=yr;++j)
                vec.push_back(Pos(i,j));
        }
        // right
        for (char i=x+1,seq=1;i<=x+range;++i,++seq)
            for (char j=y-range+seq;j<=y+range;++j)
                vec.push_back(Pos(i,j));
    }
};