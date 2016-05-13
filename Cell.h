#pragma once

#include "typedef.h"
#include "Pos.h"
#include "stlastar.h"

struct Cell
{
    char m_cost;
    GObjectWPtr m_groundIn;
    GObjectWPtr m_groundCur;
    GObjectWPtr m_skyIn;
    GObjectWPtr m_skyCur;


};

class CellQuery
{
    typedef std::vector<Cell> CellY;
    typedef std::vector<CellY> CellX;

    CellX m_cells;

public:
    CellQuery(char w, char h)
    {
        m_cells.resize(w);
        for (auto it = m_cells.begin(); it != m_cells.end(); ++it)
            it->resize(h);
    }

    Cell& at(char x, char y)
    {
        return m_cells[x][y];
    }

    Cell& at(Pos pos)
    {
        return m_cells[pos.x][pos.y];
    }
};

