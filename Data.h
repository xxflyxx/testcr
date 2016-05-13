#pragma once

struct ObjData
{

};


class DataSet
{
public:
    static DataSet& Instance()
    {
        static DataSet s_ins;
        return s_ins;
    }
    const ObjData* Get()
    {
        return NULL;
    }
};