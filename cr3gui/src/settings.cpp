//
// C++ Implementation: settings
//
// Description:
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "settings.h"
#include <crgui.h>
#include "viewdlg.h"
#include "mainwnd.h"
//#include "fsmenu.h"

#include <cri18n.h>


class CRControlsMenu;
class CRControlsMenuItem : public CRMenuItem
{
private:
    int _key;
    int _flags;
    int _command;
    int _params;
    CRControlsMenu * _controlsMenu;
    lString16 _settingKey;
    int _defCommand;
    int _defParams;
public:
    bool getCommand( int & cmd, int & params )
    {
        lString16 v = _menu->getProps()->getStringDef(LCSTR(_settingKey), "");
        cmd = _defCommand;
        params = _defParams;
        return splitIntegerList( v, lString16(","), cmd, params );
   }
    /// submenu for options dialog support
    virtual lString16 getSubmenuValue()
    {
        int cmd;
        int params;
        bool isSet = getCommand( cmd, params );
        lString16 res = Utf8ToUnicode(lString8(getCommandName( cmd, params )));
        // TODO: use default flag
        return res;
    }
    /// called on item selection
    virtual int onSelect();
    CRControlsMenuItem( CRControlsMenu * menu, int id, int key, int flags, const CRGUIAccelerator * defAcc );
    virtual void Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, bool selected );
};

class CRControlsMenu : public CRFullScreenMenu
{
    lString16 _accelTableId;
    CRGUIAcceleratorTableRef _baseAccs;
    CRGUIAcceleratorTableRef _overrideCommands;
    lString16 _settingKey;
public:
    CRPropRef getProps() { return _props; }
    lString16 getSettingKey( int key, int flags )
    {
        lString16 res = _settingKey;
        if ( key!=0 )
            res = res + L"." + lString16::itoa(key) + L"." + lString16::itoa(flags);
        return res;
    }
    lString16 getSettingLabel( int key, int flags )
    {
        return lString16(getKeyName(key, flags));
    }
    CRMenu * createCommandsMenu(int key, int flags)
    {
        lString16 label = getSettingLabel( key, flags ) + L" - " + lString16(_("choose command"));
        lString16 keyid = getSettingKey( key, flags );
        CRMenu * menu = new CRMenu(_wm, this, _id, label, LVImageSourceRef(), LVFontRef(), LVFontRef(), _props, LCSTR(keyid), 8);
        for ( int i=0; i<_overrideCommands->length(); i++ ) {
            const CRGUIAccelerator * acc = _overrideCommands->get(i);
            lString16 cmdLabel = lString16( getCommandName(acc->commandId, acc->commandParam) );
            lString16 cmdValue = lString16::itoa(acc->commandId) + L"," + lString16::itoa(acc->commandParam);
            CRMenuItem * item = new CRMenuItem( menu, i, cmdLabel, LVImageSourceRef(), LVFontRef(), cmdValue.c_str());
            menu->addItem(item);
        }
        menu->setAccelerators( getAccelerators() );
        menu->setSkinName(lString16(L"#settings"));
        menu->setValueFont(_valueFont);
        menu->setFullscreen(true);
        menu->reconfigure( 0 );
        return menu;
    }
    CRControlsMenu(CRMenu * baseMenu, int id, CRPropRef props, lString16 accelTableId, int numItems, lvRect & rc)
    : CRFullScreenMenu( baseMenu->getWindowManager(), id, lString16(_("Controls")), numItems, rc )
    {
        _menu = baseMenu;
        _props = props;
        _baseAccs = _wm->getAccTables().get( accelTableId );
        if (_baseAccs.isNull()) {
            CRLog::error("CRControlsMenu: No accelerators %s", LCSTR(_accelTableId) );
        }
        _accelTableId = accelTableId;
        CRGUIAcceleratorTableRef _overrideKeys = _wm->getAccTables().get( accelTableId + L"-override-keys" );
        if ( _overrideKeys.isNull() ) {
            CRLog::error("CRControlsMenu: No table of allowed keys for override accelerators %s", LCSTR(_accelTableId) );
            return;
        }
        _overrideCommands = _wm->getAccTables().get( accelTableId + L"-override-commands" );
        if ( _overrideCommands.isNull() ) {
            CRLog::error("CRControlsMenu: No table of allowed commands to override accelerators %s", LCSTR(_accelTableId) );
            return;
        }
        _settingKey = lString16("keymap.") + _accelTableId;
        for ( int i=0; i<_overrideKeys->length(); i++ ) {
            const CRGUIAccelerator * acc = _overrideKeys->get(i);
            CRControlsMenuItem * item = new CRControlsMenuItem(this, i, acc->keyCode, acc->keyFlags,
                         _baseAccs->findKeyAccelerator( acc->keyCode, acc->keyFlags ) );
            addItem(item);
        }
    }

    virtual bool onCommand( int command, int params )
    {
        switch ( command ) {
        case mm_Controls:
            return true;
        default:
            return CRMenu::onCommand( command, params );
        }
    }
};

/// called on item selection
int CRControlsMenuItem::onSelect()
{
    CRMenu * menu = _controlsMenu->createCommandsMenu(_key, _flags);
    _menu->getWindowManager()->activateWindow(menu);
    return 1;
}

CRControlsMenuItem::CRControlsMenuItem( CRControlsMenu * menu, int id, int key, int flags, const CRGUIAccelerator * defAcc )
: CRMenuItem(menu, id, getKeyName(key, flags), LVImageSourceRef(), LVFontRef() ), _key( key ), _flags(flags)
{
    _defCommand = _defParams = 0;
    if ( defAcc ) {
        _defCommand = defAcc->commandId;
        _defParams = defAcc->commandParam;
    }
    _controlsMenu = menu;
    _settingKey = menu->getSettingKey( key, flags );
}

void CRControlsMenuItem::Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, bool selected )
{
    lvRect itemBorders = skin->getBorderWidths();
    skin->draw( buf, rc );
    buf.SetTextColor( skin->getTextColor() );
    buf.SetBackgroundColor( skin->getBackgroundColor() );
    lvRect textRect = rc;
    lvRect borders = skin->getBorderWidths();
    textRect.shrinkBy(borders);
    skin->drawText(buf, textRect, _label, getFont(), skin->getTextColor(), skin->getBackgroundColor(),
                   SKIN_VALIGN_TOP|SKIN_HALIGN_LEFT);
    //skin->drawText( buf, textRect, _label, getFont() );
    lString16 s = getSubmenuValue();
    if ( s.empty() )
        return;
    //_menu->getValueFont()->DrawTextString( &buf, rc.right - w - 8, rc.top + hh/2 - _menu->getValueFont()->getHeight()/2, s.c_str(), s.length(), L'?', NULL, false, 0 );
    skin->drawText(buf, textRect, s, _menu->getValueFont(), skin->getTextColor(), skin->getBackgroundColor(),
                   SKIN_VALIGN_BOTTOM|SKIN_HALIGN_RIGHT);
}

bool CRSettingsMenu::onCommand( int command, int params )
{
    switch ( command ) {
    case mm_Controls:
        {
        }
        return true;
    default:
        return CRMenu::onCommand( command, params );
    }
}

#if CR_INTERNAL_PAGE_ORIENTATION==1
CRMenu * CRSettingsMenu::createOrientationMenu( CRMenu * mainMenu, CRPropRef props )
{
	item_def_t page_orientations[] = {
		{_("0` (Portrait)"), "0"},
		{_("90 `"), "1"},
		{_("180 `"), "2"},
		{_("270 `"), "3"},
		{NULL, NULL},
	};

    LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Arial")) );
    CRMenu * orientationMenu = new CRMenu(_wm, mainMenu, mm_Orientation,
            lString16(_("Page orientation")),
                            LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_ROTATE_ANGLE );
    addMenuItems( orientationMenu, page_orientations );
    orientationMenu->reconfigure( 0 );

    return orientationMenu;
}
#endif

DECL_DEF_CR_FONT_SIZES;

class FontSizeMenu : public CRMenu
{
public:
    FontSizeMenu(  CRGUIWindowManager * wm, CRMenu * parentMenu, LVFontRef valueFont, CRPropRef props  )
    : CRMenu( wm, parentMenu, mm_FontSize,
                                _("Default font size"),
                                        LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_SIZE, 10 )
    {
        _fullscreen = true;
    }

    /// submenu for options dialog support
    virtual lString16 getSubmenuValue()
    { 
        return getProps()->getStringDef(
            UnicodeToUtf8(getPropName()).c_str(), "24");
    }
};

CRMenu * CRSettingsMenu::createFontSizeMenu( CRMenu * mainMenu, CRPropRef props )
{
    lString16Collection list;
    fontMan->getFaceList( list );
    lString8 fontFace = UnicodeToUtf8(props->getStringDef( PROP_FONT_FACE, UnicodeToUtf8(list[0]).c_str() ));
    //LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, false, css_ff_sans_serif, lString8("Arial")) );
    LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Arial")) );
    CRMenu * fontSizeMenu;
    fontSizeMenu = new FontSizeMenu(_wm, mainMenu, valueFont, props );
    for ( unsigned i=0; i<sizeof(cr_font_sizes)/sizeof(int); i++ ) {
        //char name[32];
        char defvalue[200];
        //sprintf( name, "VIEWER_DLG_FONT_SIZE_%d", cr_font_sizes[i] );
        sprintf( defvalue, "%d %s", cr_font_sizes[i], _("The quick brown fox jumps over lazy dog") );
        fontSizeMenu->addItem( new CRMenuItem( fontSizeMenu, 0,
                        lString16(defvalue),
                        LVImageSourceRef(), fontMan->GetFont( cr_font_sizes[i], 400, false, css_ff_sans_serif, fontFace), lString16::itoa(cr_font_sizes[i]).c_str()  ) );
    }
    fontSizeMenu->setAccelerators( _wm->getAccTables().get("menu10") );
    //fontSizeMenu->setAccelerators( _menuAccelerators );
    //fontSizeMenu->setSkinName(lString16(L"#settings"));
    fontSizeMenu->setSkinName(lString16(L"#main"));
    fontSizeMenu->reconfigure( 0 );
    return fontSizeMenu;
}

void CRSettingsMenu::addMenuItems( CRMenu * menu, item_def_t values[] )
{
    for ( int i=0; values[i].translate_default; i++)
        menu->addItem( new CRMenuItem( menu, i,
            lString16(values[i].translate_default),
            LVImageSourceRef(),
            LVFontRef(), Utf8ToUnicode(lString8(values[i].value)).c_str() ) );
    menu->setAccelerators( _menuAccelerators );
    menu->setSkinName(lString16(L"#settings"));
    menu->reconfigure( 0 );
}


CRSettingsMenu::CRSettingsMenu( CRGUIWindowManager * wm, CRPropRef newProps, int id, LVFontRef font, CRGUIAcceleratorTableRef menuAccelerators, lvRect &rc )
: CRFullScreenMenu( wm, id, lString16(_("Settings")), 8, rc ),
  props( newProps ),
  _menuAccelerators( menuAccelerators )
{
    setSkinName(lString16(L"#settings"));

    _fullscreen = true;

	item_def_t antialiasing_modes[] = {
		{_("On for all fonts"), "2"},
		{_("On for big fonts only"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t embedded_styles[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t bookmark_icons[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t kerning_options[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t footnotes[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t showtime[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t preformatted_text[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t inverse_mode[] = {
		{_("Normal"), "0"},
		{_("Inverse"), "1"},
		{NULL, NULL},
	};

    item_def_t embolden_mode[] = {
        {_("Normal"), "0"},
        {_("Bold"), "1"},
        {NULL, NULL},
    };

    item_def_t landscape_pages[] = {
		{_("One"), "1"},
		{_("Two"), "2"},
		{NULL, NULL},
	};

	item_def_t status_line[] = {
		{_("Top"), "0"},
	//    {"VIEWER_DLG_STATUS_LINE_BOTTOM", "Bottom", "1"},
		{_("Off"), "2"},
		{NULL, NULL},
	};

	item_def_t interline_spaces[] = {
        {_("80%"), "80"},
        {_("90%"), "90"},
		{_("100%"), "100"},
		{_("110%"), "110"},
		{_("120%"), "120"},
        {_("130%"), "130"},
        {_("140%"), "140"},
        {_("150%"), "150"},
        {NULL, NULL},
	};

	item_def_t page_margins[] = {
		{"0", "0"},
		{"5", "5"},
		{"8", "8"},
		{"10", "10"},
		{"15", "15"},
		{"20", "20"},
		{"25", "25"},
		{"30", "30"},
		{NULL, NULL},
	};

    item_def_t screen_update_options[] = {
        {_("Always use fast updates"), "0"},
        {_("Don't use fast updates"), "1"},
        {_("Full updates every 2 pages"), "2"},
        {_("Full updates every 3 pages"), "3"},
        {_("Full updates every 4 pages"), "4"},
        {_("Full updates every 5 pages"), "5"},
        {_("Full updates every 6 pages"), "6"},
        {NULL, NULL},
    };


	CRLog::trace("showSettingsMenu() - %d property values found", props->getCount() );

        //setSkinName(lString16(L"#settings"));
        setSkinName(lString16(L"#main"));

        LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Arial")) );
        CRMenu * mainMenu = this;
        mainMenu->setAccelerators( _menuAccelerators );

        CRMenu * fontFaceMenu = new CRMenu(_wm, mainMenu, mm_FontFace,
                                            _("Default font face"),
                                                    LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_FACE );
        fontFaceMenu->setSkinName(lString16(L"#settings"));
        CRLog::trace("getting font face list");
        lString16Collection list;
        fontMan->getFaceList( list );
        CRLog::trace("faces found: %d", list.length());
        int i;
        for ( i=0; i<(int)list.length(); i++ ) {
            fontFaceMenu->addItem( new CRMenuItem( fontFaceMenu, i,
                                    list[i], LVImageSourceRef(), fontMan->GetFont( MENU_FONT_FACE_SIZE, 400,
									false, css_ff_sans_serif, UnicodeToUtf8(list[i])), list[i].c_str() ) );
            fontFaceMenu->setFullscreen( true );
        }
        fontFaceMenu->setAccelerators( _menuAccelerators );
        fontFaceMenu->reconfigure( 0 );

        //lString8 fontFace = UnicodeToUtf8(props->getStringDef( PROP_FONT_FACE, UnicodeToUtf8(list[0]).c_str() ));
        mainMenu->addItem( fontFaceMenu );

        CRMenu * fontSizeMenu = createFontSizeMenu( mainMenu, props );
        mainMenu->addItem( fontSizeMenu );

        CRMenu * emboldenModeMenu = new CRMenu(_wm, mainMenu, mm_Embolden,
                _("Font weight"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_WEIGHT_EMBOLDEN );
        addMenuItems( emboldenModeMenu, embolden_mode );
        mainMenu->addItem( emboldenModeMenu );

        CRMenu * fontAntialiasingMenu = new CRMenu(_wm, mainMenu, mm_FontAntiAliasing,
                _("Font antialiasing"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_ANTIALIASING );
        addMenuItems( fontAntialiasingMenu, antialiasing_modes );
        mainMenu->addItem( fontAntialiasingMenu );

        CRMenu * interlineSpaceMenu = new CRMenu(_wm, mainMenu, mm_InterlineSpace,
                _("Interline space"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_INTERLINE_SPACE );
        addMenuItems( interlineSpaceMenu, interline_spaces );
        mainMenu->addItem( interlineSpaceMenu );

#if CR_INTERNAL_PAGE_ORIENTATION==1
        CRMenu * orientationMenu = createOrientationMenu(mainMenu, props);
        mainMenu->addItem( orientationMenu );
#endif

        CRMenu * footnotesMenu = new CRMenu(_wm, mainMenu, mm_Footnotes,
                _("Footnotes at page bottom"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FOOTNOTES );
        addMenuItems( footnotesMenu, footnotes );
        mainMenu->addItem( footnotesMenu );

/*
        SetTimeMenuItem * setTime = new SetTimeMenuItem( mainMenu, mm_SetTime, _wm->translateString("VIEWER_MENU_SET_TIME", "Set time"),
                LVImageSourceRef(), menuFont, L"bla" );
        mainMenu->addItem( setTime );
*/
        CRMenu * showTimeMenu = new CRMenu(_wm, mainMenu, mm_ShowTime,
                _("Show time"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_SHOW_TIME );
        addMenuItems( showTimeMenu, showtime );
        mainMenu->addItem( showTimeMenu );

        CRMenu * landscapePagesMenu = new CRMenu(_wm, mainMenu, mm_LandscapePages,
                _("Landscape pages"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_LANDSCAPE_PAGES );
        addMenuItems( landscapePagesMenu, landscape_pages );
        mainMenu->addItem( landscapePagesMenu );

        CRMenu * preformattedTextMenu = new CRMenu(_wm, mainMenu, mm_PreformattedText,
                _("Preformatted text"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_TXT_OPTION_PREFORMATTED );
        addMenuItems( preformattedTextMenu, preformatted_text );
        mainMenu->addItem( preformattedTextMenu );

        //

        CRMenu * embeddedStylesMenu = new CRMenu(_wm, mainMenu, mm_EmbeddedStyles,
                _("Document embedded styles"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_EMBEDDED_STYLES );
        addMenuItems( embeddedStylesMenu, embedded_styles );
        mainMenu->addItem( embeddedStylesMenu );

        CRMenu * inverseModeMenu = new CRMenu(_wm, mainMenu, mm_Inverse,
                _("Inverse display"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_DISPLAY_INVERSE );
        addMenuItems( inverseModeMenu, inverse_mode );
        mainMenu->addItem( inverseModeMenu );

        CRMenu * fastUpdatesMenu = new CRMenu(_wm, mainMenu, mm_FastUpdates,
                _("Display update mode"),
                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_DISPLAY_FULL_UPDATE_INTERVAL );
        addMenuItems( fastUpdatesMenu, screen_update_options );
        mainMenu->addItem( fastUpdatesMenu );

#if 0
        CRMenu * bookmarkIconsMenu = new CRMenu(_wm, mainMenu, mm_BookmarkIcons,
                _("Show bookmark icons"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_BOOKMARK_ICONS );
        addMenuItems( bookmarkIconsMenu, bookmark_icons );
        mainMenu->addItem( bookmarkIconsMenu );
#endif

        CRMenu * statusLineMenu = new CRMenu(_wm, mainMenu, mm_StatusLine,
                _("Status line"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_STATUS_LINE );
        addMenuItems( statusLineMenu, status_line );
        mainMenu->addItem( statusLineMenu );

        CRMenu * kerningMenu = new CRMenu(_wm, mainMenu, mm_Kerning,
                _("Font kerning"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_KERNING_ENABLED );
        addMenuItems( kerningMenu, kerning_options );
        mainMenu->addItem( kerningMenu );

		//====== Hyphenation ==========
		if ( HyphMan::getDictList() ) {
            // strings from CREngine - just to catch by gettext tools
            _("[No Hyphenation]");
            _("[Algorythmic Hyphenation]");
			CRMenu * hyphMenu = new CRMenu(_wm, mainMenu, mm_Hyphenation,
					_("Hyphenation"),
					LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_HYPHENATION_DICT );
			for ( i=0; i<HyphMan::getDictList()->length(); i++ ) {
				HyphDictionary * item = HyphMan::getDictList()->get( i );
				hyphMenu->addItem( new CRMenuItem( hyphMenu, i,
					item->getTitle(),
					LVImageSourceRef(),
					LVFontRef(), item->getId().c_str() ) );
			}
			hyphMenu->setAccelerators( _menuAccelerators );
			hyphMenu->setSkinName(lString16(L"#settings"));
            hyphMenu->reconfigure( 0 );
            mainMenu->addItem( hyphMenu );
		}
        //====== Margins ==============
        CRMenu * marginsMenu = new CRMenu(_wm, mainMenu, mm_PageMargins,
                _("Page margins"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props );
        CRMenu * marginsMenuTop = new CRMenu(_wm, marginsMenu, mm_PageMarginTop,
                _("Top margin"),
                 LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_PAGE_MARGIN_TOP );
        addMenuItems( marginsMenuTop, page_margins );
        marginsMenu->addItem( marginsMenuTop );
        CRMenu * marginsMenuBottom = new CRMenu(_wm, marginsMenu, mm_PageMarginBottom,
                _("Bottom margin"),
                 LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_PAGE_MARGIN_BOTTOM );
        addMenuItems( marginsMenuBottom, page_margins );
        marginsMenu->addItem( marginsMenuBottom );
        CRMenu * marginsMenuLeft = new CRMenu(_wm, marginsMenu, mm_PageMarginLeft,
                _("Left margin"),
                 LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_PAGE_MARGIN_LEFT );
        addMenuItems( marginsMenuLeft, page_margins );
        marginsMenu->addItem( marginsMenuLeft );
        CRMenu * marginsMenuRight = new CRMenu(_wm, marginsMenu, mm_PageMarginRight,
                _("Right margin"),
                 LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_PAGE_MARGIN_RIGHT );
        addMenuItems( marginsMenuRight, page_margins );


        marginsMenu->addItem( marginsMenuRight );
		marginsMenu->setAccelerators( _menuAccelerators );
		marginsMenu->setSkinName(lString16(L"#settings"));
        marginsMenu->reconfigure( 0 );
        mainMenu->addItem( marginsMenu );

        CRControlsMenu * controlsMenu =
                new CRControlsMenu(this, mm_Controls, props, lString16("main"), 8, _rect);
        controlsMenu->setAccelerators( _menuAccelerators );
        controlsMenu->setSkinName(lString16(L"#settings"));
        controlsMenu->setValueFont(valueFont);
        controlsMenu->reconfigure( 0 );
        mainMenu->addItem( controlsMenu );
}
