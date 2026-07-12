/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/**
 * @file ingameop.c
 * Ingame options screen.
 * Pumpkin Studios. 98
 */

#include <cstdlib>							// DedrisRemastered: std::abs
#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"		// DedrisRemastered: ssprintf
#include "lib/framework/wzapp.h"
#include "lib/gamelib/gtime.h"
#include "lib/widget/widget.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/audio.h"					// for sound.
#include "lib/sound/mixer.h"
#include "lib/ivis_opengl/piepalette.h"			// DedrisRemastered: pal_RGBA per il menu 2026
#include "lib/ivis_opengl/pieblitfunc.h"		// DedrisRemastered: iV_Line/iV_Box/box fill
#include "lib/ivis_opengl/textdraw.h"			// DedrisRemastered: WzText

#include "display3d.h"
#include "intdisplay.h"
#include "hci.h"			// for wFont def.& intmode.
#include "loop.h"
#include "lib/framework/cursors.h"
#include "frontend.h"		// for textdisplay function
#include "loadsave.h"		// for textdisplay function
#include "console.h"		// to add console message
#include "keybind.h"
#include "multiplay.h"
#include "musicmanager.h"
#include "ingameop.h"
#include "mission.h"
#include "notifications.h"
#include "wrappers.h"
#include "warzoneconfig.h"
#include "qtscript.h"		// for bInTutorial
#include "radar.h"
#include "seqdisp.h"
#include "campaigninfo.h"
#include "hci/groups.h"
#include "screens/netpregamescreen.h"
#include "screens/guidescreen.h"
#include "screens/ingameopscreen.h"
#include "screens/spectatorgameoverscreen.h"
#include "titleui/options/optionsforms.h"

bool hostQuitConfirmation = true;

bool	InGameOpUp		= false;
static bool inGameHostQuitHandled = false;
static bool		isInGameConfirmQuitUp = false;

// ////////////////////////////////////////////////////////////////////////////
// DedrisRemastered — Menu di pausa "2026" (OVERWATCH-C2).
// Ricostruzione in-engine del prototipo HTML: console near-black con angoli
// smussati, staffe a mirino ai 4 angoli, header (dot pulsante + titolo +
// sottotitolo + tempo missione), footer, scanline; voci come "celle" con
// indice, icona procedurale, etichetta, hint/chevron su hover, rail sinistro
// e variante rossa "danger" per l'uscita. Tutto in zona verde (presentazione),
// disegnato coi primitivi del motore. Non tocca gli altri pannelli.
// ////////////////////////////////////////////////////////////////////////////

// -- Geometria della console 2026 (coordinate UI virtuali 640x480) --
static const int IG2026_W       = 300;
static const int IG2026_PADX    = 12;
static const int IG2026_HEADER  = 48;
static const int IG2026_FOOTER  = 26;
static const int IG2026_ROW_H   = 27;
static const int IG2026_ROW_TOP = IG2026_HEADER + 5;
#define IG2026_H(lines) (IG2026_ROW_TOP + (lines) * IG2026_ROW_H + IG2026_FOOTER)
#define IG2026_X        ((320 - IG2026_W / 2) + D_W)
#define IG2026_Y(lines) ((240 - IG2026_H(lines) / 2) + D_H)

namespace {

// -- Palette verde "Project", coerente con l'HUD in gioco --
inline PIELIGHT igPanel()     { return pal_RGBA(8, 14, 10, 236); }
inline PIELIGHT igHead()      { return pal_RGBA(20, 38, 28, 150); }
inline PIELIGHT igGreen()     { return pal_RGBA(140, 245, 165, 255); }
inline PIELIGHT igGreenSoft() { return pal_RGBA(190, 255, 210, 255); }
inline PIELIGHT igGreenDim()  { return pal_RGBA(64, 150, 100, 255); }
inline PIELIGHT igLine()      { return pal_RGBA(140, 245, 165, 60);  }
inline PIELIGHT igLineHi()    { return pal_RGBA(150, 250, 175, 160); }
inline PIELIGHT igInk()       { return pal_RGBA(206, 236, 216, 255); }
inline PIELIGHT igInkDim()    { return pal_RGBA(120, 165, 140, 255); }
inline PIELIGHT igRed()       { return pal_RGBA(255, 110, 110, 255); }
inline PIELIGHT igRedSoft()   { return pal_RGBA(255, 172, 172, 255); }

enum IGIcon { IG_ICON_NONE = 0, IG_ICON_PLAY, IG_ICON_GUIDE, IG_ICON_OPTIONS, IG_ICON_LOAD, IG_ICON_SAVE, IG_ICON_QUIT };
enum { IGK_NORMAL = 0, IGK_PRIMARY = 1, IGK_DANGER = 2 };

// centra verticalmente il testo in una banda [bandTop, bandTop+bandH]
inline int igTextTop(WzText &t, int bandTop, int bandH)
{
	return bandTop + (bandH - t.lineSize()) / 2 - t.aboveBase();
}

// segmento spesso 2px (per staffe/rail nitidi)
inline void igBar(int x0, int y0, int x1, int y1, PIELIGHT c)
{
	pie_BoxFill(x0, y0, x1, y1, c);
}

// cornice smussata (ottagono) — l'estetica "angoli tagliati" del prototipo
static void igChamferBorder(int x0, int y0, int x1, int y1, int c, PIELIGHT col)
{
	iV_Line(x0 + c, y0,     x1 - c, y0,     col); // alto
	iV_Line(x1 - c, y0,     x1,     y0 + c, col); // smusso alto-dx
	iV_Line(x1,     y0 + c, x1,     y1 - c, col); // destra
	iV_Line(x1,     y1 - c, x1 - c, y1,     col); // smusso basso-dx
	iV_Line(x1 - c, y1,     x0 + c, y1,     col); // basso
	iV_Line(x0 + c, y1,     x0,     y1 - c, col); // smusso basso-sx
	iV_Line(x0,     y1 - c, x0,     y0 + c, col); // sinistra
	iV_Line(x0,     y0 + c, x0 + c, y0,     col); // smusso alto-sx
}

// staffe a mirino ai 4 angoli, appena fuori dal pannello (2px)
static void igBrackets(int x0, int y0, int x1, int y1, PIELIGHT col)
{
	const int o = 5;   // scostamento verso l'esterno
	const int L = 13;  // lunghezza gamba
	// alto-sx
	igBar(x0 - o,     y0 - o, x0 - o + L, y0 - o + 1, col);
	igBar(x0 - o,     y0 - o, x0 - o + 1, y0 - o + L, col);
	// alto-dx
	igBar(x1 + o - L, y0 - o, x1 + o,     y0 - o + 1, col);
	igBar(x1 + o - 1, y0 - o, x1 + o,     y0 - o + L, col);
	// basso-sx
	igBar(x0 - o,     y1 + o - 1, x0 - o + L, y1 + o, col);
	igBar(x0 - o,     y1 + o - L, x0 - o + 1, y1 + o, col);
	// basso-dx
	igBar(x1 + o - L, y1 + o - 1, x1 + o,     y1 + o, col);
	igBar(x1 + o - 1, y1 + o - L, x1 + o,     y1 + o, col);
}

// icone procedurali (centro cx,cy), ~14px
static void igDrawIcon(int cx, int cy, IGIcon icon, PIELIGHT col)
{
	switch (icon)
	{
	case IG_ICON_PLAY: // triangolo pieno verso destra
	{
		const int left = cx - 5, h = 6, w = 11;
		for (int dy = -h; dy <= h; ++dy)
		{
			int xr = left + (w * (h - std::abs(dy))) / h;
			iV_Line(left, cy + dy, xr, cy + dy, col);
		}
		break;
	}
	case IG_ICON_GUIDE: // libro aperto (due pagine)
		iV_Box(cx - 6, cy - 5, cx - 1, cy + 5, col);
		iV_Box(cx + 1, cy - 5, cx + 6, cy + 5, col);
		break;
	case IG_ICON_OPTIONS: // "sliders" (impostazioni)
		for (int i = 0; i < 3; ++i)
		{
			int y = cy - 5 + i * 5;
			iV_Line(cx - 6, y, cx + 6, y, col);
			int kx = cx - 3 + i * 3;
			igBar(kx - 1, y - 2, kx + 1, y + 2, col);
		}
		break;
	case IG_ICON_LOAD: // freccia giù in un vassoio
		iV_Line(cx, cy - 6, cx, cy + 2, col);
		iV_Line(cx, cy + 2, cx - 3, cy - 1, col);
		iV_Line(cx, cy + 2, cx + 3, cy - 1, col);
		iV_Line(cx - 6, cy + 5, cx + 6, cy + 5, col);
		iV_Line(cx - 6, cy + 3, cx - 6, cy + 5, col);
		iV_Line(cx + 6, cy + 3, cx + 6, cy + 5, col);
		break;
	case IG_ICON_SAVE: // floppy
		iV_Box(cx - 6, cy - 6, cx + 6, cy + 6, col);
		iV_Line(cx - 3, cy - 6, cx - 3, cy - 2, col);
		iV_Line(cx + 3, cy - 6, cx + 3, cy - 2, col);
		iV_Line(cx - 3, cy - 2, cx + 3, cy - 2, col);
		igBar(cx - 3, cy + 1, cx + 3, cy + 6, col);
		break;
	case IG_ICON_QUIT: // porta + freccia in uscita
		iV_Line(cx - 6, cy - 6, cx - 6, cy + 6, col);
		iV_Line(cx - 6, cy - 6, cx - 2, cy - 6, col);
		iV_Line(cx - 6, cy + 6, cx - 2, cy + 6, col);
		iV_Line(cx - 2, cy, cx + 6, cy, col);
		iV_Line(cx + 6, cy, cx + 2, cy - 4, col);
		iV_Line(cx + 6, cy, cx + 2, cy + 4, col);
		break;
	default:
		break;
	}
}

// cache per-voce del menu 2026
struct IngameOpButtonCache
{
	WzText      label;
	WzText      hint;
	WzText      index;
	std::string hintStr;
	int         rowIndex = 0;
	IGIcon      icon = IG_ICON_NONE;
	int         kind = IGK_NORMAL;
};

// widget "chrome": pannello, cornice, staffe, header, footer, scanline
class InGameOpChrome : public WIDGET
{
public:
	InGameOpChrome() : WIDGET() {}
	void display(int xOffset, int yOffset) override;
private:
	WzText m_title, m_subtitle, m_timerLbl, m_timerVal, m_footL, m_footR;
};

void InGameOpChrome::display(int xOffset, int yOffset)
{
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();
	const int w  = width();
	const int h  = height();
	const int x1 = x0 + w;
	const int y1 = y0 + h;
	const int c  = 11; // smusso angoli

	// corpo del pannello (near-black) + striscia header più chiara
	pie_UniTransBoxFill(x0, y0, x1, y1, igPanel());
	pie_UniTransBoxFill(x0, y0, x1, y0 + IG2026_HEADER, igHead());

	// scanline che scorre lentamente (come .panel::after del prototipo)
	if (h > 0)
	{
		int sweepY = y0 + (int)((realTime / 14) % (unsigned)h);
		iV_Line(x0 + 3, sweepY, x1 - 3, sweepY, pal_RGBA(140, 245, 165, 36));
	}

	// cornice smussata + staffe a mirino ai 4 angoli
	igChamferBorder(x0, y0, x1, y1, c, igLineHi());
	igBrackets(x0, y0, x1, y1, igGreen());

	// divisori header/footer
	iV_Line(x0 + c, y0 + IG2026_HEADER, x1 - c, y0 + IG2026_HEADER, igLine());
	iV_Line(x0 + c, y1 - IG2026_FOOTER, x1 - c, y1 - IG2026_FOOTER, igLine());

	// dot di stato pulsante
	const bool blinkOn = ((realTime / 650) % 2) == 0;
	const int dotx = x0 + 17, doty = y0 + 15;
	igBar(dotx - 3, doty - 3, dotx + 3, doty + 3, blinkOn ? igGreen() : igGreenDim());

	// titolo + sottotitolo (bande separate, centrate come da convenzione motore)
	m_title.setText(WzString::fromUtf8(_("PAUSE MENU")), font_regular);
	m_title.render(x0 + 30, igTextTop(m_title, y0 + 6, 17), igGreenSoft());
	m_subtitle.setText(WzString::fromUtf8(_("SIMULATION SUSPENDED")), font_small);
	m_subtitle.render(x0 + 30, igTextTop(m_subtitle, y0 + 25, 14), igInkDim());

	// tempo missione (allineato a destra nell'header)
	unsigned secs = gameTime / GAME_TICKS_PER_SEC;
	char tbuf[16];
	ssprintf(tbuf, "%02u:%02u:%02u", secs / 3600, (secs / 60) % 60, secs % 60);
	m_timerLbl.setText(WzString::fromUtf8(_("MISSION TIME")), font_small);
	m_timerLbl.render(x1 - 14 - m_timerLbl.width(), igTextTop(m_timerLbl, y0 + 6, 14), igInkDim());
	m_timerVal.setText(WzString::fromUtf8(tbuf), font_regular);
	m_timerVal.render(x1 - 14 - m_timerVal.width(), igTextTop(m_timerVal, y0 + 22, 18), igGreen());

	// footer: hint ESC a sinistra (riusa la stringa già tradotta), brand a destra
	std::string escHint = std::string("[ESC]  ") + _("Resume Game");
	m_footL.setText(WzString::fromUtf8(escHint), font_small);
	m_footL.render(x0 + 14, igTextTop(m_footL, y1 - IG2026_FOOTER, IG2026_FOOTER), igInkDim());
	m_footR.setText(WzString::fromUtf8("WZ2100 · REMASTERD"), font_small);
	m_footR.render(x1 - 14 - m_footR.width(), igTextTop(m_footR, y1 - IG2026_FOOTER, IG2026_FOOTER), igGreenDim());
}

// display-function bespoke per una voce-cella del menu 2026
void displayIngameOpButton2026(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	W_BUTTON *psBut = static_cast<W_BUTTON *>(psWidget);
	assert(psWidget->pUserData != nullptr);
	IngameOpButtonCache &cache = *static_cast<IngameOpButtonCache *>(psWidget->pUserData);

	const int x0 = xOffset + psWidget->x();
	const int y0 = yOffset + psWidget->y();
	const int hgt = psWidget->height();
	const int x1 = x0 + psWidget->width();
	const int y1 = y0 + hgt;

	const bool hover = psBut->isMouseOverWidget() || (psBut->getState() & WBUT_HIGHLIGHT) != 0;
	const bool danger = (cache.kind == IGK_DANGER);
	const bool primary = (cache.kind == IGK_PRIMARY);
	const PIELIGHT accent = danger ? igRed() : igGreen();

	// sfondo cella su hover
	if (hover)
	{
		pie_UniTransBoxFill(x0, y0, x1, y1, danger ? pal_RGBA(255, 110, 110, 34) : pal_RGBA(140, 245, 165, 30));
		iV_Box(x0, y0, x1 - 1, y1 - 1, danger ? pal_RGBA(255, 120, 120, 90) : igLine());
	}
	// rail sinistro (hover o voce primaria)
	if (hover || primary)
	{
		igBar(x0, y0 + 3, x0 + 2, y1 - 3, accent);
	}

	// indice "01".."0N"
	char idx[8];
	ssprintf(idx, "%02d", cache.rowIndex);
	cache.index.setText(WzString::fromUtf8(idx), font_small);
	cache.index.render(x0 + 9, igTextTop(cache.index, y0, hgt), hover ? igInk() : igInkDim());

	// icona
	const PIELIGHT iconCol = (hover || primary) ? (danger ? igRedSoft() : igGreen()) : igInkDim();
	igDrawIcon(x0 + 35, y0 + hgt / 2, cache.icon, iconCol);

	// etichetta
	cache.label.setText(psBut->pText, font_regular);
	PIELIGHT labCol = hover ? (danger ? igRedSoft() : igGreenSoft())
	                        : (primary ? igGreenSoft() : igInk());
	cache.label.render(x0 + 54, igTextTop(cache.label, y0, hgt), labCol);

	// lato destro: hint (se presente) o chevron, solo su hover
	if (hover)
	{
		if (!cache.hintStr.empty())
		{
			cache.hint.setText(WzString::fromUtf8(cache.hintStr), font_small);
			cache.hint.render(x1 - 12 - cache.hint.width(), igTextTop(cache.hint, y0, hgt),
			                  danger ? igRedSoft() : igInkDim());
		}
		else
		{
			int chx = x1 - 16, chy = y0 + hgt / 2;
			iV_Line(chx, chy - 4, chx + 4, chy, accent);
			iV_Line(chx, chy + 4, chx + 4, chy, accent);
		}
	}
}

} // anonymous namespace

// aggiunge una voce-cella del menu 2026
static bool addIG2026Button(UDWORD id, int row, const char *string, IGIcon icon, int kind, const char *hint)
{
	W_BUTINIT sButInit;
	sButInit.formID   = INTINGAMEOP;
	sButInit.id       = id;
	sButInit.style    = WBUT_PLAIN;
	sButInit.x        = (short)IG2026_PADX;
	sButInit.y        = (short)(IG2026_ROW_TOP + (row - 1) * IG2026_ROW_H);
	sButInit.width    = (unsigned short)(IG2026_W - 2 * IG2026_PADX);
	sButInit.height   = (unsigned short)(IG2026_ROW_H - 4);
	sButInit.pDisplay = displayIngameOpButton2026;
	sButInit.pText    = string;

	IngameOpButtonCache *cache = new IngameOpButtonCache();
	cache->rowIndex = row;
	cache->icon     = icon;
	cache->kind     = kind;
	cache->hintStr  = (hint != nullptr) ? hint : "";
	sButInit.pUserData = cache;
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<IngameOpButtonCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};
	widgAddButton(psWScreen, &sButInit);
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// functions

// ////////////////////////////////////////////////////////////////////////////

static bool addIGTextButton(UDWORD id, UWORD x, UWORD y, UWORD width, const char *string, UDWORD Style)
{
	W_BUTINIT sButInit;

	//resume
	sButInit.formID		= INTINGAMEOP;
	sButInit.id			= id;
	sButInit.style		= Style;


	sButInit.x			= x;
	sButInit.y			= y;
	sButInit.width		= width;
	sButInit.height		= INTINGAMEOP_OP_H;

	sButInit.pDisplay	= displayTextOption;
	sButInit.pText		= string;
	sButInit.pUserData = new DisplayTextOptionCache();
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};
	widgAddButton(psWScreen, &sButInit);

	return true;
}

static bool addHostQuitOptions()
{
	// get rid of the old stuff.
	widgDelete(psWScreen, INTINGAMEPOPUP);
	widgDelete(psWScreen, INTINGAMEOP);

	auto const &parent = psWScreen->psForm;

	// add form
	auto inGameOp = std::make_shared<IntFormAnimated>();
	parent->attach(inGameOp);
	inGameOp->id = INTINGAMEOP;
	inGameOp->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(INTINGAMEOP3_X, INTINGAMEOP3_Y, INTINGAMEOP3_W, INTINGAMEOP3_H);
	}));

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOP_1_Y, INTINGAMEOP_OP_W, _("Resume Game"), OPALIGN);

	auto inGamePopup = std::make_shared<IntFormAnimated>();
	parent->attach(inGamePopup);
	inGamePopup->id = INTINGAMEPOPUP;
	inGamePopup->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		assert(psWScreen != nullptr);
		WIDGET * inGameOp = widgGetFromID(psWScreen, INTINGAMEOP);
		assert(inGameOp != nullptr);
		psWidget->setGeometry((pie_GetVideoBufferWidth() - 600)/2, inGameOp->y() - 26 - 20, 600, 26);
	}));

	auto label = std::make_shared<W_LABEL>();
	inGamePopup->attach(label);
	label->setGeometry(0, 0, inGamePopup->width(), inGamePopup->height());
	label->setString(_("WARNING: You're the host. If you quit, the game ends for everyone!"));
	label->setTextAlignment(WLAB_ALIGNCENTRE);
	label->setFont(font_medium, WZCOL_YELLOW);

	addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_1_X, INTINGAMEOP_2_Y, INTINGAMEOP_OP_W, _("Host Quit"), OPALIGN);

	return true;
}

// ////////////////////////////////////////////////////////////////////////////

static bool _intAddInGameOptions()
{
	audio_StopAll();

	//clear out any mission widgets - timers etc that may be on the screen
	clearMissionWidgets();

	setWidgetsStatus(true);

	//if already open, then close!
	if (widgGetFromID(psWScreen, INTINGAMEOP))
	{
		intCloseInGameOptions(false, true);
		return true;
	}

	setReticulesEnabled(false);

	// hide the spectator game over screen (if it's up) while the menu is open, so it doesn't block the menu
	setSpectatorGameOverScreenVisible(false);

	// Pause the game.
	if (!gamePaused())
	{
		kf_TogglePauseMode();
	}

	auto const &parent = psWScreen->psForm;

	// add form
	auto ingameOp = std::make_shared<IntFormAnimated>();
	parent->attach(ingameOp);
	ingameOp->id = INTINGAMEOP;
	// DedrisRemastered: il menu di pausa 2026 disegna una propria "chrome"
	ingameOp->setSkipDefaultFrameRendering(true);

	// widget "chrome" (pannello + cornice + header/footer): attaccato per PRIMO
	// così viene disegnato sotto i bottoni, e trasparente al mouse così i click
	// arrivano alle celle sopra.
	auto chrome = std::make_shared<InGameOpChrome>();
	ingameOp->attach(chrome);
	chrome->setTransparentToMouse(true);

	int row = 1;
	// 'resume' (azione primaria; ESC riprende)
	addIG2026Button(INTINGAMEOP_RESUME, row++, _("Resume Game"), IG_ICON_PLAY, IGK_PRIMARY, "ESC");

	if (hasGameGuideTopics())
	{
		addIG2026Button(INTINGAMEOP_OPENGAMEGUIDE, row++, _("View Guide"), IG_ICON_GUIDE, IGK_NORMAL, nullptr);
	}

	addIG2026Button(INTINGAMEOP_OPTIONS, row++, _("Options"), IG_ICON_OPTIONS, IGK_NORMAL, nullptr);

	if (!((bMultiPlayer && NetPlay.bComms) || bInTutorial || NETisReplay()))
	{
		if (bMultiPlayer)
		{
			addIG2026Button(INTINGAMEOP_LOAD_SKIRMISH, row++, _("Load Game"), IG_ICON_LOAD, IGK_NORMAL, nullptr);
			addIG2026Button(INTINGAMEOP_SAVE_SKIRMISH, row++, _("Save Game"), IG_ICON_SAVE, IGK_NORMAL, nullptr);
		}
		else
		{
			addIG2026Button(INTINGAMEOP_LOAD_MISSION, row++, _("Load Game"), IG_ICON_LOAD, IGK_NORMAL, nullptr);
			if (!getCamTweakOption_AutosavesOnly())
			{
				addIG2026Button(INTINGAMEOP_SAVE_MISSION, row++, _("Save Game"), IG_ICON_SAVE, IGK_NORMAL, nullptr);
			}
		}
	}

	// 'quit' (uscita, variante "danger" rossa)
	if (NetPlay.isHost && bMultiPlayer && NetPlay.bComms)
	{
		addIG2026Button(hostQuitConfirmation ? INTINGAMEOP_HOSTQUIT : INTINGAMEOP_QUIT, row++, _("Host Quit"), IG_ICON_QUIT, IGK_DANGER, nullptr);
	}
	else
	{
		addIG2026Button(INTINGAMEOP_CONFIRM_QUIT, row++, _("Quit"), IG_ICON_QUIT, IGK_DANGER, nullptr);
	}

	// calcolo layout
	int lines = row - 1;
	ingameOp->setCalcLayout([lines](WIDGET *psWidget){
		psWidget->setGeometry(IG2026_X, IG2026_Y(lines), IG2026_W, IG2026_H(lines));
	});
	chrome->setGeometry(0, 0, IG2026_W, IG2026_H(lines));

	intMode		= INT_INGAMEOP;			// change interface mode.
	InGameOpUp	= true;					// inform interface.

	wzSetCursor(CURSOR_DEFAULT);

	return true;
}

bool intAddInGameOptions()
{
	return _intAddInGameOptions();
}

static bool startInGameConfirmQuit()
{
	widgDelete(psWScreen, INTINGAMEOP); // get rid of the old stuff.
	auto const& parent = psWScreen->psForm;

	// add form
	auto ingameOp = std::make_shared<IntFormAnimated>();
	parent->attach(ingameOp);
	ingameOp->id = INTINGAMEOP;

	int row = 1;

	auto label = std::make_shared<W_LABEL>();
	ingameOp->attach(label);
	label->setGeometry(INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP4_OP_W, INTINGAMEOP_OP_H);
	if (bMultiPlayer && (NetPlay.bComms || NETisReplay()))
	{
		label->setString(_("Warning: Are you sure?")); //Do not mention saving in real multiplayer matches
	}
	else
	{
		label->setString(_("Warning: Are you sure? Any unsaved progress will be lost."));
	}
	label->setTextAlignment(WLAB_ALIGNCENTRE);
	label->setFont(font_medium, WZCOL_YELLOW);

	row++;

	// add quit confirmation text
	addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP4_OP_W, _("Confirm"), OPALIGN);
	row++;
	addIGTextButton(INTINGAMEOP_GO_BACK, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP4_OP_W, _("Back"), OPALIGN);

	ingameOp->setCalcLayout([row](WIDGET* psWidget) {
		psWidget->setGeometry(INTINGAMEOP4_X, INTINGAMEOPAUTO_Y(row), INTINGAMEOP4_W, INTINGAMEOPAUTO_H(row));
	});

	return true;
}

static bool runInGameConfirmQuit(UDWORD id)
{
	switch (id)
	{
		case INTINGAMEOP_QUIT:
			return true;

		case INTINGAMEOP_GO_BACK:
			intReopenMenuWithoutUnPausing();
			break;

		default:
			break;
	}

	return false;
}

//
// Quick hack to throw up a ingame 'popup' for when the host drops connection.
//
void resetInGameHostQuit()
{
	inGameHostQuitHandled = false;
}

void handleInGameHostQuit()
{
	if (inGameHostQuitHandled)
	{
		return;
	}
	inGameHostQuitHandled = true;

	WZ_Notification notification;
	notification.duration = GAME_TICKS_PER_SEC * 10;
	notification.contentTitle = _("Host has quit the game!");
	notification.contentText = _("The game can't continue without the host.");
	notification.tag = "hostQuitInGame";
	addNotification(notification, WZ_Notification_Trigger::Immediate());

	addConsoleMessage(_("Host has quit the game!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);

	if (!ingame.endTime.has_value())
	{
		ingame.endTime = std::chrono::steady_clock::now();
	}

	shutdownGameStartScreen();

	if (MissionResUp || intMode == INT_MISSIONRES)
	{
		// after-game results are already being displayed
		intMissionResultsUpdateButtons();
		return;
	}

	if (NetPlay.players[selectedPlayer].isSpectator)
	{
		// Special message for spectators to inform them that the game is fully over
		addConsoleMessage(_("GAME OVER"), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);
		addConsoleMessage(_("The battle is over - you can leave the room."), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);
		return;
	}

	audio_StopAll();

	//clear out any mission widgets - timers etc that may be on the screen
	clearMissionWidgets();

	// Display the after-game results, without recording a win/loss for an aborted game
	intAddMissionResult(testPlayerHasWon(), false, false, _("Host has quit the game!"));
}

// ////////////////////////////////////////////////////////////////////////////

static void ProcessOptionFinished()
{
	intMode		= INT_NORMAL;

	if (gamePaused())
	{
		kf_TogglePauseMode();
	}

	setReticulesEnabled(true);

	// re-show the spectator game over screen (if it's up)
	setSpectatorGameOverScreenVisible(true);
}

void intCloseInGameOptionsNoAnim()
{
	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}
	widgDelete(psWScreen, INTINGAMEOP);
	InGameOpUp = false;

	ProcessOptionFinished();
	resetMissionWidgets();
}

// ////////////////////////////////////////////////////////////////////////////
bool intCloseInGameOptions(bool bPutUpLoadSave, bool bResetMissionWidgets)
{
	isInGameConfirmQuitUp = false;

	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}

	if (bPutUpLoadSave)
	{
		WIDGET *widg = widgGetFromID(psWScreen, INTINGAMEOP);
		if (widg)
		{
			widgDelete(psWScreen, INTINGAMEOP);
		}

		InGameOpUp = false;
	}
	if (!bPutUpLoadSave)
	{
		// close the form.
		// Start the window close animation.
		IntFormAnimated *form = dynamic_cast<IntFormAnimated *>(widgGetFromID(psWScreen, INTINGAMEOP));
		if (form)
		{
			form->closeAnimateDelete();
			InGameOpUp = false;
		}
		form = dynamic_cast<IntFormAnimated *>(widgGetFromID(psWScreen, INTINGAMEOP_QUIT));
		if (form)
		{
			form->closeAnimateDelete();
			InGameOpUp = false;
		}
	}

	ProcessOptionFinished();

	//don't add the widgets if the load/save screen is put up or exiting to front end
	if (bResetMissionWidgets)
	{
		//put any widgets back on for the missions
		resetMissionWidgets();
	}

	// the setting for group menu display may have been modified
	intShowGroupSelectionMenu();
	return true;
}

void intReopenMenuWithoutUnPausing()
{
	isInGameConfirmQuitUp = false;

	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}
	widgDelete(psWScreen, INTINGAMEOP);
	intAddInGameOptions();
}

static bool startIGOptionsMenu()
{
	showInGameOptionsScreen(psWScreen, createOptionsBrowser(true), []() {
		// the setting for group menu display may have been modified
		intShowGroupSelectionMenu();
	});
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// process clicks made by user.
void intProcessInGameOptions(UDWORD id)
{
	if (isInGameConfirmQuitUp)
	{
		if (runInGameConfirmQuit(id))
		{
			intCloseInGameOptions(true, true);
		}
		return;
	}

	switch (id)
	{
	// NORMAL KEYS
	case INTINGAMEOP_HOSTQUIT:				//quit was pressed
		addHostQuitOptions();
		break;

	case INTINGAMEOP_CONFIRM_QUIT:
		startInGameConfirmQuit();
		isInGameConfirmQuitUp = true;
		break;

	case INTINGAMEOP_QUIT:		//quit was confirmed.
		intCloseInGameOptions(false, false);
		break;

	case INTINGAMEOP_OPENGAMEGUIDE:
		intCloseInGameOptions(false, false);
		showGuideScreen([]() { /* no=op on close func */ }, true);
		break;

	case INTINGAMEOP_OPTIONS:			//game options  was pressed
		intCloseInGameOptions(false, false);
		startIGOptionsMenu();
		break;
	case INTINGAMEOP_RESUME:			//resume was pressed.
		intCloseInGameOptions(false, true);
		break;
	case INTINGAMEOP_GO_BACK:
		intReopenMenuWithoutUnPausing();
		break;


//	case INTINGAMEOP_REPLAY:
//		intCloseInGameOptions(true, false);
//		if(0!=strcmp(getLevelName(),"CAM_1A"))
//		{
//			loopMissionState = LMS_LOADGAME;
//			strcpy(saveGameName, "replay/replay.gam");
//			addConsoleMessage(_("GAME SAVED!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
//		}
//		break;
	case INTINGAMEOP_LOAD_MISSION:
		intCloseInGameOptions(true, false);
		addLoadSave(LOAD_INGAME_MISSION, _("Load Campaign Saved Game"));	// change mode when loadsave returns
		break;
	case INTINGAMEOP_LOAD_SKIRMISH:
		intCloseInGameOptions(true, false);
		addLoadSave(LOAD_INGAME_SKIRMISH, _("Load Skirmish Saved Game"));	// change mode when loadsave returns
		break;
	case INTINGAMEOP_SAVE_MISSION:
		intCloseInGameOptions(true, false);
		addLoadSave(SAVE_INGAME_MISSION, _("Save Campaign Game"));
		break;
	case INTINGAMEOP_SAVE_SKIRMISH:
		intCloseInGameOptions(true, false);
		addLoadSave(SAVE_INGAME_SKIRMISH, _("Save Skirmish Game"));
		break;

	default:
		break;
	}
}
