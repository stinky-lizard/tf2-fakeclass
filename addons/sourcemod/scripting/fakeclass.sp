#include <sourcemod>
#include <tf2_stocks>
// ^ tf2_stocks.inc itself includes sdktools.inc and tf2.inc
#include <tf2items>

#pragma semicolon 1
#pragma newdecls required

#define PLUGIN_VERSION "1.0"


#define EF_BONEMERGE 0x001
#define EF_BONEMERGE_FASTCULL 0x080
#define EF_PARENT_ANIMATES 0x200
#define EF_NODRAW 0x020
#define EF_NOSHADOW 0x010
#define EF_NORECEIVESHADOW 0x040


public Plugin myinfo = 
{
	name = "FakeClass",
	author = "Stinky Lizard",
	description = "Allows you to change your model and animations independently of your class. Made for FREAKSERVER 'The Most Fun You Can Have Online' at freak.tf2.host.",
	version = PLUGIN_VERSION,
	url = "freak.tf2.host"
};

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	// No need for the old GetGameFolderName setup.
	EngineVersion g_engineversion = GetEngineVersion();
	if (g_engineversion != Engine_TF2)
	{
		SetFailState("This plugin was made for use with Team Fortress 2 only.");
	}
} 

Handle hDummyItemView = null;
Handle hEquipWearable = null;
int playerSkinItems[MAXPLAYERS + 1];

public void OnPluginStart()
{
	/**
	 * @note For the love of god, please stop using FCVAR_PLUGIN.
	 * Console.inc even explains this above the entry for the FCVAR_PLUGIN define.
	 * "No logic using this flag ever existed in a released game. It only ever appeared in the first hl2sdk."
	 */
	CreateConVar("sm_fakeclass_version", PLUGIN_VERSION, "Standard plugin version ConVar. Please don't change me!", FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_DONTRECORD);
	
	LoadTranslations("common.phrases");

	hDummyItemView = TF2Items_CreateItem(OVERRIDE_ALL|FORCE_GENERATION);
	TF2Items_SetClassname(hDummyItemView, "tf_wearable");
	TF2Items_SetItemIndex(hDummyItemView, -1);
	TF2Items_SetQuality(hDummyItemView, 0);
	TF2Items_SetLevel(hDummyItemView, 0);
	TF2Items_SetNumAttributes(hDummyItemView, 0);


	
	GameData hGameConf = new GameData("fakeclass");
	if(hGameConf == null) {
		SetFailState("FakeClass: Gamedata not found.");
		return;
	}
	
	StartPrepSDKCall(SDKCall_Entity);
	PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CBasePlayer::EquipWearable");
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer);
	hEquipWearable = EndPrepSDKCall();
	if(hEquipWearable == null) {
		SetFailState("FakeClass: Failed to create SDKCall for CBasePlayer::EquipWearable.");
		delete hGameConf;
		return;
	}
	
	RegConsoleCmd("fakeclass", MainCommand);
	
}

//set the player's skin.
//creates an entity that acts as the skin and attaches it to the player,
//also makes the player invisible
void SetSkin(int target, char[] skinModel, int client)
{
	//remove skin if it exists
	RemoveSkin(target);

	//create item that will get skin
	int iSkinItem = TF2Items_GiveNamedItem(target, hDummyItemView);
	
	//save skin item
	int rSkinItem = EntIndexToEntRef(iSkinItem);
	playerSkinItems[target] = rSkinItem;
	
	float pos[3];
	GetClientAbsOrigin(target, pos);
	
	DispatchKeyValueVector(rSkinItem, "origin", pos);
	DispatchKeyValue(rSkinItem, "model", skinModel);

	SetEntPropString(rSkinItem, Prop_Data, "m_iClassname", "playermodel_wearable");

	SetEntPropFloat(rSkinItem, Prop_Send, "m_flPlaybackRate", 1.0);
	
	int effects = GetEntProp(rSkinItem, Prop_Send, "m_fEffects");
	effects |= EF_BONEMERGE|EF_BONEMERGE_FASTCULL;
	effects |= EF_PARENT_ANIMATES;	
	effects &= ~(EF_NOSHADOW|EF_NORECEIVESHADOW);	
	SetEntProp(rSkinItem, Prop_Send, "m_fEffects", effects);
	
	//make it conform to the player's animations
	SetVariantString("!activator");
	AcceptEntityInput(rSkinItem, "SetParent", EntIndexToEntRef(target));

	//set the team correctly
	//TODO: hook ubercharges & set the skin for when they're charged (add condition?)
	//seems to work actually
	switch (TF2_GetClientTeam(target))
	{
		case TFTeam_Blue, TFTeam_Red: { SetEntProp(iSkinItem, Prop_Send, "m_iTeamNum", TF2_GetClientTeam(target)); }
		default: 
		{
			ReplyToCommand(client, "You can't set your skin if you aren't in the game!");
			return;
		}
	}

	//make player (i.e. anim model) invisible
	SetEntityRenderMode(target, RENDER_NONE);
	int tarEffects = GetEntProp(target, Prop_Send, "m_fEffects");
	tarEffects |= (EF_NOSHADOW|EF_NORECEIVESHADOW);
	SetEntProp(target, Prop_Send, "m_fEffects", tarEffects);
	
	//make skin item visible
	SetEntityRenderMode(rSkinItem, RENDER_TRANSCOLOR);
	SetEntityRenderColor(rSkinItem, 255, 255, 255, 255);
	
	//equip skin
	SDKCall(hEquipWearable, target, rSkinItem);
	
	SetEntProp(rSkinItem, Prop_Send, "m_bValidatedAttachedEntity", 1);
	
	SetEntPropEnt(rSkinItem, Prop_Send, "m_hOwnerEntity", target);
	
	SetEntityModel(rSkinItem, skinModel);
}

//Gets the target's skin and validates it exists.
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

bool RemoveSkin(int client)
{
	//make player anim model visible
	SetEntityRenderMode(client, RENDER_NORMAL);
	int tarEffects = GetEntProp(client, Prop_Send, "m_fEffects");
	tarEffects &= ~(EF_NOSHADOW|EF_NORECEIVESHADOW); //enable shadow
	SetEntProp(client, Prop_Send, "m_fEffects", tarEffects);
	
	//delete skin
	int index = EntRefToEntIndex(playerSkinItems[client]);
	if (playerSkinItems[client] && index != INVALID_ENT_REFERENCE)
	{
		AcceptEntityInput(index, "ClearParent");
		TF2_RemoveWearable(client, index);
		playerSkinItems[client] = 0;
		return true;
	}
	playerSkinItems[client] = 0;
	return false;
}

//set the client model, which dictates their animations (and appearance with no skin)
void SetAnim(int client, char[] model)
{
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

bool GetPathArg(int client, int args, int i, char[] tmparg, char[] path, int pathsize)
{
	if (path[0])
	{
		//it's already set
		ReplyToCommand(client, "You can only specify %s once!", tmparg);
		return false;
	}
	if (i > args || !checkArgIsVal(i))
	{
		ReplyToCommand(client, "Please enter a value for %s.", tmparg);
		return false;
	}
	GetCmdArg(i, path, pathsize);
	return true;
}

Action MainCommand(int client, int args)
{
	char skinPath[PLATFORM_MAX_PATH], animPath[PLATFORM_MAX_PATH], playerinput[256], targetName[256];
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
			if (!GetPathArg(client, args, i, tmparg, animPath, sizeof(animPath)))
			{
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
			if (!GetPathArg(client, args, i, tmparg, skinPath, sizeof(skinPath)))
			{
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
			if (!GetPathArg(client, args, i, tmparg, playerinput, sizeof(playerinput)))
			{
				return Plugin_Handled;
			}

			//get the player they actually want
			target = GetClientFromUsername(client, playerinput, targetName, sizeof(targetName));
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
		ReplyToCommand(client, "Enter `fakeclass -help` to print help in the console.");
	}


	//now actually do the stuff

	if (reset)
	{
		if (animPath[0] || skinPath[0])
		{
			ReplyToCommand(client, "Sorry, you can't set the animation or model & reset it in the same operation.");
			ReplyToCommand(client, "Please use only -anim/-model or -reset.");
			return Plugin_Handled;
		}

		switch (resetMode)
		{
			case 0:
			{
				//reset both
				RemoveAnim(target);
				RemoveSkin(target);
				ReplyToCommand(client, "Successfully reset your model and animations.");
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
				ReplyToCommand(client, "Successfully reset your model.");
			}
		}
	}

	//translate classes to paths
	if (!useFullpaths)
	{
		if (animPath[0])
		{
			char classname[PLATFORM_MAX_PATH];
			strcopy(classname, sizeof(classname), animPath);
			animPath = "models/player/";
			StrCat(animPath, sizeof(animPath), classname);
			StrCat(animPath, sizeof(animPath), ".mdl");
			
		}
		if (skinPath[0])
		{
			char classname[PLATFORM_MAX_PATH];
			strcopy(classname, sizeof(classname), skinPath);
			skinPath = "models/player/";
			StrCat(skinPath, sizeof(skinPath), classname);
			StrCat(skinPath, sizeof(skinPath), ".mdl");
		}
	}

	//check if the models are good
	if (animPath[0] && !IsModelPrecached(animPath))
	{
		if (useFullpaths) ReplyToCommand(client, "Unknown animation model!");
		else ReplyToCommand(client, "Sorry, your animation class isn't a real class. Check your spelling?");
		animPath = "";
	}
	if (skinPath[0] && !IsModelPrecached(skinPath))
	{
		if (useFullpaths) ReplyToCommand(client, "Unknown skin model!");
		else ReplyToCommand(client, "Sorry, your skin class isn't a real class. Check your spelling?");
		skinPath = "";
	}

	if (animPath[0])
	{
		SetAnim(target, animPath);
	}
	if (skinPath[0])
	{
		SetSkin(target, skinPath, client);
	}

	return Plugin_Handled;

}

bool checkArgIsVal(int i)
{
	char arg[128];

	GetCmdArg(i, arg, sizeof(arg));
	return (arg[0] != '-');
}

public void OnMapStart()
{
	/**
	 * @note Precache your models, sounds, etc. here!
	 * Not in OnConfigsExecuted! Doing so leads to issues.
	 */
}


stock void GetModelForPlayerClass(int client, char[] model, int length)
{
	TFClassType class = TF2_GetPlayerClass(client);
	GetModelForClass(class, model, length);
}

stock void GetModelForClass(TFClassType class, char[] model, int length)
{
	switch(class)
	{
		case TFClass_Unknown: { strcopy(model, length, "models/error.mdl"); }
		case TFClass_Engineer: { strcopy(model, length, "models/player/engineer.mdl"); }
		case TFClass_Scout: { strcopy(model, length, "models/player/scout.mdl"); }
		case TFClass_Medic: { strcopy(model, length, "models/player/medic.mdl"); }
		case TFClass_Soldier: { strcopy(model, length, "models/player/soldier.mdl"); }
		case TFClass_Heavy: { strcopy(model, length, "models/player/heavy.mdl"); }
		case TFClass_DemoMan: { strcopy(model, length, "models/player/demo.mdl"); }
		case TFClass_Spy: { strcopy(model, length, "models/player/spy.mdl"); }
		case TFClass_Sniper: { strcopy(model, length, "models/player/sniper.mdl"); }
		case TFClass_Pyro: { strcopy(model, length, "models/player/pyro.mdl"); }
	}
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
		case 1: { PrintToConsole(client, "Use fakeclass to change the model or animations of a player."); }
		case 2: { PrintToConsole(client, "Enter -skin <class> or -model <class> to set the model of a player to a class's."); }
		case 3: { PrintToConsole(client, "Enter -anim <class> to set the animations of a player to a class's."); }
		case 4: { PrintToConsole(client, "Enter -fullpath to use the path to a model instead of a class. This requires knowledge of Source model paths & locations."); }
		case 5: { PrintToConsole(client, "Enter -target <username> to target a specific player. The command will target yourself if this is omitted."); }
		case 6: { PrintToConsole(client, "Enter -reset [anim|model] to reset the target's animation/model. If [anim|model] is omitted both will be reset."); }
		case 7: { PrintToConsole(client, "Enter -help to print this dialogue in your console."); }
		case 8: { PrintToConsole(client, "All options (-skin, -anim, etc.) can also be specified with only the first letter (-s, -a, etc.) if you like."); }
		case 9: { PrintToConsole(client, "-----"); }
		case 10:
		{
			PrintToConsole(client, "For example: inputting 'fakeclass -s heavy -t bob' will set bob's model to heavy, without changing their animations. Inputting 'fakeclass -r' will reset your own model and animations.");
			delete list;
			return Plugin_Stop;
		}
	}
	list.Set(1, list.Get(1) + 1);
	return Plugin_Stop;
}

public void PrintHelp(int client)
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

bool IsValidClient(int client) { return client > 0 && client <= MaxClients && IsClientInGame(client); }

//gets client id of specified username. If none is found, returns -1.
//@param client Client ID of caller.
//@param user String to search against.
//@param foundName Buffer string to store found username.
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
