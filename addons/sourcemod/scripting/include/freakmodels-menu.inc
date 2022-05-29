

void MainCommandMenu(int client)
{
    //first check if the client is the server. can't show a menu to the server
    if (!IsValidClient(client))
    {
        ReplyToCommand(client, "Please use the command options, which can be seen with `freakmodel -h`, when using this command from the server console.");
        return;
    }

    //todo
}