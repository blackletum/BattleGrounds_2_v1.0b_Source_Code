//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <assert.h>

#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui/IInput.h>
#include <KeyValues.h>

#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/ScrollBarSlider.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

namespace
{

enum
{						 // scroll bar will scroll a little, then continuous scroll like in windows
	SCROLL_BAR_DELAY = 400,	 // default delay for all scroll bars
	SCROLL_BAR_SPEED = 50, // this is how fast the bar scrolls when you hold down the arrow button
	SCROLLBAR_DEFAULT_WIDTH = 17,
};

//-----------------------------------------------------------------------------
// Purpose: Scroll bar button-the arrow button that moves the slider up and down.
//-----------------------------------------------------------------------------
class ScrollBarButton : public Button
{
public:
	ScrollBarButton(Panel *parent, const char *panelName, const char *text) : Button(parent, panelName, text)
	{
		SetButtonActivationType(ACTIVATE_ONPRESSED);

		SetContentAlignment(Label::a_center);

		//BG2 - Tjoppen - vgui::HTML fix from VERC. thanks to ssba
		SetEnabled(true);
		SetMouseInputEnabled(true);
		SetUseCaptureMouse(true);
	}

	void OnMouseFocusTicked()
	{
		// pass straight up to parent
		CallParentFunction(new KeyValues("MouseFocusTicked"));
	}
 
	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		Button::ApplySchemeSettings(pScheme);

		SetFont(pScheme->GetFont("Marlett", IsProportional() ));
		SetDefaultBorder(pScheme->GetBorder("ScrollBarButtonBorder"));
        SetDepressedBorder(pScheme->GetBorder("ScrollBarButtonDepressedBorder"));
		
		SetDefaultColor(GetSchemeColor("ScrollBarButton.FgColor", pScheme), GetSchemeColor("ScrollBarButton.BgColor", pScheme));
		SetArmedColor(GetSchemeColor("ScrollBarButton.ArmedFgColor", pScheme), GetSchemeColor("ScrollBarButton.ArmedBgColor", pScheme));
		SetDepressedColor(GetSchemeColor("ScrollBarButton.DepressedFgColor", pScheme), GetSchemeColor("ScrollBarButton.DepressedBgColor", pScheme));
	}

	// Don't request focus.
	// This will keep cursor focus in main window in text entry windows.
	virtual void OnMousePressed(MouseCode code)
	{
		if (!IsEnabled())
			return;
		
		if (!IsMouseClickEnabled(code))
			return;
		
		if (IsUseCaptureMouseEnabled())
		{
			{
				SetSelected(true);
				Repaint();
			}
			
			// lock mouse input to going to this button
			//BG2 - Tjoppen - vgui::HTML fix from VERC. thanks to ssba
			//input()->SetMouseCapture(GetVPanel());
		}
	}
    virtual void OnMouseReleased(MouseCode code)
    {
		if (!IsEnabled())
			return;
		
		if (!IsMouseClickEnabled(code))
			return;
		
		if (IsUseCaptureMouseEnabled())
		{
			{
				SetSelected(false);
				Repaint();
			}
			
			// lock mouse input to going to this button
			//BG2 - Tjoppen - vgui::HTML fix from VERC. thanks to ssba
			//input()->SetMouseCapture(NULL);
		}
    }

};

}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ScrollBar::ScrollBar(Panel *parent, const char *panelName, bool vertical) : Panel(parent, panelName)
{
	_slider=null;
	_button[0]=null;
	_button[1]=null;
	_scrollDelay = SCROLL_BAR_DELAY;
	_respond = true;

	if (vertical)
	{
		// FIXME: proportional changes needed???
		SetSlider(new ScrollBarSlider(NULL, NULL, true));
		SetButton(new ScrollBarButton(NULL, NULL, "t"), 0);
		SetButton(new ScrollBarButton(NULL, NULL, "u"), 1);
		_button[0]->SetTextInset(0, 1);
		_button[1]->SetTextInset(0, -1);

		SetSize(SCROLLBAR_DEFAULT_WIDTH, 64);
	}
	else
	{
		SetSlider(new ScrollBarSlider(NULL, NULL, false));
		SetButton(new ScrollBarButton(NULL, NULL, "w"), 0);
		SetButton(new ScrollBarButton(NULL, NULL, "4"), 1);
		_button[0]->SetTextInset(0, 0);
		_button[1]->SetTextInset(0, 0);

		SetSize(64, SCROLLBAR_DEFAULT_WIDTH);
	}

	Panel::SetPaintBorderEnabled(true);
	Panel::SetPaintBackgroundEnabled(false);
	Panel::SetPaintEnabled(true);
	SetButtonPressedScrollValue(20);

	//BG2 - Tjoppen - vgui::HTML fix from VERC. thanks to ssba
	SetEnabled(true);
	SetMouseInputEnabled(true);

	Validate();
}

//-----------------------------------------------------------------------------
// Purpose: sets up the width of the scrollbar according to the scheme
//-----------------------------------------------------------------------------
void ScrollBar::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	const char *resourceString = pScheme->GetResourceString("ScrollBar.Wide");

	if (resourceString)
	{
		int value = atoi(resourceString);
		if (IsProportional())
		{
			value = scheme()->GetProportionalScaledValue(value);
		}

		if (_slider && _slider->IsVertical())
		{
			// we're vertical, so reset the width
			SetSize( value, GetTall() );
		}
		else
		{
			// we're horizontal, so the width means the height
			SetSize( GetWide(), value );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the slider's Paint border enabled.
//-----------------------------------------------------------------------------
void ScrollBar::SetPaintBorderEnabled(bool state)
{
	if ( _slider )
	{
		_slider->SetPaintBorderEnabled( state );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ScrollBar::SetPaintBackgroundEnabled(bool state)
{
	if ( _slider )
	{
		_slider->SetPaintBackgroundEnabled( state );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ScrollBar::SetPaintEnabled(bool state)
{
	if ( _slider )
	{
		_slider->SetPaintEnabled( state );
	}
}

//BG2 - Tjoppen - vgui::HTML fix from VERC. thanks to ssba
// HACKHACK: This is INSANE hackery to determine if we've clicked a button or not.
// Perhaps we should get the buttons to actually accept mouse input?
void ScrollBar::OnMousePressed(MouseCode code) {
	int x,y;
	input()->GetCursorPos(x,y);

	int px,py,sx,sy;
	GetPos(py,px);
	_slider->GetPos(sx,sy);
	
	int true_x = 0;
	int true_y = 0; // This is where the buttons *really* are.. start with top button, top-left corner
	int true_slider_x = 0;
	int true_slider_y = 0;

	// Find out where we *really* are on the screen.
	Panel *parent = GetParent();
	if( parent ) {
		int pix,piy,p2x,p2y;
		parent->GetPos(pix,piy);
		// Vertical Scrollbar
		if( !Q_stricmp(GetName(),"VertScrollBar") ) {
			//DevMsg("Vertical\n");
			true_x += (pix + parent->GetWide() - GetWide());
			true_y += piy;
			true_slider_x = true_x;
			true_slider_y = true_y + _button[0]->GetTall(); // Below the button
		}
		else if( !Q_stricmp(GetName(),"HorizScrollBar") ) {
			//DevMsg("Horizontal\n");
			true_x += pix;
			true_y += (piy + parent->GetTall() - GetTall());
			true_slider_x = true_x + _button[0]->GetWide(); // Beside the button
			true_slider_y = true_y;
		}
		
		Panel *parent2 = parent->GetParent();
		if( parent2 ) {
			parent2->GetPos(p2x,p2y);
			true_x += p2x;
			true_y += p2y;
			true_slider_x += p2x;
			true_slider_y += p2y;
		}
	}

//	DevMsg("----------\nButton at: %d,%d\nSlider at: %d,%d----------\n",true_x,true_y,true_slider_x,true_slider_y);

	//if( x >= px + GetWide() - _button[0]->GetWide() && x <= px + GetWide() ) {
	if( !Q_stricmp(GetName(),"VertScrollBar") ) {
		// Check for button press
		if( x >= true_x && x <= true_x + _button[0]->GetWide() ) {
			if( y >= true_y && y <= true_y + _button[0]->GetTall() ) {
				_button[0]->OnMousePressed(code);
				SetButtonClicked(0);
				input()->SetMouseCapture(GetVPanel());
				//DevMsg("Vert top button\n");
			}

			if( y >= true_y + GetTall() - _button[1]->GetTall() && y <= true_y + GetTall() ) { // Bottom button
				_button[1]->OnMousePressed(code);
				SetButtonClicked(1);
				input()->SetMouseCapture(GetVPanel());
				//DevMsg("Vert bottom button\n");
			}
			if( x >= true_slider_x && x <= true_slider_x + _slider->GetWide() ) {
				if( y >= true_slider_y && y <= true_slider_y + _slider->GetTall() ) { // We clicked on the slider
					_slider->OnMousePressed(code);
					SetButtonClicked(2);
					input()->SetMouseCapture(GetVPanel());
					m_iScrollX = x;
					m_iScrollY = y;
					//DevMsg("Vert slider\n");
				}
			}
		}

	}
	else if( !Q_stricmp(GetName(),"HorizScrollBar") ) {
		// Check for button press
		if( y >= true_y && y <= true_y + _button[0]->GetTall() ) {
			if( x >= true_x && x <= true_x + _button[0]->GetWide() ) {
				_button[0]->OnMousePressed(code);
				SetButtonClicked(0);
				input()->SetMouseCapture(GetVPanel());
				//DevMsg("Horiz left button\n");
			}

			if( x >= true_x + GetWide() - _button[1]->GetWide() && x <= true_x + GetWide() ) { // Bottom button
				_button[1]->OnMousePressed(code);
				SetButtonClicked(1);
				input()->SetMouseCapture(GetVPanel());
				//DevMsg("Horiz right button\n");
			}

			if( y >= true_slider_y && y <= true_slider_y + _slider->GetTall() ) {
				if( x >= true_slider_x && x <= true_slider_x + _slider->GetWide() ) { // We clicked on the slider
					_slider->OnMousePressed(code);
					SetButtonClicked(2);
					input()->SetMouseCapture(GetVPanel());
					m_iScrollX = x;
					m_iScrollY = y;
					//DevMsg("Horiz slider\n");
				}
			}
		}
	}

}

void ScrollBar::OnMouseReleased(MouseCode code) {
	if( GetClickedButton() == 1 || GetClickedButton() == 0 ) {
		_button[ GetClickedButton() ]->OnMouseReleased(code);
		input()->SetMouseCapture(NULL);
	}
	else if( GetClickedButton() == 2 ) {
		//DevMsg("Releasing the slider.\n");
		_slider->OnMouseReleased(code);
		input()->SetMouseCapture(NULL);
	}

	m_iScrollX = -1;
	m_iScrollY = -1;
	SetButtonClicked(-1);
}

//-----------------------------------------------------------------------------
// Purpose: Layout the scroll bar and buttons on screen
//-----------------------------------------------------------------------------
void ScrollBar::PerformLayout()
{
	if (_slider)
	{
		int wide, tall;
		GetPaintSize(wide,tall);
		if(_slider->IsVertical())
		{
			_slider->SetBounds(0, wide, wide, tall-(wide*2)+1);
			_button[0]->SetBounds(0,0, wide-1, wide );
			_button[1]->SetBounds(0,tall-wide ,wide-1, wide );
		}
		else
		{
			_slider->SetBounds(tall, -1, wide-(tall*2)+1, tall + 1 );
			_button[0]->SetBounds(0, 0, tall, tall);
			_button[1]->SetBounds(wide-tall, 0, tall, tall);
		}
		// after resizing our child, we should remind it to perform a layout
		_slider->InvalidateLayout();
	}
	// get tooltips to draw
	Panel::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Set the value of the scroll bar slider.
//-----------------------------------------------------------------------------
void ScrollBar::SetValue(int value)
{
	_slider->SetValue(value);
}

//-----------------------------------------------------------------------------
// Purpose: Get the value of the scroll bar slider.
//-----------------------------------------------------------------------------
int ScrollBar::GetValue()
{
	return _slider->GetValue();
}

//-----------------------------------------------------------------------------
// Purpose: Set the range of the scroll bar slider.
// This the range of numbers the slider can scroll through.
//-----------------------------------------------------------------------------
void ScrollBar::SetRange(int min,int max)
{
	_slider->SetRange(min,max);
}

//-----------------------------------------------------------------------------
// Purpose: Gets the range of the scroll bar slider.
// This the range of numbers the slider can scroll through.
//-----------------------------------------------------------------------------
void ScrollBar::GetRange(int &min, int &max)
{
    _slider->GetRange(min, max);
}

//-----------------------------------------------------------------------------
// Purpose: Send a message when the slider is moved.
// Input  : value - 
//-----------------------------------------------------------------------------
void ScrollBar::SendSliderMoveMessage(int value)
{
	PostActionSignal(new KeyValues("ScrollBarSliderMoved", "position", value));
}

//-----------------------------------------------------------------------------
// Purpose: Called when the Slider is dragged by the user
// Input  : value - 
//-----------------------------------------------------------------------------
void ScrollBar::OnSliderMoved(int value)
{
	SendSliderMoveMessage(value);
}

//-----------------------------------------------------------------------------
// Purpose: Check if the scrollbar is vertical (true) or horizontal (false)
//-----------------------------------------------------------------------------
bool ScrollBar::IsVertical()
{
	return _slider->IsVertical();
}

//-----------------------------------------------------------------------------
// Purpose: Check if the the scrollbar slider has full range.
// Normally if you have a scroll bar and the range goes from a to b and
// the slider is sized to c, the range will go from a to b-c.
// This makes it so the slider goes from a to b fully.
//-----------------------------------------------------------------------------
bool ScrollBar::HasFullRange()
{
	return _slider->HasFullRange();
}

//-----------------------------------------------------------------------------
// Purpose: Setup the indexed scroll bar button with the input params.
//-----------------------------------------------------------------------------
//LEAK: new and old slider will leak
void ScrollBar::SetButton(Button *button, int index)
{
	if(_button[index]!=null)
	{
		_button[index]->SetParent((Panel *)NULL);
	}
	_button[index]=button;
	_button[index]->SetParent(this);
	_button[index]->AddActionSignalTarget(this);
	_button[index]->SetCommand(new KeyValues("ScrollButtonPressed", "index", index));

	Validate();
}


//-----------------------------------------------------------------------------
// Purpose: Return the indexed scroll bar button
//-----------------------------------------------------------------------------
Button* ScrollBar::GetButton(int index)
{
	return _button[index];
}


//-----------------------------------------------------------------------------
// Purpose: Set up the slider.
//-----------------------------------------------------------------------------
//LEAK: new and old slider will leak
void ScrollBar::SetSlider(ScrollBarSlider *slider)
{
	if(_slider!=null)
	{
		_slider->SetParent((Panel *)NULL);
	}
	_slider=slider;
	_slider->AddActionSignalTarget(this);
	_slider->SetParent(this);

	Validate();
}

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to the slider.
//-----------------------------------------------------------------------------
ScrollBarSlider *ScrollBar::GetSlider()
{
	return _slider;
}

//-----------------------------------------------------------------------------
// Purpose: Scrolls in response to clicking and holding on up or down arrow
// The idea is to have the slider move one step then delay a bit and then
// the bar starts moving at normal speed. This gives a stepping feeling
// to just clicking an arrow once.
//-----------------------------------------------------------------------------
void ScrollBar::OnMouseFocusTicked()
{
	int direction = 0;
	
	// top button is down
	//BG2 - Tjoppen - vgui::HTML fix from VERC. thanks to ssba
	if ( _button[0]->IsSelected() )
	{
		direction = -1;
	}
	// bottom top button is down
	//BG2 - Tjoppen - vgui::HTML fix from VERC. thanks to ssba
	else if (_button[1]->IsSelected())
	{
		direction = 1;
	}

	// a button is down 
	if ( direction != 0 )  
	{
		RespondToScrollArrow(direction);
		if (_scrollDelay < system()->GetTimeMillis())
		{
			_scrollDelay = system()->GetTimeMillis() + SCROLL_BAR_SPEED;
			_respond = true; 
		}
		else
		{
			_respond = false; 
		}		
	}
	// a button is not down.
	else 
	{
		// if neither button is down keep delay at max
		_scrollDelay = system()->GetTimeMillis() + SCROLL_BAR_DELAY;
		_respond = true; 
	}

	//BG2 - Tjoppen - vgui::HTML fix from VERC. thanks to ssba
	if( GetClickedButton() == 2 ) { // We're clicking on the scrollbar
		int x,y;
		input()->GetCursorPos(x,y);
		_slider->ScreenToLocal(x,y);
		_slider->OnCursorMoved(x,y);
	}
}

//-----------------------------------------------------------------------------
// Purpose: move scroll bar in response to the first button
// Input: button and direction to move scroll bar when that button is pressed
//			direction can only by +/- 1 
// Output: whether button is down or not
//-----------------------------------------------------------------------------
void ScrollBar::RespondToScrollArrow(int const direction)
{
	if (_respond)
	{
		int newValue = _slider->GetValue() + (direction * _buttonPressedScrollValue);
		_slider->SetValue(newValue);
		SendSliderMoveMessage(newValue);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Trigger layout changes when the window size is changed.
//-----------------------------------------------------------------------------
void ScrollBar::OnSizeChanged(int wide, int tall)
{
	InvalidateLayout();
	_slider->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Set how far the scroll bar slider moves when a scroll bar button is
// pressed.
//-----------------------------------------------------------------------------
void ScrollBar::SetButtonPressedScrollValue(int value)
{
	_buttonPressedScrollValue=value;
}

//-----------------------------------------------------------------------------
// Purpose: Set the range of the rangewindow. This is how many 
// lines are displayed at one time 
// in the window the scroll bar is attached to.
// This also controls the size of the slider, its size is proportional
// to the number of lines displayed / total number of lines.
//-----------------------------------------------------------------------------
void ScrollBar::SetRangeWindow(int rangeWindow)
{
	_slider->SetRangeWindow(rangeWindow);
}

//-----------------------------------------------------------------------------
// Purpose: Get the range of the rangewindow. This is how many 
// lines are displayed at one time 
// in the window the scroll bar is attached to.
// This also controls the size of the slider, its size is proportional
// to the number of lines displayed / total number of lines.
//-----------------------------------------------------------------------------
int ScrollBar::GetRangeWindow()
{
	return _slider->GetRangeWindow();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ScrollBar::Validate()
{
	if ( _slider != null )
	{
		int buttonOffset = 0;

		for( int i=0; i<2; i++ )
		{
			if( _button[i] != null )
			{
				if( _button[i]->IsVisible() )
				{
					if( _slider->IsVertical() )
					{					
						buttonOffset += _button[i]->GetTall();
					}
					else
					{
						buttonOffset += _button[i]->GetWide();
					}
				}
			}
		}

		_slider->SetButtonOffset(buttonOffset);
	}
}
