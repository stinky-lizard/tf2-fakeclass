
Action EquipCommand(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "Printing fm_equip help in your console.");
		//todo: help
	}

	int targetList[MAXPLAYERS];
	int targetsFound = 1;
	int slot;

	char operation[32];
	GetCmdArg(1, operation, sizeof(operation));


	if (operation[0] == 'r')
	{
		//want to reset
	
		bool doAll = false;

		if (args < 2)
		{
			//theres no slot arg
			ReplyToCommand(client, "Please enter a slot to remove the FreakModels equip from, or 'all' to remove all of them.");
			return Plugin_Handled;
		}

		char slotArg[] = "000";
		GetCmdArg(2, slotArg, sizeof(slotArg));
		if (slotArg[0] == 'a')
		{
			//lets assume they meant all
			ReplyToCommand(client, "Resetting all FreakModels equips.");
			doAll = true;
		}
		else
		{
			slot = StringToInt(slotArg);
			if (slot < 1)
			{
				ReplyToCommand(client, "Sorry, FreakModels equip slots start at 1; you entered %i.", slot);
				return Plugin_Handled;
			}
			else if (slot > 99)
			{
				ReplyToCommand(client, "Sorry, you can only have up to 99 FreakModels equip slots. You entered %i.", slot);
				return Plugin_Handled;
			}
		}

		//get target if exists
		if (args < 3 && IsValidClient(client))
			targetList[0] = client;
		else if (args < 3)
		{
			ReplyToCommand(client, "Please enter a player to target as a third argument.");
			return Plugin_Handled;
		}
		else
		{
			char targetInput[MAX_NAME_LENGTH];
			GetCmdArg(3, targetInput, sizeof(targetInput));
			bool foundOne = GetClientsFromUsername(client, targetInput, targetsFound, targetList, sizeof(targetList));
			if (!foundOne) return Plugin_Handled;
		}

		char playerName[MAX_NAME_LENGTH];

		//reset it
		if (doAll)
		{
			for (int i = 0; i < targetsFound; i++)
			{
				PlayerData pData = PlayerData(targetList[i]);
				for (int j = 1; j <= pData.equippableSlotsLength; i++)
					if (pData.GetEquippable(j)) RemoveEquippable(targetList[i], j);
				pData.InitEquipList();
			}
		}
		else
		{
			for (int i = 0; i < targetsFound; i++)
			{
				if (pData.GetEquippable(slot) RemoveEquippable(targetList[i], slot);
			}
		}
	}
	else if (operation[0] == 'e' || operation[0] == 'a' || operation[0] == 's')
	{
		if (args < 2)
		{
			//theres no model arg
			ReplyToCommand(client, "Please enter a model to add and a slot to add the model to (in that order).");
			return Plugin_Handled;
		}
		else if (args < 3)
		{
			//theres no slot arg
			ReplyToCommand(client, "Please enter a number as a slot to add the model to.");
			return Plugin_Handled;
		}

		//get model
		char modelName[PLATFORM_MAX_PATH];
		GetCmdArg(2, modelName, sizeof(modelName));

		char modelPath[PLATFORM_MAX_PATH];
		strcopy(modelPath, sizeof(modelPath), modelName);
		
		//get & check slot
		char slotArg[] = "000";
		GetCmdArg(3, slotArg, sizeof(slotArg));
		slot = StringToInt(slotArg);

		if (slot < 1)
		{
			ReplyToCommand(client, "Sorry, FreakModels equip slots start at 1; you entered %i.", slot);
			return Plugin_Handled;
		}
		else if (slot > 99)
		{
			ReplyToCommand(client, "Sorry, you can only have up to 99 FreakModels equip slots. You entered %i.", slot);
			return Plugin_Handled;
		}

		//get target if exists
		if (args < 4 && IsValidClient(client))
			targetList[0] = client;
		else if (args < 4)
		{
			ReplyToCommand(client, "Please enter a player to target as a third argument.");
			return Plugin_Handled;
		}
		else
		{
			char targetInput[MAX_NAME_LENGTH];
			GetCmdArg(4, targetInput, sizeof(targetInput));
			bool foundOne = GetClientsFromUsername(client, targetInput, targetsFound, targetList, sizeof(targetList));
			if (!foundOne) return Plugin_Handled;
		}

		bool useFullpaths = false;

		//get if full paths are used & translate them if so
		if (args >= 5)
		{
			char optionArg[64];

			GetCmdArg(5, optionArg, sizeof(optionArg));

			if
			(
				StrEqual(optionArg, "-f", false) || StrEqual(optionArg, "-full", false) 
				|| StrEqual(optionArg, "-fullpath", false) || StrEqual(optionArg, "-fullpaths", false)
				|| StrEqual(optionArg, "-path", false) || StrEqual(optionArg, "-paths", false)
			)
			{
				//if there's a -f option
				//modelName and modelPath will be the name
				useFullpaths = true;
			}
		}

		if (!useFullpaths)
		{
			ToLowerCase(modelName, sizeof(modelName)); //make model names case-agnostic
			if (!GetModelFromConfig(modelName, modelPath, sizeof(modelPath)))
			{
				ReplyToCommand(client, "Sorry, we couldn't find your model (%s) in our list. Check your spelling?", modelName);
				return Plugin_Handled;
			}	
		}

		//replace \s with /s
		ReplaceString(modelPath, sizeof(modelPath), "\\", "/");

		//check if the models are good
		if (!IsModelPrecached(modelPath))
		{
			if (useFullpaths)
			{
				if (FileExists(modelPath, true))
					ReplyToCommand(client, "Sorry, your model is not precached and we cannot use it.");
				else 
					ReplyToCommand(client, "Unknown model!");
			} 
			else 
				//the file is set in the config but it wasn't precached... despite it was verified to exist before
				ReplyToCommand(client, "Sorry, an error occurred. Please report this to the server operator. (FreakModels error code: 50)");
			return Plugin_Handled;
		}

		//add the equippable

		char playerName[MAX_NAME_LENGTH];

		for (int i = 0; i < targetsFound; i++)
		{
			FuncOutput out = AddEquippable(targetList[i], modelPath, slot);
			GetClientName(targetList[i], playerName, sizeof(playerName));
			//test
			PrintToChatAll("%s equippables length is %i.", playerName, PlayerData(targetList[i]).equippableSlotsLength);
			if (targetList[i] == client)
			{
				ReplyToCommand(client, "You are now wearing \"%s\" in slot %i.", modelName, slot);
				if (out == REMOVEDEQUIP) ReplyToCommand(client, "Let it be known: you already had something equipped in slot %i - this has been overwritten.", slot);
			}
			else
			{
				ReplyToCommand(client, "%s is now wearing \"%s\" in slot %i.", playerName, modelName, slot);
				if (out == REMOVEDEQUIP) ReplyToCommand(client, "Let it be known: %s already had something equipped in slot %i - this has been overwritten.", playerName, slot);
			}
		}
	}

	return Plugin_Handled;
}


FuncOutput AddEquippable(int target, char[] modelPath, int slot)
{
    bool removedOne = false;

    PlayerData pData = PlayerData(target);

    //if there's an existing item, remove it
    if (pData.GetEquippable(slot))
    {
        RemoveEquippable(target, slot);
        removedOne = true;
    }

    int rItem = 0;

    CreateWearable(target, modelPath, rItem);

    pData.SetEquippable(rItem, slot);


    return removedOne ? REMOVEDEQUIP : GOOD;
}

void RemoveEquippable(int target, int slot)
{
    PlayerData pData = PlayerData(target);
    int rItem = pData.GetEquippable(slot);

    if (IsValidEntity(rItem)) TF2_RemoveWearable(target, EntRefToEntIndex(rItem));

    if (IsValidEntity(rItem)) RemoveEntity(rItem);

    pData.SetEquippable(0, slot);
}