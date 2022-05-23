
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
	TARGETNOTEAM,
	TARGETINVALID,
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

enum struct PlayerDataStruct
{
	int userId;

	int rSkinItem;
	char skinPath[PLATFORM_MAX_PATH];
	char animPath[PLATFORM_MAX_PATH];
}

//used globally - in many modules

ModelChangeData g_clientsCommandData[MAXPLAYERS + 1];

// Handle regularCleanupTimer = null;

#define PLAYERSDATASIZE (MAXPLAYERS + 1)

//hidden - only accessible with the PlayerData methodmap
static PlayerDataStruct g_playersData[PLAYERSDATASIZE];

Handle g_hDummyItemView = null;
Handle g_hEquipWearable = null;

methodmap PlayerData
{
	public PlayerData(int clientId)
	{
		g_playersData[clientId].userId = GetClientUserId(clientId);
		return view_as<PlayerData>(clientId);
	}
	
	property int clientId
	{
		public get() { return view_as<int>(this); }
	}
	
	property int userId
	{
		public get() { return g_playersData[this.clientId].userId; }
	}

	property int rSkinItem
	{
		public get() { return g_playersData[this.clientId].rSkinItem; }
		public set(int value) { g_playersData[this.clientId].rSkinItem = value; }
	}

	public void GetSkinPath(char[] buffer, int bufferSize)
	{
		strcopy(buffer, bufferSize, g_playersData[this.clientId].skinPath);
	}

	public void SetSkinPath(char[] path)
	{
		strcopy(g_playersData[this.clientId].skinPath, PLATFORM_MAX_PATH, path);
	}


	public void GetAnimPath(char[] buffer, int bufferSize)
	{
		strcopy(buffer, bufferSize, g_playersData[this.clientId].animPath);
	}

	public void SetAnimPath(char[] path)
	{
		strcopy(g_playersData[this.clientId].animPath, PLATFORM_MAX_PATH, path);
	}

	public void ClearData()
	{
		this.SetSkinPath("");
		this.SetAnimPath("");
		this.rSkinItem = 0;
		
		g_playersData[this.clientId].userId = 0;
	}

}

//mainly used in/for cleanup module

ArrayList allWearables = null;

//mainly used in/for config module

#define FM_CONFIGFILEPATH "configs/freakmodels.models.txt"

KeyValues config = null;
