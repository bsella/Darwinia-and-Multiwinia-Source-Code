#ifndef INCLUDED_KEYBINDINGS_WINDOW_H
#define INCLUDED_KEYBINDINGS_WINDOW_H


#include "lib/input/input.h"
#include "interface/darwinia_window.h"
#include <vector>

class PrefsKeybindingsWindow : public DarwiniaWindow
{
public:
	std::vector<InputDescription> m_bindings;
	int m_numMouseButtons;
    int m_controlMethod;

public:
    PrefsKeybindingsWindow();

    void Create();
    void Remove();

    void Render( bool _hasFocus );
};


#endif
