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
#define MAX_CONNECTIONS 5
#define MIN_CONNECTIONS 2
#define RECONNECT_COUNT 5
#define WAIT_TIME       5
#define WAIT_TIMEOUT    5
#define NICKNAME_SIZE   6
#define MAX_ATTEMPTS    5
#define START_MONEY     5000
#define BUFSIZE         256
#define PLAYER_CARDS    2

#include <ncurses.h>
/*DEBUG*/
#ifdef DEBUG
#define DEBUG_PRINT     1
#else
#define DEBUG_PRINT     0
#endif
const char nicknames[MAX_CONNECTIONS][NICKNAME_SIZE] = {"Bob", "Jack", "Pete", "Rick", "Joe"};
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
    char nickname[NICKNAME_SIZE];
    int connected;
    
    struct card cards[PLAYER_CARDS];
    
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
    int i;
    printf("SERVER INFO\n");
    printf("Descs count %i\n", main_server.desc_count);
    for(i = 0; i < main_server.desc_count; i ++){
        printf("Descriptor %i =  %i\n", i, main_server.clients[i].descriptor);
        printf("Nickname = %s \n", main_server.clients[i].nickname);
        printf("Money = %i\n", main_server.clients[i].money);
    }
    printf("END SERVER INFO\n");
}

/*void get_clients(struct server *main_server)
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
            };
            // Accept the incoming connection
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_socket = accept(main_server->server_socket, (struct sockaddr *) &client_addr, &client_addr_len);

            // Add the new client descriptor to the main_server structure and increment the descriptor count
            main_server->descriptors[main_server->desc_count] = client_socket;
            main_server->desc_count++;
            printf("ACCEPTED\n");
            printf("DESC COUNTER = %i\n", main_server->desc_count);
            sleep(5);
        }
        attempts++;
    }
}*/
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
            main_server->clients[i].connected = 1;
            main_server->desc_count++;
            printf("ACCEPTED\n");
            printf("DESC COUNTER = %i\n", main_server->desc_count);
            sleep(5);
        }
        attempts++;
    }
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
        setlocale(LC_ALL, "");
	    main_server->clients[i].fd = fdopen(curr_desc, "rw");
		main_server->clients[i].scr = newterm(getenv("TERM"), main_server->clients[i].fd, main_server->clients[i].fd);
		set_term(main_server->clients[i].scr);
		main_server->clients[i].win = newwin(13, 42, 1, 2);
        mvwprintw(main_server->clients[i].win, 13/2, 42/2, suits[0]);
		flushinp();
		if(wgetch(main_server->clients[i].win) == 'e'){
            sleep(2);
		}     
    }
}
void setup_playerss_screen(struct server *main_server)
{
    int i, curr_desc;
    
    for(i = 0; i < main_server->desc_count; i ++){
        curr_desc = main_server->clients[i].descriptor;
        //setlocale(LC_ALL, "");
        setlocale(LC_ALL, "en_US.UTF-8");
        main_server->clients[i].fd = fdopen(curr_desc, "rw");
        main_server->clients[i].scr = newterm(getenv("TERM"), main_server->clients[i].fd, main_server->clients[i].fd);
        set_term(main_server->clients[i].scr);
        main_server->clients[i].win = newwin(13, 42, 1, 2);
        mvwprintw(main_server->clients[i].win, 13/2, 42/2, suits[0]);
        wrefresh(main_server->clients[i].win);
    }
}
struct player{};
struct table{};


int main()
{
    srand(time(NULL));

    //struct deck *main_deck = init_deck();
    //fill_deck(main_deck);
    //show_all_deck_cards(*main_deck);
    struct server *main_server = init_server();

    //initscr(); // инициализация ncurses
    //printw("\u2603"); // вывод символа "☃"
    //mvprintw(LINES/2, COLS/2, suits[0]); // перемещение курсора в центр экрана и вывод символа "☃"
    //refresh(); // обновление экрана
    //getch(); // ожидание нажатия клавиши
    //endwin(); // завершение работы ncurses
    //return 0;
    printf("%s\n", suits[0]);
    get_clients(main_server);
    setup_playerss_screen(main_server);
    //sleep(10);
    //while(1){
    //    show_server_info(*main_server);
    //    sleep(5);
    //}
    //sleep(5);
    //show_server_info(*main_server);
    
    //sleep(5);
    //check_clients(main_server);

    //get_clients(main_server);
    //show_server_info(*main_server);
    return 0;

}