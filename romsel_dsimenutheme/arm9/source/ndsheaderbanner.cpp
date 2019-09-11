#include <nds.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include "common/gl2d.h"

#include "ndsheaderbanner.h"
#include "module_params.h"

extern sNDSBannerExt ndsBanner;

char gameTid[40][5] = {0};
u16 headerCRC[40] = {0};

typedef enum
{
	GL_FLIP_BOTH = (1 << 3)
} GL_FLIP_MODE_XTRA;

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
bool checkRomAP(FILE *ndsFile, int num)
{
	char ipsPath[256];
	snprintf(ipsPath, sizeof(ipsPath), "sd:/_nds/TWiLightMenu/apfix/%s-%X.ips", gameTid[num], headerCRC[num]);

	if (access(ipsPath, F_OK) == 0) {
		return false;
	}

	// Check for SDK4-5 ROMs that don't have AP measures.
	if ((memcmp(gameTid[num], "AZLJ", 4) == 0)  // Girls Mode (JAP version of Style Savvy)
	 || (memcmp(gameTid[num], "YEEJ", 4) == 0)  // Inazuma Eleven (J)
	 || (memcmp(gameTid[num], "VSO",  3) == 0)  // Sonic Classic Collection
	 || (memcmp(gameTid[num], "B2D",  3) == 0)  // Doctor Who: Evacuation Earth
	 || (memcmp(gameTid[num], "BWB",  3) == 0)  // Plants vs Zombies
	 || (memcmp(gameTid[num], "VDX",  3) == 0)	  // Daniel X: The Ultimate Power
	 || (memcmp(gameTid[num], "BUD",  3) == 0)	  // River City Super Sports Challenge
	 || (memcmp(gameTid[num], "B3X",  3) == 0)	  // River City Soccer Hooligans
	 || (memcmp(gameTid[num], "BJM",  3) == 0)	  // Disney Stitch Jam
	 || (memcmp(gameTid[num], "BZX",  3) == 0)	  // Puzzle Quest 2
	 || (memcmp(gameTid[num], "BRFP", 4) == 0)	 // Rune Factory 3 - A Fantasy Harvest Moon
	 || (memcmp(gameTid[num], "BDX",  3) == 0)   // Minna de Taikan Dokusho DS: Choo Kowaai!: Gakkou no Kaidan
	 || (memcmp(gameTid[num], "TFB",  3) == 0)  // Frozen: Olaf's Quest
	 || (memcmp(gameTid[num], "TGP",  3) == 0)   // Winx Club Saving Alfea
	 || (memcmp(gameTid[num], "B88",  3) == 0)) // DS WiFi Settings
	{
		return false;
	}
	else
	// Check for ROMs that have AP measures.
	if ((memcmp(gameTid[num], "B", 1) == 0)
	 || (memcmp(gameTid[num], "T", 1) == 0)
	 || (memcmp(gameTid[num], "V", 1) == 0)) {
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
			if (memcmp(gameTid[num], ap_list[i], 3) == 0) {
				// Found a match.
				return true;
				break;
			}
		}

	}
	
	return false;
}

char bnriconTile[41][0x23C0];

// bnriconframeseq[]
static u16 bnriconframeseq[41][64] = {0x0000};

// bnriconframenum[]
int bnriconPalLine[41] = {0};
int bnriconframenumY[41] = {0};
int bannerFlip[41] = {GL_FLIP_NONE};

// bnriconisDSi[]
bool isDirectory[40] = {false};
bool bnrSysSettings[41] = {false};
int bnrRomType[41] = {0};
bool bnriconisDSi[41] = {false};
int bnrWirelessIcon[41] = {0}; // 0 = None, 1 = Local, 2 = WiFi
bool isDSiWare[41] = {true};
int isHomebrew[41] = {0}; // 0 = No, 1 = Yes with no DSi-Extended header, 2 = Yes with DSi-Extended header

static u16 bannerDelayNum[41] = {0x0000};
int currentbnriconframeseq[41] = {0};

/**
 * Get banner sequence from banner file.
 * @param binFile Banner file.
 */
void grabBannerSequence(int iconnum)
{
	for (int i = 0; i < 64; i++)
	{
		bnriconframeseq[iconnum][i] = ndsBanner.dsi_seq[i];
	}
	currentbnriconframeseq[iconnum] = 0;
}

/**
 * Clear loaded banner sequence.
 */
void clearBannerSequence(int iconnum)
{
	for (int i = 0; i < 64; i++)
	{
		bnriconframeseq[iconnum][i] = 0x0000;
	}
	currentbnriconframeseq[iconnum] = 0;
}

/**
 * Play banner sequence.
 * @param binFile Banner file.
 */
void playBannerSequence(int iconnum)
{
	if (bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]] == 0x0001 
	&& bnriconframeseq[iconnum][currentbnriconframeseq[iconnum] + 1] == 0x0100)
	{
		// Do nothing if icon isn't animated
		bnriconPalLine[iconnum] = 0;
		bnriconframenumY[iconnum] = 0;
		bannerFlip[iconnum] = GL_FLIP_NONE;
	}
	else
	{
		u16 setframeseq = bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]];
		bnriconPalLine[iconnum] = SEQ_PAL(setframeseq);
		bnriconframenumY[iconnum] = SEQ_BMP(setframeseq);
		bool flipH = SEQ_FLIPH(setframeseq);
		bool flipV = SEQ_FLIPV(setframeseq);

		if (flipH && flipV)
		{
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		}
		else if (!flipH && !flipV)
		{
			bannerFlip[iconnum] = GL_FLIP_NONE;
		}
		else if (flipH && !flipV)
		{
			bannerFlip[iconnum] = GL_FLIP_H;
		}
		else if (!flipH && flipV)
		{
			bannerFlip[iconnum] = GL_FLIP_V;
		}

		bannerDelayNum[iconnum]++;
		if (bannerDelayNum[iconnum] >= (setframeseq & 0x00FF))
		{
			bannerDelayNum[iconnum] = 0x0000;
			currentbnriconframeseq[iconnum]++;
			if (bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]] == 0x0000)
			{
				currentbnriconframeseq[iconnum] = 0; // Reset sequence
			}
		}
	}
}
