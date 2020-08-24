#pragme once

class WindowManagerSDL
{
public:
	HWND		m_hWnd;
	HDC			m_hDC;
	HGLRC		m_hRC;

	WindowManagerSDL()
	:	m_hWnd(NULL),
		m_hDC(NULL),
		m_hRC(NULL)
	{
	}
};
