
/*
 * Stuff for the regular cleanup, by default every 2 minutes.
 */

public Action Timer_RegularCleanup(Handle timer)
{
	CleanupWearables();
	return Plugin_Continue;
}

KeyValues g_cleanedEnts = null;

void CleanupWearables(bool onlyLost = true)
{
	if (!g_cleanedEnts)
		g_cleanedEnts = new KeyValues("CleanedEntities");


	for (int i = 0; i < allWearables.Length; i++)
	{
		int rItem = allWearables.Get(i);

		char rItemStr[32];
		IntToString(rItem, rItemStr, sizeof(rItemStr));

		if ((onlyLost && ItemUsedBy(rItem) == -1) || !onlyLost)
		{
			//should be removed
			if (!IsValidEntity(rItem) || g_cleanedEnts.GetNum(rItemStr))
			{
				//is deleted
				allWearables.Erase(i);
				//we've shifted the array down, so we need to shift i down as well
				i--;

				g_cleanedEnts.DeleteKey(rItemStr);
			}
			else
			{

				if (ItemUsedBy(rItem) != -1)
				{
					//will only happen if onlyLost is false
					TF2_RemoveWearable(ItemUsedBy(rItem), EntRefToEntIndex(rItem));
					g_cleanedEnts.SetNum(rItemStr, 1);
				}

				//is not already deleted -- want else if bc it's still valid after being removed tho its marked for deletion
				else if (IsValidEntity(rItem)) 
				{
					RemoveEntity(rItem);	
					g_cleanedEnts.SetNum(rItemStr, 1);
				}
			}
		}
	}
}

/**
 * Returns the player using an item, meaning if it's referenced in g_playersData
 * TODO: when addding cosmetics add checks for cosmetics
 * @param itemReference Reference ID of the item to check.
 * @return Client index of the client using the item, or -1 if it's not used.
 */
int ItemUsedBy(const int itemReference)
{
	int usedBy = -1;

	bool done = false;

	//i = 1 to skip server
	for(int i = 1; i < PLAYERSDATASIZE; i++)
	{
		PlayerData pData = PlayerData(i);
		if (itemReference == pData.rSkinItem) 
		{
			usedBy = i;
			break;
		}
		int max = pData.equippableSlotsLength;
		for (int j = 1; j <= max; j++)
		{
			if (itemReference == pData.GetEquippable(j))
			{
				usedBy = i;
				done = true;
				break;
			}
		}
		if (done) break;
	}

	return usedBy;
}
