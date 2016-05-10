#pragma once
#include "ISync.h"
#include <map>
#include "GameObject.h"

class Sync : public ISync
{
    std::map<uint32_t, GObjectPtr> m_objs;
    ISyncCB* m_cb;
public:
    Sync(ISyncCB* cb) : m_cb(cb) {}
    virtual bool Init();
    virtual void Release();
    virtual void Update(unsigned int dt);
    virtual bool Bind(unsigned int oid, ISyncEvent* hd);

    virtual bool AddObj(int type, char x, char y);
};
