#include "KeyValues.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/CheckButton.h>

extern ConVar	cl_crosshair,

				cl_crosshair_r,
				cl_crosshair_g,
				cl_crosshair_b,
				cl_crosshair_a,

				cl_simple_smoke,

				cl_flagstatusdetail;


class CBG2OptionsPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CBG2OptionsPanel, vgui::Frame );

	/*MESSAGE_FUNC_PARAMS( RedSliderChanged, "ValueChanged", data );
#define MESSAGE_FUNC_PARAMS( name, scriptname, p1 )*/

	//BG2 - Tjoppen - don't want any virtual crap.. everything in the class. thus the reason for not using
	//				the MESSAGE_FUNC_PARAMS macro
	_MessageFuncCommon( SliderMoved, "SliderMoved", 1, vgui::DATATYPE_KEYVALUES, NULL, 0, 0 );
	_MessageFuncCommon( CheckButtonChecked, "CheckButtonChecked", 1, vgui::DATATYPE_KEYVALUES, NULL, 0, 0 );
	
	void SliderMoved( KeyValues *data )
	{
		vgui::Slider *pSender = (vgui::Slider*)data->GetPtr( "panel" );

		if( !pSender )
			return;

		//determine which
		if( pSender == m_pRedCrosshairSlider )
			cl_crosshair_r.SetValue( data->GetInt("position") );
		else if( pSender == m_pGreenCrosshairSlider )
			cl_crosshair_g.SetValue( data->GetInt("position") );
		else if( pSender == m_pBlueCrosshairSlider )
			cl_crosshair_b.SetValue( data->GetInt("position") );
		else if( pSender == m_pAlphaCrosshairSlider )
			cl_crosshair_a.SetValue( data->GetInt("position") );
	}

	void CheckButtonChecked( KeyValues *data )
	{
		vgui::CheckButton *pSender = (vgui::CheckButton*)data->GetPtr( "panel" );

		if( !pSender )
			return;

		if( pSender == m_pSimpleSmokeCheckButton )
			cl_simple_smoke.SetValue( data->GetInt("state") );
		if( pSender == m_pSimpleFlagHUDCheckButton )
			cl_flagstatusdetail.SetValue( data->GetInt("state") ? 1 : 2 );
		else if( pSender == m_pFlag0CrosshairCheckButton )
			cl_crosshair.SetValue( (cl_crosshair.GetInt() & ~1) | (data->GetInt("state") ? 1 : 0) );
		else if( pSender == m_pFlag1CrosshairCheckButton )
			cl_crosshair.SetValue( (cl_crosshair.GetInt() & ~2) | (data->GetInt("state") ? 2 : 0) );
		else if( pSender == m_pFlag2CrosshairCheckButton )
			cl_crosshair.SetValue( (cl_crosshair.GetInt() & ~4) | (data->GetInt("state") ? 4 : 0) );
		else if( pSender == m_pFlag3CrosshairCheckButton )
			cl_crosshair.SetValue( (cl_crosshair.GetInt() & ~8) | (data->GetInt("state") ? 8 : 0) );
	}

	CBG2OptionsPanel( vgui::VPANEL parent ) : BaseClass( NULL, "BG2OptionsPanel" )
	{
		SetParent( parent );

		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "SourceScheme" );
		SetScheme( scheme );
		
		SetVisible( false );

		m_pRedCrosshairSlider = new vgui::Slider( this, "RedCrosshairSlider" );
		m_pRedCrosshairSlider->SetRange( 0, 255 );
		m_pRedCrosshairSlider->SetValue( cl_crosshair_r.GetInt() );
		m_pRedCrosshairSlider->AddActionSignalTarget( this );

		m_pGreenCrosshairSlider = new vgui::Slider( this, "GreenCrosshairSlider" );
		m_pGreenCrosshairSlider->SetRange( 0, 255 );
		m_pGreenCrosshairSlider->SetValue( cl_crosshair_g.GetInt() );
		m_pGreenCrosshairSlider->AddActionSignalTarget( this );

		m_pBlueCrosshairSlider = new vgui::Slider( this, "BlueCrosshairSlider" );
		m_pBlueCrosshairSlider->SetRange( 0, 255 );
		m_pBlueCrosshairSlider->SetValue( cl_crosshair_b.GetInt() );
		m_pBlueCrosshairSlider->AddActionSignalTarget( this );

		m_pAlphaCrosshairSlider = new vgui::Slider( this, "AlphaCrosshairSlider" );
		m_pAlphaCrosshairSlider->SetRange( 0, 255 );
		m_pAlphaCrosshairSlider->SetValue( cl_crosshair_a.GetInt() );
		m_pAlphaCrosshairSlider->AddActionSignalTarget( this );

		m_pSimpleSmokeCheckButton = new vgui::CheckButton( this, "SimpleSmokeCheckButton", "" );
		m_pSimpleSmokeCheckButton->SetSelected( cl_simple_smoke.GetBool() );
		m_pSimpleSmokeCheckButton->AddActionSignalTarget( this );

		m_pSimpleFlagHUDCheckButton = new vgui::CheckButton( this, "SimpleFlagHUDCheckButton", "" );
		m_pSimpleFlagHUDCheckButton->SetSelected( cl_flagstatusdetail.GetInt() == 1 ? true : false );
		m_pSimpleFlagHUDCheckButton->AddActionSignalTarget( this );

		m_pFlag0CrosshairCheckButton = new vgui::CheckButton( this, "Flag0CrosshairCheckButton", "" );
		m_pFlag0CrosshairCheckButton->SetSelected( (cl_crosshair.GetInt() & 1) ? true : false );
		m_pFlag0CrosshairCheckButton->AddActionSignalTarget( this );

		m_pFlag1CrosshairCheckButton = new vgui::CheckButton( this, "Flag1CrosshairCheckButton", "" );
		m_pFlag1CrosshairCheckButton->SetSelected( (cl_crosshair.GetInt() & 2) ? true : false );
		m_pFlag1CrosshairCheckButton->AddActionSignalTarget( this );

		m_pFlag2CrosshairCheckButton = new vgui::CheckButton( this, "Flag2CrosshairCheckButton", "" );
		m_pFlag2CrosshairCheckButton->SetSelected( (cl_crosshair.GetInt() & 4) ? true : false );
		m_pFlag2CrosshairCheckButton->AddActionSignalTarget( this );

		m_pFlag3CrosshairCheckButton = new vgui::CheckButton( this, "Flag3CrosshairCheckButton", "" );
		m_pFlag3CrosshairCheckButton->SetSelected( (cl_crosshair.GetInt() & 8) ? true : false );
		m_pFlag3CrosshairCheckButton->AddActionSignalTarget( this );

		LoadControlSettings( "resource/ui/BG2OptionsPanel.res" );
	}

	~CBG2OptionsPanel()
	{
	}

	vgui::Slider	*m_pRedCrosshairSlider,
					*m_pGreenCrosshairSlider,
					*m_pBlueCrosshairSlider,
					*m_pAlphaCrosshairSlider;

	vgui::CheckButton	*m_pSimpleSmokeCheckButton,
						*m_pSimpleFlagHUDCheckButton,

						*m_pFlag0CrosshairCheckButton,
						*m_pFlag1CrosshairCheckButton,
						*m_pFlag2CrosshairCheckButton,
						*m_pFlag3CrosshairCheckButton;

protected:
	//virtual void OnCommand(const char *pCommand);
	/*void OnThink( void )
	{
	}*/
};

extern CBG2OptionsPanel *bg2options;