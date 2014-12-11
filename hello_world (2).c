#include "usb/usb.h"
#include "altera_up_avalon_usb.h"
#include "system.h"
#include "sys/alt_timestamp.h"

#include <assert.h>
#define MAX 300
#define MAXP 20

int main() {
	int alllosers = 0; // If everyone is a 1 in the array players
	int players[MAXP] = { 0 }; // In the index of the id states whether they are disconnected (0), lost (1), still playing (2)
	int winner = 0; // If there is a winner
	int totalplayers = 0;
	int specialid = 1; // The id of whoevers turn it is
	int newid;
	int moves = 0; // Length of current correct message
	int pass = 1;
	int five = 5;
	int nine = 9;
	int seven = 7;
	int i;
	int bytes_expected;
	int id_recvd;
	int id_byte;
	int bytes_recvd;
	int total_recvd;
	unsigned char data; // the size of the incoming message
	unsigned char id;
	unsigned char correct_message[MAX];
	unsigned char turn[11];
	unsigned char wrong[7];
	unsigned char player_win[9];
	unsigned char wronglength = 5;
	unsigned char turnlength = 9;
	unsigned char player_win_length = 7;

	correct_message[2] = 'Y';
	correct_message[3] = 'B';
	correct_message[4] = 'R';
	correct_message[5] = 'G';

	wrong[2] = 'w';
	wrong[3] = 'r';
	wrong[4] = 'o';
	wrong[5] = 'n';
	wrong[6] = 'g';

	turn[2] = 'Y';
	turn[3] = 'o';
	turn[4] = 'u';
	turn[5] = 'r';
	turn[6] = ' ';
	turn[7] = 'T';
	turn[8] = 'u';
	turn[9] = 'r';
	turn[10] = 'n';

	player_win[2] = 'Y';
	player_win[3] = 'o';
	player_win[4] = 'u';
	player_win[5] = ' ';
	player_win[6] = 'W';
	player_win[7] = 'o';
	player_win[8] = 'n';

	printf("USB Initialization\n");
	alt_up_usb_dev * usb_dev;
	usb_dev = alt_up_usb_open_dev("/dev/usb_0");
	assert(usb_dev);
	usb_device_init(usb_dev, USB_0_IRQ);

	printf("Polling USB device.  Run middleman now!\n");

	alt_timestamp_start();
	//printf("%d", alt_timestamp());
	int clocks = 0;
	while (clocks < 50000000 * 10) {
		clocks = alt_timestamp();
		usb_device_poll();
	}

	while (1) {

		unsigned char message_rx[MAX];
		bytes_expected = 0;
		bytes_recvd = 0;

		printf("Polling USB device.  Run middleman now!\n");
		alt_timestamp_start();
		//printf("%d", alt_timestamp());
		int clocks = 0;
		while (clocks < 50000000 * 1) {
			clocks = alt_timestamp();
			usb_device_poll();
		}
		printf("Done polling USB\n");

		printf("Sending the message to the Middleman\n");

		// Now receive the message from the Middleman
		printf("Waiting for data to come back from the Middleman\n");

		// First byte is the id in our message
		bytes_expected = 1;
		id_recvd = 0;
		while (id_recvd < bytes_expected) {
			id_byte = usb_device_recv(&id, 1);
			if (id_byte > 0)
				id_recvd += id_byte;
		}
		id_byte = 0;

		int actualid = (int) id;
		correct_message[0] = (char) id;
		wrong[0] = id;
		printf("The id is: %d\n", actualid);

		// Second byte is the size in our message
		bytes_expected = 1;
		total_recvd = 0;
		while (total_recvd < bytes_expected) {
			bytes_recvd = usb_device_recv(&data, 1);
			if (bytes_recvd > 0)
				total_recvd += bytes_recvd;
		}
		bytes_recvd = 0;
		correct_message[1] = data;
		wrong[1] = data;
		unsigned char message_length = data;
		printf("%c", data);

		int num_to_receive = (int) data;
		printf("About to receive %d characters:\n", num_to_receive);

		// Receive entire message
		bytes_expected = num_to_receive;
		total_recvd = 0;
		while (total_recvd < bytes_expected) {
			bytes_recvd = usb_device_recv(message_rx + total_recvd + 2, 1);
			if (bytes_recvd > 0)
				total_recvd += bytes_recvd;
		}
		bytes_recvd = 0;

		for (i = 0; i < num_to_receive; i++) {
			printf("%c", message_rx[i]);
		}

		// Checks to see if a new player needs to be added (if the first character is an N)
		if (message_rx[2] == 'N') {
			players[actualid] = 2;
			totalplayers++;
			printf("Players:");
			for (i = 0; i < MAXP; i++) {
				printf(" %i,", players[i]);
			}
			printf("\n");
			message_rx[2] = '\0';

			if (totalplayers == 1) {
				specialid = actualid;
			}
		} else if (num_to_receive == 0) {
			// Disconnects a player if the size they sent was a zero
			printf("Disconnecting player");
			players[actualid] = 0;
			totalplayers = totalplayers - 1;
			printf("Peeps: %i", totalplayers);
			// If it was the disconnected player's turn give someone else the turn
			if (actualid == specialid) {
				do {
					specialid = rand() % 20;
				} while (players[specialid] != 2 && totalplayers != 0);
				if (totalplayers == 0)
					specialid = 1;
			}
			// Check if there is a winner (disconnected players are a zero in the array,
			//connected but players who are out are a 1, and still eligable players are a 2)
			winner = 0;
			for (i = 0; i < MAXP; i++) {
				if (players[i] == 2)
					winner++;
			}
			if (winner == 1) {
				player_win[1] = (unsigned char) seven;
				usb_device_send(player_win, player_win_length + 2);
			}

		} else if (specialid == actualid) {
			// If whomever sent the message was actually their turn, parse the message
			pass = 1;
			// Check to see if the messages were the same or not
			for (i = 2; i < (moves + 2); i++) {
				if (message_rx[i] != correct_message[i]) {
					pass = 0;
					break;
				}
				pass = 1;
			}
			if (message_rx[i] == '\0' || num_to_receive != moves + 1) {
				pass = 0;
			}
			printf("Sending the message to the Middleman\n");

			if (pass == 1) {
				// Send the new correct string to everyone
				printf("passed\n");
				correct_message[i] = message_rx[i];
				correct_message[0] = 0xFF;
				usb_device_send(correct_message, message_length + 2);
				// Reset the message received to blank characters
				for (i = 0; i < MAX; i++) {
					message_rx[i] = '\0';
				}
				moves++;
			} else {
				// Set whomever got it wrong to the loser pool and send the error message to them
				printf("Loser: %i", actualid);
				players[actualid] = 1;
				printf("failed\n");
				wrong[1] = (unsigned char) five;
				usb_device_send(wrong, wronglength + 2);

				for (i = 0; i < MAX; i++) {
					message_rx[i] = '\0';
				}
				// Check for winner
				winner = 0;
				for (i = 0; i < MAXP; i++) {
					if (players[i] == 2)
						winner++;
				}

			}
			// Check to see if everyone has lost
			alllosers = 0;
			for (i = 0; i < MAXP; i++) {
				if (players[i] == 1)
					alllosers = 1;
				else if (players[i] == 2) {
					alllosers = 0;
					break;
				}
			}

			// Re-assign turn to a random valid player
			do {
				specialid = rand() % 20;
			} while (players[specialid] != 2 && totalplayers != 0
					&& alllosers != 1);
			if (totalplayers == 0 || winner == 1 || alllosers == 1)
				specialid = actualid;
		}
		// Reset game if there is a winner
		if (winner == 1) {
			for (i = 0; i < MAX; i++) {
				message_rx[i] = '\0';
				correct_message[i] = '\0';
			}
			moves = 0;
			for (i = 0; i < MAXP; i++) {
				if (players[i] == 2)
					player_win[0] = i;
			}
			// Tell the person they won
			player_win[1] = (unsigned char) seven;
			usb_device_send(player_win, player_win_length + 2);

			// Put everyone back in the game
			winner = 0;
			for (i = 0; i < MAXP; i++) {
				if (players[i] == 1)
					players[i] = 2;
			}
		}

		// Tell the random person it's their turn
		turn[1] = (unsigned char) nine;
		turn[0] = specialid;
		usb_device_send(turn, turnlength + 2);

		for (i = 0; i < 20; i++) {
			printf(" %i,", correct_message[i]);
		}
	}
	printf("\n");
	printf("Message Echo Complete\n");

	return 0;
}
