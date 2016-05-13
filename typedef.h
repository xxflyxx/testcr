#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#define VAR_DEF(type, name) private: \
type m_##name; \
public: \
const type& Get##name() { return m_##name; } \
void Set##name(const type& v) { m_##name = v; }

class GObject;
typedef boost::shared_ptr<GObject> GObjectPtr;
typedef boost::weak_ptr<GObject> GObjectWPtr;

enum GObjectType
{
    GObjectType_walker = 0x1,
    GObjectType_flyer = 0x2,
    GObjectType_build = 0x4,
    GObjectType_tower = 0x8,
    GObjectType_bullet = 0x10
};

enum Direct
{
    Direct_n,
    Direct_nw,
    Direct_sw,
    Direct_s,
    Direct_se,
    Direct_ne
};

enum ObjKind
{
    // ground
    ObjKind_ground_start,
    ObjKind_guowang = ObjKind_ground_start,
    ObjKind_huweita,
    ObjKind_qishi,
    ObjKind_gongjianshou,
    ObjKind_ground_end,

    // fly
    ObjKind_flyer_start = 10000,
    ObjKind_cangying = ObjKind_flyer_start,
    ObjKind_flyer_end,

    // bullet
    ObjKind_bullet_start = 20000,
    ObjKind_gongjianshou_blt = ObjKind_bullet_start,
    ObjKind_bullet_end,
};

enum Camp
{
    Camp_a,
    Camp_b
};