#pragma once
#include "ISync.h"
#include <map>
#include "GameObject.h"
#include "Cell.h"

class Sync : public ISync
{
    CellQuery m_cellQuery;
    GObjectQuery m_objs;
    ISyncCB* m_cb;
public:
    Sync(ISyncCB* cb) : m_cb(cb), m_cellQuery(100, 100) {}
    virtual bool Init();
    virtual void Release();
    virtual void Update(unsigned int dt);

    virtual bool AddObj(int kind, char x, char y, char camp);
};
