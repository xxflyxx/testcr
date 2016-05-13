#pragma once
#include "stdafx.h"

//���������εı߳�Ϊa��


//��������ľ��ζ�λ�Ļ�����λ
#define unitx(a)    ((a)*1.7320508f)  //sqrt(3) * a
#define unity(a)    ((a)*1.5f)                    //1.5 * a


//��������ľ������������λ
float g_unitx;
float g_unity;
float g_MinDistance2;     // (a*sqrt(3)/2)^2

                           //���������εı߳�
void SetCellSize(int a)
{
    if (a > 0)
    {
        g_unitx = unitx(a);
        g_unity = unity(a);

        //����֮����3 �߳���ƽ����������������С���ͱ�Ȼ����
        g_MinDistance2 = a*a*0.75f;
    }
}


//��ȡ������ľ���ƽ��
inline int distance2(int x1, int y1, int x2, int y2)
{
    return ((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));
}


//������갴�µĵ����꣨x��y��
//���ر����������ε���������
void GetCell(int x, int y, int *lpCellX, int *lpCellY)
{
    //λ�ھ�����������ϵ�����CELL���ĵ�
    POINT points[3];
    //��ǰ����
    int dist;
    int mindist = (int)(g_MinDistance2 * 100); //һ���ǳ����ֵ
    int i, index;//index:�����������
    //���������λ����һ������������
    int cx = (int)(x / g_unitx);
    int cy = (int)(y / g_unity);

    points[0].x = (int)(g_unitx * cx);
    points[1].x = (int)(g_unitx * (cx + 0.5));
    points[2].x = (int)(g_unitx * (cx + 1));
    //����cy�Ƿ���ż���������������������
    if (cy % 2 == 0)
    {
        //ż��ʱ����������ɵ�������
        points[0].y = points[2].y = (int)(g_unity * cy);
        points[1].y = (int)(g_unity * (cy + 1));
    }
    else
    {
        //����ʱ�������������������
        points[0].y = points[2].y = (int)(g_unity * (cy + 1));
        points[1].y = (int)(g_unity * cy);
    }

    //�����ҳ���������һ�������
    for (i = 0; i < 3; i++)
    {
        //��������ƽ��
        dist = distance2(x, y, points[i].x, points[i].y);

        //����Ѿ��϶�������
        if (dist < g_MinDistance2)
        {
            index = i;
            break;
        }

        //������С����ֵ������
        if (dist < mindist)
        {
            mindist = dist;
            index = i;
        }
    }

    //����index ���Ǳ�����Ľ��
    *lpCellX = points[index].x;
    *lpCellY = points[index].y;
}

//��������CELL�����ĵ�ͱ߳�a�����Cell�������ε������˵�
void GetCellPoints(int cellx, int celly, int a, POINT *lpPoints)
{
    if (lpPoints == NULL) return;

    lpPoints[0].x = cellx;
    lpPoints[0].y = celly - a;

    lpPoints[1].x = cellx + (int)(g_unitx*0.5);
    lpPoints[1].y = celly - a / 2;

    lpPoints[2].x = lpPoints[1].x;
    lpPoints[2].y = celly + a / 2;

    lpPoints[3].x = cellx;
    lpPoints[3].y = celly + a;

    lpPoints[4].x = cellx - (int)(g_unitx*0.5);
    lpPoints[4].y = celly + a / 2;

    lpPoints[5].x = lpPoints[4].x;
    lpPoints[5].y = celly - a / 2;
}

// x*cx
// x*cy+2*y*cy

// ����������λ
//@param xPos ���룬�����ѯ�ĵ��x����
//@param yPos ���룬�����ѯ�ĵ��y����
//@param cell_x ������ĵ����������x����
//@param cell_y ������ĵ����������y����
void getHotCell(int xPos, int yPos, int& cell_x, int& cell_y)
{
#define CELL_BORDER        (40) //�����α߳�
#define GRID_WIDTH        (CELL_BORDER*1.5f)
#define GRID_HEIGHT        (CELL_BORDER*0.8660254f)    // sqr(3)/2=0.8660254
#define TEMP_1            ((GRID_WIDTH*GRID_WIDTH - GRID_HEIGHT*GRID_HEIGHT)/2.f)
#define TEMP_2            ((GRID_WIDTH*GRID_WIDTH + GRID_HEIGHT*GRID_HEIGHT)/2.f)

    cell_y = (int)(yPos / GRID_HEIGHT);
    float y = yPos - cell_y*GRID_HEIGHT;

    cell_x = (int)(xPos / GRID_WIDTH);
    float x = xPos - cell_x*GRID_WIDTH;

    if ((cell_x + cell_y) & 1)
    {
        if (x*GRID_WIDTH - y*GRID_HEIGHT > TEMP_1 ) cell_x++;
    }
    else
    {
        if (x*GRID_WIDTH + y*GRID_HEIGHT > TEMP_2) cell_x++;
    }

    cell_x = (cell_x + (1 - (cell_y & 1))) / 2;

//     cell_x = (int)(xPos / GRID_WIDTH);
//     cell_y = (int)(yPos / GRID_HEIGHT);


}

void getHotCellPoints(int cellx, int celly, int a, POINT *lpPoints)
{
    if (lpPoints == NULL) return;

    lpPoints[0].x = cellx*GRID_WIDTH - CELL_BORDER/2;
    lpPoints[0].y = (cellx + celly*2 - 1)*GRID_HEIGHT;

    lpPoints[1].x = cellx*GRID_WIDTH + CELL_BORDER/2;
    lpPoints[1].y = (cellx + celly*2 - 1)*GRID_HEIGHT;

    lpPoints[2].x = cellx*GRID_WIDTH + CELL_BORDER;
    lpPoints[2].y = (cellx + celly*2)*GRID_HEIGHT;

    lpPoints[3].x = cellx*GRID_WIDTH + CELL_BORDER/2;
    lpPoints[3].y = (cellx + celly*2 + 1)*GRID_HEIGHT;

    lpPoints[4].x = cellx*GRID_WIDTH - CELL_BORDER/2;
    lpPoints[4].y = (cellx + celly*2 + 1)*GRID_HEIGHT;

    lpPoints[5].x = cellx*GRID_WIDTH - CELL_BORDER;
    lpPoints[5].y = (cellx + celly*2)*GRID_HEIGHT;

//     lpPoints[0].x = cellx;
//     lpPoints[0].y = celly - a;
// 
//     lpPoints[1].x = cellx + (int)(g_unitx*0.5);
//     lpPoints[1].y = celly - a / 2;
// 
//     lpPoints[2].x = lpPoints[1].x;
//     lpPoints[2].y = celly + a / 2;
// 
//     lpPoints[3].x = cellx;
//     lpPoints[3].y = celly + a;
// 
//     lpPoints[4].x = cellx - (int)(g_unitx*0.5);
//     lpPoints[4].y = celly + a / 2;
// 
//     lpPoints[5].x = lpPoints[4].x;
//     lpPoints[5].y = celly - a / 2;
}
