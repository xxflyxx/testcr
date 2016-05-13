#pragma once

#ifdef SYNC_EXPORTS
#define SYNC_API __declspec(dllexport)
#else
#define SYNC_API //__declspec(dllimport)
#endif


class ISyncEvent
{
public:
    virtual ~ISyncEvent() {}
    virtual void OnMoveTo(char x, char y) {}
    virtual void OnKickTo(char x, char y) {}
    virtual void OnAttack() {}
    virtual void OnDead() {}
};

class ISyncObjQuery
{
protected:
    virtual ~ISyncObjQuery() {}
public:
    virtual void SetEvtHandle(ISyncEvent* hd) = 0;
    virtual unsigned int Id() const = 0;
    virtual int GetType() const = 0;
};

class ISyncCB
{
public:
    virtual ~ISyncCB() {}
    virtual void OnNewObject(const ISyncObjQuery* query) {}
};

class ISync
{
protected:
    virtual ~ISync() { }
public:
    SYNC_API static ISync* Create(ISyncCB* cb);
    virtual bool Init() = 0;
    virtual void Release() = 0;
    virtual void Update(unsigned int dt) = 0;

    virtual bool AddObj(int kind, char x, char y, char camp) = 0; //for test
};


