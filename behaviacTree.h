#pragma once

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>

class BVNode;
class BVTree;
class BVStack;

typedef boost::shared_ptr<BVNode> BVNodePtr;
typedef boost::shared_ptr<BVTree> BVTreePtr;
typedef boost::shared_ptr<BVStack> BVStackPtr;

typedef boost::property_tree::ptree PropTree;

enum BVState
{
    BVState_invalid,
    BVState_fail,
    BVState_succ,
    BVState_running
};

class BVNodeFactory
{
    typedef BVNodePtr(*pfuncCreateNode)(const PropTree& pt, uint32_t& seq);

    std::map<std::string, pfuncCreateNode> m_creators;
public:
    BVNodePtr Create(const std::string& name, const PropTree& pt, uint32_t& seq) const
    {
        auto it = m_creators.find(name);
        if (it!=m_creators.end())
            return it->second(pt, seq);
        return BVNodePtr();
    }


    void Register(const std::string& name, pfuncCreateNode creator)
    {
        m_creators[name] = creator;
    }

    static BVNodeFactory& Instance()
    {
        static BVNodeFactory s_ins;
        return s_ins;
    }
};

class BVStack : public std::vector<BVState>
{
public:
    class Vars
    {
    public:
        virtual ~Vars() {}
        virtual void Reset() = 0;
    };

public:
    BVStack() : m_tick(0) {}
    ~BVStack()
    {
        for (auto it=m_vars.begin();it!=m_vars.end();++it)
            delete *it;
        m_vars.clear();
    }

    void Clear(size_t sz)
    {
        assign(sz, BVState_invalid);
        for (auto it=m_vars.begin();it!=m_vars.end();++it)
            delete *it;
        m_vars.clear();
        m_vars.resize(sz);
    }

    void ResetAll(size_t sz)
    {
        assign(sz, BVState_invalid);
        m_vars.resize(sz);
        for (auto it=m_vars.begin();it!=m_vars.end();++it)
        {
            if (*it)
                (*it)->Reset();
        }
    }

    void Reset(uint32_t nodeId)
    {
        if (m_vars[nodeId])
            m_vars[nodeId]->Reset();
        at(nodeId) = BVState_invalid;
    }

    Vars* GetVars(uint32_t nodeId)
    {
        assert(nodeId<m_vars.size());
        return m_vars[nodeId]!=NULL?m_vars[nodeId]:NULL;
    }

    void SetVars(uint32_t nodeId, Vars* v)
    {
        if (m_vars[nodeId])
            delete m_vars[nodeId];
        m_vars[nodeId] = v;
    }

    void UpdateTick(uint32_t dt) { m_tick += dt; }
    uint32_t Tick() { return m_tick;}
private:
    std::vector<Vars*> m_vars;
    uint32_t m_tick;
};

class BVNode
{
protected:
    uint32_t m_id;
    std::vector<BVNodePtr> m_child;
    std::string m_name;

    std::string ParseMethod(const std::string& str);
    std::vector<std::string> ParseMethodParams(const std::string& str);
    std::string ParseVar(const std::string& str);
public:
    BVNode(const PropTree& pt, uint32_t& seq);
    virtual ~BVNode();

	virtual BVState Exec(void* obj, BVStack& stk) const = 0;

    void LoadChild(const PropTree& pt, uint32_t& seq);

    void Reset(BVStack& stk) const;

    BVState State(const BVStack& stk) { return stk[m_id]; }

	void SetName(const std::string& name) { m_name = name; }
	const std::string& Name()             { return m_name; }
	uint32_t Id() const { return m_id; }

	typedef std::vector<BVNodePtr>::iterator Iter;
	Iter Begin() { return m_child.begin(); }
	Iter End()   { return m_child.end(); }
};


class BVTree
{
    BVStack m_stk;
    BVNodePtr m_bv;
    uint32_t m_dbgId;

    BVTree() : m_dbgId(0) {}
public:
    BVTree(const std::string& ph);
    BVTree(std::stringstream& ph);

    BVTreePtr Clone();
    void ResetFrom(const PropTree& pt);
    void ResetForm(const BVTree& tree);

    void Update(void* obj, uint32_t dt);

    const BVStack& Stack() { return m_stk; }
    void SetDbgId(uint32_t dbgId) { m_dbgId = dbgId; }
    uint32_t DbgId() { return m_dbgId; }
	BVNodePtr Nodes() { return m_bv; }
};

class BVTreeMap
{
    std::map<std::string, BVTreePtr> m_tree;
public:
    void Init(const std::string& dir);
    BVTreePtr Create(const std::string& name);
    void Debug();
    static BVTreeMap& Instance()
    {
        static BVTreeMap s_ins;
        return s_ins;
    }
};


#define BV_REGISTER_SECTION1(cls, fun)                          \
class BVNodeAction_##cls##fun : public BVNode                  \
{                                                               \
public:                                                         \
    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)  \
    {                                                           \
        return BVNodePtr(new BVNodeAction_##cls##fun(pt, seq));\
    }                                                           \
    BVNodeAction_##cls##fun(const PropTree& pt, uint32_t& seq) \
        : BVNode(pt, seq)                                       \
    {                                                           \
        auto str = pt.get<std::string>("property.<xmlattr>.Method"); \
        std::vector<std::string> vec = ParseMethodParams(str);  \

#define BV_REGISTER_SECTION2(cls, fun, ...)                     \
    }                                                           \
    BVState Exec(void* obj, BVStack& stk) const                 \
    {                                                           \
        BVState& st = stk[m_id];                                \
        if (st==BVState_invalid)                                \
            st = BVState_running;                               \
        if (st==BVState_running)                                \
            st = ((cls*)obj)->fun(##__VA_ARGS__);               \
        return st;                                              \
    }                                                           \
private:

#define BV_REGISTER_SECTION3(cls, fun)  };                      \
    BVNodeFactory::Instance().Register(#cls"::"#fun, BVNodeAction_##cls##fun::Create);

#define BV_VAR_DEF(t,n) t p##n;
#define BV_VAR_READ(t,n) p##n = boost::lexical_cast<t>(vec[n]);

#define BV_REGISTER_P0(cls, fun)                                \
    BV_REGISTER_SECTION1(cls, fun)                              \
    BV_REGISTER_SECTION2(cls, fun)                              \
    BV_REGISTER_SECTION3(cls, fun)


#define BV_REGISTER_P1(cls, fun, t0)                            \
    BV_REGISTER_SECTION1(cls, fun)                              \
    BV_VAR_READ(t0, 0)                                          \
    BV_REGISTER_SECTION2(cls, fun, p0)                          \
    BV_VAR_DEF(t0, 0)                                           \
    BV_REGISTER_SECTION3(cls, fun)

#define BV_REGISTER_P2(cls, fun, t0, t1)                        \
    BV_REGISTER_SECTION1(cls, fun)                              \
    BV_VAR_READ(t0, 0)                                          \
    BV_VAR_READ(t1, 1)                                          \
    BV_REGISTER_SECTION2(cls, fun, p0, p1)                      \
    BV_VAR_DEF(t0, 0)                                           \
    BV_VAR_DEF(t1, 1)                                           \
    BV_REGISTER_SECTION3(cls, fun)

#define BV_REGISTER_P3(cls, fun, t0, t1, t2)                    \
    BV_REGISTER_SECTION1(cls, fun)                              \
    BV_VAR_READ(t0, 0)                                          \
    BV_VAR_READ(t1, 1)                                          \
    BV_VAR_READ(t2, 2)                                          \
    BV_REGISTER_SECTION2(cls, fun, p0, p1, p2)                  \
    BV_VAR_DEF(t0, 0)                                           \
    BV_VAR_DEF(t1, 1)                                           \
    BV_VAR_DEF(t2, 2)                                           \
    BV_REGISTER_SECTION3(cls, fun)

#define BV_REGISTER_P4(cls, fun, t0, t1, t2, t3)                \
    BV_REGISTER_SECTION1(cls, fun)                              \
    BV_VAR_READ(t0, 0)                                          \
    BV_VAR_READ(t1, 1)                                          \
    BV_VAR_READ(t2, 2)                                          \
    BV_VAR_READ(t3, 3)                                          \
    BV_REGISTER_SECTION2(cls, fun, p0, p1, p2, p3)              \
    BV_VAR_DEF(t0, 0)                                           \
    BV_VAR_DEF(t1, 1)                                           \
    BV_VAR_DEF(t2, 2)                                           \
    BV_VAR_DEF(t3, 3)                                           \
    BV_REGISTER_SECTION3(cls, fun)
