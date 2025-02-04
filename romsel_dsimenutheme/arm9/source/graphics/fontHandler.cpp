#include <nds/arm9/dldi.h>
#include "fontHandler.h"
#include <list>

#include "common/dsimenusettings.h"
#include "myDSiMode.h"
#include "common/flashcard.h"
#include "common/systemdetails.h"
#include "common/tonccpy.h"
#include "TextEntry.h"
#include "ThemeConfig.h"

FontGraphic smallFont;
FontGraphic largeFont;

std::list<TextEntry> topText, bottomText;

bool shouldClear[] = {false, false};


void fontInit() {
	//iprintf("fontInit()\n");

	extern u32 rotatingCubesLoaded;
	bool useExpansionPak = (sys().isRegularDS() && ((*(u16*)(0x020000C0) != 0 && *(u16*)(0x020000C0) != 0x5A45) || *(vu16*)(0x08240000) == 1) && (*(u16*)(0x020000C0) != 0 || !rotatingCubesLoaded)
							&& (io_dldi_data->ioInterface.features & FEATURE_SLOT_NDS));

	// Load font graphics
	std::string fontPath = std::string(sdFound() ? "sd:" : "fat:") + "/_nds/TWiLightMenu/extras/fonts/" + ms().font;
	smallFont = FontGraphic({fontPath + ((dsiFeatures()&&!sys().arm7SCFGLocked() || useExpansionPak) ? "/small-dsi.nftr" : "/small-ds.nftr"), fontPath + "/small.nftr", "nitro:/graphics/font/small.nftr"}, useExpansionPak);
	largeFont = FontGraphic({fontPath + ((dsiFeatures()&&!sys().arm7SCFGLocked() || useExpansionPak) ? "/large-dsi.nftr" : "/large-ds.nftr"), fontPath + "/large.nftr", "nitro:/graphics/font/large.nftr"}, useExpansionPak);

	// Load palettes
	u16 palette[] = {
		tc().fontPalette1(),
		tc().fontPalette2(),
		tc().fontPalette3(),
		tc().fontPalette4(),
	};
	tonccpy(BG_PALETTE, palette, sizeof(palette));
	tonccpy(BG_PALETTE_SUB, palette, sizeof(palette));
}

void fontReinit() {
	//iprintf("fontReinit()\n");

	// Load font graphics
	std::string fontPath = std::string("fat:") + "/_nds/TWiLightMenu/extras/fonts/" + ms().font;
	smallFont = FontGraphic({fontPath + "/small-ds.nftr", fontPath + "/small.nftr", "nitro:/graphics/font/small.nftr"}, false);
	largeFont = FontGraphic({fontPath + "/large-ds.nftr", fontPath + "/large.nftr", "nitro:/graphics/font/large.nftr"}, false);

	// Load palettes (Already loaded)
	/*u16 palette[] = {
		tc().fontPalette1(),
		tc().fontPalette2(),
		tc().fontPalette3(),
		tc().fontPalette4(),
	};
	tonccpy(BG_PALETTE, palette, sizeof(palette));
	tonccpy(BG_PALETTE_SUB, palette, sizeof(palette));*/
}

static std::list<TextEntry> &getTextQueue(bool top) {
	return top ? topText : bottomText;
}

FontGraphic &getFont(bool large) {
	return large ? largeFont : smallFont;
}

void updateText(bool top) {
	if(top)	return; // Assign some VRAM and remove this to be able to print to top

	// Clear before redrawing
	if(shouldClear[top]) {
		dmaFillWords(0, FontGraphic::textBuf[top], 256 * 192);
		shouldClear[top] = false;
	}

	// Draw text
	auto &text = getTextQueue(top);
	for(auto it = text.begin(); it != text.end(); ++it) {
		getFont(it->large).print(it->x, it->y, top, it->message, it->align);
	}
	text.clear();

	// Copy buffer to the screen
	tonccpy(bgGetGfxPtr(top ? 6 : 2), FontGraphic::textBuf[top], 256 * 192);
}

void clearText(bool top) {
	shouldClear[top] = true;
}

void clearText() {
	clearText(true);
	clearText(false);
}

void printSmall(bool top, int x, int y, std::string_view message, Alignment align) {
	getTextQueue(top).emplace_back(false, x, y, message, align);
}
void printSmall(bool top, int x, int y, std::u16string_view message, Alignment align) {
	getTextQueue(top).emplace_back(false, x, y, message, align);
}

void printLarge(bool top, int x, int y, std::string_view message, Alignment align) {
	getTextQueue(top).emplace_back(true, x, y, message, align);
}
void printLarge(bool top, int x, int y, std::u16string_view message, Alignment align) {
	getTextQueue(top).emplace_back(true, x, y, message, align);
}

int calcSmallFontWidth(std::string_view text) {
	return smallFont.calcWidth(text);
}
int calcSmallFontWidth(std::u16string_view text) {
	return smallFont.calcWidth(text);
}

int calcLargeFontWidth(std::string_view text) {
	return largeFont.calcWidth(text);
}
int calcLargeFontWidth(std::u16string_view text) {
	return largeFont.calcWidth(text);
}

int calcSmallFontHeight(std::string_view text) { return calcSmallFontHeight(FontGraphic::utf8to16(text)); }
int calcSmallFontHeight(std::u16string_view text) {
	int lines = 1;
	for(auto c : text) {
		if(c == '\n')
			lines++;
	}
	return lines * smallFont.height();
}

int calcLargeFontHeight(std::string_view text) { return calcLargeFontHeight(FontGraphic::utf8to16(text)); }
int calcLargeFontHeight(std::u16string_view text) {
	int lines = 1;
	for(auto c : text) {
		if(c == '\n')
			lines++;
	}
	return lines * largeFont.height();
}

u8 smallFontHeight(void) {
	return smallFont.height();
}

u8 largeFontHeight(void) {
	return largeFont.height();
}
