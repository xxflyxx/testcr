// sync.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "sync.h"

ISync* ISync::Create(ISyncCB* cb)
{
    return new Sync(cb);
}

bool Sync::Init()
{
    auto obj = new GObject();
    m_objs[0].reset(obj);
    m_cb->OnNewObject(obj);
    return true;
}

void Sync::Release()
{
}

void Sync::Update(unsigned int dt)
{
}

bool Sync::Bind(unsigned int oid, ISyncEvent * hd)
{
    auto it = m_objs.find(oid);
    if (it != m_objs.end())
    {
        it->second->SetEvtHandle(hd);
        return true;
    }
    return false;
}

bool Sync::AddObj(int type, char x, char y)
{
    return true;
}
