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

GObject::GObject()
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
    m_cellQuery = NULL;
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
        if (cell.m_groundCur
            && cell.m_groundCur->m_camp!= m_camp
            && cell.m_groundCur->GetType()&type)
        {
            auto obj = cell.m_groundCur;
            int dis = m_curPos.Distance(obj->m_curPos) - radius - obj->m_radius;
            if (dis<mindis)
            {
                mindis = dis;
                nearestobj = obj;
            }
        }
        if (cell.m_skyCur
            && cell.m_groundCur->m_camp != m_camp
            && cell.m_skyCur->GetType()&type)
        {
            auto obj = cell.m_skyCur;
            int dis = m_curPos.Distance(obj->m_curPos) - radius - obj->m_radius;
            if (dis < mindis)
            {
                mindis = dis;
                nearestobj = obj;
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


bool GObject::CompareWeight(GObjectPtr obj)
{
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
        return BVState_fail;
    }
    if (++m_pathNodeFrame < m_speed)
        return BVState_running;
    ++m_pathNode;
    m_pathNodeFrame = 0;

    m_cellQuery->at(m_curPos).m_groundCur.reset();
    m_curPos = next;
    GObjectPtr self = shared_from_this();
    cell.m_groundCur = self;
    cell.m_groundIn.reset();
    if (m_pathNode < m_path.size())
    {
        Cell& n = m_cellQuery->at(m_path[m_pathNode]);
        if (!CompareWeight(n.m_groundIn))
            n.m_groundIn = self;
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
        return BVState_fail;
    }

    GObjectPtr self = shared_from_this();
    if (cell.m_skyIn != self)
    {
        if (cell.m_skyIn->m_weight<m_weight)
            cell.m_skyIn = self;
    }
    if (++m_pathNodeFrame < m_speed)
        return BVState_running;

    ++m_pathNode;
    m_pathNodeFrame = 0;

    m_cellQuery->at(m_curPos).m_groundCur.reset();
    m_curPos = next;
    cell.m_skyCur = self;
    cell.m_skyIn.reset();
    if (m_pathNode < m_path.size())
    {
        Cell& n = m_cellQuery->at(m_path[m_pathNode]);
        if (!CompareWeight(n.m_skyIn))
            n.m_skyIn = self;
    }
    return BVState_succ;
}