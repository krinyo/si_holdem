int main()
{
    int i;

    struct server   *MS = init_server();
    struct deck     *MD = init_deck();
    struct table    *MT = init_table();

    fill_deck(MD);

    while(1){
        get_client_descriptors(MS);
        shuffle_deck(MD);
        give_cards_to_all_clients(MS, MD);
        get_blinds(MS, MT); //ready
        show_table_to_clients(MT);  //not ready

        while(players_bets_equal(MS)){  //ready
            handle_client_command(MS->clients[i]);
            show_table_to_clients(MT);
            kick_folded_players(MS); //not ready
        }

        put_cards_to_table(MD, MT, 3);// not ready
        
        for(i = 0; i < MS->desc_count; i ++){
            handle_client_command(MS->clients[i]);
            show_table_to_clients(MT);
            kick_folded_players(MS); //not ready
        }
        
        put_cards_to_table(MD, MT, 1);// not ready

        for(i = 0; i < MS->desc_count; i ++){
            handle_client_command(MS->clients[i]);
            show_table_to_clients(MT);
            kick_folded_players(MS); //not ready
        }

        put_cards_to_table(MD, MT, 1);// not ready

        for(i = 0; i < MS->desc_count; i ++){
            handle_client_command(MS->clients[i]);
            show_table_to_clients(MT);
            kick_folded_players(MS); //not ready
        }
        reset_clients_structs();//folded change // not ready
        find_winner(MS, MT); //not ready
    }
}