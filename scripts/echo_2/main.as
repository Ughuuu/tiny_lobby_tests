namespace main {
    string echo(string message)
    {
        print("Hello echo " + message);
        lobby::start_timer("echo", 1, array<any> = {any(message)});
        Lobby@ l = lobby::get();
        print("What is my id ?");
        print(l.id);
        return message + "abc";
    }
    string test1(string msg1, int msg2, bool msg3, float msg4, array<int> msg5, dictionary msg6)
    {
        print("Hello echo\n");
        return msg1;
    }
    int test2(string msg1, int msg2, bool msg3, float msg4, array<int> msg5, dictionary msg6)
    {
        print("Hello echo\n");
        return msg2;
    }
    bool test3(string msg1, int msg2, bool msg3, float msg4, array<int> msg5, dictionary msg6)
    {
        print("Hello echo\n");
        return msg3;
    }
    float test4(string msg1, int msg2, bool msg3, float msg4, array<int> msg5, dictionary msg6)
    {
        print("Hello echo\n");
        return msg4;
    }
    array<int> test5(string msg1, int msg2, bool msg3, float msg4, array<int> msg5, dictionary msg6)
    {
        print("Hello echo\n");
        return msg5;
    }
    dictionary test6(string msg1, int msg2, bool msg3, float msg4, array<int> msg5, dictionary msg6)
    {
        print("Hello echo\n");
        return msg6;
    }

    void _on_create()
    {
        print("Game created!\n");
    }

    void _on_join()
    {
        print("A player joined!\n");
    }

    void _on_chat(string message)
    {
        print("Chat: " + message + "\n");
    }

    void _on_tags(dictionary tags)
    {
        array<string> keys = tags.getKeys();
        for (uint i = 0; i < keys.length(); i++)
        {
            string key = keys[i];
            string value;
            tags.get(key, value);
            print("Tag: " + key + " = " + value + "\n");
        }
    }

    void _on_kick(string kicked_peer_id)
    {
        print("Player kicked: " + kicked_peer_id + "\n");
    }

    void _on_ready(bool ready)
    {
        print("Ready state: " + (ready ? "Ready" : "Not Ready") + "\n");
    }

    void _on_seal(bool seal)
    {
        print("Room sealed: " + (seal ? "Yes" : "No") + "\n");
    }

    void _on_left()
    {
        print("A player left the game.\n");
    }

    void _on_tick()
    {
        print("On tick\n");
    }
}
