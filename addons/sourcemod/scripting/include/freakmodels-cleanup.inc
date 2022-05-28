
/*
 * Stuff for the regular cleanup, by default every 3 minutes.
 */

public Action Timer_RegularCleanup(Handle timer)
{
	CleanupWearables();
}

void CleanupWearables(bool onlyLost = true)
{
	for (int i = 0; i < allWearables.Length; i++)
	{
		int rItem = allWearables.Get(i);
		if ((onlyLost && ItemUsedBy(rItem) == -1) || !onlyLost)
		{
			//should be removed
			if (!IsValidEntity(rItem))
			{
				//is deleted
				allWearables.Erase(i);
				//we've shifted the array down, so we need to shift i down as well
				i--;
			}
			else
			{
				if (ItemUsedBy(rItem) != -1)
					//will only happen if onlyLost is false
					TF2_RemoveWearable(ItemUsedBy(rItem), rItem);

				//is not already deleted
				if (IsValidEntity(rItem)) 
					RemoveEntity(rItem);
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
		for (int j = 1; j <= pData.equippableSlotsLength; j++)
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
