
Action EquipCommand(int client, int args)
{
	if (args < 1)
	{
		Equip_PrintHelp(client);
		return Plugin_Handled;
	}

	int targetList[MAXPLAYERS];
	int targetsFound = 1;
	int slot;

	char operation[32];
	GetCmdArg(1, operation, sizeof(operation));

	operation[0] = CharToLower(operation[0]);

	if (operation[0] == 'h')
		Equip_PrintHelp(client);
	else if (operation[0] == 'i')
	{
		//get target if exists
		if (args < 2 && IsValidClient(client))
			targetList[0] = client;
		else if (args < 2)
		{
			ReplyToCommand(client, "Please enter a player to view the FreakModels equip info of.");
			return Plugin_Handled;
		}
		else
		{
			char targetInput[MAX_NAME_LENGTH];
			GetCmdArg(2, targetInput, sizeof(targetInput));
			bool foundOne = GetClientsFromUsername(client, targetInput, targetsFound, targetList, sizeof(targetList));
			if (!foundOne) return Plugin_Handled; //client was already alerted their input was invalid by GetClientsFromUsername
		}

		char playerName[MAX_NAME_LENGTH];
		char modelPath[PLATFORM_MAX_PATH];

		for (int p = 0; p < targetsFound; p++)
		{
			GetClientName(targetList[p], playerName, sizeof(playerName));

			PlayerData pData = PlayerData(targetList[p]);

			static int slotsLength;
			slotsLength = pData.equippableSlotsLength;

			if (slotsLength < 1)
			{
				ReplyToCommand(client, "%s has no equipped models.", playerName);
				continue;
			}

			for (int i = 1; i <= slotsLength; i++)
			{
				if (pData.GetEquippable(i) && IsValidEntity(pData.GetEquippable(i)))
				{			
					GetEntPropString(pData.GetEquippable(i), Prop_Data, "m_ModelName", modelPath, sizeof(modelPath));
					ReplyToCommand(client, "%s: Slot %i: %s", playerName, i, modelPath)
				}
			}
		}
	}
	else if (operation[0] == 'r')
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
			if (!foundOne) return Plugin_Handled; //client was already alerted their input was invalid by GetClientsFromUsername
		}

		char playerName[MAX_NAME_LENGTH];

		//reset it
		if (doAll)
		{
			for (int i = 0; i < targetsFound; i++)
			{
				if (targetList[i] == client) ReplyToCommand(client, "Resetting all FreakModels equips on you.");
				else 
				{
					GetClientName(targetList[i], playerName, sizeof(playerName));
					ReplyToCommand(client, "Resetting all FreakModels equips on %s.", playerName);
				}

				PlayerData pData = PlayerData(targetList[i]);
				int max = pData.equippableSlotsLength;
				for (int j = 1; j <= max; j++)
				{
					if (pData.GetEquippable(j) && IsValidEntity(pData.GetEquippable(j)))
						RemoveEquippable(targetList[i], j);
				}
				pData.InitEquipList();
			}
		}
		else
		{
			char modelPath[PLATFORM_MAX_PATH];

			for (int i = 0; i < targetsFound; i++)
			{
				int toRemove = PlayerData(targetList[i]).GetEquippable(slot);
				if (!toRemove)
				{
					ReplyToCommand(client, "There is no model in slot %i.", slot);
				}
				else
				{
					GetEntPropString(toRemove, Prop_Data, "m_ModelName", modelPath, sizeof(modelPath));
					if (targetList[i] == client) ReplyToCommand(client, "Removing your FreakModels equip in slot %i. (%s)", slot, modelPath);
					else 
					{
						GetClientName(targetList[i], playerName, sizeof(playerName));
						ReplyToCommand(client, "Removing %s's FreakModels equip in slot %i. (%s)", playerName, slot, modelPath);
					}

					RemoveEquippable(targetList[i], slot);
				}
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
	if (!rItem) return; //its already 0

	if (IsValidEntity(rItem)) TF2_RemoveWearable(target, EntRefToEntIndex(rItem));

	if (IsValidEntity(rItem)) RemoveEntity(rItem);

	pData.SetEquippable(0, slot);
}

void Equip_PrintHelp(int client)
{
	ReplyToCommand(client, "Printing fm_equip help in your console.");

	PrintToConsole(client, "Use fm_equip to add a model to a player as a wearable.\nThe player will not actually receive the item in their inventory, nor will it change their stats in any way.\nOptions:\n- Enter `fm_equip add` to add a model to a player.\n- Enter `fm_equip remove` to remove a model from a player.\n- Enter `fm_equip info` to view a player's current models.\n\nAdd mode usage: `fm_equip add <model> <slot> [target] [-fullpath]\n- In place of <model>, add the name of the model as defined by the server operator.\n- In place of <slot>, add the slot (between 1 and 99) to add the wearable to on the player.\n- Optional: In place of [target], enter a player username (or a multi-target, e.g. @all) to target another player.\n- Optional: Add -fullpath or -f at the end to use the path to a model instead of a model's name. This can use any loaded model.\n");
	PrintToConsole(client, "Remove mode usage: `fm_equip remove <slot> [target]\n- In place of <slot>, add the slot containing the wearable you want to remove.\n- Optional: In place of [target], enter a player username (or a multi-target, e.g. @all) to target another player.\n\nInfo mode usage: `fm_equip info <target>\n- In place of <target>, enter a player username (or a multi-target, e.g. @all) to view that player's info.\n");

}
