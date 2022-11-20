
public void OnInventoryApplied(Event event, const char[] name, bool dontBroadcast)
{
	int userId = event.GetInt("userid", -1);
	if (userId == -1) return;

	int clientId = GetClientOfUserId(userId);
	if (!IsValidClient(clientId)) return;
		
	//does this client have an animation set?
	PlayerData pData = PlayerData(clientId);

	pData.inventoryChanged = true;

	char animPath[PLATFORM_MAX_PATH];
	char skinPath[PLATFORM_MAX_PATH];

	pData.GetAnimPath(animPath, sizeof(animPath));
	pData.GetSkinPath(skinPath, sizeof(skinPath));

	if (animPath[0] || skinPath[0])
	{	
		CreateTimer(0.2, Timer_InventoryApplied, userId);
		CreateTimer(2.0, Timer_CheckPlayerAfterInventoryApplied, userId);
	}
}

public Action Timer_InventoryApplied(Handle timer, int userId)
{
	int client = GetClientOfUserId(userId);
	if (!IsValidClient(client)) return Plugin_Handled;
	
	PlayerData pData = PlayerData(client);
	
	char animPath[PLATFORM_MAX_PATH];
	pData.GetAnimPath(animPath, sizeof(animPath));
	SetAnim(client, animPath);
	
	//we should assume the skin's been deleted - if it has then pData hasn't been updated
	char skinPath[PLATFORM_MAX_PATH];
	pData.GetSkinPath(skinPath, sizeof(skinPath));
	SetSkin(client, skinPath);

	return Plugin_Handled;
}

public Action Timer_CheckPlayerAfterInventoryApplied(Handle timer, int userId)
{
	int client = GetClientOfUserId(userId);
	if (!IsValidClient(client)) return Plugin_Handled;

	PlayerData pData = PlayerData(client);

	char setAnimPath[PLATFORM_MAX_PATH];
	pData.GetAnimPath(setAnimPath, sizeof(setAnimPath));

	char curAnimPath[PLATFORM_MAX_PATH];
	GetClientModel(client, curAnimPath, sizeof(curAnimPath));

	char setSkinPath[PLATFORM_MAX_PATH];
	pData.GetSkinPath(setSkinPath, sizeof(setSkinPath));

	if (!StrEqual(curAnimPath, setAnimPath))
		SetAnim(client, setAnimPath);

	if (!IsValidEntity(pData.rSkinItem))
	{
		PrintToChatAll("A");
		SetSkin(client, setSkinPath);
	}

	return Plugin_Handled;
}