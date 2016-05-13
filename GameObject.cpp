#include "stdafx.h"
#include "GameObject.h"
#include "Cell.h"

class SearchState : public Pos, public AStarState<SearchState, int>
{
    enum Mode
    {
        Mode_walker,
        Mode_walkerCost,
        Mode_fly
    };
    int m_goalDis;
    CellQuery* m_query;
    int m_mode;
public:
    SearchState(): m_goalDis(0), m_query(NULL), m_mode(Mode_walker){}
    SearchState(const Pos& pos, int goalDis, int mode, CellQuery* query)
        : Pos(pos)
        , m_goalDis(goalDis)
        , m_mode(mode)
        , m_query(query)
    {}

    int GoalDistanceEstimate(SearchState& nodeGoal)
    {
        return Distance(nodeGoal);
    }
    bool IsGoal(SearchState& nodeGoal)
    {
        return Distance(nodeGoal) <= m_goalDis;
    }
    bool GetSuccessors(AStarSearch<SearchState, int>* astarsearch, SearchState* parent_node)
    {
        std::vector<Pos> vec;
        GetNearest(vec, 1);
        for (auto it = vec.begin(); it != vec.end();++it)
        {
            astarsearch->AddSuccessor(SearchState(*it, m_goalDis, m_mode, m_query));
        }
        return true;
    }
    int GetCost(SearchState& successor)
    {
        if (m_mode == Mode_fly)
            return 1;
        else if (m_mode == Mode_walker)
            return m_query->at(x, y).m_cost > 1 ? 999 : 1;
        else
            return m_query->at(x, y).m_cost;
    }
    bool IsSameState(SearchState& rhs) { return rhs.x == x&&rhs.y == y; }
};


void GObjectQuery::Insert(const GObjectPtr & ptr)
{
    m_cont[ptr->GetType()].insert(ptr);
}

void GObjectQuery::Remove(const GObjectPtr & ptr)
{
    m_cont[ptr->GetType()].erase(ptr);
}

GObject::GObject(const Pos& pos, int kind, char camp, CellQuery* cellQuery, GObjectQuery* objQuery)
    : m_lastPos(pos)
    , m_curPos(pos)
    , m_camp(camp)
    , m_cellQuery(cellQuery)
    , m_objQuery(objQuery)
{
    static ISyncEvent dummy;
    m_evthd = &dummy;
    m_shape = 0;
    m_camp = 0;
    m_weight = 1;
    m_radius = 1;
    m_range = 1;
    m_fullhealth = 100;
    m_speed = 20;
    m_fly = false;
    m_type = GObjectType_walker;
    m_id = 0;
    m_lastPos.x = 0;
    m_lastPos.y = 0;
    m_curPos.x = 0;
    m_curPos.y = 0;
    m_health = 100;
    m_pathNode = 0;
    m_pathNodeFrame = 0;
    m_objQuery = NULL;
}

BVState GObject::SearchTarget(int type, char radius)
{
    std::vector<Pos> vec;
    m_curPos.GetNearest(vec, radius);
    GObjectPtr nearestobj;
    int mindis = 100;
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        auto cell = m_cellQuery->at(it->x, it->y);
        GObjectPtr groundCur(cell.m_groundCur);
        if (groundCur
            && groundCur->m_camp!= m_camp
            && groundCur->GetType()&type)
        {
            int dis = m_curPos.Distance(groundCur->m_curPos) - radius - groundCur->m_radius;
            if (dis<mindis)
            {
                mindis = dis;
                nearestobj = groundCur;
            }
        }
        GObjectPtr skyCur(cell.m_skyCur);
        if (skyCur
            && skyCur->m_camp != m_camp
            && skyCur->GetType()&type)
        {
            int dis = m_curPos.Distance(skyCur->m_curPos) - radius - skyCur->m_radius;
            if (dis < mindis)
            {
                mindis = dis;
                nearestobj = skyCur;
            }
        }
    }
    if (nearestobj)
    {
        m_target = nearestobj;
        m_targetPos = nearestobj->m_curPos;
        return BVState_succ;
    }
    else
    {
        m_target.reset();
        return BVState_fail;
    }
}

BVState GObject::SearchTower()
{
    GObjectPtr nearestobj;
    int mindis = 100;

    const auto& objs = m_objQuery->Get(GObjectType_tower);
    for (auto it = objs.begin(); it != objs.end(); ++it)
    {
        const GObjectPtr& obj = *it;
        if (obj->m_camp == m_camp)
            continue;
        int dis = m_curPos.Distance(obj->m_curPos);
        if (dis < mindis)
        {
            mindis = dis;
            nearestobj = obj;
        }
    }
    if (nearestobj)
    {
        m_target = nearestobj;
        m_targetPos = nearestobj->m_curPos;
        return BVState_succ;
    }
    else
    {
        m_target.reset();
        return BVState_fail;
    }
}


BVState GObject::FindPath(int mode)
{
    GObjectPtr obj(m_target);
    if (!obj)
        return BVState_fail;

    int goalDis = m_radius + obj->m_radius - 1;

    AStarSearch<SearchState, int> astarsearch;

    SearchState nodeStart(m_curPos, goalDis, mode, m_cellQuery);
    SearchState nodeEnd(m_targetPos, goalDis, mode, m_cellQuery);
    astarsearch.SetStartAndGoalStates(nodeStart, nodeEnd);
    unsigned int state;
    unsigned int SearchSteps = 0;
    do
    {
        state = astarsearch.SearchStep();
        SearchSteps++;
        if (SearchSteps > 100)
            break;
    } while (state == AStarSearch<SearchState>::SEARCH_STATE_SEARCHING);

    if (state == AStarSearch<SearchState>::SEARCH_STATE_SUCCEEDED)
    {
        SearchState* node = astarsearch.GetSolutionStart();
        m_path.clear();
        for (;;)
        {
            m_path.push_back(*node);
            node = astarsearch.GetSolutionNext();
            if (!node)
                break;
        };
        astarsearch.FreeSolutionNodes();
        return BVState_succ;
    }
    return BVState_fail;
}


bool GObject::CompareWeight(GObjectWPtr wobj)
{
    GObjectPtr obj(wobj);
    if (!obj)
        return false;
    if (obj.get() == this)
        return false;
    return obj->m_weight >= m_weight ? true : false;
}

void GObject::SetEvtHandle(ISyncEvent* hd)
{
    m_evthd = hd;
}

void GObject::Update(unsigned int dt)
{
    if (m_bv)
        m_bv->Update(this, dt);
}

BVState GObject::AttackAni()
{
    m_evthd->OnAttack();
    return BVState_succ;
}

BVState GObject::Damage(int val)
{
    GObjectPtr target(m_target);
    if (!target)
        return BVState_fail;
    target->BeHurt(val);
    return BVState_succ;
}

void GObject::BeHurt(int val)
{
    m_health-=val;
    if (m_health<0)
    {
        m_evthd->OnDead();
        ClearSelf();
    }
}

void GObject::ClearSelf()
{
    m_objQuery->Remove(shared_from_this());
}

BVState Walker::Move()
{
    if (m_pathNode >= m_path.size())
        return BVState_succ;
    const Pos& next = m_path[m_pathNode];
    Cell& cell = m_cellQuery->at(next);
    if (CompareWeight(cell.m_groundCur)
        || CompareWeight(cell.m_groundIn))
    {
        m_pathNodeFrame = 0;
        m_evthd->OnKickTo(m_curPos.x, m_curPos.y);
        return BVState_fail;
    }
    if (++m_pathNodeFrame < m_speed)
        return BVState_running;

    m_cellQuery->at(m_curPos).m_groundCur.reset();
    m_curPos = next;

    ++m_pathNode;
    m_pathNodeFrame = 0;

    GObjectPtr self = shared_from_this();
    cell.m_groundCur = self;
    cell.m_groundIn.reset();
    if (m_pathNode < m_path.size())
    {
        const Pos& p = m_path[m_pathNode];
        Cell& n = m_cellQuery->at(p);
        if (!CompareWeight(n.m_groundIn))
            n.m_groundIn = self;
        m_evthd->OnMoveTo(p.x, p.y);
    }
    return BVState_succ;
}

BVState Flyer::Move()
{
    if (m_pathNode >= m_path.size())
        return BVState_succ;
    const Pos& next = m_path[m_pathNode];
    Cell& cell = m_cellQuery->at(next);
    if (CompareWeight(cell.m_skyCur)
        || CompareWeight(cell.m_skyIn))
    {
        m_pathNodeFrame = 0;
        m_evthd->OnKickTo(m_curPos.x, m_curPos.y);
        return BVState_fail;
    }
    if (++m_pathNodeFrame < m_speed)
        return BVState_running;

    m_cellQuery->at(m_curPos).m_skyCur.reset();
    m_curPos = next;

    ++m_pathNode;
    m_pathNodeFrame = 0;

    GObjectPtr self = shared_from_this();
    cell.m_skyCur = self;// todo: 当前格子的要被挤开
    cell.m_skyIn.reset();
    if (m_pathNode < m_path.size())
    {
        const Pos& p = m_path[m_pathNode];
        Cell& n = m_cellQuery->at(p);
        if (!CompareWeight(n.m_skyIn))
            n.m_skyIn = self;
        m_evthd->OnMoveTo(p.x, p.y);
    }
    return BVState_succ;
}

BVState Bullet::Move()
{
    if (m_pathNode >= m_path.size())
        return BVState_succ;
    if (++m_pathNodeFrame < m_speed)
        return BVState_running;
    ++m_pathNode;
    m_pathNodeFrame = 0;
    return BVState_succ;
}
