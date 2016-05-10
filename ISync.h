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
};

class ISyncObjQuery
{
protected:
    virtual ~ISyncObjQuery() {}
public:
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
    virtual bool Bind(unsigned int oid, ISyncEvent* hd) = 0;

    virtual bool AddObj(int type, char x, char y) = 0; //for test
};


