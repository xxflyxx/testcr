#include "stdafx.h"
#include "behaviacTree.h"
#include <boost/asio.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp> 
#include <boost/algorithm/string.hpp>
#include <set>
#include <boost/thread.hpp>
#include <Windows.h>
#include <boost/weak_ptr.hpp>

#define BV_HOT_RELOAD
#define BV_DEBUG
#define BV_DEBUG_CLIENT

#define STKVARS(type, name, create)  type* name=(type*)stk.GetVars(m_id);if (!name){name=create;stk.SetVars(m_id, name);}
#define STKRUNCHK(st) if (st==BVState_invalid){st=BVState_running;}else if(st!=BVState_running){return st;}

BVTree::BVTree(const std::string& ph)
{
    PropTree pt;
    boost::property_tree::xml_parser::read_xml(ph, pt);
    ResetFrom(pt);
}

BVTree::BVTree(std::stringstream& ph)
{
    PropTree pt;
    boost::property_tree::xml_parser::read_xml(ph, pt);
    ResetFrom(pt);
}

void BVTree::Update(void* obj, uint32_t dt)
{
    if (!m_bv)
        return;
    m_stk.UpdateTick(dt);
    if (BVState_running!=m_bv->Exec(obj, m_stk))
        m_stk.ResetAll(m_stk.size());
}

BVTreePtr BVTree::Clone()
{
    BVTreePtr ptr(new BVTree);
    ptr->m_bv = m_bv;
    ptr->m_stk = m_stk;
    return ptr;
}

void BVTree::ResetFrom(const PropTree& pt)
{
    auto itfd = pt.find("behavior");
    if (itfd == pt.not_found())
        return;
    itfd = pt.to_iterator(itfd)->second.find("node");
    if (itfd == pt.not_found())
        return;
    auto& node = pt.to_iterator(itfd)->second;
    auto& att = node.get_child("<xmlattr>");
    auto name = att.get<std::string>("class");
    uint32_t seq=0;
    m_bv = BVNodeFactory::Instance().Create(name, node, seq);

    m_stk.Clear(seq);
}

void BVTree::ResetForm(const BVTree& tree)
{
    m_bv = tree.m_bv;
    m_stk = tree.m_stk;
}

//按顺序调用子节点，一个成功则返回成功，所有失败则返回失败
class BVNodeSelect : public BVNode
{
    BVNodeSelect(const PropTree& pt, uint32_t& seq) : BVNode(pt, seq) {}
public:
    BVState Exec(void* obj, BVStack& stk) const
    {
        BVState& st = stk[m_id];
        STKRUNCHK(st);
        for (auto it=m_child.begin();it!=m_child.end();++it)
        {
            BVState sub = (*it)->Exec(obj, stk);
            if (sub == BVState_running)
                return st;
            if (sub == BVState_succ)
            {
                st=BVState_succ;
                return st;
            }
        }
        st = BVState_fail;
        return st;
    }
    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)
    {
        return BVNodePtr(new BVNodeSelect(pt, seq));
    }
};

//随机顺序调用子节点，一个成功则返回成功，所有失败则返回失败
class BVNodeSelectRand : public BVNode
{
    class BVNodeSelectRandVar : public BVStack::Vars
    {
        size_t m_count;
    public:
        std::vector<size_t> seq;

        BVNodeSelectRandVar(size_t count)
            : m_count(count)
        {
            Reset();
        }
        void Reset()
        {
            seq.resize(m_count);
            for (size_t i=0;i<m_count;++i)
                seq[i]=i;
            std::random_shuffle(seq.begin(), seq.end());
        }
    };

    BVNodeSelectRand(const PropTree& pt, uint32_t& seq)
        : BVNode(pt, seq)
    {
    }
public:
    BVState Exec(void* obj, BVStack& stk) const
    {
        BVState& st = stk[m_id];
        STKRUNCHK(st);
        STKVARS(BVNodeSelectRandVar, vars, new BVNodeSelectRandVar(m_child.size()));
        auto& seq = vars->seq;
        for (auto it=seq.begin();it!=seq.end();++it)
        {
            BVState sub = m_child[*it]->Exec(obj, stk);
            if (sub == BVState_running)
                return st;
            if (sub == BVState_succ)
            {
                st=BVState_succ;
                return st;
            }
        }
        st = BVState_fail;
        return st;
    }
    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)
    {
        return BVNodePtr(new BVNodeSelectRand(pt, seq));
    }
};

//概率调用子节点，一个成功则返回成功，所有失败则返回失败
class BVNodeSelectPro : public BVNode
{
    std::map<int, int> m_rndSection;
    int m_section;

    class BVNodeSelectProVar : public BVStack::Vars
    {
        const BVNodeSelectPro* m_hold;
    public:
        int m_res;
        BVNodeSelectProVar(const BVNodeSelectPro* hold) : m_hold(hold) { Reset(); }
        void Reset()
        {
            m_res = m_hold->m_rndSection.upper_bound(rand()%m_hold->m_section)->second;
        }
    };

    BVNodeSelectPro(const PropTree& pt, uint32_t& seq)
        : BVNode(pt, seq)
        , m_section(0)
    {
        int i=0;
        BOOST_FOREACH(const PropTree::value_type &v, pt)
        {
            if(v.first == "node")
            {
                auto& node = v.second;
                auto name = node.get_child("<xmlattr>").get<std::string>("class");
                if (name == "DecoratorWeight")
                {
                    BOOST_FOREACH(const PropTree::value_type &v1, node)
                    {
                        if(v1.first == "property")
                        {
                            auto& att = v1.second.get_child("<xmlattr>");
                            auto it = att.find("Weight");
                            if (it!=att.not_found())
                            {
                                auto str = att.to_iterator(it)->second.data();
                                m_section += boost::lexical_cast<int>(ParseVar(str));
                                m_rndSection[m_section]=i++;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
public:
    BVState Exec(void* obj, BVStack& stk) const
    {
        BVState& st = stk[m_id];
        STKRUNCHK(st);
        STKVARS(BVNodeSelectProVar, vars, new BVNodeSelectProVar(this));
        st = m_child[vars->m_res]->Exec(obj, stk);
        return st;
    }
    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)
    {
        return BVNodePtr(new BVNodeSelectPro(pt, seq));
    }
};

//按顺序调用子节点，所有成功则返回成功，一个失败则返回失败
class BVNodeSequence : public BVNode
{
    BVNodeSequence(const PropTree& pt, uint32_t& seq) : BVNode(pt, seq) {}
public:
    BVState Exec(void* obj, BVStack& stk) const
    {
        BVState& st = stk[m_id];
        STKRUNCHK(st);
        for (auto it=m_child.begin();it!=m_child.end();++it)
        {
            BVState sub = (*it)->Exec(obj, stk);
            if (sub == BVState_running)
                return st;
            if (sub == BVState_fail)
            {
                st=BVState_fail;
                return st;
            }
        }
        st = BVState_succ;
        return st;
    }

    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)
    {
        return BVNodePtr(new BVNodeSequence(pt, seq));
    }
};

//并行处理，返回条件根据参数决定
class BVNodeParallel : public BVNode
{
    bool m_succAll;
    bool m_failAll;

    BVNodeParallel(const PropTree& pt, uint32_t& seq)
        : BVNode(pt, seq)
        , m_failAll(false)
        , m_succAll(true)
    {
        BOOST_FOREACH(const PropTree::value_type &v, pt)
        {
            if (v.first=="property")
            {
                auto& node = v.second.get_child("<xmlattr>");
                auto it = node.find("FailurePolicy");
                if (it!=node.not_found())
                {
                    if (node.to_iterator(it)->second.data()!="FAIL_ON_ONE")
                        m_failAll = true;
                }
                it = node.find("SuccessPolicy");
                if (it!=node.not_found())
                {
                    if (node.to_iterator(it)->second.data()!="SUCCEED_ON_ALL")
                        m_succAll = false;
                }
            }
        }
    }

public:
    BVState Exec(void* obj, BVStack& stk) const
    {
        BVState& st = stk[m_id];
        STKRUNCHK(st);
        uint32_t succNum=0;
        uint32_t failNum=0;
        for (size_t i=0; i<m_child.size();++i)
        {
            BVState sub = m_child[i]->Exec(obj, stk);
            if (sub==BVState_succ)
            {
                ++succNum;
                if (!m_succAll)
                {
                    st = BVState_succ;
                    return st;
                }
            }
            else if (sub==BVState_fail)
            {
                ++failNum;
                if (!m_failAll)
                {
                    st = BVState_fail;
                    return st;
                }
            }
        }

        if (m_succAll && (succNum==m_child.size()))
        {
            st = BVState_succ;
            return st;
        }
        if (m_failAll && (failNum==m_child.size()))
        {
            st = BVState_fail;
            return st;
        }
        if (succNum+failNum>=m_child.size())
        {
            st = BVState_fail;
            return st;
        }
        return st;
    }
    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)
    {
        return BVNodePtr(new BVNodeParallel(pt, seq));
    }
};

//循环调用N次
class BVNodeLoop : public BVNode
{
    int m_count;
    BVState m_util;

    class BVNodeLoopVar : public BVStack::Vars
    {
    public:
        int m_count;
        BVNodeLoopVar() : m_count(0) {}
        void Reset() { m_count = 0; }
    };

    BVNodeLoop(const PropTree& pt, uint32_t& seq)
        : BVNode(pt, seq)
        , m_count(-1)
        , m_util(BVState_invalid)
    {
        m_count = boost::lexical_cast<int>(ParseVar(pt.get<std::string>("property.<xmlattr>.Count")));
        BOOST_FOREACH(const PropTree::value_type &v, pt)
        {
            if(v.first == "property")
            {
                auto& node = v.second.get_child("<xmlattr>");
                auto it = node.find("Count");
                if (it!=node.not_found())
                    m_count = boost::lexical_cast<int>(ParseVar(node.to_iterator(it)->second.data()));
                it = node.find("Until");
                if (it!=node.not_found())
                {
                    if (node.to_iterator(it)->second.data() == "true")
                        m_util = BVState_succ;
                    else
                        m_util = BVState_fail;
                }
            }
        }

    }

public:
    BVState Exec(void* obj, BVStack& stk) const
    {
        BVState& st = stk[m_id];
        STKRUNCHK(st);
        const BVNodePtr& child = (*m_child.begin());
        BVState sub = child->Exec(obj, stk);
        if (sub == BVState_running)
            return st;
        if (sub == m_util)
        {
            st = sub;
            return st;
        }
        if (m_count==-1)
        {
            child->Reset(stk);
            return st;
        }
        STKVARS(BVNodeLoopVar, vars, new BVNodeLoopVar());
        ++vars->m_count;
        if (vars->m_count<m_count)
        {
            child->Reset(stk);
            return st;
        }
        st = sub;
        return st;
    }
    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)
    {
        return BVNodePtr(new BVNodeLoop(pt, seq));
    }
};

class BVNodeNoop : public BVNode
{
    BVNodeNoop(const PropTree& pt, uint32_t& seq) : BVNode(pt, seq) {}
public:
    BVState Exec(void* /*obj*/, BVStack& stk) const
    {
        stk[m_id] = BVState_succ;
        return BVState_succ;
    }
    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)
    {
        return BVNodePtr(new BVNodeNoop(pt, seq));
    }
};

class BVNodeNot : public BVNode
{
    BVNodeNot(const PropTree& pt, uint32_t& seq) : BVNode(pt, seq) {}
public:
    BVState Exec(void* obj, BVStack& stk) const
    {
        STKRUNCHK(stk[m_id]);
        if (m_child.empty())
        {
            return BVState_succ;
        }
        BVState sub = (*m_child.begin())->Exec(obj, stk);
        if (sub==BVState_succ)
            return BVState_fail;
        else if(sub == BVState_fail)
            return BVState_succ;
        return sub;
    }
    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)
    {
        return BVNodePtr(new BVNodeNot(pt, seq));
    }
};

class BVNodeWait : public BVNode
{
    uint32_t m_time;
    class BVNodeWaitVar : public BVStack::Vars
    {
    public:
        uint32_t m_over;
        BVNodeWaitVar() : m_over(0) { }
        void Reset() { m_over=0; }
    };

    BVNodeWait(const PropTree& pt, uint32_t& seq)
        : BVNode(pt, seq)
    {
        m_time = boost::lexical_cast<uint32_t>(ParseVar(pt.get<std::string>("property.<xmlattr>.Time")));
    }
public:
    BVState Exec(void* /*obj*/, BVStack& stk) const
    {
        BVState& st = stk[m_id];
        STKRUNCHK(st);
        STKVARS(BVNodeWaitVar, vars, new BVNodeWaitVar());
        if (vars->m_over == 0)
            vars->m_over = stk.Tick()+m_time;
        else if (stk.Tick()>vars->m_over)
            st = BVState_succ;
        return st;
    }
    static BVNodePtr Create(const PropTree& pt, uint32_t& seq)
    {
        return BVNodePtr(new BVNodeWait(pt, seq));
    }
};


BVNode::BVNode(const PropTree& pt, uint32_t& seq)
    : m_id(seq++)
{
    LoadChild(pt, seq);
}

BVNode::~BVNode()
{
    m_child.clear();
}

void BVNode::LoadChild(const PropTree& pt, uint32_t& seq)
{
    BOOST_FOREACH(const PropTree::value_type &v, pt)
    {
        if(v.first == "node")
        {
            auto& node = v.second;
			auto& att = node.get_child("<xmlattr>");
            auto name = att.get<std::string>("class");
            if (name == "DecoratorWeight")
            {
                LoadChild(node, seq);
                continue;
            }
            if (name == "Action")
                name = ParseMethod(node.get<std::string>("property.<xmlattr>.Method"));

            BVNodePtr ptr = BVNodeFactory::Instance().Create(name, node, seq);
#ifdef BV_DEBUG_CLIENT
			if (!ptr)
				ptr = BVNodeNoop::Create(node, seq);
#endif
            if (ptr)
            {
                ptr->SetName(name);
                m_child.push_back(ptr);
            }
			else
			{
				std::string err("BVNode::LoadChild not exist of ");
				err += name;
				throw std::runtime_error(err);
			}
        }
    }
}

void BVNode::Reset(BVStack& stk) const
{
    if (stk[m_id]==BVState_invalid)
        return;
    stk.Reset(m_id);
    for (auto it=m_child.begin();it!=m_child.end();++it)
        (*it)->Reset(stk);
}

std::string BVNode::ParseMethod(const std::string& str)
{
    size_t start = str.find('.');
    if (start==std::string::npos)
        return std::string();
    start+=1;
    size_t end = str.rfind('(');
    return str.substr(start, end-start);
}

std::vector<std::string> BVNode::ParseMethodParams(const std::string& str)
{
    size_t start = str.find('(');
    if (start==std::string::npos)
        return std::vector<std::string>();
    start+=1;
    size_t end = str.rfind(')');
    std::vector<std::string> vec;
    if (end == start)
        return vec;
    auto cont = str.substr(start, end-start);
    boost::split(vec, cont, boost::is_any_of(","));
    return vec;
}

std::string BVNode::ParseVar(const std::string& str)
{
    size_t start = str.rfind(' ');
    return str.substr(start+1);
}

#ifdef BV_HOT_RELOAD
typedef boost::weak_ptr<BVTree> BVTreeWPtr;
static std::map<std::string, std::vector<BVTreeWPtr> > s_reloadTree;
static std::set<std::string> s_changefile; //文件变化的时候会产生重复通知，用set去重
static boost::mutex s_mtx;

void monitor_thread(const std::string& dir)
{
    const int buf_size = 2048;
    TCHAR buf[buf_size];

    DWORD dwBufWrittenSize;
    HANDLE hDir;
    hDir = CreateFileA(dir.c_str(),
        FILE_LIST_DIRECTORY, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (hDir == INVALID_HANDLE_VALUE)
    {
        DWORD dwErrorCode;
        dwErrorCode = GetLastError();
        CloseHandle(hDir);
        return;
    }  

    for (;;)
    {
        if(ReadDirectoryChangesW(hDir, &buf, buf_size,
            TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE
            , &dwBufWrittenSize, NULL, NULL))
        {
            FILE_NOTIFY_INFORMATION * pfiNotifyInfo = (FILE_NOTIFY_INFORMATION*)buf;
            DWORD dwNextEntryOffset;

            do
            {
                dwNextEntryOffset = pfiNotifyInfo->NextEntryOffset;
                DWORD dwFileNameLength = pfiNotifyInfo->FileNameLength;

                boost::filesystem::path ph(dir);
                ph /= (std::wstring(pfiNotifyInfo->FileName, dwFileNameLength/2));
                if (boost::filesystem::is_regular(ph))
                {
                    boost::mutex::scoped_lock lock(s_mtx);
                    s_changefile.insert(ph.string());
                }

                if(dwNextEntryOffset != 0)
                {
                    pfiNotifyInfo= (FILE_NOTIFY_INFORMATION*)((BYTE*)pfiNotifyInfo + dwNextEntryOffset);
                }

            }while (dwNextEntryOffset != 0);
        }  
    }
}
#ifdef BV_DEBUG
typedef boost::asio::ip::tcp AsioTcp;
typedef AsioTcp::socket AsioSck;
typedef boost::shared_ptr<AsioSck> AsioSckPtr;
static boost::asio::io_service s_io;
static boost::asio::ip::tcp::acceptor s_acceptor(s_io, AsioTcp::v4(), 47665);
struct RmtDbgInfo
{
    uint32_t id;
    char name[32];
    RmtDbgInfo(){ memset(this, 0, sizeof(RmtDbgInfo)); }
};
static std::map<AsioSckPtr, RmtDbgInfo> s_rmtdbgs;
static RmtDbgInfo s_readData;
void OnRead(const AsioSckPtr& sck, const boost::system::error_code& error)
{
    if (error)
    {
		s_rmtdbgs.erase(sck);
        return;
    }
	s_rmtdbgs[sck] = s_readData;
    boost::asio::async_read(*sck, boost::asio::buffer(&s_readData, sizeof(s_readData)), boost::bind(OnRead, sck, boost::asio::placeholders::error));
}
void StartAccept();
void OnAccept(const AsioSckPtr& sck, const boost::system::error_code& error)
{
    if (!error)
		boost::asio::async_read(*sck, boost::asio::buffer(&s_readData, sizeof(s_readData)), boost::bind(OnRead, sck, boost::asio::placeholders::error));
	StartAccept();
}
void StartAccept()
{
    AsioSckPtr sck(new AsioSck(s_io));
    s_acceptor.async_accept(*sck, boost::bind(OnAccept, sck, boost::asio::placeholders::error));
}
#endif // BV_DEBUG
#endif // BV_HOT_RELOAD

void BVTreeMap::Debug()
{
#ifdef BV_HOT_RELOAD
    std::set<std::string> changeset;
    {
        boost::mutex::scoped_lock lock(s_mtx);
        changeset.swap(s_changefile);
    }
    for (auto it=changeset.begin();it!=changeset.end();++it)
    {
        boost::filesystem::path ph(*it);
        auto treename = ph.stem().string();
        BVTreePtr ntr(new BVTree(ph.string()));
        m_tree[treename]=ntr;
        auto vec = s_reloadTree[treename];
        for (size_t i=0;i<vec.size();++i)
        {
            BVTreePtr tr = vec[i].lock();
            if (tr)
                tr->ResetForm(*ntr);
        }
    }
#ifdef BV_DEBUG
    s_io.poll();
	for (auto iter = s_rmtdbgs.begin();iter!=s_rmtdbgs.end();++iter)
    {
		auto& rmt = iter->second;
        auto it = s_reloadTree.find(rmt.name);
        if(it!=s_reloadTree.end())
        {
            auto& vec = it->second;
            for (size_t i=0;i<vec.size();++i)
            {
                BVTreePtr tr = vec[i].lock();
                if (tr && tr->DbgId() == rmt.id)
                {
                    boost::system::error_code ignored_error;
					const BVStack& stk = tr->Stack();
					uint32_t sz = (uint32_t)stk.size();
                    boost::asio::write(*iter->first, boost::asio::buffer((void*)&sz, sizeof(uint32_t)), boost::asio::transfer_all(), ignored_error);
                    boost::asio::write(*iter->first, boost::asio::buffer(stk), boost::asio::transfer_all(), ignored_error);
                    break;
                }
            }
        }
    }
#endif // BV_DEBUG
#endif // BV_HOT_RELOAD
}

void BVTreeMap::Init(const std::string& dir)
{
    namespace fs = boost::filesystem;

    BVNodeFactory::Instance().Register("Selector", BVNodeSelect::Create);
    BVNodeFactory::Instance().Register("SelectorStochastic", BVNodeSelectRand::Create);
    BVNodeFactory::Instance().Register("SelectorProbability", BVNodeSelectPro::Create);
    BVNodeFactory::Instance().Register("Sequence", BVNodeSequence::Create);
    BVNodeFactory::Instance().Register("Parallel", BVNodeParallel::Create);
    BVNodeFactory::Instance().Register("DecoratorLoop", BVNodeLoop::Create);
    BVNodeFactory::Instance().Register("DecoratorLoopUntil", BVNodeLoop::Create);
    BVNodeFactory::Instance().Register("Noop", BVNodeNoop::Create);
    BVNodeFactory::Instance().Register("Wait", BVNodeWait::Create);
    BVNodeFactory::Instance().Register("DecoratorNot", BVNodeNot::Create);

    if (dir.empty())
        return;
    fs::path fullpath (dir, fs::native);
    if(!fs::exists(fullpath))
        return;
    fs::recursive_directory_iterator end_iter;
    for(fs::recursive_directory_iterator iter(fullpath); iter!=end_iter; ++iter)
    {
        if (!fs::is_regular(*iter))
            continue;
        const fs::path& ph = iter->path();
        if (ph.extension() == ".xml")
        {
            m_tree[ph.stem().string()].reset(new BVTree(ph.string()));
        }
    }
#ifdef BV_HOT_RELOAD
    boost::thread th(monitor_thread, dir);
#ifdef BV_DEBUG
    StartAccept();
#endif
#endif
}


BVTreePtr BVTreeMap::Create(const std::string& name)
{
    auto it = m_tree.find(name);
    if (it!=m_tree.end())
    {
        BVTreePtr cl = it->second->Clone();
#ifdef BV_DEBUG
        s_reloadTree[name].push_back(BVTreeWPtr(cl));
#endif
        return cl;
    }
    return BVTreePtr();
}



// int _tmain(int argc, _TCHAR* argv[])
// {
//     BVTreeMap::Instance().Init();
//     BVTreeMap::Instance().Load("BVTree");
// 	return 0;
// }
