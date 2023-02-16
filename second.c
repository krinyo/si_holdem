//gcc -Wall -g -o second second.c -DDEBUG -libncurses(w)
//W - is really important
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SUITS_COUNT     4
#define SUIT_SIZE       8
#define NUMBERS_COUNT   13
#define NUMBER_SIZE     3

#define DECK_SIZE       52

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <locale.h>
#define SERVER_PORT     15000
#define MAX_CONNECTIONS 3
#define MIN_CONNECTIONS 2
#define RECONNECT_COUNT 5
#define WAIT_TIME       5
#define WAIT_TIMEOUT    5
#define NICKNAME_SIZE   6
#define MAX_ATTEMPTS    5
#define START_MONEY     5000
#define BUFSIZE         256
#define BUFF_SIZE       7
#define CLIENT_CARDS    2
#define START_SES_MON   0

#define MAX_MSG_SIZE    15
#define MAX_TABLE_CARDS 5
#define BIG_BLIND       100
#define START_BLIND_POS -1
#include <ncurses.h>

/*DEBUG*/
#ifdef DEBUG
#define DEBUG_PRINT     1
#else
#define DEBUG_PRINT     0
#endif
const char nicknames[MAX_CONNECTIONS][NICKNAME_SIZE] = {"Bob", "Jack" };//"Pete", "Rick", "Joe"};
//char suits[SUITS_COUNT][SUIT_SIZE] = {
//    {0xE2, 0x99, 0xA0, 0x00}, // ♠
//    {0xE2, 0x99, 0xA3, 0x00}, // ♣
//    {0xE2, 0x99, 0xA6, 0x00}, // ♦
//    {0xE2, 0x99, 0xA5, 0x00}  // ♥
//};
const char suits[SUITS_COUNT][SUIT_SIZE] = 
    { "\u2660", "\u2665", "\u2666", "\u2663" };

const char numbers[NUMBERS_COUNT][NUMBER_SIZE] = 
    { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" };

struct card{
    char suit[SUIT_SIZE];
    char number[NUMBER_SIZE];
    int suit_int;
    int number_int;
};
void show_card(struct card cd)
{
    printf("|%s %s|\n", cd.number, cd.suit);
}

struct deck{
    struct card *first_card;
    int current_size;
    struct card *start_pointer;
};
void fill_deck(struct deck *main_deck)
{
    int i, j, idx;
    struct card *first_card = main_deck->first_card;
    for(i = 0; i < NUMBERS_COUNT; i ++){
        for(j = 0; j < SUITS_COUNT; j ++){
            idx = i + (j * NUMBERS_COUNT);
            sprintf(first_card[idx].number, "%s", numbers[i]);
            sprintf(first_card[idx].suit, "%s", suits[j]);
        }
    }
}
struct deck *init_deck()
{
    struct deck *main_deck = malloc(sizeof(struct deck));
    main_deck->first_card = malloc(sizeof(struct card) * DECK_SIZE);
    if(main_deck->first_card == NULL){
        exit(1);
    }
    fill_deck(main_deck);
    main_deck->current_size = DECK_SIZE;
    main_deck->start_pointer = main_deck->first_card;
    return main_deck;
}
void show_all_deck_cards(struct deck main_deck){
    int i;
    for(i = 0; i < main_deck.current_size; i ++){
        show_card(main_deck.first_card[i]);
        //printf("%i \n", i);
    }
}
void swap_card_positions(struct deck *main_deck, int idx_fm, int idx_to){
    struct card temp;
    struct card *first_card = main_deck->first_card;

    memcpy(&temp, first_card+idx_fm, sizeof(struct card));
    memcpy(first_card+idx_fm, first_card+idx_to, sizeof(struct card));
    memcpy(first_card+idx_to, &temp, sizeof(struct card));
}
void shuffle_deck(struct deck *main_deck){
    int i, r;
    for(i = 0; i < DECK_SIZE; i ++){
        r = (rand() % (DECK_SIZE - 1)) + 1;
        swap_card_positions(main_deck, i, r);
    }
}
void pop_deck_card(struct deck *main_deck){
    main_deck->first_card ++;
    main_deck->current_size --;
}
void pop_deck_cards(struct deck *main_deck, int count){
    int i;
    for(i = 0; i < count; i ++){
        pop_deck_card(main_deck);
    }
}
void reset_deck(struct deck *main_deck){
    main_deck->first_card = main_deck->start_pointer;
    main_deck->current_size = DECK_SIZE;
}

struct client{
    struct sockaddr_in address;
    int descriptor;
    int money;
    int session_money;
    char nickname[NICKNAME_SIZE];
    int connected;
    int is_folded;
    
    struct card cards[CLIENT_CARDS];
    unsigned int turn_money;
    int combo_num;

    FILE *fd;
    SCREEN *scr;
    WINDOW *win;
};
struct server{
    int server_socket;
    struct sockaddr_in socket_params;
	int desc_count;
    struct client clients[MAX_CONNECTIONS];
};
void init_server_socket(struct server *main_server)
{
    main_server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    main_server->socket_params.sin_family = AF_INET;
    main_server->socket_params.sin_port = htons(SERVER_PORT);
    main_server->socket_params.sin_addr.s_addr = htonl(INADDR_ANY);
}
void try_to_bind_socket(struct server *main_server)
{
	int opt = 1;
	setsockopt(main_server->server_socket, SOL_SOCKET,
			SO_REUSEADDR, &opt, sizeof(opt));
    
	while(-1 == bind(main_server->server_socket,
                    (struct sockaddr*)&(main_server->socket_params),
                        sizeof(main_server->socket_params)))
    {
        if(DEBUG_PRINT){printf("Trying to bind server socket\n");}
        sleep(2);
    }
    if(DEBUG_PRINT){printf("Successfully binded!\n");}
}
struct server *init_server()
{
    struct server *main_server = malloc(sizeof(struct server));
    
    init_server_socket(main_server);
    try_to_bind_socket(main_server);
    listen(main_server->server_socket, MAX_CONNECTIONS);

    main_server->desc_count = 0;

    if(DEBUG_PRINT){ printf("Server is up. Listening on: %d:%d\n", INADDR_ANY, SERVER_PORT); }
    return main_server;
}
void show_server_info(struct server main_server)
{
    printf("SERVER INFO\n");
    printf("Descs count %i\n", main_server.desc_count);
    printf("END SERVER INFO\n");
}
void show_clients_info(struct server main_server)
{
    int i, j;
    for(i = 0; i < main_server.desc_count; i ++){
        printf("Descriptor %i =  %i\n", i, main_server.clients[i].descriptor);
        printf("Nickname = %s \n", main_server.clients[i].nickname);
        printf("Money = %i\n", main_server.clients[i].money);
        printf("Session money = %i \n", main_server.clients[i].session_money);
        for(j = 0; j < CLIENT_CARDS; j ++){
            show_card(main_server.clients[i].cards[j]);
        }
    }
}
void get_clients(struct server *main_server)
{
    fd_set readfds;
    struct timeval timeout;
    int attempts = 0;
    int result;
    while(attempts < MAX_ATTEMPTS || main_server->desc_count < MIN_CONNECTIONS){
        FD_ZERO(&readfds);
        FD_SET(main_server->server_socket, &readfds);

        timeout.tv_sec = WAIT_TIME;
        timeout.tv_usec = 0;

        result = select(main_server->server_socket + 1, &readfds, NULL, NULL, &timeout);
        if(-1 == result){
            printf("Error: select() failed!\n");
        }
        else if(0 == result){
            printf("Timeout: No new connections.\n");
            sleep(5);
        }
        else{
            if(main_server->desc_count >= MAX_CONNECTIONS){
                printf("REACHED MAXIMUM PLAYERS");
                break;
            }
            // Accept the incoming connection
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_socket = accept(main_server->server_socket, (struct sockaddr *) &client_addr, &client_addr_len);

            //add client in empty slot of array
            int i;
            for(i = 0; i < MAX_CONNECTIONS; i++) {
                if(main_server->clients[i].connected == 0) {
                    break;
                }
            }
            if(i == MAX_CONNECTIONS) {
                printf("Error: No empty slot found!\n");
                continue;
            }

            // Add the new client descriptor to the main_server structure and increment the descriptor count
            main_server->clients[i].address = client_addr;
            main_server->clients[i].descriptor = client_socket;
            strcpy(main_server->clients[i].nickname, nicknames[i]);
            main_server->clients[i].money = START_MONEY;
            main_server->clients[i].session_money = START_SES_MON;
            main_server->clients[i].connected = 1;
            main_server->desc_count++;
            printf("ACCEPTED\n");
            printf("DESC COUNTER = %i\n", main_server->desc_count);
            sleep(5);
        }
        attempts++;
    }
}
void get_client(struct server *main_server)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket = accept(main_server->server_socket, (struct sockaddr *) &client_addr, &client_addr_len);
    
    main_server->desc_count ++;
    main_server->clients[main_server->desc_count - 1].address = client_addr;
    main_server->clients[main_server->desc_count - 1].descriptor = client_socket;
    strcpy(main_server->clients[main_server->desc_count - 1].nickname, nicknames[main_server->desc_count - 1]);
    main_server->clients[main_server->desc_count - 1].money = START_MONEY;
    main_server->clients[main_server->desc_count - 1].connected = 1;
    printf("ACCEPTED CLIENT\n");

}
void check_clients(struct server *main_server) {

    for(int i = 0; i < main_server->desc_count; i++) {
        char buffer[BUFSIZE];
        memset(buffer, 0, BUFSIZE);
        if(main_server->clients[i].connected) {
            if(recv(main_server->clients[i].descriptor, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT) == 0) {
                printf("Client %s disconnected\n", main_server->clients[i].nickname);
                close(main_server->clients[i].descriptor);
                main_server->clients[i].connected = 0;
                main_server->desc_count--;
            }
        }
    }
}
void setup_players_screen(struct server *main_server)
{
    int i, curr_desc;
    
    for(i = 0; i < main_server->desc_count; i ++){
        curr_desc = main_server->clients[i].descriptor;
        //setlocale does something, don't actually understand
        setlocale(LC_ALL, "");
        //setlocale(LC_ALL, "en_US.UTF-8");
        main_server->clients[i].fd = fdopen(curr_desc, "rw");
        main_server->clients[i].scr = newterm(getenv("TERM"), main_server->clients[i].fd, main_server->clients[i].fd);
        set_term(main_server->clients[i].scr);
        main_server->clients[i].win = newwin(13, 42, 1, 2);
        mvwprintw(main_server->clients[i].win, 13/2, 42/2, suits[0]);
        //mvwprintw(main_server->clients[i].win, 13/2, 42/2, "%s", "♥");
        wrefresh(main_server->clients[i].win);
    }
}
struct table{
    struct card table_cards[MAX_TABLE_CARDS];
    unsigned int table_money;
    int table_cards_count;
    int winner_pos;
    int max_bet;
    int big_blind_sum;
    int big_blind_pos;
    int small_blind_pos;

    char table_message[MAX_MSG_SIZE];
};
struct table *init_table()
{
    struct table *main_table = malloc(sizeof(struct table));
    int i;
    for(i = 0; i < MAX_TABLE_CARDS; i ++){
        strcpy(main_table->table_cards[i].number, "");
        strcpy(main_table->table_cards[i].suit, "");
    }
    main_table->table_money = 0;
    main_table->table_cards_count = 0;
    main_table->winner_pos = -1;
    main_table->max_bet = 0;
    main_table->big_blind_sum = BIG_BLIND;
    main_table->big_blind_pos = START_BLIND_POS;
    strcpy(main_table->table_message, "TEST MESSAGE");
    return main_table;
};
void show_table_info(struct table main_table)
{
    int i;
    for(i = 0; i < main_table.table_cards_count; i ++){
        show_card(main_table.table_cards[i]);
    }
    printf("TABLE MONEY = %i\n", main_table.table_money);
    printf("TABLE CARDS COUNT = %i\n", main_table.table_cards_count);
    printf("TABLE MESSAGE %s \n", main_table.table_message);
}
//game funcs
void give_cards_to_client(struct client *client, struct deck *main_deck)
{
    int i;
    for(i = 0; i < CLIENT_CARDS; i ++){
        memcpy(&(client->cards[i]), main_deck->first_card, sizeof(struct card));
        pop_deck_card(main_deck);
    }
}
void give_cards_to_all_clients(struct server *main_server, struct deck *main_deck)
{
    int i;
    for(i = 0; i < main_server->desc_count; i ++){
        give_cards_to_client(&(main_server->clients[i]), main_deck);
    }
}
void put_cards_to_table(struct table *main_table, struct deck *main_deck, int count)
{
    int i;
    for(i = 0; i < count; i ++){
        memcpy(&(main_table->table_cards[main_table->table_cards_count]), main_deck->first_card, sizeof(struct card));
        pop_deck_card(main_deck);
        main_table->table_cards_count ++;
    }
}
int client_bet(struct client *curr_client, int sum)
{
    if(sum <= curr_client->money && sum > 0){
        curr_client->money -= sum;
        curr_client->session_money += sum;
        return 0;
    }
    else if(sum > curr_client->money){
        curr_client->session_money += curr_client->money;
        curr_client->money = 0;
        return 0;
    }
    else{
        return -1;
    }
}
void handle_client_commandd(struct client *curr_client)
{
    int command, bet_money;
    char buff[7];

    while(1){
        printf("in the cycle of command handling\n");
        set_term(curr_client->scr);
	    wmove(curr_client->win, 10, 2);
        flushinp();
        command = wgetch(curr_client->win);
        if(command == 'b'){
            recv(curr_client->descriptor, buff, 7, 0);
            printf("%s\n", buff);
            if((bet_money = atoi(buff)) != 0){
                client_bet(curr_client, bet_money);
                printf("%i\n", bet_money);
                break;
            }
        }
        else if(command == 'c'){
            printf("checked\n");
            break;
        }
        else if(command == 'f'){
            printf("folded\n");
            curr_client->is_folded = 1;
            break;
        }
        else{

        }
    }
}
void handle_client_commanddd(struct client *curr_client)
{
    int command, bet_money;
    char buff[BUFF_SIZE];

    while (1) {
        int bytes_received = 0;
        int ret;
        set_term(curr_client->scr);
	    wmove(curr_client->win, 10, 2);

        printf("Awaiting client command...\n");

        while (bytes_received < BUFF_SIZE) {
            ret = recv(curr_client->descriptor, &buff[bytes_received], BUFF_SIZE - bytes_received, 0);
            if (ret == -1) {
                perror("recv");
                return;
            }
            if (ret == 0) {
                printf("Client disconnected\n");
                return;
            }
            bytes_received += ret;
        }
        buff[BUFF_SIZE - 1] = '\0';

        if (buff[0] != 'b' && buff[0] != 'c' && buff[0] != 'f') {
            printf("Invalid command format.\n");
            continue;
        }

        switch (buff[0]) {
            case 'b':
                bet_money = atoi(&buff[1]);
                if (bet_money == 0 || bet_money > curr_client->money) {
                    printf("Invalid bet amount.\n");
                    wmove(curr_client->win, 10, 2);
                    continue;
                }
                client_bet(curr_client, bet_money);
                printf("Client bet %d\n", bet_money);
                break;
            case 'c':
                printf("Client checked\n");
                wmove(curr_client->win, 10, 2);
                break;
            case 'f':
                printf("Client folded\n");
                wmove(curr_client->win, 10, 2);
                curr_client->is_folded = 1;
                break;
        }
        wrefresh(curr_client->win);
        break;
    }
}
void handle_client_command(struct client *curr_client)
{
    int command, bet_money;
    char buff[BUFF_SIZE];

    while (curr_client->is_folded != 1) {
        int bytes_received = 0;
        int ret;

        printf("Awaiting client command...\n");

        while (bytes_received < BUFF_SIZE) {
            ret = recv(curr_client->descriptor, &buff[bytes_received], 1, 0);
            if (ret == -1) {
                perror("recv");
                return;
            }
            if (ret == 0) {
                printf("Client disconnected\n");
                return;
            }
            bytes_received += ret;
            if (buff[bytes_received - 1] == '\n') {
                break;
            }
        }
        buff[bytes_received - 1] = '\0';

        if (buff[0] != 'b' && buff[0] != 'c' && buff[0] != 'f') {
            printf("Invalid command format.\n");
            continue;
        }

        switch (buff[0]) {
            case 'b':
                bet_money = atoi(&buff[1]);
                if (bet_money == 0 || bet_money > curr_client->money) {
                    printf("Invalid bet amount.\n");
                    continue;
                }
                client_bet(curr_client, bet_money);
                printf("Client bet %d\n", bet_money);
                break;
            case 'c':
                printf("Client checked\n");
                break;
            case 'f':
                printf("Client folded\n");
                curr_client->is_folded = 1;
                break;
        }

        break;
    }
}
int clients_bets_equal(struct server *main_server)
{
    int i,
        prev_client_money = main_server->clients[0].session_money;
    for(i = 1; i < main_server->desc_count; i ++){
        if(main_server->clients[i].session_money != prev_client_money){
            return 0;
        }
        prev_client_money = main_server->clients[i].session_money;
    }
    return 1;
}
void handle_all_clients(struct server *main_server)
{
    int i = 0;
    /*for(i = 0; i < main_server->desc_count; i ++){
        handle_client_command(&main_server->clients[i]);
    }*/
    while(!clients_bets_equal(main_server)){
        if(i == main_server->desc_count){
            i = 0;
        }
        handle_client_command(&main_server->clients[i]);
        i++;
    }
}
void client_fold(struct client *curr_client, struct table *main_table)
{
    curr_client->is_folded = 1;
}
int client_check(struct client *curr_client)
{
    //turn goes to next player
    return 0;
}
void calculate_blinds_pos(struct server *main_server, struct table *main_table)
{
    int desc_count = main_server->desc_count;
    int big_blind_pos = main_table->big_blind_pos + 1;
    int small_blind_pos = big_blind_pos + 1;

    big_blind_pos = big_blind_pos <= desc_count ? big_blind_pos : (START_BLIND_POS + 1);
    small_blind_pos = small_blind_pos <= desc_count ? small_blind_pos : (START_BLIND_POS + 1);

    main_table->big_blind_pos = big_blind_pos;
    main_table->small_blind_pos = small_blind_pos;
}
void get_big_blind(struct server *main_server, struct table *main_table)
{
    client_bet(&(main_server->clients[main_table->big_blind_pos]), BIG_BLIND);
}
void get_small_blind(struct server *main_server, struct table *main_table)
{
    client_bet(&(main_server->clients[main_table->small_blind_pos]), (BIG_BLIND/2));
}
void get_blinds(struct server *main_server, struct table *main_table)
{
    calculate_blinds_pos(main_server, main_table);
    get_big_blind(main_server, main_table);
    get_small_blind(main_server, main_table);
}

void show_table_to_client(struct client *curr_client, struct table *main_table)
{
    set_term(curr_client->scr);
    curr_client->win = newwin(13, 42, 1, 2);
    //MENU
    mvwprintw(curr_client->win, 0, 0, "[F]old");
    mvwprintw(curr_client->win, 0, 7, "[C]heck");
    mvwprintw(curr_client->win, 0, 15, "[B]et'number'");
    //TABLE
    mvwprintw(curr_client->win, 5, 5, "%s", main_table->table_cards[0].number);
    mvwprintw(curr_client->win, 5, 7, "%s", main_table->table_cards[0].suit);
    mvwprintw(curr_client->win, 5, 9, "%s", main_table->table_cards[1].number);
    mvwprintw(curr_client->win, 5, 11, "%s", main_table->table_cards[1].suit);
    mvwprintw(curr_client->win, 5, 13, "%s", main_table->table_cards[2].number);
    mvwprintw(curr_client->win, 5, 15, "%s", main_table->table_cards[2].suit);
    mvwprintw(curr_client->win, 5, 17, "%s", main_table->table_cards[3].number);
    mvwprintw(curr_client->win, 5, 19, "%s", main_table->table_cards[3].suit);
    mvwprintw(curr_client->win, 5, 21, "%s", main_table->table_cards[4].number);
    mvwprintw(curr_client->win, 5, 23, "%s", main_table->table_cards[4].suit);
    mvwprintw(curr_client->win, 5, 25, "%s", main_table->table_cards[5].number);
    mvwprintw(curr_client->win, 5, 27, "%s", main_table->table_cards[5].suit);

    //PLAYER
    wrefresh(curr_client->win);
}
void show_table_to_clients(struct server *main_server, struct table *main_table)
{
    int i;
    for(i = 0; i < main_server->desc_count; i ++){
        show_table_to_client(&(main_server->clients[i]), main_table);
    }
}
int main()
{
    srand(time(NULL));

    struct server *main_server = init_server();
    struct deck *main_deck = init_deck();
    struct table *main_table = init_table();
    fill_deck(main_deck);
    
    //get_client(main_server);
    get_clients(main_server);
    //reset_deck(main_deck);
    shuffle_deck(main_deck);
    give_cards_to_all_clients(main_server, main_deck);
    show_clients_info(*main_server);
    show_table_info(*main_table);

    setup_players_screen(main_server);
    get_blinds(main_server, main_table);
    show_table_to_clients(main_server, main_table);
    //handle_client_command(&main_server->clients[0]);
    handle_all_clients(main_server);
    //show_all_deck_cards(*main_deck);
    show_clients_info(*main_server);
    sleep(5);
    return 0;

}