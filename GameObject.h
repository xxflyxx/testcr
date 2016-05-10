#pragma once
#include "Pos.h"
#include "Cell.h"
#include "behaviacTree.h"
#include <set>
#include <boost/enable_shared_from_this.hpp>
#include "ISync.h"

class GObjectQuery
{
    typedef std::set<GObjectPtr> Objs;
    typedef std::map<int, Objs> ObjMap;

    ObjMap m_cont;
public:
    void Insert(const GObjectPtr& ptr);
    void Remove(const GObjectPtr& ptr);
    const Objs& Get(int type) { return m_cont[type]; }
};

class GObject : public boost::enable_shared_from_this<GObject>, public ISyncObjQuery
{
    ISyncEvent* m_evthd;
public:
    uint32_t m_shape; // 外形
    char m_camp; // 阵营
    char m_weight; // 体重
    char m_radius; // 半径
    char m_range; // 攻击范围
    uint32_t m_fullhealth;
    uint32_t m_speed;
    bool m_fly;	// 空中单位
    int m_type; // 生物类型 GameObjectType
    uint32_t m_id;
    Pos m_lastPos;
    Pos m_curPos;
    uint32_t m_health;
    GObjectWPtr m_target; // 目标
    Pos m_targetPos;
    std::vector<Pos> m_path; // 路径
    size_t m_pathNode;
    uint32_t m_pathNodeFrame;
    CellQuery* m_cellQuery;
    GObjectQuery* m_objQuery;

    BVState SearchTarget(int type, char radius);
    BVState SearchTower();
    BVState FindPath(int mode);

    BVState AttackAni();
    BVState Damage(uint32_t val);

    bool CompareWeight(GObjectPtr obj);
public:
    GObject();
    uint32_t Id() const { return m_id; }
    int GetType() const { return m_type; }
    void SetEvtHandle(ISyncEvent* hd); 
};

class Walker : public GObject
{
    BVState Move();
public:
protected:
private:
};

class Flyer : public GObject
{
    BVState Move();
public:
protected:
private:
};

class Bullet : public GObject
{
    BVState Move();
public:
protected:
private:
};