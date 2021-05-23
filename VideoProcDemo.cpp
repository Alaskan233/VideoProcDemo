// VideoProcDemo.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "VideoProcDemo.h"
#include <stdio.h>
#include <commdlg.h>
#include <shobjidl_core.h>
#include <windowsx.h>

#define MAX_LOADSTRING 100
#define ABS(a) a < 0 ? -a : a
#define EDGE(a) a < 0 ? 0 : a

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

//浏览文件后保存选择文件名字
OPENFILENAME ofn;								// 文件浏览框
TCHAR szFile[MAX_PATH];							// 保存文件的名字
TCHAR szFile2[MAX_PATH];						// 保存文件的名字

static int vedioFirstStatus = 0;				// 视频1的播放状态 1 播放 2 暂停 3 停止
static int vedioFirstStatus2 = 0;				// 视频1的播放状态 1 播放 2 暂停 3 停止

#define IMAGE_WIDTH    352
#define IMAGE_HIGHT    288
typedef struct COLOR
{
	BYTE b;
	BYTE g;
	BYTE r;
}RGBCOLOR;
//用于加载视频文件
static FILE* ifp;  //file pointer
static BYTE      mybuf[45621248]; //arry,store the video file
static BYTE* pBity, *pBitu, *pBitv;
static int       y[288][352], u[144][176], v[144][176];

//背景图像
static FILE* ifpback;
static const char* filenameback = "background.bmp";
static unsigned char mybufback[IMAGE_WIDTH * IMAGE_HIGHT * 3 + 100];
static BITMAPFILEHEADER* pbmfh;
static BITMAPINFO* pbmi;
static BYTE* pbits;
static int cxDib, cyDib;

//融合图像
static COLOR det_image[IMAGE_HIGHT][IMAGE_WIDTH];//要显示的目标图像
static int n = 0;

//用于加载视频文件2
static FILE* ifp2;  //file pointer
static BYTE      mybuf2[45621248]; //arry,store the video file
static BYTE* pBity2, *pBitu2, *pBitv2;
static int       y2[288][176], u2[144][88], v2[144][88];

//背景图像2
static FILE* ifpback2;
static const char* filenameback2 = "background.bmp";
static unsigned char mybufback2[IMAGE_WIDTH * IMAGE_HIGHT * 3 + 100];
static BITMAPFILEHEADER* pbmfh2;
static BITMAPINFO* pbmi2;
static BYTE* pbits2;
static int cxDib2, cyDib2;

//融合图像2
static COLOR det_image2[IMAGE_HIGHT][IMAGE_WIDTH/2];//要显示的目标图像
static int n2 = 0;

//划线
int draw_mode = 0;                              // 绘图模式，0代表不绘图，1代表画线
bool mouse_L_pressed = false;                   // 鼠标左键是否处于被按下的状态
static int lastxPos = NULL;						// 上一个X坐标
static int lastyPos = NULL;						// 上一个Y坐标
static int startx[IMAGE_WIDTH * IMAGE_HIGHT];	// 开始X坐标
static int starty[IMAGE_WIDTH * IMAGE_HIGHT];	// 开始Y坐标
static int endx[IMAGE_WIDTH * IMAGE_HIGHT];		// 结束X坐标
static int endy[IMAGE_WIDTH * IMAGE_HIGHT];		// 结束Y坐标
static int count = 0;

//移动
static int currentxPos = 176;
static int currentyPos = -144;

//边缘图
float smooth_kernel[3][3] = { {0.11,0.11,0.11},
							  {0.11,0.11,0.11},
							  {0.11,0.11,0.11} };//用于图像模糊的滤波核
int smooth = 0;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void FilterBmp(float kernel[3][3]);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
	// 初始化全局字符串
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_VIDEOPROCDEMO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VIDEOPROCDEMO));

	MSG msg;

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VIDEOPROCDEMO));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_VIDEOPROCDEMO);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 将实例句柄存储在全局变量中

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 分析菜单选择:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_OPEN:
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = _T("YUV视频文件(*.yuv)\0*.yuv\0\0");
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&ofn) == TRUE) {
				//打开视频文件
				if (szFile[0] != '\0') {
					n = 0;
					fopen_s(&ifp, szFile, "r");
					fread(mybuf, 45621248, 1, ifp);
					pBity = mybuf;
					pBitu = mybuf + 352 * 288;
					pBitv = mybuf + 352 * 288 + 176 * 144;

					//打开图像文件
					fopen_s(&ifpback, filenameback, "r");
					fread(mybufback, 307200, 1, ifpback);
					pbmfh = (BITMAPFILEHEADER*)mybufback;
					pbmi = (BITMAPINFO*)(pbmfh + 1);
					pbits = (BYTE*)pbmfh + pbmfh->bfOffBits;
					cxDib = pbmi->bmiHeader.biWidth;
					cyDib = pbmi->bmiHeader.biHeight;

					vedioFirstStatus = 1;
					SetTimer(hWnd, 1, 40, NULL);
				}
				break;
			}
		case ID_PLAY:
			if (szFile[0] != '\0' && vedioFirstStatus == 2) {
				vedioFirstStatus = 1;
			} else if(szFile[0] != '\0' && vedioFirstStatus == 3) {
				fopen_s(&ifp, szFile, "r");
				fread(mybuf, 45621248, 1, ifp);
				pBity = mybuf;
				pBitu = mybuf + 352 * 288;
				pBitv = mybuf + 352 * 288 + 176 * 144;

				//打开图像文件
				fopen_s(&ifpback, filenameback, "r");
				fread(mybufback, 307200, 1, ifpback);
				pbmfh = (BITMAPFILEHEADER*)mybufback;
				pbmi = (BITMAPINFO*)(pbmfh + 1);
				pbits = (BYTE*)pbmfh + pbmfh->bfOffBits;
				cxDib = pbmi->bmiHeader.biWidth;
				cyDib = pbmi->bmiHeader.biHeight;

				vedioFirstStatus = 1;
			}
			break;
		case ID_PAUSE:
			if (vedioFirstStatus == 1) {
				vedioFirstStatus = 2;
			}
			break;
		case ID_STOP:
			if (vedioFirstStatus != 3) {
				n = 0;
				vedioFirstStatus = 3;
			}
			break;

		case ID_OPEN2:
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = szFile2;
			ofn.nMaxFile = sizeof(szFile2);
			ofn.lpstrFilter = _T("YUV视频文件(*.yuv)\0*.yuv\0\0");
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&ofn) == TRUE) {
				//打开视频文件
				if (szFile2[0] != '\0') {
					n2 = 0;
					fopen_s(&ifp2, szFile2, "r");
					fread(mybuf2, 45621248, 1, ifp2);
					pBity2 = mybuf2;
					pBitu2 = mybuf2 + 352 * 288;
					pBitv2 = mybuf2 + 352 * 288 + 176 * 144;

					//打开图像文件
					fopen_s(&ifpback2, filenameback2, "r");
					fread(mybufback2, 307200, 1, ifpback2);
					pbmfh2 = (BITMAPFILEHEADER*)mybufback2;
					pbmi2 = (BITMAPINFO*)(pbmfh2 + 1);
					pbits2 = (BYTE*)pbmfh2 + pbmfh2->bfOffBits;
					cxDib2 = pbmi2->bmiHeader.biWidth;
					cyDib2 = pbmi2->bmiHeader.biHeight;

					vedioFirstStatus2 = 1;
					SetTimer(hWnd, 1, 40, NULL);
				}
				break;
			}
		case ID_PLAY2:
			if (szFile2[0] != '\0' && vedioFirstStatus2 == 2) {
				vedioFirstStatus2 = 1;
			}
			else if (szFile2[0] != '\0' && vedioFirstStatus2 == 3) {
				fopen_s(&ifp2, szFile2, "r");
				fread(mybuf2, 45621248, 1, ifp2);
				pBity2 = mybuf2;
				pBitu2 = mybuf2 + 352 * 288;
				pBitv2 = mybuf2 + 352 * 288 + 176 * 144;

				//打开图像文件
				fopen_s(&ifpback2, filenameback2, "r");
				fread(mybufback2, 307200, 1, ifpback2);
				pbmfh2 = (BITMAPFILEHEADER*)mybufback2;
				pbmi2 = (BITMAPINFO*)(pbmfh2 + 1);
				pbits2 = (BYTE*)pbmfh2 + pbmfh2->bfOffBits;
				cxDib2 = pbmi2->bmiHeader.biWidth;
				cyDib2 = pbmi2->bmiHeader.biHeight;

				vedioFirstStatus2 = 1;
			}
			break;
		case ID_PAUSE2:
			if (vedioFirstStatus2 == 1) {
				vedioFirstStatus2 = 2;
			}
			break;
		case ID_STOP2:
			if (vedioFirstStatus2 != 3) {
				n2 = 0;
				vedioFirstStatus2 = 3;
			}
			break;
		case ID_START_DRAW_LINE:
			if (szFile[0] != '\0')
				draw_mode = 1;
			break;
		case ID_STOP_DRAW_LINE:
			draw_mode = 0;
			break;
		case ID_ORIGINAL:
			smooth = 0;
			break;
		case ID_SMOOTH:
			smooth = 1;
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_LBUTTONDOWN:
		mouse_L_pressed = true;
		break;
	case WM_LBUTTONUP:
		mouse_L_pressed = false;
		lastxPos = NULL;
		lastyPos = NULL;
		break;
	case WM_MOUSEMOVE:
		// 绘图
		if (mouse_L_pressed && draw_mode == 1) {
			HDC hdc = GetDC(hWnd);
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			if (lastxPos != NULL && lastyPos != NULL) {
				MoveToEx(hdc, lastxPos, lastyPos, NULL);
				LineTo(hdc, xPos, yPos);
				startx[count] = lastxPos;
				starty[count] = lastyPos;
				endx[count] = xPos;
				endy[count] = yPos;
				count++;
			}
			lastxPos = xPos;
			lastyPos = yPos;
		// 拖拽移动
		} else if (mouse_L_pressed && draw_mode == 0) {
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			int xMove;
			int yMove;
			if (lastxPos != NULL && lastyPos != NULL) {
				if (xPos > currentxPos && xPos < currentxPos + 176 && yPos > currentyPos && currentyPos < currentyPos + 144) {
					xMove = lastxPos - xPos;
					yMove = lastyPos - yPos;
					if (currentxPos - xMove > 0 && currentxPos - xMove < 176 && currentyPos - yMove > -144 && currentyPos - yMove < 0) {
						currentxPos -= xMove;
						currentyPos -= yMove;
					}
				}
			}
			lastxPos = xPos;
			lastyPos = yPos;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TIMER:
		hdc = GetDC(hWnd);
		if (vedioFirstStatus == 1) {
			n = n + 1;
			if (n > 299) n = 0;
			pBity = pBity + (352 * 288 + 2 * (176 * 144)) * n;
			pBitu = pBity + 352 * 288;
			pBitv = pBitu + 176 * 144;

			//read a new frame of the yuv file
			for (int i = 0; i < 144; i++)
				for (int j = 0; j < 176; j++)
				{
					u[i][j] = *(pBitu + j + 176 * (i));
					v[i][j] = *(pBitv + j + 176 * (i));
				}


			//read y，and translate yuv int rgb and display the pixel
			for (int i = 0; i < 288; i++)
				for (int j = 0; j < 352; j++)
				{
					//read y
					y[i][j] = *(pBity + j + (i) * 352);
					//translate
					int r = (298 * (y[i][j] - 16) + 409 * (v[i / 2][j / 2] - 128) + 128) / 256;// >> 8;
					if (r < 0) r = 0;
					if (r > 255) r = 255;
					int g = (298 * (y[i][j] - 16) - 100 * (u[i / 2][j / 2] - 128) - 208 * (v[i / 2][j / 2] - 128) + 128) / 255;// >> 8;
					if (g < 0) g = 0;
					if (g > 255) g = 255;
					int b = (298 * (y[i][j] - 16) + 516 * (u[i / 2][j / 2] - 128) + 128) / 256;// >> 8;
					if (b < 0) b = 0;
					if (b > 255) b = 255;

					det_image[288 - i - 1][j].r = r;
					det_image[288 - i - 1][j].g = g;
					det_image[288 - i - 1][j].b = b;

				}

			SetDIBitsToDevice(hdc,
				0,
				0,
				352,
				288,
				0,
				0,
				0,
				288,
				det_image,
				pbmi,
				DIB_RGB_COLORS);

			pBity = mybuf; // let pBity to point at the first place of the file
		}

		if (vedioFirstStatus2 == 1) {
			n2 = n2 + 1;
			if (n2 > 299) n2 = 0;
			pBity2 = pBity2 + (352 * 288 + 2 * (176 * 144)) * n2;
			pBitu2 = pBity2 + 352 * 288;
			pBitv2 = pBitu2 + 176 * 144;

			//read a new frame of the yuv file
			for (int i = 0; i < 144; i++)
				for (int j = 0; j < 88; j++)
				{
					u2[i][j] = *(pBitu2 + 2 * j + 176 * (i));
					v2[i][j] = *(pBitv2 + 2 * j + 176 * (i));
				}


			//read y，and translate yuv int rgb and display the pixel
			for (int i = 0; i < 288; i++)
				for (int j = 0; j < 176; j++)
				{
					//read y
					y2[i][j] = *(pBity2 + 2 * j + (i) * 352);
					//translate
					int r = (298 * (y2[i][j] - 16) + 409 * (v2[i / 2][j / 2] - 128) + 128) / 256;// >> 8;
					if (r < 0) r = 0;
					if (r > 255) r = 255;
					int g = (298 * (y2[i][j] - 16) - 100 * (u2[i / 2][j / 2] - 128) - 208 * (v2[i / 2][j / 2] - 128) + 128) / 255;// >> 8;
					if (g < 0) g = 0;
					if (g > 255) g = 255;
					int b = (298 * (y2[i][j] - 16) + 516 * (u2[i / 2][j / 2] - 128) + 128) / 256;// >> 8;
					if (b < 0) b = 0;
					if (b > 255) b = 255;

					det_image2[288 - i - 1][j].r = r;
					det_image2[288 - i - 1][j].g = g;
					det_image2[288 - i - 1][j].b = b;

				}

			if (smooth == 1) {
				FilterBmp(smooth_kernel);
			}
			
			SetDIBitsToDevice(hdc,
				currentxPos,
				currentyPos,
				176,
				288,
				0,
				0,
				0,
				144,
				det_image2,
				pbmi2,
				DIB_RGB_COLORS);

			pBity2 = mybuf2; // let pBity to point at the first place of the file
		}


		for (int i = 0; i < count; i++) {
			MoveToEx(hdc, startx[i], starty[i], NULL);
			LineTo(hdc, endx[i], endy[i]);
		}
		
		ReleaseDC(hWnd, hdc);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void FilterBmp(float kernel[3][3])
{
	int i, j, x, y, idx, idy;
	byte r, g, b;
	float sumR, sumG, sumB;
	int finalR, finalG, finalB;
	float weight;

	for (i = 0; i < 288; i++)
	{
		for (j = 0; j < 176; j++)
		{
			//以每个像素为中心进行3x3的加权求和
			sumR = sumG = sumB = 0;
			for (y = i - 1; y <= i + 1; y++)
			{
				for (x = j - 1; x <= j + 1; x++)
				{
					if (y < 0 || y >= 288 || x < 0 || x >= 176)
					{
						//如果待加权位置越界，则其“像素”值视为0,0,0
						r = g = b = 0;
					}
					else {
						//获得待加权位置的像素值
						r = det_image2[y][x].r;
						g = det_image2[y][x].g;
						b = det_image2[y][x].b;
					}

					//与滤波核的对应位置进行加权求和
					idy = y - (i - 1);
					idx = x - (j - 1);
					weight = kernel[idy][idx];
					sumR += weight * r;
					sumG += weight * g;
					sumB += weight * b;
				}
			}

			//将结果输出
			r = ABS(sumR);
			g = ABS(sumG);
			b = ABS(sumB);
			finalR = det_image2[i][j].r - ABS(sumR);
			finalG = det_image2[i][j].g - ABS(sumG);
			finalB = det_image2[i][j].b - ABS(sumB);
			det_image2[i][j].r = EDGE(finalR);
			det_image2[i][j].g = EDGE(finalG);
			det_image2[i][j].b = EDGE(finalB);
		}
	}
}
