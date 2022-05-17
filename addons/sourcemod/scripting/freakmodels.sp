#include <sourcemod>
#include <tf2_stocks>
// ^ tf2_stocks.inc itself includes sdktools.inc and tf2.inc
#include <tf2items>
#include <sdkhooks>

#pragma semicolon 1
#pragma newdecls required

#define PLUGIN_VERSION "1.2b"

#define EF_BONEMERGE 0x001
#define EF_BONEMERGE_FASTCULL 0x080
#define EF_PARENT_ANIMATES 0x200
#define EF_NODRAW 0x020
#define EF_NOSHADOW 0x010
#define EF_NORECEIVESHADOW 0x040

#define FM_CONFIGFILEPATH "configs/freakmodels.models.txt"

//this is just for clarity of what numbers mean what codes
enum FuncOutput
{
	GOOD,
	SETSKIN_TARGETNOTEAM,
	GETPATHARG_ALREADYFILLED,
	GETPATHARG_NOVAL
}

Handle hDummyItemView = null;
Handle hEquipWearable = null;
KeyValues modelConfig = null;
int playerSkinItems[MAXPLAYERS + 2];

public Plugin myinfo = 
{
	name = "FreakModels",
	author = "Stinky Lizard",
	description = "Allows you to change your model and animations independently of your class. Made for FREAKSERVER 'The Most Fun You Can Have Online' at freak.tf2.host.",
	version = PLUGIN_VERSION,
	url = "freak.tf2.host"
};


/*
 FORWARDS
 =======================================================
 */

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	// No need for the old GetGameFolderName setup.
	EngineVersion g_engineversion = GetEngineVersion();
	if (g_engineversion != Engine_TF2)
	{
		SetFailState("FreakModels was made for use with Team Fortress 2 only.");
	}
} 

public void OnPluginStart()
{
	/**
	 * @note For the love of god, please stop using FCVAR_PLUGIN.
	 * Console.inc even explains this above the entry for the FCVAR_PLUGIN define.
	 * "No logic using this flag ever existed in a released game. It only ever appeared in the first hl2sdk."
	 */
	CreateConVar("sm_freakmodels_version", PLUGIN_VERSION, "Standard plugin version ConVar. Please don't change me!", FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_DONTRECORD);
	
	LoadTranslations("common.phrases");

	hDummyItemView = TF2Items_CreateItem(OVERRIDE_ALL|FORCE_GENERATION);
	TF2Items_SetClassname(hDummyItemView, "tf_wearable");
	TF2Items_SetItemIndex(hDummyItemView, -1); //Q: playermodel2 uses 65535. Is there a reason why?
	TF2Items_SetQuality(hDummyItemView, 0);
	TF2Items_SetLevel(hDummyItemView, 0);
	TF2Items_SetNumAttributes(hDummyItemView, 0);

	
	GameData hGameConf = new GameData("freakmodels.data");
	if(hGameConf == null) {
		SetFailState("FreakModels: Gamedata (addons/sourcemod/gamedata/freakmodels.data.txt) not found.");
		delete hGameConf;
		return;
	}
	
	StartPrepSDKCall(SDKCall_Entity);
	PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CBasePlayer::EquipWearable");
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer);
	hEquipWearable = EndPrepSDKCall();
	if(hEquipWearable == null) {
		SetFailState("FreakModels: Failed to create SDKCall for CBasePlayer::EquipWearable.");
		delete hGameConf;
		return;
	}
	delete hGameConf;
	
	//model configs
	RefreshConfigFromFile();

	RegConsoleCmd("freakmodel", MainCommand);
	RegAdminCmd("freakmodel_manage", ManageCommand, ADMFLAG_GENERIC);
}

//precache models found in the config file
public void OnMapStart()
{
	//first go to the first model name
	if (!modelConfig.GotoFirstSubKey(false)) return; //there are no defined models
	
	char curModelName[PLATFORM_MAX_PATH];
	char curModelPath[PLATFORM_MAX_PATH];

	do
	{
		modelConfig.GetSectionName(curModelName, sizeof(curModelName));
		
		//get the model path
		//it's great that you can traverse to a "value key" - its what allows you to iterate over this...
		//but I wish you could get an array of all the subkeys in the current section.
		//i think that would be much cleaner, since you could just iterate over all the keys & use the Get functions
		modelConfig.GoBack();
		modelConfig.GetString(curModelName, curModelPath, sizeof(curModelPath));
		modelConfig.JumpToKey(curModelName);

		
		//does this model exist?
		if (!FileExists(curModelPath, true))
		{
			LogMessage("Warning: FreakModels: The model named %s in the config file does not correspond to a real file.", curModelName);
			continue;
		}

		//is this model already precached?
		if (IsModelPrecached(curModelPath))
			continue;

		PrecacheModel(curModelPath);

	}
	while(modelConfig.GotoNextKey(false));

	modelConfig.Rewind();
	modelConfig.JumpToKey("models");

}

//Remove any skins from players (since they're items, they stay after) & re-enable any players
public void OnPluginEnd()
{
	for (int i = 0; i < sizeof(playerSkinItems); i++)
	{
		if (playerSkinItems[i])
		{
			//there's a skin, whether it's real or deleted somehow
			RemoveSkin(i);
			PrintToChat(i, "FreakModels is being unloaded or reloaded. Your skin has been removed (but feel free to re-apply it!)");
		}
	}
}

//Remove any skins from players that disconnect
public void OnClientDisconnect(int client)
{
	RemoveSkin(client);
}

public void OnEntityDestroyed(int entity)
{
	bool entFound = false;
	int i = 0;

	for (; i < sizeof(playerSkinItems); i++)
	{
		//this is called before the entity is actually removed i think
		//and this is called when the round changes & the skin is removed!
		//so we can re-enable the player on round change
		if (EntRefToEntIndex(playerSkinItems[i]) == entity)
		{
			entFound = true;
			break;
		}
	}

	if (entFound)
	{
		//i is the player whose skin was destroyed
		MakePlayerVisible(i);
		playerSkinItems[i] = 0;
	}
}


/*
 CORE FUNCTIONS
 =======================================
 */

/**
 * Creates & gives a wearable to a player.
 * @param target client ID of target.
 * @param model full path to model.
 * @return __REFERENCE__ ID of created entity, or -1 if error (not in game).
 */
int createWearable(int target, char[] model)
{
	//are they in game?
	TFTeam team = TF2_GetClientTeam(target);
	if (team != TFTeam_Blue && team != TFTeam_Red) return -1; //not in game

	//create item that will get skin
	int iSkinItem = TF2Items_GiveNamedItem(target, hDummyItemView);
	
	int rSkinItem = EntIndexToEntRef(iSkinItem);

	float pos[3];
	GetClientAbsOrigin(target, pos);
	
	DispatchKeyValueVector(rSkinItem, "origin", pos);
	DispatchKeyValue(rSkinItem, "model", model);

	SetEntPropString(rSkinItem, Prop_Data, "m_iClassname", "playermodel_wearable");
	//set team
	SetEntProp(iSkinItem, Prop_Send, "m_iTeamNum", team);
	SetEntPropFloat(rSkinItem, Prop_Send, "m_flPlaybackRate", 1.0);
	
	//give effects - bonemerge, animations, shadow
	int effects = GetEntProp(rSkinItem, Prop_Send, "m_fEffects");
	effects |= EF_BONEMERGE|EF_BONEMERGE_FASTCULL;
	effects |= EF_PARENT_ANIMATES;	
	effects &= ~(EF_NOSHADOW|EF_NORECEIVESHADOW);	
	SetEntProp(rSkinItem, Prop_Send, "m_fEffects", effects);
	
	//make it conform to the player's animations
	SetVariantString("!activator");
	AcceptEntityInput(rSkinItem, "SetParent", EntIndexToEntRef(target));
	SetEntProp(rSkinItem, Prop_Send, "m_bValidatedAttachedEntity", 1);
	
	SetEntPropEnt(rSkinItem, Prop_Send, "m_hOwnerEntity", target);
	
	if (!CheckModelGood(model))
	{
		PrintToServer("Warning: FreakModels: A non-precached skin model was about to be used. This would've crashed the server. (Error 32)");
		SetEntityModel(rSkinItem, "models/error.mdl");
		return rSkinItem;
	}
	SetEntityModel(rSkinItem, model);

	return rSkinItem;
}

/**
 * Makes a player invisible.
 * Exactly the reverse of MakePlayerVisible.
 * @param target Client ID of target.
 */
void MakePlayerInvisible(int target)
{
	SetEntityRenderMode(target, RENDER_NONE);
	int tarEffects = GetEntProp(target, Prop_Send, "m_fEffects");
	tarEffects |= (EF_NOSHADOW|EF_NORECEIVESHADOW);
	SetEntProp(target, Prop_Send, "m_fEffects", tarEffects);

}

/**
 * Makes a player visible.
 * Exactly the reverse of MakePlayerInvisible.
 * @param target Client ID of target.
 */
void MakePlayerVisible(int target)
{
	SetEntityRenderMode(target, RENDER_NORMAL);
	int tarEffects = GetEntProp(target, Prop_Send, "m_fEffects");
	tarEffects &= ~(EF_NOSHADOW|EF_NORECEIVESHADOW); //enable shadow
	SetEntProp(target, Prop_Send, "m_fEffects", tarEffects);
}

/**
 * set the player's skin.
 * creates an entity that acts as the skin and attaches it to the player,
 * also makes the player invisible
 * @return one of SetSkinOutput
 */
FuncOutput SetSkin(int target, char[] skinModel)
{
	//remove skin if it exists
	RemoveSkin(target);

	int rSkinItem = createWearable(target, skinModel);
	if (rSkinItem == -1) return SETSKIN_TARGETNOTEAM;
	playerSkinItems[target] = rSkinItem;
	
	//make player (i.e. anim model) invisible
	MakePlayerInvisible(target);

	//make skin item visible
	SetEntityRenderMode(rSkinItem, RENDER_NORMAL);
	//Q: arthurdead's plugins use this, reason?
	// SetEntityRenderMode(rSkinItem, RENDER_TRANSCOLOR);
	// SetEntityRenderColor(rSkinItem, 255, 255, 255, 255);
	
	//equip skin
	SDKCall(hEquipWearable, target, rSkinItem);

	return GOOD;
}

/**
 * Gets the target's skin and validates it exists.
 * @param target client ID of target.
 * @return Entity index of skin, or 0 if none exists.
 */
stock int GetSkin(int target)
{
	int iSkin = EntRefToEntIndex(playerSkinItems[client]);

	if (iSkin == INVALID_ENT_REFERENCE)
	{
		//the entity was deleted somehow
		RemoveSkin(target);
		return 0;
	}
	else return iSkin;
}

/**
 * Removes a skin from a player, deleting the skin entity and making the player animation model visible.
 * @param target Client ID of the target player.
 * @return true if a skin was removed, false if there was no skin to begin with.
 */
bool RemoveSkin(int target)
{
	//make player anim model visible
	MakePlayerVisible(target);
	
	//delete skin
	int index = EntRefToEntIndex(playerSkinItems[target]);
	if (playerSkinItems[target] && index != INVALID_ENT_REFERENCE)
	{
		AcceptEntityInput(index, "ClearParent");
		TF2_RemoveWearable(target, index);
		playerSkinItems[target] = 0;
		return true;
	}
	playerSkinItems[target] = 0;
	return false;
}

//set the client model, which dictates their animations (and appearance with no skin)
void SetAnim(int client, char[] model)
{
	if (!CheckModelGood(model))
	{
		PrintToServer("Warning: FreakModels: A non-precached animation model was about to be used. This would've crashed the server. (Error 31)");
		return;
	}
	SetVariantString(model);
	AcceptEntityInput(client, "SetCustomModel");
	SetEntProp(client, Prop_Send, "m_bUseClassAnimations", 1);
}

//reset the client model, which dictates their animations (and appearance with no skin)
void RemoveAnim(int client)
{
	SetVariantString("");
	AcceptEntityInput(client, "SetCustomModel");
}


/*
 INTERFACE FUNCTIONS
 =============================================
 */

Action MainCommand(int client, int args)
{
	char skinPath[PLATFORM_MAX_PATH], animPath[PLATFORM_MAX_PATH];
	char skinName[PLATFORM_MAX_PATH], animName[PLATFORM_MAX_PATH];
	char targetinput[256];
	int target = client, resetMode;
	bool useFullpaths = false, validArgEntered = false, reset = false;

	//first read the cmd args to figure out what to do

	char tmparg[128];

	for (int i = 1; i <= args; i++)
	{
		GetCmdArg(i, tmparg, sizeof(tmparg));

		if (StrEqual(tmparg, "-a", false) || StrEqual(tmparg, "-anim", false) || StrEqual(tmparg, "-animation", false))
		{
			validArgEntered = true;
			//read next cmd for anim
			i++;
			FuncOutput out = GetPathArg(args, i, animName, sizeof(animName));
			if (out != GOOD)
			{
				ReplyToCommand(client, out == GETPATHARG_NOVAL ? "Please enter a value for %s." : "You can't specify %s twice!", tmparg);
				return Plugin_Handled;
			}
		}
		else if 
		(
			StrEqual(tmparg, "-m", false) || StrEqual(tmparg, "-model", false) 
			|| StrEqual(tmparg, "-s", false) || StrEqual(tmparg, "-skin", false)
		)
		{
			validArgEntered = true;
			//read next cmd for skin
			i++;
			FuncOutput out = GetPathArg(args, i, skinName, sizeof(skinName));
			if (out != GOOD)
			{
				ReplyToCommand(client, out == GETPATHARG_NOVAL ? "Please enter a value for %s." : "You can't specify %s twice!", tmparg);
				return Plugin_Handled;
			}
		}
		else if
		(
			StrEqual(tmparg, "-t", false) || StrEqual(tmparg, "-target", false)
			|| StrEqual(tmparg, "-p", false) || StrEqual(tmparg, "-player", false)
		)
		{
			validArgEntered = true;
			//read next cmd for player
			i++;
			FuncOutput out = GetPathArg(args, i, targetinput, sizeof(targetinput));
			if (out != GOOD)
			{
				ReplyToCommand(client, out == GETPATHARG_NOVAL ? "Please enter a value for %s." : "You can't specify %s twice!", tmparg);
				return Plugin_Handled;
			}

			//get the player they actually want
			char targetName[65];
			target = GetClientFromUsername(client, targetinput, targetName, sizeof(targetName));
		}
		else if 
		(
			StrEqual(tmparg, "-f", false) || StrEqual(tmparg, "-full", false) 
			|| StrEqual(tmparg, "-fullpath", false) || StrEqual(tmparg, "-fullpaths", false)
			|| StrEqual(tmparg, "-path", false) || StrEqual(tmparg, "-paths", false)
		)
		{
			validArgEntered = true;
			useFullpaths = true;
		}
		else if (StrEqual(tmparg, "-h", false) || StrEqual(tmparg, "-help", false))
		{
			validArgEntered = true;
			PrintHelp(client);
		}
		else if (StrEqual(tmparg, "-r", false) || StrEqual(tmparg, "-reset", false))
		{
			validArgEntered = true;
			//TODO check if they entered a cmd arg that starts with a hyphen after this; if so set the reset mode to that, if not do both
			//TODO do this correctly
			reset = true;
		}
	}

	if (!validArgEntered)
	{
		if (args > 0)
		{
			ReplyToCommand(client, "Sorry, couldn't understand your arguments.");
		}
		ReplyToCommand(client, "Enter `freakmodel -help` to print help in the console.");
	}


	//now actually do the stuff

	if (reset)
	{
		if (animName[0] || skinName[0])
		{
			ReplyToCommand(client, "Sorry, you can't set the animation or skin & reset it in the same operation.");
			ReplyToCommand(client, "Please use only -anim/-skin or -reset.");
			return Plugin_Handled;
		}

		switch (resetMode)
		{
			case 0:
			{
				//reset both
				RemoveAnim(target);
				RemoveSkin(target);
				ReplyToCommand(client, "Successfully reset your skin and animations.");
			}
			case 1: 
			{
				//reset anim
				RemoveAnim(target);
				ReplyToCommand(client, "Successfully reset your animations.");
			}
			case 2:
			{
				//reset model
				RemoveSkin(target);
				ReplyToCommand(client, "Successfully reset your skin.");
			}
		}
	}

	//translate model names to paths
	if (!useFullpaths)
	{
		if (animName[0])
		{
			if (!modelConfig.GetString(animName, animPath, sizeof(animPath)))
			{
				ReplyToCommand(client, "Sorry, we couldn't find your animation model (%s) in our list. Check your spelling?", animName);
				animPath = "";
				animName = "";
			}
			
		}
		if (skinName[0])
		{
			if (!modelConfig.GetString(skinName, skinPath, sizeof(skinPath)))
			{
				ReplyToCommand(client, "Sorry, we couldn't find your skin model (%s) in our list. Check your spelling?", skinName);
				skinPath = "";
				skinName = "";
			}
		}
	}
	else
	{
		if (animName[0]) animPath = animName;
		if (skinName[0]) skinPath = skinName;
	}

	//check if the models are good
	if (animPath[0] && !IsModelPrecached(animPath))
	{
		if (useFullpaths)
		{
			if (FileExists(animPath, true)) 
				ReplyToCommand(client, "Sorry, your animation model is not precached and we cannot use it.");
			else 
				ReplyToCommand(client, "Unknown animation model!");
		} 
		else 
			//the file is set in the config but it wasn't precached... despite it was verified to exist before
			ReplyToCommand(client, "Sorry, an error occurred. Please report this to the server operator. (FreakModels error code: 21)");
		animPath = "";
	}
	if (skinPath[0] && !IsModelPrecached(skinPath))
	{
		if (useFullpaths)
		{
			if (FileExists(skinPath, true)) 
				ReplyToCommand(client, "Sorry, your skin model is not precached and we cannot use it.");
			else 
				ReplyToCommand(client, "Unknown skin model!");
		} 
		else 
			ReplyToCommand(client, "Sorry, an error occurred. Please report this to the server operator. (FreakModels error code: 22)");
		skinPath = "";
	}

	char targetString[65];

	if (animPath[0])
	{
		SetAnim(target, animPath);

		GetTargetString(client, target, targetString, sizeof(targetString));
		if (useFullpaths) 
			ReplyToCommand(client, "Successfully set %s animation model to %s.", targetString, animPath);
		else
		{
			ReplyToCommand(client, "Successfully set %s animations to \"%s\"", targetString, animName);
		}

	}
	if (skinPath[0])
	{
		GetTargetString(client, target, targetString, sizeof(targetString));
		
		if (SetSkin(target, skinPath) == SETSKIN_TARGETNOTEAM) 
			ReplyToCommand(client, "You can't set %s skin; %s aren't in the game!", targetString, (client == target) ? "you" : "they");
		//successfully set the skin confirmations:
		else if (useFullpaths) 
			ReplyToCommand(client, "Successfully set %s skin model to %s.", targetString, skinPath);
		else
		{
			ReplyToCommand(client, "Successfully set %s skin to \"%s\"", targetString, skinName);
		}
	}

	return Plugin_Handled;

}

Action ManageCommand(int client, int args)
{

	if (args < 1)
	{
		ManageCommandHelp(client);
		return Plugin_Handled;
	}

	char operation[65];
	GetCmdArg(1, operation, sizeof(operation));

	if(StrEqual(operation, "refreshconfig", false))
	{
		RefreshConfigFromFile();
		ReplyToCommand(client, "Configuration refreshed from file.");
		ReplyToCommand(client, "Map will need to be changed to precache any new models.");
	}
	else if (StrEqual(operation, "help", false))
	{
		ManageCommandHelp(client);
	}
	else
	{
		ReplyToCommand(client, "Unknown operation.");
	}

	return Plugin_Handled;
}


/*
 HELPER FUNCTIONS
 ============================================
 */

bool GetTargetString(int client, int target, char[] buffer, int buffersize)
{
	if (client == target) strcopy(buffer, buffersize, "your");
	else
	{
		char name[64];
		GetClientName(target, name, sizeof(name));
		StrCat(name, sizeof(name), "'s");
		strcopy(buffer, buffersize, name);
	}
	return client == target;
}

/**
 * Gets a value arg, performing user input checks along the way
 * @param args how many args are in the full command
 * @param i arg to get value of
 * @param path buffer to store value string into
 * @param pathsize size of path
 * @return one of GETPATHARG_ALREADYFILLED, GETPATHARG_NOVAL, or GOOD
 */
FuncOutput GetPathArg(int args, int i, char[] path, int pathsize)
{
	if (path[0]) return GETPATHARG_ALREADYFILLED;
	if (i > args || !checkArgIsVal(i)) return GETPATHARG_NOVAL;

	GetCmdArg(i, path, pathsize);
	return GOOD;
}

bool checkArgIsVal(int i)
{
	char arg[128];

	GetCmdArg(i, arg, sizeof(arg));
	return (arg[0] != '-');
}

Action TimedReply(Handle timer, Handle hndl)
{
	
	//fuck this. replytocommand doesn't print in the right order :(
	ArrayList list = view_as<ArrayList>(hndl);

	int client;
	if (list.Get(0) < 0)
	{
		client = 0;
	}
	else 
	{
		client = GetClientOfUserId(list.Get(0));	
	}
	
	
	switch(list.Get(1))
	{
		case 1:  { PrintToConsole(client, "Use freakmodel to change the model or animations of a player."); }
		case 2:  { PrintToConsole(client, "Important options: "); }
		case 3:  { PrintToConsole(client, "- Enter -skin <model> or -model <model> to set the appearance of a player to a model."); }
		case 4:  { PrintToConsole(client, "- Enter -anim <model> to set the animations of a player to a model's."); }
		// case 4: { PrintToConsole(client, "Enter -reset [anim|model] to reset the target's animation/model. If [anim|model] is omitted both will be reset."); }
		case 5:  { PrintToConsole(client, "- Enter -reset to reset a player's animation & skin."); }
		case 6:  { PrintToConsole(client, "--------------------------------"); }
		case 7:  { PrintToConsole(client, "Less important options: "); }
		case 8:  { PrintToConsole(client, "- Enter -fullpath to use the path to a model instead of a name. This requires knowledge of Source model paths & locations."); }
		case 9:  { PrintToConsole(client, "- Enter -target <username> to target a specific player. The command will target yourself if this is omitted."); }
		case 10: { PrintToConsole(client, "- Enter -help to print this dialogue in your console."); }
		case 11: { PrintToConsole(client, "All options (-skin, -anim, etc.) can also be specified with only the first letter (-s, -a, etc.) if you like."); }
		case 12: { PrintToConsole(client, "--------------------------------"); }
		case 13: { PrintToConsole(client, "Model names & their associated models are defined by the server operator. By default, you can use the classes (scout, medic, etc.), but there may be more!"); }
		case 14: { PrintToConsole(client, "--------------------------------"); }
		case 15:
		{
			PrintToConsole(client, "For example: inputting 'freakmodel -s heavy -t bob' will set bob's model to heavy, without changing their animations; inputting 'freakmodel -r' will reset your own model and animations.");
			delete list;
			return Plugin_Handled;
		}
	}
	list.Set(1, list.Get(1) + 1);
	return Plugin_Handled;
}

void PrintHelp(int client)
{
	ReplyToCommand(client, "Printing help in your console.");
	//this is horrible but ReplyToCommand doesn't want to print in the right order without it :(
	//list is this: [userId, lineNum]
	ArrayList list = new ArrayList();
	if (client < 1)
	{
		list.Push(-1);
	}
	else
	{
		list.Push(GetClientUserId(client));
	}
	
	list.Push(1);
	CreateTimer(0.1, TimedReply, list);
	CreateTimer(0.2, TimedReply, list);
	CreateTimer(0.3, TimedReply, list);
	CreateTimer(0.4, TimedReply, list);
	CreateTimer(0.5, TimedReply, list);
	CreateTimer(0.6, TimedReply, list);
	CreateTimer(0.7, TimedReply, list);
	CreateTimer(0.8, TimedReply, list);
	CreateTimer(0.9, TimedReply, list);
	CreateTimer(1.0, TimedReply, list);
	
	
}

void ManageCommandHelp(int client)
{
	ReplyToCommand(client, "Printing FreakModels manage command options.");
	ReplyToCommand(client, "- refreshconfig: Refreshes the model configuration from file. Map will need to be changed to precache & use any new models.");
}


bool IsValidClient(int client) { return client > 0 && client <= MaxClients && IsClientInGame(client); }

/**
 * gets client id of specified username. If none is found, returns -1.
 * @param client Client ID of caller.
 * @param user String to search against.
 * @param foundName Buffer string to store found username.
 */
int GetClientFromUsername(int client, char[] user, char[] foundName, int foundNameSize)
{
	//find a player to match the entered user
	int targetList[MAXPLAYERS];
	bool tn_is_ml;
	
	//trim quotes in case they added them
	StripQuotes(user);
	
	int targetFound = ProcessTargetString(user, client, targetList, MAXPLAYERS, COMMAND_FILTER_ALIVE, foundName, foundNameSize, tn_is_ml);
	
	if (targetFound <= 0)
	{
		//couldn't find one
		ReplyToTargetError(client, targetFound);
		return -1;
	}
	else
	{
		//could find one
		for (int i = 0; i < targetFound; i++)
		{
			if (IsValidClient(targetList[i]))
			{
				return targetList[i];
			}
		}
		//shouldn't happen - processtargetstring should have checked they're all valid
		ReplyToCommand(client, "Sorry, something went wrong. Try again? (Error code 10)");
		return -1;
	}
}


/*
 CONFIG FUNCTIONS
 ============================================
 */

/**
 * Refreshes the configuration from the config file. Creates one if necessary.
 */
void RefreshConfigFromFile()
{
	char configFilePath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, configFilePath, sizeof(configFilePath), FM_CONFIGFILEPATH);

	if (!FileExists(configFilePath))
		CreateConfigFile();

	if (modelConfig != null)
		delete modelConfig;

	modelConfig = ReadModelsFromConfig();
	if (!modelConfig.JumpToKey("models"))
		PrintToServer("Warning: FreakModels: Failed to read from config file, or there are no models defined in it. Consider adding some, or deleting the file to initialize it.");
}

/**
 * Creates a config file with the correct format.
 * @param addClasses Add the classes & their models to the config file by default.
 */
void CreateConfigFile(bool addClasses=true)
{
	char filePath[PLATFORM_MAX_PATH];

	BuildPath(Path_SM, filePath, sizeof(filePath), FM_CONFIGFILEPATH);

	KeyValues models = new KeyValues("FreakModels");
	models.JumpToKey("models", true);

	if (addClasses)
	{
		models.SetString("scout", "models/player/scout.mdl");
		models.SetString("soldier", "models/player/soldier.mdl");
		models.SetString("pyro", "models/player/pyro.mdl");
		models.SetString("demoman", "models/player/demo.mdl");
		models.SetString("demo", "models/player/demo.mdl"); //for backwards compat
		models.SetString("heavy", "models/player/heavy.mdl");
		models.SetString("engineer", "models/player/engineer.mdl");
		models.SetString("medic", "models/player/medic.mdl");
		models.SetString("sniper", "models/player/sniper.mdl");
		models.SetString("spy", "models/player/spy.mdl");
	}

	models.Rewind();
	models.ExportToFile(filePath);
}

/**
 * Reads the models configuration. Assumes it is created (if it isn't it will return an empty KV.)
 * @return A KeyValues of the models (root node "FreakModels", models stored in key "models")
 */
KeyValues ReadModelsFromConfig()
{
	char filePath[PLATFORM_MAX_PATH];

	BuildPath(Path_SM, filePath, sizeof(filePath), FM_CONFIGFILEPATH);

	KeyValues models = new KeyValues("FreakModels");
	models.ImportFromFile(filePath);
	return models;
}

bool CheckModelGood(char[] model)
{
	return IsModelPrecached(model);
}