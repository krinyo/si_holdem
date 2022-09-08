#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <locale.h>

//DECK
#define DECK_SIZE       52
#define SUITS_SIZE      4
#define NUMBERS_SIZE    13
#define ONE_NUMBER_SIZE 3
#define UNICODE_EL_SIZE 4
//TABLE
#define MAX_TABLE_CARDS 5
#define MAX_PLAYERS     2
#define MIN_PLAYERS     2
#define START_STATE     1
#define START_SES_MONEY 0
#define START_TAB_CARDS 0
#define PREFLOP_PUT     0
#define FLOP_PUT        3
#define TURN_PUT        1
#define RIVER_PUT       1
//PLAYER
#define PLAYER_CARD_SUM 2
#define MAX_NAME_SIZE   256
#define TEST_PLAYER_MON 1000
#define ACTION_LENGTH	8
#define ACTIONS_COUNT	6
#define MESSAGE_SIZE	40
//SERVER
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_PORT     14888
#define MAX_CONNECTIONS 2
#define RECONNECT_COUNT 5
#define WAIT_TIMEOUT    5

//FRONTEND
#include <ncurses.h>

/*DEBUG*/
#ifdef DEBUG
#define DEBUG_PRINT     1
#else
#define DEBUG_PRINT     0
#endif

const char suits[SUITS_SIZE][UNICODE_EL_SIZE] = {"♠", "♥", "♦", "♣"};
//const char suits[SUITS_SIZE][UNICODE_EL_SIZE] = {"<3<", "<3", "<>", "+"};

const char numbers[NUMBERS_SIZE][ONE_NUMBER_SIZE] = 
        {"2", "3", "4", "5", "6", "7", "8", 
        "9", "10", "J", "Q", "K", "A"};
/*const char player_actions[ACTIONS_COUNT][ACTION_LENGTH] = 
	{"Fold", "Bet", "Raise",
	"Re-raise", "Check", "Call"};
*/
enum player_actions{
	Fold = 0,
	Bet,
	Raise,
	Check,
	Call
};
//-------------------------------------
struct card{
        char number[ONE_NUMBER_SIZE];
        char suit[UNICODE_EL_SIZE];
        struct card *next_card_ptr;
};
void show_card(struct card current_card)
{
        printf("|%s %s| \n", current_card.number, current_card.suit);
}
//------------------------------------
struct deck{
        int size;
        struct card *first_card;
	struct card *start_mem_point;
};
static struct card *fill_deck(struct card *first_card)
{
	int i, j;
        int counter = 0;
        for(i = 0; i < SUITS_SIZE; i ++){
                for(j = 0; j < NUMBERS_SIZE; j ++){
                        sprintf(first_card[counter].number,"%s", numbers[j]);
                        sprintf(first_card[counter].suit,"%s", suits[i]);
                        (first_card + counter)->next_card_ptr =
                                i != DECK_SIZE-1 ? first_card + counter + 1 : NULL;
                        counter ++;
                }
        }
	return first_card;
}
struct deck *init_deck()
{
        struct deck *deck_struct = malloc(sizeof(struct deck));
		
	deck_struct->size = DECK_SIZE;
	deck_struct->first_card = malloc(sizeof(struct card) * DECK_SIZE);
	deck_struct->start_mem_point = deck_struct->first_card;
	
	fill_deck(deck_struct->first_card);
        
	return deck_struct;
}
static void swap_card_positions(struct card *first_card, int first, int second)
{
        char temp_number[ONE_NUMBER_SIZE] = "";
        char temp_suit[UNICODE_EL_SIZE] = "";

        sprintf(temp_number, "%s", first_card[first].number);
        sprintf(temp_suit, "%s", first_card[first].suit);

        sprintf(first_card[first].number, "%s", first_card[second].number);
        sprintf(first_card[first].suit, "%s", first_card[second].suit);

        sprintf(first_card[second].number, "%s", temp_number);
        sprintf(first_card[second].suit, "%s", temp_suit);
}
void shuffle_deck(struct deck *deck_struct)
{
        int i, r;
        for(i = 0; i < DECK_SIZE; i ++){
                r = (rand() % (DECK_SIZE - 1)) + 1;
                //if(DEBUG_PRINT){ printf("RANDOM NUMBER %i \n", r); }
                swap_card_positions(deck_struct->first_card, i, r);
        }
}
void show_deck(struct deck *deck_struct)//for debug
{
        int i;
        for(i = 0; i < deck_struct->size; i++){
                printf("|%s %s| %i \n",
				(deck_struct->first_card+i)->number,
				(deck_struct->first_card+i)->suit, i);
        }
}
void remove_cards_from_deck(struct deck *deck_struct, int count)
{
	int i;
	for(i = 0; i < count; i ++){
		deck_struct->first_card =
			deck_struct->first_card->next_card_ptr;
	}
	deck_struct->size -= count;
}
void reset_deck(struct deck *deck_struct)
{
	deck_struct->first_card = deck_struct->start_mem_point;
	deck_struct->size = DECK_SIZE;
}
//------------------------------------
enum player_states{
	no_betted = 1,
	betted
};
struct player{
        int table_position;
        unsigned int player_money;
        int descriptor;
	FILE *file_descriptor;
	enum player_states player_state;
	char nickname[MAX_NAME_SIZE];
	struct card player_cards[PLAYER_CARD_SUM];
	int player_cards_sum;
	SCREEN *player_screen;
	WINDOW *player_window;
	int is_folded;
	unsigned int turn_money;
	//char player_message[MESSAGE_SIZE];
};

struct player init_player(int table_pos, unsigned int mon,
		int desc, FILE *f_desc, SCREEN *scr, WINDOW *win, char nickname[])
{
	struct player *current_player = malloc(sizeof(struct player));
	current_player->table_position = table_pos;
	current_player->player_money = mon;
	current_player->descriptor = desc;
	current_player->file_descriptor = f_desc;
	current_player->player_state = no_betted;
	current_player->player_cards_sum = 0;
	current_player->player_screen = scr;
	current_player->player_window = malloc(sizeof(WINDOW));
	current_player->player_window = win;
	sprintf(current_player->nickname, "%s", nickname);
	current_player->is_folded = 0;
	current_player->turn_money = 0;
	//sprintf(current_player->player_message, "%s", "");
	return *(current_player);
}
void show_player_cards(struct player current_player)
{
        int i;
        printf("%s %i\n",current_player.nickname,
                        current_player.player_money);
        for(i = 0; i < PLAYER_CARD_SUM; i ++){
                printf("|%s %s| \n", current_player.player_cards[i].number,
                                current_player.player_cards[i].suit);
        }
}
//------------------------------------
enum table_states{
        waiting = 1,
        preflop,
        flop,
        turn,
        river,
        winner
};
struct table{
        enum table_states table_state;
        int players_count;
        unsigned int session_money;
	int table_cards_count;
	struct card table_cards[MAX_TABLE_CARDS];
	struct player players[MAX_PLAYERS];
	char players_message[MESSAGE_SIZE];
};

struct table *init_table()
{
        struct table *main_table = malloc(sizeof(struct table));	
        main_table->table_state = START_STATE;
        main_table->players_count = 0;
	main_table->table_cards_count = 0;
	main_table->session_money = START_SES_MONEY;
	sprintf(main_table->players_message, "%s", "");
        return main_table;
}
void show_table_info(struct table *main_table)
{
	printf("TABLE INFO:\n");
	printf("%i table state\n", main_table->table_state);
	printf("%i players count\n", main_table->players_count);
	printf("%i table money\n", main_table->session_money);
	printf("%i cards on table\n", main_table->table_cards_count);
	int i;
	printf("PLAYERS----------\n");
	for(i = 0; i < main_table->players_count; i ++){
		show_player_cards(main_table->players[i]);
	}
	printf("TABLE CARDS----------\n");
	for(i = 0; i < main_table->table_cards_count; i ++){
		show_card(main_table->table_cards[i]);
	}

}
//------------------------------------

struct server{
        int server_socket;
        struct sockaddr_in socket_params;
        int descriptors[MAX_PLAYERS];
	int desc_count;
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
                if(DEBUG_PRINT){printf("Trying to bind socket\n");}
                sleep(2);
        }
        if(DEBUG_PRINT){printf("SUCCESSFULLY BINDED!\n");}
}
struct server *init_server()
{
        struct server *main_server = malloc(sizeof(struct server));
        
        init_server_socket(main_server);
        try_to_bind_socket(main_server);
        listen(main_server->server_socket, MAX_CONNECTIONS);

        if(DEBUG_PRINT){ printf("Listening on: %d:%d\n", INADDR_ANY, SERVER_PORT); }
        return main_server;
}
void get_client_descriptors(struct server *main_server)
{
        int conn_counter = 0,
	    getting_tryings = 0,
            server_socket = main_server->server_socket,
            client_descs[MAX_PLAYERS];
        while(conn_counter < MIN_PLAYERS && getting_tryings < RECONNECT_COUNT){
                if((client_descs[conn_counter] =
                                        accept(server_socket, NULL, NULL)) != -1){
		      	main_server->descriptors[conn_counter] = client_descs[conn_counter];
			main_server->desc_count ++;
                	if(DEBUG_PRINT){printf("ACCEPTED!\n");}
			conn_counter ++;
                }
                if(DEBUG_PRINT){printf("WAITING FOR PLAYERS!\n");}
                getting_tryings ++;
                sleep(WAIT_TIMEOUT);
        }
}
//--------------------------------------------//

void place_players_to_table(struct server *main_server,
		struct table *main_table)
{
	char nickname[8];
	get_client_descriptors(main_server);
	int desc_counter;
	FILE *fd;
	SCREEN *scr;
	WINDOW *win;
	for(desc_counter = 0; desc_counter < main_server->desc_count; desc_counter ++){
		sprintf(nickname, "%s%i", "Player", desc_counter);
		setlocale(LC_ALL, "");
		fd = fdopen(main_server->descriptors[desc_counter], "rw");
		scr = newterm(getenv("TERM"), fd, fd);
		set_term(scr);
		win = newwin(13, 42, 1, 2);
		mvwprintw(win, 13/2, 42/2, "Enter e to play:");
		if(wgetch(win) == 'e'){
			main_table->players[desc_counter] = init_player(desc_counter,
					TEST_PLAYER_MON,
					main_server->descriptors[desc_counter],
					fd, scr, win, nickname);
			main_table->players_count ++;
		}
	}
	main_server->desc_count = 0;
}
void give_cards_to_player(struct player *current_player, struct deck *deck_ptr)
{
        int i;
        for(i = 0; i < PLAYER_CARD_SUM; i ++){
                sprintf(current_player->player_cards[i].suit,"%s",
				(deck_ptr->first_card + i)->suit);
                sprintf(current_player->player_cards[i].number,"%s",
                                (deck_ptr->first_card + i)->number);
		current_player->player_cards_sum ++;
        }
	remove_cards_from_deck(deck_ptr, PLAYER_CARD_SUM);
}

void give_cards_to_all_players(struct table *main_table, struct deck *deck_ptr)
{
	int i;
	for(i = 0; i < main_table->players_count; i ++){
		give_cards_to_player(&(main_table->players[i]), deck_ptr);
	}
}
void put_cards_to_table(struct table *main_table, struct deck *deck_ptr, int count)
{
	int i, index;
	int counter = 0;
	for(i = 0; i < count; i ++){
		index = main_table->table_cards_count;
		sprintf(main_table->table_cards[index].suit, "%s",
				(deck_ptr->first_card + counter)->suit);
		sprintf(main_table->table_cards[index].number, "%s",
				(deck_ptr->first_card + counter)->number);
		counter ++;
		main_table->table_cards_count ++;
		//main_table->cards[i];
	}
	remove_cards_from_deck(deck_ptr, count);
}
/*void draw_player(struct player *player, WINDOW *pl_win, int draw_cards)
{
	int nick_x, nick_y, mon_x, mon_y, c_one_x, c_one_y, c_two_x, c_two_y;
	*mvwprintw(pl_win, 10, 21, main_table->players[i].nickname);

	sprintf(player_money_str, "%s", "$");
	sprintf(player_money_str + 1, "%d", main_table->players[i].player_money);
	mvwprintw(pl_win, 11, 11, player_money_str);

	mvwprintw(pl_win, 11, 21, main_table->players[i].player_cards[0].number);
	mvwprintw(pl_win, 11, 23, main_table->players[i].player_cards[0].suit);

	mvwprintw(pl_win, 11, 26, main_table->players[i].player_cards[1].number);
	mvwprintw(pl_win, 11, 28, main_table->players[i].player_cards[1].suit);
}*/
void show_table_to_players(struct table *main_table)
{
	int i,j;

	//int i, width, height;
	//FILE *fh;
	SCREEN *scr;
	WINDOW *pl_win;
	char player_money_str[256];
	char table_money[256];
	char turn_money[256];
	for(i = 0; i < main_table->players_count; i ++){
		scr = main_table->players[i].player_screen;
		pl_win = main_table->players[i].player_window;
		set_term(scr);
		wclear(pl_win);
		
		box(pl_win, 1, 0);
		//MENU
		mvwprintw(pl_win, 0, 0, "[F]old");
		mvwprintw(pl_win, 0, 7, "[C]heck");
		//mvwprintw(pl_win, 0, 11, "Raise");
		mvwprintw(pl_win, 0, 12, "[B]et'number'");
		
		//TABLE
		mvwprintw(pl_win, 5, 5, main_table->table_cards[0].number);
		mvwprintw(pl_win, 5, 7, main_table->table_cards[0].suit);
		mvwprintw(pl_win, 5, 9, main_table->table_cards[1].number);
		mvwprintw(pl_win, 5, 11, main_table->table_cards[1].suit);
		mvwprintw(pl_win, 5, 13, main_table->table_cards[2].number);
		mvwprintw(pl_win, 5, 15, main_table->table_cards[2].suit);
		mvwprintw(pl_win, 5, 17, main_table->table_cards[3].number);
		mvwprintw(pl_win, 5, 19, main_table->table_cards[3].suit);
		mvwprintw(pl_win, 5, 21, main_table->table_cards[4].number);
		mvwprintw(pl_win, 5, 23, main_table->table_cards[4].suit);
		mvwprintw(pl_win, 5, 25, main_table->table_cards[5].number);
		mvwprintw(pl_win, 5, 27, main_table->table_cards[5].suit);

		sprintf(table_money, "%s", "$");
		sprintf(table_money + 1, "%d", main_table->session_money);
		mvwprintw(pl_win, 6, 21, table_money);

		//PLAYER1
		sprintf(turn_money, "%s", "$");
		sprintf(turn_money + 1, "%d", main_table->players[0].turn_money);
		mvwprintw(pl_win, 8, 18, turn_money);

		mvwprintw(pl_win, 9, 18, main_table->players[0].nickname);

		sprintf(player_money_str, "%s", "$");
		sprintf(player_money_str + 1, "%d", main_table->players[0].player_money);
		mvwprintw(pl_win, 10, 18, player_money_str);
		
		mvwprintw(pl_win, 1, 1, main_table->players_message);
		
		if(i == 0){
			mvwprintw(pl_win, 11, 18,
					main_table->players[0].player_cards[0].number);
			mvwprintw(pl_win, 11, 20,
					main_table->players[0].player_cards[0].suit);
			mvwprintw(pl_win, 11, 22,
				main_table->players[0].player_cards[1].number);
			mvwprintw(pl_win, 11, 24,
					main_table->players[0].player_cards[1].suit);
			mvwprintw(pl_win, 6, 31, "| || |");
		}
		//PLAYER2
		//mvwprintw(pl_win, 8, 18, main_table->players[i].turn_money);
		sprintf(turn_money, "%s", "$");
		sprintf(turn_money + 1, "%d", main_table->players[1].turn_money);
		mvwprintw(pl_win, 3, 31, turn_money);
	
		mvwprintw(pl_win, 4, 31, main_table->players[1].nickname);

		sprintf(player_money_str, "%s", "$");
		sprintf(player_money_str + 1, "%d", main_table->players[1].player_money);
		mvwprintw(pl_win, 5, 31, player_money_str);
		
		mvwprintw(pl_win, 1, 1, main_table->players_message);

		if(i == 1){
			mvwprintw(pl_win, 6, 31,
					main_table->players[1].player_cards[0].number);
			mvwprintw(pl_win, 6, 33,
					main_table->players[1].player_cards[0].suit);
			mvwprintw(pl_win, 6, 35,
				main_table->players[1].player_cards[1].number);
			mvwprintw(pl_win, 6, 37,
					main_table->players[1].player_cards[1].suit);
			mvwprintw(pl_win, 11, 18, "| || |");
		}
		

		//ENDING
		wmove(pl_win, 10, 2);	
		wrefresh(pl_win);
		//wgetch(pl_win);
	}	
}

void bet(struct player *current_player, unsigned int count)
{
	if(current_player->player_money >= count){
		current_player->turn_money += count;
		current_player->player_money -= count;
	}
	else{

	}
}
void handle_command(struct player *current_player, struct table *main_table)
{
	char ch;
	char buff[7];
	int bet_money;

			
test_command:
	sprintf(main_table->players_message , "%s %d", "Turn of Player",
			current_player->table_position);
	show_table_to_players(main_table);
	set_term(current_player->player_screen);
	wmove(current_player->player_window, 10, 2);
	flushinp();
	ch = wgetch(current_player->player_window);
	if(/*wgetch(current_player->player_window)*/ ch == 'b'){
		recv(current_player->descriptor, buff, 7, 0);
		if((bet_money = atoi(buff)) != 0){
			bet(current_player, bet_money);
		}
		else{
			goto test_command;
		}
	}
	else if(ch == 'c')
	{
		wmove(current_player->player_window, 10, 2);
		printf("This is your char|%c|", ch);		
	}
	else if(ch == 'f')
	{
		current_player->is_folded = 1;	
	}
	else{
		sprintf(main_table->players_message, "%s", "Not right command");
		wmove(current_player->player_window, 10, 2);
		printf("This is your char|%c|", ch);
		show_table_to_players(main_table);
		goto test_command;
	}
}
void handle_all_players(struct table *main_table)
{
	int i;
	for(i = 0 ; i < main_table->players_count; i ++){
		handle_command(&(main_table->players[i]), main_table);
		show_table_to_players(main_table);
	}
}
int enough_players(struct table *main_table)
{
	int players_in_game = main_table->players_count;
	int players_fold;
	int i;
	for(i = 0; i < players_in_game; i ++){
		if(main_table->players[i].is_folded == 0){
			players_fold += 1;
		}
	}
	return (players_in_game - players_fold) >= 2 ? 1 : 0;
}

int equal_players_betting(struct table *main_table)
{
	int i,
	    first = main_table->players[0].turn_money;
	for(i = 1 ; i < main_table->players_count; i ++){
		if(first != main_table->players[i].turn_money){
			return 0;
		}
	}
	return 1;
}
void make_blinds(struct table *main_table)
{
	int i;
	for(i = 0; i < main_table->players_count; i ++){
		if(i == 0){
			bet(&main_table->players[i], 10);	
		}
		if(i == 1){
			bet(&main_table->players[i], 15); 
		}
	}
}
//--------------------------------------------//
int main()
{
	srand(time(NULL));	

	struct server *main_server = init_server();
	struct table *main_table = init_table();
	struct deck *main_deck = init_deck();

	place_players_to_table(main_server, main_table);
	shuffle_deck(main_deck);
	give_cards_to_all_players(main_table, main_deck);
	make_blinds(main_table);
	show_table_info(main_table);
	show_table_to_players(main_table);
	do{
		handle_all_players(main_table);
		//handle_command(&main_table->players[0], main_table);
		//handle_command(&main_table->players[1], main_table);

	}
	while(!equal_players_betting(main_table));
	put_cards_to_table(main_table, main_deck, FLOP_PUT);
	show_table_to_players(main_table);
	
	do{
		//handle_command(&main_table->players[0], main_table);
		//handle_command(&main_table->players[1], main_table);
		handle_all_players(main_table);

	}
	while(!equal_players_betting(main_table));
	put_cards_to_table(main_table, main_deck, TURN_PUT);
	show_table_to_players(main_table);

	do{
		//handle_command(&main_table->players[0], main_table);
		//handle_command(&main_table->players[1], main_table);
		handle_all_players(main_table);

	}
	while(!equal_players_betting(main_table));
	put_cards_to_table(main_table, main_deck, RIVER_PUT);
	show_table_to_players(main_table);

	do{
		//handle_command(&main_table->players[0], main_table);
		//handle_command(&main_table->players[1], main_table);
		handle_all_players(main_table);

	}
	while(!equal_players_betting(main_table));
	show_table_info(main_table);
	sleep(100);
}
