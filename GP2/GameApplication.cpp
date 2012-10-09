#include "GameApplication.h"

struct Vertex
{
	D3DXVECTOR3 Pos;
	D3DXCOLOR color;
	D3DXVECTOR2 texCoords;
};

CGameApplication::CGameApplication(void)
{
	m_pWindow=NULL;
	m_pD3D10Device=NULL;
	m_pRenderTargetView=NULL;
	m_pSwapChain=NULL;
	m_pVertexBuffer=NULL;
	m_pDepthStencilView=NULL;
	m_pDepthStencilTexture=NULL;
	m_pDiffuseTexture=NULL;

}

CGameApplication::~CGameApplication(void)
{
	if (m_pD3D10Device)
		m_pD3D10Device->ClearState();
	if (m_pDiffuseTexture)
		m_pDiffuseTexture->Release();
	
	if (m_pVertexBuffer)
		m_pVertexBuffer->Release();
	if (m_pVertexLayout)
		m_pVertexLayout->Release();

	if (m_pEffect)
		m_pEffect->Release();

	if (m_pRenderTargetView)
		m_pRenderTargetView->Release();
	if (m_pDepthStencilTexture)
		m_pDepthStencilTexture->Release();
	if (m_pDepthStencilView)
		m_pDepthStencilView->Release();
	if (m_pSwapChain)
		m_pSwapChain->Release();
	if (m_pD3D10Device)
		m_pD3D10Device->Release();

	if (m_pWindow)
	{
		delete m_pWindow;
		m_pWindow=NULL;
	}
}

bool CGameApplication::init()
{
	if (!initWindow())
		return false;

	if (!initGraphics())
		return false;

	if (!initGame())
		return false;

	return true;
}

bool CGameApplication::initGame()
{
	DWORD dwShaderFlags=D3D10_SHADER_ENABLE_STRICTNESS;
#if defined(DEBUG)||defined(_DEBUG)
	dwShaderFlags|=D3D10_SHADER_DEBUG;
#endif

	if(FAILED(D3DX10CreateEffectFromFile(TEXT("Texture.fx"),NULL,NULL,"fx_4_0",dwShaderFlags,0,m_pD3D10Device,NULL,NULL,&m_pEffect,NULL,NULL)))
	{
		MessageBox(NULL,TEXT("The FX file cannot be located. Please run this executable from the directory that contains the FX file."),TEXT("Error"),MB_OK);
		return false;
	}
	m_pTechnique=m_pEffect->GetTechniqueByName("Render");

	D3D10_BUFFER_DESC bd;
	bd.Usage=D3D10_USAGE_DEFAULT;
	bd.ByteWidth=sizeof(Vertex)*4;
	bd.BindFlags=D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags=0;
	bd.MiscFlags=0;

	D3D10_BUFFER_DESC indexBufferDesc; //instance of index buffer
	indexBufferDesc.Usage=D3D10_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth=sizeof(int)*6; //need changed
	indexBufferDesc.BindFlags=D3D10_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags=0;
	indexBufferDesc.MiscFlags=0;

	Vertex vertices[]=
	{
		{D3DXVECTOR3(-0.5f,0.5f,0.5f),D3DXCOLOR(0.0f,0.0f,1.0f,0.0f),D3DXVECTOR2(0.0f,0.0f)},//top left
		{D3DXVECTOR3(0.5f,-0.5f,0.5f),D3DXCOLOR(0.0f,1.0f,0.0f,0.0f),D3DXVECTOR2(1.0f,1.0f)},//bottom right
		{D3DXVECTOR3(-0.5f,-0.5f,0.5f),D3DXCOLOR(1.0f,0.0f,0.0f,0.0f),D3DXVECTOR2(0.0f,1.0f)},//bottom left
		{D3DXVECTOR3(0.5f,0.5f,0.5f),D3DXCOLOR(1.0f,1.0f,0.0f,0.0f),D3DXVECTOR2(1.0f,0.0f)},//top right
	};

	int indices[]={0,1,2,0,3,1, //front
				};

	D3D10_SUBRESOURCE_DATA InitData;
	InitData.pSysMem=vertices;

	D3D10_SUBRESOURCE_DATA IndexBufferInitialData;
	IndexBufferInitialData.pSysMem=indices;

	if(FAILED(m_pD3D10Device->CreateBuffer(&bd,&InitData,&m_pVertexBuffer)))
		return false;

	if(FAILED(m_pD3D10Device->CreateBuffer(&indexBufferDesc,&IndexBufferInitialData,&m_pIndexBuffer)))
		return false;

	D3D10_INPUT_ELEMENT_DESC layout[]=
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D10_INPUT_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D10_INPUT_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,28,D3D10_INPUT_PER_VERTEX_DATA,0},
	};

	UINT numElements=sizeof(layout)/sizeof(D3D10_INPUT_ELEMENT_DESC);
	D3D10_PASS_DESC PassDesc;
	m_pTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);

	if(FAILED(m_pD3D10Device->CreateInputLayout(layout,numElements,PassDesc.pIAInputSignature,PassDesc.IAInputSignatureSize,&m_pVertexLayout)))
	{
		return false;
	}

	m_pD3D10Device->IASetInputLayout(m_pVertexLayout);

	UINT stride=sizeof(Vertex);
	UINT offset=0;
	m_pD3D10Device->IASetVertexBuffers(0,1,&m_pVertexBuffer,&stride,&offset);

	m_pD3D10Device->IASetIndexBuffer(m_pIndexBuffer,DXGI_FORMAT_R32_UINT,0);

	m_pD3D10Device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DXVECTOR3 cameraPos(0.0f,0.0f,-4.0f);
	D3DXVECTOR3 cameraLook(0.0f,0.0f,1.0f);
	D3DXVECTOR3 cameraUp(0.0f,1.0f,0.0f);
	D3DXMatrixLookAtLH(&m_matView,&cameraPos,&cameraLook,&cameraUp);

	D3D10_VIEWPORT vp;
	UINT numViewPorts=1;
	m_pD3D10Device->RSGetViewports(&numViewPorts,&vp);

	D3DXMatrixPerspectiveFovLH(&m_matProjection,(float)D3DX_PI * 0.25f,vp.Width/(FLOAT)vp.Height,0.1f,100.0f);

	m_pViewMatrixVariable=m_pEffect->GetVariableByName("matView")->AsMatrix();
	m_pProjectionMatrixVariable=m_pEffect->GetVariableByName("matProjection")->AsMatrix();

	m_pProjectionMatrixVariable->SetMatrix((float*)m_matProjection);

	m_vecPosition=D3DXVECTOR3(0.0f,0.0f,0.0f);
	m_vecScale=D3DXVECTOR3(1.0f,1.0f,1.0f);
	m_vecRotation=D3DXVECTOR3(0.0f,0.0f,0.0f);
	m_pWorldMatrixVariable=m_pEffect->GetVariableByName("matWorld")->AsMatrix();

	if(FAILED(D3DX10CreateShaderResourceViewFromFile(m_pD3D10Device,TEXT("face.png"),NULL,NULL,&m_pDiffuseTexture,NULL)))
	{
		MessageBox(NULL,TEXT("Can't load Texture"),TEXT("Error"),MB_OK);
		return false;
	}

	m_pDiffuseTextureVariable=m_pEffect->GetVariableByName("diffuseTexture")->AsShaderResource();

	return true;
}

bool CGameApplication::run()
{
	while(m_pWindow->running())
	{
		if (! m_pWindow->checkForWindowMessages())
		{
			update();
			render();
		}
	}
	return true;
}

void CGameApplication::render()
{
	float ClearColor[4] = {0.0f, 0.125f, 0.3f, 1.0f};
	m_pD3D10Device->ClearRenderTargetView(m_pRenderTargetView, ClearColor);
	m_pD3D10Device->ClearDepthStencilView(m_pDepthStencilView,D3D10_CLEAR_DEPTH,1.0f,0);

	m_pViewMatrixVariable->SetMatrix((float*)m_matView);

	m_pWorldMatrixVariable->SetMatrix((float*)m_matWorld);

	m_pDiffuseTextureVariable->SetResource(m_pDiffuseTexture);
	D3D10_TECHNIQUE_DESC techDesc;
	m_pTechnique->GetDesc(&techDesc);
	for(UINT p=0;p<techDesc.Passes;++p)
	{
		m_pTechnique->GetPassByIndex(p)->Apply(0);
		m_pD3D10Device->DrawIndexed(6,0,0);
	}

	m_pSwapChain->Present(0,0);
}

void CGameApplication::update()
{
	D3DXMatrixScaling(&m_matScale,m_vecScale.x,m_vecScale.y,m_vecScale.z);
	D3DXMatrixRotationYawPitchRoll(&m_matRotation,m_vecRotation.y,m_vecRotation.x,m_vecRotation.z);
	D3DXMatrixTranslation(&m_matTranslation,m_vecPosition.x,m_vecPosition.y,m_vecPosition.z);
	D3DXMatrixMultiply(&m_matWorld,&m_matScale,&m_matRotation);
	D3DXMatrixMultiply(&m_matWorld,&m_matWorld,&m_matTranslation);
}

bool CGameApplication::initGraphics()
{
	RECT windowRect;
	GetClientRect(m_pWindow->getHandleToWindow(),&windowRect);

	UINT width=windowRect.right-windowRect.left;
	UINT height=windowRect.bottom-windowRect.top;

	UINT createDeviceFlags=0;
#ifdef _DEBUG
	createDeviceFlags|=D3D10_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof(sd));

	if (m_pWindow->isFullScreen())
		sd.BufferCount=2;
	else
		sd.BufferCount=1;

	sd.OutputWindow = m_pWindow->getHandleToWindow();
	sd.Windowed = (BOOL)(!m_pWindow->isFullScreen());
	sd.BufferUsage= DXGI_USAGE_RENDER_TARGET_OUTPUT;

	sd.SampleDesc.Count=1;
	sd.SampleDesc.Quality=0;

	sd.BufferDesc.Width=width; //width of buffer (same as window)
	sd.BufferDesc.Height=height; //height of buffer( same as window)
	sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator=60; //refresh rate (60 hz)
	sd.BufferDesc.RefreshRate.Denominator=1;

	if(FAILED(D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, D3D10_SDK_VERSION, &sd, &m_pSwapChain, &m_pD3D10Device)))
		return false;

	ID3D10Texture2D *pBackBuffer;
	if(FAILED(m_pSwapChain->GetBuffer(0,__uuidof(ID3D10Texture2D),(void**)&pBackBuffer)))
		return false;

	if(FAILED(m_pD3D10Device->CreateRenderTargetView(pBackBuffer,NULL,&m_pRenderTargetView)))
	{
		pBackBuffer->Release();
		return false;
	}
	pBackBuffer->Release();

	D3D10_TEXTURE2D_DESC descDepth;
	descDepth.Width=width;
	descDepth.Height=height;
	descDepth.MipLevels=1;
	descDepth.ArraySize=1;
	descDepth.Format=DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count=1;
	descDepth.SampleDesc.Quality=0;
	descDepth.Usage=D3D10_USAGE_DEFAULT;
	descDepth.BindFlags=D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags=0;
	descDepth.MiscFlags=0;

	if(FAILED(m_pD3D10Device->CreateTexture2D(&descDepth,NULL,&m_pDepthStencilTexture)))
		return false;

	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format=descDepth.Format;
	descDSV.ViewDimension=D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice=0;

	if(FAILED(m_pD3D10Device->CreateDepthStencilView(m_pDepthStencilTexture,&descDSV,&m_pDepthStencilView)))
		return false;

	m_pD3D10Device->OMSetRenderTargets(1,&m_pRenderTargetView,m_pDepthStencilView);

	D3D10_VIEWPORT vp;
	vp.Width=width;
	vp.Height=height;
	vp.MinDepth=0.0f;
	vp.MaxDepth=1.0f;
	vp.TopLeftX=0;
	vp.TopLeftY=0;
	m_pD3D10Device->RSSetViewports(1,&vp);

	return true;
}

bool CGameApplication::initWindow()
{
	m_pWindow=new CWin32Window();
	if (!m_pWindow->init(TEXT("Lab 1 - Create Device"),800,640,false))
		return false;

	return true;
}

