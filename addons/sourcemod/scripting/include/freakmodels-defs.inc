
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

//using an arrayList instead?
// #define NUMALLOWEDWEARABLES 32

enum struct PlayerDataStruct
{
	int userId;

	int rSkinItem;
	char skinPath[PLATFORM_MAX_PATH];
	char animPath[PLATFORM_MAX_PATH];

	ArrayList equippedItems;
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
		if (IsValidClient(clientId)) g_playersData[clientId].userId = GetClientUserId(clientId);
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

	public void InitEquipList()
	{
		if (g_playersData[this.clientId].equippedItems) delete g_playersData[this.clientId].equippedItems;
		g_playersData[this.clientId].equippedItems = new ArrayList();
	}

	public void ClearData()
	{
		//cant use SetSkinPath/SetAnimPath bc methodmaps and their members aren't hoisted :(
		//reset skin path
		strcopy(g_playersData[this.clientId].skinPath, PLATFORM_MAX_PATH, "");
		//reset anim path
		strcopy(g_playersData[this.clientId].animPath, PLATFORM_MAX_PATH, "");

		this.rSkinItem = 0;
		
		g_playersData[this.clientId].userId = 0;

		this.InitEquipList();
	}

	public void CheckValid()
	{
		//if there's no user id, then this PlayerData wasn't valid on creation, and shouldn't be assumed to be valid now
		if (!(this.userId))
		{
			int c = this.clientId;
			this.ClearData();
			ThrowError("PlayerData of client index %i is not a real player.", c);
		}
	}

	public void GetSkinPath(char[] buffer, int bufferSize)
	{
		//we should be allowed to get the properties of a nonexistent player. they should be 0.
		strcopy(buffer, bufferSize, g_playersData[this.clientId].skinPath);
	}

	public void SetSkinPath(char[] path)
	{
		//we should not be allowed to set the props of a nonexistent player, for obvious reasons.
		this.CheckValid();
		strcopy(g_playersData[this.clientId].skinPath, PLATFORM_MAX_PATH, path);
	}


	public void GetAnimPath(char[] buffer, int bufferSize)
	{
		strcopy(buffer, bufferSize, g_playersData[this.clientId].animPath);
	}

	public void SetAnimPath(char[] path)
	{
		this.CheckValid();
		strcopy(g_playersData[this.clientId].animPath, PLATFORM_MAX_PATH, path);
	}

	public void SetEquippable(int itemReference, int slot)
	{
		this.CheckValid();

		if (slot < 1) ThrowError("The SetEquippable slot %i is invalid; slots start counting from 1.", slot);

		ArrayList list = g_playersData[this.clientId].equippedItems;

		if (list.Length < slot)
			//not long enough
			list.Resize(slot);

		list.Set(slot - 1, itemReference);

		//TODO: should I delete this? Does deleting a handle delete the object in memory or just the reference to the object?
		delete list;
	}

	public int GetEquippable(int slot)
	{

		ArrayList list = g_playersData[this.clientId].equippedItems;
		if (list.Length < slot) return 0;
		else return list.Get(slot - 1);
	}
}

//mainly used in/for cleanup module

ArrayList allWearables = null;

//mainly used in/for config module

#define FM_CONFIGFILEPATH "configs/freakmodels.models.txt"

KeyValues config = null;
