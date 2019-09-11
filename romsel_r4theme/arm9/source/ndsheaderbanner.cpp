#include <nds.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include "common/gl2d.h"

#include "ndsheaderbanner.h"
#include "module_params.h"

extern sNDSBannerExt ndsBanner;

// Needed to test if homebrew
char tidBuf[4];

typedef enum
{
	GL_FLIP_BOTH = (1 << 3)
} GL_FLIP_MODE_XTRA;

/**
 * Get the title ID.
 * @param ndsFile DS ROM image.
 * @param buf Output buffer for title ID. (Must be at least 4 characters.)
 * @return 0 on success; non-zero on error.
 */
int grabTID(FILE *ndsFile, char *buf)
{
	fseek(ndsFile, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
	size_t read = fread(buf, 1, 4, ndsFile);
	return !(read == 4);
}

/**
 * Get SDK version from an NDS file.
 * @param ndsFile NDS file.
 * @param filename NDS ROM filename.
 * @return 0 on success; non-zero on error.
 */
u32 getSDKVersion(FILE *ndsFile)
{
	sNDSHeaderExt NDSHeader;
	fseek(ndsFile, 0, SEEK_SET);
	fread(&NDSHeader, 1, sizeof(NDSHeader), ndsFile);
	if (NDSHeader.arm7destination >= 0x037F8000)
		return 0;
	return getModuleParams(&NDSHeader, ndsFile)->sdk_version;
}

/**
 * Check if NDS game has AP.
 * @param ndsFile NDS file.
 * @param filename NDS ROM filename.
 * @return true on success; false if no AP.
 */
bool checkRomAP(FILE *ndsFile)
{
	char game_TID[5];
	u16 headerCRC16 = 0;
	fseek(ndsFile, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
	fread(game_TID, 1, 4, ndsFile);
	fseek(ndsFile, offsetof(sNDSHeaderExt, headerCRC16), SEEK_SET);
	fread(&headerCRC16, sizeof(u16), 1, ndsFile);
	game_TID[4] = 0;

	char ipsPath[256];
	snprintf(ipsPath, sizeof(ipsPath), "sd:/_nds/TWiLightMenu/apfix/%s-%X.ips", game_TID, headerCRC16);

	if (access(ipsPath, F_OK) == 0) {
		return false;
	}

	// Check for SDK4-5 ROMs that don't have AP measures.
	if ((memcmp(game_TID, "AZLJ", 4) == 0)  // Girls Mode (JAP version of Style Savvy)
	 || (memcmp(game_TID, "YEEJ", 4) == 0)  // Inazuma Eleven (J)
	 || (memcmp(game_TID, "VSO",  3) == 0)  // Sonic Classic Collection
	 || (memcmp(game_TID, "B2D",  3) == 0)  // Doctor Who: Evacuation Earth
	 || (memcmp(game_TID, "BWB",  3) == 0)  // Plants vs Zombies
	 || (memcmp(game_TID, "VDX",  3) == 0)	  // Daniel X: The Ultimate Power
	 || (memcmp(game_TID, "BUD",  3) == 0)	  // River City Super Sports Challenge
	 || (memcmp(game_TID, "B3X",  3) == 0)	  // River City Soccer Hooligans
	 || (memcmp(game_TID, "BJM",  3) == 0)	  // Disney Stitch Jam
	 || (memcmp(game_TID, "BZX",  3) == 0)	  // Puzzle Quest 2
	 || (memcmp(game_TID, "BRFP", 4) == 0)	 // Rune Factory 3 - A Fantasy Harvest Moon
	 || (memcmp(game_TID, "BDX",  3) == 0)   // Minna de Taikan Dokusho DS: Choo Kowaai!: Gakkou no Kaidan
	 || (memcmp(game_TID, "TFB",  3) == 0)  // Frozen: Olaf's Quest
	 || (memcmp(game_TID, "TGP",  3) == 0)   // Winx Club Saving Alfea
	 || (memcmp(game_TID, "B88",  3) == 0)) // DS WiFi Settings
	{
		return false;
	}
	else
	// Check for ROMs that have AP measures.
	if ((memcmp(game_TID, "B", 1) == 0)
	 || (memcmp(game_TID, "T", 1) == 0)
	 || (memcmp(game_TID, "V", 1) == 0)) {
		return true;
	} else {
		static const char ap_list[][4] = {
			"YBN",	// 100 Classic Books
			"YBU",	// Blue Dragon: Awakened Shadow
			"ABT",	// Bust-A-Move DS
			"YV5",	// Dragon Quest V: Hand of the Heavenly Bride
			"YVI",	// Dragon Quest VI: Realms of Revelation
			"YDQ",	// Dragon Quest IX: Sentinels of the Starry Skies
			"CJR",	// Dragon Quest Monsters: Joker 2
			"AFX",	// Final Fantasy Crystal Chronicles: Ring of Fates
			"CFI",	// Final Fantasy Crystal Chronicles: Echoes of Time
			"YHG",	// Houkago Shounen
			"YEE",	// Inazuma Eleven (EUR)
			"YKG",	// Kindgom Hearts: 358/2 Days
			"YLU", 	// Last Window: The Secret of Cape West
			"UZP",	// Learn with Pokemon: Typing Adventure
			"CLJ",	// Mario & Luigi: Bowser's Inside Story
			"COL",	// Mario & Sonic at the Olympic Winter Games
			"YFQ",	// Nanashi no Geemu
			"C24",	// Phantasy Star 0
			"IPK",	// Pokemon HeartGold Version
			"IPG",	// Pokemon SoulSilver Version
			"IRA",	// Pokemon Black Version
			"IRB",	// Pokemon White Version
			"IRE",	// Pokemon Black Version 2
			"IRD",	// Pokemon White Version 2
			"C3J",	// Professor Layton and the Unwound Future
			"CQ2",	// Sengoku Spirits: Gunshi Den
			"CQ3",	// Sengoku Spirits: Moushou Den
			"YR4",	// Sengoku Spirits: Shukun Den
			"CS3",	// Sonic and Sega All Stars Racing
			"AZL",	// Style Savvy
			"AS7",	// Summon Night: Twin Age
			"YWV",	// Taiko no Tatsujin DS: Nanatsu no Shima no Daibouken!
			"CCU",	// Tomodachi Collection
		};

		// TODO: If the list gets large enough, switch to bsearch().
		for (unsigned int i = 0; i < sizeof(ap_list)/sizeof(ap_list[0]); i++) {
			if (memcmp(game_TID, ap_list[i], 3) == 0) {
				// Found a match.
				return true;
				break;
			}
		}

	}
	
	return false;
}

// bnriconframeseq[]
static u16 bnriconframeseq[64] = {0x0000};

// bnriconframenum[]
int bnriconPalLine = 0;
int bnriconframenumY = 0;
int bannerFlip = GL_FLIP_NONE;

// bnriconisDSi[]
bool isDirectory = false;
int bnrRomType = 0;
bool bnriconisDSi = false;
int bnrWirelessIcon = 0; // 0 = None, 1 = Local, 2 = WiFi
bool isDSiWare = false;
int isHomebrew = 0; // 0 = No, 1 = Yes with no DSi-Extended header, 2 = Yes with DSi-Extended header

/**
 * Get banner sequence from banner file.
 * @param binFile Banner file.
 */
void grabBannerSequence()
{
	for (int i = 0; i < 64; i++)
	{
		bnriconframeseq[i] = ndsBanner.dsi_seq[i];
	}
}

/**
 * Clear loaded banner sequence.
 */
void clearBannerSequence()
{
	for (int i = 0; i < 64; i++)
	{
		bnriconframeseq[i] = 0x0000;
	}
}

static u16 bannerDelayNum = 0x0000;
int currentbnriconframeseq = 0;

/**
 * Play banner sequence.
 * @param binFile Banner file.
 */
void playBannerSequence()
{
	if (bnriconframeseq[currentbnriconframeseq] == 0x0001 && bnriconframeseq[currentbnriconframeseq + 1] == 0x0100)
	{
		// Do nothing if icon isn't animated
		bnriconPalLine = 0;
		bnriconframenumY = 0;
		bannerFlip = GL_FLIP_NONE;
	}
	else
	{
		u16 setframeseq = bnriconframeseq[currentbnriconframeseq];
		bnriconPalLine = SEQ_PAL(setframeseq);
		bnriconframenumY =  SEQ_BMP(setframeseq);
		bool flipH = SEQ_FLIPH(setframeseq);
		bool flipV = SEQ_FLIPV(setframeseq);

		if (flipH && flipV)
		{
			bannerFlip = GL_FLIP_BOTH;
		}
		else if (!flipH && !flipV)
		{
			bannerFlip = GL_FLIP_NONE;
		}
		else if (flipH && !flipV)
		{
			bannerFlip = GL_FLIP_H;
		}
		else if (!flipH && flipV)
		{
			bannerFlip = GL_FLIP_V;
		}

		bannerDelayNum++;
		if (bannerDelayNum >= (setframeseq & 0x00FF))
		{
			bannerDelayNum = 0x0000;
			currentbnriconframeseq++;
			if (bnriconframeseq[currentbnriconframeseq] == 0x0000)
			{
				currentbnriconframeseq = 0; // Reset sequence
			}
		}
	}
}
