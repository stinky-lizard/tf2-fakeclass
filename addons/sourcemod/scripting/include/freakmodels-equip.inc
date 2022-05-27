
FuncOutput AddEquippable(int target, char[] modelPath, int slot)
{

    PlayerData pData = PlayerData(target);

    //if there's an existing item, remove it
    if (pData.GetEquippable(slot))
        RemoveEquippable(target, slot);

    int rItem = 0;

    CreateWearable(target, modelPath, rItem);

    pData.SetEquippable(rItem);

    return GOOD;
}

void RemoveEquippable(int target, int slot)
{
    PlayerData pData = PlayerData(target);
    int rItem = pData.GetEquippable(slot);

    TF2_RemoveWearable(target, EntRefToEntIndex(rItem));

    if (IsValidEntity(rItem)) RemoveEntity(rItem);

    pData.SetEquippable(0, slot);
}