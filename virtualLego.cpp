////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: 박창현 Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <windows.h>
#include <string>

using namespace std;
// Direct3D 장치 포인터
IDirect3DDevice9* Device = NULL;

// 윈도우 크기 설정
const int Width = 1024;
const int Height = 768;

// 공(ball)의 개수와 초기 위치 정의
const float spherePos[4][2] = { {-2.7f,0}, {+2.4f,0}, {3.3f,0}, {-2.7f,-0.9f} };
// 공의 색상 설정
const D3DXCOLOR sphereColor[4] = { d3d::RED, d3d::RED, d3d::YELLOW, d3d::WHITE };

// -----------------------------------------------------------------------------
// 변환 행렬 선언 (월드, 뷰, 투영)
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

// 공 반지름 및 기타 상수 정의
#define M_RADIUS 0.29   // 공 반지름
#define P_RADIUS 0.21   //플레이어 머리용 반지름
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 1 
#define PLANE_X 6   //게임판의 x
#define PLANE_Y 9   //게임판의 Y
#define WALL_THICKNESS 0.3  //벽의 두께

//맵 배치에 사용
int map[15][15] = {
        {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


// -----------------------------------------------------------------------------
// 공(CSphere) 클래스 정의
// -----------------------------------------------------------------------------
class CSphere {
protected:
    float center_x, center_y, center_z; // 공의 중심 좌표
    float m_radius;                     // 공 반지름
    float m_velocity_x;                 // x축 속도
    float m_velocity_z;                 // z축 속도
    
public:
    // 생성자: 공의 초기 상태를 설정
    CSphere(void) {
        D3DXMatrixIdentity(&m_mLocal); // 로컬 변환 행렬 초기화
        ZeroMemory(&m_mtrl, sizeof(m_mtrl)); // 재질 정보 초기화
        m_radius = 0;
        m_velocity_x = 0;
        m_velocity_z = 0;
        m_pSphereMesh = NULL;          // 메쉬 초기화
    }

    ~CSphere(void) {}

    // 공 객체 생성
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE) {
        if (NULL == pDevice) return false;

        // 공 재질 설정
        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        // 공 메쉬 생성
        if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
            return false;

        return true;
    }

    // 공 객체 삭제
    void destroy(void) {
        if (m_pSphereMesh != NULL) {
            m_pSphereMesh->Release();
            m_pSphereMesh = NULL;
        }
    }

    // 공 그리기
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld) {
        if (NULL == pDevice) return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld); // 월드 변환 설정
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal); // 로컬 변환 적용
        pDevice->SetMaterial(&m_mtrl); // 재질 설정
        m_pSphereMesh->DrawSubset(0); // 메쉬 그리기
    }
    // 공 위치와 속도 업데이트
    virtual void ballUpdate(float timeDiff) {
        const float TIME_SCALE = 3.3; // 시간 스케일 조정
        D3DXVECTOR3 cord = this->getCenter(); // 현재 공의 중심 좌표
        double vx = abs(this->getVelocity_X());
        double vz = abs(this->getVelocity_Z());

        if (vx > 0.01 || vz > 0.01) {
            float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
            float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

            
            if (tX <= -4.5f + P_RADIUS) {
                tX = -4.5 + P_RADIUS;
            }
            else if (tX >= 4.5 - P_RADIUS) {
                tX = 4.5 - P_RADIUS;
            }
            if (tZ <= -4.5f + P_RADIUS) {
                tZ = -4.5 + P_RADIUS;
            }
            else if (tZ >= 4.5 - P_RADIUS) {
                tZ = 4.5 - P_RADIUS;
            }
            this->setCenter(tX, cord.y, tZ); // 새로운 위치 설정
        }
        else {
            this->setPower(0, 0); // 속도가 매우 낮으면 멈춤
        }

        double rate = 1 - (1 - DECREASE_RATE) * timeDiff * 400; 
        if (rate < 0) rate = 0;
        this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);
    }

    // 속도 및 위치 제어 메서드
    double getVelocity_X() { return this->m_velocity_x; }
    double getVelocity_Z() { return this->m_velocity_z; }
    void setPower(double vx, double vz) { this->m_velocity_x = vx; this->m_velocity_z = vz; }
    void setCenter(float x, float y, float z) {
        D3DXMATRIX m;
        center_x = x; center_y = y; center_z = z;
        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }
    
    float virtual getRadius(void) const { return (float)(M_RADIUS); }
    const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
    D3DXVECTOR3 getCenter(void) const {
        return D3DXVECTOR3(center_x, center_y, center_z);
    }
    

private:
    D3DXMATRIX m_mLocal;    // 로컬 변환 행렬
    D3DMATERIAL9 m_mtrl;    // 재질 정보
    ID3DXMesh* m_pSphereMesh; // 메쉬 포인터
};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

    float               m_x;
    float               m_z;
    float                   m_width;
    float                   m_depth;
    float               m_height;

public:
    CWall(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_width = 0;
        m_depth = 0;
        m_pBoundMesh = NULL;
    }
    ~CWall(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;

        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        m_width = iwidth;
        m_depth = idepth;

        if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
            return false;
        return true;
    }
    void destroy(void)
    {
        if (m_pBoundMesh != NULL) {
            m_pBoundMesh->Release();
            m_pBoundMesh = NULL;
        }
    }
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
        m_pBoundMesh->DrawSubset(0);
    }

    

    

    void setPosition(float x, float y, float z)
    {
        D3DXMATRIX m;
        this->m_x = x;
        this->m_z = z;

        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }

    float getHeight(void) const { return M_HEIGHT; }

    

private:
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh* m_pBoundMesh;
};

class CExplosion : public CWall {
private:
    bool e_isActive = false;
    //폭발 이펙트 활성화
    float e_time = 0;
    //생성 후 지난 시간
    float e_setTime = 1;
    //지속시간 세팅
    
public:

    void activate(float x, float y, float z) {
        e_isActive = true;
        e_time = 0;
        setPosition(x, y, z);
    }

    void explosionUpdate(float deltaTime) {
        if (!e_isActive) return;

        e_time += deltaTime;
        if (e_time > e_setTime) {
            e_isActive = false;
        }
    }

    bool getActive() {
        return e_isActive;
    }
};

class CBoom : public CSphere {
private:
    float b_time;
    bool b_isActive = false;
    float b_setTime = 2;
    int b_numOfBoom = 1;
    const int b_max = 10;
    int b_indexX =0;
    int b_indexZ =0;
    int b_range = 1;
    CExplosion explosion[5];
    int numOfExp = 5;

public:
    bool createExplosion(IDirect3DDevice9* pDevice) {
        for (int i = 0; i < numOfExp; i++) {
            if (false == explosion[i].create(pDevice, -1, -1, 0.6f, 0.1f, 0.6f, d3d::RED)) return false;
        }
        return true;
    }
    //Explosion 생성


    void pressKey(float sx, float py, float sz) {
        if (!b_isActive) {
            this -> b_isActive = true;
            setCenter(sx, py, sz);
            this -> b_time = 0;
        }
    }
    //키 눌렀을 때

    void boomUpdate(float timeDelta) {
        if (!b_isActive) {
            return;
        }

        b_time += timeDelta;
        if (b_time >= b_setTime) {

            b_isActive = false;

            explosion[0].activate(-4.2 + 0.6*b_indexX, 0, 4.2 - 0.6*b_indexZ); // 폭탄 위치
            for (int i = 0; i < b_range; i++) {
                explosion[1 + i].activate(-4.2 + 0.6 * (b_indexX+1), 0, 4.2 - 0.6 * b_indexZ); // X +
                explosion[2 + i].activate(-4.2 + 0.6 * (b_indexX-1), 0, 4.2 - 0.6 * b_indexZ); // X -
                explosion[3 + i].activate(-4.2 + 0.6 * b_indexX, 0, 4.2 - 0.6 * (b_indexZ+1)); // Z +
                explosion[4 + i].activate(-4.2 + 0.6 * b_indexX, 0, 4.2 - 0.6 * (b_indexZ-1)); // Z -
            }
        }
    }

    void updateExplosions(float timeDelta) {
        for (int i = 0; i < 5; ++i) {
            explosion[i].explosionUpdate(timeDelta);
        }
    }

    void drawExplosions(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld) {
        for (int i = 0; i < 5; ++i) {
            if (explosion[i].getActive()) {
                explosion[i].draw(pDevice, mWorld);
            }
        }
    }

    bool getActive() {
        return b_isActive;
    }
    //active 값 반환

    void setNumOfBoom(int numOfBoom) {
        if (numOfBoom <= b_max) {
            b_numOfBoom = numOfBoom;
        }
    }
    //아이템 먹고 폭탄 개수 늘려주기

    int getNumOfBoom() {
        return b_numOfBoom;
    }

    void setIndexXY(int indexX, int indexY) {
        this->b_indexX = indexX;
        this->b_indexZ = indexY;
    }
};

class Player : public CSphere { //플레이어 저장하는 클래스
private:
    int playerLife = 3;//플레이어 목숨
    int bombRange = 1;//폭탄 범위
    int bombCap = 1;//폭탄용량
    float playerSpeed = 1.5f;   //기본으로 존재하는 플레이어 스피드
    int playerIndexX;
    int playerIndexY;
public:
    void updatePlayerIndex() {
        //0,0칸은 -4.5, 4.5임 여기서 플레이어 반지름을 빼서 벽에서 못나가게 하는걸 구하자
        this->playerIndexX = (int)((this->center_x + 4.5f) / 0.6f);
        this->playerIndexY = (int)((4.5f - this->center_z) / 0.6f);

        string message = "x: " + to_string(this->playerIndexX) + " y: " + to_string(this->playerIndexY);
        OutputDebugString((message + "\n").c_str());
    }

    

    int getPlayerIndexX() {
        return playerIndexX;
    }

    int getPlayerIndexY() {
        return playerIndexY;
    }
    
    double getPlayerSpeed() { return this->playerSpeed; }

    int getPlayerLife() {
        return this-> playerLife;
    }

    void setPlayerLife() {
        this->playerLife -= 1;
        if (this->playerLife > 0) { // 목숨은 음수가 될 수 없음
            
        }
        else {
            //게임오버 실행
        }
    }

    // Getter와 Setter for bombRange
    int getBombRange() {
        return bombRange;
    }

    void setBombRange(int range) {//setter 대신에 그냥 아이템 먹으면 이거 증가함
        this->bombRange += 1;
    }

    // Getter와 Setter for bombCap
    int getBombCap() {
        return this-> bombCap;
    }

    void setBombCap(int cap) {//setter 대신에 그냥 아이템 먹으면 이거 증가함
        this->bombCap += 1;
    }



    void setPlayerSpeed(float speed) {//증가는 얼마나 증가할지 생각해야겠는데
        this->playerSpeed += 0.3f;
    }

    float getRadius(void)  const override { return (float)(P_RADIUS); };

    void ballUpdate(float timeDiff) override {
        const float TIME_SCALE = 3.3; // 시간 스케일 조정
        D3DXVECTOR3 cord = this->getCenter(); // 현재 공의 중심 좌표
        double vx = abs(this->getVelocity_X());
        double vz = abs(this->getVelocity_Z());
        this->updatePlayerIndex();
        if (vx > 0.01 || vz > 0.01) {
            float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
            float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

            //벽 나가는거 막는 코드
            if (tX <= -4.5f + P_RADIUS) {
                tX = -4.5 + P_RADIUS;
            }
            else if (tX >= 4.5 - P_RADIUS) {
                tX = 4.5 - P_RADIUS;
            }
            if (tZ <= -4.5f + P_RADIUS) {
                tZ = -4.5 + P_RADIUS;
            }
            else if (tZ >= 4.5 - P_RADIUS) {
                tZ = 4.5 - P_RADIUS;
            }

            int indexX = this->playerIndexX;
            int indexY = this->playerIndexY;
            if (m_velocity_x < 0) {//좌
                //즉 현재 인덱스 x의 감소에서 체크
                //[][] 두번째 인덱스
                if (indexX >= 1) {
                    if (map[indexY][indexX-1] >= 1) {//박스나 폭탄 지정하는 상수면 이동불가
                        //-4.2 + 0.6 * (indexX-1)가 상자의 center
                        if (tX <= -4.2 + 0.6 * (indexX - 1) + 0.15f + 0.275f) {
                            tX = -4.2 + 0.6 * (indexX - 1) + 0.15f + 0.275f;
                        }
                    }
                }
            }
            else if (m_velocity_x > 0) {//우
                if (indexX <= 13) {//14안넘게
                    if (map[indexY][indexX + 1] >= 1) {//박스나 폭탄 지정하는 상수면 이동불가
                        //-4.2 + 0.6 * (indexX+1)가 상자의 center
                        if (tX >= -4.2 + 0.6 * (indexX + 1) - 0.15f - 0.275f) {
                            tX = -4.2 + 0.6 * (indexX + 1) - 0.15f - 0.275f;
                        }
                    }
                }
            }
            else if (m_velocity_z > 0) {//상
                if (indexY >= 0) {
                    if (map[indexY-1][indexX] >= 1) {//박스나 폭탄 지정하는 상수면 이동불가
                        //-4.2 + 0.6 * (indexX-1)가 상자의 center
                        if (tZ >= 4.2 - 0.6 * (indexY - 1) - 0.15f - 0.275f) {
                            tZ = 4.2 - 0.6 * (indexY - 1) - 0.15f - 0.275f;
                        }
                    }
                }
            }
            else if (m_velocity_z < 0) {//하
                if (indexY <= 13) {//14안넘게
                    if (map[indexY + 1 ][indexX] >= 1) {//박스나 폭탄 지정하는 상수면 이동불가
                        //-4.2 + 0.6 * (indexX-1)가 상자의 center
                        if (tZ <= 4.2 - 0.6 * (indexY + 1) + 0.15f + 0.275f) {
                            tZ = 4.2 - 0.6 * (indexY + 1) + 0.15f + 0.275f;
                        }
                    }
                }
            }

            

            this->setCenter(tX, cord.y, tZ); // 새로운 위치 설정
        }
        
    }
    void bindingPlayerBody(CWall& body) {
        body.setPosition(this->center_x, 0.3f, this->center_z);

    }
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
    CLight(void)
    {
        static DWORD i = 0;
        m_index = i++;
        D3DXMatrixIdentity(&m_mLocal);
        ::ZeroMemory(&m_lit, sizeof(m_lit));
        m_pMesh = NULL;
        m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_bound._radius = 0.0f;
    }
    ~CLight(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
    {
        if (NULL == pDevice)
            return false;
        if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
            return false;

        m_bound._center = lit.Position;
        m_bound._radius = radius;

        m_lit.Type = lit.Type;
        m_lit.Diffuse = lit.Diffuse;
        m_lit.Specular = lit.Specular;
        m_lit.Ambient = lit.Ambient;
        m_lit.Position = lit.Position;
        m_lit.Direction = lit.Direction;
        m_lit.Range = lit.Range;
        m_lit.Falloff = lit.Falloff;
        m_lit.Attenuation0 = lit.Attenuation0;
        m_lit.Attenuation1 = lit.Attenuation1;
        m_lit.Attenuation2 = lit.Attenuation2;
        m_lit.Theta = lit.Theta;
        m_lit.Phi = lit.Phi;
        return true;
    }
    void destroy(void)
    {
        if (m_pMesh != NULL) {
            m_pMesh->Release();
            m_pMesh = NULL;
        }
    }
    bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return false;

        D3DXVECTOR3 pos(m_bound._center);
        D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
        D3DXVec3TransformCoord(&pos, &pos, &mWorld);
        m_lit.Position = pos;

        pDevice->SetLight(m_index, &m_lit);
        pDevice->LightEnable(m_index, TRUE);
        return true;
    }

    void draw(IDirect3DDevice9* pDevice)
    {
        if (NULL == pDevice)
            return;
        D3DXMATRIX m;
        D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
        pDevice->SetTransform(D3DTS_WORLD, &m);
        pDevice->SetMaterial(&d3d::WHITE_MTRL);
        m_pMesh->DrawSubset(0);
    }

    D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
    DWORD               m_index;
    D3DXMATRIX          m_mLocal;
    D3DLIGHT9           m_lit;
    ID3DXMesh* m_pMesh;
    d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall   g_legoPlane;
CWall   g_legowall[4];
CWall   playerBody[2];


CLight   g_light;

CWall boundaryLineByX[14];//15x15사이즈 가로눈금선
CWall boundaryLineByY[14];//15x15사이즈 세로눈금선



double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

Player player[2];//플레이어 1p,2p



int ss_y = 14;
int ss_x = 12;
int validCount = 0;


CBoom testBoom;



// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{

}

// initialization
bool Setup()
{
    int i;
    OutputDebugStringA("This is a test message.\n");
    D3DXMatrixIdentity(&g_mWorld);
    D3DXMatrixIdentity(&g_mView);
    D3DXMatrixIdentity(&g_mProj);

    //15x15 사이즈 만들듯 흠
    // create plane and set the position
    if (false == g_legoPlane.create(Device, -1, -1, 9, 0.03f, 9, d3d::WHITE)) return false;
    g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);
    //-0.00012 + 0.015

    // create walls and set the position. note that there are four walls
    if (false == g_legowall[0].create(Device, -1, -1, 9, WALL_THICKNESS, 0.12f, d3d::DARKRED)) return false;
    g_legowall[0].setPosition(0.0f, 0.12f, 4.56f);
    if (false == g_legowall[1].create(Device, -1, -1, 0.12f, WALL_THICKNESS, 9, d3d::DARKRED)) return false;
    g_legowall[1].setPosition(-4.56f, 0.12f, 0.0f);


    // Device ,float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE
    //가로길이, 벽 높이, 세로길이
    if (false == g_legowall[2].create(Device, -1, -1, 0.12f, WALL_THICKNESS, 9, d3d::DARKRED)) return false;
    g_legowall[2].setPosition(4.56f, 0.12f, 0.0f);
    if (false == g_legowall[3].create(Device, -1, -1, 9, WALL_THICKNESS, 0.12f, d3d::DARKRED)) return false;
    g_legowall[3].setPosition(0.0f, 0.12f, -4.56f);

    // create four balls and set the position

    //경계선 그리기
    //한칸 사이 간격은 0.6이다
    //한칸이 즉 0.6*0.6
    for (int i = 0; i < 14; i++) {

        if (false == boundaryLineByX[i].create(Device, -1, -1, 0.05f, 0.01f, 9, d3d::BLACK)) return false;
        boundaryLineByX[i].setPosition(-4.5f + 0.6 + 0.6*i, 0.015f, 0.0f);
r

        if (false == boundaryLineByY[i].create(Device, -1, -1, 9, 0.05f, 0.01f, d3d::BLACK)) return false;
        boundaryLineByY[i].setPosition(0.0f, 0.015f, -4.5f + 0.6 + 0.6 * i);
    }







    //플레이어 1 생성
    if (false == player[0].create(Device, d3d::RED)) return false;
    player[0].setCenter(.0f, (float)P_RADIUS + 0.5f, -3.5f);
    player[0].setPower(0, 0);

    if (false == playerBody[0].create(Device, -1, -1, 0.3f, 0.6f, 0.3f, d3d::RED)) return false;
    playerBody[0].setPosition(0.0f, 0.3f, 0.0f);
    

    //test용 boom
    if (false == testBoom.create(Device, d3d::BLACK)) return false;
    if (false == testBoom.createExplosion(Device))return false;


    //플레이어 2 생성 
    if (false == player[1].create(Device, d3d::BLUE)) return false;
    player[1].setCenter(4.5f, (float)P_RADIUS + 0.5f, -3.5f);
    player[1].setPower(0, 0);
    if (false == playerBody[1].create(Device, -1, -1, 0.3f, 0.6f, 0.3f, d3d::BLUE)) return false;
    playerBody[1].setPosition(0.0f, 0.3f, 0.0f);


    // light setting 
    D3DLIGHT9 lit;
    ::ZeroMemory(&lit, sizeof(lit));
    lit.Type = D3DLIGHT_POINT;
    lit.Diffuse = d3d::WHITE;
    lit.Specular = d3d::WHITE * 0.9f;
    lit.Ambient = d3d::WHITE * 0.9f;
    lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
    lit.Range = 1000.0f;
    lit.Attenuation0 = 0.0f;
    lit.Attenuation1 = 0.9f;
    lit.Attenuation2 = 0.0f;
    if (false == g_light.create(Device, lit))
        return false;

    

    //카메라 위치 고정할듯?
    // Position and aim the camera.
    //z가 좀 기울어지게 하는ㄴ 
    D3DXVECTOR3 pos(0.0f, 10.0f, -10.0f);
    D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 up(0.0f, 0.1f, 5.0f);
    D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
    Device->SetTransform(D3DTS_VIEW, &g_mView);

    // Set the projection matrix.
    D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
        (float)Width / (float)Height, 1.0f, 100.0f);
    Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

    // Set render states.
    Device->SetRenderState(D3DRS_LIGHTING, TRUE);
    Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

    g_light.setLight(Device, g_mWorld);
    return true;
}

void Cleanup(void)
{
    g_legoPlane.destroy();
    for (int i = 0; i < 3; i++) {
        g_legowall[i].destroy();
    }
    destroyAllLegoBlock();
    g_light.destroy();
    
}


// timeDelta represents the time between the current image frame and the last image frame.
// timeDelta는 현재 이미지와 마지막 이미지의 프레임사이를 나타낸다.
// the distance of moving balls should be "velocity * timeDelta"
// 움직인 고으이 거리는 velocity * timeDelta이다.
bool Display(float timeDelta)
{
    int i = 0;
    int j = 0;
    

    

    if (Device)
    {
        Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
        Device->BeginScene();

        // update the position of each ball. during update, check whether each ball hit by walls.
        
        
        

        

        
        for (int i = 0; i < 14; i++) {
            boundaryLineByX[i].draw(Device, g_mWorld);

            boundaryLineByY[i].draw(Device, g_mWorld);
        }

        // draw plane, walls, and spheres
        g_legoPlane.draw(Device, g_mWorld);
        for (i = 0; i < 4; i++) {
            g_legowall[i].draw(Device, g_mWorld);
            
        }
        
        
        //g_light.draw(Device);
        

        //플레이어 출력, 몸통도 같이
        player[0].ballUpdate(timeDelta);
        player[0].draw(Device, g_mWorld);
        playerBody[0].draw(Device, g_mWorld);
        player[0].bindingPlayerBody(playerBody[0]);
        player[1].ballUpdate(timeDelta);
        player[1].draw(Device, g_mWorld);
        playerBody[1].draw(Device, g_mWorld);
        player[1].bindingPlayerBody(playerBody[1]);

        if (testBoom.getActive()) {
            testBoom.draw(Device, g_mWorld);
        }
        testBoom.boomUpdate(timeDelta);
        //boom의 active 값이 true인 경우에만 보이도록 설정
        testBoom.drawExplosions(Device, g_mWorld);
        testBoom.updateExplosions(timeDelta);

        Device->EndScene();
        Device->Present(0, 0, 0, 0);
        Device->SetTexture(0, NULL);
    }
    return true;
}






LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool wire = false;
    static bool isReset = true;
    static int old_x = 0;
    static int old_y = 0;
    static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;
    static float xpower = 0.0f, ypower = 0.0f;  // 현재 이동 속도
    static bool keyStates[256] = { false };
    double playerOneSp = player[0].getPlayerSpeed();
    double playerTwoSp = player[1].getPlayerSpeed();
    //중요한건 이속아이템을 먹을 변형이 가능한가?
    //getVelocity 얻어와서 변형해주면 될듯?
    switch (msg) {
    case WM_DESTROY:
    {
        ::PostQuitMessage(0);
        break;
    }
    case WM_KEYDOWN:
    {
        player[0].updatePlayerIndex();
        keyStates[wParam] = true;
        switch (wParam) {
        case VK_ESCAPE:
            ::DestroyWindow(hwnd);
            break;
        case VK_RETURN:
            if (NULL != Device) {
                wire = !wire;
                Device->SetRenderState(D3DRS_FILLMODE,
                    (wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
            }
            break;
        case 'W':
            player[0].setPower(0, playerOneSp);
            break;
        case 'A':
            player[0].setPower(-playerOneSp, 0);
            break;
        case 'S':
            player[0].setPower(0, - playerOneSp);
            break;
            
        case 'D':
            player[0].setPower(playerOneSp, 0);
            break;

        case 'F':
            player[0].updatePlayerIndex();

            testBoom.pressKey(-4.2 + 0.6*player[0].getPlayerIndexX(), player[0].getCenter().y - 0.1, 4.2 - 0.6 * player[0].getPlayerIndexY());
            testBoom.setIndexXY(player[0].getPlayerIndexX(), player[0].getPlayerIndexY());

            break;

        case VK_UP:
            player[1].setPower(0, playerTwoSp);
            break;
        case VK_DOWN:
            player[1].setPower(0, -playerTwoSp);
           
            break;
        case VK_LEFT:
            player[1].setPower(-playerTwoSp, 0);
            break;
        case VK_RIGHT:
            player[1].setPower(playerTwoSp, 0);
            break;
        default:
            break;
        }
        
        break;
    }
    case WM_KEYUP:
        
        switch (wParam) {
        case 'W':
            if (player[0].getVelocity_X() == 0) {//a나 d키 누르고 있으면 안변하게
                player[0].setPower(0, 0);
            }
            
            break;
        case 'A':
            if (player[0].getVelocity_Z() == 0) {
                player[0].setPower(0, 0);
            }
            break;
        case 'S':
            if (player[0].getVelocity_X() == 0) {
                player[0].setPower(0, 0);
            }
            break;

        case 'D':
            if (player[0].getVelocity_Z() == 0) {
                player[0].setPower(0, 0);
            }
            break;

        case 'F':

        case VK_UP:
            if (player[1].getVelocity_X() == 0) {//a나 d키 누르고 있으면 안변하게
                player[1].setPower(0, 0);
            }
            break;
        case VK_DOWN:
            if (player[1].getVelocity_X() == 0) {//a나 d키 누르고 있으면 안변하게
                player[1].setPower(0, 0);
            }
            break;
        case VK_LEFT:
            if (player[1].getVelocity_Z() == 0) {//a나 d키 누르고 있으면 안변하게
                player[1].setPower(0, 0);
            }
            break;

        case VK_RIGHT:
            if (player[1].getVelocity_Z() == 0) {//a나 d키 누르고 있으면 안변하게
                player[1].setPower(0, 0);
            }
            break;
        default:
            break;
        
        }
        break; 
    default:
     
        break;

    }
    

    //player.setPower(xpower, ypower);  // 현재 속도 설정

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
    HINSTANCE prevInstance,
    PSTR cmdLine,
    int showCmd)
{
    srand(static_cast<unsigned int>(time(NULL)));

    if (!d3d::InitD3D(hinstance,
        Width, Height, true, D3DDEVTYPE_HAL, &Device))
    {
        ::MessageBox(0, "InitD3D() - FAILED", 0, 0);
        return 0;
    }

    if (!Setup())
    {
        ::MessageBox(0, "Setup() - FAILED", 0, 0);
        return 0;
    }
    //Display가 계속해서 실행됨
    d3d::EnterMsgLoop(Display);

    Cleanup();

    Device->Release();

    return 0;
}