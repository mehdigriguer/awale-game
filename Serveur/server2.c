#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "client2.h"
#include "game_logic.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   GameState game;
   int game_started = 0;

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         clients[actual] = c;
         actual++;

         // Démarrer le jeu lorsque deux clients sont connectés
         if (actual == 2 && !game_started)
         {
            initialize_game(&game);
            game_started = 1;
            broadcast(clients, actual, "La partie commence!");
         }
      }
      else if (game_started)
      {
         for (i = 0; i < actual; i++)
         {
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);

               if (c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  game_started = 0;
                  send_message_to_all_clients(clients, client, actual, "Un joueur s'est déconnecté. La partie est annulée.", 1);
               }
               else
               {
                  int move = atoi(buffer);
                  int result = make_move(&game, game.current_turn, move);

                  // Vérifier si c'est le tour du joueur
                  if (i != game.current_turn)
                  {
                     write_client(client.sock, "Ce n'est pas votre tour.\n");
                  }
                  else 
                  {
                     if (result == -1)
                     {
                        write_client(client.sock, "Coup invalide. Essayez de nouveau.\n");
                     }
                     else if (result == -2)
                     {
                        write_client(client.sock, "Coup invalide l'adversaire. Essayez de nouveau.\n");
                     }
                     else if (result >= 0) {
                        char board_buffer[BUF_SIZE];
                        board_in_buffer(&game, board_buffer, sizeof(board_buffer));
                        broadcast(clients, actual, board_buffer);

                        if (check_game_end(&game)) {
                           int winner = determine_winner(&game);
                           if (winner == 0)
                                 send_message_to_all_clients(clients, client, actual, "Joueur 1 gagne!", 1);
                           else if (winner == 1)
                                 send_message_to_all_clients(clients, client, actual, "Joueur 2 gagne!", 1);
                           else
                                 send_message_to_all_clients(clients, client, actual, "Match nul!", 1);

                           game_started = 0;
                        } else {
                           game.current_turn = (game.current_turn + 1) % 2; // Passer au tour suivant seulement si le coup est valide
                           char turn_message[BUF_SIZE];
                           snprintf(turn_message, sizeof(turn_message), "C'est le tour du Joueur %d.\n", game.current_turn + 1);
                           broadcast(clients, actual, turn_message);
                        }
                     }
                  }
                  break;
               }
            }
         }
      }
   }
    clear_clients(clients, actual);
    end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void broadcast(Client *clients,  int actual, const char *buffer)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   strncat(message, buffer, sizeof message - strlen(message) - 1);
   for(i = 0; i < actual; i++)
   {
      write_client(clients[i].sock, message);
   }
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}