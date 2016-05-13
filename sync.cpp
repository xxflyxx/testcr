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
    AddObj(ObjKind_guowang);
    return true;
}

void Sync::Release()
{
    delete this;
}

void Sync::Update(unsigned int dt)
{
    for (int i=GObjectType_walker;i<=GObjectType_bullet;i = i<<1)
    {
        const auto& objs = m_objs.Get(i);
        for (auto it=objs.begin();it!=objs.end();++it)
        {
            (*it)->Update(dt);
        }
    }
}

bool Sync::AddObj(int kind, char x, char y, char camp)
{
    GObjectPtr obj;
    if (kind < ObjKind_ground_end)
         obj.reset(new Walker(Pos(x,y), kind, camp, &m_cellQuery, &m_objs));
    else if (kind < ObjKind_flyer_end)
        obj.reset(new Flyer(Pos(x,y), kind, camp, &m_cellQuery, &m_objs));
    else if (kind < ObjKind_bullet_end)
        obj.reset(new Flyer(Pos(x,y), kind, camp, &m_cellQuery, &m_objs));
    m_objs.Insert(obj);
    m_cb->OnNewObject(obj.get());

    return true;
}
