
#define EF_BONEMERGE 0x001
#define EF_BONEMERGE_FASTCULL 0x080
#define EF_PARENT_ANIMATES 0x200
#define EF_NODRAW 0x020
#define EF_NOSHADOW 0x010
#define EF_NORECEIVESHADOW 0x040

//this is just for clarity of what numbers mean what codes
enum FuncOutput
{
	GOOD,
	SETSKIN_TARGETNOTEAM,
	GETPATHARG_ALREADYFILLED,
	GETPATHARG_NOVAL
}

enum struct ModelChangeData
{
	bool isReset;
	bool useFullpaths;

	int targets[MAXPLAYERS];
	int numTargets;

	char animPath[PLATFORM_MAX_PATH];
	char animName[PLATFORM_MAX_PATH];

	char skinPath[PLATFORM_MAX_PATH];
	char skinName[PLATFORM_MAX_PATH];

}

enum struct PlayerData
{
	int rSkinItem;
	char skinPath[PLATFORM_MAX_PATH];
	char animPath[PLATFORM_MAX_PATH];
}


ModelChangeData g_clientsCommandData[MAXPLAYERS];

// Handle regularCleanupTimer = null;

PlayerData g_playersData[MAXPLAYERS];


Handle g_hDummyItemView = null;
Handle g_hEquipWearable = null;
KeyValues config = null;
ArrayList allWearables = null;