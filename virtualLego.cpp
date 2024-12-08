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
#define BOX_LENGTH 0.5f  // 상자 길이

//맵 배치에 사용
int map[15][15] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
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
//일단 폭탄은 2로 설정


//게임상태 정의
enum GameState {
    STATE_MENU,    //인터페이스
    STATE_GAME,    //게임중
    STATE_GAMEOVER //게임오버
};

GameState g_GameState = STATE_MENU;

void destroyItemBoxAt(int x, int z);



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

    int e_indexX;
    int e_indexZ;

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

    bool checkExp(int x, int y) {
        if (e_isActive) {
            if (e_indexX == x && e_indexZ == y) {
                return true;
            }
            return false;
        }
        return false;
    }

    bool getActive() {
        return e_isActive;
    }

    void setIndex(int x, int z) {
        this->e_indexX = x;
        this->e_indexZ = z;
    }
};

class CBoom : public CSphere {
private:
    float b_time;
    bool b_isActive = false;
    float b_setTime = 2;
    int b_numOfBoom = 1;
    const int b_max = 10;

    int b_indexX = 0;
    int b_indexZ = 0;
    int b_range = 2;
    CExplosion explosion[9];
    int numOfExp = 9;

    bool player1 = false;
    bool player2 = false;


public:
    //Explosion 생성
    bool createExplosion(IDirect3DDevice9* pDevice) {
        for (int i = 0; i < numOfExp; i++) {
            if (false == explosion[i].create(pDevice, -1, -1, 0.6f, 0.1f, 0.6f, d3d::RED)) return false;
        }
        return true;
    }

    //키 눌렀을 때
    void pressKey(float sx, float py, float sz) {
        if (!b_isActive) {
            player1 = false;
            player2 = false;
            this->b_isActive = true;
            setCenter(sx, py, sz);
            this->b_time = 0;
            map[b_indexZ][b_indexX] = 2;
        }
    }


    void boomUpdate(float timeDelta) {
        if (!b_isActive) {
            return;
        }

        b_time += timeDelta;
        if (b_time >= b_setTime) {
            map[b_indexZ][b_indexX] = 0;
            b_isActive = false;

            explosion[0].activate(-4.2 + 0.6 * b_indexX, 0, 4.2 - 0.6 * b_indexZ); // 폭탄 위치
            explosion[0].setIndex(b_indexX, b_indexZ);

            for (int i = 0; i < b_range; i++) {
                if (b_indexX + i + 1 < 15) {
                    if (map[b_indexZ][b_indexX + i + 1] == 1) {
                        destroyItemBoxAt(b_indexX + i + 1, b_indexZ);  // 아이템 상자 삭제
                        break;
                    }
                    explosion[1 + i * 4].activate(-4.2 + 0.6 * (b_indexX + i + 1), 0, 4.2 - 0.6 * b_indexZ); // X +
                    explosion[1 + i * 4].setIndex(b_indexX + i + 1, b_indexZ);
                }
            }

            for (int i = 0; i < b_range; i++) {
                if (b_indexX - i - 1 >= 0) {
                    if (map[b_indexZ][b_indexX - i - 1] == 1) {
                        destroyItemBoxAt(b_indexX - i - 1, b_indexZ);  // 아이템 상자 삭제
                        break;
                    }
                    explosion[2 + i * 4].activate(-4.2 + 0.6 * (b_indexX - i - 1), 0, 4.2 - 0.6 * b_indexZ); // X -
                    explosion[2 + i * 4].setIndex(b_indexX - i - 1, b_indexZ);
                }
            }

            for (int i = 0; i < b_range; i++) {
                if (b_indexZ + i + 1 < 15) {
                    if (map[b_indexZ + i + 1][b_indexX] == 1) {
                        destroyItemBoxAt(b_indexX, b_indexZ + i + 1);  // 아이템 상자 삭제
                        break;
                    }
                    explosion[3 + i * 4].activate(-4.2 + 0.6 * b_indexX, 0, 4.2 - 0.6 * (b_indexZ + i + 1)); // Z +
                    explosion[3 + i * 4].setIndex(b_indexX, b_indexZ + i + 1);
                }
            }

            for (int i = 0; i < b_range; i++) {
                if (b_indexZ - i - 1 >= 0) {
                    if (map[b_indexZ - i - 1][b_indexX] == 1) {
                        destroyItemBoxAt(b_indexX, b_indexZ - i - 1);  // 아이템 상자 삭제
                        break;
                    }
                    explosion[4 + i * 4].activate(-4.2 + 0.6 * b_indexX, 0, 4.2 - 0.6 * (b_indexZ - i - 1)); // Z -
                    explosion[4 + i * 4].setIndex(b_indexX, b_indexZ - i - 1);
                }
            }

        }
    }

    void updateExplosions(float timeDelta) {
        for (int i = 0; i < 9; ++i) {
            explosion[i].explosionUpdate(timeDelta);
        }
    }

    bool checkExplosion2(int x, int y) {
        if (!player1) {
            for (int i = 0; i < 9; i++) {
                if (explosion[i].checkExp(x, y)) {
                    player1 = true;
                    return true;
                }
            }
        }
        return false;
    }

    bool checkExplosion1(int x, int y) {
        if (!player2) {
            for (int i = 0; i < 9; i++) {
                if (explosion[i].checkExp(x, y)) {
                    player2 = true;
                    return true;
                }
            }
        }
        return false;
    }

    void drawExplosions(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld) {
        for (int i = 0; i < 9; ++i) {
            if (explosion[i].getActive()) {
                explosion[i].draw(pDevice, mWorld);
            }
        }
    }

    //active 값 반환
    bool getActive() {
        return b_isActive;
    }

    //아이템 먹고 폭탄 개수 늘려주기
    void setNumOfBoom(int numOfBoom) {
        if (numOfBoom <= b_max) {
            b_numOfBoom = numOfBoom;
        }
    }

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
    int bombRange = 2;//폭탄 범위
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
        //OutputDebugString((message + "\n").c_str());
    }



    int getPlayerIndexX() {
        return playerIndexX;
    }

    int getPlayerIndexY() {
        return playerIndexY;
    }

    double getPlayerSpeed() { return this->playerSpeed; }

    int getPlayerLife() {
        return this->playerLife;
    }

    void setPlayerLife() {
        this->playerLife -= 1;
        char buffer[100];
        sprintf(buffer, "Player Life: %d\n", this->playerLife);
        OutputDebugString(buffer);
        if (this->playerLife > 0) { // 목숨은 음수가 될 수 없음

        }
        else {
            //게임오버 실행
        }
    }

    //플레이어 리셋
    void resetPlayer() {
        this->playerLife = 3;
        this->bombRange = 1;
        this->bombCap = 1;
        this->playerSpeed = 1.5f;
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
        return this->bombCap;
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
                    if (map[indexY][indexX - 1] >= 1) {//박스나 폭탄 지정하는 상수면 이동불가
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
                    if (map[indexY - 1][indexX] >= 1) {//박스나 폭탄 지정하는 상수면 이동불가
                        //-4.2 + 0.6 * (indexX-1)가 상자의 center
                        if (tZ >= 4.2 - 0.6 * (indexY - 1) - 0.15f - 0.275f) {
                            tZ = 4.2 - 0.6 * (indexY - 1) - 0.15f - 0.275f;
                        }
                    }
                }
            }
            else if (m_velocity_z < 0) {//하
                if (indexY <= 13) {//14안넘게
                    if (map[indexY + 1][indexX] >= 1) {//박스나 폭탄 지정하는 상수면 이동불가
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

class ItemBox {
private:
    float m_length;  // 정육면체 한 변의 길이
    D3DXMATRIX m_mLocal;  // 로컬 변환 행렬
    D3DMATERIAL9 m_mtrl;   // 재질 정보
    ID3DXMesh* m_pBoxMesh; // 정육면체 메쉬 포인터

    int m_indexX, m_indexZ;  // 해당 상자의 X, Z 좌표
public:
    // 생성자
    ItemBox() : m_length(BOX_LENGTH), m_pBoxMesh(nullptr) {
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        D3DXMatrixIdentity(&m_mLocal); // 로컬 변환 행렬 초기화
    }

    // 정육면체 생성
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f)) {
        if (pDevice == nullptr) return false;

        // 정육면체 재질 설정
        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
        m_mtrl.Power = 5.0f;

        // 정육면체 메쉬 생성
        if (FAILED(D3DXCreateBox(pDevice, m_length, m_length, m_length, &m_pBoxMesh, nullptr))) {
            return false;
        }

        return true;
    }

    // 정육면체 그리기
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld) {
        if (pDevice == nullptr || m_pBoxMesh == nullptr) return;

        pDevice->SetTransform(D3DTS_WORLD, &mWorld);  // 월드 변환 설정
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);  // 로컬 변환 적용
        pDevice->SetMaterial(&m_mtrl);  // 재질 설정
        m_pBoxMesh->DrawSubset(0);  // 메쉬 그리기
    }

    // 위치 설정
    void setPosition(float x, float y, float z) {
        D3DXMATRIX m;
        D3DXMatrixTranslation(&m, x, y, z);  // 주어진 위치로 이동
        m_mLocal = m;  // 로컬 변환 행렬 설정
    }

    // 위치 및 변환 관련 메서드
    const D3DXMATRIX& getLocalTransform() const {
        return m_mLocal;
    }

    // 정육면체의 한 변의 길이 반환
    float getLength() const {
        return m_length;
    }

    // 정육면체 메쉬 삭제
    void destroy() {
        if (m_pBoxMesh) {
            m_pBoxMesh->Release();
            m_pBoxMesh = nullptr;
        }

        // 해당 좌표의 map 값을 0으로 설정
        if (m_indexX >= 0 && m_indexZ >= 0) {
            map[m_indexZ][m_indexX] = 0;  // 좌표 위치에서 map 값 제거
        }
    }

    void setIndex(float x, float z) {
        m_indexX = x;
        m_indexZ = z;
    }

    int getIndexX() const {
        return m_indexX;
    }

    int getIndexZ() const {
        return m_indexZ;
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

CONST int MAX_BOOM = 5;
CBoom b_player1[MAX_BOOM];
CBoom b_player2[MAX_BOOM];

ID3DXFont* g_pFont = NULL; //폰트1 변수
ID3DXFont* g_pFontLarge = NULL; //폰트2 변수

// 아이템 상자
ItemBox itembox;
ItemBox* itemMap[15][15] = { nullptr };
int itemBox_num = 20;   // 전체 아이템 상자 개수


//게임 초기화 함수들(게임 오버됐을때 다시 시작)

//맵 초기화 
//초기 맵배열
const int initialMap[15][15] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
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
//맵을 초기 상태로 재설정
void resetMap() {
    for (int y = 0; y < 15; y++) {
        for (int x = 0; x < 15; x++) {
            map[y][x] = initialMap[y][x];
        }
    }
}

//폭탄상태 초기화
void resetBombs() {
    for (int i = 0; i < MAX_BOOM; i++) {
        // 플레이어 1의 폭탄 초기화
        b_player1[i].destroy(); // 기존 폭탄 제거
        b_player1[i].create(Device, d3d::BLACK);
        b_player1[i].createExplosion(Device);

        // 플레이어 2의 폭탄 초기화
        b_player2[i].destroy(); // 기존 폭탄 제거
        b_player2[i].create(Device, d3d::GREEN);
        b_player2[i].createExplosion(Device);
    }
}


//위치 초기화
void resetPlayerPositions() {
    // 플레이어 1
    player[0].setCenter(0.0f, (float)P_RADIUS + 0.5f, -3.5f);
    player[0].setPower(0, 0);

    // 플레이어 2
    player[1].setCenter(4.5f, (float)P_RADIUS + 0.5f, -3.5f);
    player[1].setPower(0, 0);
}


//게임상태 초기화(total)
void resetGame() {

    player[0].resetPlayer();
    player[1].resetPlayer();

    resetMap();
    resetBombs();
    resetPlayerPositions();


}

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

void destroyAllLegoBlock(void)
{

}

// 특정 좌표의 아이템 상자 삭제
void destroyItemBoxAt(int x, int z) {
    if (x < 0 || x >= 15 || z < 0 || z >= 15) {
        return;  // 범위를 벗어난 경우 처리하지 않음
    }

    if (itemMap[z][x] != nullptr) {
        itemMap[z][x]->destroy();  // 아이템 상자 메쉬 삭제 및 map 갱신
        delete itemMap[z][x];  // 객체 메모리 해제
        itemMap[z][x] = nullptr;  // itemMap에서 해당 객체 제거
    }
}

// 모든 아이템 상자 삭제
void destroyAllItemBoxes() {
    // itemMap을 순회하면서 모든 아이템 상자 삭제
    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            if (itemMap[i][j] != nullptr) {
                itemMap[i][j]->destroy();  // 객체 내 리소스 해제
                delete itemMap[i][j];      // 객체 삭제
                itemMap[i][j] = nullptr;   // 포인터 초기화
            }
        }
    }
}

void setRandomItemBox() {
    ItemBox* itembox = new ItemBox();  // 아이템 상자 객체 생성

    float startX = -4.1f;
    float startZ = 3.8f;
    float intervalX = 0.5857f;  // X 간격
    float intervalZ = 0.5786f;  // Z 간격

    // 랜덤 (i, j) 좌표 생성
    int randomI = rand() % 15;  // 0부터 14까지의 랜덤 값 (X축)
    int randomJ = rand() % 15;  // 0부터 14까지의 랜덤 값 (Z축)

    // (randomI, randomJ)에 해당하는 정확한 좌표 계산
    float randomX = startX + randomI * intervalX;  // X 좌표
    float randomZ = startZ - randomJ * intervalZ;  // Z 좌표 (반대로 빼줘야 위에서 아래로 간다)

    itembox->create(Device, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f));  // 초록 상자
    itembox->setPosition(randomX, 0.5f, randomZ);  // 랜덤 위치 설정

    // X, Z 반대 주의!
    map[randomJ][randomI] = 1;  // map 배열에 위치 표시

    itembox->setIndex(randomI, randomJ);

    itemMap[randomJ][randomI] = itembox;  // itemMap 배열에 추가
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


    // 경계선 그리기
    // 한칸 사이 간격은 0.6이다
    // 한칸이 즉 0.6*0.6
    for (int i = 0; i < 14; i++) {

        if (false == boundaryLineByX[i].create(Device, -1, -1, 0.05f, 0.01f, 9, d3d::BLACK)) return false;
        boundaryLineByX[i].setPosition(-4.5f + 0.6 + 0.6 * i, 0.015f, 0.0f);

        if (false == boundaryLineByY[i].create(Device, -1, -1, 9, 0.05f, 0.01f, d3d::BLACK)) return false;
        boundaryLineByY[i].setPosition(0.0f, 0.015f, -4.5f + 0.6 + 0.6 * i);
    }

    // 아이템 상자 랜덤 배치
    for (int i = 0; i < itemBox_num; i++) {
        setRandomItemBox();
    }

    //플레이어 1 생성
    if (false == player[0].create(Device, d3d::RED)) return false;
    player[0].setCenter(.0f, (float)P_RADIUS + 0.5f, -3.5f);
    player[0].setPower(0, 0);

    if (false == playerBody[0].create(Device, -1, -1, 0.3f, 0.6f, 0.3f, d3d::RED)) return false;
    playerBody[0].setPosition(0.0f, 0.3f, 0.0f);


    //boom 생성
    for (int i = 0; i < MAX_BOOM; i++) {
        if (false == b_player1[i].create(Device, d3d::BLACK)) return false;
        if (false == b_player1[i].createExplosion(Device))return false;

        if (false == b_player2[i].create(Device, d3d::GREEN)) return false;
        if (false == b_player2[i].createExplosion(Device))return false;
    }


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


    //폰트1
    D3DXFONT_DESC fd;
    ZeroMemory(&fd, sizeof(fd));
    fd.Height = 24; // 글자 크기
    fd.Width = 0;
    fd.Weight = FW_BOLD;
    fd.CharSet = DEFAULT_CHARSET;
    fd.OutputPrecision = OUT_DEFAULT_PRECIS;
    fd.Quality = DEFAULT_QUALITY;
    fd.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    strcpy_s(fd.FaceName, "Arial");

    if (FAILED(D3DXCreateFontIndirect(Device, &fd, &g_pFont))) {
        MessageBox(0, "Font creation failed", 0, 0);
        return false;
    }

    //폰트2
    D3DXFONT_DESC fdLarge;
    ZeroMemory(&fdLarge, sizeof(fdLarge));
    fdLarge.Height = 48; // 글자 크기 크게
    fdLarge.Width = 0;
    fdLarge.Weight = FW_BOLD;
    fdLarge.CharSet = DEFAULT_CHARSET;
    fdLarge.OutputPrecision = OUT_DEFAULT_PRECIS;
    fdLarge.Quality = DEFAULT_QUALITY;
    fdLarge.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    strcpy_s(fdLarge.FaceName, "Arial");

    if (FAILED(D3DXCreateFontIndirect(Device, &fdLarge, &g_pFontLarge))) {
        MessageBox(0, "Large Font creation failed", 0, 0);
        return false;
    }


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
// 움직인 공의 거리는 velocity * timeDelta이다.
bool Display(float timeDelta)
{
    int i = 0;
    int j = 0;

    if (Device)
    {
        Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
        Device->BeginScene();

        // 게임 상태에 따른 화면 렌더링 분기
        switch (g_GameState) {
        case STATE_MENU:
        {
            RECT rc;
            SetRect(&rc, 0, Height / 3, Width, Height / 3 + 100);

            //제목
            if (g_pFontLarge) {
                g_pFontLarge->DrawText(NULL, "BOMBER GAME", -1, &rc,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE, D3DCOLOR_XRGB(255, 255, 0));
            }

            //시작안내
            RECT rc2;
            //가운데보다 약간 아래 부분에 표시하기 위해 top 값을 Height/4 + space
            SetRect(&rc2, 0, Height / 3 + 100, Width, Height / 4 + 200);
            if (g_pFont) {
                g_pFont->DrawText(NULL, "Press ENTER to start", -1, &rc2,
                    DT_CENTER | DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
            }
            break;
        }
        case STATE_GAME:  //게임중
        {
            for (int i = 0; i < 14; i++) {
                boundaryLineByX[i].draw(Device, g_mWorld);
                boundaryLineByY[i].draw(Device, g_mWorld);
            }

            g_legoPlane.draw(Device, g_mWorld);
            for (i = 0; i < 4; i++) {
                g_legowall[i].draw(Device, g_mWorld);
            }

            player[0].ballUpdate(timeDelta);
            player[0].draw(Device, g_mWorld);
            playerBody[0].draw(Device, g_mWorld);
            player[0].bindingPlayerBody(playerBody[0]);
            player[1].ballUpdate(timeDelta);
            player[1].draw(Device, g_mWorld);
            playerBody[1].draw(Device, g_mWorld);
            player[1].bindingPlayerBody(playerBody[1]);

            // 아이템 상자들 그리기
            for (int i = 0; i < 15; ++i) {
                for (int j = 0; j < 15; ++j) {
                    if (itemMap[i][j] != nullptr) {
                        itemMap[i][j]->draw(Device, g_mWorld);  // 각 아이템 상자 그리기
                    }
                }
            }

            //1 player's boom
            for (int i = 0; i < MAX_BOOM; i++) {
                if (b_player1[i].getActive()) {
                    b_player1[i].draw(Device, g_mWorld);
                }
                b_player1[i].boomUpdate(timeDelta);
                b_player1[i].drawExplosions(Device, g_mWorld);


                player[0].updatePlayerIndex();
                player[1].updatePlayerIndex();

                b_player1[i].updateExplosions(timeDelta);
                if (b_player1[i].checkExplosion1(player[0].getPlayerIndexX(), player[0].getPlayerIndexY())) {
                    player[0].setPlayerLife();
                }
                if (b_player1[i].checkExplosion2(player[1].getPlayerIndexX(), player[1].getPlayerIndexY())) {
                    player[1].setPlayerLife();
                }

                //2 player's boom
                if (b_player2[i].getActive()) {
                    b_player2[i].draw(Device, g_mWorld);
                }
                b_player2[i].boomUpdate(timeDelta);
                b_player2[i].drawExplosions(Device, g_mWorld);


                player[0].updatePlayerIndex();
                player[1].updatePlayerIndex();

                b_player2[i].updateExplosions(timeDelta);
                if (b_player2[i].checkExplosion1(player[0].getPlayerIndexX(), player[0].getPlayerIndexY())) {
                    player[0].setPlayerLife();
                }
                if (b_player2[i].checkExplosion2(player[1].getPlayerIndexX(), player[1].getPlayerIndexY())) {
                    player[1].setPlayerLife();
                }
            }


            // 플레이어 라이프 표시
            if (g_pFontLarge) {
                RECT lifeRect;
                SetRect(&lifeRect, 10, 10, 0, 0); // 왼쪽 상단 근처
                char lifeText[128];
                sprintf_s(lifeText, "P1 Life: %d", player[0].getPlayerLife());
                // 플레이어 1 목숨: 빨간색
                g_pFontLarge->DrawText(NULL, lifeText, -1, &lifeRect, DT_NOCLIP, D3DCOLOR_XRGB(255, 0, 0));

                SetRect(&lifeRect, 10, 70, 0, 0); // P1 목숨 아래쪽에 P2 목숨 표시
                sprintf_s(lifeText, "P2 Life: %d", player[1].getPlayerLife());
                // 플레이어 2 목숨: 파란색
                g_pFontLarge->DrawText(NULL, lifeText, -1, &lifeRect, DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 255));
            }

            // 플레이어 목숨 검사 후 STATE_GAMEOVER 전환 로직
            if (player[0].getPlayerLife() <= 0 || player[1].getPlayerLife() <= 0) {
                g_GameState = STATE_GAMEOVER;
            }

            break;
        }
        case STATE_GAMEOVER: //게임오버화면
        {
            bool player1Won = (player[0].getPlayerLife() > 0);
            bool player2Won = (player[1].getPlayerLife() > 0);
            const char* winnerText = player1Won ? "Player1 Wins!" : "Player2 Wins!";
            D3DCOLOR winnerColor = player1Won ? D3DCOLOR_XRGB(255, 0, 0) : D3DCOLOR_XRGB(0, 0, 255);

            RECT rc;
            SetRect(&rc, 0, Height / 2 - 50, Width, Height / 2 + 50);
            if (g_pFontLarge) {
                g_pFontLarge->DrawText(NULL, winnerText, -1, &rc,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE, winnerColor);
            }
            RECT rc2;
            SetRect(&rc2, 0, Height / 2 + 60, Width, Height / 2 + 120);
            if (g_pFont) {
                g_pFont->DrawText(NULL, "Press ENTER to restart", -1, &rc2,
                    DT_CENTER | DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
            }

            break;
        }
        }

        //b_player1.boomUpdate(timeDelta);
        ////boom의 active 값이 true인 경우에만 보이도록 설정
        //b_player1.drawExplosions(Device, g_mWorld);
        //player[0].updatePlayerIndex();
        //bool p1 = b_player1.updateExplosions(timeDelta,player[0].getPlayerIndexX(),player[0].getPlayerIndexY());
        //if (p1) {
        //    player[0].setPlayerLife();
        //}

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
            if (g_GameState == STATE_MENU) {
                g_GameState = STATE_GAME;
            }
            else if (g_GameState == STATE_GAMEOVER) {
                g_GameState = STATE_GAME;
                resetGame(); // 게임 상태 초기화
            }
            break;
        case 'W':
            player[0].setPower(0, playerOneSp);
            break;
        case 'A':
            player[0].setPower(-playerOneSp, 0);
            break;
        case 'S':
            player[0].setPower(0, -playerOneSp);
            break;

        case 'D':
            player[0].setPower(playerOneSp, 0);
            break;

        case 'F':
            player[0].updatePlayerIndex();
            for (int i = 0; i < player[0].getBombCap(); i++) {
                if (!b_player1[i].getActive()) {
                    b_player1[i].setIndexXY(player[0].getPlayerIndexX(), player[0].getPlayerIndexY());
                    b_player1[i].pressKey(-4.2 + 0.6 * player[0].getPlayerIndexX(), M_RADIUS - 0.1, 4.2 - 0.6 * player[0].getPlayerIndexY());
                    break;
                }
            }
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
        case 'L':
            player[1].updatePlayerIndex();
            for (int i = 0; i < player[1].getBombCap(); i++) {
                if (!b_player2[i].getActive()) {
                    b_player2[i].setIndexXY(player[1].getPlayerIndexX(), player[1].getPlayerIndexY());
                    b_player2[i].pressKey(-4.2 + 0.6 * player[1].getPlayerIndexX(), M_RADIUS - 0.1, 4.2 - 0.6 * player[1].getPlayerIndexY());
                    break;
                }
            }
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
