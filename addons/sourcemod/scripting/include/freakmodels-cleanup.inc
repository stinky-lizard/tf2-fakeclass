
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
			}
			else
			{
                //TODO: check if this entity is worn by going thru every client and checking m_hMyWearables
                //if it is, then use TF2_RemoveWearable

				//is not already deleted
				if (IsValidEntity(rItem)) RemoveEntity(rItem);
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
	int usedBy;

	for(int i = 0; i < sizeof(g_playersData); i++)
	{
		if (itemReference == g_playersData[i].rSkinItem) usedBy = i;
	}

	return usedBy;
}
