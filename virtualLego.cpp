////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: ��â�� Chang-hyeon Park, 
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
// Direct3D ��ġ ������
IDirect3DDevice9* Device = NULL;

// ������ ũ�� ����
const int Width = 1024;
const int Height = 768;

// ��(ball)�� ������ �ʱ� ��ġ ����
const float spherePos[4][2] = { {-2.7f,0}, {+2.4f,0}, {3.3f,0}, {-2.7f,-0.9f} };
// ���� ���� ����
const D3DXCOLOR sphereColor[4] = { d3d::RED, d3d::RED, d3d::YELLOW, d3d::WHITE };

// -----------------------------------------------------------------------------
// ��ȯ ��� ���� (����, ��, ����)
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

// �� ������ �� ��Ÿ ��� ����
#define M_RADIUS 0.29   // �� ������
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 1 // �ӵ� ������ 0.9982
#define PLANE_X 6   //�������� x
#define PLANE_Y 9   //�������� Y
#define WALL_THICKNESS 0.3  //���� �β�

//���� �����̰� �������� true, ���� �߻��ϱ� ���� false
boolean gameRound = false;
//�̹� �Ķ� ���� �ε������� �ι� �������� �浹�� �Ǹ鼭 ������ �ʰ� �ϱ� ���� ����
//�ٸ� ������Ʈ�� �ε����� alreadyHit�� �ٽ� false�� �ȴ�.
boolean alreadyHit = false;

// -----------------------------------------------------------------------------
// ��(CSphere) Ŭ���� ����
// -----------------------------------------------------------------------------
class CSphere {
private:
    float center_x, center_y, center_z; // ���� �߽� ��ǥ
    float m_radius;                     // �� ������
    float m_velocity_x;                 // x�� �ӵ�
    float m_velocity_z;                 // z�� �ӵ�
    float playerSpeed = 1.5f;   //�⺻���� �����ϴ� �÷��̾� ���ǵ�
    int playerIndexX;
    int playerIndexY;
public:
    // ������: ���� �ʱ� ���¸� ����
    CSphere(void) {
        D3DXMatrixIdentity(&m_mLocal); // ���� ��ȯ ��� �ʱ�ȭ
        ZeroMemory(&m_mtrl, sizeof(m_mtrl)); // ���� ���� �ʱ�ȭ
        m_radius = 0;
        m_velocity_x = 0;
        m_velocity_z = 0;
        m_pSphereMesh = NULL;          // �޽� �ʱ�ȭ
    }

    

    ~CSphere(void) {}

    // �� ��ü ����
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE) {
        if (NULL == pDevice) return false;

        // �� ���� ����
        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        // �� �޽� ����
        if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
            return false;

        return true;
    }

    // �� ��ü ����
    void destroy(void) {
        if (m_pSphereMesh != NULL) {
            m_pSphereMesh->Release();
            m_pSphereMesh = NULL;
        }
    }

    // �� �׸���
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld) {
        if (NULL == pDevice) return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld); // ���� ��ȯ ����
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal); // ���� ��ȯ ����
        pDevice->SetMaterial(&m_mtrl); // ���� ����
        m_pSphereMesh->DrawSubset(0); // �޽� �׸���
    }

    //�� �������� Ȯ��
    bool isBallOut(CSphere& ball) {
        D3DXVECTOR3 center = this->getCenter();
        if (center.z < -PLANE_Y / 2.0f - 0.5f) {
            this->setPower(0, 0);
            alreadyHit = false;
            gameRound = false;
            
            return true;
        }
        return false;
    }

    // �� ���� �浹�ߴ��� Ȯ��
    bool hasIntersected(CSphere& ball) {
        D3DXVECTOR3 center1 = this->getCenter(); 
        D3DXVECTOR3 center2 = ball.getCenter(); 

        float dx = center1.x - center2.x;
        float dz = center1.z - center2.z;
        
        float distance = sqrt(dx * dx + dz * dz);

        float radiusSum = this->getRadius() + ball.getRadius();

        
        
        return distance <= radiusSum;
        
    }

    // �浹 ó��
    boolean hitBy(CSphere& ball) {
        if (!this->hasIntersected(ball)) return false;
        // �浹 ���� ���� ���
        D3DXVECTOR3 center1 = this->getCenter();
        D3DXVECTOR3 center2 = ball.getCenter();

        D3DXVECTOR3 collisionNormal;
        collisionNormal.x = center1.x - center2.x;
        collisionNormal.z = center1.z - center2.z;

        // �浹 ���� ���� ����ȭ
        float length = sqrt(collisionNormal.x * collisionNormal.x + collisionNormal.z * collisionNormal.z);
        collisionNormal.x /= length;
        collisionNormal.z /= length;

        // ���� �ӵ�
        D3DXVECTOR3 velocity(ball.getVelocity_X(), 0, ball.getVelocity_Z());

        // �ӵ� ���Ϳ� �浹 ������ ������꿡 ���� ����
        float dotProduct = velocity.x * collisionNormal.x + velocity.z * collisionNormal.z;

        // ���� �ӵ����� �浹������ ������ �ι� ����
        D3DXVECTOR3 reflectedVelocity;
        reflectedVelocity.x = velocity.x - 2 * dotProduct * collisionNormal.x;
        reflectedVelocity.z = velocity.z - 2 * dotProduct * collisionNormal.z;

        // ���ο� �ӵ� ����
        ball.setPower(reflectedVelocity.x, reflectedVelocity.z);
        return true;
    }

    // �� ��ġ�� �ӵ� ������Ʈ
    void ballUpdate(float timeDiff) {
        const float TIME_SCALE = 3.3; // �ð� ������ ����
        D3DXVECTOR3 cord = this->getCenter(); // ���� ���� �߽� ��ǥ
        double vx = abs(this->getVelocity_X());
        double vz = abs(this->getVelocity_Z());

        if (vx > 0.01 || vz > 0.01) {
            float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
            float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

            
            if (tX <= -4.5f + M_RADIUS) {
                tX = -4.5 + M_RADIUS;
            }
            else if (tX >= 4.5 - M_RADIUS) {
                tX = 4.5 - M_RADIUS;
            }
            if (tZ <= -4.5f + M_RADIUS) {
                tZ = -4.5 + M_RADIUS;
            }
            else if (tZ >= 4.5 - M_RADIUS) {
                tZ = 4.5 - M_RADIUS;
            }
            this->setCenter(tX, cord.y, tZ); // ���ο� ��ġ ����
        }
        else {
            this->setPower(0, 0); // �ӵ��� �ſ� ������ ����
        }

        double rate = 1 - (1 - DECREASE_RATE) * timeDiff * 400; 
        if (rate < 0) rate = 0;
        this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);
    }

    // �ӵ� �� ��ġ ���� �޼���
    double getVelocity_X() { return this->m_velocity_x; }
    double getVelocity_Z() { return this->m_velocity_z; }
    void setPower(double vx, double vz) { this->m_velocity_x = vx; this->m_velocity_z = vz; }
    void setCenter(float x, float y, float z) {
        D3DXMATRIX m;
        center_x = x; center_y = y; center_z = z;
        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }
    void setPlayerSpeed(float newSpeed) { this->playerSpeed = newSpeed; }
    double getPlayerSpeed() { return this->playerSpeed; }
    float getRadius(void) const { return (float)(M_RADIUS); }
    const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
    D3DXVECTOR3 getCenter(void) const {
        return D3DXVECTOR3(center_x, center_y, center_z);
    }
    void updatePlayerIndex() {
        //0,0ĭ�� -4.5, 4.5�� ���⼭ �÷��̾� �������� ���� ������ �������� �ϴ°� ������
        this->playerIndexX = (int)((this->center_x + 4.5f) / 0.6f);
        this->playerIndexY = (int)((4.5f - this->center_y) / 0.6f);
        
        string message = "x: " + to_string(this->playerIndexX) + " y: " + to_string(this->playerIndexY);
        OutputDebugString((message + "\n").c_str());
    }

private:
    D3DXMATRIX m_mLocal;    // ���� ��ȯ ���
    D3DMATERIAL9 m_mtrl;    // ���� ����
    ID3DXMesh* m_pSphereMesh; // �޽� ������
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

    int hasIntersected(CSphere& ball)
    {
        
        //���ΰ� �� ����, center y + ������ + �β�/2�� y�� ������ 
        //���� �׳� plane�� �������� x�� y�� Ư����ġ�� �����ߴ��� Ȯ���ϸ��
        
        D3DXVECTOR3 center = ball.getCenter();
        
        if (center.z + M_RADIUS >= PLANE_Y / 2.0f) {//���� ��
            ball.setCenter(center.x, center.y, PLANE_Y / 2.0f - M_RADIUS);
            return 1;
        }
        else if (center.x + M_RADIUS  > PLANE_X / 2.0f) {//�� ������
            ball.setCenter(PLANE_X / 2.0f - M_RADIUS, center.y, center.z);
            return 2;
        }
        else if (center.x - M_RADIUS < -PLANE_X / 2.0f) {//�� ����
            ball.setCenter(-PLANE_X / 2.0f + M_RADIUS, center.y, center.z);
            return 3;
        }



        //return distance <= radiusSum;
        // Insert your code here.
        return 0;
    }

    void hitBy(CSphere& ball)
    {
        int direction = hasIntersected(ball);
        if ( direction == 0) {
            
        }
        else if (direction == 1) {
            ball.setPower(ball.getVelocity_X(), -ball.getVelocity_Z());
            
            alreadyHit = false;
        }
        else{
            ball.setPower(-ball.getVelocity_X(), ball.getVelocity_Z());
            alreadyHit = false;
        }
        
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


CLight   g_light;

CWall boundaryLineByX[14];//15x15������ ���δ��ݼ�
CWall boundaryLineByY[14];//15x15������ ���δ��ݼ�



double g_camera_pos[3] = { 0.0, 5.0, -8.0 };
CSphere player[2];//�÷��̾� 1p,2p
CSphere   g_target_blueball;

int ss_y = 14;
int ss_x = 12;
int validCount = 0;






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

    //15x15 ������ ����� ��
    // create plane and set the position
    if (false == g_legoPlane.create(Device, -1, -1, 9, 0.03f, 9, d3d::WHITE)) return false;
    g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

    // create walls and set the position. note that there are four walls
    if (false == g_legowall[0].create(Device, -1, -1, 9, WALL_THICKNESS, 0.12f, d3d::DARKRED)) return false;
    g_legowall[0].setPosition(0.0f, 0.12f, 4.56f);
    if (false == g_legowall[1].create(Device, -1, -1, 0.12f, WALL_THICKNESS, 9, d3d::DARKRED)) return false;
    g_legowall[1].setPosition(-4.56f, 0.12f, 0.0f);
    
    
    // Device ,float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE
    //���α���, �� ����, ���α���
    if (false == g_legowall[2].create(Device, -1, -1, 0.12f, WALL_THICKNESS,9, d3d::DARKRED)) return false;
    g_legowall[2].setPosition(4.56f, 0.12f, 0.0f);
    if (false == g_legowall[3].create(Device, -1, -1, 9, WALL_THICKNESS, 0.12f, d3d::DARKRED)) return false;
    g_legowall[3].setPosition(0.0f, 0.12f, -4.56f);

    // create four balls and set the position

    //��輱 �׸���
    //��ĭ ���� ������ 0.6�̴�
    //��ĭ�� �� 0.6*0.6
    for (int i = 0; i < 14; i++) {
        if (false == boundaryLineByX[i].create(Device, -1, -1, 0.01f, 0.01f, 9, d3d::BLACK)) return false;
        boundaryLineByX[i].setPosition(-4.5f + 0.6 + 0.6*i, 0.12f, 0.0f);

        if (false == boundaryLineByY[i].create(Device, -1, -1, 9, 0.01f, 0.01f, d3d::BLACK)) return false;
        boundaryLineByY[i].setPosition(0.0f, 0.12f, -4.5f + 0.6 + 0.6 * i);
    }
    
    

    

    

    //�÷��̾� 1 ����
    if (false == player[0].create(Device, d3d::RED)) return false;
    player[0].setCenter(.0f, (float)M_RADIUS, -3.5f);
    player[0].setPower(0, 0);
    //�÷��̾� 2 ���� 
    if (false == player[1].create(Device, d3d::BLUE)) return false;
    player[1].setCenter(4.5f, (float)M_RADIUS, -3.5f);
    player[1].setPower(0, 0);
    // create blue ball for set direction
    if (false == g_target_blueball.create(Device, d3d::WHITE)) return false;
    g_target_blueball.setCenter(.0f, (float)M_RADIUS, -4.0f);

    // light setting 
    D3DLIGHT9 lit;
    ::ZeroMemory(&lit, sizeof(lit));
    lit.Type = D3DLIGHT_POINT;
    lit.Diffuse = d3d::WHITE;
    lit.Specular = d3d::WHITE * 0.9f;
    lit.Ambient = d3d::WHITE * 0.9f;
    lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
    lit.Range = 100.0f;
    lit.Attenuation0 = 0.0f;
    lit.Attenuation1 = 0.9f;
    lit.Attenuation2 = 0.0f;
    if (false == g_light.create(Device, lit))
        return false;

    

    //ī�޶� ��ġ �����ҵ�?
    // Position and aim the camera.
    //z�� �� �������� �ϴ¤� 
    D3DXVECTOR3 pos(0.0f, 13.0f, -0.0001f);
    D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 up(0.0f, 0.1f, 0.0f);
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
// timeDelta�� ���� �̹����� ������ �̹����� �����ӻ��̸� ��Ÿ����.
// the distance of moving balls should be "velocity * timeDelta"
// ������ ������ �Ÿ��� velocity * timeDelta�̴�.
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
        
        g_target_blueball.draw(Device, g_mWorld);
        //g_light.draw(Device);
        
        player[0].ballUpdate(timeDelta);
        player[0].draw(Device, g_mWorld);

        player[1].ballUpdate(timeDelta);
        player[1].draw(Device, g_mWorld);

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
    static float xpower = 0.0f, ypower = 0.0f;  // ���� �̵� �ӵ�
    static bool keyStates[256] = { false };
    double playerOneSp = player[0].getPlayerSpeed();
    double playerTwoSp = player[1].getPlayerSpeed();
    //�߿��Ѱ� �̼Ӿ������� ������ ������ �����Ѱ�?
    //getVelocity ���ͼ� �������ָ� �ɵ�?
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
            if (player[0].getVelocity_X() == 0) {//a�� dŰ ������ ������ �Ⱥ��ϰ�
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
        case VK_UP:
            if (player[1].getVelocity_X() == 0) {//a�� dŰ ������ ������ �Ⱥ��ϰ�
                player[1].setPower(0, 0);
            }
            break;
        case VK_DOWN:
            if (player[1].getVelocity_X() == 0) {//a�� dŰ ������ ������ �Ⱥ��ϰ�
                player[1].setPower(0, 0);
            }
            break;
        case VK_LEFT:
            if (player[1].getVelocity_Z() == 0) {//a�� dŰ ������ ������ �Ⱥ��ϰ�
                player[1].setPower(0, 0);
            }
            break;

        case VK_RIGHT:
            if (player[1].getVelocity_Z() == 0) {//a�� dŰ ������ ������ �Ⱥ��ϰ�
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
    

    //player.setPower(xpower, ypower);  // ���� �ӵ� ����

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
    //Display�� ����ؼ� �����
    d3d::EnterMsgLoop(Display);

    Cleanup();

    Device->Release();

    return 0;
}